#ifndef __F2FS_TYPE_H__
#define __F2FS_TYPE_H__

typedef unsigned char __u8;
typedef unsigned short __le16;
typedef unsigned int __le32;
typedef unsigned long long __le64;

#define __packed __attribute__((packed))

/* LITTLE_ENDIAN */
static inline le16_to_cpu(__le16 le)
{
	return le;
}

static inline le32_to_cpu(__le32 le)
{
	return le;
}

static inline le64_to_cpu(__le64 le)
{
	return le;
}

#endif /*__F2FS_TYPE_H__*/
