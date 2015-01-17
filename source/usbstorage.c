/*-------------------------------------------------------------

usbstorage_starlet.c -- USB mass storage support, inside starlet
Copyright (C) 2009 Kwiirk

If this driver is linked before libogc, this will replace the original 
usbstorage driver by svpe from libogc
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/

#include <unistd.h>
#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "debug.h"

/* IOCTL commands */
#define UMS_BASE			(('U'<<24)|('M'<<16)|('S'<<8))
#define USB_IOCTL_UMS_INIT	        (UMS_BASE+0x1)
#define USB_IOCTL_UMS_GET_CAPACITY      (UMS_BASE+0x2)
#define USB_IOCTL_UMS_READ_SECTORS      (UMS_BASE+0x3)
#define USB_IOCTL_UMS_WRITE_SECTORS	(UMS_BASE+0x4)
#define USB_IOCTL_UMS_READ_STRESS	(UMS_BASE+0x5)
#define USB_IOCTL_UMS_SET_VERBOSE	(UMS_BASE+0x6)

#define WBFS_BASE (('W'<<24)|('F'<<16)|('S'<<8))
#define USB_IOCTL_WBFS_OPEN_DISC	        (WBFS_BASE+0x1)
#define USB_IOCTL_WBFS_READ_DISC	        (WBFS_BASE+0x2)

#define USB_IOCTL_WBFS_READ_DEBUG	        (WBFS_BASE+0x13)
#define USB_IOCTL_WBFS_SET_DEVICE	        (WBFS_BASE+0x14)
#define USB_IOCTL_WBFS_SET_FRAGLIST         (WBFS_BASE+0x15)

#define UMS_HEAPSIZE			0x8000
#define USB_MEM2_SIZE           0x10000
// 0x10000 = 64 KB = 128 x 512 sector = 16 x 4k sector

/* Variables */
static char fs[] ATTRIBUTE_ALIGN(32) = "/dev/usb2";
static char fs2[] ATTRIBUTE_ALIGN(32) = "/dev/usb123";
static char fs3[] ATTRIBUTE_ALIGN(32) = "/dev/usb/ehc";
 
static s32 hid = -1, fd = -1;
static u32 sector_size;
static void *usb_buf2;
static mutex_t usb_mutex = LWP_MUTEX_NULL;

extern void* SYS_AllocArena2MemLo(u32 size,u32 align);

static inline s32 __USBStorage_isMEM2Buffer(const void *buffer)
{
	// MEM1: 0x80000000 (cached) 0xC0000000 (uncached)
	// MEM2: 0x90000000 (cached) 0xD0000000 (uncached)
	return ((u32)buffer & 0x10000000) != 0;

	// Why is this important? MEM1 seems to work just fine
	// so let's skip this check
	//return true;
}


u32 USBStorage_GetCapacity(u32 *_sector_size)
{
	if (fd >= 0) {
		s32 ret;

		ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_GET_CAPACITY, ":i", &sector_size);

		/*
		static int first = 1;
		if (first) {
			printf("\nSECTORS: %u\n", ret);
			printf("SEC SIZE: %u\n", sector_size);
			printf("HDD SIZE: %u GB [%u]\n", ret/1024/1024*sector_size/1024, sector_size);
			Menu_PrintWait();
			first = 0;
		}
		*/
		//dbg_printf("capacity %d %d\n", sector_size, ret);

		if (ret && _sector_size)
			*_sector_size = sector_size;

		return ret;
	}

	return 0;
}

