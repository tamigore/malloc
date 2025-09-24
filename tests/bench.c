/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bench.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/19 14:35:00 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/20 11:32:58 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void bench(size_t N, size_t SZ)
{
    void **arr = malloc(N * sizeof(void*));
    if (!arr) {
        fprintf(stderr, "alloc failure for pointer table\n");
        return;
    }
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (size_t i=0;i<N;++i)
        arr[i]=malloc(SZ);
    for (size_t i=0;i<N;++i)
        free(arr[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)/1e9;
    printf("Benchmark %zu alloc/free(%zu): %.6f s\n", N, (size_t)SZ, elapsed);
    free(arr);
}

int main(int argc, char **argv)
{
    size_t N = 100000;
    size_t SZ = 64;
    if (argc >= 2) N = (size_t)strtoull(argv[1], NULL, 10);
    if (argc >= 3) SZ = (size_t)strtoull(argv[2], NULL, 10);
    bench(N, SZ);
    return 0;
}
