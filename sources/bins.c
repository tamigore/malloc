/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bins.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/19 14:10:19 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/26 11:26:05 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc_bin.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

// Simple segregated free lists for sizes up to SMALL_MAX.
// Bin size class granularity = MALLOC_ALIGN (16).
// Index formula: (size / MALLOC_ALIGN) - 1 capped.

#ifndef BIN_GRANULARITY
#define BIN_GRANULARITY MALLOC_ALIGN
#endif

// Dynamic bin array sized on first use based on current SMALL_MAX.
static t_block **g_bins = NULL;
static size_t g_bin_count = 0;

static void bins_init(void)
{
    if (g_bins)
        return;
    size_t small_max = SMALL_MAX; // runtime value
    g_bin_count = (small_max / BIN_GRANULARITY) + 1;
    // mmap for simplicity; zero-initialized
    size_t bytes = g_bin_count * sizeof(t_block*);
    g_bins = (t_block**)mmap(NULL, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (g_bins == MAP_FAILED) {
        g_bins = NULL; // fallback to heap allocation (last resort)
    }
    if (!g_bins) {
        g_bins = (t_block**)calloc(g_bin_count, sizeof(t_block*));
        if (!g_bins) {
            g_bin_count = 0; // disable bins
        }
    }
}

static inline int block_in_any_zone(t_block *b)
{
    if (!b) return 0;
    if (b->zone) {
        char *zs = (char*)b->zone + b->zone->data_offset;
        char *ze = zs + b->zone->capacity;
        if ((char*)b >= zs && (char*)b + (ptrdiff_t)sizeof(t_block) <= ze)
            return 1;
    }
    for (t_zone *z = g_zones; z; z = z->next) {
        char *zs = (char*)z + z->data_offset;
        char *ze = zs + z->capacity;
        if ((char*)b >= zs && (char*)b + (ptrdiff_t)sizeof(t_block) <= ze) { b->zone = z; return 1; }
    }
    return 0;
}

static inline size_t clamp_index(size_t idx)
{
    if (!g_bins || g_bin_count == 0)
        return 0;
    if (idx >= g_bin_count)
        return g_bin_count - 1;
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
    // Defensive: ensure the block header looks sane and belongs to us
    if (!block_in_any_zone(b))
        return;
    bins_init();
    if (!g_bins) return;
    size_t idx = bin_index(b->size);
    b->bin_prev = NULL;
    b->bin_next = g_bins[idx];
    if (g_bins[idx])
        g_bins[idx]->bin_prev = b;
    g_bins[idx] = b;
}

static void bin_detach(t_block *b)
{
    if (!b)
        return;
    if (b->bin_prev)
        b->bin_prev->bin_next = b->bin_next;
    else
    {
        size_t idx = bin_index(b->size);
        if (g_bins[idx] == b)
            g_bins[idx] = b->bin_next;
    }
    if (b->bin_next)
        b->bin_next->bin_prev = b->bin_prev;
    b->bin_next = b->bin_prev = NULL;
}

t_block *malloc_bin_take(size_t size)
{
    bins_init();
    if (!g_bins) return NULL;
    size_t idx = bin_index(size);
    for (size_t i = idx; i < g_bin_count; ++i)
    {
        t_block *b = g_bins[i];
        while (b)
        { // first-fit inside bin
            t_block *next = b->bin_next; // save next in case we detach
            // Validate node; if corrupted or foreign, unlink from this bin
            if (!block_in_any_zone(b))
            {
                if (b->bin_prev)
                    b->bin_prev->bin_next = b->bin_next;
                else if (g_bins[i] == b)
                    g_bins[i] = b->bin_next;
                if (b->bin_next)
                    b->bin_next->bin_prev = b->bin_prev;
                // clear links of the bad node to avoid re-walking
                b->bin_next = b->bin_prev = NULL;
                b = next;
                continue;
            }
            if (b->free && b->size >= size)
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
{ // explicit removal (e.g., before coalesce)
    if (!b)
        return;
    if (!block_in_any_zone(b))
        return; // ignore foreign/corrupt
    if (b->free) {
        bins_init();
        if (g_bins)
            bin_detach(b);
    }
}
