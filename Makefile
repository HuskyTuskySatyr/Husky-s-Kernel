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

# Explicitly ordered source files (must come first)
ORDERED_SRCS = \
    $(KERNEL_DIR)/libs/System/standard.c \
    $(KERNEL_DIR)/libs/System/system.c

# Find all other kernel C files (excluding ordered ones)
OTHER_KERNEL_SRCS = $(filter-out $(ORDERED_SRCS), $(shell find $(KERNEL_DIR) -name '*.c'))

# Object files (ordered first, then others)
ORDERED_OBJS = $(patsubst $(KERNEL_DIR)/%.c, $(BUILD_DIR)/%.o, $(ORDERED_SRCS))
OTHER_OBJS = $(patsubst $(KERNEL_DIR)/%.c, $(BUILD_DIR)/%.o, $(OTHER_KERNEL_SRCS))
KERNEL_OBJS = $(ORDERED_OBJS) $(OTHER_OBJS)

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build all
all: $(BUILD_DIR) $(ISO_FILE)

# Explicit compilation of ordered sources (must come first)
$(BUILD_DIR)/libs/System/standard.o: $(KERNEL_DIR)/libs/System/standard.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) -I$(KERNEL_DIR)/libs/System

$(BUILD_DIR)/libs/System/system.o: $(KERNEL_DIR)/libs/System/system.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) -I$(KERNEL_DIR)/libs/System

# Compile other kernel C files into object files
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS)

# Assemble bootloader
$(BOOT_OBJ): $(BOOT_DIR)/boot.s | $(BUILD_DIR)
	$(AS) $< -o $@

# Link everything into a binary (ordered objects first)
$(KERNEL_BIN): $(BOOT_OBJ) $(KERNEL_OBJS)
	$(LD) -T $(LINKER_SCRIPT) -o $@ $(LDFLAGS) $^ -lgcc

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
	qemu-system-i386 -cdrom $(ISO_FILE) -nographic -d int,cpu_reset