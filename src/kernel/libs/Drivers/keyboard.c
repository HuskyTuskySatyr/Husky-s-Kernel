#include "usb.h"
#include "kernel.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// PS/2 Keyboard Constants
#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

// PS/2 Status Register Bits
#define PS2_STATUS_OUTPUT_FULL 0x01
#define PS2_STATUS_INPUT_FULL  0x02

// USB Keyboard Specific Constants
#define USB_KEYBOARD_INTERFACE_CLASS    USB_HID_CLASS
#define USB_KEYBOARD_INTERFACE_SUBCLASS USB_HID_SUBCLASS_BOOT
#define USB_KEYBOARD_INTERFACE_PROTOCOL USB_HID_PROTOCOL_KEYBOARD
#define MAX_USB_DEVICES 128

// Global USB Keyboard Device
static usb_device_t *usb_keyboard_device = NULL;
int max_devices = 128;
usb_device_t usb_devices[128];

// Function Prototypes
static bool ps2_keyboard_wait_for_input();
static uint8_t ps2_keyboard_read_data();
static void ps2_keyboard_init();
static bool usb_keyboard_init();
static uint8_t usb_keyboard_read_key();
int usb_enumerate_devices(usb_devices, max_devices);

// PS/2 Keyboard Functions

// Wait for the PS/2 controller to have data available
static bool ps2_keyboard_wait_for_input() {
    uint16_t timeout = 10000; // Timeout after 10000 attempts
    while (timeout--) {
        if (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) {
            return true;
        }
    }
    return false;
}

char scancode_to_ascii[128] = {
    [0x01] = 1, // Escape
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4',
    [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8',
    [0x0A] = '9', [0x0B] = '0', [0x0C] = '-', [0x0D] = '=',
    [0x0E] = 2, // Backspace
    [0x0F] = '  ', // Tab
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r',
    [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
    [0x18] = 'o', [0x19] = 'p', [0x1A] = '[', [0x1B] = ']',
    [0x1C] = 4, // Enter
    [0x1D] = 5,    // Left Ctrl
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f',
    [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k',
    [0x26] = 'l', [0x27] = ';', [0x28] = '\'', [0x29] = '`',
    [0x2A] = 6,    // Left Shift
    [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v',
    [0x30] = 'b', [0x31] = 'n', [0x32] = 'm', [0x33] = ',',
    [0x34] = '.', [0x35] = '/', [0x36] = 7, // Right Shift
    [0x37] = '*', [0x38] = 8, // Left Alt
    [0x39] = ' ', // Space
};
// Read a byte from the PS/2 data port
static uint8_t ps2_keyboard_read_data() {
    if (ps2_keyboard_wait_for_input()) {
        return scancode_to_ascii[inb(PS2_DATA_PORT)];
    }
    return 0;
}

// Initialize the PS/2 keyboard
static void ps2_keyboard_init() {
    // Disable PS/2 devices
    outb(PS2_COMMAND_PORT, 0xAD);
    outb(PS2_COMMAND_PORT, 0xA7);

    // Flush the output buffer
    while (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) {
        inb(PS2_DATA_PORT);
    }

    // Enable PS/2 devices
    outb(PS2_COMMAND_PORT, 0xAE);
    outb(PS2_COMMAND_PORT, 0xA8);

    // Enable keyboard interrupts
    outb(PS2_COMMAND_PORT, 0x20);
    uint8_t status = inb(PS2_DATA_PORT) | 0x01;
    outb(PS2_COMMAND_PORT, 0x60);
    outb(PS2_DATA_PORT, status);

    printk("PS/2 Keyboard Initialized\n");
}

// USB Keyboard Functions

// Initialize the USB keyboard
static bool usb_keyboard_init() {
    for (uint8_t i = 0; i < 256; i++) {
        usb_device_t *device = &usb_devices[i];
        if (device->class_code == USB_KEYBOARD_INTERFACE_CLASS &&
            device->subclass_code == USB_KEYBOARD_INTERFACE_SUBCLASS &&
            device->protocol_code == USB_KEYBOARD_INTERFACE_PROTOCOL) {
            usb_keyboard_device = device;
            printk("USB Keyboard Found at Address: %d\n", device->address);
            return true;
        }
    }
    printk("No USB Keyboard Found\n");
    return false;
}

// Read a key from the USB keyboard
static uint8_t usb_keyboard_read_key() {
    if (!usb_keyboard_device) {
        return 0;
    }

    uint8_t buffer[8];
    if (usb_control_transfer(usb_keyboard_device, 0x81, 0x06, 0x0100, 0x00, buffer, 8)) {
        return buffer[2]; // Return the keycode from the HID report
    }
    return 0;
}

// Combined Keyboard Functions

// Initialize both USB and PS/2 keyboards
void keyboard_init() {
    ps2_keyboard_init();
    if (!usb_keyboard_init()) {
        printk("Falling back to PS/2 Keyboard\n");
    }
}

// Read a key from either USB or PS/2 keyboard
uint8_t keyboard_read_key() {
    if (usb_keyboard_device) {
        return usb_keyboard_read_key();
    } else {
        return ps2_keyboard_read_data();
    }
}