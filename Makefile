# Compiler and assembler
AS = i686-elf-as
CC = i686-elf-gcc
LD = i686-elf-gcc
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -ffreestanding -O2 -nostdlib

# Directories
SRC_DIR = src
TOOLS_DIR = tools
BUILD_DIR = build
KERNEL_DIR = $(SRC_DIR)/kernel
BOOT_DIR = $(SRC_DIR)/bootstrap
GRUB_DIR = $(BUILD_DIR)/boot/grub
BOOT_BIN = $(BUILD_DIR)/boot/huskyos.bin
# Files
BOOT_OBJ = $(BUILD_DIR)/boot.o
KERNEL_BIN = $(BUILD_DIR)/huskyos.bin
ISO_FILE = $(BUILD_DIR)/huskyos.iso
LINKER_SCRIPT = ld/linker.ld

# Find all kernel C files
KERNEL_SRCS = $(shell find $(KERNEL_DIR) -name '*.c')

KERNEL_OBJS = $(patsubst $(KERNEL_DIR)/%.c, $(BUILD_DIR)/%.o, $(KERNEL_SRCS))

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	echo $(KERNEL_SRCS)
	echo $(KERNEL_OBJS)

# Build all
all: $(BUILD_DIR) $(ISO_FILE)

$(BUILD_DIR)/kernel.o: $(KERNEL_DIR)/kernel.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS)

# Compile kernel C files into object files
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS)

# Assemble bootloader
$(BOOT_OBJ): $(BOOT_DIR)/boot.s | $(BUILD_DIR)
	$(AS) $< -o $@

# Link everything into a binary
$(KERNEL_BIN): $(BOOT_OBJ) $(KERNEL_OBJS)
	$(LD) -T $(LINKER_SCRIPT) -o $@ $(LDFLAGS) $^ -lgcc

# Assemble bootloader and convert to binary
# Create ISO structure and image
$(ISO_FILE): $(KERNEL_BIN)
	mkdir -p $(GRUB_DIR)
	cp $(KERNEL_BIN) $(BUILD_DIR)/boot/huskyos.bin
	cp grub.cfg $(GRUB_DIR)/grub.cfg
	grub-mkrescue -o $(ISO_FILE) $(BUILD_DIR) -- -volid "HUSKYOS"

# Clean up build files
clean:
	rm -rf $(BUILD_DIR) $(ISO_FILE)

# Run in QEMU
run: $(ISO_FILE)
	 --nographic -d int,cpu_reset