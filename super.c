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
#include "utils.h"

int f2fs_fill_super(struct f2fs_super *super, char *devpath)
{
	struct f2fs_super_block *raw_super = NULL;
	struct page *sp1;
	int ret = 0, super_ver = 0;
	unsigned int crc = 0;
	size_t crc_offset = 0;

	memset(super, 0, sizeof(struct f2fs_super));
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
	if(super_ver >= 2) {
		free_page(sp1);
		return -1;
	}

	ret = read_page(sp1, super->fd, super_ver);
	if(ret < 0) {
		free_page(sp1);
		perror("read_page");
		return ret;
	}
	super_ver++;

	raw_super = (void *)((char *)page_address(sp1) + F2FS_SUPER_OFFSET);
	if(le32_to_cpu(raw_super->magic) != F2FS_SUPER_MAGIC) {
		printf("BAD Magic %X\n", le32_to_cpu(raw_super->magic));
		goto retry;
	}

	if(!F2FS_HAS_FEATURE(raw_super, F2FS_FEATURE_SB_CHKSUM)) {
//		printf("skip crc.\n");
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
	struct page *page = NULL;

	if(super->raw_cp) {
		f2fs_free(super->raw_cp);
	}

	if(super->nat_bits) {
		f2fs_free(super->nat_bits);
	}

	page = address_to_page((char *)super->raw_super - F2FS_SUPER_OFFSET);
	free_page(page);
	super->raw_super = NULL;
	return 0;
}

int f2fs_get_valid_checkpoint(struct f2fs_super *super)
{
	unsigned long cpblk = le32_to_cpu(super->raw_super->cp_blkaddr);
	struct page *cp1_page = NULL, *cp2_page = NULL;
	struct page *cp1_bak_page = NULL, *cp2_bak_page = NULL;
	struct f2fs_checkpoint *cptmp1 = NULL, *cptmp2 = NULL;
	struct f2fs_checkpoint *cp = NULL;
	int ret = 0, cp_blocks = 0, blocksize = 0, i = 0;

	blocksize = le32_to_cpu(super->raw_super->log_blocksize);
	cp_blocks = le32_to_cpu(super->raw_super->cp_payload) + 1;
	cp = f2fs_malloc(cp_blocks * blocksize);
	if(cp == NULL) {
		perror("f2fs_malloc");
		return -ENOMEM;
	}

	cp1_page = alloc_page();
	if(cp1_page == NULL) {
		perror("alloc page");
		return -ENOMEM;
	}

	ret = read_page(cp1_page, super->fd, cpblk);
	if(ret < 0) {
		f2fs_free(cp);
		free_page(cp1_page);
		perror("read page");
		return ret;
	}

	cptmp1 = page_address(cp1_page);

	cp2_page = alloc_page();
	if(cp2_page == NULL) {
		f2fs_free(cp);
		free_page(cp1_page);
		perror("alloc page");
		return -ENOMEM;
	}

	cpblk += 1 << le32_to_cpu(super->raw_super->log_blocks_per_seg);
	ret = read_page(cp2_page, super->fd, cpblk);
	if(ret < 0) {
		f2fs_free(cp);
		free_page(cp1_page);
		free_page(cp2_page);
		perror("read page");
		return ret;
	}

	cptmp2 = page_address(cp2_page);

	if(le32_to_cpu(cptmp1->checkpoint_ver) >= le32_to_cpu(cptmp2->checkpoint_ver)) {
//		printf("use checkpoint1\n");
		memcpy(cp, cptmp1, blocksize);
		super->cp_ver = 0;
		cpblk = le32_to_cpu(super->raw_super->cp_blkaddr);
	} else {
//		printf("use checkpoint2\n");
		memcpy(cp, cptmp2, blocksize);
		super->cp_ver = 1;
		cpblk = cpblk;
	}

	for(i=1; i<cp_blocks; i++) {
		ret = read_page(cp1_page, super->fd, cpblk + 1);
		if(ret < 0) {
			perror("read page");
			f2fs_free(cp);
			free_page(cp1_page);
			free_page(cp2_page);
			return -1;
		}

		memcpy(cp + blocksize * i, page_address(cp1_page), blocksize);
	}
	super->raw_cp = cp;
	free_page(cp1_page);
	free_page(cp2_page);
	return 0;
}

int f2fs_read_inode(struct f2fs_super *super, struct f2fs_inode *inode, inode_t ino)
{
	struct page *nat_page = NULL, *inode_page = NULL;
	struct f2fs_nat_block *nat = NULL;
	struct f2fs_raw_inode *raw_inode;
	int ret = 0, byteoff = 0, bitoff = 0;
	unsigned long blkaddr = 0, tmpaddr, blocks_per_seg = 0;

	memset(inode, 0, sizeof(struct f2fs_inode *));
	nat_page = alloc_page();
	if(nat_page == NULL) {
		perror("alloc page");
		return -ENOMEM;
	}

	tmpaddr = ((unsigned long)ino / NAT_ENTRY_PER_BLOCK);
	blocks_per_seg = 1 << le32_to_cpu(super->raw_super->log_blocks_per_seg);
	blkaddr = le32_to_cpu(super->raw_super->nat_blkaddr) + (tmpaddr / blocks_per_seg) *
			blocks_per_seg * 2 + tmpaddr % blocks_per_seg;

	byteoff = ino / 8;
	bitoff = ino % 8;
	if(!(super->nat_bitmap[byteoff] & (1 << bitoff))) {
		blkaddr += blocks_per_seg;
	}

	ret = read_page(nat_page, super->fd, blkaddr);
	if(ret < 0) {
		free_page(nat_page);
		perror("read page");
		return -1;
	}

	nat = (void *)page_address(nat_page);
	inode_page = alloc_page();
	if(inode_page == NULL) {
		free_page(nat_page);
		perror("alloc page");
		return -ENOMEM;
	}

	blkaddr = le32_to_cpu(nat->entries[ino % NAT_ENTRY_PER_BLOCK].block_addr);
	ret = read_page(inode_page, super->fd, blkaddr);
	if(ret < 0) {
		free_page(inode_page);
		free_page(nat_page);
		perror("read page");
		return -1;
	}

	raw_inode = (void *)page_address(inode_page);

	inode->raw_inode = raw_inode;
	inode->nat_block = nat;
	inode->ino = ino;
	inode->count = 1;
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

int f2fs_get_inode(struct f2fs_inode *inode)
{
	if(inode->count <= 0) {
		BUG("The inode was incorrect.\n");
		return -1;
	}
	return inode->count++;
}

int f2fs_put_inode(struct f2fs_inode *inode)
{
	int count = 0;
	inode->count--;
	count = inode->count;
	if(inode->count == 0) {
		f2fs_free_inode(inode);
		f2fs_free(inode);
	}
	return count;
}

struct dir_iter *dir_iter_start(struct f2fs_super *super, struct f2fs_inode *inode)
{
	struct dir_iter *iter = NULL;
	struct f2fs_dentry_block *dentry_block = NULL;
	struct page *page = NULL;
	int ret = 0;
	struct f2fs_node *node = NULL;
	void *inline_data = NULL;
	int reserved_size = 0, bitmap_size = 0;

	if(!S_ISDIR(le32_to_cpu(inode->raw_inode->i_mode))) {
		return NULL;
	}

	iter = (void *)f2fs_malloc(sizeof(struct dir_iter));
	if(iter == NULL) {
		return NULL;
	}

	memset((void *)iter, 0, sizeof(struct dir_iter));
	if(le32_to_cpu(inode->raw_inode->i_inline) && F2FS_INLINE_DENTRY) {
		node = (struct f2fs_node *)inode->raw_inode;
		inline_data = inline_data_addr(inode->raw_inode);
		reserved_size = INLINE_RESERVED_SIZE(inode->raw_inode);
		bitmap_size = INLINE_DENTRY_BITMAP_SIZE(inode->raw_inode);

		iter->dentry_bitmap = inline_data;
		iter->dentry_inline = 1;
		iter->dentry = inline_data + bitmap_size + reserved_size;
		iter->entry_cnt = NR_INLINE_DENTRY(inode->raw_inode);

	} else {
		page = alloc_page();
		ret = read_page(page, super->fd, le32_to_cpu(inode->raw_inode->i_addr[0]));
		dentry_block = page_address(page);
		iter->dentry_bitmap = dentry_block->dentry_bitmap;
		iter->dentry_inline = 0;
		iter->dentry = dentry_block->dentry;
		iter->entry_cnt = SIZE_OF_DENTRY_BITMAP;
	}

	iter->inode = inode;
	iter->off = 2;
	iter->dentry_block = dentry_block;
	iter->super = super;
	iter->pos = NULL;
	return iter;
}

struct f2fs_inode *dir_iter_next(struct dir_iter *iter)
{
	struct f2fs_inode *inode = NULL;
	struct f2fs_dentry_block *dentry = NULL;
	int i = 0, byteoff = 0, bitoff = 0, ret = 0;
	inode_t ino = 0;
	struct f2fs_inode *tmp = NULL;

	if(iter == NULL) {
		return NULL;
	}

	/*TODO*/

	inode = iter->inode;
	for(i=iter->off; i< iter->entry_cnt; i++) {
		byteoff = i / BITS_PER_BYTE;
		bitoff = i % BITS_PER_BYTE;
		if(!(iter->dentry_bitmap[byteoff] & (1 << bitoff))) {
			continue;
		}

		if(iter->pos != NULL) {
			f2fs_put_inode(iter->pos);
			iter->pos = NULL;
		}

		tmp = (void *)f2fs_malloc(sizeof(struct f2fs_inode));
		if(tmp == NULL) {
			return NULL;
		}

		ino = le32_to_cpu(iter->dentry[i].ino);
		ret = f2fs_read_inode(iter->super, tmp, ino);
		if(ret < 0) {
			f2fs_free(tmp);
			return NULL;
		}
		iter->off = i + 1;
		iter->pos = tmp;
		return tmp;
	}
	return NULL;
}

void dir_iter_end(struct dir_iter *iter)
{
	struct page *page = NULL;
	if(iter == NULL) {
		return;
	}

	if(iter->dentry_block) {
		page = address_to_page(iter->dentry_block);
		free_page(page);
	}

	if(iter->pos != NULL) {
		f2fs_put_inode(iter->pos);
		iter->pos = NULL;
	}
	f2fs_free(iter);
}

int f2fs_read_ssa(struct f2fs_super *super)
{
#if 0
	struct f2fs_super_block *raw_super = super->raw_super;
	struct f2fs_checkpoint *raw_cp = super->raw_cp;
	struct page *ssa_page = NULL;
	struct f2fs_summary *summary = NULL;
	struct f2fs_journal *journal = NULL;
	struct f2fs_summary_block *summary_block = NULL;
	block_t blkaddr = 0;
	int ret  = 0, i = 0, j = 0;

	if(!is_set_ckpt_flags(raw_cp, CP_COMPACT_SUM_FLAG)) {
		printf("TODO:");
		return -1;
	}

	ssa_page = alloc_page();
	if(ssa_page == NULL) {
		perror("alloc page");
		return -ENOMEM;
	}

	blkaddr = start_sum_block(super);
	ret = read_page(ssa_page, super->fd, blkaddr);
	if(ret < 0) {
		free_page(ssa_page);
		perror("read page");
		return -1;
	}

	summary = page_address(ssa_page) + SUM_JOURNAL_SIZE * 2;

	for(i=0; i<le32_to_cpu(raw_cp->cur_data_blkoff[0]); i++) {
		printf("%d %d %d\n", summary[i].nid, summary[i].version, summary[i].ofs_in_node);
	}

	journal = page_address(ssa_page);
	printf("sum: %d\n", journal->n_nats);
	for(i=0; i<NAT_JOURNAL_ENTRIES; i++) {
		printf("nat %d %d %d %d\n",
			journal->nat_j.entries[i].nid,
			journal->nat_j.entries[i].ne.version,
			journal->nat_j.entries[i].ne.ino,
			journal->nat_j.entries[i].ne.block_addr);
	}

	journal = page_address(ssa_page) + SUM_JOURNAL_SIZE;
	printf("sum: %d\n", journal->n_sits);
	for(i=0; i<SIT_JOURNAL_ENTRIES; i++) {
		printf("sit %d %d %d\n",
			journal->sit_j.entries[i].segno,
			journal->sit_j.entries[i].se.vblocks,
			journal->sit_j.entries[i].se.mtime);
	}

	for(j=0; j<3; j++) {
		blkaddr = (raw_super->ssa_blkaddr + raw_cp->cur_node_segno[j]);
		ret = read_page(ssa_page, super->fd, blkaddr);
		summary_block = page_address(ssa_page);
		printf("segno:%d addr:%lu node%d sum: %d\n", raw_cp->cur_node_segno[j],
			blkaddr, j, summary_block->journal.n_nats);
		for(i=0; i<summary_block->journal.n_nats; i++) {
			journal = &summary_block->journal;
			printf("node nat %d %d %d %d\n",
				journal->nat_j.entries[i].nid,
				journal->nat_j.entries[i].ne.version,
				journal->nat_j.entries[i].ne.ino,
				journal->nat_j.entries[i].ne.block_addr);
		}
	}
#endif

	return 0;
}

static int __get_free_nat_bitmaps(struct f2fs_super *super)
{
	unsigned long nat_bits_blocks = 0;
	struct f2fs_nat_bitmap *nat_bits = NULL;
	unsigned int nat_bits_bytes = 0;
	unsigned int nat_segs = 0;
	block_t nat_bits_addr = 0;
	struct page *page = NULL;
	int ret = 0, i = 0;

	nat_segs = le32_to_cpu(super->raw_super->segment_count_nat) >> 1;
	super->nat_blocks = nat_segs << le32_to_cpu(super->raw_super->log_blocks_per_seg);
	nat_bits_bytes = super->nat_blocks / BITS_PER_BYTE;
	nat_bits_blocks = F2FS_BLK_ALIGN((nat_bits_bytes >> 1) + 8);
	nat_bits_addr = __start_cp_addr(super) +
		(1 << le32_to_cpu(super->raw_super->log_blocks_per_seg)) -
		nat_bits_blocks;

	nat_bits = (void *)f2fs_malloc(nat_bits_blocks << F2FS_BLKSIZE_BITS);
	if(nat_bits == NULL) {
		perror("f2fs_malloc");
		return -ENOMEM;
	}

	page = alloc_page();
	if(page == NULL) {
		perror("alloc_page");
		f2fs_free(nat_bits);
		return -ENOMEM;
	}

	for(i=0; i<nat_bits_blocks; i++) {
		ret = read_page(page, super->fd, nat_bits_addr++);
		if(ret < 0) {
			f2fs_free(nat_bits);
			goto out;
		}

		memcpy(nat_bits + (i << F2FS_BLKSIZE_BITS),
			page_address(page), F2FS_BLKSIZE);
	}
	super->nat_bits = nat_bits;

out:
	free_page(page);
	return ret;
}

int f2fs_build_nat_bitmap(struct f2fs_super *super)
{
	struct f2fs_checkpoint *raw_cp = super->raw_cp;
	int ret = 0, offset = 0;
	char *bitmap = NULL;

	ret = __get_free_nat_bitmaps(super);
	if(ret < 0) {
		return ret;
	}

	if(is_set_ckpt_flags(raw_cp, CP_LARGE_NAT_BITMAP_FLAG)) {
		bitmap = raw_cp->sit_nat_version_bitmap + sizeof(__le32);
	}

	if(super->raw_super->cp_payload) {
		bitmap = raw_cp->sit_nat_version_bitmap;
	} else {
		offset = le32_to_cpu(raw_cp->sit_ver_bitmap_bytesize);
		bitmap = raw_cp->sit_nat_version_bitmap + offset;
	}

	super->nat_bitmap = bitmap;
	return 0;
}

