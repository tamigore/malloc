/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_blocks.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:26:56 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/27 12:55:12 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_BLOCKS_H
#define MALLOC_BLOCKS_H

#include <stddef.h>
#include <stdint.h>

// Alignment 16 bytes
#define MALLOC_ALIGN 16UL
#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))

typedef enum e_zone_type
{
	ZONE_TINY = 0,
	ZONE_SMALL = 1,
	ZONE_LARGE = 2
} t_zone_type;

// TINY_MAX / SMALL_MAX remain unchanged while ensuring page-size multiples.
size_t malloc_tiny_max(void);
size_t malloc_small_max(void);
#define TINY_MAX (malloc_tiny_max())
#define SMALL_MAX (malloc_small_max())

// Forward declaration for t_block
struct s_zone;

// Layout kept 16-byte aligned and size multiple of 16 (static assert in debug header).
typedef struct s_block
{
	size_t size;			  // aligned payload size
	size_t requested;		  // original requested size
	struct s_block *next;	  // next block in zone list
	struct s_block *prev;	  // prev block in zone list
	struct s_block *bin_next; // next in size-class free list
	struct s_block *bin_prev; // prev in size-class free list
	struct s_zone *zone;	  // owning zone back-pointer
	char free;				  // free flag
} __attribute__((aligned(16))) t_block;

typedef struct s_zone
{
	t_zone_type type;
	size_t capacity;	 // usable bytes for blocks after data_offset
	size_t used;		 // sum of allocated payload sizes
	size_t data_offset;	 // aligned offset to first block area
	struct s_zone *next; // next zone
	t_block *blocks;	 // first block
	t_block *tail;		 // last block
	// Bin metadata (stored in one arbitrary zone to avoid extra global vars)
	t_block **bins;      // dynamic array of bin heads
	size_t bin_count;    // number of bins
} t_zone;

// Global head of all zones
extern t_zone *g_zones;

#endif
