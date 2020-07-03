#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "page.h"
#include "f2fs_type.h"
#include "f2fs.h"

int f2fs_fill_super(struct f2fs_super *super, char *devpath)
{
	struct f2fs_super_block *raw_super = NULL;
	struct page *sp1, *sp2;
	int ret = 0;
	super->fd = open(devpath, O_RDWR);
	if(super->fd < 0) {
		perror("open");
		return super->fd;
	}

	sp1 = alloc_page();
	if(sp1 == NULL) {
		perror("alloc_page");
		return -ENOMEM;
	}
	ret = read_page(sp1, super->fd, 0);
	if(ret < 0) {
		perror("read_page");
		return ret;
	}

	raw_super = (void *)((char *)page_address(sp1) + F2FS_SUPER_OFFSET);
	super->raw_super = raw_super;
	return 0;
}

int f2fs_get_valid_checkpoint(struct f2fs_super *super)
{
	unsigned long cpblk = le32_to_cpu(super->raw_super->cp_blkaddr);
	struct page *cp_page = NULL;
	struct f2fs_checkpoint *cp1 = NULL, *cp2 = NULL;
	int ret = 0;

	cp_page = alloc_page();
	if(cp_page == NULL) {
		perror("alloc page");
		return -ENOMEM;
	}

	ret = read_page(cp_page, super->fd, cpblk);
	if(ret < 0) {
		perror("read page");
		return ret;
	}

	cp1 = page_address(cp_page);
	super->raw_cp = cp1;
	return 0;
}
