/*****************************************
* This code is from Mighty Channel 11
* which is based on the TriiForce source.
* This also includes code from Simple
* FS Dumper
* modified by R2-D2199
*****************************************/
#include <stdio.h>
#include <ogcsys.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <dirent.h>

#include "debug.h"
#include "nand.h"
#include "sys.h"
#include "video.h"
#include "cfg.h"

#define BLOCKSIZE 2048

#define DIRENT_T_FILE 0
#define DIRENT_T_DIR 1

#define INIT_FIRST 1
#define INIT_RESET 0

/* Buffer */
static const char fs[] ATTRIBUTE_ALIGN(32) = "/dev/fs";
static const char fat[] ATTRIBUTE_ALIGN(32) = "fat";
static u32 inbuf[8] ATTRIBUTE_ALIGN(32);

static nandDevice ndevList[] =
{
    { "Disable",						0,	0x00,	0x00 },
    { "SD/SDHC Card",					1,	0xF0,	0xF1 },
    { "USB 2.0 Mass Storage Device",	2,	0xF2,	0xF3 },
};

static int mounted = 0;
static int partition = 0;
static int fullmode = 0;
static char path[32] = "\0";

u8 get_nand_device() {
	return strncmp(CFG.nand_emu_path, "usb", 3) == 0 ? EMU_USB : EMU_SD;
}

s32 Nand_Mount(nandDevice *dev)
{
    s32 fd, ret;
    u32 inlen = 0;
    ioctlv *vector = NULL;
    u32 *buffer = NULL;
    int rev = IOS_GetRevision();

    /* Open FAT module */
    fd = IOS_Open(fat, 0);
    if (fd < 0)
        return fd;

    /* Prepare vector */
    if((rev >= 21 && rev < 30000) || is_ios_d2x())
    {
        // NOTE:
        // The official cIOSX rev21 by Waninkoko ignores the partition argument
        // and the nand is always expected to be on the 1st partition.
        // However this way earlier d2x betas having revision 21 take in
        // consideration the partition argument.
        inlen = 1;

        /* Allocate memory */
        buffer = (u32 *)memalign(32, sizeof(u32)*3);

        /* Set vector pointer */
        vector = (ioctlv *)buffer;

        buffer[0] = (u32)(buffer + 2);
        buffer[1] = sizeof(u32);
        buffer[2] = (u32)partition;
    }

    /* Mount device */
    ret = IOS_Ioctlv(fd, dev->mountCmd, inlen, 0, vector);

    /* Close FAT module */
    //!TODO: Figure out why this causes a freeze
    //IOS_Close(fd);

    /* Free memory */
    if(buffer != NULL)
        free(buffer);

    return ret;
}

s32 Nand_Unmount(nandDevice *dev)
{
    s32 fd, ret;

    // Open FAT module
    fd = IOS_Open(fat, 0);
    if (fd < 0)
        return fd;

    // Unmount device
    ret = IOS_Ioctlv(fd, dev->umountCmd, 0, 0, NULL);

    // Close FAT module
    IOS_Close(fd);

    return ret;
}

s32 Nand_Enable(nandDevice *dev)
{
    s32 fd, ret;
    u32 *buffer = NULL;
    int rev;

    // Open /dev/fs
    fd = IOS_Open(fs, 0);
    if (fd < 0)
        return fd;

    rev = IOS_GetRevision();
    // Set input buffer
    if(((rev >= 21 && rev < 30000) || is_ios_d2x()) && dev->mode != 0)
    {
        //FULL NAND emulation since rev18
        //needed for reading images on triiforce mrc folder using ISFS commands
        inbuf[0] = dev->mode | fullmode;
    }
    else
    {
        inbuf[0] = dev->mode; //old method
    }

    // Enable NAND emulator
    if((rev >= 21 && rev < 30000) || is_ios_d2x())
    {
        // NOTE:
        // The official cIOSX rev21 by Waninkoko provides an undocumented feature
        // to set nand path when mounting the device.
        // This feature has been discovered during d2x development.
        int pathlen = strlen(path)+1;

        /* Allocate memory */
        buffer = (u32 *)memalign(32, (sizeof(u32)*5)+pathlen);

        buffer[0] = (u32)(buffer + 4);
        buffer[1] = sizeof(u32);   // actually not used by cios
        buffer[2] = (u32)(buffer + 5);
        buffer[3] = pathlen;       // actually not used by cios
        buffer[4] = inbuf[0];
        strcpy((char*)(buffer+5), path);

        ret = IOS_Ioctlv(fd, 100, 2, 0, (ioctlv *)buffer);
    }
    else
    {
        ret = IOS_Ioctl(fd, 100, inbuf, sizeof(inbuf), NULL, 0);
    }

    /* Free memory */
    if(buffer != NULL)
        free(buffer);

    // Close /dev/fs
    IOS_Close(fd);

    return ret;
}

