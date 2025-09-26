/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_malloc.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:27:53 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/26 11:05:22 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __MALLOC_H__
#define __MALLOC_H__

#include <sys/mman.h>
#include <unistd.h>

#include "malloc_blocks.h"
#include "malloc_debug.h"
#include "malloc_bin.h"
#include "malloc_pthread.h"

inline static t_block *ptr_to_block(void *ptr)
{
	return (t_block*)((char*)ptr - sizeof(t_block));
}

// Memory display function
void show_alloc_mem();

void free(void *ptr);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);

#endif
