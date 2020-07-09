#ifndef __F2FS_TYPE_H__
#define __F2FS_TYPE_H__
#include <stdlib.h>

#define BITS_PER_BYTE 8
typedef unsigned char __u8;
typedef unsigned short __le16;
typedef unsigned int __le32;
typedef unsigned long long __le64;
typedef unsigned long inode_t;
typedef unsigned long long block_t;

#define __packed __attribute__((packed))

static inline void __BUG__()
{
	char *buf = malloc(sizeof(char));
	free(buf);free(buf);

	buf = NULL;
	*buf = 0;

	exit(-1);
}

#define BUG(format, ...) do { \
	printf(format, ##__VA_ARGS__); \
	__BUG__(); \
} while(0);

#define offsetof(TYPE, MEMBER)  ((size_t)&((TYPE *)0)->MEMBER)

/* LITTLE_ENDIAN */
static inline unsigned short le16_to_cpu(__le16 le)
{
	return le;
}

static inline unsigned int le32_to_cpu(__le32 le)
{
	return le;
}

static inline unsigned long long le64_to_cpu(__le64 le)
{
	return le;
}

extern int malloc_count;
static inline void *f2fs_malloc(size_t size)
{
	void *ptr = malloc(size);
	if(ptr != NULL) {
		malloc_count++;
	}
	return ptr;
}

static inline void f2fs_free(void *pt)
{
	if(pt != NULL) {
		malloc_count--;
		free(pt);
	}
}

#undef free
#define free(ptr) f2fs_free(ptr)

#undef malloc
#define malloc(size) f2fs_malloc(size)
#endif /*__F2FS_TYPE_H__*/