s32 Nand_Disable(void)
{
    s32 fd, ret;

    // Open /dev/fs
    fd = IOS_Open(fs, 0);
    if (fd < 0)
        return fd;

    // Set input buffer
    inbuf[0] = 0;

    // Disable NAND emulator
    ret = IOS_Ioctl(fd, 100, inbuf, sizeof(inbuf), NULL, 0);

    // Close /dev/fs
    IOS_Close(fd);

    return ret;
}


s32 Enable_Emu(int selection)
{
    if(mounted != 0)
        return -1;

    s32 ret;
    nandDevice *ndev = NULL;
    ndev = &ndevList[selection];

    ret = Nand_Mount(ndev);
    if (ret < 0)
    {
        dbg_printf(" ERROR Mount! (ret = %d)\n", ret);
        return ret;

    }

    ret = Nand_Enable(ndev);
    if (ret < 0)
    {
        dbg_printf(" ERROR Enable! (ret = %d)\n", ret);
        return ret;
    }
    mounted = selection;
    return 0;
}

s32 Disable_Emu()
{
    if(mounted==0)
        return 0;

    nandDevice *ndev = NULL;
    ndev = &ndevList[mounted];

    Nand_Disable();
    Nand_Unmount(ndev);

    mounted = 0;

    return 0;
}

void Set_Partition(int p)
{
    partition = p;
}

void Set_Path(const char* p)
{
    int i = 0;

    while(p[i] != '\0' && i < 31)
    {
        path[i] = p[i];
        i++;
    }
    while(path[i-1] == '/')
    {
        path[i-1] = '\0';
        --i;
    }
    path[i] = '\0';
}

void Set_FullMode(int fm)
{
    fullmode = fm ? 0x100 : 0;
}

const char* Get_Path(void)
{
    return path;
}

s32 dumpfile(char source[1024], char destination[1024])
{
	u8 *buffer;
	fstats *status;

	FILE *file;
	int fd;
	s32 ret;
	u32 size;

	fd = ISFS_Open(source, ISFS_OPEN_READ);
	if (fd < 0) 
	{
		dbg_printf("\nError: ISFS_OpenFile(%s) returned %d\n", source, fd);
		return fd;
	}
	
	file = fopen(destination, "wb");
	if (!file)
	{
		dbg_printf("\nError: fopen(%s) returned 0\n", destination);
		ISFS_Close(fd);
		return -1;
	}

	status = memalign(32, sizeof(fstats) );
	ret = ISFS_GetFileStats(fd, status);
	if (ret < 0)
	{
		dbg_printf("\nISFS_GetFileStats(fd) returned %d\n", ret);
		ISFS_Close(fd);
		fclose(file);
		free(status);
		return ret;
	}
	Con_ClearLine();
	dbg_printf("Dumping file %s, size = %uKB", source, (status->file_length / 1024)+1);

	buffer = (u8 *)memalign(32, BLOCKSIZE);
	u32 restsize = status->file_length;
	while (restsize > 0)
	{
		if (restsize >= BLOCKSIZE)
		{
			size = BLOCKSIZE;
		} else
		{
			size = restsize;
		}
		ret = ISFS_Read(fd, buffer, size);
		if (ret < 0)
		{
			dbg_printf("\nISFS_Read(%d, %p, %d) returned %d\n", fd, buffer, size, ret);
			ISFS_Close(fd);
			fclose(file);
			free(status);
			free(buffer);
			return ret;
		}
		ret = fwrite(buffer, 1, size, file);
		if(ret < 0) 
		{
			dbg_printf("\nfwrite error%d\n", ret);
			ISFS_Close(fd);
			fclose(file);
			free(status);
			free(buffer);
			return ret;
		}
		restsize -= size;
	}
	ISFS_Close(fd);
	fclose(file);
	free(status);
	free(buffer);
	return 0;
}

int isdir(char *path)
{
	s32 res;
	u32 num = 0;

	res = ISFS_ReadDir(path, NULL, &num);
	if(res < 0)
		return 0;
		
	return 1;
}

void get_attribs(char *path, u32 *ownerID, u16 *groupID, u8 *ownerperm, u8 *groupperm, u8 *otherperm)
{
	s32 res;
	u8 attributes;
	res = ISFS_GetAttr(path, ownerID, groupID, &attributes, ownerperm, groupperm, otherperm);
	if(res != ISFS_OK)
		dbg_printf("Error getting attributes of %s! (result = %d)\n", path, res);
}

s32 __FileCmp(const void *a, const void *b)
{
	dirent_t *hdr1 = (dirent_t *)a;
	dirent_t *hdr2 = (dirent_t *)b;
	
	if (hdr1->type == hdr2->type)
	{
		return strcmp(hdr1->name, hdr2->name);
	} else
	{
		if (hdr1->type == DIRENT_T_DIR)
		{
			return -1;
		} else
		{
			return 1;
		}
	}
}

