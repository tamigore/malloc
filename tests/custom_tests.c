/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   custom_tests.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:52:57 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 16:41:26 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "test.h"

#ifdef CUSTOM_ALLOCATOR
#include "ft_malloc.h"
#include <stdio.h>
#include <stdlib.h>

static int classify_size(size_t sz) { if (sz <= TINY_MAX) return 0; if (sz <= SMALL_MAX) return 1; return 2; }

static void test_tiny_boundary(void)
{
    void *a = malloc(1);
    void *b = malloc(TINY_MAX);
    CU_ASSERT_PTR_NOT_NULL(a);
    CU_ASSERT_PTR_NOT_NULL(b);
    if (a && b)
    {
        CU_ASSERT(malloc_debug_valid(a));
        CU_ASSERT(malloc_debug_valid(b));
        CU_ASSERT(classify_size(malloc_debug_requested(a)) == 0);
        CU_ASSERT(classify_size(malloc_debug_requested(b)) == 0);
    }
    free(a); free(b);
}

static void test_small_boundary(void)
{
    void *a = malloc(TINY_MAX + 1);
    void *b = malloc(SMALL_MAX);
    CU_ASSERT_PTR_NOT_NULL(a); CU_ASSERT_PTR_NOT_NULL(b);
    if (a && b)
    {
        CU_ASSERT(malloc_debug_valid(a));
        CU_ASSERT(malloc_debug_valid(b));
        CU_ASSERT(classify_size(malloc_debug_requested(a)) == 1);
        CU_ASSERT(classify_size(malloc_debug_requested(b)) == 1);
    }
    free(a); free(b);
}

static void test_large_allocation(void)
{
    void *p = malloc(SMALL_MAX + 1);
    CU_ASSERT_PTR_NOT_NULL(p);
    if (p) {
        CU_ASSERT(malloc_debug_valid(p));
        CU_ASSERT(classify_size(malloc_debug_requested(p)) == 2);
    }
    free(p);
}

static void test_coalesce_basic(void)
{
    void *a = malloc(64), *b = malloc(64), *c = malloc(64);
    CU_ASSERT_PTR_NOT_NULL(a); CU_ASSERT_PTR_NOT_NULL(b); CU_ASSERT_PTR_NOT_NULL(c);
    free(b);
    void *d = malloc(48);
    CU_ASSERT_PTR_NOT_NULL(d);
    free(a); free(c); free(d);
}

void size()
{
    printf("t_block sizeof: %zu\n", sizeof(t_block));

    void *a=malloc(1);
    void *b;
    b = a;
    a = b;
    printf("a req=%zu b req=%zu a size=%zu b size=%zu\n",
        ((t_block*) ((char*)a - sizeof(t_block)))->requested,
        ((t_block*) ((char*)b - sizeof(t_block)))->requested,
        ((t_block*) ((char*)a - sizeof(t_block)))->size,
        ((t_block*) ((char*)b - sizeof(t_block)))->size);
    free(a);
    // free(b);
}

void add_custom_tests(CU_pSuite suite)
{
    if (!suite) return;
    if ( !CU_add_test(suite, "tiny boundary", test_tiny_boundary) ||
         !CU_add_test(suite, "small boundary", test_small_boundary) ||
         !CU_add_test(suite, "large allocation", test_large_allocation) ||
         !CU_add_test(suite, "coalesce basic", test_coalesce_basic) ||
         !CU_add_test(suite, "size", size))
        fprintf(stderr, "[warn] failed to add some custom tests\n");
}

void show()
{
    printf("--- show_alloc_mem() ---\n");
    show_alloc_mem();
    printf("------------------------\n");
}

#else

void add_custom_tests(CU_pSuite suite) { (void)suite; }
void show(void) {}

#endif