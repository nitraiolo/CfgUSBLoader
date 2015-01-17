
// FAT support, banner sounds and alt.dol by oggzee
// Banner title for playlog by Clipper

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <ogcsys.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/statvfs.h>
#include <ctype.h>
#include <sdcard/wiisd_io.h>

#include "libwbfs/libwbfs.h"
#include "sdhc.h"
#include "usbstorage.h"
#include "utils.h"
#include "video.h"
#include "wbfs.h"
#include "wdvd.h"
#include "subsystem.h"
#include "splits.h"
#include "fat.h"
#include "partition.h"
#include "cfg.h"
#include "wbfs_fat.h"
#include "util.h"
#include "gettext.h"

/* Constants */
#define MAX_NB_SECTORS	32

/* WBFS device */
s32 wbfsDev = WBFS_MIN_DEVICE;

// partition
int wbfs_part_fs  = PART_FS_WBFS;
u32 wbfs_part_idx = 0;
u32 wbfs_part_lba = 0;

/* WBFS HDD */
wbfs_t *hdd = NULL;

/* WBFS callbacks */
static rw_sector_callback_t readCallback  = NULL;
static rw_sector_callback_t writeCallback = NULL;

/* Variables */
static u32 nb_sectors;
u32 wbfs_dev_sector_size;

static mutex_t wbfs_disc_mutex = LWP_MUTEX_NULL;

void __WBFS_Spinner(s32 x, s32 max)
{
	static time_t start;
	static u32 expected;

	f32 percent, size;
	u32 d, h, m, s;

	/* First time */
	if (!x) {
		start    = time(0);
		expected = 300;
	}

	/* Elapsed time */
	d = time(0) - start;

	if (x != max) {
		/* Expected time */
		if (d && x)
			expected = (expected * 3 + d * max / x) / 4;

		/* Remaining time */
		d = (expected > d) ? (expected - d) : 1;
	}

	/* Calculate time values */
	h =  d / 3600;
	m = (d / 60) % 60;
	s =  d % 60;

	/* Calculate percentage/size */
	percent = (x * 100.0) / max;
	if (hdd) {
		size = (hdd->wii_sec_sz / GB_SIZE) * max;
	} else {
		size = (0x8000 / GB_SIZE) * max;
	}

	Con_ClearLine();

	/* Show progress */
	if (x != max) {
		printf_(gt("%.2f%% of %.2fGB (%c) ETA: %d:%02d:%02d"),
				percent, size, "/-\\|"[(x / 10) % 4], h, m, s);
		printf("\r");
		fflush(stdout);
	} else {
		printf_(gt("%.2fGB copied in %d:%02d:%02d"), size, h, m, s);
		printf("  \n");
	}

	__console_flush(1);
}

s32 __WBFS_ReadDVD(void *fp, u32 lba, u32 len, void *iobuf)
{
	void *buffer = NULL;

	u64 offset;
	u32 mod, size;
	s32 ret;

	/* Calculate offset */
	offset = ((u64)lba) << 2;

	/* Calcualte sizes */
	mod  = len % 32;
	size = len - mod;

	/* Read aligned data */
	if (size) {
		ret = WDVD_UnencryptedRead(iobuf, size, offset);
		if (ret < 0)
			goto out;
	}

	/* Read non-aligned data */
	if (mod) {
		/* Allocate memory */
		buffer = memalign(32, 0x20);
		if (!buffer)
			return -1;

		/* Read data */
		ret = WDVD_UnencryptedRead(buffer, 0x20, offset + size);
		if (ret < 0)
			goto out;

		/* Copy data */
		memcpy(iobuf + size, buffer, mod);
	}

	/* Success */
	ret = 0;

out:
	/* Free memory */
	if (buffer)
		free(buffer);

	return ret;
}

s32 __WBFS_ReadUSB(void *fp, u32 lba, u32 count, void *iobuf)
{
	u32 cnt = 0;
	s32 ret;

	/* Do reads */
	while (cnt < count) {
		void *ptr     = ((u8 *)iobuf) + (cnt * wbfs_dev_sector_size);
		u32   sectors = (count - cnt);

		/* Read sectors is too big */
		if (sectors > MAX_NB_SECTORS)
			sectors = MAX_NB_SECTORS;

		/* USB read */
		ret = USBStorage_ReadSectors(lba + cnt, sectors, ptr);
		if (ret < 0)
			return ret;

		/* Increment counter */
		cnt += sectors;
	}

	return 0;
}

