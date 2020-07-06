#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "page.h"
#include "f2fs_type.h"
#include "f2fs.h"
#include "crc32.h"
#include "super.h"

void print_hex(void *hex, int size);
int f2fs_fill_super(struct f2fs_super *super, char *devpath)
{
	struct f2fs_super_block *raw_super = NULL;
	struct page *sp1;
	int ret = 0, cp_ver = 0;
	unsigned int crc = 0;
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
	if(le32_to_cpu(raw_super->magic) != F2FS_SUPER_MAGIC) {
		printf("BAD Magic %X\n", le32_to_cpu(raw_super->magic));
		goto retry;
	}

	if(!F2FS_HAS_FEATURE(raw_super, F2FS_FEATURE_SB_CHKSUM)) {
		printf("skip crc.\n");
		goto out;
	}

	crc_offset = le32_to_cpu(raw_super->checksum_offset);
	if(crc_offset != offsetof(struct f2fs_super_block, crc)) {
		printf("BAD CRC offset %lu\n", crc_offset);
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

int f2fs_umount(struct f2fs_super *super)
{
	struct page *sp_page = NULL;

	sp_page = address_to_page((char *)super->raw_super - F2FS_SUPER_OFFSET);
	free_page(sp_page);

	/*TODO*/
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
		free_page(cp1_page);
		perror("read page");
		return ret;
	}

	cp1 = page_address(cp1_page);

	cp2_page = alloc_page();
	if(cp2_page == NULL) {
		free_page(cp1_page);
		perror("alloc page");
		return -ENOMEM;
	}

	cpblk += 1 << le32_to_cpu(super->raw_super->log_blocks_per_seg);
	ret = read_page(cp2_page, super->fd, cpblk);
	if(ret < 0) {
		free_page(cp2_page);
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

int f2fs_read_inode(struct f2fs_super *super, struct f2fs_inode *inode, inode_t ino)
{
	struct page *nat_page = NULL, *inode_page = NULL;
	struct f2fs_nat_block *nat = NULL;
	struct f2fs_raw_inode *raw_inode;
	int ret = 0, byteoff = 0, bitoff = 0;
	unsigned long blkaddr = 0, tmpaddr;

	nat_page = alloc_page();
	if(nat_page == NULL) {
		perror("alloc page");
		return -ENOMEM;
	}

	tmpaddr = ((unsigned long)ino / NAT_ENTRY_PER_BLOCK);
	blkaddr = le32_to_cpu(super->raw_super->nat_blkaddr) + tmpaddr + (tmpaddr) * 2;

	byteoff = ino / 8;
	bitoff = ino % 8;
	if(super->raw_cp->sit_nat_version_bitmap[byteoff] & (1 << bitoff)) {
		printf("here\n");
		blkaddr++;
	}

	printf("[%lu]:%lu\n", ino, blkaddr);
	ret = read_page(nat_page, super->fd, blkaddr);
	if(ret < 0) {
		free_page(nat_page);
		perror("read page");
		return -1;
	}

	nat = (void *)page_address(nat_page);
	printf("%d %d %d\n", nat->entries[ino % NAT_ENTRY_PER_BLOCK].version,
		le32_to_cpu(nat->entries[ino % NAT_ENTRY_PER_BLOCK].ino),
		le32_to_cpu(nat->entries[ino % NAT_ENTRY_PER_BLOCK].block_addr));

	inode_page = alloc_page();
	if(inode_page == NULL) {
		free_page(nat_page);
		perror("alloc page");
		return -ENOMEM;
	}

	blkaddr = le32_to_cpu(nat->entries[ino % NAT_ENTRY_PER_BLOCK].block_addr);
	printf("blkaddr %lu\n", blkaddr);
	ret = read_page(inode_page, super->fd, blkaddr);
	if(ret < 0) {
		free_page(inode_page);
		free_page(nat_page);
		perror("read page");
		return -1;
	}

	raw_inode = (void *)page_address(inode_page);
	printf("%lu\n", (size_t)le64_to_cpu(raw_inode->i_mode));

	inode->raw_inode = raw_inode;
	inode->nat_block = nat;
	inode->ino = ino;
	return 0;
}

void f2fs_free_inode(struct f2fs_inode *inode)
{
	struct page *page = NULL;

	if(inode->raw_inode != NULL) {
		page = address_to_page(inode->raw_inode);
		free_page(page);
	}
	if(inode->nat_block != NULL) {
		page = address_to_page(inode->nat_block);
		free_page(page);
	}
}

struct dir_iter *dir_iter_start(struct f2fs_super *super, struct f2fs_inode *inode)
{
	struct dir_iter *iter = NULL;
	struct f2fs_dentry_block *dentry_block = NULL;
	struct page *page = NULL;
	int ret = 0;
	if(!S_ISDIR(le32_to_cpu(inode->raw_inode->i_mode))) {
		return NULL;
	}

	iter = (void *)malloc(sizeof(struct dir_iter));
	if(iter == NULL) {
		return NULL;
	}

	memset((void *)iter, 0, sizeof(struct dir_iter));
	if(le32_to_cpu(inode->raw_inode->i_inline) && F2FS_INLINE_DENTRY) {
		iter->dentry_inline = 1;
		printf("iter inline not support\n");
		return NULL;
	}

	page = alloc_page();
	ret = read_page(page, super->fd, le32_to_cpu(inode->raw_inode->i_addr[0]));
	dentry_block = page_address(page);
	printf("inode:%u\n", le32_to_cpu(dentry_block->dentry[1].ino));

	iter->inode = inode;
	iter->off = 0;
	iter->dentry = dentry_block;
	iter->super = super;
	return iter;
}

struct f2fs_inode *dir_iter_next(struct dir_iter *iter)
{
	struct f2fs_inode *inode = iter->inode;
	struct f2fs_dentry_block *dentry = iter->dentry;
	int i = 0, byteoff = 0, bitoff = 0, ret = 0;
	inode_t ino = 0;

	for(i=iter->off; i< SIZE_OF_DENTRY_BITMAP * BITS_PER_BYTE; i++) {
		byteoff = i / BITS_PER_BYTE;
		bitoff = i % BITS_PER_BYTE;
		if(!(dentry->dentry_bitmap[byteoff] & (1 << bitoff))) {
			continue;
		}

		iter->off = i + 1;
		ino = le32_to_cpu(dentry->dentry[i].ino);
		if(f2fs_is_vaild_inode(&iter->tmp)) {
			f2fs_free_inode(&iter->tmp);
		}
		ret = f2fs_read_inode(iter->super, &iter->tmp, ino);
		if(ret < 0) {
			return NULL;
		}
		return &iter->tmp;
	}
	return NULL;
}

void dir_iter_end(struct dir_iter *iter)
{
	struct page *page = NULL;
	if(iter == NULL) {
		return;
	}

	page = address_to_page(iter->dentry);
	free_page(page);

	if(f2fs_is_vaild_inode(&iter->tmp)) {
		f2fs_free_inode(&iter->tmp);
	}
	free(iter);
}

