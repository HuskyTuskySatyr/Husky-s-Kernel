#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "libs/Drivers/kernel.h"
#include "libs/Drivers/keyboard.h"
#include "libs/Drivers/usb.h"
#include "libs/Drivers/disk.h"
#include "libs/Drivers/pci.h"
#include "libs/System/system.h"

extern int initAcpi(void);       
extern void acpiPowerOff(void);  
extern int check_type(HBA_PORT *port);

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

char normal_to_shifted[128] = {
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

// Max command length
uint8_t terminal_input(char* input_buffer, size_t* buffer_index, char* shift) {
    uint8_t key;
    key = keyboard_read_key();

    if (key != 0) {
        // Handle special keys like Enter and Backspace
        if (key == 4) { // Enter key
            input_buffer[*buffer_index] = '\0'; // Null-terminate the string
            shift = 0;
            return 1; // Return 1 to signal that we should process the input
        } else if (key == 2) { // Backspace key
            shift = 0;
            if (*buffer_index > 0) {
                (*buffer_index)--; // Decrease the buffer index to erase the last character
                terminal_putchar('\b'); // Remove one char
            }
        } else if (key == 1) { // Escape key
            shift = 0;
            if (*buffer_index > 0) {
                
            }
        } else if (key == 5) { // LCtrl key
            shift = 0;
            if (*buffer_index > 0) {
            }
        } else if (key == 6){ // Shift key
            shift = 1;
        } else if (key == 7){ // Shift key
            shift = 1;
        }
        else {
            if (shift!=0) {
                key=normal_to_shifted[key];
            }
            input_buffer[*buffer_index] = key;  // Store the character in the buffer
            (*buffer_index)++;
            terminal_putchar(key); // Print the key
            shift = 0;
        }
    }
    return 0;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    asm volatile("inw %1, %0" : "=a"(result) : "d"(port));
    return result;
}

static inline void outw(uint16_t port, uint16_t value) {
    asm volatile("outw %0, %1" : : "a"(value), "d"(port));
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

// Function to handle the terminal prompt and process commands
void terminal_prompt() {
    char command[256]; // Command buffer
    size_t buffer_index = 0; 

    terminal_writestring("Terminal ready. Type 'help' for a list of commands.\n");

    while (1) {
        terminal_writestring("\n>>> ");
        buffer_index = 0;
        char* shift = 0;
        // Get user input
        while (!terminal_input(command, &buffer_index, shift));

        if (strcmp(command, "help") == 0) {
            printk("\nAvailable commands:\n");
            printk("  help            - Show this help message\n");
            printk("  lsdisks         - Shows the disks");
            printk("  reboot          - Show the next keypress\n");
            printk("  shutdown        - Exit the terminal\n");
        } 
        else if (strcmp(command, "reboot") == 0) {
            if (initAcpi() == 0) {
                printk("ACPI initialization successfully.\n");
                acpiPowerOff();
            } else {
                printk("ACPI initialization failed. Unable to boot.\n");
            }
            break;
        } else if (strcmp(command, "lsdisks") == 0) {
            struct pci_device_list pci_devices = pci_enumerate_devices();
            printk("Gotten disk list, checking type.\n");
            struct pci_device_list *ahci_devices = get_ahci_devices(pci_devices);

            for (int i = 0; i < ahci_devices->length; ++i) {
                printk(ahci_devices->pci_device->device_id);
            }
            
        }
        else if (strcmp(command, "shutdown") == 0) {
            if (initAcpi() == 0) {
                printk("ACPI initialization successfully.\n");
                acpiPowerOff();
            } else {
                printk("ACPI initialization failed. Unable to boot.\n");
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

    terminal_prompt();
    
}
