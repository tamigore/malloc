/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   print.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/27 15:31:54 by tamigore          #+#    #+#             */
/*   Updated: 2025/09/27 15:31:55 by tamigore         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "print.h"

static void print_str(const char *s)
{
    while (*s)
        write(1, s++, 1);
}

static void print_int(int n)
{
    char buf[12];
    int i = 10, neg = 0;
    buf[11] = '\0';
    if (n == 0)
    {
        write(1, "0", 1);
        return;
    }
    if (n < 0)
    {
        neg = 1;
        n = -n;
    }
    while (n && i)
    {
        buf[i--] = '0' + (n % 10);
        n /= 10;
    }
    if (neg)
        buf[i--] = '-';
    print_str(&buf[i + 1]);
}

static void print_ptr(void *p)
{
    uintptr_t addr = (uintptr_t)p;
    char buf[19];
    const char *hex = "0123456789abcdef";
    int i = 17;
    buf[18] = '\0';
    if (addr == 0)
    {
        write(1, "(nil)", 5);
        return;
    }
    while (addr && i > 1)
    {
        buf[i--] = hex[addr % 16];
        addr /= 16;
    }
    buf[i--] = 'x';
    buf[i--] = '0';
    print_str(&buf[i + 1]);
}

static void print_size_t(size_t n)
{
    char buf[21];
    int i = 19;
    buf[20] = '\0';
    if (n == 0)
    {
        write(1, "0", 1);
        return;
    }
    while (n && i)
    {
        buf[i--] = '0' + (n % 10);
        n /= 10;
    }
    print_str(&buf[i + 1]);
}

void print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    while (*fmt)
    {
        if (*fmt == '%' && *(fmt + 1))
        {
            fmt++;
            if (*fmt == 'd')
            {
                int n = va_arg(args, int);
                print_int(n);
            }
            else if (*fmt == 's')
            {
                char *s = va_arg(args, char *);
                print_str(s ? s : "(null)");
            }
            else if (*fmt == 'p')
            {
                void *p = va_arg(args, void *);
                print_ptr(p);
            }
            else if (*fmt == 'u')
            {
                size_t n = va_arg(args, size_t);
                print_size_t(n);
            }
            else
                write(1, fmt, 1);
        }
        else
            write(1, fmt, 1);
        fmt++;
    }
    va_end(args);
}