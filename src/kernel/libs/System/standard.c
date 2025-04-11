#include "../Drivers/kernel.h"
#include "system.h"
#include "system.h"
#include <stdarg.h>
#include <stdint.h>

int strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

void itoa(int value, char *str, int base) {
    char *ptr = str, *ptr1 = str, tmp_char;
    int tmp_value;

    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return;
    }

    if (value < 0 && base == 10) {
        *ptr++ = '-';
        ptr1++;
        value = -value;
    }

    while (value) {
        tmp_value = value % base;
        *ptr++ = (tmp_value < 10) ? ('0' + tmp_value) : ('a' + tmp_value - 10);
        value /= base;
    }
    *ptr = '\0';

    // Reverse the string
    while (ptr1 < --ptr) {
        tmp_char = *ptr;
        *ptr = *ptr1;
        *ptr1 = tmp_char;
        ptr1++;
    }
}

uint16_t inw(uint16_t port) {
    uint16_t result;
    asm volatile("inw %1, %0" : "=a"(result) : "d"(port));
    return result;
}

void outw(uint16_t port, uint16_t value) {
    asm volatile("outw %0, %1" : : "a"(value), "d"(port));
}

void printk(const char *format, ...) {
    char buffer[256];  // Temporary buffer
    char *buf_ptr = buffer;
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '%' && *(format + 1)) {
            format++;
            if (*format == 'd') {  // Integer formatting
                int value = va_arg(args, int);
                char num_buffer[12];
                itoa(value, num_buffer, 10);
                for (char *num_ptr = num_buffer; *num_ptr; num_ptr++) {
                    *buf_ptr++ = *num_ptr;
                }
            } else if (*format == 's') {  // String formatting
                char *str = va_arg(args, char *);
                while (*str) {
                    *buf_ptr++ = *str++;
                }
            } else {
                *buf_ptr++ = '%';  
                *buf_ptr++ = *format;
            }
        } else {
            *buf_ptr++ = *format;
        }
        format++;
    }
    *buf_ptr = '\0'; // Null-terminate

    va_end(args);

    // Ignore terminal_writeline error
    terminal_writeline(buffer);
}

