/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   print.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/27 15:31:57 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/27 15:31:58 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PRINT_H
#define PRINT_H

#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>

void print(const char *fmt, ...);

#endif