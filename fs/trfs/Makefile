TRFS_VERSION="0.1"

obj-m:= trfs.o main.o

EXTRA_CFLAGS += -DTRFS_VERSION=\"$(WRAPFS_VERSION)\"

obj-$(CONFIG_TR_FS) += trfs.o

trfs-y := dentry.o file.o inode.o main.o super.o lookup.o mmap.o

all:trfs

trfs:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f *.o
