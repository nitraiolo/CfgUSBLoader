#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ogcsys.h>
#include <sdcard/wiisd_io.h>
#include <sys/statvfs.h>

#include "util.h"
#include "gettext.h"
#include "sdhc.h"
#include "debug.h"
#include "video.h"

/* IOCTL comamnds */
#define IOCTL_SDHC_INIT		0x01
#define IOCTL_SDHC_READ		0x02
#define IOCTL_SDHC_WRITE	0x03
#define IOCTL_SDHC_ISINSERTED	0x04

#define SDHC_HEAPSIZE		0x8000
#define SDHC_MEM2_SIZE		0x10000
#define GB_SIZE         1073741824.0
// 0x10000 = 64 KB = 128 x 512 sector

int sdhc_mode_sd = 0;
int sdhc_inited = 0;

/* Variables */
static char fs[] ATTRIBUTE_ALIGN(32) = "/dev/sdio/sdhc";

static s32 hid = -1, fd = -1;
static u32 sector_size = SDHC_SECTOR_SIZE;
static void *sdhc_buf2;

extern void* SYS_AllocArena2MemLo(u32 size,u32 align);

bool SDHC_Init(void)
{
	s32 ret;

	if (sdhc_inited) return true;

	if (sdhc_mode_sd) {
		sdhc_inited = __io_wiisd.startup();
		return sdhc_inited;
	}

	/* Already open */
	if (fd >= 0)
		return true;

	/* Create heap */
	if (hid < 0) {
		hid = iosCreateHeap(SDHC_HEAPSIZE);
		if (hid < 0)
			goto err;
	}

	// allocate buf2
	if (sdhc_buf2 == NULL) {
		sdhc_buf2 = SYS_AllocArena2MemLo(SDHC_MEM2_SIZE, 32);
		if (sdhc_buf2 == NULL) {
			printf("ERR: sdhc mem2\n");
			sleep(3);
			goto err;
		}
	}

	/* Open SDHC device */
	fd = IOS_Open(fs, 0);
	dbg_printf("open(%s)=%d\n", fs, fd);
	if (fd < 0)
		goto err;

	/* Initialize SDHC */
	ret = IOS_IoctlvFormat(hid, fd, IOCTL_SDHC_INIT, ":");
	if (ret)
		goto err;

	sdhc_inited = 1;
	return true;

err:
	/* Close SDHC device */
	if (fd >= 0) {
		IOS_Close(fd);
		fd = -1;
	}

	return false;
}

bool SDHC_Close(void)
{
	sdhc_inited = 0;
	if (sdhc_mode_sd) {
		return __io_wiisd.shutdown();
	}

	/* Close SDHC device */
	if (fd >= 0) {
		IOS_Close(fd);
		fd = -1;
	}

	/*if (hid > 0) {
		iosDestroyHeap(hid);
		hid = -1;
	}*/

	return true;
}

bool SDHC_IsInserted(void)
{
	s32 ret;
	if (sdhc_mode_sd) {
		return __io_wiisd.isInserted();
	}

	/* Check if SD card is inserted */
	ret = IOS_IoctlvFormat(hid, fd, IOCTL_SDHC_ISINSERTED, ":");

	return (!ret) ? true : false;
}

bool SDHC_ReadSectors(u32 sector, u32 count, void *buffer)
{
	//printf("SD-R(%u %u)\n", sector, count);
	if (sdhc_mode_sd) {
		return __io_wiisd.readSectors(sector, count, buffer);
	}

	u32 size;
	s32 ret = -1;

	/* Device not opened */
	if (fd < 0)
		return false;

	/* Buffer not aligned */
	if ((u32)buffer & 0x1F) {
		if (!sdhc_buf2) return false;
		int cnt;
		int max_sec = SDHC_MEM2_SIZE / sector_size;
		//dbg_printf("sdhc_read(%u,%u) unaligned(%p)\n", sector, count, buffer);
		while (count) {
			if (count > max_sec) cnt = max_sec; else cnt = count;
			size = cnt * sector_size;
			ret = IOS_IoctlvFormat(hid, fd, IOCTL_SDHC_READ,
					"ii:d", sector, cnt, sdhc_buf2, size);
			//dbg_printf("sdhc_read_chunk(%u,%u)=%d\n", sector, cnt, ret);
			if (ret) return false;
			memcpy(buffer, sdhc_buf2, size);
			count -= cnt;
			sector += cnt;
			buffer += size;
		}
	} else {
		size = sector_size * count;
		/* Read data */
		ret = IOS_IoctlvFormat(hid, fd, IOCTL_SDHC_READ,
				"ii:d", sector, count, buffer, size);
	}

	return (!ret) ? true : false;
}

