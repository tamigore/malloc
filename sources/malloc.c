/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/17 11:21:08 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/26 11:58:35 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

t_zone *g_zones = NULL;

static t_zone_type classify(size_t size)
{
    size_t tiny = TINY_MAX;
    size_t small = SMALL_MAX;
    if (size <= tiny)
		return ZONE_TINY;
    if (size <= small)
		return ZONE_SMALL;
    return ZONE_LARGE;
}

static size_t pagesize(void)
{
    static size_t ps = 0;
    if (ps == 0)
    {
#ifdef __APPLE__
		ps = (size_t)getpagesize();
#else
		ps = (size_t)sysconf(_SC_PAGESIZE);
#endif
		if (ps == 0)
			ps = 4096; // fallback
    }
    return ps;
}

// Decide dynamic thresholds as page-size multiples.
// Strategy:
//  - Base tiny target = 128 (historical). Round up to next divisor of page size such that
//    at least 64 tiny blocks fit into one multi-page zone allocation.
//  - Base small target = 4096 (historical). Ensure small_max >= 4 * tiny_max and is a
//    multiple of page size.
// We cache results after first computation.
static size_t g_tiny_max = 0;
static size_t g_small_max = 0;

size_t malloc_tiny_max(void)
{
	if (g_tiny_max)
		return g_tiny_max;
	size_t ps = pagesize();
	size_t base = 128UL;
	// Ensure multiple of page size but not exceeding half a page to keep density high.
	// If page size < base fallback to base aligned to 16.
	if (ps <= base)
	{
		g_tiny_max = ALIGN_UP(base, MALLOC_ALIGN);
		return g_tiny_max;
	}
	// Choose a divisor of page size near base: try ps/ (ps/base rounded)
	size_t blocks_per_page_target = ps / base; // e.g., 4096/128 = 32
	if (blocks_per_page_target == 0)
		blocks_per_page_target = 1;
	size_t candidate = ps / blocks_per_page_target; // 4096/32 = 128
	// Round candidate up to alignment
	candidate = ALIGN_UP(candidate, MALLOC_ALIGN);
	// Guarantee candidate divides page size (so N * candidate = page size)
	while (ps % candidate != 0)
		candidate += MALLOC_ALIGN;
	g_tiny_max = candidate;
	return g_tiny_max;
}

size_t malloc_small_max(void)
{
	if (g_small_max)
		return g_small_max;
	size_t ps = pagesize();
	size_t tiny = malloc_tiny_max();
	size_t base = 4096UL;
	// Ensure small_max at least 4 * tiny and a multiple of page size.
	size_t min_small = tiny * 4;
	if (base < min_small)
		base = min_small;
	// Round base up to page size multiple.
	size_t candidate = ALIGN_UP(base, ps);
	g_small_max = candidate;
	return g_small_max;
}

static size_t zone_allocation_size(t_zone_type t, size_t request)
{
    size_t ps = pagesize();
    if (t == ZONE_LARGE)
    {
        size_t need = ALIGN_UP(sizeof(t_zone) + sizeof(t_block) + request, ps);
        return need;
    }
    size_t max_block = (t == ZONE_TINY) ? TINY_MAX : SMALL_MAX;
    size_t one = sizeof(t_block) + max_block;
    // Keep zone size a multiple of page size large enough for ~100 blocks
    size_t raw = one * 100 + sizeof(t_zone);
    return ALIGN_UP(raw, ps);
}

