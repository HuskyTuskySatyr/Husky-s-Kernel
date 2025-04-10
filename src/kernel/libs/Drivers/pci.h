#ifndef PCI_H
#define PCI_H

struct pci_device {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint32_t port;  // Base address (or I/O port) for the device
};

struct pci_device_list {
    struct pci_device *pci_device;
    int length;
};

struct pci_device_list pci_enumerate_devices();
uint32_t pci_read_config_space(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

#endif // SYSTEM_H
