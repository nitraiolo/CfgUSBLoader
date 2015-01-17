#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ogcsys.h>
#include <fat.h>
#include <dirent.h>
#include <sys/stat.h> //for mkdir

#define DEV_MOUNT "fat"
#include "fat_mine.h"
#include <sdcard/wiisd_io.h>
#include "usbstorage.h"
extern const DISC_INTERFACE __io_usb2storage;

const DISC_INTERFACE* interface;
/*typedef struct {
	// Device mount point
	char *mount;

	// Device interface
	const DISC_INTERFACE *interface;
} fatDevice;*/

s32 Fat_Mount(int dev){
	s32 ret;

	if(dev==SD)
		interface=&__io_wiisd;
	else if(dev==USB)
		interface=&__io_wiiums;
	else
		return -1;
	

	// Initialize SDHC interface
	ret = interface->startup();
	if (!ret)
		return -1;

	// Mount device
	ret = fatMountSimple(DEV_MOUNT, interface);
	if (!ret)
		return -2;

	return 0;
}

s32 Fat_Unmount(void){
	s32 ret;

	// Unmount device
	fatUnmount(DEV_MOUNT);

	// Shutdown SDHC interface
	ret = interface->shutdown();
	if (!ret)
		return -1;

	return 0;
}

s32 Fat_ReadFile(const char *filepath, void **outbuf){
	FILE *fp     = NULL;
	void *buffer = NULL;

	u32         filelen;

	s32 ret;

	/* Open file */
	fp = fopen(filepath, "rb");
	if (!fp)
		goto err;

	/* Get filesize */
	fseek(fp, 0, SEEK_END);
	filelen = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* Allocate memory */
	buffer = malloc(filelen);
	if (!buffer)
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

int Fat_MakeDir(const char *dirname){
	int ret=-1;
	DIR *dir;

	//if(dirname[strlen(dirname)-2]=='/')
	//	return false;
		
	dir=opendir(dirname);
	if(dir){
		ret=1;
		closedir(dir);
	}else{
		mkdir(dirname, S_IREAD | S_IWRITE);
		ret=0;
	}

	return ret;
}

bool Fat_CheckFile(const char *filepath){
	FILE *fp     = NULL;

	/* Open file */
	fp = fopen(filepath, "rb");
	if (!fp)
		return false;

	fclose(fp);

	return true;
}

s32 Fat_SaveFile(const char *filepath, void **outbuf, u32 outlen){
	s32 ret;
	FILE *fd;
	fd = fopen(filepath, "wb");
	if(fd){
		ret=fwrite(*outbuf, 1, outlen, fd);
		fclose(fd);
		//printf(" FWRITE: %d ",ret);
	}else{
		ret=-1;
	}

	return ret;
}