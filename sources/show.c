/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/18 14:08:43 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/27 13:57:21 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#ifndef MAP_ANONYMOUS
#ifdef MAP_ANON
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

// ---------------- Snapshot Structures ----------------
typedef struct s_range
{
	void *start;
	void *end;
	size_t size;
} t_range;

typedef struct s_type_snapshot
{
	const char *label;
	t_zone_type type;
	t_zone **zones; // zone pointer array (addresses at snapshot time)
	size_t zone_count;
	t_range *allocs; // allocated (in-use) block payload ranges
	size_t alloc_count;
	t_range *frees; // free block payload ranges
	size_t free_count;
	size_t used_sum;	 // sum of zone->used at snapshot
	size_t capacity_sum; // sum of zone->capacity at snapshot
} t_type_snapshot;

// ---------------- Local helpers (no allocation after unlock) ----------------
static int ptr_cmp(const void *a, const void *b)
{
	const t_zone *za = *(const t_zone *const *)a;
	const t_zone *zb = *(const t_zone *const *)b;
	if (za < zb)
		return -1;
	if (za > zb)
		return 1;
	return 0;
}

static int range_cmp(const void *a, const void *b)
{
	const t_range *ra = (const t_range *)a;
	const t_range *rb = (const t_range *)b;
	if (ra->start < rb->start)
		return -1;
	if (ra->start > rb->start)
		return 1;
	return 0;
}

// mmap wrapper that never calls malloc (keeps us reentrancy-safe while holding lock)
static void *snap_alloc(size_t bytes)
{
	if (!bytes)
		return NULL;
#ifdef MAP_ANONYMOUS
	void *p = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#else
#ifdef MAP_ANON
	void *p = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#else
	void *p = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0); // fallback
#endif
#endif
	if (p == MAP_FAILED)
		return NULL;
	return p;
}

static void free_snapshot_arrays(t_type_snapshot *s)
{
	if (!s)
		return;
	if (s->zones)
		munmap(s->zones, s->zone_count * sizeof(t_zone *));
	if (s->allocs)
		munmap(s->allocs, s->alloc_count * sizeof(t_range));
	if (s->frees)
		munmap(s->frees, s->free_count * sizeof(t_range));
	// zero fields (not strictly required)
	*s = (t_type_snapshot){0};
}

// Collect snapshot for a given type while allocator lock is held.
static void snapshot_type(t_type_snapshot *out, t_zone_type type, const char *label, int want_free)
{
	*out = (t_type_snapshot){.label = label, .type = type};
	// Count zones
	size_t zc = 0;
	for (t_zone *z = g_zones; z; z = z->next)
		if (z->type == type)
			zc++;
	if (!zc)
		return; // leave empty snapshot
	out->zones = (t_zone **)snap_alloc(zc * sizeof(t_zone *));
	if (!out->zones)
		return; // allocation failure => skip snapshot
	// Fill zone pointer array
	size_t zi = 0;
	for (t_zone *z = g_zones; z; z = z->next)
		if (z->type == type)
			out->zones[zi++] = z;
	out->zone_count = zc;
	if (zc > 1)
		qsort(out->zones, zc, sizeof(t_zone *), ptr_cmp);
	// First pass: count blocks
	size_t alloc_cnt = 0, free_cnt = 0;
	size_t used_sum = 0, cap_sum = 0;
	for (size_t i = 0; i < zc; ++i)
	{
		used_sum += out->zones[i]->used;
		cap_sum += out->zones[i]->capacity;
		for (t_block *b = out->zones[i]->blocks; b; b = b->next)
		{
			if (!b->free)
				alloc_cnt++;
			else if (want_free)
				free_cnt++;
		}
	}
	out->used_sum = used_sum;
	out->capacity_sum = cap_sum;
	if (alloc_cnt)
		out->allocs = (t_range *)snap_alloc(alloc_cnt * sizeof(t_range));
	if (want_free && free_cnt)
		out->frees = (t_range *)snap_alloc(free_cnt * sizeof(t_range));
	if ((alloc_cnt && !out->allocs) || (free_cnt && want_free && !out->frees))
		return; // partial failure; treat as empty (arrays may be NULL)
	// Second pass: populate ranges
	size_t ai = 0, fi = 0;
	for (size_t i = 0; i < zc; ++i)
	{
		for (t_block *b = out->zones[i]->blocks; b; b = b->next)
		{
			void *start = (char *)b + sizeof(t_block);
			if (!b->free && out->allocs)
			{
				out->allocs[ai].start = start;
				out->allocs[ai].end = (char *)start + b->size;
				out->allocs[ai].size = b->size;
				ai++;
			}
			else if (want_free && b->free && out->frees)
			{
				out->frees[fi].start = start;
				out->frees[fi].end = (char *)start + b->size;
				out->frees[fi].size = b->size;
				fi++;
			}
		}
	}
	out->alloc_count = alloc_cnt;
	out->free_count = free_cnt;
	if (out->allocs && alloc_cnt > 1)
		qsort(out->allocs, alloc_cnt, sizeof(t_range), range_cmp);
	if (out->frees && free_cnt > 1)
		qsort(out->frees, free_cnt, sizeof(t_range), range_cmp);
}

// Print a snapshot (no allocator lock held) WITHOUT colors.
static void print_snapshot(const t_type_snapshot *s, int show_stats, int show_free, size_t *ptotal)
{
	if (!s || !s->zone_count)
		return;
	printf("%s : %p\n", s->label, (void *)s->zones[0]);
	if (show_stats)
	{
		size_t free_bytes = (s->capacity_sum >= s->used_sum) ? (s->capacity_sum - s->used_sum) : 0;
		printf("# stats: zones=%zu used=%zu capacity=%zu free=%zu\n", s->zone_count, s->used_sum, s->capacity_sum, free_bytes);
	}
	for (size_t i = 0; i < s->alloc_count && s->allocs; ++i)
	{
		printf("%p - %p : %zu bytes\n", s->allocs[i].start, s->allocs[i].end, s->allocs[i].size);
		*ptotal += s->allocs[i].size;
	}
	if (show_free && s->frees)
	{
		for (size_t i = 0; i < s->free_count; ++i)
			printf("FREE %p - %p : %zu bytes\n", s->frees[i].start, s->frees[i].end, s->frees[i].size);
	}
}

void show_alloc_mem()
{
	int show_stats = (getenv("MALLOC_SHOW_STATS") != NULL);
	int show_free = (getenv("MALLOC_SHOW_FREE") != NULL);

	malloc_lock();
	t_type_snapshot snaps[3];
	snapshot_type(&snaps[0], ZONE_TINY, "TINY", show_free);
	snapshot_type(&snaps[1], ZONE_SMALL, "SMALL", show_free);
	snapshot_type(&snaps[2], ZONE_LARGE, "LARGE", show_free);
	malloc_unlock();

	size_t total = 0;
	print_snapshot(&snaps[0], show_stats, show_free, &total);
	print_snapshot(&snaps[1], show_stats, show_free, &total);
	print_snapshot(&snaps[2], show_stats, show_free, &total);
	printf("Total : %zu bytes\n", total);

	free_snapshot_arrays(&snaps[0]);
	free_snapshot_arrays(&snaps[1]);
	free_snapshot_arrays(&snaps[2]);
}
