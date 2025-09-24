/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:55:45 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 16:06:57 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>
#include <pthread.h>

#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200809L
#endif

// Forward declaration for custom tests
void add_custom_tests(CU_pSuite suite);
void show();

// Worker context for multithreaded stress test
typedef struct s_worker_ctx
{
	unsigned tid;
	unsigned iters;
	size_t   max_size;
	unsigned slots;
} t_worker_ctx;

#endif