/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_pthread.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:27:47 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/24 15:27:47 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_PTHREAD_H
#define MALLOC_PTHREAD_H

#include <pthread.h>

// Thread-safety primitives for the allocator
void malloc_lock(void);
void malloc_unlock(void);

#endif