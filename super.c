#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "page.h"
#include "f2fs_type.h"
#include "f2fs.h"
#include "./crc32/crc32.h"

int f2fs_fill_super(struct f2fs_super *super, char *devpath)
{
	struct f2fs_super_block *raw_super = NULL;
	struct page *sp1;
	int ret = 0, cp_ver = 0;
	unsigned long crc = 0;
	size_t crc_offset = 0;
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

retry:
	if(cp_ver >= 2) {
		free_page(sp1);
		return -1;
	}

	ret = read_page(sp1, super->fd, cp_ver);
	if(ret < 0) {
		free_page(sp1);
		perror("read_page");
		return ret;
	}
	cp_ver++;

	raw_super = (void *)((char *)page_address(sp1) + F2FS_SUPER_OFFSET);

	if(!F2FS_HAS_FEATURE(raw_super, F2FS_FEATURE_SB_CHKSUM)) {
		goto out;
	}

	if(le32_to_cpu(raw_super->magic) != F2FS_SUPER_MAGIC) {
		printf("BAD Magic %X\n", le32_to_cpu(raw_super->magic));
		goto retry;
	}

	crc_offset = le32_to_cpu(raw_super->checksum_offset);
	if(crc_offset != offsetof(struct f2fs_super_block, crc)) {
		printf("BAD CRC offset %d\n", crc_offset);
		goto retry;
	}

	crc = crc32_classic((void *)raw_super, crc_offset);
	crc = crc32_finalize(crc);

	if(crc != le32_to_cpu(raw_super->crc)) {
		printf("BAD CRC:%X(%X)\n", le32_to_cpu(raw_super->crc), crc);
		goto retry;
	}

out:
	super->raw_super = raw_super;
	return 0;
}

int f2fs_get_valid_checkpoint(struct f2fs_super *super)
{
	unsigned long cpblk = le32_to_cpu(super->raw_super->cp_blkaddr);
	struct page *cp1_page = NULL, *cp2_page = NULL;
	struct page *cp1_bak_page = NULL, *cp2_bak_page = NULL;
	struct f2fs_checkpoint *cp1 = NULL, *cp1_bak, *cp2 = NULL, *cp2_bak;
	int ret = 0;

	cp1_page = alloc_page();
	if(cp1_page == NULL) {
		perror("alloc page");
		return -ENOMEM;
	}

	ret = read_page(cp1_page, super->fd, cpblk);
	if(ret < 0) {
		perror("read page");
		return ret;
	}

	cp1 = page_address(cp1_page);

	cp2_page = alloc_page();
	if(cp2_page == NULL) {
		perror("alloc page");
		return -ENOMEM;
	}

	cpblk += 1 << le32_to_cpu(super->raw_super->log_blocks_per_seg);
	ret = read_page(cp2_page, super->fd, cpblk);
	if(ret < 0) {
		perror("read page");
		return ret;
	}

	cp2 = page_address(cp2_page);

	if(le32_to_cpu(cp1->checkpoint_ver) >= le32_to_cpu(cp2->checkpoint_ver)) {
		super->raw_cp = cp1;
		super->raw_cp_bak = cp2;
	} else {
		super->raw_cp = cp2;
		super->raw_cp_bak = cp1;
	}
	return 0;
}
