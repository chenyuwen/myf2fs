#ifndef __PAGE_H__
#define __PAGE_H__
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "f2fs_type.h"

#define F2FS_PAGE_SIZE 4096

struct page {
	unsigned char dat[F2FS_PAGE_SIZE];
};

static inline void *page_address(struct page *page)
{
	return (void *)page;
}

static inline struct page *address_to_page(void *address)
{
	return (struct page *)address;
}

static inline struct page *alloc_page()
{
	return (void *)f2fs_malloc(F2FS_PAGE_SIZE);
}

static inline void free_page(struct page *page)
{
	f2fs_free(page_address(page));
}

static inline int read_page(struct page *page, int fd, int sector)
{
	size_t len = 0;
	lseek(fd, sector * F2FS_PAGE_SIZE, SEEK_SET);
	len = read(fd, page_address(page), F2FS_PAGE_SIZE);
	return len;
}

#endif /*__PAGE_H__*/
