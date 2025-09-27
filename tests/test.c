/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/15 17:20:47 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 18:23:45 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Custom test harness (CUnit removed)
#include "test.h"
#include "ft_malloc.h"

#ifndef CUSTOM_ALLOCATOR
__attribute__((weak)) void add_custom_tests(void) {}
__attribute__((weak)) void show(void) {}
#endif

// Minimal assertion helpers
static int g_failures = 0;
static void t_fail(const char *name, const char *msg)
{
	++g_failures;
	fprintf(stderr, "[FAIL] %s: %s\n", name, msg);
}
static void t_assert(int cond, const char *name, const char *expr)
{
	if (!cond)
		t_fail(name, expr);
}

// Registry
#define MAX_TESTS 128
static t_test_case g_tests[MAX_TESTS];
static int g_test_count = 0;
void test_register(const char *name, t_test_fn fn)
{
	if (g_test_count < MAX_TESTS)
	{
		g_tests[g_test_count].name = name;
		g_tests[g_test_count].fn = fn;
		g_test_count++;
	}
}
int test_run_all(void)
{
	for (int i = 0; i < g_test_count; ++i)
		g_tests[i].fn();
	return g_failures;
}
void test_report(void) { fprintf(stderr, "Tests run: %d Failed: %d\n", g_test_count, g_failures); }

// Worker thread
static void *worker(void *arg)
{
	t_worker_ctx *ctx = (t_worker_ctx *)arg;
	void **arr = malloc(ctx->slots * sizeof(void *));
	if (!arr)
		return NULL;
	for (unsigned i = 0; i < ctx->slots; ++i)
		arr[i] = NULL;
	unsigned seed = (unsigned)(0x9e3779b9u ^ (ctx->tid * 2654435761u));
	for (unsigned it = 0; it < ctx->iters; ++it)
	{
		seed = seed * 1664525u + 1013904223u;
		unsigned idx = seed % ctx->slots;
		seed = seed * 1664525u + 1013904223u;
		size_t sz = (seed % ctx->max_size) + 1;
		if (arr[idx])
		{
			free(arr[idx]);
			arr[idx] = NULL;
		}
		arr[idx] = malloc(sz);
		if (arr[idx])
			memset(arr[idx], (int)(sz & 0xFF), sz < 16 ? sz : 16);
	}
	for (unsigned i = 0; i < ctx->slots; ++i)
		free(arr[i]);
	free(arr);
	return NULL;
}

static void test_multithread_stress(void)
{
	const unsigned threads = 8;
	const unsigned iters = 10000;
	const size_t max_size = 2048;
	const unsigned slots = 256;
	pthread_t th[threads];
	t_worker_ctx ctx[threads];
	for (unsigned i = 0; i < threads; ++i)
	{
		ctx[i].tid = i;
		ctx[i].iters = iters;
		ctx[i].max_size = max_size;
		ctx[i].slots = slots;
		int rc = pthread_create(&th[i], NULL, worker, &ctx[i]);
		t_assert(rc == 0, "multithread_stress", "pthread_create==0");
	}
	for (unsigned i = 0; i < threads; ++i)
		if (th[i])
			pthread_join(th[i], NULL);
}

static void test_realloc_growth(void)
{
	void *p = malloc(32);
	t_assert(p != NULL, "realloc_growth", "malloc 32");
	if (!p)
		return;
	memset(p, 0xAB, 32);
	void *q = realloc(p, 2000);
	t_assert(q != NULL, "realloc_growth", "realloc 2000");
	if (q)
	{
		unsigned char *uc = (unsigned char *)q;
		for (int i = 0; i < 32; ++i)
			if (uc[i] != 0xAB)
			{
				t_fail("realloc_growth", "data mismatch");
				break;
			}
		free(q);
	}
}

static void test_many_small_allocs(void)
{
	const size_t N = 1024;
	void **arr = malloc(N * sizeof(void *));
	t_assert(arr != NULL, "many_small_allocs", "array alloc");
	if (!arr)
		return;
	for (size_t i = 0; i < N; ++i)
	{
		arr[i] = malloc(16);
		if (!arr[i])
		{
			t_fail("many_small_allocs", "malloc 16 NULL");
			break;
		}
	}
	for (size_t i = 0; i < N; ++i)
		free(arr[i]);
	free(arr);
}

static void register_core_tests(void)
{
	test_register("realloc growth", test_realloc_growth);
	test_register("many small allocs", test_many_small_allocs);
	test_register("multithread stress", test_multithread_stress);
}

int main(void)
{
	register_core_tests();
	add_custom_tests();
	int fails = test_run_all();
	test_report();
	show();
	return fails ? 1 : 0;
}