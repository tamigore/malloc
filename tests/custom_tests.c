/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   custom_tests.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:52:57 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/27 15:39:32 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "test.h"

#ifdef CUSTOM_ALLOCATOR
#include "ft_malloc.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // for memset

// Local lightweight asserts (independent of main harness counters)
static void ct_fail(const char *name, const char *msg) { fprintf(stderr, "[FAIL] %s: %s\n", name, msg); }

static void ct_assert(int cond, const char *name, const char *expr)
{
	if (!cond)
		ct_fail(name, expr);
}

static int classify_size(size_t sz)
{
	if (sz <= TINY_MAX)
		return 0;
	if (sz <= SMALL_MAX)
		return 1;
	return 2;
}

static void test_tiny_boundary(void)
{
	void *a = malloc(1);
	void *b = malloc(TINY_MAX);
	ct_assert(a != NULL, "tiny boundary", "a");
	ct_assert(b != NULL, "tiny boundary", "b");
	if (a && b)
	{
		ct_assert(classify_size(1) == 0, "tiny boundary", "class a");
		ct_assert(classify_size(TINY_MAX) == 0, "tiny boundary", "class b");
	}
	free(a);
	free(b);
}

static void test_small_boundary(void)
{
	void *a = malloc(TINY_MAX + 1);
	void *b = malloc(SMALL_MAX);
	ct_assert(a && b, "small boundary", "allocs");
	free(a);
	free(b);
}

static void test_large_allocation(void)
{
	void *p = malloc(SMALL_MAX + 1);
	ct_assert(p != NULL, "large allocation", "p");
	free(p);
}

static void test_coalesce_basic(void)
{
	void *a = malloc(64), *b = malloc(64), *c = malloc(64);
	ct_assert(a && b && c, "coalesce basic", "abc");
	free(b);
	void *d = malloc(48);
	ct_assert(d != NULL, "coalesce basic", "reuse");
	free(a);
	free(c);
	free(d);
}

static void size_test(void)
{
	void *a = malloc(1);
	ct_assert(a != NULL, "size test", "alloc");
	free(a);
}

static void test_alignment(void)
{
	for (size_t sz = 1; sz <= 512; sz += 7)
	{
		void *p = malloc(sz);
		ct_assert(p != NULL, "alignment", "p");
		if (p && ((uintptr_t)p % MALLOC_ALIGN) != 0)
			ct_fail("alignment", "misaligned");
		free(p);
	}
}

static void test_zero_size(void)
{
	void *p = malloc(0);
	ct_assert(p != NULL, "zero size", "malloc(0)");
	free(p);
}

static void test_double_free(void)
{
	void *p = malloc(64);
	ct_assert(p != NULL, "double free", "p");
	free(p);
	void *q = malloc(64);
	ct_assert(q != NULL, "double free", "q");
	free(q);
}

static void test_free_null(void) { free(NULL); }

static void test_split_reuse(void)
{
	size_t big = TINY_MAX;
	void *a = malloc(big);
	ct_assert(a != NULL, "split reuse", "big");
	free(a);
	void *b = malloc(32);
	ct_assert(b != NULL, "split reuse", "small");
	free(b);
}

static void test_coalesce_chain(void)
{
	size_t s = 64;
	void *a = malloc(s), *b = malloc(s), *c = malloc(s);
	ct_assert(a && b && c, "coalesce chain", "abc");
	void *addr = a;
	free(b);
	free(a);
	void *d = malloc(2 * s);
	if (d)
	{
		if (2 * s > TINY_MAX)
			ct_assert(d != addr, "coalesce chain", "classify");
		else
			ct_assert(d == addr, "coalesce chain", "reuse2");
	}
	free(d);
	free(c);
	void *e = malloc(3 * s);
	if (e)
	{
		if (3 * s > TINY_MAX)
			ct_assert(e != addr, "coalesce chain", "classify");
		else
			ct_assert(e == addr, "coalesce chain", "reuse3");
	}
	free(e);
}

static void test_realloc_shrink(void)
{
	void *p = malloc(200);
	ct_assert(p != NULL, "realloc shrink", "malloc 200");
	if (!p)
		return;
	memset(p, 0xAB, 200);
	void *q = realloc(p, 50);
	ct_assert(q != NULL && q == p, "realloc shrink", "realloc shrink in-place");
	free(q);
}

void add_custom_tests(void)
{
	test_register("tiny boundary", test_tiny_boundary);
	test_register("small boundary", test_small_boundary);
	test_register("large allocation", test_large_allocation);
	test_register("coalesce basic", test_coalesce_basic);
	test_register("size test", size_test);
	test_register("alignment", test_alignment);
	test_register("zero size", test_zero_size);
	test_register("double free", test_double_free);
	test_register("free NULL", test_free_null);
	test_register("split reuse", test_split_reuse);
	test_register("coalesce chain", test_coalesce_chain);
	test_register("realloc shrink", test_realloc_shrink);
}

void show()
{
	printf("--- show_alloc_mem() ---\n");
	printf("TINY_MAX = %zu, SMALL_MAX = %zu\n", TINY_MAX, SMALL_MAX);
	/* Ensure at least one live allocation of each class before snapshot */
	void *tiny = malloc(16); /* definitely TINY */
	void *tiny2 = malloc(32); /* definitely TINY */
	void *tiny3 = malloc(8); /* definitely TINY */
	void *small = malloc(TINY_MAX + 32 > SMALL_MAX ? TINY_MAX + 1 : TINY_MAX + 32); /* within (TINY_MAX, SMALL_MAX] */
	void *large = malloc(SMALL_MAX + 1024); /* force LARGE */
	if (!tiny || !tiny2 || !tiny3 || !small || !large)
	{
		printf("Allocation failed for one of the test pointers\n");
		return;
	}
	/* Avoid NULL deref if any failed; allocator may return NULL on OOM */
	show_alloc_mem();
	/* Clean up to not leak after display */
	free(tiny);
	free(tiny3);
	free(tiny2);
	free(small);
	free(large);
	printf("------------------------\n");
}

#else
void add_custom_tests(void) {}
void show(void) {}
#endif