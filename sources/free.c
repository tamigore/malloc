/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/17 11:21:03 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 19:17:51 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"
#include <stdio.h>
#include <stdlib.h>

static int malloc_env_verify(void)
{
	static int init = 0; static int val = 0;
	if (!init) { init = 1; val = (getenv("MALLOC_DEBUG_VERIFY") != NULL); }
	return val;
}

static int zone_verify_chain(t_zone *z, int verbose)
{
	char *zone_start = (char*)z + z->data_offset;
	char *zone_end   = zone_start + z->capacity;
	t_block *prev = NULL;
	for (t_block *b = z->blocks; b; prev = b, b = b->next)
	{
		if (b->prev != prev) { if (verbose) fprintf(stderr,"[verify] prev mismatch %p (expected %p got %p)\n", (void*)b, (void*)prev, (void*)b->prev); return 0; }
		if ((char*)b < zone_start || (char*)b + (ptrdiff_t)sizeof(t_block) > zone_end) { if (verbose) fprintf(stderr,"[verify] header OOB %p\n", (void*)b); return 0; }
		char *payload_end = (char*)b + sizeof(t_block) + b->size;
		if (payload_end > zone_end) { if (verbose) fprintf(stderr,"[verify] payload OOB %p\n", (void*)b); return 0; }
		if (b->next && (char*)b->next <= (char*)b) { if (verbose) fprintf(stderr,"[verify] non-monotonic %p -> %p\n", (void*)b, (void*)b->next); return 0; }
	}
	return 1;
}

static void zone_recompute_layout(t_zone *z)
{
	if (!z) return;
	t_block *last = NULL;
	size_t safety = 0;
	char *zone_start = (char*)z + z->data_offset;
	char *zone_end = zone_start + z->capacity;
	for (t_block *b = z->blocks; b; b = b->next)
	{
		if (++safety > 100000) { fprintf(stderr,"[verify] abort: excessive blocks (loop?)\n"); break; }
		if ((char*)b < zone_start || (char*)b >= zone_end) { fprintf(stderr,"[verify] abort: block outside zone during recompute %p\n", (void*)b); break; }
		if (b->next && (char*)b->next <= (char*)b) { fprintf(stderr,"[verify] abort: non-monotonic next %p -> %p\n", (void*)b,(void*)b->next); break; }
		last = b;
	}
	z->tail = last;
	(void)zone_start; (void)zone_end; /* span removed */
	if (malloc_env_verify())
		zone_verify_chain(z, 1);
}

static int block_in_zone(t_zone *z, t_block *b)
{
	char *zs = (char*)z + z->data_offset;
	char *ze = (char*)z + z->data_offset + z->capacity;
	return ((char*)b >= zs && (char*)b < ze);
}

static t_block *coalesce_block(t_block *b)
{
	// Merge with next while next is free
	while (b->next && b->next->free)
	{
		t_block *n = b->next;
		malloc_bin_remove(n);
		b->size += sizeof(t_block) + n->size;
		b->next = n->next;
		if (n->next)
			n->next->prev = b;
	}
	// Merge backward if previous is free (then return previous as canonical)
	if (b->prev && b->prev->free)
	{
		t_block *p = b->prev;
		malloc_bin_remove(p); // remove previous from bin before enlarging
		p->size += sizeof(t_block) + b->size;
		p->next = b->next;
		if (b->next)
			b->next->prev = p;
		b = p;
		if (b->zone)
			b->zone = b->zone; // no-op, placeholder to emphasize retention
		// After merging backward, also continue merging forward (already done) but ensure loop effect
		while (b->next && b->next->free)
		{
			t_block *n = b->next;
			malloc_bin_remove(n);
			b->size += sizeof(t_block) + n->size;
			b->next = n->next;
			if (n->next)
				n->next->prev = b;
		}
	}
	return b;
}

void free(void *ptr)
{
	malloc_lock();
	if (!ptr)
	{
		malloc_unlock();
		return;
	}
    t_block *b = ptr_to_block(ptr);
    t_zone *owner = b->zone; // back-pointer (may be NULL if corruption)
    if (!owner || !block_in_zone(owner, b))
    {
        // Fallback: attempt to locate via linear scan only if zone pointer missing
        for (t_zone *z = g_zones; z; z = z->next) {
            if (block_in_zone(z, b)) { owner = z; break; }
        }
        if (!owner) { malloc_unlock(); return; }
        b->zone = owner; // repair if possible
    }
	// Validate header magic to avoid treating foreign pointers as ours
	if (b->magic != 0xB10C0DEU)
	{   malloc_unlock(); return; }
	if (b->free)
	{   malloc_unlock(); return; } // double free guard (silent)
	b->free = 1;
	if (owner->used >= b->size)
		owner->used -= b->size;
	if (owner->type == ZONE_LARGE)
	{
		// Unlink owner from g_zones and unmap entire mapping
		t_zone **pp = &g_zones;
		while (*pp && *pp != owner)
			pp = &(*pp)->next;
		if (*pp)
			*pp = owner->next;
		// Total mapping size originally requested when creating this large zone:
		// alloc = data_offset + capacity
		size_t total = owner->data_offset + owner->capacity;
		munmap(owner, total);
		malloc_unlock();
		return;
	}
	// Coalesce adjacent free blocks FIRST, then insert final merged block in bins.
	// Inserting before coalesce and then changing size corrupts bin lists
	// because bin_detach computes index from current size.
	// We need owner zone to maintain tail/span if end block merges.
	b = coalesce_block(b);
	// Ensure merged canonical block retains correct zone pointer
	b->zone = owner;
	// Recompute zone tail/span (also validates chain if enabled)
	zone_recompute_layout(owner);
	b->bin_next = b->bin_prev = NULL;
	malloc_bin_insert(b);
	malloc_unlock();
}
