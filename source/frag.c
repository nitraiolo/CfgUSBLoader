#include <ogcsys.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "fat.h"
#include "ntfs_frag.h"
#include "ext2_frag.h"
#include "libwbfs/libwbfs.h"
#include "wbfs.h"
#include "wbfs_fat.h"
#include "usbstorage.h"
#include "frag.h"
#include "wpad.h"
#include "cfg.h"
#include "gettext.h"
#include "wdvd.h"

int _FAT_get_fragments (const char *path, _frag_append_t append_fragment, void *callback_data);

FragList *frag_list = NULL;


void frag_init(FragList *ff, int maxnum)
{
	memset(ff, 0, sizeof(Fragment) * (maxnum+1));
	ff->maxnum = maxnum;
}

void frag_dump(FragList *ff)
{
	if (CFG.debug == 0) return;
	int i;
	printf("frag list: %d %d %x\n", ff->num, ff->size, ff->size);
	for (i=0; i<ff->num; i++) {
		if (i>10) {
			printf("...\n");
			break;
		}
		printf(" %d : %8x %8x %8x\n", i,
				ff->frag[i].offset,
				ff->frag[i].count,
				ff->frag[i].sector);
	}
}

int frag_append(FragList *ff, u32 offset, u32 sector, u32 count)
{
	int n;
	if (count) {
		n = ff->num - 1;
		if (ff->num > 0
			&& ff->frag[n].offset + ff->frag[n].count == offset
			&& ff->frag[n].sector + ff->frag[n].count == sector)
		{
			// merge
			ff->frag[n].count += count;
		}
		else
		{
			// add
			if (ff->num >= ff->maxnum) {
				// too many fragments
				return -500;
			}
			n = ff->num;
			ff->frag[n].offset = offset;
			ff->frag[n].sector = sector;
			ff->frag[n].count  = count;
			ff->num++;
		}
	}
	ff->size = offset + count;
	return 0;
}

int _frag_append(void *ff, u32 offset, u32 sector, u32 count)
{
	return frag_append(ff, offset, sector, count);
}

int frag_concat(FragList *ff, FragList *src)
{
	int i, ret;
	u32 size = ff->size;
	//printf("concat: %d %d <- %d %d\n", ff->num, ff->size, src->num, src->size);
	for (i=0; i<src->num; i++) {
		ret = frag_append(ff, size + src->frag[i].offset,
				src->frag[i].sector, src->frag[i].count);
		if (ret) return ret;
	}
	ff->size = size + src->size;
	//printf("concat: -> %d %d\n", ff->num, ff->size);
	return 0;
}

// in case a sparse block is requested,
// the returned poffset might not be equal to requested offset
// the difference should be filled with 0
int frag_get(FragList *ff, u32 offset, u32 count,
		u32 *poffset, u32 *psector, u32 *pcount)
{
	int i;
	u32 delta;
	//printf("frag_get(%u %u)\n", offset, count);
	for (i=0; i<ff->num; i++) {
		if (ff->frag[i].offset <= offset
			&& ff->frag[i].offset + ff->frag[i].count > offset)
		{
			delta = offset - ff->frag[i].offset;
			*poffset = offset;
			*psector = ff->frag[i].sector + delta;
			*pcount = ff->frag[i].count - delta;
			if (*pcount > count) *pcount = count;
			goto out;
		}
		if (ff->frag[i].offset > offset
			&& ff->frag[i].offset < offset + count)
		{
			delta = ff->frag[i].offset - offset;
			*poffset = ff->frag[i].offset;
			*psector = ff->frag[i].sector;
			*pcount = ff->frag[i].count;
			count -= delta;
			if (*pcount > count) *pcount = count;
			goto out;
		}
	}
	// not found
	if (offset + count > ff->size) {
		// error: out of range!
		printf("ERROR: frag_get range %d %d %d\n", offset, count, ff->size);
		return -1;
	}
	// if inside range, then it must be just sparse, zero filled
	// return empty block at the end of requested
	*poffset = offset + count;
	*psector = 0;
	*pcount = 0;
	out:
	//printf("=>(%u %u %u)\n", *poffset, *psector, *pcount);
	return 0;
}

