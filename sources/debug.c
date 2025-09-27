/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/19 14:10:16 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/26 11:26:05 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

static t_block *ptr_to_block_internal(void *ptr)
{
	if (!ptr)
		return NULL;
	return (t_block *)((char *)ptr - sizeof(t_block));
}

static int header_in_our_zones(t_block *b)
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

// Conservative structural validation (no magic canary):
//  - header lies inside a known zone
//  - size is >= 0 and aligned (implicit by allocation path)
//  - if next exists, it is after current header
//  - if prev exists, its next points (eventually) forward (light check)
static int block_structurally_valid(t_block *b)
{
	if (!header_in_our_zones(b))
		return 0;
	if (b->size > (1UL << 48)) // arbitrary large sanity cap
		return 0;
	if (b->next && (char *)b->next <= (char *)b)
		return 0;
	if (b->prev && b->prev->next != b && b->prev->next != NULL)
		return 0;
	return 1;
}

int malloc_debug_valid(void *ptr)
{
	malloc_lock();
	t_block *b = ptr_to_block_internal(ptr);
	int ok = (b && block_structurally_valid(b));
	malloc_unlock();
	return ok;
}

size_t malloc_debug_aligned_size(void *ptr)
{
	malloc_lock();
	t_block *b = ptr_to_block_internal(ptr);
	if (!b)
	{
		malloc_unlock();
		return 0;
	}
	if (!block_structurally_valid(b))
	{
		malloc_unlock();
		return 0;
	}
	size_t s = b->size;
	malloc_unlock();
	return s;
}

size_t malloc_debug_requested(void *ptr)
{
	malloc_lock();
	t_block *b = ptr_to_block_internal(ptr);
	if (!b)
	{
		malloc_unlock();
		return 0;
	}
	if (!block_structurally_valid(b))
	{
		malloc_unlock();
		return 0;
	}
	size_t r = b->requested;
	malloc_unlock();
	return r;
}
