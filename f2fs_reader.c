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
	for(i=0; i<size; i++) {
		printf("%c%c ", buffer[(hex[i] >> 4) & 0xF],
			buffer[hex[i] & 0xF]);
	}
}

void print_super(struct f2fs_super *super)
{
	struct f2fs_super_block *raw_super = super->raw_super;

	printf("magic:");print_hex(&raw_super->magic, 4);
	printf("\nmajor_ver:");print_hex(&raw_super->major_ver, 2);
	printf("\nuuid:");print_hex(&raw_super->uuid, 16);
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

	print_super(&super);

//	unmount_super();
	return 0;
}
