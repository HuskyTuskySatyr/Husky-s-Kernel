#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "usb.h"
#include "kernel.h"

// Global USB Host Controller
usb_host_controller_t usb_hc;
usb_device_t usb_devicesclass[128];

// Function to read a byte from a port


// Function to read a 32-bit value from PCI configuration space
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outb(PCI_CONFIG_ADDRESS, address);
    return inb(PCI_CONFIG_DATA);
}

// Function to detect the USB host controller
bool detect_usb_host_controller() {
    // Scan PCI devices for a USB host controller
    for (uint8_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor_device = pci_read_config(bus, slot, func, 0x00);
                uint16_t vendor_id = vendor_device & 0xFFFF;
                uint16_t device_id = vendor_device >> 16;

                // Check if this is a USB host controller
                if (vendor_id == 0xFFFF) continue; // Invalid vendor ID
                uint8_t class_code = pci_read_config(bus, slot, func, 0x08) >> 24;
                uint8_t subclass_code = pci_read_config(bus, slot, func, 0x08) >> 16 & 0xFF;

                if (class_code == 0x0C && subclass_code == 0x03) { // USB class and subclass
                    uint8_t prog_if = pci_read_config(bus, slot, func, 0x08) >> 8 & 0xFF;
                    usb_hc.type = prog_if & 0xF0; // Determine the type (UHCI, OHCI, EHCI, xHCI)
                    usb_hc.base_address = pci_read_config(bus, slot, func, 0x10) & 0xFFFFFFF0; // Base address
                    usb_hc.initialized = true;
                    printk("USB Host Controller detected: ");
                    return true;
                }
            }
        }
    }
    return false;
}

// Function to perform a USB control transfer
bool usb_control_transfer(usb_device_t *device, uint8_t request_type, uint8_t request,
                          uint16_t value, uint16_t index, void *data, uint16_t length) {
    // Placeholder for control transfer logic
    printk("Control transfer to device ");
    (void)request_type; (void)request; (void)value; (void)index; (void)data; (void)length; // Suppress unused parameter warnings
    return true; // Return true if successful
}

// Function to read a USB device descriptor
bool usb_get_descriptor(usb_device_t *device, uint8_t descriptor_type, uint8_t index, void *buffer, uint16_t length) {
    // Send a GET_DESCRIPTOR request
    uint8_t request_type = 0x80; // IN, standard, device
    return usb_control_transfer(device, request_type, USB_REQUEST_GET_DESCRIPTOR, (descriptor_type << 8) | index, 0, buffer, length);
}

// Function to enumerate USB devices
int usb_enumerate_devices(usb_device_t *device_list, int max_devices) {
    int device_count = 0;

    for (uint8_t address = 1; address <= 127 && device_count < max_devices; address++) {
        usb_device_t device = {0};
        device.address = address;

        uint8_t device_descriptor[18];
        if (!usb_get_descriptor(&device, USB_DESCRIPTOR_DEVICE, 0, device_descriptor, sizeof(device_descriptor))) {
            continue; // No device found
        }

        device.class_code = device_descriptor[4];
        device.subclass_code = device_descriptor[5];
        device.protocol_code = device_descriptor[6];

        // Store device in the list
        device_list[device_count++] = device;
    }

    return device_count; // Return number of detected devices
}
// Main USB driver initialization
void usb_driver_init() {
    if (!detect_usb_host_controller()) {
        printk("No USB Host Controller found!");
        return;
    }

    if (usb_enumerate_devices(usb_devicesclass, 128) != 0) {
        printk("USB Keyboard detected!");
    } else {
        printk("No USB Keyboard found.");
    }
}