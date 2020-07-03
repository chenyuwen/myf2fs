#ifndef __F2FS_H__
#define __F2FS_H__
#include <errno.h>
#include "f2fs_type.h"

#define F2FS_SUPER_OFFSET               1024    /* byte-size offset */
#define F2FS_MAX_EXTENSION              64      /* # of extension entries */
#define F2FS_EXTENSION_LEN              8       /* max size of extension */

#define F2FS_MAX_QUOTAS         3

#define VERSION_LEN     256
#define MAX_VOLUME_NAME         512
#define MAX_PATH_LEN            64
#define MAX_DEVICES             8

/*
 * For superblock
 */
struct f2fs_device {
        __u8 path[MAX_PATH_LEN];
        __le32 total_segments;
} __packed; 

struct f2fs_super_block { 
        __le32 magic;                   /* Magic Number */
        __le16 major_ver;               /* Major Version */
        __le16 minor_ver;               /* Minor Version */
        __le32 log_sectorsize;          /* log2 sector size in bytes */
        __le32 log_sectors_per_block;   /* log2 # of sectors per block */
        __le32 log_blocksize;           /* log2 block size in bytes */
        __le32 log_blocks_per_seg;      /* log2 # of blocks per segment */
        __le32 segs_per_sec;            /* # of segments per section */
        __le32 secs_per_zone;           /* # of sections per zone */
        __le32 checksum_offset;         /* checksum offset inside super block */
        __le64 block_count;             /* total # of user blocks */
        __le32 section_count;           /* total # of sections */
        __le32 segment_count;           /* total # of segments */
        __le32 segment_count_ckpt;      /* # of segments for checkpoint */
        __le32 segment_count_sit;       /* # of segments for SIT */
        __le32 segment_count_nat;       /* # of segments for NAT */
        __le32 segment_count_ssa;       /* # of segments for SSA */
        __le32 segment_count_main;      /* # of segments for main area */
        __le32 segment0_blkaddr;        /* start block address of segment 0 */
        __le32 cp_blkaddr;              /* start block address of checkpoint */
        __le32 sit_blkaddr;             /* start block address of SIT */
        __le32 nat_blkaddr;             /* start block address of NAT */
        __le32 ssa_blkaddr;             /* start block address of SSA */
        __le32 main_blkaddr;            /* start block address of main area */
        __le32 root_ino;                /* root inode number */
        __le32 node_ino;                /* node inode number */
        __le32 meta_ino;                /* meta inode number */
        __u8 uuid[16];                  /* 128-bit uuid for volume */
        __le16 volume_name[MAX_VOLUME_NAME];    /* volume name */
        __le32 extension_count;         /* # of extensions below */
        __u8 extension_list[F2FS_MAX_EXTENSION][F2FS_EXTENSION_LEN];/* extension array */
        __le32 cp_payload;
        __u8 version[VERSION_LEN];      /* the kernel version */
        __u8 init_version[VERSION_LEN]; /* the initial kernel version */
        __le32 feature;                 /* defined features */
        __u8 encryption_level;          /* versioning level for encryption */
        __u8 encrypt_pw_salt[16];       /* Salt used for string2key algorithm */
        struct f2fs_device devs[MAX_DEVICES];   /* device list */
        __le32 qf_ino[F2FS_MAX_QUOTAS]; /* quota inode numbers */
        __u8 hot_ext_count;             /* # of hot file extension */
        __u8 reserved[310];             /* valid reserved region */
        __le32 crc;                     /* checksum of superblock */
} __packed;

struct f2fs_super {
	int fd;
	struct f2fs_super_block *raw_super;
};

#endif /*__F2FS_H__*/