s32 __WBFS_WriteUSB(void *fp, u32 lba, u32 count, void *iobuf)
{
	u32 cnt = 0;
	s32 ret;

	/* Do writes */
	while (cnt < count) {
		void *ptr     = ((u8 *)iobuf) + (cnt * wbfs_dev_sector_size);
		u32   sectors = (count - cnt);

		/* Write sectors is too big */
		if (sectors > MAX_NB_SECTORS)
			sectors = MAX_NB_SECTORS;

		/* USB write */
		ret = USBStorage_WriteSectors(lba + cnt, sectors, ptr);
		if (ret < 0)
			return ret;

		/* Increment counter */
		cnt += sectors;
	}

	return 0;
}

s32 __WBFS_ReadSDHC(void *fp, u32 lba, u32 count, void *iobuf)
{
	u32 cnt = 0;
	s32 ret;

	/* Do reads */
	while (cnt < count) {
		void *ptr     = ((u8 *)iobuf) + (cnt * wbfs_dev_sector_size);
		u32   sectors = (count - cnt);

		/* Read sectors is too big */
		if (sectors > MAX_NB_SECTORS)
			sectors = MAX_NB_SECTORS;

		/* SDHC read */
		ret = SDHC_ReadSectors(lba + cnt, sectors, ptr);
		if (!ret)
			return -1;

		/* Increment counter */
		cnt += sectors;
	}

	return 0;
}

s32 __WBFS_WriteSDHC(void *fp, u32 lba, u32 count, void *iobuf)
{
	u32 cnt = 0;
	s32 ret;

	/* Do writes */
	while (cnt < count) {
		void *ptr     = ((u8 *)iobuf) + (cnt * wbfs_dev_sector_size);
		u32   sectors = (count - cnt);

		/* Write sectors is too big */
		if (sectors > MAX_NB_SECTORS)
			sectors = MAX_NB_SECTORS;

		/* SDHC write */
		ret = SDHC_WriteSectors(lba + cnt, sectors, ptr);
		if (!ret)
			return -1;

		/* Increment counter */
		cnt += sectors;
	}

	return 0;
}


s32 WBFS_Init_Dev(u32 device)
{
	s32 ret = -1;

	if (wbfs_disc_mutex == LWP_MUTEX_NULL) {
		LWP_MutexInit(&wbfs_disc_mutex, true);
	}

	/* Try to mount device */
	switch (device) {
		case WBFS_DEVICE_USB:
			{
				long long t1 = TIME_D(usb_init);
				long long t2;
				get_time(&TIME.usb_retry1);
				/* Initialize USB storage */
				ret = USBStorage_Init();

				if (ret >= 0) {
					/* Setup callbacks */
					readCallback  = __WBFS_ReadUSB;
					writeCallback = __WBFS_WriteUSB;

					/* Device info */
					nb_sectors = USBStorage_GetCapacity(&wbfs_dev_sector_size);

					get_time(&TIME.usb_retry2);
					t2 = TIME_D(usb_init);
					TIME.usb_retry2 -= (t2 - t1);
					goto out;
				}
			}
			break;

		case WBFS_DEVICE_SDHC:
			{
				/* Initialize SDHC */
				ret = SDHC_Init();
				// returns true=ok false=error 
				if (!ret && !sdhc_mode_sd) {
					// try normal SD
					sdhc_mode_sd = 1;
					ret = SDHC_Init();
				}
				if (ret) {
					/* Setup callbacks */
					readCallback  = __WBFS_ReadSDHC;
					writeCallback = __WBFS_WriteSDHC;

					/* Device info */
					nb_sectors  = 0;
					wbfs_dev_sector_size = SDHC_SECTOR_SIZE;

					goto out;
				}

				ret = -1;
			}
			break;

		default:
			return -1;
	}
out:
	return ret;
}

