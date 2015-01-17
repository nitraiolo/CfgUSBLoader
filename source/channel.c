#include <gccore.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <dirent.h>

#include "isfs.h"
#include "channel.h"
#include "sys.h"
#include "nand.h"
#include "debug.h"
#include "util.h"
#include "disc.h"
#include "cfg.h"
#include "fileOps.h"

bool check_text(char *s)
{
    int i = 0;
    for(i=0; i < strlen(s); i++)
    {
		if (s[i] < 32 || s[i] > 165)
		{
			s[i] = '?';
			//return false;
		}
	}  

	return true;
}

char *get_name_from_banner_buffer(u8 *buffer)
{
	char *out;
	u32 length;
	u32 i = 0;
	while (buffer[0x21 + i*2] != 0x00)
	{
		i++;
	}
	length = i;
	if (length == 0)
		return NULL;
	out = allocate_memory(length+12);
	if(out == NULL)
	{
		dbg_printf("Allocating memory for buffer failed\n");
		return NULL;
	}
	memset(out, 0x00, length+12);
   
	i = 0;
	while (buffer[0x21 + i*2] != 0x00)
	{
		out[i] = (char) buffer[0x21 + i*2];
		i++;
	}
	return out;
}

char *read_name_from_open_banner_app_file(FILE *fp)
{
	u8 *buffer = NULL;
	u32 filesize;
	
	u8 imet[4] = {0x49, 0x4D, 0x45, 0x54};
	
	if (fp == NULL)
	{
		return NULL;
	}
	
	filesize = 0x640;		//dont need whole file first sector is more than enough
	buffer = mem2_alloc(filesize);
	fread(buffer, 1, filesize, fp);
	
	if (memcmp((buffer+0x80), imet, 4) == 0)
	{
		char *out = get_name_from_banner_buffer((void *)((u32)buffer+208));
		SAFE_FREE(buffer);
		return out;
	}

	SAFE_FREE(buffer);
	return NULL;
}

char *read_name_from_banner_app(u64 titleid)
{
	u8 *buffer = NULL;
	u32 filesize;
	
	DIR *sdir;
	FILE *fp;
	struct dirent *entry;
	static char path_buffer[255] ATTRIBUTE_ALIGN(32);
	
	u8 imet[4] = {0x49, 0x4D, 0x45, 0x54};
	
	sprintf(path_buffer, "%s/title/%08x/%08x/content", CFG.nand_emu_path, TITLE_HIGH(titleid), TITLE_LOW(titleid));
	
	dbg_printf("opendir(%s)\n", path_buffer);
	sdir = opendir(path_buffer);
	if (sdir) do {
		entry = readdir(sdir);
		if (entry)
		{
			if (!strncmp(entry->d_name, ".", 1) || !strncmp(entry->d_name, "..", 2))
 			{
				continue;
 			}
			sprintf(path_buffer, "%s/title/%08x/%08x/content/%s", CFG.nand_emu_path, TITLE_HIGH(titleid), TITLE_LOW(titleid), entry->d_name);
			
			dbg_printf("fopen(%s)\n", path_buffer);
			fp = fopen(path_buffer, "rb");
			if (fp == NULL)
			{
				continue;
			}
			
			fseek (fp, 0, SEEK_END);
			filesize = ftell(fp);
			fseek (fp, 0, SEEK_SET);
			dbg_printf("filesize = %d\n", filesize);
			filesize = 0x640;		//dont need whole file first sector is more than enough
			buffer = mem2_alloc(filesize);
			fread(buffer, 1, filesize, fp);
			dbg_printf("fclose\n");
			fclose (fp);
			
			if (memcmp((buffer+0x80), imet, 4) == 0)
			{
				dbg_printf("get_name_from_banner_buffer(%s)\n", entry->d_name);
				char *out = get_name_from_banner_buffer((void *)((u32)buffer+208));
				if (out == NULL)
				{
					SAFE_FREE(buffer);
					closedir(sdir);
					return NULL;
				}
			   
				SAFE_FREE(buffer);
				closedir(sdir);
				return out;
			}
			SAFE_FREE(buffer);
		}
	} while (entry);

	if (sdir) closedir(sdir);
	dbg_printf("read_name_from_banner_app END\n");
	return NULL;
}

char *read_name_from_banner_bin(u64 titleid)
{
    char path[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32);
	u8 *buffer = NULL;
	u32 filesize;
	FILE *fp;
   
	sprintf(path, "%s/title/%08x/%08x/data/banner.bin", CFG.nand_emu_path, TITLE_HIGH(titleid), TITLE_LOW(titleid));
 
	fp = fopen(path, "rb");
    if (fp == NULL)
    {
		return NULL;
	}
	
	filesize = 0xff;		//dont need whole file first sector is more than enough
	buffer = mem2_alloc(filesize);
	fread(buffer, 1, filesize, fp);
	fclose (fp);

	char *out = get_name_from_banner_buffer(buffer);
	if (out == NULL)
	{
		SAFE_FREE(buffer);
		return NULL;
	}
       
	SAFE_FREE(buffer);

	return out;            
}

