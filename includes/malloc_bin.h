/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_bin.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:26:53 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/27 14:05:11 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_BIN_H
#define MALLOC_BIN_H

#include "malloc_blocks.h"

// Segregated bins API
// Filter by desired zone type to prevent cross-class reuse (e.g., tiny request reusing small block)
t_block *malloc_bin_take(size_t size, t_zone_type type);
void malloc_bin_insert(t_block *b);
void malloc_bin_remove(t_block *b);

#endif