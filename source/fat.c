
// Modified by oggzee

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ogcsys.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sdcard/wiisd_io.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <sys/statvfs.h>

#include <fat.h>
#include "ntfs.h"
#include "ext2.h"

#include "sdhc.h"
#include "usbstorage.h"
#include "wbfs.h"
#include "util.h"
#include "wpad.h"
#include "menu.h"
#include "gettext.h"
#include "cfg.h"
#include "sys.h"
#include "partition.h"
#include "fat.h"
#include "video.h"

#define GB_SIZE         1073741824.0

extern void ntfsInit();
void fat_Unmount(const char* name);
void _FAT_mem_init();

/* Constants */
#define FAT_CACHE 32
#define FAT_SECTORS 64
#define FAT_SECTORS_SD 32

extern sec_t _FAT_startSector;

extern s32 wbfsDev;

MountTable mtab = { 0 };

s32 Fat_ReadFile(const char *filepath, void **outbuf)
{
	FILE *fp     = NULL;
	void *buffer = NULL;

	struct stat filestat;
	u32         filelen;

	s32 ret;

	/* Get filestats */
	ret = stat(filepath, &filestat);
	if (ret != 0) {
		//printf("File not found %s %d\n", filepath, (int)filestat.st_size); sleep(3);
		return -1;
	}

	/* Get filesize */
	filelen = filestat.st_size;

	/* Allocate memory */
	buffer = memalign(32, filelen);
	if (!buffer)
		goto err;

	/* Open file */
	fp = fopen(filepath, "rb");
	if (!fp)
		goto err;

	/* Read file */
	ret = fread(buffer, 1, filelen, fp);
	if (ret != filelen)
		goto err;

	/* Set pointer */
	*outbuf = buffer;

	goto out;

err:
	/* Free memory */
	if (buffer)
		free(buffer);

	/* Error code */
	ret = -1;

out:
	/* Close file */
	if (fp)
		fclose(fp);

	return ret;
}


// fat cache alloc

#if 1

static void *fat_pool = NULL;
static size_t fat_size;
#define FAT_SLOTS (FAT_CACHE * MOUNT_MAX)
#define FAT_SLOT_SIZE (512 * FAT_SECTORS)
#define FAT_SLOT_SIZE_MIN (512 * FAT_SECTORS_SD)
static int fat_alloc[FAT_SLOTS];

void _FAT_mem_init()
{
	if (fat_pool) return;
	fat_size = FAT_SLOTS * FAT_SLOT_SIZE;
	fat_pool = LARGE_memalign(32, fat_size);
}

void* _FAT_mem_allocate(size_t size)
{
	return malloc(size);
}

void* _FAT_mem_align(size_t size)
{
	//dbg_printf("FAT_mem %d\n", size);
	if (size < FAT_SLOT_SIZE_MIN || size > FAT_SLOT_SIZE) goto fallback;
	if (fat_pool == NULL) goto fallback;
	int i;
	for (i=0; i<FAT_SLOTS; i++) {
		if (fat_alloc[i] == 0) {
			void *ptr = fat_pool + i * FAT_SLOT_SIZE;
			fat_alloc[i] = 1;
			return ptr;
		}	
	}
	fallback:
	dbg_printf("FAT memalign %d\n", size);
	return memalign (32, size);		
}

void _FAT_mem_free(void *mem)
{
	if (fat_pool == NULL || mem < fat_pool || mem >= fat_pool + fat_size) {
		free(mem);
		return;
	}
	int i;
	for (i=0; i<FAT_SLOTS; i++) {
		if (fat_alloc[i]) {
			void *ptr = fat_pool + i * FAT_SLOT_SIZE;
			if (mem == ptr) {
				fat_alloc[i] = 0;
				return;
			}
		}	
	}
	// FATAL
	printf("\n");
	printf(gt("FAT: ALLOC ERROR %p %p %p"), mem, fat_pool, fat_pool + fat_size);
	printf("\n");
	sleep(5);
	exit(1);
}

#else

void _FAT_mem_init()
{
}

void* _FAT_mem_allocate (size_t size) {
	return malloc (size);
}

