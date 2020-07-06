#include <stdio.h>
#include "f2fs.h"

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

void print_hex(char *hex, int size)
{
	int i = 0;
	static const char buffer[] = "0123456789ABCDEF";
	for(i=0; i < size; i++) {
		printf("%c%c ", buffer[(hex[i] >> 4) & 0xF],
			buffer[hex[i] & 0xF]);
		if(i % 16 == 15 && i < (size - 1)) {
			printf("\n");
		}
	}
}

void print_super(struct f2fs_super *super)
{
	struct f2fs_super_block *raw_super = super->raw_super;

	print_hex((void *)raw_super, sizeof(struct f2fs_super_block));

	printf("\nmagic: %X", le32_to_cpu(raw_super->magic));
	printf("\nmajor_ver: %X", le16_to_cpu(raw_super->major_ver));
	printf("\nuuid:");print_hex(&raw_super->uuid, 16);
	printf("\ncp_blkaddr:%d", raw_super->cp_blkaddr);
	printf("\nlog_blocks_per_seg:%d", raw_super->log_blocks_per_seg);
	printf("\nchecksum_offset:%d", raw_super->checksum_offset);
	printf("\n");
}

void print_checkpoint(struct f2fs_super *super)
{
	struct f2fs_checkpoint *raw_cp = super->raw_cp;

	printf("checkpoint_ver: %X", le64_to_cpu(raw_cp->checkpoint_ver));
	printf("\n");
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
		return ret;
	}

	print_super(&super);
	print_checkpoint(&super);

//	unmount_super();
	return 0;
}