static t_zone *create_zone(t_zone_type t, size_t request)
{
	size_t alloc = zone_allocation_size(t, request);
	void *mem = mmap(NULL, alloc, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (mem == MAP_FAILED)
		return NULL;
	t_zone *z = (t_zone*)mem;
	z->type = t;
	// Align data region start
	size_t raw_off = sizeof(t_zone);
	size_t aligned_off = ALIGN_UP(raw_off, MALLOC_ALIGN);
	z->data_offset = aligned_off;
	z->capacity = alloc - aligned_off;
	z->used = 0;
	z->next = NULL;
	z->blocks = NULL;
	z->tail = NULL;
	// Insert at list head for simplicity (could keep type ordering later)
	z->next = g_zones; g_zones = z;
	return z;
}

static void *block_payload(t_block *b)
{
	return (void*)((char*)b + sizeof(t_block));
}

static inline int block_in_zone_local(t_zone *z, t_block *b)
{
    char *zs = (char*)z + z->data_offset;
    char *ze = zs + z->capacity;
    return ((char*)b >= zs && (char*)b + (ptrdiff_t)sizeof(t_block) <= ze);
}

static void split_block_if_large(t_zone *z, t_block *b, size_t needed)
{
	// needed is aligned payload size. If block->size is significantly larger, split.
	size_t excess = b->size - needed;
	size_t min_split = sizeof(t_block) + MALLOC_ALIGN; // reserve room for header + minimal payload
	if (excess >= min_split) {
		// New block starts after allocated portion
		char *base = (char*)b;
		size_t old_size = b->size;
		b->size = needed;
		t_block *nb = (t_block*)(base + sizeof(t_block) + needed);
		nb->size = old_size - needed - sizeof(t_block);
		nb->requested = 0;
		nb->free = 1;
		nb->prev = b;
		nb->next = b->next;
		nb->bin_next = nb->bin_prev = NULL;
		if (b->next) b->next->prev = nb;
		b->next = nb;
		if (!z)
			z = b->zone; // use back-pointer if caller passed NULL
		nb->zone = z;
		if (z && z->tail == b)
			z->tail = nb;
		malloc_bin_insert(nb);
	}
}

static t_block *append_block(t_zone *z, size_t size, size_t requested)
{
    char *zone_start = (char*)z + z->data_offset;
	char *zone_limit = zone_start + z->capacity;
	char *insert;
	if (!z->tail)
		insert = zone_start;
	else
		insert = (char*)z->tail + sizeof(t_block) + z->tail->size;
	if (insert + (ptrdiff_t)(sizeof(t_block) + size) > zone_limit)
		return NULL;
	t_block *b = (t_block*)insert;
    b->size = size;
    b->requested = requested;
	b->free = 0;
    b->prev = z->tail;
    b->next = NULL;
    if (!z->blocks)
        z->blocks = b;
    if (z->tail)
        z->tail->next = b;
    z->tail = b;
    z->used += size;
	/* span removed */
	b->bin_next = b->bin_prev = NULL;
	b->zone = z;
    return b;
}

static t_block *alloc_from_zone(t_zone *z, size_t size, size_t requested)
{
    // Fallback: append new block at zone end (bins searched globally before calling this)
    t_block *b = append_block(z, size, requested);
    return b;
}

static t_block *allocate(size_t requested)
{
	size_t aligned = ALIGN_UP(requested, MALLOC_ALIGN);
	t_zone_type t = classify(aligned);
	if (t == ZONE_LARGE)
	{
		size_t alloc = sizeof(t_zone)+sizeof(t_block)+aligned;
		size_t ps = pagesize();
		alloc = ALIGN_UP(alloc, ps);
		void *mem = mmap(NULL, alloc, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if (mem == MAP_FAILED)
			return NULL;
		t_zone *z = (t_zone*)mem;
		z->type = ZONE_LARGE;
		// Large zone: header followed by aligned data region
		size_t raw_off = sizeof(t_zone);
		size_t aligned_off = ALIGN_UP(raw_off, MALLOC_ALIGN);
		z->data_offset = aligned_off;
		z->capacity = alloc - aligned_off;
		z->used = aligned;
		z->next = g_zones;
		g_zones = z;
		z->blocks = (t_block*)((char*)z + z->data_offset);
		z->tail = z->blocks;
		t_block *b = z->blocks;
		b->size = aligned;
		b->requested = requested;
		b->free = 0;
		b->prev = NULL;
		b->next = NULL;
		   b->bin_next = b->bin_prev = NULL;
		   b->zone = z;
		return b;
	}
	// Try bins first (only for non-large)
	t_block *reuse = malloc_bin_take(aligned);
	if (reuse) 
	{
		reuse->requested = requested;
		// zone->used accounted using back-pointer; block came from a free bin
		if (reuse->zone)
			reuse->zone->used += aligned;
		split_block_if_large(reuse->zone, reuse, aligned);
		return reuse;
	}
	// find existing zone of that type with space (append path)
	for (t_zone *z = g_zones; z; z = z->next)
	{
		if (z->type == t)
		{
			t_block *b = alloc_from_zone(z, aligned, requested);
			if (b)
			{
				split_block_if_large(z, b, aligned);
				return b;
			}
		}
	}
	t_zone *z = create_zone(t, aligned);
	if (!z)
		return NULL;
	t_block *nb = alloc_from_zone(z, aligned, requested);
	if (nb)
		split_block_if_large(z, nb, aligned);
	return nb;
}

void *malloc(size_t size)
{
	malloc_lock();
	if (size==0)
		size=1; // ANSI permits
	// Overflow guard: ensure size plus headers won't wrap
	if (size > (size_t)-1 / 2)
	{
		malloc_unlock();
		return NULL;
	}
	t_block *b = allocate(size);
	if (!b)
	{
		malloc_unlock();
		return NULL;
	}
	void *p = block_payload(b);
	// Alignment should already be guaranteed by header alignment + size alignment.
	// If this assert pattern triggers in future debugging, revisit header struct.
	// (No pointer shifting here to keep ptr_to_block subtraction valid.)
	malloc_unlock();
	return p;
}