void* _FAT_mem_align (size_t size) {
	return memalign (32, size);
}

void _FAT_mem_free (void* mem) {
	free (mem);
}

#endif


void* ntfs_alloc (size_t size)
{
	return _FAT_mem_allocate(size);
}

void* ntfs_align (size_t size)
{
	return _FAT_mem_align(size);
}

void ntfs_free (void* mem)
{
	_FAT_mem_free(mem);
}


void* _EXT2_cache_mem_align (size_t a, size_t size)
{
	return _FAT_mem_align(size);
}

void _EXT2_cache_mem_free (void* mem)
{
	_FAT_mem_free(mem);
}

// fix for newlib/libfat unmount without :

void fat_Unmount (const char* name)
{
	char name2[16];
	strcpy(name2, name);
	strcat(name2, ":");
	fatUnmount(name2);
}


int mount_add(char *name, int device, sec_t sector, int fstype, int flags)
{
	dbg_printf("mount_add(%s,%d,%u,%d)\n", name, device, sector, fstype);
	if (mtab.num >= MOUNT_MAX) return -1;
	MountPoint *m = &mtab.point[mtab.num];
	STRCOPY(m->name, name);
	m->device = device;
	m->sector = sector;
	m->fstype = fstype;
	m->flags  = flags;
	mtab.num++;
	return 0;
}

MountPoint* mount_find_part(int device, sec_t sector)
{
	int i;
	MountPoint *m;
	//dbg_printf("m_find(%d,%u)", device, sector);
	for (i=0; i<mtab.num; i++)
	{
		m = &mtab.point[i];
		if (m->device == device && m->sector == sector) {
			//dbg_printf("=%s:\n", m->name);
			return m;
		}
	}
	//dbg_printf("=0\n");
	return NULL;
}

// remove trailing ':' if present
void mount_drive2name(char *drive, char *name)
{
	int i;
	strcpy(name, drive);
	i = strlen(name) - 1;
	if (i < 0) return;
	if (name[i] == ':') name[i] = 0;
}

// append trailing ':' if not present
void mount_name2drive(char *name, char *drive)
{
	int i;
	strcpy(drive, name);
	i = strlen(drive) - 1;
	if (i < 0) return;
	if (drive[i] != ':') strcat(drive, ":");
}

// name can be "usb" or "usb:"
MountPoint* mount_find(char *name)
{
	int i;
	MountPoint *m;
	char m_name[16];

	mount_drive2name(name, m_name);
	for (i=0; i<mtab.num; i++)
	{
		m = &mtab.point[i];
		if (strcmp(m->name, m_name) == 0) return m;
	}
	return NULL;
}

int mount_del(char *name)
{
	dbg_printf("mount_del(%s)\n", name);
	MountPoint *m = mount_find(name);
	if (!m) return -1;
	int i = m - mtab.point;
	memset(m, 0, sizeof(*m));
	memmove(m, m+1, (mtab.num-i-1)*sizeof(*m));
	mtab.num--;
	return 0;
}


int IO_InitSDHC(int verbose)
{
	int ret;
	int retval = 0;
	if (sdhc_inited) return retval;
	get_time(&TIME.sd_init1);
	//sdhc_mode_sd = 0;
	if ( is_ios_type(IOS_TYPE_WANIN) && (IOS_GetRevision() == 18) ) {
		// sdhc device is broken on ios 249 rev 18
		sdhc_mode_sd = 1;
	}
	// Initialize SD/SDHC interface
	retry:
	//dbg_printf("SD startup\n");
	ret = my_io_sdhc.startup();
	if (!ret) {
		if (!sdhc_mode_sd) {
			dbg_printf("ERROR: SDHC init! (%d, %d)\n", ret, errno);
			sdhc_mode_sd = 1;
			goto retry;
		}
		if (verbose) {
			printf_x("ERROR: SDHC init! (%d, %d)\n", ret, errno);
		}
		retval = -5;
	}
	get_time(&TIME.sd_init2);
	return retval;
}

