/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_bin.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:26:53 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 18:42:52 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_BIN_H
#define MALLOC_BIN_H

#include "malloc_blocks.h"

// Segregated bins API
t_block *malloc_bin_take(size_t size);
void     malloc_bin_insert(t_block *b);
void     malloc_bin_remove(t_block *b);

#endif /* MALLOC_BIN_H */