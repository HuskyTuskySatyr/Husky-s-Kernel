#include <stddef.h>
#include <stdint.h>
#include "fs.h"
#include "system.h"
#include "../Drivers/disk.h"

#define BLOCK_SIZE 512

extern bool read(HBA_PORT *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buf);

bool read_ext4_superblock(HBA_PORT *port, ext4_superblock_t *superblock) {
    uint32_t superblock_start_lba = 1024 / 512;  // 1024 bytes is typical for EXT4 superblock, assuming 512-byte sectors
    uint32_t superblock_start_hba = 0;           // Higher part of the LBA (if necessary)

    uint8_t buffer[1024];  // Buffer to hold the superblock (1024 bytes)

    // Read the EXT4 superblock into the buffer
    if (!read(port, superblock_start_lba, superblock_start_hba, 2, buffer)) {
        return false;  // Reading failed
    }

    // Ensure the superblock structure is the same size as the buffer
    memcpy(superblock, buffer, sizeof(ext4_superblock_t));

    return true;
}


bool read_block(HBA_PORT *port, uint32_t block_num, uint16_t *buffer) {
    uint32_t startl = block_num & 0xFFFFFFFF;  // Lower 32 bits of LBA
    uint32_t starth = block_num >> 32;         // Upper 32 bits of LBA (if needed)

    // Read a single block from the disk
    if (!read(port, startl, starth, 1, buffer)) {
        // Failed to read the block
        return false;
    }

    return true;
}

bool read_all_blocks(HBA_PORT *port, ext4_superblock_t *superblock) {
    // Get the block size from the superblock (log2 block size)
    uint32_t block_size = 1024 << superblock->log2_block_size_minus10;

    // Allocate a buffer for reading a single block
    uint16_t *buffer = (uint16_t *)malloc(block_size);

    if (buffer == NULL) {
        return false;  // Memory allocation failed
    }

    uint32_t block_num = superblock->s_first_data_block;  // Start from the first data block

    while (true) {
        // Read a block from the disk
        if (!read_block(port, block_num, buffer)) {
            free(buffer);  // Free the buffer if reading fails
            return false;
        }
        // Assuming the block count is not exhausted, move to the next block
        block_num++;
    }

    free(buffer);  // Free the buffer after use
    return true;
}

//for completion