int IO_CloseSDHC()
{
	bool ret = true;
	/* Shutdown SDHC interface */
	if (sdhc_mode_sd == 0) {
		// don't shutdown sdhc if we're booting from it
		if (wbfsDev != WBFS_DEVICE_SDHC) {
			//ret = my_io_sdhc.shutdown(); // this is NOP
			ret = SDHC_Close();
		}
	} else {
		ret = __io_wiisd.shutdown();
	}
	if (!ret) return -1;
	return 0;
}

int IO_InitUSB(int verbose)
{
	int ret;
	// Initialize USB interface
	dbg_printf("USB startup\n");
	ret = my_io_usbstorage.startup();
	if (!ret) {
		if (verbose) {
			printf_x(gt("ERROR: USB init! (%d)"), ret);
			printf("\n");
			sleep(1);
		}
		return -4;
	}
	return 0;
}


int MountFS(char *aname, int device, sec_t sector, int fstype, int verbose)
{
	s32 ret = 0;
	const DISC_INTERFACE *io;
	int page_count = FAT_CACHE;
	int page_size = FAT_SECTORS;
	MountPoint *m;
	int flags = 0;
	char name[16];

	dbg_printf("Mount %s %d %d %d\n", aname, device, sector, fstype);
	mount_drive2name(aname, name);

	_FAT_mem_init();

	m = mount_find_part(device, sector);
	if (m) {
		printf("ERROR: '%s' (%d,%d) already mounted as '%s'", name, device, sector, m->name);
		sleep(2);
		return -1;
	}
	m = mount_find(name);
	if (m) {
		printf("ERROR: '%s' (%d,%d) already mounted as '%s' (%d,%d)",
				name, device, sector, m->name, m->device, m->sector);
		sleep(2);
		return -2;
	}
	if (mtab.num >= MOUNT_MAX) {
		printf("ERROR: too many mounts! (%d)", mtab.num);
		sleep(2);
		return -3;
	}

	if (device == WBFS_DEVICE_USB) {

		ret = IO_InitUSB(verbose);
		if (ret) return ret;
		io = &my_io_usbstorage;
		u32 sec_size;
		USBStorage_GetCapacity(&sec_size);
		if (sec_size > 512) {
			page_size /= 8; // 8 = 4096/512
		}

	} else if (device == WBFS_DEVICE_SDHC) {

		ret = IO_InitSDHC(verbose);
		if (ret) return ret;
		if (sdhc_mode_sd == 1) {
			page_size = FAT_SECTORS_SD;
		}
		io = &my_io_sdhc;

	} else {
		printf("ERROR: Invalid device %d\n", device);
		return -6;
	}

	if (fstype == PART_FS_FAT) {

		// FAT MOUNT
		dbg_printf("fatMount(%s,%u)", name, sector);
		ret = fatMount(name, io, sector, page_count, page_size);
		dbg_printf(" = %d %d\n", ret, _FAT_startSector);

		if (ret) {
			if (sector == 0) {
				sector = _FAT_startSector;
				dbg_printf("FAT sector %u", sector);
			} else if (sector != _FAT_startSector) {
				printf("ERROR: FAT mount sector %x not %x", sector, _FAT_startSector);
				fat_Unmount(name);
				sleep(5);
				return -10;
			}
		}

	} else if (fstype == PART_FS_NTFS) {

		ntfsInit();
		// ntfsInit resets locale settings
		// which breaks unicode in console
		// so we change it back to C-UTF-8
		setlocale(LC_CTYPE, "C-UTF-8");
		setlocale(LC_MESSAGES, "C-UTF-8");

		switch (CFG.ntfs_write)
		{
			default:
			case 0:
				flags = NTFS_DEFAULT | NTFS_READ_ONLY;
				if (device == WBFS_DEVICE_USB) {
					io = &my_io_usbstorage_ro;
				} else if (device == WBFS_DEVICE_SDHC) {
					io = &my_io_sdhc_ro;
				}
				break;
			case 1: flags = NTFS_DEFAULT | NTFS_RECOVER; break;
			case 2: flags = NTFS_DEFAULT; break;
		}

		if (sector == 0) {
			sec_t *plist;
			int n = ntfsFindPartitions(io, &plist);
			if (n > 0 && plist) {
				sector = plist[0];
				SAFE_FREE(plist);
			} else {
				dbg_printf("No NTFS partition found!\n");
				return -20;
			}
		}

		// NTFS MOUNT
		dbg_printf("ntfsMount(%s,%u)", name, sector);
		ret = ntfsMount(name, io, sector, page_count, page_size, flags);
		dbg_printf(" = %d\n", ret);

		if (!ret) {
			if (errno == EDIRTY) {
				printf_(gt("ERROR: ntfs was not cleanly unmounted"));
				sleep(5);
				return -21;
			}
		}

	} else if (fstype == PART_FS_EXT) {

		// EXT MOUNT
		dbg_printf("ext2Mount(%s,%u)", name, sector);
		// readonly = 0; write = EXT2_FLAG_RW
		ret = ext2Mount(name, io, sector, page_count, page_size,
				(0 | EXT2_FLAG_64BITS | EXT2_FLAG_JOURNAL_DEV_OK));
		dbg_printf(" = %d\n", ret);

	} else {
		printf("Invalid fs type %d\n", fstype);
		return -40;
	}

	if (!ret) {
		// mount failed
		dbg_printf("ERROR: mount %s/%s (%d)",
				device == WBFS_DEVICE_USB ? "USB" : "SDHC",
				fstype == PART_FS_FAT ? "FAT" : "NTFS",
				errno);
		if (verbose) {
			printf("ERROR: mount %s/%s (%d)\n",
					device == WBFS_DEVICE_USB ? "USB" : "SDHC",
					fstype == PART_FS_FAT ? "FAT" : "NTFS",
					errno);
			sleep(2);
		}
		return -50;
	}

	// OK

	mount_add(name, device, sector, fstype, flags);

	return 0;
}

