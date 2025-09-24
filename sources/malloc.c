/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/17 11:21:08 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 15:26:32 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

t_zone *g_zones = NULL;

static t_zone_type classify(size_t size)
{
	if (size <= TINY_MAX)
		return ZONE_TINY;
	if (size <= SMALL_MAX)
		return ZONE_SMALL;
	return ZONE_LARGE;
}

static size_t pagesize(void)
{
	static size_t ps=0;
#ifdef __APPLE__
	ps = (size_t)getpagesize();
#else
	ps = (size_t)sysconf(_SC_PAGESIZE);
#endif
	return ps;
}

static size_t zone_allocation_size(t_zone_type t, size_t request)
{
	size_t ps = pagesize();
	if (t == ZONE_LARGE)
	{
		size_t need = ALIGN_UP(sizeof(t_zone) + sizeof(t_block) + request, ps);
		return need;
	}
	// Aim for >=100 blocks of max size for TINY/SMALL
	size_t max_block = (t==ZONE_TINY) ? TINY_MAX : SMALL_MAX;
	size_t one = sizeof(t_block) + max_block;
	size_t target = one * 100 + sizeof(t_zone);
	return ALIGN_UP(target, ps);
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
	// Insert at list head for simplicity (could keep type ordering later)
	z->next = g_zones; g_zones = z;
	return z;
}

static void *block_payload(t_block *b)
{
	return (void*)((char*)b + sizeof(t_block));
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
		nb->magic = 0xB10C0DEU;
		nb->size = old_size - needed - sizeof(t_block);
		nb->requested = 0;
		nb->free = 1;
		nb->prev = b;
		nb->next = b->next;
		nb->bin_next = nb->bin_prev = NULL;
		if (b->next) b->next->prev = nb;
		b->next = nb;
		// Link into zone list if needed
		malloc_bin_insert(nb);
	}
	(void)z;
}

static t_block *append_block(t_zone *z, size_t size, size_t requested)
{
	// Walk to find end and leftover space
	char *zone_start = (char*)z + z->data_offset;
	// Compute used span by traversing blocks
	t_block *last = NULL;
	for (t_block *b = z->blocks; b; b = b->next)
		last = b;
	size_t used_bytes = 0;
	if (z->blocks) // compute end of last block
		used_bytes = ((char*)last + sizeof(t_block) + last->size) - zone_start;
	size_t needed = sizeof(t_block) + size;
	if (used_bytes + needed > z->capacity)
		return NULL;
	t_block *b = (t_block*)(zone_start + used_bytes);
	b->magic = 0xB10C0DEU;
	b->size = size;
	b->requested = requested;
	b->free = 0;
	b->prev = last;
	b->next = NULL;
	if (!z->blocks)
		z->blocks = b;
	else
		last->next = b;
	z->used += size;
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
		t_block *b = z->blocks;
		b->magic = 0xB10C0DEU;
		b->size = aligned;
		b->requested = requested;
		b->free = 0;
		b->prev = NULL;
		b->next = NULL;
		return b;
	}
	// Try bins first (only for non-large)
	t_block *reuse = malloc_bin_take(aligned);
	if (reuse) 
	{
		reuse->requested = requested;
		// zone->used accounted: we add aligned size; it was previously free so used did not include it
		// Need owner zone to increase used; find via linear zone search
		for (t_zone *oz = g_zones; oz; oz = oz->next)
		{
			char *zs = (char*)oz + oz->data_offset;
			char *ze = zs + oz->capacity;
			if ((char*)reuse >= zs && (char*)reuse < ze)
			{
				oz->used += aligned;
				break;
			}
		}
		split_block_if_large(NULL, reuse, aligned);
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