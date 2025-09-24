/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/19 14:10:16 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/19 14:34:51 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

static t_block *ptr_to_block_internal(void *ptr)
{
    if (!ptr)
        return NULL;
    return (t_block*)((char*)ptr - sizeof(t_block));
}

static int header_in_our_zones(t_block *b)
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

int malloc_debug_valid(void *ptr)
{
    malloc_lock();
    t_block *b = ptr_to_block_internal(ptr);
    if (!b)
    {   malloc_unlock(); return 0; }
    if (!header_in_our_zones(b))
    {   malloc_unlock(); return 0; }
    int ok = (b->magic == 0xB10C0DEU);
    malloc_unlock();
    return ok;
    // return 1;
}

size_t malloc_debug_aligned_size(void *ptr)
{
    malloc_lock();
    t_block *b = ptr_to_block_internal(ptr);
    if (!b)
    {   malloc_unlock(); return 0; }
    if (!header_in_our_zones(b))
    {   malloc_unlock(); return 0; }
    if (b->magic != 0xB10C0DEU)
    {   malloc_unlock(); return 0; }
    size_t s = b->size;
    malloc_unlock();
    return s;
}

size_t malloc_debug_requested(void *ptr)
{
    malloc_lock();
    t_block *b = ptr_to_block_internal(ptr);
    if (!b)
    {   malloc_unlock(); return 0; }
    if (!header_in_our_zones(b))
    {   malloc_unlock(); return 0; }
    if (b->magic != 0xB10C0DEU)
    {   malloc_unlock(); return 0; }
    size_t r = b->requested;
    malloc_unlock();
    return r;
}
