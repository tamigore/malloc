/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_blocks.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:26:56 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 19:17:51 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_BLOCKS_H
#define MALLOC_BLOCKS_H

#include <stddef.h>
#include <stdint.h>

// Forward declaration so we can embed a back-pointer inside t_block
struct s_zone; 

// Alignment 16 bytes
#define MALLOC_ALIGN 16UL
#define ALIGN_UP(x,a) (((x) + (a) - 1) & ~((a) - 1))

typedef enum e_zone_type
{
	ZONE_TINY = 0,
	ZONE_SMALL = 1,
	ZONE_LARGE = 2
} t_zone_type;

// Size limits for TINY and SMALL allocations
#ifndef TINY_MAX
# define TINY_MAX 128UL
#endif
#ifndef SMALL_MAX
# define SMALL_MAX 4096UL
#endif

typedef struct s_block
{
	uint32_t        magic;      // canary for corruption detection
	uint32_t        _pad;       // keep alignment for size/requested
	size_t          size;       // aligned payload size
	size_t          requested;  // original requested size
	char            free;       // free flag
	char            _pad2[7];   // padding (will be reused if we compact later)
	struct s_block *next;       // next block in zone list
	struct s_block *prev;       // prev block in zone list
	struct s_block *bin_next;   // next in size-class free list
	struct s_block *bin_prev;   // prev in size-class free list
	struct s_zone  *zone;       // NEW: owning zone back-pointer
} __attribute__((aligned(16))) t_block;

typedef struct s_zone
{
	t_zone_type		type;
	size_t			capacity;   // usable bytes for blocks after data_offset
	size_t			used;       // sum of allocated payload sizes
	size_t			data_offset;// aligned offset to first block area
	struct s_zone	*next;      // next zone
	t_block			*blocks;    // first block
	t_block		*tail;      // last block (for O(1) append)
} t_zone;

// Global head of all zones (ordered: TINY -> SMALL -> LARGE appended)
extern t_zone *g_zones;

#endif
