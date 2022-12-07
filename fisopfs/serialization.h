#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include "inode.h"
void serialize(superblock_t *superblock, const char *path);
int deserialize(superblock_t *superblock, const char *path);
#endif