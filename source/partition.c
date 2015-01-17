// Modified by oggzee

#include <stdio.h>
#include <string.h>
#include <ogcsys.h>
#include <unistd.h>

#include "partition.h"
#include "sdhc.h"
#include "usbstorage.h"
#include "utils.h"
#include "wbfs.h"
#include "libwbfs/libwbfs.h"
#include "ext2.h"
#include "gettext.h"

/* 'partition table' structure */
typedef struct {
	/* Zero bytes */
	u8 padding[446];

	/* Partition table entries */
	partitionEntry entries[MAX_PARTITIONS];

	// 0x55 0xAA signature
	u8 sig_55aa[2];
} ATTRIBUTE_PACKED partitionTable;
// total size = 512

s32 Partition_GetEntries(u32 device, partitionEntry *outbuf, u32 *outval)
{
	static partitionTable table ATTRIBUTE_ALIGN(32);

	u32 cnt, sector_size;
	s32 ret;

	/* Read from specified device */
	switch (device) {
	case WBFS_DEVICE_USB: {
		/* Get sector size */
		ret = USBStorage_GetCapacity(&sector_size);
		if (ret == 0)
			return -1;

		/* Read partition table */
		u8* table_buf = memalign(32, sector_size);
		ret = USBStorage_ReadSectors(0, 1, table_buf);
		memcpy(&table, table_buf, sizeof(table));
		SAFE_FREE(table_buf);
		if (ret < 0)
			return ret;

		break;
	}

	case WBFS_DEVICE_SDHC: {
		/* SDHC sector size */
		sector_size = SDHC_SECTOR_SIZE;

		/* Read partition table */
		ret = SDHC_ReadSectors(0, 1, &table);
		if (!ret)
			return -1;

		break;
	}

	default:
		return -1;
	}


	/* Swap endianess */
	for (cnt = 0; cnt < 4; cnt++) {
		partitionEntry *entry = &table.entries[cnt];

		entry->sector = swap32(entry->sector);
		entry->size   = swap32(entry->size);
	}

	/* Set partition entries */
	memcpy(outbuf, table.entries, sizeof(table.entries));

	/* Set sector size */
	*outval = sector_size;

	return 0;
}

bool Device_ReadSectors(u32 device, u32 sector, u32 count, void *buffer)
{
	s32 ret;

	/* Read from specified device */
	switch (device) {
		case WBFS_DEVICE_USB:
			ret = USBStorage_ReadSectors(sector, count, buffer);
			if (ret < 0)
				return false;
			return true;

		case WBFS_DEVICE_SDHC:
			return SDHC_ReadSectors(sector, count, buffer);
	}

	return false;
}

bool Device_WriteSectors(u32 device, u32 sector, u32 count, void *buffer)
{
	s32 ret;

	/* Read from specified device */
	switch (device) {
		case WBFS_DEVICE_USB:
			ret = USBStorage_WriteSectors(sector, count, buffer);
			if (ret < 0)
				return false;
			return true;

		case WBFS_DEVICE_SDHC:
			return SDHC_WriteSectors(sector, count, buffer);
	}

	return false;
}

int is_valid_ptable(partitionTable *table)
{
	int i, n;
	partitionEntry *entry = &table->entries[0];
	// check if there is something that looks like a partition table
	n = 0;
	for (i=0; i<4; i++) {
		// boot (status) has to be 0 or 0x80
		if (entry[i].boot != 0 && entry[i].boot != 0x80) return 0;
		// check if partition used
		if (entry[i].type == 0) continue;
		n++;
		// type has to be between 0-0x27 or 0x80-0xff
		if (entry[i].type > 0x27 && entry[i].type < 0x80) return 0;
		// used partition start and size have to be > 0
		if (entry[i].start == 0 || entry[i].size == 0) return 0;
	}
	// there must be at least 1 valid partition
	if (n == 0) return 0;
	// all ok, looks like we have a partition table
	return 1;
}