s32 USBStorage_OpenDev()
{
	/* Already open */
	if (fd >= 0)
		return fd;

	/* Create heap */
	if (hid < 0) {
		hid = iosCreateHeap(UMS_HEAPSIZE);
		if (hid < 0)
			return IPC_ENOMEM;  // = -22
	}

	// allocate buf2
	if (usb_buf2 == NULL) {
		usb_buf2 = SYS_AllocArena2MemLo(USB_MEM2_SIZE, 32);
		if (usb_buf2 == NULL) {
			printf("ERR: usb mem2\n");
			sleep(3);
			return IPC_ENOMEM;  // = -22
		}
	}

	/* Open USB device */
	fd = IOS_Open(fs, 0);
	dbg_printf("open(%s)=%d", fs, fd);
	if (fd < 0) {
		dbg_printf("\n");
		fd = IOS_Open(fs2, 0);
		dbg_printf("open(%s)=%d", fs2, fd);
	}
	if (fd < 0) {
		dbg_printf("\n");
		fd = IOS_Open(fs3, 0);
		dbg_printf("open(%s)=%d", fs3, fd);
	}
	if (fd < 0) {
		dbg_printf("\n");
	}
	LWP_MutexInit(&usb_mutex, false);
	return fd;
}

s32 USBStorage_Init(void)
{
	s32 ret;
	u32 cap;
	u32 sect_size;
	get_time(&TIME.usb_init1);
	USBStorage_OpenDev();
	get_time(&TIME.usb_open);
	if (fd < 0)
		return fd;

	/* Initialize USB storage */
	ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_INIT, ":");
	dbg_printf(" init:%d", ret);
	get_time(&TIME.usb_cap);

	/* Get device capacity */
	cap = USBStorage_GetCapacity(&sect_size);
	dbg_printf(" cap:%u ss:%u\n", cap, sect_size);
	get_time(&TIME.usb_init2);
	if (cap < 10000)
		goto err;

	return 0;

err:
	/* Close USB device */
	if (fd >= 0) {
		IOS_Close(fd);
		fd = -1;
	}

	return -1;
}

void USBStorage_Deinit(void)
{
	/* Close USB device */
	if (fd >= 0) {
		IOS_Close(fd);
		fd = -1;
	}
	/*if (hid > 0) {
		iosDestroyHeap(hid);
		hid = -1;
	}*/
	LWP_MutexDestroy(usb_mutex);
	usb_mutex = LWP_MUTEX_NULL;
}

s32 USBStorage_ReadSectors(u32 sector, u32 numSectors, void *buffer)
{
	u32 size;
	s32 ret = -1;

	/* Device not opened */
	if (fd < 0)
		return fd;

	/* check align and MEM1 buffer */
	if (((u32)buffer & 0x1F) || (!__USBStorage_isMEM2Buffer(buffer))) {
		if (!usb_buf2) return IPC_ENOMEM;
		int cnt;
		int max_sec = USB_MEM2_SIZE / sector_size;
		//dbg_printf("usb_read(%u,%u) unaligned(%p)\n", sector, numSectors, buffer);
		while (numSectors) {
			if (numSectors > max_sec) cnt = max_sec; else cnt = numSectors;
			size = cnt * sector_size;
			LWP_MutexLock(usb_mutex);
			ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_READ_SECTORS,
					"ii:d", sector, cnt, usb_buf2, size);
			memcpy(buffer, usb_buf2, size);
			LWP_MutexUnlock(usb_mutex);
			//dbg_printf("usb_read_chunk(%u,%u)=%d\n", sector, cnt, ret);
			if (ret < 0) return ret;
			numSectors -= cnt;
			sector += cnt;
			buffer += size;
		}
	} else {
		size = sector_size * numSectors;
		/* Read data */
		LWP_MutexLock(usb_mutex);
		ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_READ_SECTORS,
				"ii:d", sector, numSectors, buffer, size);
		LWP_MutexUnlock(usb_mutex);
	}
	//dbg_printf("read %u %u = %d\n", sector, numSectors, ret);

	return ret;
}

