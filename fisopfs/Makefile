CC = gcc
CFLAGS := -ggdb3 -O2 -Wall -std=c11 -D_FILE_OFFSET_BITS=64
CFLAGS += -Wno-unused-function -Wvla bitmap.c dir.c file.c inode.c

# Flags for FUSE
LDLIBS := $(shell pkg-config fuse --cflags --libs )

# Name for the filesystem!
FS_NAME := fisopfs

all: build

# fisopfs: fisopfs.c ./*
# 	$(CC) $(CFLAGS) ./*.c fisopfs.c -o fisopfs 2>&1

build: $(FS_NAME)

format: .clang-files .clang-format
	xargs -r clang-format -i <$<

clean:
	rm -rf $(EXEC) *.o core vgcore.* $(FS_NAME)

.PHONY: all build clean format