//	fp is an already open app file containing the name which is much faster or it may be NULL 
char *get_channel_name(u64 titleid, FILE *fp)
{
	char *temp = NULL;
	u32 low;
	low = TITLE_LOW(titleid);

	// TODO
//	dbg_printf("Getting the name for: %08x...\n", titleid);
	temp = read_name_from_open_banner_app_file(fp);
	if (temp == NULL)
		temp = read_name_from_banner_bin(titleid);
	if (temp == NULL)
	{
		dbg_printf("read_name_from_banner_bin failed...\n");
		temp = read_name_from_banner_app(titleid);
	}
   
	if (temp != NULL)
	{
		check_text(temp);

		if (*(char *)&low == 'W')
		{
				return temp;
		}
		switch(low & 0xFF)
		{
			case 'E':
				memcpy(temp+strlen(temp), " (NTSC-U)", 9);
				break;
			case 'P':
				memcpy(temp+strlen(temp), " (PAL)", 6);
				break;
			case 'J':
				memcpy(temp+strlen(temp), " (NTSC-J)", 9);
				break;  
			case 'L':
				memcpy(temp+strlen(temp), " (PAL)", 6);
				break;  
			case 'N':
				memcpy(temp+strlen(temp), " (NTSC-U)", 9);
				break;          
			case 'M':
				memcpy(temp+strlen(temp), " (PAL)", 6);
				break;
			case 'K':
				memcpy(temp+strlen(temp), " (NTSC)", 7);
				break;
			default:
				break;                          
		}
	}
   
	if (temp == NULL)
	{
		temp = allocate_memory(6);
		memset(temp, 0, 6);
		memcpy(temp, (char *)(&low), 4);
	}
//	dbg_printf("Found name: %s...\n", temp);
	return temp;
}

int CHANNEL_Banner(struct discHdr *hdr, SoundInfo *snd)
{
	void *banner = NULL;
	//dbg_printf("WBFS_Banner(%.6s %d %d)\n", discid, getSound, getTitle);
	static char path_buffer[255] ATTRIBUTE_ALIGN(32);

	snd->dsp_data = NULL;
	
	u8 *buffer = NULL;
	u32 filesize;
	DIR *sdir;
	FILE *fp;
	struct dirent *entry;
	
	u8 imet[4] = {0x49, 0x4D, 0x45, 0x54};
   
	sprintf(path_buffer, "%s/title/00010001/%02x%02x%02x%02x/data/banner.bin", CFG.nand_emu_path, hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
 
// 	TODO banner.bin appears to never have the required audio info someone else fix the code if it does
//	until then changed it to always get the banner sound from the content dir
//	instead of just when banner.bin dosent exist
//	fp = fopen(path_buffer, "rb");
//  if (fp == NULL)
//    {
	sprintf(path_buffer, "%s/title/00010001/%02x%02x%02x%02x/content", CFG.nand_emu_path, hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
	
	sdir = opendir(path_buffer);
	if (sdir) {
		do {
			entry = readdir(sdir);
			if (entry)
			{
				if (!strncmp(entry->d_name, ".", 1) || !strncmp(entry->d_name, "..", 2))
				{
					continue;
				}
				sprintf(path_buffer, "%s/title/00010001/%02x%02x%02x%02x/content/%s", CFG.nand_emu_path, hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3], entry->d_name);
				
				fp = fopen(path_buffer, "rb");
				if (fp == NULL)
				{
					continue;
				}
				
				fseek (fp, 0, SEEK_END);
				filesize = ftell(fp);
				fseek (fp, 0, SEEK_SET);
				buffer = mem2_alloc(filesize);
				if (buffer == NULL)
				{
					fclose (fp);
					continue;	//skip files bigger than memory
				}
				fread(buffer, 1, filesize, fp);
				fclose (fp);
				
				if (memcmp((buffer+0x80), imet, 4) == 0)
				{
					banner = mem2_alloc(filesize-0x40);
					memcpy(banner, buffer+0x40, filesize-0x40);
					SAFE_FREE(buffer);
					break;
				}
				SAFE_FREE(buffer);
			}
		} while (entry);
	closedir(sdir);
	}
//	} else {
//		fseek (fp, 0, SEEK_END);
//		filesize = ftell(fp);
//		fseek (fp, 0, SEEK_SET);
//		banner = mem2_alloc(filesize);
//		fread(banner, 1, filesize, fp);
//		fclose (fp);
//	}

	if (banner != NULL)
		parse_banner_snd(banner, snd);
	SAFE_FREE(banner);
	return 0;
}

u64 getChannelSize(struct discHdr *hdr) {
	static char path_buffer[255] ATTRIBUTE_ALIGN(32);
	sprintf(path_buffer, "%s/title/00010001/%02x%02x%02x%02x", CFG.nand_emu_path, hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
	
	return fsop_GetFolderBytes(path_buffer);
}

u64 getChannelReqIos(struct discHdr *hdr) {
	u64 ReqIos = 0;
	static char path_buffer[255] ATTRIBUTE_ALIGN(32);

	sprintf(path_buffer, "%s/title/00010001/%02x%02x%02x%02x/content/title.tmd", CFG.nand_emu_path, hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
	FILE *fp = fopen(path_buffer, "rb");
	if (fp) {
		fseek(fp, 0x184 , SEEK_SET);
		fread(&(ReqIos), 8, 1, fp);
		fclose(fp);
	}
	return ReqIos;
}

s32 Channel_RemoveGame(struct discHdr *hdr) {
	static char path_buffer[255] ATTRIBUTE_ALIGN(32);

	sprintf(path_buffer, "%s/ticket/00010001/%02x%02x%02x%02x.tik", CFG.nand_emu_path, hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
	remove(path_buffer);

	sprintf(path_buffer, "%s/title/00010001/%02x%02x%02x%02x", CFG.nand_emu_path, hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
	fsop_deleteFolder(path_buffer);
	
	return 0;
}

