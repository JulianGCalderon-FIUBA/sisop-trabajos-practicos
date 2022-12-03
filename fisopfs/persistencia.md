# Saving

- Save superblock table bitmap: 16 bytes
    - Write each uint64_t at time

- Save each non-free inode table: 4096 bytes each    

- Save each inode page, in order: 4096 bytes each


# Loading

- Load superblock table bitmap: 16 bytes
    - Load one uint64_t at time

- For each zero-bit in bitmap, load 4096 from file to memory (one inode table)
    - Save pointer to superblock->inode_table[zero-bit position]

- For each occupied inode (using table bitmap) ordered by id, load inode pages
    - For each block in stats.st_blocks
        - Load page from file
        - Update inode->pages[i]


