#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

int strcmp(const char *str1, const char *str2);
void printk(const char *format, ...);
void terminal_putchar(char c);
uint16_t inw(uint16_t port);
unsigned char inb(unsigned short port);
void outw(uint16_t port, uint16_t value);
void outb(uint16_t port, uint8_t value);

#endif // SYSTEM_H
