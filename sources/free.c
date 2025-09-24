/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/17 11:21:03 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 16:36:04 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

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
	// Find its zone (linear search; can optimize later)
	t_zone *z = g_zones;
	t_zone *owner = NULL;
	while (z)
	{
		if (block_in_zone(z, b))
		{
			owner = z;
			break;
		}
		z = z->next;
	}
	if (!owner) // Pointer not recognized; undefined per spec, ignore safely.
	{   malloc_unlock(); return; }
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
	b = coalesce_block(b);
	b->bin_next = b->bin_prev = NULL;
	malloc_bin_insert(b);
	malloc_unlock();
}