bool SDHC_WriteSectors(u32 sector, u32 count, void *buffer)
{
	if (sdhc_mode_sd) {
		return __io_wiisd.writeSectors(sector, count, buffer);
	}

	u32 size;
	s32 ret = -1;

	/* Device not opened */
	if (fd < 0)
		return false;

	/* Buffer not aligned */
	if ((u32)buffer & 0x1F) {
		if (!sdhc_buf2) return false;
		int cnt;
		int max_sec = SDHC_MEM2_SIZE / sector_size;
		//dbg_printf("sdhc_write(%u,%u) unaligned(%p)\n", sector, count, buffer);
		while (count) {
			if (count > max_sec) cnt = max_sec; else cnt = count;
			size = cnt * sector_size;
			memcpy(sdhc_buf2, buffer, size);
			ret = IOS_IoctlvFormat(hid, fd, IOCTL_SDHC_WRITE,
					"ii:d", sector, cnt, sdhc_buf2, size);
			//dbg_printf("sdhc_write_chunk(%u,%u)=%d\n", sector, cnt, ret);
			if (ret) return false;
			count -= cnt;
			sector += cnt;
			buffer += size;
		}
	} else {
		size = sector_size * count;
		/* Read data */
		ret = IOS_IoctlvFormat(hid, fd, IOCTL_SDHC_WRITE,
				"ii:d", sector, count, buffer, size);
	}

	return (!ret) ? true : false;
}

bool SDHC_ClearStatus(void)
{
	return true;
}

bool __io_SDHC_Close(void)
{
	// do nothing.
	return true;
}

bool __io_SDHC_NOP(void)
{
	// do nothing.
	return true;
}

const DISC_INTERFACE my_io_sdhc = {
	DEVICE_TYPE_WII_SD,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_WII_SD,
	(FN_MEDIUM_STARTUP)&SDHC_Init,
	(FN_MEDIUM_ISINSERTED)&SDHC_IsInserted,
	(FN_MEDIUM_READSECTORS)&SDHC_ReadSectors,
	(FN_MEDIUM_WRITESECTORS)&SDHC_WriteSectors,
	(FN_MEDIUM_CLEARSTATUS)&SDHC_ClearStatus,
	//(FN_MEDIUM_SHUTDOWN)&SDHC_Close
	(FN_MEDIUM_SHUTDOWN)&__io_SDHC_Close
};

const DISC_INTERFACE my_io_sdhc_ro = {
	DEVICE_TYPE_WII_SD,
	FEATURE_MEDIUM_CANREAD | FEATURE_WII_SD,
	(FN_MEDIUM_STARTUP)      &SDHC_Init,
	(FN_MEDIUM_ISINSERTED)   &SDHC_IsInserted,
	(FN_MEDIUM_READSECTORS)  &SDHC_ReadSectors,
	(FN_MEDIUM_WRITESECTORS) &__io_SDHC_NOP, // &SDHC_WriteSectors,
	(FN_MEDIUM_CLEARSTATUS)  &SDHC_ClearStatus,
	//(FN_MEDIUM_SHUTDOWN)&SDHC_Close
	(FN_MEDIUM_SHUTDOWN)     &__io_SDHC_Close
};

s32 SD_DiskSpace(f32 *used, f32 *free)
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
	ret = statvfs("sd:", &vfs);
	Con_ClearLine();
	if (ret) return 0;

	/* FS size in GB */
	size = (f32)vfs.f_frsize * (f32)vfs.f_blocks / GB_SIZE;
	*free = (f32)vfs.f_frsize * (f32)vfs.f_bfree / GB_SIZE;
	*used = size - *free;

	return 0;
}