s32 Partition_GetEntriesEx(u32 device, partitionEntry *outbuf, u32 *psect_size, int *num)
{
	static union {
		u8 buf[4096];
		partitionTable table;
		wbfs_head_t head;
	} tbl ATTRIBUTE_ALIGN(32);
	partitionTable *table = &tbl.table;
	partitionEntry *entry;

	u32 i, sector_size;
	s32 ret;
	int maxpart = *num;
	int is_raw = 0;

	// Get sector size
	switch (device) {
	case WBFS_DEVICE_USB:
		ret = USBStorage_GetCapacity(&sector_size);
		if (ret == 0) return -1;
		break;
	case WBFS_DEVICE_SDHC:
		sector_size = SDHC_SECTOR_SIZE;
		break;
	default:
		return -1;
	}
	/* Set sector size */
	*psect_size = sector_size;

	if (sector_size < 512 || sector_size > 4096) {
		printf("ERROR: sector size: %u\n", sector_size);
		sleep(5);
		return -2;
	}

	u32 ext = 0;
	u32 next = 0;

	// Read partition table
	ret = Device_ReadSectors(device, 0, 1, tbl.buf);
	if (!ret) return -1;
	// Check if it's a RAW FS disc, without partition table
	ret = get_fs_type(table);
	dbg_printf("fstype(%d)=%d\n", device, ret);
	if (ret != PART_FS_UNK) {
		// looks like a raw fs
		if (!is_valid_ptable(table)) {
			// if invalid part. table then yes it's raw
			is_raw = 1;
		} else {
			dbg_printf("WARNING: ambiguous part.table!\n", ret);
			// ambiguous: looks like a raw fs and a valid part. table
			// make a decision based on device type
			// sd: assume raw; usb: assume part. table
			if (device == WBFS_DEVICE_SDHC) {
				is_raw = 1;
			}
		}
	}
	if (is_raw) {
		dbg_printf("RAW\n", ret);
		memset(outbuf, 0, sizeof(table->entries));
		// create a fake partition entry
		if (ret == PART_FS_WBFS) {
			wbfs_head_t *head = &tbl.head;
			outbuf->size = wbfs_ntohl(head->n_hd_sec);
		} else {
			outbuf->size = 1;
		}
		if (ret == PART_FS_NTFS) {
			outbuf->type = 0x07;
		} else {
			outbuf->type = 0x0b;
		}
		*num = 1;
		return 0;
	}
	/* Swap endianess */
	for (i = 0; i < 4; i++) {
		entry = &table->entries[i];
		entry->sector = swap32(entry->sector);
		entry->size   = swap32(entry->size);
		if (!ext && part_is_extended(entry->type)) {
			ext = entry->sector;
		}
	}
	/* Set partition entries */
	memcpy(outbuf, table->entries, sizeof(table->entries));
	// num primary
	*num = 4;

	if (!ext) return 0;

	next = ext;
	// scan extended partition for logical
	for(i=0; i<maxpart-4; i++) {
		ret = Device_ReadSectors(device, next, 1, tbl.buf);
		if (!ret) break;
		if (i == 0) {
			// handle the invalid scenario where wbfs is on an EXTENDED
			// partition instead of on the Logical inside Extended.
			if (get_fs_type(table) == PART_FS_WBFS) break;
		}
		entry = &table->entries[0];
		entry->sector = swap32(entry->sector);
		entry->size   = swap32(entry->size);
		if (entry->type && entry->size && entry->sector) {
			// rebase to abolute address
			entry->sector += next;
			// add logical
			memcpy(&outbuf[*num], entry, sizeof(*entry));
			(*num)++;
			// get next
			entry++;
			if (entry->type && entry->size && entry->sector) {
				next = ext + swap32(entry->sector);
			} else {
				break;
			}
		}
	}

	return 0;
}

bool part_is_extended(int type)
{
	if (type == 0x05) return true;
	if (type == 0x0f) return true;
	return false;
}

bool part_is_data(int type)
{
	if (type && !part_is_extended(type)) return true;
	return false;
}

bool part_valid_data(partitionEntry *entry)
{
	if (entry->size && entry->type && entry->sector) {
		return part_is_data(entry->type);
	}
	return false;
}

char* part_type_data(int type)
{
	switch (type) {
		case 0x01: return "FAT12";
		case 0x04: return "FAT16";
		case 0x06: return "FAT16"; // FAT16+
		case 0x07: return "NTFS";
		case 0x0b: return "FAT32";
		case 0x0c: return "FAT32";
		case 0x0e: return "FAT16";
		case 0x82: return "LxSWP";
		case 0x83: return "LINUX";
		case 0x8e: return "LxLVM";
		case 0xa8: return "OSX";
		case 0xab: return "OSXBT";
		case 0xaf: return "OSXHF";
		case 0xe8: return "LUKS";
	}
	return NULL;
}

const char *part_type_name(int type)
{
	static char unk[8];
	if (type == 0) return gt("UNUSED");
	if (part_is_extended(type)) return gt("EXTEND");
	char *p = part_type_data(type);
	if (p) return p;
	sprintf(unk, "UNK-%02x", type);
	return unk;
}

int get_fs_type(void *buff)
{
	char *buf = buff;
	// WBFS
	wbfs_head_t *head = (wbfs_head_t *)buff;
	if (head->magic == wbfs_htonl(WBFS_MAGIC)) return PART_FS_WBFS;
	// 55AA
	if (buf[0x1FE] == 0x55 && buf[0x1FF] == 0xAA) {
		// FAT
		if (memcmp(buf+0x36,"FAT",3) == 0) return PART_FS_FAT; //FAT16;
		if (memcmp(buf+0x52,"FAT",3) == 0) return PART_FS_FAT; //FAT32;
		// NTFS
		if (memcmp(buf+0x03,"NTFS",4) == 0) return PART_FS_NTFS;
	}
	return PART_FS_UNK;
}

bool is_type_fat(int type)
{
	//return (type == PART_FS_FAT16 || type == PART_FS_FAT32);
	return (type == PART_FS_FAT);
}


