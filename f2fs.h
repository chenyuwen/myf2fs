#ifndef __F2FS_H__
#define __F2FS_H__
#include <errno.h>
#include "f2fs_type.h"

#define F2FS_SUPER_MAGIC        0xF2F52010

#define F2FS_SUPER_OFFSET               1024    /* byte-size offset */
#define F2FS_MAX_EXTENSION              64      /* # of extension entries */
#define F2FS_EXTENSION_LEN              8       /* max size of extension */

#define F2FS_MAX_QUOTAS         3

/*
 * For further optimization on multi-head logs, on-disk layout supports maximum
 * 16 logs by default. The number, 16, is expected to cover all the cases
 * enoughly. The implementaion currently uses no more than 6 logs.
 * Half the logs are used for nodes, and the other half are used for data.
 */
#define MAX_ACTIVE_LOGS 16
#define MAX_ACTIVE_NODE_LOGS    8
#define MAX_ACTIVE_DATA_LOGS    8

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

struct f2fs_checkpoint {
        __le64 checkpoint_ver;          /* checkpoint block version number */
        __le64 user_block_count;        /* # of user blocks */
        __le64 valid_block_count;       /* # of valid blocks in main area */
        __le32 rsvd_segment_count;      /* # of reserved segments for gc */
        __le32 overprov_segment_count;  /* # of overprovision segments */
        __le32 free_segment_count;      /* # of free segments in main area */

        /* information of current node segments */ 
        __le32 cur_node_segno[MAX_ACTIVE_NODE_LOGS];
        __le16 cur_node_blkoff[MAX_ACTIVE_NODE_LOGS];
        /* information of current data segments */
        __le32 cur_data_segno[MAX_ACTIVE_DATA_LOGS];
        __le16 cur_data_blkoff[MAX_ACTIVE_DATA_LOGS];
        __le32 ckpt_flags;              /* Flags : umount and journal_present */
        __le32 cp_pack_total_block_count;       /* total # of one cp pack */
        __le32 cp_pack_start_sum;       /* start block number of data summary */
        __le32 valid_node_count;        /* Total number of valid nodes */
        __le32 valid_inode_count;       /* Total number of valid inodes */
        __le32 next_free_nid;           /* Next free node number */
        __le32 sit_ver_bitmap_bytesize; /* Default value 64 */
        __le32 nat_ver_bitmap_bytesize; /* Default value 256 */
        __le32 checksum_offset;         /* checksum offset inside cp block */
        __le64 elapsed_time;            /* mounted time */
        /* allocation type of current segment */
        unsigned char alloc_type[MAX_ACTIVE_LOGS];

        /* SIT and NAT version bitmap */
        unsigned char sit_nat_version_bitmap[1];
} __packed;

struct f2fs_super {
	int fd;
	struct f2fs_super_block *raw_super;
	struct f2fs_checkpoint *raw_cp;
};

#endif /*__F2FS_H__*/
