#ifndef __SUPER_H__
#define __SUPER_H__

#include "f2fs.h"

struct dir_iter {
	int dentry_inline;
	int off;
	struct f2fs_super *super;
	struct f2fs_inode *inode, tmp;
	struct f2fs_dentry_block *dentry;
};

static inline int f2fs_is_vaild_inode(struct f2fs_inode *inode)
{
	if(inode == NULL) {
		return 0;
	}

	if(inode->raw_inode == NULL) {
		return 0;
	}
	return 1;
}

int f2fs_fill_super(struct f2fs_super *super, char *devpath);
int f2fs_umount(struct f2fs_super *super);
int f2fs_get_valid_checkpoint(struct f2fs_super *super);
int f2fs_read_ssa(struct f2fs_super *super);
int f2fs_read_inode(struct f2fs_super *super, struct f2fs_inode *inode, inode_t ino);
void f2fs_free_inode(struct f2fs_inode *inode);
int f2fs_build_nat_bitmap(struct f2fs_super *super);

struct dir_iter *dir_iter_start(struct f2fs_super *super, struct f2fs_inode *inode);
struct f2fs_inode *dir_iter_next(struct dir_iter *iter);
void dir_iter_end(struct dir_iter *iter);

#endif /*__SUPER_H__*/
