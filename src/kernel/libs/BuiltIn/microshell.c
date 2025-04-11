#include "microshell.h"
#include "../System/system.h"
#include "../Drivers/keyboard.h"
#include "../Drivers/kernel.h"
#include <stdarg.h>
#include <stdbool.h>

static msh_ctx_t ctx;
// Hardware I/O functions
extern uint16_t inw(uint16_t port);
extern void outw(uint16_t port, uint16_t value);
// Terminal functions
extern void terminal_writeline(const char* text);
extern void printk(const char *format, ...);
extern void terminal_putchar(char c);
// Utility functions
extern char* itoa(int value, char* str, int base);
extern char* strdup(const char* s);

// Redefinition again cuz of the order. (im avoiding changing makefile, cuz it breaks easily.)
char normal_to_shifted2[128] = {
    ['0'] = ')', ['1'] = '!', ['2'] = '@', ['3'] = '#', ['4'] = '$', 
    ['5'] = '%', ['6'] = '^', ['7'] = '&', ['8'] = '*', ['9'] = '(', 
    ['a'] = 'A', ['b'] = 'B', ['c'] = 'C', ['d'] = 'D', ['e'] = 'E', 
    ['f'] = 'F', ['g'] = 'G', ['h'] = 'H', ['i'] = 'I', ['j'] = 'J', 
    ['k'] = 'K', ['l'] = 'L', ['m'] = 'M', ['n'] = 'N', ['o'] = 'O', 
    ['p'] = 'P', ['q'] = 'Q', ['r'] = 'R', ['s'] = 'S', ['t'] = 'T', 
    ['u'] = 'U', ['v'] = 'V', ['w'] = 'W', ['x'] = 'X', ['y'] = 'Y', 
    ['z'] = 'Z', 
    ['-'] = '_', ['='] = '+', 
    ['['] = '{', [']'] = '}', 
    [';'] = ':', ['\''] = '"', 
    [','] = '<', ['.'] = '>', 
    ['/'] = '?', 
    ['\\'] = '|',
};


char* msh_read_line() {
    static char buf[256];
    size_t pos = 0;
    char shift = 0;

    while(pos < sizeof(buf)-1) {
        uint8_t key = keyboard_read_key(); // DIRECT keyboard.h call
        
        if(key == 0) continue; // No key pressed

        // Handle key exactly like your example
        if(key == 4) { // Enter
            buf[pos] = '\0';
            return strdup(buf);
        }
        else if(key == 2) { // Backspace
            if(pos > 0) {
                pos--;
                terminal_putchar('\b');
            }
        }
        else if(key == 6 || key == 7) { // Shift
            shift = 1;
        }
        else {
            if(shift) {
                key = normal_to_shifted2[key];
                shift = 0;
            }
            buf[pos++] = key;
            terminal_putchar(key); // Echo
        }
    }
    buf[sizeof(buf)-1] = '\0';
    return strdup(buf);
}

static msh_var_t builtin_input() {
    msh_var_t ret = {0};
    char* input = msh_read_line();
    if(input) {
        ret.type = MSH_TYPE_STR;
        ret.str_val = input;
    }
    return ret;
}

// Helper: Skip whitespace
static void skip_whitespace() {
    while (isspace(ctx.code[ctx.pc])) ctx.pc++;
}

// Helper: Parse identifier
static char* parse_ident() {
    skip_whitespace();
    static char buf[32];
    int i = 0;
    while (isalnum(ctx.code[ctx.pc]) || ctx.code[ctx.pc] == '_') {
        if (i < 31) buf[i++] = ctx.code[ctx.pc];
        ctx.pc++;
    }
    buf[i] = '\0';
    return buf;
}

// Helper: Find variable
static msh_var_t* find_var(const char* name) {
    for (uint32_t i = 0; i < ctx.var_count; i++) {
        if (strcmp(ctx.vars[i].name, name) == 0) {
            return &ctx.vars[i];
        }
    }
    return NULL;
}