s32 WBFS_Init(u32 device, u32 timeout)
{
	u32 cnt;
	s32 ret = -1;

	/* Wrong timeout */
	if (!timeout)
		return -1;

	/* Try to mount device */
	for (cnt = 0; cnt < timeout; cnt++) {

		ret = WBFS_Init_Dev(device);
		if (ret >= 0) break;

		Gui_Console_Enable();
		printf("%d ", cnt + 1);

		/* Sleep 1 second */
		sleep(1);
	}

	printf_("%s\n", ret < 0 ? gt("ERROR!") : gt("OK!"));
	return ret;
}

bool WBFS_Close()
{
	/* Close hard disk */
	if (hdd) {
		wbfs_close(hdd);
		hdd = NULL;
	}
	UnmountFS(GAME_MOUNT);
	wbfs_part_fs = 0;
	wbfs_part_idx = 0;
	wbfs_part_lba = 0;
	strcpy(wbfs_fs_drive, "");

	return 0;
}

bool WBFS_Mounted()
{
	return (hdd != NULL);
}

bool WBFS_Selected()
{
	//if (wbfs_part_fs && wbfs_part_lba && *wbfs_fs_drive) return true;
	// RAW device fs will have lba=0
	if (wbfs_part_fs && *wbfs_fs_drive) return true;
	return WBFS_Mounted();
}

s32 WBFS_Open(void)
{
	/* Close hard disk */
	if (hdd)
		wbfs_close(hdd);
	
	/* Open hard disk */
	wbfs_part_fs = 0;
	wbfs_part_idx = 0;
	wbfs_part_lba = 0;
	hdd = wbfs_open_hd(readCallback, writeCallback, NULL, wbfs_dev_sector_size, nb_sectors, 0);
	if (!hdd)
		return -1;
	wbfs_part_idx = 1;
	wbfs_part_lba = hdd->part_lba;

	return 0;
}

s32 WBFS_OpenPart(u32 part_fs, u32 part_idx, u32 part_lba, u32 part_size, char *partition)
{
	// close
	WBFS_Close();

	dbg_printf("openpart(%d %d %d %d)\n", part_fs, part_idx, part_lba, part_size);
	if (part_fs == PART_FS_UNK) return -1;
	if (part_fs == PART_FS_WBFS) {
		if (WBFS_OpenLBA(part_lba, part_size)) return -1;
	} else {
		MountPoint *mp = mount_find_part(wbfsDev, part_lba);
		if (mp) {
			mount_name2drive(mp->name, wbfs_fs_drive);
		} else {
			if (MountFS(GAME_MOUNT, wbfsDev, part_lba, part_fs, 1)) return -1;
			mount_name2drive(GAME_MOUNT, wbfs_fs_drive);
		}
	}

	// success
	wbfs_part_fs  = part_fs;
	wbfs_part_idx = part_idx;
	wbfs_part_lba = part_lba;
	sprintf(partition, "%s%d", get_fs_name(part_fs), wbfs_part_idx);
	dbg_printf("game part=%s\n", partition);
	return 0;
}

bool is_game_fs(int device, sec_t sector)
{
	char path[100];
	struct stat st;
	MountPoint *mp;
	dbg_printf("is_game_fs(%d,%d)\n", device, sector);
	mp = mount_find_part(device, sector);
	if (mp) {
		dbg_printf("check %s:%s\n", mp->name, CFG.wbfs_fat_dir);
		sprintf(path, "%s:%s", mp->name, CFG.wbfs_fat_dir);
		if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
			return true;
		}
	}
	return false;
}