s32 USBStorage_WriteSectors(u32 sector, u32 numSectors, const void *buffer)
{
	u32 size;
	s32 ret = -1;

	/* Device not opened */
	if (fd < 0)
		return fd;

	/* check align and MEM1 buffer */
	if (((u32)buffer & 0x1F) || (!__USBStorage_isMEM2Buffer(buffer))) {
		if (!usb_buf2) return IPC_ENOMEM;
		int cnt;
		int max_sec = USB_MEM2_SIZE / sector_size;
		//dbg_printf("usb_write(%u,%u) unaligned(%p)\n", sector, numSectors, buffer);
		while (numSectors) {
			if (numSectors > max_sec) cnt = max_sec; else cnt = numSectors;
			size = cnt * sector_size;
			LWP_MutexLock(usb_mutex);
			memcpy(usb_buf2, buffer, size);
			ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_WRITE_SECTORS,
					"ii:d", sector, cnt, usb_buf2, size);
			LWP_MutexUnlock(usb_mutex);
			//dbg_printf("usb_write_chunk(%u,%u)=%d\n", sector, cnt, ret);
			if (ret < 0) return ret;
			numSectors -= cnt;
			sector += cnt;
			buffer += size;
		}
	} else {
		size = sector_size * numSectors;
		/* Write data */
		LWP_MutexLock(usb_mutex);
		ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_WRITE_SECTORS,
				"ii:d", sector, numSectors, buffer, size);
		LWP_MutexUnlock(usb_mutex);
	}

	return ret;
}

// DISC_INTERFACE methods

static bool __io_usb_IsInserted(void)
{
	s32 ret;
	u32 sec_size;
	if (fd < 0) return false;
	ret = USBStorage_GetCapacity(&sec_size);
	if (ret == 0) return false;
	if (sec_size < 512 || sec_size > 4096) return false;
	// sector sizes other than 512 will hang/crash libfat/libntfs
	// so we explicitly don't support it here
	//if (sec_size != 512) return false;
	return true;
}

static bool __io_usb_Startup(void)
{
	if (USBStorage_Init() < 0) return false;
	return __io_usb_IsInserted();
}

int usb_verbose = 0;

bool __io_usb_ReadSectors(u32 sector, u32 count, void *buffer)
{
	s32 ret = USBStorage_ReadSectors(sector, count, buffer);
	if (usb_verbose) {
		printf("usb-r: %x [%d] = %d\n", sector, count, ret);
		//sleep(1);
	}
	return ret >= 0;
	// hermes and waninkoko up to rev20: success = 1
	// rev21: success = 0
}

bool __io_usb_WriteSectors(u32 sector, u32 count, void *buffer)
{
	/*if (!buffer || count>128) {
    	printf("USBWR %d %d %p \n", sector, count, buffer);
		Wpad_WaitButtons();
	}*/
	s32 ret = USBStorage_WriteSectors(sector, count, buffer);
	//printf("usb-w: %d %d %d\n", sector, count, ret); sleep(1);
	return ret >= 0;
}

static bool __io_usb_ClearStatus(void)
{
	return true;
}

static bool __io_usb_Shutdown(void)
{
	// do nothing
	return true;
}

static bool __io_usb_NOP(void)
{
	// do nothing
	return true;
}

const DISC_INTERFACE my_io_usbstorage = {
	DEVICE_TYPE_WII_USB,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_WII_USB,
	(FN_MEDIUM_STARTUP)      &__io_usb_Startup,
	(FN_MEDIUM_ISINSERTED)   &__io_usb_IsInserted,
	(FN_MEDIUM_READSECTORS)  &__io_usb_ReadSectors,
	(FN_MEDIUM_WRITESECTORS) &__io_usb_WriteSectors,
	(FN_MEDIUM_CLEARSTATUS)  &__io_usb_ClearStatus,
	(FN_MEDIUM_SHUTDOWN)     &__io_usb_Shutdown
};

