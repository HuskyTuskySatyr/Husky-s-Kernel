#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "libs/Drivers/kernel.h"
#include "libs/Drivers/keyboard.h"
#include "libs/Drivers/usb.h"
#include "libs/Drivers/disk.h"
#include "libs/Drivers/pci.h"
#include "libs/System/system.h"
#include "libs/BuiltIn/microshell.h"
#include "libs/BuiltIn/keyboard_layouts.h"

extern int initAcpi(void);       
extern void acpiPowerOff(void);  
extern int check_type(HBA_PORT *port);

// KEYBOARD LAYOUT CHANGE IF NECESSERY
KeyboardLayout current_layout = LAYOUT_US;

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
bool ctrl = false;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xB8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) 
{
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
        if (terminal_row == VGA_HEIGHT) {
            terminal_row = 0; // Reset to top if screen overflows
        }
    }
    else if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--; // Move back one column
        } else if (terminal_row > 0) {
            terminal_row--;
            terminal_column = VGA_WIDTH - 1; // Move to end of previous row
        }
        terminal_putentryat(' ', terminal_color, terminal_column, terminal_row); // Clear the character
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT) {
                terminal_row = 0;
            }
        }
    }
}


void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}

void terminal_writeline(const char* data) 
{
	terminal_write(data, strlen(data));
}