s32 get_nand_dir(char *path, dirent_t **ent, s32 *cnt)
{
	s32 res;
	u32 num = 0;

	int i, j, k;

	//Get the entry count.
	res = ISFS_ReadDir(path, NULL, &num);
	if(res != ISFS_OK)
	{
		dbg_printf("Error: could not get dir entry count! (result: %d)\n", res);
		return -1;
	}

	//Allocate aligned buffer.
	char *nbuf = (char *)allocate_memory((ISFS_MAXPATH + 1) * num);
	char ebuf[ISFS_MAXPATH + 1];
	char pbuf[ISFS_MAXPATH + 1];

	if(nbuf == NULL)
	{
		dbg_printf("Error: could not allocate buffer for name list!\n");
		return -2;
	}

	//Get the name list.
	res = ISFS_ReadDir(path, nbuf, &num);
	if(res != ISFS_OK)
	{
		dbg_printf("Error: could not get name list! (result: %d)\n", res);
		return -3;
	}
	
	//Set entry cnt.
	*cnt = num;

	if (*ent)
	{
		free(*ent);
	}
	*ent = allocate_memory(sizeof(dirent_t) * num);

	u32 ownerID = 0;
	u16 groupID = 0;
	u8 attributes = 0;
	u8 ownerperm = 0;
	u8 groupperm = 0;
	u8 otherperm = 0;

	//Split up the name list.
	for(i = 0, k = 0; i < num; i++)
	{
		//The names are seperated by zeroes.
		for(j = 0; nbuf[k] != 0; j++, k++)
			ebuf[j] = nbuf[k];
		ebuf[j] = 0;
		k++;

		//Fill in the dir name.
		strcpy((*ent)[i].name, ebuf);
		//Build full path.
		if(strcmp(path, "/") != 0)
			sprintf(pbuf, "%s/%s", path, ebuf);
		else
			sprintf(pbuf, "/%s", ebuf);
		//Dir or file?
		(*ent)[i].type = ((isdir(pbuf) == 1) ? DIRENT_T_DIR : DIRENT_T_FILE);

		res = ISFS_GetAttr(pbuf, &ownerID, &groupID, &attributes, &ownerperm, &groupperm, &otherperm);
		if(res != ISFS_OK)
		{
			ownerID = 0;
			groupID = 0;
			attributes = 0;
			ownerperm = 0;
			groupperm = 0;
			otherperm = 0;
		}
		(*ent)[i].ownerID = ownerID;
		(*ent)[i].groupID = groupID;
		(*ent)[i].attributes = attributes;
		(*ent)[i].ownerperm = ownerperm;
		(*ent)[i].groupperm = groupperm;
		(*ent)[i].otherperm = otherperm;
	}
	qsort(*ent, *cnt, sizeof(dirent_t), __FileCmp);
	
	free(nbuf);
	return 0;
}

bool dumpfolder(char source[1024], char destination[1024])
{
	s32 tcnt;
	int ret;
	int i;
	char path[1024];
	char path2[1024];
	dirent_t *dir = NULL;

	get_nand_dir(source, &dir, &tcnt);

	if (strlen(source) == 1)
	{
		sprintf(path, "%s", destination);
	} else
	{
		sprintf(path, "%s%s", destination, source);
	}

	ret = (u32)opendir(path);
	if (ret == 0)
	{
		ret = mkdir(path, 0777);
		if (ret < 0)
		{
			dbg_printf("Error making directory %d...\n", ret);
			free(dir);
			return false;
		}
	}
	
	for(i = 0; i < tcnt; i++) 
	{				
		if (strlen(source) == 1)
		{
			sprintf(path, "%s%s", source, dir[i].name);
		} else
		{
			sprintf(path, "%s/%s", source, dir[i].name);
		}
		
		if(dir[i].type == DIRENT_T_FILE) 
		{
			sprintf(path2, "%s%s", destination, path);

			//dbg_printf("Dumping file: %s\n", path);
			//dbg_printf("To: %s\n", path2);

			//sleep(5);
			ret = dumpfile(path, path2);
			if (ret < 0)
			{
				sleep(1);
				free(dir);
				return false;
			}
		} else
		{
			if(dir[i].type == DIRENT_T_DIR) 
			{
				if (!dumpfolder(path, destination))
				{
					free(dir);
					return false;
				}
			}	
		}
	}
	free(dir);
	//dbg_printf("Dumping folder %s complete\n\n", source);
	return true;
}

s32 get_dir_count(char *path, u32 *cnt)
{
	if (cnt == NULL) return -2;

	u32 temp = 0;
	s32 ret = ISFS_ReadDir(path, NULL, &temp);
	if (ret != ISFS_OK)
	{
		dbg_printf("Error: ISFS_ReadDir('%s') ret = %d\n", path, ret);
		return -1;
	}
	*cnt = temp;

	return 1;
}