/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bench_micro.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: automated <allocator@test>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 19:35:00 by automated         #+#    #+#             */
/*   Updated: 2025/09/24 19:35:00 by automated        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

/* High resolution time helper */
static double now_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static uint64_t xorshift64(uint64_t *s)
{
	uint64_t x = *s;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	*s = x;
	return x;
}

static void touch(void *p, size_t sz)
{
	if (!p)
		return;
	size_t step = sz < 32 ? sz : 32; /* touch first up to 32 bytes */
	memset(p, (int)(sz & 0xFF), step);
}

/* Scenario 1: fixed size alloc/free loop */
static void bench_fixed(size_t iters, size_t sz)
{
	double t0 = now_sec();
	for (size_t i = 0; i < iters; ++i)
	{
		void *p = malloc(sz);
		touch(p, sz);
		free(p);
	}
	double t1 = now_sec();
	printf("fixed,%zu,%zu,%.6f\n", iters, sz, t1 - t0);
}

/* Scenario 2: random size <= max, immediate free */
static void bench_rand_sizes(size_t iters, size_t max_sz)
{
	uint64_t seed = 0x123456789ABCDEF0ULL;
	double t0 = now_sec();
	for (size_t i = 0; i < iters; ++i)
	{
		size_t sz = (xorshift64(&seed) % max_sz) + 1;
		void *p = malloc(sz);
		touch(p, sz);
		free(p);
	}
	double t1 = now_sec();
	printf("rand_immediate,%zu,%zu,%.6f\n", iters, max_sz, t1 - t0);
}

/* Scenario 3: working set of pointers with churn */
static void bench_working_set(size_t iters, size_t set_size, size_t max_sz)
{
	void **set = calloc(set_size, sizeof(void *));
	if (!set)
	{
		fprintf(stderr, "set alloc failed\n");
		return;
	}
	uint64_t seed = 0xCAFEBABEDEADBEEFULL;
	double t0 = now_sec();
	for (size_t i = 0; i < iters; ++i)
	{
		size_t idx = xorshift64(&seed) % set_size;
		if (set[idx])
		{
			free(set[idx]);
			set[idx] = NULL;
		}
		size_t sz = (xorshift64(&seed) % max_sz) + 1;
		set[idx] = malloc(sz);
		touch(set[idx], sz);
	}
	for (size_t i = 0; i < set_size; ++i)
		free(set[i]);
	free(set);
	double t1 = now_sec();
	printf("working_set,%zu,%zu/%zu,%.6f\n", iters, set_size, max_sz, t1 - t0);
}

/* Scenario 4: realloc growth + occasional shrink */
static void bench_realloc(size_t iters, size_t start_sz, size_t max_sz)
{
	uint64_t seed = 0xA55A55A55ULL;
	void *p = malloc(start_sz);
	size_t cur = start_sz;
	double t0 = now_sec();
	for (size_t i = 0; i < iters; ++i)
	{
		size_t target = (xorshift64(&seed) % max_sz) + 1;
		if (target > cur)
		{
			void *n = realloc(p, target);
			if (n)
			{
				p = n;
				cur = target;
			}
		}
		else if ((i & 0x7) == 0)
		{ /* occasional shrink */
			void *n = realloc(p, target);
			if (n)
			{
				p = n;
				cur = target;
			}
		}
		touch(p, cur);
	}
	free(p);
	double t1 = now_sec();
	printf("realloc_mix,%zu,%zu-%zu,%.6f\n", iters, start_sz, max_sz, t1 - t0);
}

/* Scenario 5: fragmentation stress (allocate many, free pattern, reallocate) */
static void bench_fragment(size_t blocks, size_t block_sz)
{
	void **arr = malloc(blocks * sizeof(void *));
	if (!arr)
	{
		fprintf(stderr, "fragment arr alloc fail\n");
		return;
	}
	for (size_t i = 0; i < blocks; ++i)
	{
		arr[i] = malloc(block_sz);
		touch(arr[i], block_sz);
	}
	/* free every other */
	for (size_t i = 0; i < blocks; i += 2)
	{
		free(arr[i]);
		arr[i] = NULL;
	}
	/* allocate new blocks for freed spots with slightly different size */
	size_t new_sz = block_sz / 2 + 1;
	for (size_t i = 0; i < blocks; i += 2)
	{
		arr[i] = malloc(new_sz);
		touch(arr[i], new_sz);
	}
	/* free all */
	for (size_t i = 0; i < blocks; ++i)
		free(arr[i]);
	free(arr);
	/* Not timed in phases to keep output compact; measure total time */
	// Could extend to separate phases if needed.
	printf("fragment_pattern,%zu,%zu->%zu,done\n", blocks, block_sz, new_sz);
}

static void usage(const char *prog)
{
	fprintf(stderr,
			"Usage: %s [scenario] [args...]\n"
			"Scenarios (CSV output):\n"
			"  fixed iters size\n"
			"  rand iters max_size\n"
			"  ws iters set_size max_size\n"
			"  realloc iters start max\n"
			"  frag blocks block_size\n",
			prog);
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		usage(argv[0]);
		return 1;
	}
	const char *mode = argv[1];
	if (!strcmp(mode, "fixed"))
	{
		if (argc < 4)
		{
			usage(argv[0]);
			return 1;
		}
		bench_fixed(strtoull(argv[2], NULL, 10), strtoull(argv[3], NULL, 10));
	}
	else if (!strcmp(mode, "rand"))
	{
		if (argc < 4)
		{
			usage(argv[0]);
			return 1;
		}
		bench_rand_sizes(strtoull(argv[2], NULL, 10), strtoull(argv[3], NULL, 10));
	}
	else if (!strcmp(mode, "ws"))
	{
		if (argc < 5)
		{
			usage(argv[0]);
			return 1;
		}
		bench_working_set(strtoull(argv[2], NULL, 10), strtoull(argv[3], NULL, 10), strtoull(argv[4], NULL, 10));
	}
	else if (!strcmp(mode, "realloc"))
	{
		if (argc < 5)
		{
			usage(argv[0]);
			return 1;
		}
		bench_realloc(strtoull(argv[2], NULL, 10), strtoull(argv[3], NULL, 10), strtoull(argv[4], NULL, 10));
	}
	else if (!strcmp(mode, "frag"))
	{
		if (argc < 4)
		{
			usage(argv[0]);
			return 1;
		}
		bench_fragment(strtoull(argv[2], NULL, 10), strtoull(argv[3], NULL, 10));
	}
	else
	{
		usage(argv[0]);
		return 1;
	}
	return 0;
}