int UnmountFS(char *aname)
{
	MountPoint *m;
	char name[16];
	char drive[16];

	dbg_printf("Unmount %s\n", aname);

	mount_drive2name(aname, name);
	mount_name2drive(name, drive);

	m = mount_find(name);
	if (!m) return -1;

	if (m->fstype == PART_FS_FAT) {
		fat_Unmount(name);
	} else if (m->fstype == PART_FS_NTFS) {
		ntfsUnmount(name, true);
	} else if (m->fstype == PART_FS_EXT) {
		ext2Unmount(drive);
	} else {
		return -2;
	}

	mount_del(name);

	return 0;
}

int MountAll(MountTable *mt)
{
	int i;
	int r, ret = 0;
	for (i=0; i<mt->num; i++) {
		r = MountFS(mt->point[i].name, mt->point[i].device, mt->point[i].sector, mt->point[i].fstype, 0);
		if (r) ret = r;
	}
	return ret;
}

int UnmountAll(MountTable *save_mtab)
{
	int i;
	MountTable mt;
	memcpy(&mt, &mtab, sizeof(mtab));
	if (save_mtab) {
		memcpy(save_mtab, &mtab, sizeof(mtab));
	}
	for (i=0; i<mt.num; i++) {
		UnmountFS(mt.point[i].name);
	}
	IO_CloseSDHC();
	return 0;
}

// mount first FAT -OR- first NTFS on sd:
int MountSDHC()
{
	int ret;
	int retval = 0; // OK
	sec_t sector;

	// already mounted?
	if (mount_find(SDHC_MOUNT)) return 0;
	
	// init
	ret = IO_InitSDHC(0);
	if (ret) return ret;

	get_time(&TIME.sd_mount1);
	// find first FAT partition
	sector = -1;
	ret = Partition_FindFS(WBFS_DEVICE_SDHC, PART_FS_FAT, 1, &sector);
	if (ret == 0) {
		// fat found.
		ret = MountFS(SDHC_MOUNT, WBFS_DEVICE_SDHC, sector, PART_FS_FAT, 0);
		if (ret == 0) goto out;
	}
	// find first NTFS partition
	sector = -1;
	ret = Partition_FindFS(WBFS_DEVICE_SDHC, PART_FS_NTFS, 1, &sector);
	if (ret == 0) {
		// fat found.
		ret = MountFS(SDHC_MOUNT, WBFS_DEVICE_SDHC, sector, PART_FS_NTFS, 0);
		if (ret == 0) goto out;
	}
	retval = -1; // ERR
out:
	get_time(&TIME.sd_mount2);
	return retval;
}