char *get_fs_name(int i)
{
	switch (i) {
		case PART_FS_WBFS: return "WBFS";
		case PART_FS_FAT:  return "FAT";
		case PART_FS_NTFS: return "NTFS";
		case PART_FS_EXT:  return "EXT";
	}
	return "UNK";
}

s32 Partition_GetList(u32 device, PartList *plist)
{
	partitionEntry *entry = NULL;
	PartInfo *pinfo = NULL;
	int i, ret;
	int linux_found = 0;

	memset(plist, 0, sizeof(PartList));

	// Get partition entries
	plist->num = MAX_PARTITIONS_EX;
	ret = Partition_GetEntriesEx(device, plist->pentry, &plist->sector_size, &plist->num);
	if (ret < 0) {
		return -1;
	}

	char buf[plist->sector_size];

	dbg_printf("Plist(%d)=%d ss:%u\n", device, plist->num, plist->sector_size);
	// scan partitions for filesystem type
	for (i = 0; i < plist->num; i++) {
		pinfo = &plist->pinfo[i];
		entry = &plist->pentry[i];
		pinfo->fs_type = PART_FS_UNK;
		if (entry->sector || entry->size || entry->type) {
			dbg_printf("P#%d %u %u %d\n", i, 
				entry->sector, entry->size, entry->type);
		}
		if (!entry->size) continue;
		if (!entry->type) continue;
		if (!entry->sector) {
			// RAW partition will start at sector 0
			if (plist->num != 1) continue;
		}
		// even though wrong, it's possible WBFS is on an extended part.
		//if (!part_is_data(entry->type)) continue;
		if (!Device_ReadSectors(device, entry->sector, 1, buf)) continue;
		pinfo->fs_type = get_fs_type(buf);
		if (pinfo->fs_type == PART_FS_WBFS) {
			// multiple wbfs on sdhc not supported
			if (device == WBFS_DEVICE_SDHC && (plist->fs_n[PART_FS_WBFS] > 1 || i > 4)) {
				continue;
			}
		}
		if (pinfo->fs_type != PART_FS_UNK) {
			plist->fs_n[pinfo->fs_type]++;
			pinfo->fs_index = plist->fs_n[pinfo->fs_type];
		} else if (entry->type == PART_TYPE_LINUX) {
			linux_found = 1;
		}
	}
	// scan for linux ext2fs
	if (linux_found) {
		const DISC_INTERFACE *io;
		sec_t *ext_part;
		int j;
		if (device == WBFS_DEVICE_SDHC) {
			io = &my_io_sdhc_ro;
		} else {
			io = &my_io_usbstorage_ro;
		}
		ret = ext2FindPartitions(io, &ext_part);
		if (ret > 0 && ext_part) {
			for (i = 0; i < plist->num; i++) {
				pinfo = &plist->pinfo[i];
				entry = &plist->pentry[i];
				if (!entry->sector || !entry->size || !entry->type) continue;
				if (pinfo->fs_type != PART_FS_UNK) continue;
				for (j=0; j<ret; j++) {
					if (ext_part[j] == entry->sector) {
						pinfo->fs_type = PART_FS_EXT;
						plist->fs_n[pinfo->fs_type]++;
						pinfo->fs_index = plist->fs_n[pinfo->fs_type];
						break;
					}
				}
			}
			SAFE_FREE(ext_part);
		}
	}

	return 0;
}


int Partition_FixEXT(u32 device, int part, u32 sec_size)
{
	static partitionTable table ATTRIBUTE_ALIGN(32);
	int ret;

	if (sec_size != 512) return -1;
	if (part < 0 || part > 3) return -1;
	// Read partition table
	ret = Device_ReadSectors(device, 0, 1, &table);
	if (!ret) return -1;
	if (part_is_extended(table.entries[part].type)) {
		table.entries[part].type = 0x0b; // FAT32 
		ret = Device_WriteSectors(device, 0, 1, &table);
		if (!ret) return -1;
		return 0;
	}
	return -1;
}

int PartList_FindFS(PartList *plist, int part_fstype, int seq_i, sec_t *sector)
{
	int i;
	PartInfo *pinfo = NULL;
	dbg_printf("part_find(%d %d)\n");
	if (seq_i <= 0) return -2; // index has to start with 1
	for (i=0; i<plist->num; i++) {
		pinfo = &plist->pinfo[i];
		if (pinfo->fs_type == part_fstype && pinfo->fs_index == seq_i) {
			goto found;
		}
	}
	// not found
	return -1;

found:
	if (sector) {
		*sector = plist->pentry[i].sector;
	}
	//dbg_printf("Part found: %u\n", *sector);
	return i;
}

int Partition_FindFS(u32 device, int part_fstype, int seq_i, sec_t *sector)
{
	int ret;
	PartList plist;

	//dbg_printf("Part_Find(%d, %d, %d)\n", device, part_fstype, seq_i);
	if (seq_i <= 0) return -2; // index has to start with 1

	ret = Partition_GetList(device, &plist);
	if (ret < 0) return ret;

	ret = PartList_FindFS(&plist, part_fstype, seq_i, sector);
	if (ret < 0) return ret;
	return 0;
}