static msh_var_t convert_var(const msh_var_t* var, msh_type_t target_type) {
    msh_var_t result = {0};
    result.type = target_type;
    
    switch(var->type) {
        case MSH_TYPE_INT:
            if(target_type == MSH_TYPE_STR) {
                result.str_val = malloc(32);
                if (result.str_val != NULL) {
                    itoa(var->int_val, result.str_val, 10);
                }
            } else if(target_type == MSH_TYPE_FLOAT) {
                result.float_val = (float)var->int_val;
            } else {
                result = *var;
            }
            break;
            
        case MSH_TYPE_FLOAT:
            if(target_type == MSH_TYPE_STR) {
                result.str_val = malloc(32);
                if (result.str_val != NULL) {
                    // Convert float to string
                    int int_part = (int)var->float_val;
                    int frac_part = (int)((var->float_val - int_part) * 1000);
                    if (frac_part < 0) frac_part = -frac_part;
                    
                    // First convert integer part
                    itoa(int_part, result.str_val, 10);
                    
                    // Find end of string
                    char* p = result.str_val;
                    while (*p) p++;
                    
                    // Add decimal point and fractional part
                    *p++ = '.';
                    itoa(frac_part, p, 10);
                }
            } else if(target_type == MSH_TYPE_INT) {
                result.int_val = (int)var->float_val;
            } else {
                result = *var;
            }
            break;
            
        case MSH_TYPE_STR:
            if(target_type == MSH_TYPE_INT) {
                result.int_val = atoi(var->str_val);
            } else if(target_type == MSH_TYPE_FLOAT) {
                result.float_val = atof(var->str_val);
            } else {
                result = *var;
            }
            break;
            
        default:
            result = *var;
            break;
    }
    
    return result;
}
// Helper: Evaluate expression
static msh_var_t eval_expr() {
    skip_whitespace();
    msh_var_t result = {0};
    msh_var_t left = {0};

    // Number literal
    if (isdigit(ctx.code[ctx.pc])) {
        result.type = MSH_TYPE_INT;
        result.int_val = 0;
        while (isdigit(ctx.code[ctx.pc])) {
            result.int_val = result.int_val * 10 + (ctx.code[ctx.pc] - '0');
            ctx.pc++;
        }
        left = result;
    }
    // String literal
    else if (ctx.code[ctx.pc] == '"') {
        ctx.pc++;
        char* start = (char*)&ctx.code[ctx.pc];
        while (ctx.code[ctx.pc] != '"' && ctx.code[ctx.pc] != '\0') ctx.pc++;
        if (ctx.code[ctx.pc] != '"') {
            ctx.had_error = true;
            return result;
        }
        size_t len = &ctx.code[ctx.pc] - start;
        result.str_val = malloc(len + 1);
        strncpy(result.str_val, start, len);
        result.str_val[len] = '\0';
        result.type = MSH_TYPE_STR;
        ctx.pc++;
        left = result;
    }
    // Variable reference
    else if (isalpha(ctx.code[ctx.pc])) {
        char* name = parse_ident();
        msh_var_t* var = find_var(name);
        if (var) {
            result = *var;
            if (result.type == MSH_TYPE_STR) {
                result.str_val = strdup(result.str_val);
            }
            left = result;
        } else {
            ctx.had_error = true;
        }
    }
    // Boolean literals
    else if (strncmp(&ctx.code[ctx.pc], "True", 4) == 0) {
        result.type = MSH_TYPE_BOOL;
        result.bool_val = true;
        ctx.pc += 4;
        left = result;
    }
    else if (strncmp(&ctx.code[ctx.pc], "False", 5) == 0) {
        result.type = MSH_TYPE_BOOL;
        result.bool_val = false;
        ctx.pc += 5;
        left = result;
    }
    // Hardware I/O: inw()
    else if (strncmp(&ctx.code[ctx.pc], "inw(", 4) == 0) {
        ctx.pc += 4;
        msh_var_t arg = eval_expr();
        if (arg.type == MSH_TYPE_INT && ctx.code[ctx.pc] == ')') {
            ctx.pc++;
            result.type = MSH_TYPE_INT;
            result.int_val = inw ? inw(arg.int_val) : 0;
            left = result;
        } else {
            ctx.had_error = true;
        }
    }
    // Hardware I/O: outw()
    else if (strncmp(&ctx.code[ctx.pc], "outw(", 5) == 0) {
        ctx.pc += 5;
        msh_var_t port = eval_expr();
        if (ctx.code[ctx.pc] != ',') {
            ctx.had_error = true;
        } else {
            ctx.pc++;
            msh_var_t value = eval_expr();
            if (port.type == MSH_TYPE_INT && value.type == MSH_TYPE_INT && ctx.code[ctx.pc] == ')') {
                ctx.pc++;
                if (outw) outw(port.int_val, value.int_val);
                result.type = MSH_TYPE_NONE;
                left = result;
            } else {
                ctx.had_error = true;
            }
        }
    }
    // input()
    else if (strncmp(&ctx.code[ctx.pc], "input(", 6) == 0) {
        ctx.pc += 6;
        if (ctx.code[ctx.pc] == ')') {
            ctx.pc++;
            return builtin_input();
        } else {
            msh_var_t prompt = eval_expr();
            if (prompt.type == MSH_TYPE_STR) {
                printk(prompt.str_val);
                if (prompt.str_val) free(prompt.str_val);
            }
            if (ctx.code[ctx.pc] != ')') {
                ctx.had_error = true;
                return (msh_var_t){0};
            }
            ctx.pc++;
            return builtin_input();
        }
    }

    skip_whitespace();
    char op = ctx.code[ctx.pc];

    if(op == '+' || op == '-' || op == '*' || op == '/') {
        ctx.pc++;
        msh_var_t right = eval_expr();
        msh_type_t common_type = left.type;

        if(left.type != right.type) {
            if(left.type == MSH_TYPE_FLOAT || right.type == MSH_TYPE_FLOAT) {
                common_type = MSH_TYPE_FLOAT;
                left = convert_var(&left, MSH_TYPE_FLOAT);
                right = convert_var(&right, MSH_TYPE_FLOAT);
            } else if(left.type == MSH_TYPE_INT || right.type == MSH_TYPE_INT) {
                common_type = MSH_TYPE_INT;
                left = convert_var(&left, MSH_TYPE_INT);
                right = convert_var(&right, MSH_TYPE_INT);
            }
        }

        result.type = common_type;

        switch(op) {
            case '+':
                if(common_type == MSH_TYPE_INT)
                    result.int_val = left.int_val + right.int_val;
                else if(common_type == MSH_TYPE_FLOAT)
                    result.float_val = left.float_val + right.float_val;
                else if(common_type == MSH_TYPE_STR) {
                    char* new_str = malloc(strlen(left.str_val) + strlen(right.str_val) + 1);
                    strcpy(new_str, left.str_val);
                    strcat(new_str, right.str_val);
                    result.str_val = new_str;
                }
                break;
            case '-':
                if(common_type == MSH_TYPE_INT)
                    result.int_val = left.int_val - right.int_val;
                else if(common_type == MSH_TYPE_FLOAT)
                    result.float_val = left.float_val - right.float_val;
                break;
            case '*':
                if(common_type == MSH_TYPE_INT)
                    result.int_val = left.int_val * right.int_val;
                else if(common_type == MSH_TYPE_FLOAT)
                    result.float_val = left.float_val * right.float_val;
                break;
            case '/':
                if(common_type == MSH_TYPE_INT && right.int_val != 0)
                    result.int_val = left.int_val / right.int_val;
                else if(common_type == MSH_TYPE_FLOAT && right.float_val != 0.0f)
                    result.float_val = left.float_val / right.float_val;
                else
                    ctx.had_error = true;
                break;
        }

        if(left.type != common_type && left.type == MSH_TYPE_STR && left.str_val)
            free(left.str_val);
        if(right.type != common_type && right.type == MSH_TYPE_STR && right.str_val)
            free(right.str_val);
    } else {
        result = left;
    }

    return result;
}

