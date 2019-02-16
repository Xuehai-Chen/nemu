#include "common.h"
#include "fs.h"

#define DEFAULT_ENTRY ((void *)0x4000000)

uintptr_t loader(_Protect *as, const char *filename) {
	int fd = fs_open(filename, 0, 0);
	size_t sz = fs_filesz(fd);
	fs_lseek(fd, 0, SEEK_SET);
	fs_read(fd, DEFAULT_ENTRY, sz);
	return (uintptr_t)DEFAULT_ENTRY;
}
