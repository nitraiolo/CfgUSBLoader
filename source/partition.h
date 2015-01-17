#ifndef _PARTITION_H_
#define _PARTITION_H_

#include <gccore.h>

/* 'partition entry' structure */
typedef struct {
	/* Boot indicator */
	u8 boot;

	/* Starting CHS */
	u8 start[3];

	/* Partition type */
	u8 type;

	/* Ending CHS */
	u8 end[3];

	/* Partition sector */
	u32 sector;

	/* Partition size */
	u32 size;
} ATTRIBUTE_PACKED partitionEntry;

/* Constants */
#define MAX_PARTITIONS		4
#define MAX_PARTITIONS_EX	10

#define PART_TYPE_LINUX 0x83

#define PART_FS_WBFS 0
#define PART_FS_FAT  1
#define PART_FS_NTFS 2
#define PART_FS_EXT  3
#define PART_FS_UNK  4

#define PART_FS_FIRST 0
#define PART_FS_NUM  4

typedef struct
{
	int fs_type;
	// sequence indexes of a fs type starting with 1
	int fs_index;
} PartInfo;

typedef struct
{
	int num;
	u32 sector_size;
	partitionEntry pentry[MAX_PARTITIONS_EX];
	int fs_n[PART_FS_NUM];
	PartInfo pinfo[MAX_PARTITIONS_EX];
} PartList;

/* Prototypes */
s32 Partition_GetEntries(u32 device, partitionEntry *outbuf, u32 *outval);
s32 Partition_GetEntriesEx(u32 device, partitionEntry *outbuf, u32 *outval, int *num);
bool Device_ReadSectors(u32 device, u32 sector, u32 count, void *buffer);
bool Device_WriteSectors(u32 device, u32 sector, u32 count, void *buffer);
s32 Partition_GetList(u32 device, PartList *plist);
int Partition_FixEXT(u32 device, int part, u32 sec_size);
int PartList_FindFS(PartList *plist, int part_fstype, int seq_i, sec_t *sector);
int Partition_FindFS(u32 device, int part_fstype, int seq_i, sec_t *sector);

bool  part_is_extended(int type);
bool  part_is_data(int type);
char* part_type_data(int type);
const char* part_type_name(int type);
bool  part_valid_data(partitionEntry *entry);
int   get_fs_type(void *buf);
bool  is_type_fat(int type);
char* get_fs_name(int i);

#endif
