#ifndef MICROSHELL_H
#define MICROSHELL_H

#include <stdbool.h>
#include <stdint.h>

// Value types
typedef enum {
    MSH_TYPE_NONE,
    MSH_TYPE_INT,
    MSH_TYPE_FLOAT,
    MSH_TYPE_STR,
    MSH_TYPE_BOOL
} msh_type_t;

// Variable structure
typedef struct {
    char name[32];
    msh_type_t type;
    union {
        int int_val;
        float float_val;
        char* str_val;
        bool bool_val;
    };
} msh_var_t;

// Execution context
typedef struct {
    msh_var_t vars[64];  // Fixed-size variable table
    uint32_t var_count;
    const char* code;
    uint32_t pc;
    bool had_error;
} msh_ctx_t;

// Forward declarations for static functions
static void skip_whitespace();
static char* parse_ident();
static msh_var_t* find_var(const char* name);
static msh_var_t eval_expr();
static msh_var_t convert_var(const msh_var_t* var, msh_type_t target_type);
static msh_var_t builtin_input();

// Keyboard functions (ensure these are declared in the keyboard.h)
typedef uint8_t (*msh_keyread_fn)(void);

// Add to public API
void msh_register_input(char* (*input_fn)(void));

// Public API
void msh_init(void);
bool msh_execute(const char* code);
void msh_register_printk(void (*printk_fn)(const char*));

#endif // MICROSHELL_H
