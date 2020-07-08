#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include "f2fs.h"
#include "super.h"
#include "utils.h"

void usage()
{
	printf("f2fs dev super\n");
	printf("f2fs dev sit\n");
	printf("f2fs dev ssa\n");
	printf("f2fs dev nat\n");
	printf("f2fs dev ls [dir]\n");
	printf("f2fs dev mkdir [dir]\n");
	printf("f2fs dev rm [file]\n");
	printf("f2fs dev touch [file]\n");
}

void print_super(struct f2fs_super *super)
{
	struct f2fs_super_block *raw_super = super->raw_super;

	printf("\nmagic: %X", le32_to_cpu(raw_super->magic));
	printf("\nmajor_ver: %X", le16_to_cpu(raw_super->major_ver));
	printf("\nuuid:");print_hex(&raw_super->uuid, 16);
	printf("\ncp_blkaddr:%d", raw_super->cp_blkaddr);
	printf("\nlog_blocks_per_seg:%d", raw_super->log_blocks_per_seg);
	printf("\nchecksum_offset:%d", raw_super->checksum_offset);
	printf("\nsegment_count_ssa:%d", raw_super->segment_count_ssa);
	printf("\n");
}

void print_checkpoint(struct f2fs_super *super)
{
	struct f2fs_checkpoint *raw_cp = super->raw_cp;
	int i = 0;

	printf("checkpoint_ver: %LX", le64_to_cpu(raw_cp->checkpoint_ver));
	for(i=0; i<3; i++) {
		printf("\ncur_node_segno[%d][%u]", i, le32_to_cpu(raw_cp->cur_node_segno[i]));
		printf("\ncur_node_blkof[%d][%u]", i, le16_to_cpu(raw_cp->cur_node_blkoff[i]));
		printf("\nalloc_type[%d][%u]", i, le16_to_cpu(raw_cp->alloc_type[i]));
	}

	for(i=0; i<3; i++) {
		printf("\ncur_data_segno[%d][%u]", i, le32_to_cpu(raw_cp->cur_data_segno[i]));
		printf("\ncur_data_blkof[%d][%u]", i, le16_to_cpu(raw_cp->cur_data_blkoff[i]));
		printf("\nalloc_type[%d][%u]", i, le16_to_cpu(raw_cp->alloc_type[i + 8]));
	}
	printf("\n");
}

static int path_lookup(struct f2fs_super *super, char *path)
{
	struct f2fs_inode *iter_pos, posbuf;
	struct f2fs_inode *found;
	struct dir_iter *iter = NULL;
	int next_ino = 0;
	char name[256];
	int i = 0, nameoff = 0;

	if(path[0] != '/') {
		printf("Not supported path:%s\n", path);
		return 0;
	}

	found = &super->root_inode;
	while(1) {
		nameoff = 0;
		name[0] = '\0';

		/* skip // */
		for(; path[i]!='\0'; i++) {
			if(path[i] != '/') {
				break;
			}
		}

		for(; path[i]!='\0'; i++) {
			if(path[i] == '/' || path[i] == '\0') {
				break;
			}
			name[nameoff++] = path[i];
		}

		if(nameoff == 0) {
			goto out;
		}
		name[nameoff] = '\0';

		printf("name:%s\n", name);
		iter = dir_iter_start(super, found, &posbuf);
		while(iter_pos = dir_iter_next(iter)) {
			printf("iter %s\n", iter_pos->raw_inode->i_name);
			if(!strcmp(name, iter_pos->raw_inode->i_name)) {
				printf("Found %s\n", name);
				break;
			}
		}
		dir_iter_end(iter);
	}

out:
	return 0;
}

int main(int argc, char **argv)
{
	struct f2fs_super super;
	int ret = 0;

	if(argc <= 1) {
		return -1;
	}

	ret = f2fs_fill_super(&super, argv[1]);
	if(ret < 0) {
		return ret;
	}

	ret = f2fs_get_valid_checkpoint(&super);
	if(ret < 0) {
		f2fs_umount(&super);
		return ret;
	}

	ret = f2fs_build_nat_bitmap(&super);
	if(ret < 0) {
		f2fs_umount(&super);
		return ret;
	}

	ret = f2fs_read_ssa(&super);
	if(ret < 0) {
		f2fs_umount(&super);
		return ret;
	}

	print_super(&super);
	print_checkpoint(&super);

	f2fs_read_inode(&super, &super.root_inode, le32_to_cpu(super.raw_super->root_ino));
	printf("%d\n", super.root_inode.raw_inode->i_mode);
	if(!S_ISDIR(le32_to_cpu(super.root_inode.raw_inode->i_mode))) {
		f2fs_free_inode(&super.root_inode);
		f2fs_umount(&super);
		printf("Error: the root was not a dir.\n");
		return ret;
	}

	path_lookup(&super, "/dir1/file1");

	f2fs_umount(&super);
	return 0;
}