s32 WBFS_OpenNamed(char *partition)
{
	int i = 0;
	u32 part_fs  = PART_FS_WBFS;
	u32 part_idx = 0;
	u32 part_lba = 0;
	s32 ret = 0;
	PartList plist;
	int x, fs;

	// close
	WBFS_Close();

	dbg_printf("open_part(%s)\n", partition);

	// Get partition entries
	ret = Partition_GetList(wbfsDev, &plist);
	if (ret || plist.num == 0) return -1;

	// parse partition option
	if (strcasecmp(partition, "auto") == 0) {
		int fs_list[] = { PART_FS_WBFS, PART_FS_FAT, PART_FS_NTFS }; // PART_FS_EXT
		int n = sizeof(fs_list) / sizeof(int);
		for (x=0; x<n; x++) {
			fs = fs_list[x];
			i = PartList_FindFS(&plist, fs, 1, NULL);
			if (i < 0) continue;
			if ((fs == PART_FS_WBFS) || is_game_fs(wbfsDev, plist.pentry[i].sector)) {
				part_fs = fs;
				part_idx = 1;
				goto found;
			}
		}
	} else {
		for (x=0; x<PART_FS_NUM; x++) {
			fs = PART_FS_FIRST + x;
			char *fsname = get_fs_name(fs);
			int len = strlen(fsname);
			if (strncasecmp(partition, fsname, len) == 0) {
				int idx = atoi(partition + len);
				if (idx < 1 || idx > 9) goto err;
				i = PartList_FindFS(&plist, fs, idx, NULL);
				if (i >= 0) {
					part_fs = fs;
					part_idx = idx;
					goto found;
				}
			}
		}
	}
	// nothing found
	goto err;

found:
	if (i >= plist.num) goto err;
	// set partition lba sector
	part_lba = plist.pentry[i].sector;

	if (WBFS_OpenPart(part_fs, part_idx, part_lba, plist.pentry[i].size, partition)) {
		goto err;
	}
	// success
	dbg_printf("OK! partition=%s\n", partition);
	return 0;

err:
	Gui_Console_Enable();
	printf(gt("Invalid partition: '%s'"), partition);
	printf("\n");
	__console_flush(0);
	sleep(2);
	return -1;
}


s32 WBFS_OpenLBA(u32 lba, u32 size)
{
	wbfs_t *part = NULL;

	/* Open partition */
	part = wbfs_open_partition(readCallback, writeCallback, NULL, wbfs_dev_sector_size, size, lba, 0);
	if (!part) return -1;

	/* Close current hard disk */
	if (hdd) wbfs_close(hdd);
	hdd = part;

	return 0;
}

s32 WBFS_Format(u32 lba, u32 size)
{
	wbfs_t *partition = NULL;

	/* Reset partition */
	partition = wbfs_open_partition(readCallback, writeCallback, NULL, wbfs_dev_sector_size, size, lba, 1);
	if (!partition)
		return -1;

	/* Free memory */
	wbfs_close(partition);

	return 0;
}

s32 WBFS_GetCount(u32 *count)
{
	if (wbfs_part_fs) return WBFS_FAT_GetCount(count);

	/* No device open */
	if (!hdd)
		return -1;

	/* Get list length */
	*count = wbfs_count_discs(hdd);

	return 0;
}

s32 WBFS_GetHeaders(void *outbuf, u32 cnt, u32 len)
{
	if (wbfs_part_fs) return WBFS_FAT_GetHeaders(outbuf, cnt, len);

	u32 idx, size;
	s32 ret;

	/* No device open */
	if (!hdd)
		return -1;

	for (idx = 0; idx < cnt; idx++) {
		u8 *ptr = ((u8 *)outbuf) + (idx * len);

		/* Get header */
		ret = wbfs_get_disc_info(hdd, idx, ptr, len, &size);
		if (ret != 0)
			return ret;
	}

	return 0;
}

s32 WBFS_CheckGame(u8 *discid)
{
	wbfs_disc_t *disc = NULL;

	/* Try to open game disc */
	disc = WBFS_OpenDisc(discid);
	if (disc) {
		/* Close disc */
		WBFS_CloseDisc(disc);
		return 1;
	}

	return 0;
}

