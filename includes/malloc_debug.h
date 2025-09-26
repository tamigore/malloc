/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_debug.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:27:45 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/26 11:26:05 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_DEBUG_H
#define MALLOC_DEBUG_H

#include "malloc_blocks.h"

// Static assert to ensure header alignment
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(t_block) % 16 == 0, "t_block header not 16-byte multiple");
#endif

size_t malloc_debug_aligned_size(void *ptr);
size_t malloc_debug_requested(void *ptr);
int    malloc_debug_valid(void *ptr); // structural validity (no canary)

#endif