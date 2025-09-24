/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/18 14:08:43 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 18:42:52 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"
#include <stdlib.h>
#include <stdio.h>

#define CLR_RESET "\033[0m"
#define CLR_TINY  "\033[36m"  /* cyan */
#define CLR_SMALL "\033[33m"  /* yellow */
#define CLR_LARGE "\033[35m"  /* magenta */
#define CLR_STAT  "\033[90m"  /* bright black */
#define CLR_FREE  "\033[32m"  /* green */

typedef struct s_range
{
	void *start;
	void *end;
	size_t size;
} t_range;

static int ptr_cmp(const void *a, const void *b)
{
	const t_zone *za = *(const t_zone* const*)a;
	const t_zone *zb = *(const t_zone* const*)b;
	if (za < zb)
		return -1;
	if (za > zb)
		return 1;
	return 0;
}

static int range_cmp(const void *a, const void *b)
{
	const t_range *ra = (const t_range*)a;
	const t_range *rb = (const t_range*)b;
	if (ra->start < rb->start)
		return -1;
	if (ra->start > rb->start)
		return 1;
	return 0;
}

static void collect_and_print_type(t_zone_type type, const char *label, size_t *ptotal,
	int show_stats, int show_free, int use_color)
{
	// First pass: count zones of this type
	size_t zone_count = 0;
	for (t_zone *z = g_zones; z; z = z->next)
		if (z->type == type)
			zone_count++;
	if (!zone_count)
		return;
	t_zone **zones = (t_zone**)mmap(NULL, zone_count * sizeof(t_zone*),
		PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (zones == MAP_FAILED)
		return; // fall back silently
	size_t idx=0;
	for (t_zone *z = g_zones; z; z = z->next)
		if (z->type == type)
			zones[idx++] = z;
	// sort zones by address
	if (zone_count > 1)
		qsort(zones, zone_count, sizeof(t_zone*), ptr_cmp);
	// Count allocated blocks and free blocks (free blocks only if show_free requested)
	size_t block_count = 0;
	size_t free_block_count = 0;
	size_t used_sum = 0; // sum of zone->used for stats (should equal allocated bytes)
	for (size_t i = 0; i < zone_count; ++i)
	{
		used_sum += zones[i]->used;
		for (t_block *b = zones[i]->blocks; b; b = b->next)
		{
			if (!b->free)
				block_count++;
			else if (show_free)
				free_block_count++;
		}
	}
	t_range *ranges = NULL;
	t_range *free_ranges = NULL;
	if (block_count)
	{
		ranges = (t_range*)mmap(NULL, block_count * sizeof(t_range),
			PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if (ranges == MAP_FAILED)
			ranges = NULL;
	}
	if (show_free && free_block_count)
	{
		free_ranges = (t_range*)mmap(NULL, free_block_count * sizeof(t_range),
			PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if (free_ranges == MAP_FAILED)
			free_ranges = NULL;
	}
	size_t r=0, fr=0;
	for (size_t i=0;i<zone_count;++i)
	{
		for (t_block *b = zones[i]->blocks; b; b = b->next)
		{
			void *start = (char*)b + sizeof(t_block);
			if (!b->free && ranges)
			{
				ranges[r].start = start;
				ranges[r].end = (char*)start + b->size;
				ranges[r].size = b->size;
				r++;
			} else if (b->free && free_ranges)
			{
				free_ranges[fr].start = start;
				free_ranges[fr].end = (char*)start + b->size;
				free_ranges[fr].size = b->size;
				fr++;
			}
		}
	}
	if (ranges && block_count > 1)
		qsort(ranges, block_count, sizeof(t_range), range_cmp);
	if (free_ranges && free_block_count > 1)
		qsort(free_ranges, free_block_count, sizeof(t_range), range_cmp);
	// Print header using first zone address
	const char *c = "";
	if (use_color)
	{
		if (type == ZONE_TINY)
			c = CLR_TINY;
		else if (type == ZONE_SMALL)
			c = CLR_SMALL;
		else
			c = CLR_LARGE;
	}
	if (use_color)
		printf("%s%s : %p%s\n", c, label, (void*)zones[0], CLR_RESET);
	else
		printf("%s : %p\n", label, (void*)zones[0]);
	if (show_stats)
	{
		size_t cap_sum = 0;
		for (size_t i = 0; i < zone_count; ++i)
			cap_sum += zones[i]->capacity;
		size_t free_bytes = (cap_sum >= used_sum) ? (cap_sum - used_sum) : 0;
		if (use_color)
			printf("%s# stats: zones=%zu used=%zu capacity=%zu free=%zu%s\n", CLR_STAT, zone_count, used_sum, cap_sum, free_bytes, CLR_RESET);
		else
			printf("# stats: zones=%zu used=%zu capacity=%zu free=%zu\n", zone_count, used_sum, cap_sum, free_bytes);
	}
	for (size_t i=0;i<block_count && ranges; ++i)
	{
		printf("%p - %p : %zu bytes\n", ranges[i].start, ranges[i].end, ranges[i].size);
		*ptotal += ranges[i].size;
	}
	if (show_free && free_ranges)
	{
		for (size_t i=0;i<free_block_count; ++i)
		{
			if (use_color)
				printf("%sFREE %p - %p : %zu bytes%s\n", CLR_FREE, free_ranges[i].start, free_ranges[i].end, free_ranges[i].size, CLR_RESET);
			else
				printf("FREE %p - %p : %zu bytes\n", free_ranges[i].start, free_ranges[i].end, free_ranges[i].size);
		}
	}
	if (free_ranges)
		munmap(free_ranges, free_block_count * sizeof(t_range));
	if (ranges)
		munmap(ranges, block_count * sizeof(t_range));
	munmap(zones, zone_count * sizeof(t_zone*));
}

void show_alloc_mem()
{
	malloc_lock();
	size_t total = 0;
	int show_stats = (getenv("MALLOC_SHOW_STATS") != NULL);
	int show_free  = (getenv("MALLOC_SHOW_FREE")  != NULL);
	int use_color  = (getenv("MALLOC_COLOR")      != NULL);
	collect_and_print_type(ZONE_TINY,  "TINY",  &total, show_stats, show_free, use_color);
	collect_and_print_type(ZONE_SMALL, "SMALL", &total, show_stats, show_free, use_color);
	collect_and_print_type(ZONE_LARGE, "LARGE", &total, show_stats, show_free, use_color);
	if (use_color)
		printf("%sTotal : %zu bytes%s\n", CLR_STAT, total, CLR_RESET);
	else
		printf("Total : %zu bytes\n", total);
	malloc_unlock();
}