// Core execution
bool msh_execute(const char* code) {
    ctx.code = code;
    ctx.pc = 0;
    ctx.had_error = false;
    
    while (ctx.code[ctx.pc] != '\0' && !ctx.had_error) {
        skip_whitespace();
        
        // Variable assignment
        if (isalpha(ctx.code[ctx.pc])) {
            char* name = parse_ident();
            skip_whitespace();
            if (ctx.code[ctx.pc] == '=') {
                ctx.pc++;
                msh_var_t value = eval_expr();
                if (!ctx.had_error) {
                    msh_var_t* var = find_var(name);
                    if (!var) {
                        if (ctx.var_count < 64) {
                            var = &ctx.vars[ctx.var_count++];
                            strncpy(var->name, name, 31);
                        } else {
                            ctx.had_error = true;
                        }
                    }
                    if (var) {
                        // Free old string if needed
                        if (var->type == MSH_TYPE_STR && var->str_val) {
                            free(var->str_val);
                        }
                        *var = value;
                    }
                }
            }
        }
        // printk() statement
        else if (strncmp(&ctx.code[ctx.pc], "print(", 7) == 0) {
            ctx.pc += 7;
            msh_var_t arg = eval_expr();
            if (!ctx.had_error && printk) {
                if (arg.type == MSH_TYPE_STR) {
                    printk(arg.str_val);
                } else if (arg.type == MSH_TYPE_INT) {
                    char buf[32];
                    itoa(arg.int_val, buf, 10);
                    printk(buf);
                }
            }
            if (ctx.code[ctx.pc] != ')') ctx.had_error = true;
            else ctx.pc++;
        }
        // if statement
        else if (strncmp(&ctx.code[ctx.pc], "if ", 3) == 0) {
            ctx.pc += 3;
            msh_var_t cond = eval_expr();
            if (cond.type != MSH_TYPE_BOOL) ctx.had_error = true;
            skip_whitespace();
            if (ctx.code[ctx.pc] != ':') ctx.had_error = true;
            else ctx.pc++;
            
            if (!ctx.had_error && cond.bool_val) {
                // Execute indented block
                while (isspace(ctx.code[ctx.pc])) ctx.pc++;
                uint32_t block_start = ctx.pc;
                while (ctx.code[ctx.pc] != '\0' && 
                       (ctx.code[ctx.pc] != '\n' || !isspace(ctx.code[ctx.pc+1]))) {
                    ctx.pc++;
                }
                char* block = strndup(&ctx.code[block_start], ctx.pc - block_start);
                msh_execute(block);
                free(block);
            }
        }
        // while statement (similar to if)
        else if (strncmp(&ctx.code[ctx.pc], "while ", 6) == 0) {
            ctx.pc += 6;
            uint32_t cond_start = ctx.pc;
            msh_var_t cond = eval_expr();
            if (cond.type != MSH_TYPE_BOOL) ctx.had_error = true;
            skip_whitespace();
            if (ctx.code[ctx.pc] != ':') ctx.had_error = true;
            else ctx.pc++;
            
            while (!ctx.had_error && cond.bool_val) {
                // Execute indented block
                while (isspace(ctx.code[ctx.pc])) ctx.pc++;
                uint32_t block_start = ctx.pc;
                while (ctx.code[ctx.pc] != '\0' && 
                       (ctx.code[ctx.pc] != '\n' || !isspace(ctx.code[ctx.pc+1]))) {
                    ctx.pc++;
                }
                char* block = strndup(&ctx.code[block_start], ctx.pc - block_start);
                msh_execute(block);
                free(block);
                
                // Re-evaluate condition
                ctx.pc = cond_start;
                cond = eval_expr();
                skip_whitespace();
                if (ctx.code[ctx.pc] != ':') ctx.had_error = true;
                else ctx.pc++;
            }
        }
        else {
            ctx.pc++;
        }
    }
    
    return !ctx.had_error;
}

void msh_init(void) {
    memset(&ctx, 0, sizeof(ctx));
}