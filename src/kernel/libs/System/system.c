#include <stddef.h>
#include "system.h"
#include "../Drivers/kernel.h"
#include "time.h"

#define HEAP_SIZE 0x100000
static uint8_t heap[HEAP_SIZE];
static size_t heap_index = 0;

#define PAGE_SIZE 0x1000
#define PHYS_MEM_SIZE 0x1000000  // 16 MB
#define MAX_PAGES (PHYS_MEM_SIZE / PAGE_SIZE)

static uint8_t physical_memory[PHYS_MEM_SIZE];
static uint8_t page_bitmap[MAX_PAGES];  // 0 = free, 1 = used

void* alloc_page() {
    for (size_t i = 0; i < MAX_PAGES; i++) {
        if (!page_bitmap[i]) {
            page_bitmap[i] = 1;
            return &physical_memory[i * PAGE_SIZE];
        }
    }
    return NULL;
}

void free_page(void* ptr) {
    size_t offset = (uintptr_t)ptr - (uintptr_t)physical_memory;
    if (offset % PAGE_SIZE != 0) return; // Not aligned

    size_t index = offset / PAGE_SIZE;
    if (index < MAX_PAGES) {
        page_bitmap[index] = 0;
    }
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}


void free(void *ptr) {
    // No-op in bump allocator
    (void)ptr;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1, *p2 = s2;
    for (size_t i = 0; i < n; ++i) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

void *memcpy(void *dest, const void *src, size_t n) {
    asm volatile (
        "rep movsb"
        : "+D" (dest), "+S" (src), "+c" (n)
        :
        : "memory"
    );
    return dest;
}

int isspace(int c) {
    return (c == ' ' || c == '\t' || c == '\n' ||
            c == '\v' || c == '\f' || c == '\r');
}

int atoi(const char *str) {
    int result = 0;
    int sign = 1;

    // Skip leading whitespace
    while (isspace(*str)) {
        str++;
    }

    // Handle optional sign
    if (*str == '-' || *str == '+') {
        if (*str == '-') sign = -1;
        str++;
    }

    // Parse digits
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

void* malloc(size_t size) {
    if (heap_index + size > HEAP_SIZE) return NULL;  // Out of memory
    void* ptr = &heap[heap_index];
    heap_index += size;
    return ptr;
}

// Simple memset implementation
void* memset(void* ptr, int value, size_t num) {
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (uint8_t)value;
    }
    return ptr;
}

bool isalpha(int c) {
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

// String Functions
char* strdup(const char* s) {
    if (s == NULL) return NULL;
    
    size_t len = strlen(s) + 1;
    char* new = malloc(len);
    
    if (new) {
        memcpy(new, s, len);
    }
    
    return new;
}

char* strndup(const char* str, size_t n) {
    // Allocate memory for the new string
    char* dup = malloc(n + 1);  // +1 for the null-terminator
    if (!dup) {
        return NULL;  // Memory allocation failed
    }

    // Copy up to n characters from str to dup
    size_t i = 0;
    while (i < n && str[i] != '\0') {
        dup[i] = str[i];
        i++;
    }

    // Null-terminate the destination string
    dup[i] = '\0';

    return dup;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;

    // Copy characters from src to dest until we hit n or null byte
    while (i < n && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }

    // If we haven't copied n characters, pad with '\0'
    while (i < n) {
        dest[i] = '\0';
        i++;
    }

    return dest;
}

char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while ((*dest++ = *src++));
    return original_dest;
}

// String concatenation
char* strcat(char* dest, const char* src) {
    char* original_dest = dest;
    
    // Find end of dest
    while (*dest) dest++;
    
    // Copy src to end of dest
    while ((*dest++ = *src++));
    
    return original_dest;
}

// String comparison (first n characters)
int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

bool isdigit(int c) {
    return (c >= '0' && c <= '9');
}

bool isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

// String to double conversion =========================================

double atof(const char* str) {
    double value = 0.0;
    double fraction = 1.0;
    bool negative = false;
    bool found_decimal = false;

    // Skip whitespace
    while (*str == ' ' || *str == '\t') str++;

    // Handle sign
    if (*str == '-') {
        negative = true;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Process digits
    while (*str) {
        if (isdigit(*str)) {
            if (found_decimal) {
                fraction *= 0.1;
                value += (*str - '0') * fraction;
            } else {
                value = value * 10.0 + (*str - '0');
            }
        } else if (*str == '.') {
            if (found_decimal) break; // Second decimal point
            found_decimal = true;
        } else {
            break; // Invalid character
        }
        str++;
    }

    return negative ? -value : value;
}

// Memory Mapping Implementation =======================================

typedef struct {
    void* addr;
    size_t size;
    int prot;
    bool allocated;
} MemoryMapping;

#define MAX_MAPPINGS 256
static MemoryMapping mappings[MAX_MAPPINGS];
static size_t next_mapping = 0;

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
    (void)fd; (void)offset;
    
    if (next_mapping >= MAX_MAPPINGS) {
        return MAP_FAILED;
    }

    // Simple bump allocator for demo purposes
    void* allocated_addr = malloc(length);
    if (!allocated_addr) {
        return MAP_FAILED;
    }

    mappings[next_mapping] = (MemoryMapping){
        .addr = allocated_addr,
        .size = length,
        .prot = prot,
        .allocated = true
    };
    next_mapping++;

    return allocated_addr;
}

int munmap(void* addr, size_t length) {
    (void)length; // Simple implementation ignores length
    
    for (size_t i = 0; i < next_mapping; i++) {
        if (mappings[i].addr == addr && mappings[i].allocated) {
            free(addr);
            mappings[i].allocated = false;
            return 0; // Success
        }
    }
    return -1; // Failed to find mapping
}

void wrstr(const char *str) {
    printk(str);  // Use printk to write the entire string
}