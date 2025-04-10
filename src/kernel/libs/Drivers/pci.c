#include <stdint.h>
#include "pci.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// Function to write to PCI configuration address port
void pci_write_config_address(uint32_t address) {
    outl(PCI_CONFIG_ADDRESS, address);
}

uint32_t inl(uint32_t port) {
    uint32_t value;
    asm volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void outl(uint16_t port, uint32_t value) {
    asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

// Function to read from PCI configuration data port
uint32_t pci_read_config_data() {
    return inl(PCI_CONFIG_DATA);
}

uint32_t pci_read_config_space(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    __asm__ volatile ("outl %0, %1" :: "a"(address), "Nd"(PCI_CONFIG_ADDRESS));
    uint32_t data;
    __asm__ volatile ("inl %1, %0" : "=a"(data) : "Nd"(PCI_CONFIG_DATA));
    return data;
}


#define MAX_PCI_DEVICES 128 // Define a maximum number of devices we can handle

// Function to enumerate all PCI devices
// Function to enumerate PCI devices and retrieve their base address or port
struct pci_device_list pci_enumerate_devices() {
    struct pci_device_list dev_list;
    dev_list.pci_device = malloc(MAX_PCI_DEVICES * sizeof(struct pci_device));

    // Check if memory allocation was successful
    if (!dev_list.pci_device) {
        printk("Error: Failed to allocate memory for PCI devices list.\n");
        dev_list.length = 0;
        return dev_list;
    }

    int device_count = 0;
    uint32_t vendor_device_id, vendor_id, device_id, base_address;

    printk("Starting PCI enumeration...\n");

    // Iterate over all buses, devices, and functions
    for (uint8_t bus = 0; bus < 256; bus++) {
        if (bus == 256) {
            printk("Bus enumeration finished.\n");
            break;  // Break the outer loop once bus reaches 256
        }
        for (uint8_t device = 0; device < 32; device++) {
            for (uint8_t function = 0; function < 8; function++) {
                // Build the PCI configuration address
                uint32_t address = (bus << 16) | (device << 11) | (function << 8) | 0x80000000;
                pci_write_config_address(address);

                // Read vendor and device ID
                vendor_device_id = pci_read_config_data();

                // If we reach an invalid response, skip this device
                if (vendor_device_id == 0xFFFFFFFF) {
                    // Skip invalid device and continue to the next one
                    continue;
                }

                vendor_id = vendor_device_id & 0xFFFF;
                device_id = (vendor_device_id >> 16) & 0xFFFF;

                // Check for valid vendor and device ID
                if (vendor_id != 0xFFFF && device_id != 0xFFFF) {
                    // Found a valid device, store its details
                    if (device_count < MAX_PCI_DEVICES) {
                        dev_list.pci_device[device_count].bus = bus;
                        dev_list.pci_device[device_count].device = device;
                        dev_list.pci_device[device_count].function = function;
                        dev_list.pci_device[device_count].vendor_id = vendor_id;
                        dev_list.pci_device[device_count].device_id = device_id;

                        // Retrieve base address or I/O port of the device
                        base_address = pci_read_config_data();  // Read BAR0
                        if (base_address != 0) {
                            dev_list.pci_device[device_count].port = base_address;
                        } else {
                            printk("Warning: Base address for device %d:%d:%d is invalid.\n", bus, device, function);
                        }

                        device_count++;
                    } else {
                        printk("Error: Exceeded maximum number of PCI devices (%d).\n", MAX_PCI_DEVICES);
                        break;
                    }
                }
            }
        }

        // If we've reached the maximum device count, break out of the bus loop
        if (device_count >= MAX_PCI_DEVICES) {
            break;
        }
    }

    dev_list.length = device_count;
    printk("PCI enumeration finished. %d devices found.\n", device_count);

    return dev_list;
}

// I/O port access functions (these would need to be implemented for your platform)

