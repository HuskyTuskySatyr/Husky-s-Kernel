#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stddef.h>

typedef long off_t;  // Define off_t manually

void shutdown();
void reboot();
void terminal_writeline(const char* data);
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
void* memset(void* ptr, int value, size_t num);
void* malloc(size_t size);
int memcmp(const void *s1, const void *s2, size_t n);

#endif // SYSTEM_H
