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
	struct f2fs_inode *root;
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

static inline int get_extra_isize(struct f2fs_raw_inode *raw_inode)
{
	return le16_to_cpu(raw_inode->i_extra_isize) / sizeof(__le32);
}

static inline int get_inline_xattr_addrs(struct f2fs_raw_inode *raw_inode)
{
	if(raw_inode->i_inline && F2FS_INLINE_XATTR ||
		raw_inode->i_inline && F2FS_INLINE_XATTR) {
		return DEFAULT_INLINE_XATTR_ADDRS;
	}
	return 0;
}

#define DEF_INLINE_RESERVED_SIZE        1
static inline void *inline_data_addr(struct f2fs_raw_inode *raw_inode)
{
	int extra_size = get_extra_isize(raw_inode);

	return (void *)&(raw_inode->i_addr[extra_size + DEF_INLINE_RESERVED_SIZE]);
}

/* for inline stuff */
#define MAX_INLINE_DATA(inode)  (sizeof(__le32) *                       \
				(CUR_ADDRS_PER_INODE(inode) -           \
				get_inline_xattr_addrs(inode) - \
				DEF_INLINE_RESERVED_SIZE))

/* for inline dir */
#define NR_INLINE_DENTRY(inode) (MAX_INLINE_DATA(inode) * BITS_PER_BYTE / \
				((SIZE_OF_DIR_ENTRY + F2FS_SLOT_LEN) * \
				BITS_PER_BYTE + 1))

#define INLINE_DENTRY_BITMAP_SIZE(inode)        ((NR_INLINE_DENTRY(inode) + \
				BITS_PER_BYTE - 1) / BITS_PER_BYTE)

#define INLINE_RESERVED_SIZE(inode)     (MAX_INLINE_DATA(inode) - \
				((SIZE_OF_DIR_ENTRY + F2FS_SLOT_LEN) * \
				NR_INLINE_DENTRY(inode) + \
				INLINE_DENTRY_BITMAP_SIZE(inode)))

#endif /*__F2FS_H__*/
