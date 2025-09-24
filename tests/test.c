/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/15 17:20:47 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 16:39:28 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Generic CUnit test runner. Internal allocator tests live in custom_tests.c
// and are only compiled into the custom test binary (with -DCUSTOM_ALLOCATOR).
#include "test.h"

static void *worker(void *arg)
{
	t_worker_ctx *ctx = (t_worker_ctx*)arg;
	void **arr = malloc(ctx->slots * sizeof(void*));
	if (!arr)
		return NULL;
	for (unsigned i=0;i<ctx->slots;++i)
		arr[i]=NULL;
	unsigned seed = (unsigned)(0x9e3779b9u ^ (ctx->tid * 2654435761u));
	for (unsigned it=0; it<ctx->iters; ++it)
	{
		// simple LCG
		seed = seed*1664525u + 1013904223u;
		unsigned idx = seed % ctx->slots;
		seed = seed*1664525u + 1013904223u;
		size_t sz = (seed % ctx->max_size) + 1;
		if (arr[idx])
		{
			free(arr[idx]);
			arr[idx]=NULL;
		}
		arr[idx] = malloc(sz);
		if (arr[idx]) // touch memory to force actual use
			memset(arr[idx], (int)(sz & 0xFF), sz < 16 ? sz : 16);
	}
	for (unsigned i=0;i<ctx->slots;++i)
		free(arr[i]);
	free(arr);
	return NULL;
}

static void test_multithread_stress(void)
{
	const unsigned threads = 8;
	const unsigned iters_per_thread = 10000;
	const size_t   max_size = 2048;
	const unsigned slots = 256;
	pthread_t th[threads];
	t_worker_ctx ctx[threads];
	for (unsigned i=0;i<threads;++i)
	{
		ctx[i].tid=i; ctx[i].iters=iters_per_thread; ctx[i].max_size=max_size; ctx[i].slots=slots;
		int rc = pthread_create(&th[i], NULL, worker, &ctx[i]);
		CU_ASSERT_EQUAL(rc, 0);
	}
	for (unsigned i=0;i<threads;++i)
		if (th[i])
			pthread_join(th[i], NULL);
}

static void test_realloc_growth(void)
{
	void *p = malloc(32);
	CU_ASSERT_PTR_NOT_NULL(p);
	if (!p)
		return;
	memset(p, 0xAB, 32);
	void *q = realloc(p, 2000);
	CU_ASSERT_PTR_NOT_NULL(q);
	if (q)
	{
		unsigned char *uc = (unsigned char*)q;
		for (int i = 0; i < 32; ++i)
		{
			if (uc[i] != 0xAB)
			{
				CU_FAIL("data mismatch");
				break;
			}
		}
		free(q);
	}
}

// Additional generic smoke test exercising multiple small allocations.
static void test_many_small_allocs(void)
{
	const size_t N = 1024;
	void **arr = malloc(N * sizeof(void*));
	CU_ASSERT_PTR_NOT_NULL(arr);
	if (!arr)
		return;
	for (size_t i=0;i<N;++i)
	{
		arr[i] = malloc(16);
		if (!arr[i])
		{
			CU_FAIL("allocation returned NULL");
			break;
		}
	}
	for (size_t i=0;i<N;++i)
		free(arr[i]);
	free(arr);
}

int main()
{
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();
	CU_pSuite suite = CU_add_suite("zone_allocator", NULL, NULL);
	if (!suite)
	{
		CU_cleanup_registry();
		return CU_get_error();
	}
	if ( (!CU_add_test(suite, "realloc growth", test_realloc_growth)) ||
	    (!CU_add_test(suite, "many small allocs", test_many_small_allocs)) ||
	    (!CU_add_test(suite, "multithread stress", test_multithread_stress)) )
	{
		CU_cleanup_registry();
		return CU_get_error();
	}
	// Add custom allocator tests (no-op in baseline build)
	add_custom_tests(suite);
	CU_basic_set_mode(CU_BRM_NORMAL);
	CU_basic_run_tests();
	show();
	unsigned int fails = CU_get_number_of_failures();
	CU_cleanup_registry();
	return fails ? 1 : 0;
}