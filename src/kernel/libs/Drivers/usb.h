#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <stdbool.h>

// PCI Constants
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// USB Host Controller Types
#define USB_HC_UHCI 0x00
#define USB_HC_OHCI 0x10
#define USB_HC_EHCI 0x20
#define USB_HC_XHCI 0x30

// USB Constants
#define USB_REQUEST_GET_DESCRIPTOR 0x06
#define USB_DESCRIPTOR_DEVICE      0x01
#define USB_DESCRIPTOR_CONFIG      0x02
#define USB_DESCRIPTOR_INTERFACE   0x04
#define USB_DESCRIPTOR_ENDPOINT    0x05

#define USB_HID_CLASS              0x03
#define USB_HID_SUBCLASS_BOOT      0x01
#define USB_HID_PROTOCOL_KEYBOARD  0x01

// Maximum size for a configuration descriptor
#define MAX_CONFIG_DESCRIPTOR_SIZE 256

// USB Device Structure
typedef struct {
    uint8_t address;
    uint8_t class_code;
    uint8_t subclass_code;
    uint8_t protocol_code;
    uint8_t num_endpoints;
    struct {
        uint8_t endpoint_address;
        uint8_t transfer_type;
        uint8_t direction;
        uint16_t max_packet_size;
        uint8_t polling_interval;
    } endpoints[16];
} usb_device_t;

// USB Host Controller Structure
typedef struct {
    uint32_t base_address;
    uint8_t type;
    bool initialized;
} usb_host_controller_t;

// Global USB Host Controller
extern usb_host_controller_t usb_hc;

// Function Prototypes

// Low-level I/O functions
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);

// PCI Configuration Space Access
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

// USB Host Controller Detection
bool detect_usb_host_controller();

// USB Control Transfer
bool usb_control_transfer(usb_device_t *device, uint8_t request_type, uint8_t request,
    uint16_t value, uint16_t index, void *data, uint16_t length);

// USB Descriptor Handling
bool usb_get_descriptor(usb_device_t *device, uint8_t descriptor_type, uint8_t index, void *buffer, uint16_t length);

// USB Device Enumeration
extern usb_device_t usb_devices[128];

int usb_enumerate_devices(usb_device_t *device_list, int max_devices);

// Main USB Driver Initialization
void usb_driver_init();

#endif // USB_H