int frag_remap(FragList *ff, FragList *log, FragList *phy)
{
	int i;
	int ret;
	u32 offset;
	u32 sector;
	u32 count;
	u32 delta;
	for (i=0; i<log->num; i++) {
		delta = 0;
		count = 0;
		do {
			ret = frag_get(phy,
					log->frag[i].sector + delta + count,
					log->frag[i].count - delta - count,
					&offset, &sector, &count);
			if (ret) return ret; // error
			delta = offset - log->frag[i].sector;
			ret = frag_append(ff, log->frag[i].offset + delta, sector, count);
			if (ret) return ret; // error
			//frag_dump(ff); Wpad_WaitButtonsCommon();
		} while (count + delta < log->frag[i].count);
	}
	return 0;
}


int get_frag_list(u8 *id)
{
	char fname[1024];
	char fname1[1024];
	struct stat st;
	FragList *fs = NULL;
	FragList *fa = NULL;
	FragList *fw = NULL;
	int ret;
	int i, j;
	int is_wbfs = 0;
	int ret_val = -1;

	if (wbfs_part_fs == PART_FS_WBFS) return 0;

	ret = WBFS_FAT_find_fname(id, fname, sizeof(fname));
	if (!ret) return -2;

	if (strcasecmp(strrchr(fname,'.'), ".wbfs") == 0) {
		is_wbfs = 1;
	}

	fs = malloc(sizeof(FragList));
	fa = malloc(sizeof(FragList));
	fw = malloc(sizeof(FragList));

	frag_init(fa, MAX_FRAG);

	for (i=0; i<10; i++) {
		frag_init(fs, MAX_FRAG);
		if (i > 0) {
			fname[strlen(fname)-1] = '0' + i;
			if (stat(fname, &st) == -1) break;
		}
		strcpy(fname1, fname);
		//printf("::*%s\n", strrchr(fname,'/'));
		if (wbfs_part_fs == PART_FS_FAT) {
			ret = _FAT_get_fragments(fname, &_frag_append, fs);
			if (ret) {
				printf("ERROR: fat getf: %d\n", ret);
				// don't return failure, let it fallback to old method
				//ret_val = ret;
				ret_val = 0;
				goto out;
			}
		} else {
			if (wbfs_part_fs == PART_FS_NTFS) {
				ret = _NTFS_get_fragments(fname, &_frag_append, fs);
				if (ret) {
					printf("ERROR: ntfs getf: %d\n", ret);
					if (ret == -50 || ret == -500) {
						printf(gt("Too many fragments! %d"), fs->num);
						printf("\n");
					}
					if (ret == -31) {
						printf(gt("NTFS compression not supported!"));
						printf("\n");
					}
					if (ret == -32 || ret == -33 || ret == -35) {
						printf(gt("NTFS encryption not supported!"));
						printf("\n");
					}
					ret_val = ret;
					goto out;
				}
			} else if (wbfs_part_fs == PART_FS_EXT) {
				ret = _EXT2_get_fragments(fname, &_frag_append, fs);
				if (ret) {
					printf("ERROR: ext getf: %d\n", ret);
					ret_val = ret;
					goto out;
				}
			} else {
				printf("Unsupported FS %s %d!\n",
						get_fs_name(wbfs_part_fs), wbfs_part_fs);
			}
			// offset to start of partition
			int part_sec;
			MountPoint *m = mount_find(wbfs_fs_drive);
			if (!m) {
				printf("mount %s not found!\n", wbfs_fs_drive);
				ret_val = -3;
				goto out;
			}
			part_sec = m->sector;

			for (j=0; j<fs->num; j++) {
				fs->frag[j].sector += part_sec;
			}
		}
		frag_dump(fs);
		frag_concat(fa, fs);
	}

	if (i>0) {
		//printf("=====\n");
		frag_dump(fa);
	}

	frag_list = malloc(sizeof(FragList));
	frag_init(frag_list, MAX_FRAG);
	if (is_wbfs) {
		// if wbfs file format, remap.
		//printf("=====\n");
		wbfs_disc_t *d = WBFS_OpenDisc(id);
		if (!d) { ret_val = -4; goto out; }
		frag_init(fw, MAX_FRAG);
		dbg_printf("wbfs dev sector size: %d\n", wbfs_dev_sector_size);
		ret = wbfs_get_fragments(d, &_frag_append, fw, wbfs_dev_sector_size);
		if (ret) { ret_val = -5; goto out; }
		WBFS_CloseDisc(d);
		frag_dump(fw);
		// DEBUG: frag_list->num = MAX_FRAG-10; // stress test
		ret = frag_remap(frag_list, fw, fa);
		if (ret) { ret_val = -6; goto out; }
	} else {
		// .iso does not need remap just copy
		//printf("fa:\n");
		//frag_dump(fa);
		memcpy(frag_list, fa, sizeof(FragList));
	}

	//printf("frag_list:\n");
	frag_dump(frag_list);
	// ok
	ret_val = 0;

out:
	if (ret_val) {
		// error
		SAFE_FREE(frag_list);
	}
	SAFE_FREE(fs);
	SAFE_FREE(fa);
	SAFE_FREE(fw);
	return ret_val;
}

