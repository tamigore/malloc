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
#include <pthread.h>

#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200809L
#endif

typedef void (*t_test_fn)(void);
typedef struct s_test_case { const char *name; t_test_fn fn; } t_test_case;

void test_register(const char *name, t_test_fn fn);
int  test_run_all(void);
void test_report(void);
void add_custom_tests(void);
void show();

typedef struct s_worker_ctx { unsigned tid; unsigned iters; size_t max_size; unsigned slots; } t_worker_ctx;

#endif