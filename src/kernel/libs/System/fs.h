#ifndef FS_H
#define FS_H

typedef struct {
    uint32_t total_inodes;           // 0-3: Total number of inodes in filesystem
    uint32_t total_blocks;           // 4-7: Total number of blocks in filesystem
    uint32_t reserved_blocks;        // 8-11: Number of reserved blocks
    uint32_t unallocated_blocks;     // 12-15: Total number of unallocated blocks
    uint32_t unallocated_inodes;     // 16-19: Total number of unallocated inodes
    uint32_t superblock_block_num;   // 20-23: Block number of block containing the superblock
    uint32_t log2_block_size_minus10; // 24-27: log2(block size) - 10
    uint32_t log2_fragment_size_minus10; // 28-31: log2(fragment size) - 10
    uint32_t blocks_per_group;       // 32-35: Number of blocks in each block group
    uint32_t fragments_per_group;    // 36-39: Number of fragments in each block group
    uint32_t inodes_per_group;       // 40-43: Number of inodes in each block group
    time_t last_mount_time;          // 44-47: Last mount time (POSIX time)
    time_t last_written_time;        // 48-51: Last written time (POSIX time)
    uint16_t mount_count;            // 52-53: Number of times the volume has been mounted since fsck
    uint16_t max_mounts_before_fsck; // 54-55: Number of mounts before fsck is required
    uint16_t magic_signature;        // 56-57: Magic signature (0xef53)
    uint16_t fs_state;               // 58-59: Filesystem state
    uint16_t error_action;           // 60-61: What to do when an error is detected
    uint16_t version_minor;          // 62-63: Minor portion of version
    uint32_t last_fsck_time;         // 64-67: POSIX time of last fsck
    uint32_t fsck_interval;          // 68-71: Interval (in POSIX time) between forced fscks
    uint32_t os_id;                  // 72-75: Operating system ID
    uint32_t version_major;          // 76-79: Major portion of version
    uint16_t reserved_blocks_uid;    // 80-81: User ID that can use reserved blocks
    uint16_t reserved_blocks_gid;    // 82-83: Group ID that can use reserved blocks
} ext4_superblock;

#endif // SYSTEM_H