// mount first FAT to usb: -AND- first NTFS on ntfs:
int MountUSB()
{
	int ret;
	int retval = -1;
	sec_t sector;

	ret = IO_InitUSB(0);
	if (ret) return ret;

	get_time(&TIME.usb_mount1);
	// find first FAT partition
	sector = -1;
	if (mount_find(USB_MOUNT)) {
		// already mounted
		retval = 0;
	} else {
		ret = Partition_FindFS(WBFS_DEVICE_USB, PART_FS_FAT, 1, &sector);
		if (ret == 0) {
			// fat found.
			ret = MountFS(USB_MOUNT, WBFS_DEVICE_USB, sector, PART_FS_FAT, 0);
			if (ret == 0) retval = 0; // OK
		}
	}

	// find first NTFS partition
	sector = -1;
	if (mount_find(NTFS_MOUNT)) {
		// already mounted
		retval = 0;
	} else {
		ret = Partition_FindFS(WBFS_DEVICE_USB, PART_FS_NTFS, 1, &sector);
		if (ret == 0) {
			// ntfs found.
			ret = MountFS(NTFS_MOUNT, WBFS_DEVICE_USB, sector, PART_FS_NTFS, 0);
			if (ret == 0) retval = 0; // OK
		}
	}

	// find first EXT partition
	sector = -1;
	if (mount_find(EXT_MOUNT)) {
		// already mounted
		retval = 0;
	} else {
		ret = Partition_FindFS(WBFS_DEVICE_USB, PART_FS_EXT, 1, &sector);
		if (ret == 0) {
			// ext found.
			ret = MountFS(EXT_MOUNT, WBFS_DEVICE_USB, sector, PART_FS_EXT, 0);
			if (ret == 0) retval = 0; // OK
		}
	}

	get_time(&TIME.usb_mount2);
	return retval;
}

// remount NTFS with write permission if enabled
void RemountNTFS()
{
	int i;
	MountTable mt;
	MountPoint *mp;
	if (!CFG.ntfs_write) return;
	memcpy(&mt, &mtab, sizeof(mtab));
	for (i=0; i<mt.num; i++) {
		mp = &mt.point[i];
		if ((mp->fstype == PART_FS_NTFS) && (mp->flags & NTFS_READ_ONLY)) {
			UnmountFS(mp->name);
			dbg_printf("remounting %s\n", mp->name);
			MountFS(mp->name, mp->device, mp->sector, mp->fstype, 1);
		}
	}
}

// print:
// FS: [ sd: usb: ntfs: game: ]
void MountPrint_str(char *str, int size)
{
	int i;
	snprintf(str, size, "FS: [ ");
	str_seek_end(&str, &size);
	for (i=0; i<mtab.num; i++)
	{
		snprintf(str, size, "%s: ", mtab.point[i].name);
		str_seek_end(&str, &size);
	}
	snprintf(str, size, "]\n");
}

void MountPrint()
{
	char str[100];
	MountPrint_str(str, sizeof(str));
	printf_("%s", str);
}

s32 FAT_DiskSpace(f32 *used, f32 *free)
{
	f32 size;
	int ret;
	struct statvfs vfs;

	*used = 0;
	*free = 0;
	
	extern void printf_(const char *fmt, ...);
	printf("\r");
	printf_(gt("calculating space, please wait..."));
	printf("\r");
	ret = statvfs("usb:", &vfs);
	Con_ClearLine();
	if (ret) return 0;

	/* FS size in GB */
	size = (f32)vfs.f_frsize * (f32)vfs.f_blocks / GB_SIZE;
	*free = (f32)vfs.f_frsize * (f32)vfs.f_bfree / GB_SIZE;
	*used = size - *free;

	return 0;
}