s32 WBFS_AddGame(void)
{
	if (wbfs_part_fs) return WBFS_FAT_AddGame();
	s32 ret;

	/* No device open */
	if (!hdd)
		return -1;

	/* Add game to device */
	partition_selector_t part_sel = ALL_PARTITIONS;
	int copy_1_1 = 0;
	switch (CFG.install_partitions) {
		default:
		case CFG_INSTALL_GAME:
			part_sel = ONLY_GAME_PARTITION;
			break;
		case CFG_INSTALL_ALL:
			part_sel = ALL_PARTITIONS;
			break;
		case CFG_INSTALL_1_1:
		case CFG_INSTALL_ISO:
			part_sel = ALL_PARTITIONS;
			copy_1_1 = 1;
			break;
	}
	ret = wbfs_add_disc(hdd, __WBFS_ReadDVD, NULL, __WBFS_Spinner, part_sel, copy_1_1);
	if (ret < 0)
		return ret;

	return 0;
}

s32 WBFS_RemoveGame(u8 *discid)
{
	if (wbfs_part_fs) return WBFS_FAT_RemoveGame(discid);
	s32 ret;

	/* No device open */
	if (!hdd)
		return -1;

	/* Remove game from device */
	ret = wbfs_rm_disc(hdd, discid);
	if (ret < 0)
		return ret;

	return 0;
}

s32 WBFS_GameSize(u8 *discid, f32 *size)
{
	wbfs_disc_t *disc = NULL;

	u32 sectors;

	/* Open disc */
	disc = WBFS_OpenDisc(discid);
	if (!disc)
		return -2;

	/* Get game size in sectors */
	sectors = wbfs_disc_sector_used(disc, NULL);

	/* Copy value */
	*size = (disc->p->wbfs_sec_sz / GB_SIZE) * sectors;

	/* Close disc */
	WBFS_CloseDisc(disc);

	return 0;
}

s32 WBFS_GameSize2(u8 *discid, u64 *comp_size, u64 *real_size)
{
	wbfs_disc_t *disc = NULL;

	u32 sectors, real_sec;

	/* Open disc */
	disc = WBFS_OpenDisc(discid);
	if (!disc)
		return -2;

	/* Get game size in sectors */
	sectors = wbfs_disc_sector_used(disc, &real_sec);

	/* Copy value */
	*comp_size = ((u64)disc->p->wbfs_sec_sz) * sectors;
	*real_size = ((u64)disc->p->wbfs_sec_sz) * real_sec;

	/* Close disc */
	WBFS_CloseDisc(disc);

	return 0;
}

s32 WBFS_DVD_Size(u64 *comp_size, u64 *real_size)
{
	if (wbfs_part_fs) return WBFS_FAT_DVD_Size(comp_size, real_size);
	s32 ret;
	u32 comp_sec = 0, last_sec = 0;

	/* No device open */
	if (!hdd)
		return -1;

	/* Add game to device */
	partition_selector_t part_sel;
	if (CFG.install_partitions) {
		part_sel = ALL_PARTITIONS;
	} else {
		part_sel = ONLY_GAME_PARTITION;
	}
	ret = wbfs_size_disc(hdd, __WBFS_ReadDVD, NULL, part_sel, &comp_sec, &last_sec);
	if (ret < 0)
		return ret;

	*comp_size = ((u64)hdd->wii_sec_sz) * comp_sec;
	*real_size = ((u64)hdd->wii_sec_sz) * (last_sec+1);

	return 0;
}


s32 WBFS_DiskSpace(f32 *used, f32 *free)
{
	if (wbfs_part_fs) return WBFS_FAT_DiskSpace(used, free);
	f32 ssize;
	u32 cnt;

	/* No device open */
	if (!hdd)
		return -1;

	/* Count used blocks */
	cnt = wbfs_count_usedblocks(hdd);

	/* Sector size in GB */
	ssize = hdd->wbfs_sec_sz / GB_SIZE;

	/* Copy values */
	*free = ssize * cnt;
	*used = ssize * (hdd->n_wbfs_sec - cnt);

	return 0;
}

wbfs_disc_t* WBFS_OpenDisc(u8 *discid)
{
	wbfs_disc_t *ret = NULL;
	//dbg_printf("WBFS_OpenDisc(%.6s) %d\n", discid, wbfs_part_fs);
	LWP_MutexLock(wbfs_disc_mutex);
	if (wbfs_part_fs) {
		ret = WBFS_FAT_OpenDisc(discid);
	} else {
		if (!hdd) {
			/* No device open */
			ret = NULL;
		} else {
			/* Open disc */
			ret = wbfs_open_disc(hdd, discid);
		}
	}
	if (ret == NULL) {
		LWP_MutexUnlock(wbfs_disc_mutex);
	}
	return ret;
}

