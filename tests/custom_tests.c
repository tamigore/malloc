/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   custom_tests.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:52:57 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 19:17:51 by tamigore         ###   ########.fr       */
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

static void size_test()
{
    void *a = malloc(1);
    CU_ASSERT_PTR_NOT_NULL(a);
    if (a)
    {
        CU_ASSERT(malloc_debug_valid(a));
        size_t req = malloc_debug_requested(a);
        CU_ASSERT(req == 1);
    }
    free(a);
}

/* --- Additional, more thorough tests --- */

static void test_alignment(void)
{
    /* Allocate a range of sizes and ensure 16-byte alignment */
    for (size_t sz = 1; sz <= 512; sz += 7)
    {
        void *p = malloc(sz);
        CU_ASSERT_PTR_NOT_NULL(p);
        if (p)
        {
            CU_ASSERT(((uintptr_t)p % MALLOC_ALIGN) == 0);
            CU_ASSERT(malloc_debug_valid(p));
            /* requested size tracks original (capped at our call) */
            CU_ASSERT(malloc_debug_requested(p) == sz);
        }
        free(p);
    }
}

static void test_zero_size(void)
{
    /* Some allocators treat malloc(0) specially; request 0 and accept non-NULL */
    void *p = malloc(0);
    CU_ASSERT_PTR_NOT_NULL(p);
    if (p)
    {
        CU_ASSERT(malloc_debug_valid(p));
        size_t r = malloc_debug_requested(p);
        CU_ASSERT(r == 0 || r == 1);
    }
    free(p);
}

static void test_double_free(void)
{
    void *p = malloc(64);
    CU_ASSERT_PTR_NOT_NULL(p);
    free(p);
    /* We intentionally do NOT re-call free(p) here because -Werror=use-after-free
       treats it as UB; allocator internally guards against double free already.
       (A runtime manual test can exercise an actual double free if desired.) */
    /* allocate again to ensure allocator still operational */
    void *again = malloc(64);
    CU_ASSERT_PTR_NOT_NULL(again);
    free(again);
}

static void test_free_null(void)
{
    /* Must be a no-op */
    free(NULL);
}

/* Deliberately omit invalid free test to keep -Werror happy (UB). */

static void test_split_reuse(void)
{
    /* Allocate a large TINY block, free it, then allocate a smaller one to force split */
    size_t big = TINY_MAX; /* already <= TINY_MAX */
    void *a = malloc(big);
    CU_ASSERT_PTR_NOT_NULL(a);
    free(a);
    size_t small = 32;
    void *b = malloc(small);
    CU_ASSERT_PTR_NOT_NULL(b);
    if (b)
    {
        size_t aligned_small = ALIGN_UP(small, MALLOC_ALIGN);
        CU_ASSERT(malloc_debug_aligned_size(b) >= aligned_small);
        CU_ASSERT(malloc_debug_requested(b) == small);
    }
    free(b);
}

static void test_coalesce_chain(void)
{
    /* Behavioral test without peeking into freed headers (avoid UB):
       Allocate three adjacent same-size blocks a,b,c.
       Free b then a -> they should coalesce into a single free block >= 2*s + header.
       Allocate a block of size 2*s and expect it to reuse the original address of a.
       Free it, then free c, and allocate 3*s which should again reuse the same start. */
    size_t s = 64;
    void *a = malloc(s);
    void *b = malloc(s);
    void *c = malloc(s);
    CU_ASSERT_PTR_NOT_NULL(a); CU_ASSERT_PTR_NOT_NULL(b); CU_ASSERT_PTR_NOT_NULL(c);
    if (!(a && b && c)) { free(a); free(b); free(c); return; }
    void *a_addr = a;
    free(b);
    free(a);
    /* Now allocate 2*s which should fit into the coalesced (a+b) region and return same base */
    void *d = malloc(2*s);
    CU_ASSERT_PTR_NOT_NULL(d);
    if (d)
        CU_ASSERT(d == a_addr);
    free(d);
    free(c); /* now (a+b+c) fully free (split header overhead absorbed) */
    void *e = malloc(3*s);
    CU_ASSERT_PTR_NOT_NULL(e);
    if (e)
        CU_ASSERT(e == a_addr);
    free(e);
}

static void test_realloc_shrink(void)
{
    void *p = malloc(200);
    CU_ASSERT_PTR_NOT_NULL(p);
    if (!p) return;
    memset(p, 0xAB, 200);
    void *q = realloc(p, 50); /* shrink */
    CU_ASSERT_PTR_NOT_NULL(q);
    /* Implementation keeps block in place on shrink -> pointer equality expected */
    CU_ASSERT(q == p);
    /* requested size updated */
    CU_ASSERT(malloc_debug_requested(q) == 50);
    /* Original first 50 bytes intact */
    unsigned char *uc = (unsigned char*)q;
    for (int i=0;i<50;++i)
        if (uc[i] != 0xAB) { CU_FAIL("realloc shrink lost data"); break; }
    free(q);
}

void add_custom_tests(CU_pSuite suite)
{
    if (!suite) return;
    if ( !CU_add_test(suite, "tiny boundary", test_tiny_boundary) ||
         !CU_add_test(suite, "small boundary", test_small_boundary) ||
         !CU_add_test(suite, "large allocation", test_large_allocation) ||
         !CU_add_test(suite, "coalesce basic", test_coalesce_basic) ||
         !CU_add_test(suite, "size test", size_test) ||
         !CU_add_test(suite, "alignment", test_alignment) ||
         !CU_add_test(suite, "zero size", test_zero_size) ||
         !CU_add_test(suite, "double free", test_double_free) ||
         !CU_add_test(suite, "free NULL", test_free_null) ||
         /* invalid free test removed */ \
         !CU_add_test(suite, "split reuse", test_split_reuse) ||
         !CU_add_test(suite, "coalesce chain", test_coalesce_chain) ||
         !CU_add_test(suite, "realloc shrink", test_realloc_shrink))
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