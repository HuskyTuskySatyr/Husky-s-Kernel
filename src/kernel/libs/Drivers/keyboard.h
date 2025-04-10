#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "usb.h"
#include <stdint.h>
#include <stdbool.h>



// PS/2 Status Register Bits
#define PS2_STATUS_OUTPUT_FULL 0x01
#define PS2_STATUS_INPUT_FULL  0x02

// USB Keyboard Specific Constants
#define USB_KEYBOARD_INTERFACE_CLASS    USB_HID_CLASS
#define USB_KEYBOARD_INTERFACE_SUBCLASS USB_HID_SUBCLASS_BOOT
#define USB_KEYBOARD_INTERFACE_PROTOCOL USB_HID_PROTOCOL_KEYBOARD

// Global USB Keyboard Device
extern usb_device_t *usb_keyboard_device;

// Function Prototypes for PS/2 Keyboard
static bool ps2_keyboard_wait_for_input();
static uint8_t ps2_keyboard_read_data();
static void ps2_keyboard_init();

// Function Prototypes for USB Keyboard
static bool usb_keyboard_init();
static uint8_t usb_keyboard_read_key();

// Combined Keyboard Functions
void keyboard_init();        // Initializes both USB and PS/2 keyboards
uint8_t keyboard_read_key(); // Reads a key from either USB or PS/2 keyboard

#endif // KEYBOARD_H
