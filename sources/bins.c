/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bins.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/19 14:10:19 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 15:24:40 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc_bin.h"

// Simple segregated free lists for sizes up to SMALL_MAX.
// Bin size class granularity = MALLOC_ALIGN (16).
// Index formula: (size / MALLOC_ALIGN) - 1 capped.

#ifndef BIN_GRANULARITY
#define BIN_GRANULARITY MALLOC_ALIGN
#endif

#define BIN_COUNT ((SMALL_MAX / BIN_GRANULARITY) + 1)

static t_block *g_bins[BIN_COUNT];

static inline int block_in_any_zone(t_block *b)
{
    if (!b)
        return 0;
    for (t_zone *z = g_zones; z; z = z->next)
    {
        char *zs = (char*)z + z->data_offset;
        char *ze = zs + z->capacity;
        if ((char*)b >= zs && (char*)b + (ptrdiff_t)sizeof(t_block) <= ze)
            return 1;
    }
    return 0;
}

static inline size_t clamp_index(size_t idx)
{
    if (idx >= BIN_COUNT)
        return BIN_COUNT - 1;
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
    if (b->magic != 0xB10C0DEU || !block_in_any_zone(b))
        return;
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
    size_t idx = bin_index(size);
    // Search idx and upwards for first non-empty bin.
    for (size_t i = idx; i < BIN_COUNT; ++i)
    {
        t_block *b = g_bins[i];
        while (b)
        { // first-fit inside bin
            t_block *next = b->bin_next; // save next in case we detach
            // Validate node; if corrupted or foreign, unlink from this bin
            if (!block_in_any_zone(b) || b->magic != 0xB10C0DEU)
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
    if (b && b->free)
        bin_detach(b);
}