void WBFS_CloseDisc(wbfs_disc_t *disc)
{
	if (wbfs_part_fs) {
		WBFS_FAT_CloseDisc(disc);
	} else {
		if (hdd && disc) {
			/* Close disc */
			wbfs_close_disc(disc);
		}
	}
	if (disc) {
		LWP_MutexUnlock(wbfs_disc_mutex);
	}
}

typedef struct {
	u8 filetype;
	char name_offset[3];
	u32 fileoffset;
	u32 filelen;
} __attribute__((packed)) FST_ENTRY;


char *fstfilename2(FST_ENTRY *fst, u32 index)
{
	u32 count = _be32((u8*)&fst[0].filelen);
	u32 stringoffset;
	if (index < count)
	{
		//stringoffset = *(u32 *)&(fst[index]) % (256*256*256);
		stringoffset = _be32((u8*)&(fst[index])) % (256*256*256);
		return (char *)((u32)fst + count*12 + stringoffset);
	} else
	{
		return NULL;
	}
}

int WBFS_GetDolList(u8 *discid, DOL_LIST *list)
{
	FST_ENTRY *fst = NULL;
	int fst_size;

	list->num = 0;

	wbfs_disc_t* d = WBFS_OpenDisc(discid);
	if (!d) return -1;
	fst_size = wbfs_extract_file(d, "", (void*)&fst);
	WBFS_CloseDisc(d);
	if (!fst || fst_size < 0) return -1;

	u32 count = _be32((u8*)&fst[0].filelen);
	u32 i;

	for (i=1;i<count;i++) {
		char * fname = fstfilename2(fst, i);
		int len = strlen(fname);
		if (len > 4 && stricmp(fname+len-4, ".dol") == 0) {
			if (list->num >= DOL_LIST_MAX) break;
			STRCOPY(list->name[list->num], fname);
			list->num++;
		}
	}

	free(fst);
	return 0;
}

int WBFS_Banner(u8 *discid, SoundInfo *snd, u8 *title, u8 getSound, u8 getTitle)
{
	void *banner = NULL;
	int size;
	//dbg_printf("WBFS_Banner(%.6s %d %d)\n", discid, getSound, getTitle);

	if (!getSound && !getTitle) return 0;

	snd->dsp_data = NULL;
	wbfs_disc_t* d = WBFS_OpenDisc(discid);
	if (!d) return -1;
	size = wbfs_extract_file(d, "opening.bnr", &banner);
	WBFS_CloseDisc(d);
	if (!banner || size <= 0) return -1;

	//printf("\nopening.bnr: %d\n", size);
	if (getTitle) 
	{
		s32 lang = getTitle - 2;
		if (lang < 0)
			lang = CFG_read_active_game_setting(discid).language - 1;
		if (lang < 0)
			lang = CONF_GetLanguage();
		parse_banner_title(banner, title, lang);
		// if title is empty revert to english
		char z2[2] = {0,0}; // utf16: 2 bytes
		if (getTitle == 1 && memcmp(title, z2, 2) == 0) {
			parse_banner_title(banner, title, CONF_LANG_ENGLISH);
			// if still empty, find first valid.
			if (memcmp(title, z2, 2) == 0) {
				for (lang=0; lang<10; lang++) {
					parse_banner_title(banner, title, lang);
					if (memcmp(title, z2, 2) != 0) break;
				}
			}
			if (memcmp(title, z2, 2) == 0) {
				// final check, if still empty, use english
				// in case there is some text after the zeroes (unlikely)
				parse_banner_title(banner, title, CONF_LANG_ENGLISH);
			}
		}

	}
	if (getSound) parse_banner_snd(banner, snd);
	SAFE_FREE(banner);
	return 0;
}

#ifdef FAKE_GAME_LIST

#include <malloc.h>
#include "wbfs.h"

// initial/max game list size
int fake_games = 200;
//int fake_games = 0;

// current game list size
int fake_num = 0;

struct discHdr *fake_list = NULL;

