#ifndef __F2FS_H__
#define __F2FS_H__
#include <errno.h>
#include "f2fs_type.h"
#include "f2fs_fs.h"
#include "page.h"

#define F2FS_SUPER_MAGIC        0xF2F52010

struct f2fs_nat_bitmap {
	__le64 cp_checksum;
	char bitmap[1];
} __pack;

struct f2fs_inode {
	inode_t ino;
	int count;
	struct f2fs_nat_block *nat_block;
	struct f2fs_raw_inode *raw_inode;
};

struct f2fs_super {
	int fd;
	int cp_ver;
	block_t nat_blocks;
	struct f2fs_super_block *raw_super, *raw_super_bak;
	struct f2fs_checkpoint *raw_cp, *raw_cp_bak;
	struct f2fs_nat_bitmap *nat_bits;
	char *nat_bitmap;
	struct f2fs_inode root_inode;
};

#define F2FS_FEATURE_ENCRYPT            0x0001
#define F2FS_FEATURE_BLKZONED           0x0002
#define F2FS_FEATURE_ATOMIC_WRITE       0x0004
#define F2FS_FEATURE_EXTRA_ATTR         0x0008
#define F2FS_FEATURE_PRJQUOTA           0x0010
#define F2FS_FEATURE_INODE_CHKSUM       0x0020
#define F2FS_FEATURE_FLEXIBLE_INLINE_XATTR      0x0040
#define F2FS_FEATURE_QUOTA_INO          0x0080
#define F2FS_FEATURE_INODE_CRTIME       0x0100
#define F2FS_FEATURE_LOST_FOUND         0x0200
#define F2FS_FEATURE_VERITY             0x0400  /* reserved */
#define F2FS_FEATURE_SB_CHKSUM          0x0800

#define F2FS_HAS_FEATURE(raw_super, mask) \
	((le32_to_cpu(raw_super->feature) & (mask)) != 0)

static inline int is_set_ckpt_flags(struct f2fs_checkpoint *cp, unsigned long flags)
{
	return !!(le32_to_cpu(cp->ckpt_flags) & flags);
}

static inline block_t __start_cp_addr(struct f2fs_super *super)
{
	block_t blkaddr = le32_to_cpu(super->raw_super->cp_blkaddr);

	if(super->cp_ver) {
		blkaddr += le32_to_cpu(super->raw_super->log_blocks_per_seg);
	}
	return blkaddr;
}

static inline block_t start_sum_block(struct f2fs_super *super)
{
	return __start_cp_addr(super) +
		le32_to_cpu(super->raw_cp->cp_pack_start_sum);
}

#endif /*__F2FS_H__*/
