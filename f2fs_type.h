#ifndef __F2FS_TYPE_H__
#define __F2FS_TYPE_H__

typedef unsigned char __u8;
typedef unsigned short __le16;
typedef unsigned int __le32;
typedef unsigned long long __le64;

#define __packed __attribute__((packed))

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

#endif /*__F2FS_TYPE_H__*/