int is_fake(char *id)
{
	int i;
	for (i=0; i<fake_num; i++) {
		// ignore region, check only first 5
		if (strncmp((char*)fake_list[i].id, (char*)id, 5) == 0) return 1;
	}
	return 0;
}

#if 0
// WBFS_GetCount
s32 dbg_WBFS_GetCount(u32 *count)
{
	DIR *dir;
	struct dirent *dent;
	char id[8], *p;
	int ret, cnt, len;

	fake_num = 0;
	SAFE_FREE(fake_list);
	if (fake_games < 0) fake_games = 0;

	// first get the real list, then add fake entries
	ret = WBFS_GetCount((u32*)&cnt);
	if (ret >= 0) {
		len = sizeof(struct discHdr) * cnt;
		fake_list = (struct discHdr *)memalign(32, len);
		if (!fake_list) return -1;
		memset(fake_list, 0, len);
		ret = WBFS_GetHeaders(fake_list, cnt, sizeof(struct discHdr));
		if (ret >= 0) fake_num = cnt;
		//printf("real games num: %d\n", fake_num); sleep(2);
	}
	if (fake_num > fake_games) fake_num = fake_games;

	//printf("fake dir: %s\n", CFG.covers_path); sleep(1);
	dir = opendir(CFG.covers_path);
	if (!dir) {
		printf("fake dir error! %s\n", CFG.covers_path); sleep(2);
		return 0;
	}

	while ((dent = readdir(dir)) != NULL) {
		if (fake_num >= fake_games) break;
		if (dent->d_name[0] == '.') continue;
		if (strstr(dent->d_name, ".png") == NULL
			&& strstr(dent->d_name, ".PNG") == NULL) continue;
		memset(id, 0, sizeof(id));
		STRCOPY(id, dent->d_name);
		p = strchr(id, '.');
		if (p == NULL) continue;
		*p = 0;
		// check if already exists, do not ignore region, we want more games ;)
		if (is_fake(id)) continue;
		fake_list = realloc(fake_list, sizeof(struct discHdr) * (fake_num+1));
		memset(fake_list+fake_num, 0, sizeof(struct discHdr));
		memcpy(fake_list[fake_num].id, id, sizeof(fake_list[fake_num].id));
		STRCOPY(fake_list[fake_num].title, dent->d_name);
		//printf("fake %d %.6s %s\n", fake_num,
		//		fake_list[fake_num].id, fake_list[fake_num].title);
		fake_num++;

	}
	closedir(dir);
	//sleep(2);
	*count = fake_num;
	return 0;
}
#else
// WBFS_GetCount

void add_fake(char *name, char *val)
{
	char id[8];
	if (fake_num >= fake_games) return;
	// is ID?
	if (strlen(name) != 6) return;
	memset(id, 0, sizeof(id));
	STRCOPY(id, name);
	// check if already exists, do not ignore region, we want more games
	if (is_fake(id)) return;
	fake_list = realloc(fake_list, sizeof(struct discHdr) * (fake_num+1));
	memset(fake_list+fake_num, 0, sizeof(struct discHdr));
	memcpy(fake_list[fake_num].id, id, sizeof(fake_list[fake_num].id));
	STRCOPY(fake_list[fake_num].title, val);
	//printf("fake %d %.6s %s\n", fake_num,
	//		fake_list[fake_num].id, fake_list[fake_num].title);
	fake_num++;
}

s32 dbg_WBFS_GetCount(u32 *count)
{
	char fname[100];

	fake_num = 0;
	SAFE_FREE(fake_list);
	if (fake_games < 0) fake_games = 0;
	sprintf(fname, "%s/%s", USBLOADER_PATH, "gamelist2.txt");
	printf("fake list: %s\n", fname); sleep(1);
	cfg_parsefile(fname, add_fake);
	//add_fake("RHAP01", "wii play");
	*count = fake_num;
	return 0;
}
#endif

// WBFS_GetHeaders
s32 dbg_WBFS_GetHeaders(void *outbuf, u32 cnt, u32 len)
{
	memcpy(outbuf, fake_list, cnt*len);
	return 0;
}

#endif