int set_frag_list(u8 *id)
{
	dbg_printf("set_frag_list %s\n", id);
	if (wbfs_part_fs == PART_FS_WBFS) return -1;
	if (frag_list == NULL) {
		/*
		if (wbfs_part_fs == PART_FS_FAT) {
			// fall back to old fat method
			printf(gt("FAT: fallback to old method"));
			printf("\n");
	   		return 0;
		}
		// ntfs has no fallback, return error
		*/
		return -2;
	}

	/*int xx = 66;
	WDVD_hello(&xx);
	printf("btn\n");
	Wpad_WaitButtonsCommon();*/
	
	//Wpad_WaitButtonsCommon();
	// (+1 for header which is same size as fragment)
	int size = sizeof(Fragment) * (frag_list->num + 1);
	int ret;
	DCFlushRange(frag_list, size);
	/*if (CFG.ios_mload) {
		ret = USBStorage_WBFS_SetFragList(frag_list, size);
	} else */{
		//USBStorage_Deinit();
		ret = WDVD_SetFragList(wbfsDev, frag_list, size);
	}
	dbg_printf("set_frag: %d\n", ret);
	
	
	/*int x = 66;
	ret = WDVD_hello(&x);
	Wpad_WaitButtonsCommon();
	printf("btn\n");*/

	if (ret) {
		printf("set_frag: %d\n", ret);
		return ret;
	}

	//usb_debug_dump(0);
	// verify id matches
	static char discid[32] ATTRIBUTE_ALIGN(32);
	memset(discid, 0, sizeof(discid));
	/*if (CFG.ios_mload) {
		ret = USBStorage_WBFS_Read(0, 8, discid);
	} else */{
		ret = WDVD_UnencryptedRead(discid, 32, 0);
	}
	dbg_printf("ID: [%.6s] [%.6s]\n", id, discid);
	//usb_debug_dump(0);
	//printf("r:%d id:%s\n", ret, discid);
	if (memcmp(id, discid, 6) != 0) {
		printf(gt("ERROR: setting up fragments %d %d"), ret, frag_list->num);
		printf("\n");
		printf(gt("ID mismatch: [%.6s] [%.6s]"), id, discid);
		printf("\n");
		Wpad_WaitButtonsCommon();
		return -3;
	}
	/*
	else {
		printf(gt("ID match: [%.6s] [%.6s]"), id, discid);
		printf("\n");
	}
	*/
	return 0;
}

