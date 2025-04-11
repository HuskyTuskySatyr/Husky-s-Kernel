#ifndef SYSTEM_H
#define SYSTEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Kernel-specific types
typedef long off_t;

// Memory mapping flags
#define PROT_READ       0x1
#define PROT_WRITE      0x2
#define PROT_EXEC       0x4
#define MAP_PRIVATE     0x02
#define MAP_ANONYMOUS   0x20
#define MAP_FAILED      ((void*)-1)

// Character types
bool isalpha(int c);
bool isdigit(int c);
bool isalnum(int c);

// String conversion
double atof(const char* str);

// Typedefs
typedef uint32_t dword;
typedef uint16_t word;
typedef uint8_t byte;
typedef long off_t;  // Standard definition for off_t

// Function declarations
void free(void *ptr);
int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int isspace(int c);
int atoi(const char *str);
void* malloc(size_t size);
void* memset(void* ptr, int value, size_t num);
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
void wrstr(const char *str);
// String operations
char* strcpy(char* dest, const char* src);
size_t strlen(const char* str);
char* strcat(char* dest, const char* src);
int strncmp(const char* s1, const char* s2, size_t n);
char* strdup(const char* s);
char *strncpy(char *dest, const char *src, size_t n);
char* strndup(const char* str, size_t n);
// ACPI related
int initAcpi(void);
void acpiPowerOff(void);

#endif