// read-only
const DISC_INTERFACE my_io_usbstorage_ro = {
	DEVICE_TYPE_WII_USB,
	FEATURE_MEDIUM_CANREAD | FEATURE_WII_USB,
	(FN_MEDIUM_STARTUP)      &__io_usb_Startup,
	(FN_MEDIUM_ISINSERTED)   &__io_usb_IsInserted,
	(FN_MEDIUM_READSECTORS)  &__io_usb_ReadSectors,
	(FN_MEDIUM_WRITESECTORS) &__io_usb_NOP,  //&__io_usb_WriteSectors,
	(FN_MEDIUM_CLEARSTATUS)  &__io_usb_ClearStatus,
	(FN_MEDIUM_SHUTDOWN)     &__io_usb_Shutdown
};


s32 USBStorage_WBFS_Open(char *buffer)
{
	void *buf = (void *)buffer;
	u32   len = 8;

	s32 ret;

	/* Device not opened */
	if (fd < 0)
		return fd;

	/* MEM1 buffer */
	if (!__USBStorage_isMEM2Buffer(buffer)) {
		/* Allocate memory */
		//buf = iosAlloc(hid, len);
		buf = usb_buf2;
		if (!buf)
			return IPC_ENOMEM;
		memcpy(buf, buffer, len);
	}

	extern u32 wbfs_part_lba;
	u32 part = wbfs_part_lba;
	/* Read data */
	ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_WBFS_OPEN_DISC, "dd:", buf, len, &part, 4);

	return ret;
}

#if 0
// woffset is in 32bit words, len is in bytes
s32 USBStorage_WBFS_Read(u32 woffset, u32 len, void *buffer)
{
	void *buf = (void *)buffer;
	s32 ret;

	USBStorage_OpenDev();
	/* Device not opened */
	if (fd < 0)
		return fd;

	/* MEM1 buffer */
	if (!__USBStorage_isMEM2Buffer(buffer)) {
		/* Allocate memory */
		//buf = iosAlloc(hid, len);
		buf = usb_buf2;
		if (!buf)
			return IPC_ENOMEM;
	}
	*(char*)buf = 0;

	/* Read data */
	ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_WBFS_READ_DISC, "ii:d", woffset, len, buf, len);

	/* Copy data */
	if (buf != buffer) {
		memcpy(buffer, buf, len);
		//iosFree(hid, buf);
	}

	return ret;
}

s32 USBStorage_WBFS_ReadDebug(u32 off, u32 size, void *buffer)
{
	void *buf = (void *)buffer;

	s32 ret;

	USBStorage_OpenDev();
	/* Device not opened */
	if (fd < 0)
		return fd;

	/* MEM1 buffer */
	if (!__USBStorage_isMEM2Buffer(buffer)) {
		/* Allocate memory */
		//buf = iosAlloc(hid, len);
		buf = usb_buf2;
		if (!buf)
			return IPC_ENOMEM;
	}

	/* Read data */
	ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_WBFS_READ_DEBUG, "ii:d", off, size, buf, size);

	/* Copy data */
	if (buf != buffer) {
		memcpy(buffer, buf, size);
		//iosFree(hid, buf);
	}

	return ret;
}


s32 USBStorage_WBFS_SetDevice(int dev)
{
	s32 ret;
	static s32 retval = 0;
	retval = 0;
	USBStorage_OpenDev();
	// Device not opened
	if (fd < 0) return fd;
	// ioctl
	ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_WBFS_SET_DEVICE, "i:i", dev, &retval);
	if (retval) return retval;
	return ret;
}

s32 USBStorage_WBFS_SetFragList(void *p, int size)
{
	s32 ret;
	USBStorage_OpenDev();
	// Device not opened
	if (fd < 0) return fd;
	// ioctl
	DCFlushRange(p, size);
	ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_WBFS_SET_FRAGLIST, "d:", p, size);
	return ret;
}


void usb_debug_dump(int arg)
{
	//return;
	char buf[2048]="";
	//printf("\nehc fd: %d\n", fd);
	int r = USBStorage_WBFS_ReadDebug(arg, sizeof(buf), buf);
	printf("\n: %d %.2000s\n", r, buf);
}

#endif

