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

static int f2fs_free_path(struct f2fs_super *super, struct path *path)
{
	struct path *next = path, *tmp;

	do {
		//printf("free:%s\n", next->inode->raw_inode->i_name);
		f2fs_put_inode(next->inode);
		tmp = next;
		next = next->next;
		free(tmp);
	} while(next != path);

	return 0;
}

static struct path *path_lookup(struct f2fs_super *super, char *dir)
{
	struct f2fs_inode *iter_pos;
	struct dir_iter *iter = NULL;
	int next_ino = 0;
	char name[256];
	int i = 0, nameoff = 0, found = 0;
	struct path *path, *tmp, *new;

	if(dir[0] != '/') {
		return NULL;
	}

	path = (void *)malloc(sizeof(struct path));
	if(path == NULL) {
		perror("malloc");
		return NULL;
	}

	path->next = path;
	path->prev = path;
	f2fs_get_inode(super->root);
	path->inode = super->root;
	while(1) {
		nameoff = 0;
		name[0] = '\0';

		/* skip // */
		for(; dir[i]!='\0'; i++) {
			if(dir[i] != '/') {
				break;
			}
		}

		for(; dir[i]!='\0'; i++) {
			if(dir[i] == '/' || dir[i] == '\0') {
				break;
			}
			name[nameoff++] = dir[i];
		}

		if(nameoff == 0) {
			break;
		}
		name[nameoff] = '\0';

		/* . */
		if(nameoff == 1 && name[0] == '.') {
			continue;
		}

		/* .. */
		if(nameoff == 2 && name[0] == '.' && name[1] == '.') {
			if(path->prev->inode == super->root) {
				goto out;
			}

			tmp = path->prev;
			tmp->prev->next = path;
			path->prev = tmp->prev;
			f2fs_put_inode(tmp->inode);
			free(tmp);
			continue;
		}

		found = 0;
		iter = dir_iter_start(super, path->prev->inode);
		while(iter_pos = dir_iter_next(iter)) {
			if(!strcmp(name, iter_pos->raw_inode->i_name)) {
				new = (void *)malloc(sizeof(struct path));
				if(new == NULL) {
					dir_iter_end(iter);
					goto out;
				}
				f2fs_get_inode(iter_pos);
				new->inode = iter_pos;
				new->next = path->prev->next;
				new->prev = path->prev;

				path->prev->next = new;
				path->prev = new;
				found = 1;
				break;
			}
		}
		dir_iter_end(iter);

		if(found == 0) {
			goto out;
		}
	}
	return path;

out:
	f2fs_free_path(super, path);
	return NULL;
}

int main(int argc, char **argv)
{
	struct f2fs_super super;
	struct dir_iter *iter;
	int ret = 0;
	struct f2fs_inode *pos = NULL;
	struct path *path = NULL;

	if(argc <= 2) {
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

//	print_super(&super);
//	print_checkpoint(&super);
	super.root = (void *)malloc(sizeof(struct f2fs_inode));
	if(super.root == NULL) {
		f2fs_umount(&super);
		return ret;
	}

	f2fs_read_inode(&super, super.root, le32_to_cpu(super.raw_super->root_ino));
	f2fs_get_inode(super.root);
	if(!S_ISDIR(le32_to_cpu(super.root->raw_inode->i_mode))) {
		printf("Error: the root was not a dir.\n");
		f2fs_put_inode(super.root);
		f2fs_umount(&super);
		return ret;
	}

	path = path_lookup(&super, argv[2]);
	if(path == NULL) {
		printf("No such file or directory:%s\n", argv[2]);
		f2fs_put_inode(super.root);
		f2fs_umount(&super);
		return -1;
	}

	if(S_ISDIR(le32_to_cpu(path->prev->inode->raw_inode->i_mode))) {
		printf("DIR : .\nDIR : ..\n");

		iter = dir_iter_start(&super, path->prev->inode);
		while(pos = dir_iter_next(iter)) {
			if(S_ISDIR(le32_to_cpu(pos->raw_inode->i_mode))) {
				printf("DIR : %s\n", pos->raw_inode->i_name);
			} else {
				printf("FILE: %s\n", pos->raw_inode->i_name);
			}
		}
		dir_iter_end(iter);
	} else {
		printf("FILE: %s\n", path->prev->inode->raw_inode->i_name);
	}

	f2fs_free_path(&super, path);
	f2fs_umount(&super);
	return 0;
}
