/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bins.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/19 14:10:19 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/27 13:02:41 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc_bin.h"
#include <sys/mman.h>
#ifndef MAP_ANONYMOUS
# ifdef MAP_ANON
#  define MAP_ANONYMOUS MAP_ANON
# endif
#endif
#include <unistd.h>
#include <stdlib.h>

// Simple segregated free lists for sizes up to SMALL_MAX.
// Bin size class granularity = MALLOC_ALIGN (16).
// Index formula: (size / MALLOC_ALIGN) - 1 capped.

#ifndef BIN_GRANULARITY
#define BIN_GRANULARITY MALLOC_ALIGN
#endif

// We eliminate separate global variables for bins by attaching bin metadata
// (array pointer + count) to the first allocated zone (g_zones head). The
// helper accessors below fetch or lazily initialize these fields.

static inline t_block ***bins_array_ref(void) { return g_zones ? &g_zones->bins : NULL; }
static inline size_t *bins_count_ref(void) { return g_zones ? &g_zones->bin_count : NULL; }

static void bins_init(void)
{
	if (!g_zones)
		return; // need at least one zone before we can host bins metadata
	if (g_zones->bins)
		return;
	size_t small_max = SMALL_MAX; // runtime value
	size_t count = (small_max / BIN_GRANULARITY) + 1;
	size_t bytes = count * sizeof(t_block *);
	t_block **arr = (t_block **)mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (arr == MAP_FAILED)
		arr = NULL;
	if (!arr)
	{
		arr = (t_block **)calloc(count, sizeof(t_block *));
		if (!arr)
		{
			g_zones->bin_count = 0;
			return; // disable bins
		}
	}
	// mmap / calloc both zero initialised. Store metadata.
	g_zones->bins = arr;
	g_zones->bin_count = count;
}

static inline int block_in_any_zone(t_block *b)
{
	if (!b)
		return 0;
	if (b->zone)
	{
		char *zs = (char *)b->zone + b->zone->data_offset;
		char *ze = zs + b->zone->capacity;
		if ((char *)b >= zs && (char *)b + (ptrdiff_t)sizeof(t_block) <= ze)
			return 1;
	}
	for (t_zone *z = g_zones; z; z = z->next)
	{
		char *zs = (char *)z + z->data_offset;
		char *ze = zs + z->capacity;
		if ((char *)b >= zs && (char *)b + (ptrdiff_t)sizeof(t_block) <= ze)
		{
			b->zone = z;
			return 1;
		}
	}
	return 0;
}

static inline size_t clamp_index(size_t idx)
{
	if (!g_zones || !g_zones->bins || g_zones->bin_count == 0)
		return 0;
	if (idx >= g_zones->bin_count)
		return g_zones->bin_count - 1;
	return idx;
}

static inline size_t bin_index(size_t size)
{
	size_t aligned = ALIGN_UP(size, BIN_GRANULARITY);
	size_t idx = (aligned / BIN_GRANULARITY);
	if (idx)
		idx -= 1; // size in [16] => idx 0
	return clamp_index(idx);
}

void malloc_bin_insert(t_block *b)
{
	if (!b || !b->free)
		return;
	if (!block_in_any_zone(b))
		return;
	bins_init();
	if (!g_zones || !g_zones->bins)
		return;
	size_t idx = bin_index(b->size);
	b->bin_prev = NULL;
	b->bin_next = g_zones->bins[idx];
	if (g_zones->bins[idx])
		g_zones->bins[idx]->bin_prev = b;
	g_zones->bins[idx] = b;
}

static void bin_detach(t_block *b)
{
	if (!b || !g_zones || !g_zones->bins)
		return;
	if (b->bin_prev)
		b->bin_prev->bin_next = b->bin_next;
	else
	{
		size_t idx = bin_index(b->size);
		if (g_zones->bins[idx] == b)
			g_zones->bins[idx] = b->bin_next;
	}
	if (b->bin_next)
		b->bin_next->bin_prev = b->bin_prev;
	b->bin_next = b->bin_prev = NULL;
}

t_block *malloc_bin_take(size_t size, t_zone_type want_type)
{
	bins_init();
	if (!g_zones || !g_zones->bins)
		return NULL;
	size_t idx = bin_index(size);
	for (size_t i = idx; i < g_zones->bin_count; ++i)
	{
		t_block *b = g_zones->bins[i];
		while (b)
		{
			t_block *next = b->bin_next;
			if (!block_in_any_zone(b))
			{
				if (b->bin_prev)
					b->bin_prev->bin_next = b->bin_next;
				else if (g_zones->bins[i] == b)
					g_zones->bins[i] = b->bin_next;
				if (b->bin_next)
					b->bin_next->bin_prev = b->bin_prev;
				b->bin_next = b->bin_prev = NULL;
				b = next;
				continue;
			}
			// Enforce zone type match; skip mismatched bins
			if (b->free && b->size >= size && b->zone && b->zone->type == want_type)
			{
				bin_detach(b);
				b->free = 0;
				return b;
			}
			b = next;
		}
	}
	return NULL;
}

void malloc_bin_remove(t_block *b)
{
	if (!b)
		return;
	if (!block_in_any_zone(b))
		return;
	if (b->free)
	{
		bins_init();
		if (g_zones && g_zones->bins)
			bin_detach(b);
	}
}
