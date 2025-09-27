/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/17 11:21:11 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/17 16:35:50 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

static void *ft_memcpy(void *dst, const void *src, size_t n)
{
	unsigned char *uc_dst = (unsigned char *)dst;
	unsigned char *uc_src = (unsigned char *)src;

	if (!dst && !src)
		return (NULL);
	if (dst == src || n == 0)
		return (dst);
	while (n--)
		*uc_dst++ = *uc_src++;
	return (dst);
}

void *realloc(void *ptr, size_t size)
{
	malloc_lock();
	if (!ptr)
	{
		malloc_unlock();
		return malloc(size);
	}
	if (size == 0)
	{
		free(ptr);
		malloc_unlock();
		return NULL;
	}
	t_block *b = ptr_to_block(ptr);
	if (b->size >= size)
	{
		b->requested = size; // adjust logical requested size downward (no physical shrink)
		malloc_unlock();
		return ptr; // current block big enough
	}
	// Allocate new block and copy
	void *n = malloc(size);
	if (!n)
	{
		malloc_unlock();
		return NULL;
	}
	size_t copy = b->size < size ? b->size : size;
	ft_memcpy(n, ptr, copy);
	free(ptr);
	malloc_unlock();
	return n;
}