char us_normal_to_shifted[128] = {
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


void printk_hex(uint8_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    char hex_string[5] = "0x00"; // Format: 0xHH

    hex_string[2] = hex_chars[(value >> 4) & 0x0F]; // High nibble
    hex_string[3] = hex_chars[value & 0x0F]; // Low nibble

    terminal_writestring(hex_string);
}

uint8_t terminal_input(char* input_buffer, size_t* buffer_index, char* shift) {
    uint8_t key;
    key = keyboard_read_key();

    if (key != 0) {
        // Handle special keys like Enter and Backspace
        if (key == 4) { // Enter key
            input_buffer[*buffer_index] = '\0'; // Null-terminate the string
            *shift = 0;
            return 1; // Return 1 to signal that we should process the input
        } else if (key == 2) { // Backspace key
            *shift = 0;
            if (*buffer_index > 0) {
                (*buffer_index)--; // Decrease the buffer index to erase the last character
                terminal_putchar('\b'); // Remove one char
            }
        } else if (key == 1) { // Escape key
            *shift = 0;
            if (*buffer_index > 0) {
                // Clear input buffer or handle escape key logic
            }
        } else if (key == 5) { // LCtrl key
            *shift = 0;
            if (*buffer_index > 0) {
                // Handle control key logic
            }
        } else if (key == 6 || key == 7) { // Shift key
            *shift = 1; // Enable shift mode
        } else {
            // Convert key to appropriate layout
            char input_char;
            if (*shift != 0) {
                // Use the shifted layout
                input_char = shifted_layout[current_layout][key];
            } else {
                // Use the normal layout
                input_char = us_ascii_to_layout[current_layout][key];
            }

            input_buffer[*buffer_index] = input_char;  // Store the character in the buffer
            (*buffer_index)++;
            terminal_putchar(input_char); // Print the key
            *shift = 0;  // Reset shift state
        }
    }
    return 0;
}

unsigned char inb(unsigned short port) {
    unsigned char result;
    // Inline assembly for reading a byte from a port (x86)
    __asm__ __volatile__ ("inb %1, %0" : "=a"(result) : "d"(port));
    return result;
}

void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

struct ahci_device_list {
    struct pci_device *pci_device;
    int length;
};

int MAX_DEVICES = 128;

HBA_PORT* get_hba_port(uint32_t bus, uint32_t device, uint32_t function)
{
    #define PROT_READ  0x1  // Page can be read
    #define PROT_WRITE 0x2  // Page can be written
    #define MAP_SHARED 0x01 // Changes are visible to other processes
    // Read the PCI config space to get the BAR address (Base Address Register)
    void* pci_config = pci_read_config_space(bus, device, function, 0x10);
    uint32_t bar_address = *((uint32_t*)pci_config);  // BAR0 address for this example

    // Map the memory for the HBA
    HBA_PORT* hba_port = (HBA_PORT*)mmap(NULL, sizeof(HBA_PORT), PROT_READ | PROT_WRITE, MAP_SHARED, -1, bar_address);

    return hba_port;
}


struct pci_device_list *get_ahci_devices(struct pci_device_list device_list) {
    // Count how many AHCI devices there are
    int ahci_count = 0;
    for (int i = 0; i < device_list.length; ++i) {
        struct pci_device *dev = &device_list.pci_device[i];
        // Check if the device is AHCI using check_type function
        if (check_type(get_hba_port(dev->bus,dev->device,dev->function))) {
            ahci_count++;
        }
    }
    printk(ahci_count);
    // Allocate memory for the AHCI devices
    struct pci_device *ahci_devices = malloc(ahci_count * sizeof(struct pci_device));

    // Populate the AHCI devices list
    int index = 0;
    for (int i = 0; i < device_list.length; ++i) {
        printk(i);
        struct pci_device *dev = &device_list.pci_device[i];
        if (check_type(get_hba_port(dev->bus,dev->device,dev->function))) {
            ahci_devices[index] = *dev; // Copy the AHCI device to the list
            index++;
        }
    }

    // Return the list of AHCI devices
    struct pci_device_list ahci_device_list = {
        .pci_device = ahci_devices,
        .length = ahci_count
    };
    struct pci_device_list *val = &ahci_device_list;
    return val;
}

#define EDITOR_BUFFER_SIZE 4096
static char editor_buffer[EDITOR_BUFFER_SIZE];
#define KEY_CTRL_Q 16  // Typically Ctrl+Q
#define KEY_CTRL_R 18  // Typically Ctrl+R

// Editor state structure
typedef struct {
    char buffer[EDITOR_BUFFER_SIZE];
    size_t position;
    size_t cursor_x;
    size_t cursor_y;
    bool is_active;
} EditorState;

static EditorState editor = {0};
static size_t editor_buffer_pos = 0;

// Editor state tracking
enum editor_state {
    EDITOR_INACTIVE,
    EDITOR_ACTIVE,
    EDITOR_EXECUTING
};
static enum editor_state current_editor_state = EDITOR_INACTIVE;

// Helper function to clear the editor buffer
void editor_clear_buffer() {
    memset(editor_buffer, 0, EDITOR_BUFFER_SIZE);
    editor_buffer_pos = 0;
}

// Editor input handler
void editor_init() {
    memset(editor.buffer, 0, EDITOR_BUFFER_SIZE);
    editor.position = 0;
    editor.cursor_x = 0;
    editor.cursor_y = 0;
    editor.is_active = false;
}

// Editor input handler
void editor_handle_input() {
    uint8_t key = keyboard_read_key();
    char* shifted;
    
    if (key == 0) {
        return;  // No key pressed
    }

    // Handle CTRL key combinations first
    else if (key == 5) {  // CTRL key pressed
        ctrl = true;
        *shifted = false;
        return;
    }
    else if (key == 6 || key == 7) { // Shift key
        *shifted = 1; // Enable shift mode
        ctrl = false;
    }
    else if (ctrl) {
        if (*shifted != 0) {
            // Use the shifted layout
            key = shifted_layout[current_layout][key];
        } else {
            // Use the normal layout
            key = us_ascii_to_layout[current_layout][key];
        }
        if (key == 'q') {  // CTRL+Q to quit
            editor.is_active = false;
            ctrl = false;
            *shifted = false;
            terminal_writestring("\nExited editor. Buffer contents saved.\n");
            return;
        }
        else if (key == 'r') {  // CTRL+R to run
            editor.is_active = false;
            ctrl = false;
            *shifted = false;
            terminal_writestring("\nExecuting code...\n");
            execute_buffer();
            return;
        }
        ctrl = false;  // If not a recognized CTRL combo
    }

    // Handle normal input
    else if (key == 4) {  // Enter
        
        editor.buffer[editor.position++] = '\n';
        editor.cursor_x = 0;
        editor.cursor_y++;
        *shifted = false;
    } 
    else if (key == 2) {  // Backspace
        *shifted = false;
        if (editor.position > 0) {
            // Remove the character from buffer
            editor.position--;
            editor.buffer[editor.position] = '\0';  // Null-terminate
            
            // Update cursor position
            if (editor.buffer[editor.position] == '\n') {
                editor.cursor_y--;
                // Calculate previous line length
                size_t i = editor.position;
                while (i > 0 && editor.buffer[i-1] != '\n') i--;
                editor.cursor_x = editor.position - i;
            } 
            else {
                editor.cursor_x--;
            }
            
            // Visual feedback
            terminal_putchar('\b');
            terminal_putchar(' ');
            terminal_putchar('\b');
            return;  // Skip full redraw for backspace
        }
    } 
    else if (key < 128) {  // Regular character
        if (*shifted != 0) {
            // Use the shifted layout
            key = shifted_layout[current_layout][key];
        } else {
            // Use the normal layout
            key = us_ascii_to_layout[current_layout][key];
        }
        *shifted = false;
        editor.buffer[editor.position++] = key;
        editor.cursor_x++;
    }
    
    // Full redraw (except for backspace which handled it already)
    if (key != 2) {
        terminal_initialize();
        terminal_writestring(editor.buffer);
        terminal_row = editor.cursor_y;
        terminal_column = editor.cursor_x;
    }
}

// Simple executor (very basic - just displays the content for now)
void execute_buffer() {
    terminal_writestring("\n--- Executing Program ---\n");

    // Ensure proper null-termination
    size_t exec_len = editor_buffer_pos;
    if (exec_len >= EDITOR_BUFFER_SIZE) {
        exec_len = EDITOR_BUFFER_SIZE - 1;
    }
    editor_buffer[exec_len] = '\0';

    bool overall_success = true;
    char* current = editor_buffer;
    char* line_start = editor_buffer;
    int in_line = 0;

    // Manual line-by-line processing
    while (*current != '\0') {
        if (*current == '\n') {
            *current = '\0';  // Temporarily terminate line
            
            // Only execute non-empty lines
            if (line_start < current) {
                terminal_writestring("> ");
                terminal_writestring(line_start);
                terminal_putchar('\n');
                
                bool line_success = msh_execute(line_start);
                overall_success = overall_success && line_success;
                
                if (!line_success) {
                    terminal_writestring("! Error in this line\n");
                    break;
                }
            }
            
            *current = '\n';  // Restore newline
            line_start = current + 1;
        }
        current++;
    }

    // Execute the last line if no trailing newline
    if (line_start < current) {
        terminal_writestring("> ");
        terminal_writestring(line_start);
        terminal_putchar('\n');
        
        overall_success = msh_execute(line_start) && overall_success;
    }

    terminal_writestring(overall_success ? 
        "\n--- Execution Successful ---\n" :
        "\n--- Execution Failed ---\n");
}

int split_command(char *command, char **args) {
    int arg_count = 0;
    char *token = strtok(command, " ");  // Split by spaces

    while (token != NULL) {
        args[arg_count++] = token;  // Store each argument in the array
        token = strtok(NULL, " ");  // Get the next token
    }

    return arg_count;
}

// Function to handle the terminal prompt and process commands
void terminal_prompt() {
    char command[256];  // Command buffer
    size_t buffer_index = 0; 
    char *args[10];  // Array to hold the arguments (supports up to 10 arguments)

    terminal_writestring("Terminal ready. Type 'help' for a list of commands.\n");

    while (1) {
        terminal_writestring("\n>>> ");
        buffer_index = 0;
        char* shift = 0;

        // Get user input
        while (!terminal_input(command, &buffer_index, shift));

        // Split the command into arguments
        int arg_count = split_command(command, args);

        if (arg_count == 0) continue;  // No command, continue loop

        // Process the first argument (command name)
        if (strcmp(args[0], "help") == 0) {
            printk("\nAvailable commands:\n");
            printk("  help            - Show this help message\n");
            printk("  lsdisks         - Shows the disks\n");
            printk("  reboot          - Reboot the system\n");
            printk("  shutdown        - Exit the terminal\n");
            printk("  edit            - Start the program editor\n");
            printk("  run             - Execute the current program\n");
            printk("  cl              - Change Layout\n");
            printk("  getkey          - Shows keycode pressed\n");
        }
        else if (strcmp(args[0], "cl") == 0) {
            // Handle the 'cl' command to change layout
            if (arg_count > 1) {  // Ensure there's a second argument
                if (strcmp(args[1], "en_us") == 0) {
                    current_layout = LAYOUT_US;
                } else if (strcmp(args[1], "de_de") == 0) {
                    current_layout = LAYOUT_DE;
                } else if (strcmp(args[1], "fr_fr") == 0) {
                    current_layout = LAYOUT_FR;
                } else if (strcmp(args[1], "es_es") == 0) {
                    current_layout = LAYOUT_ES;
                } else if (strcmp(args[1], "ru_ru") == 0) {
                    current_layout = LAYOUT_RU;
                } else {
                    terminal_writestring("Invalid layout. Available layouts: en_us, de_de, fr_fr, es_es, ru_ru\n");
                    continue;
                }
                terminal_writestring("Layout changed successfully.\n");
            } else {
                terminal_writestring("Usage: cl <layout_name>\n");
            }
        }
        else if (strcmp(args[0], "getkey") == 0) {
            // Handle the 'cl' command to change layout
            if (arg_count > 1) {  // Ensure there's a second argument
                printk(args[1]);
            } else {
                terminal_writestring("Usage: getkey <key>\n");
            }
        }
        else if (strcmp(args[0], "edit") == 0) {
            editor.is_active = true;
            terminal_writestring("Editor started. LCtrl+Q to quit, LCtrl+R to run\n");
            while (editor.is_active == true) {
                editor_handle_input();
            }
        }
        else if (strcmp(args[0], "run") == 0) {
            if (editor_buffer_pos > 0) {
                execute_buffer();
            } else {
                terminal_writestring("No program to execute. Use 'edit' first.\n");
            }
        }
        else if (strcmp(args[0], "reboot") == 0) {
            if (initAcpi() == 0) {
                printk("ACPI initialization successfully.\n");
                acpiPowerOff();
            } else {
                printk("ACPI initialization failed. Unable to reboot.\n");
            }
            break;
        }
        else if (strcmp(args[0], "lsdisks") == 0) {
            struct pci_device_list pci_devices = pci_enumerate_devices();
            printk("Gotten disk list, checking type.\n");
            struct pci_device_list *ahci_devices = get_ahci_devices(pci_devices);

            for (int i = 0; i < ahci_devices->length; ++i) {
                printk(ahci_devices->pci_device->device_id);
            }
        }
        else if (strcmp(args[0], "shutdown") == 0) {
            if (initAcpi() == 0) {
                printk("ACPI initialization successfully.\n");
                acpiPowerOff();
            } else {
                printk("ACPI initialization failed. Unable to shutdown.\n");
            }
            break;
        }
        else {
            terminal_writestring("Unknown command: '");
            terminal_writestring(command);
            terminal_writestring("'. Type 'help' for a list of commands.\n");
        }
    }
}

void kernel_main(void) 
{
	terminal_initialize();
    editor_init();
    msh_init();
    terminal_prompt();
}
