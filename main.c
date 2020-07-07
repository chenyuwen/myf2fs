#include <stdio.h>
#include <sys/stat.h>
#include "f2fs.h"
#include "super.h"

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

void print_hex(void *hex, int size)
{
	int i = 0;
	char *tmp = hex;
	static const char buffer[] = "0123456789ABCDEF";
	for(i=0; i < size; i++) {
		printf("%c%c ", buffer[(tmp[i] >> 4) & 0xF],
			buffer[tmp[i] & 0xF]);
		if(i % 16 == 15 && i < (size - 1)) {
			printf("\n");
		}
	}
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
	}

	for(i=0; i<3; i++) {
		printf("\ncur_data_segno[%d][%u]", i, le32_to_cpu(raw_cp->cur_data_segno[i]));
		printf("\ncur_data_blkof[%d][%u]", i, le16_to_cpu(raw_cp->cur_data_blkoff[i]));
	}
	printf("\n");
}

int main(int argc, char **argv)
{
	struct f2fs_super super;
	struct f2fs_inode root_inode, *iter_pos;
	struct dir_iter *iter = NULL;
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

	ret = f2fs_read_ssa(&super);
	if(ret < 0) {
		f2fs_umount(&super);
		return ret;
	}

	print_super(&super);
	print_checkpoint(&super);

	f2fs_read_inode(&super, &root_inode, le32_to_cpu(super.raw_super->root_ino));
	printf("%d\n", root_inode.raw_inode->i_mode);
	if(!S_ISDIR(le32_to_cpu(root_inode.raw_inode->i_mode))) {
		f2fs_free_inode(&root_inode);
		f2fs_umount(&super);
		printf("Error: the root was not a dir.\n");
		return ret;
	}

	iter = dir_iter_start(&super, &root_inode);
	while(iter_pos = dir_iter_next(iter)) {
		printf("iter %s\n", iter_pos->raw_inode->i_name);
	}
	dir_iter_end(iter);

	f2fs_umount(&super);
	return 0;
}
