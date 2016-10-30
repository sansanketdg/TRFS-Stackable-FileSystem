TRFS_VERSION="0.1"

EXTRA_CFLAGS += -DTRFS_VERSION=\"$(WRAPFS_VERSION)\"

obj-$(CONFIG_TR_FS) += trfs.o

trfs-y := dentry.o file.o inode.o main.o super.o lookup.o mmap.o
