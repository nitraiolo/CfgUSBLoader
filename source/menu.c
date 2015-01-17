
// Modified by oggzee & usptactical

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdarg.h>
#include <ctype.h>
#include <wctype.h>
#include <wchar.h>
#include <locale.h>
#include <asndlib.h>
#include <dirent.h>
#include <gccore.h>
#include <ogc/ipc.h>
#include <ogc/lwp_threads.h>
#include <ogc/machine/processor.h>
#include <fat.h>

#include "disc.h"
#include "fat.h"
#include "cache.h"
#include "gui.h"
#include "menu.h"
#include "partition.h"
#include "restart.h"
#include "sys.h"
#include "util.h"
#include "utils.h"
#include "video.h"
#include "wbfs.h"
#include "libwbfs/libwbfs.h"
#include "wpad.h"
#include "patchcode.h"
#include "cfg.h"
#include "http.h"
#include "dns.h"
#include "wdvd.h"
#include "music.h"
#include "subsystem.h"
#include "net.h"
#include "fst.h"
#include "wiip.h"
#include "frag.h"
#include "xml.h" /* WiiTDB database - Lustar */
#include "sort.h"
#include "gettext.h"
#include "playlog.h"
#include "dolmenu.h"
#include "gc.h"
#include "gc_wav.h"
#include "fileOps.h"
#include "sdhc.h"
#include "nand.h"
#include "dol.h"
#include "savegame.h"
#include "channel.h"
#include "RuntimeIOSPatch.h"

void _unstub_start();

void*	dolchunkoffset[64];		
u32		dolchunksize[64];			
u32		dolchunkcount;

extern void __exception_closeall();

#define TITLE_UPPER(x)		((u32)((x) >> 32))
#define TITLE_LOWER(x)		((u32)(x))
#define TITLE_ID(x,y)		(((u64)(x) << 32) | (y))

typedef void (*entrypoint) (void);

#define CHANGE(V,M) {V+=change; if(V>(M)) V=(M); if(V<0) V=0;}
#define HW_PPCSPEED                     ((vu32*)0xCD800018)


char CFG_VERSION[] = CFG_VERSION_STR;

void Sys_Exit();

/* Gamelist buffer */
struct discHdr *all_gameList = NULL;
struct discHdr *fav_gameList = NULL;
struct discHdr *gameList = NULL;
struct discHdr *filter_gameList = NULL;

/* Gamelist variables */
bool enable_favorite = false;
s32 all_gameCnt = 0;
s32 fav_gameCnt = 0;
s32 filter_gameCnt = 0;
extern s32 filter_type;
extern s32 filter_index;

bool MountWBFS = true;

s32 gameCnt = 0, gameSelected = 0, gameStart = 0, chanSelected = 0, chanStart = 0, mycount = 0;

bool imageNotFound = false;

/* admin unlock mode variables */
static int disable_format = 0;
static int disable_remove = 0;
static int disable_install = 0;
static int disable_options = 0;
static int confirm_start = 1;
static bool unlock_init = true;

extern char wbfs_fs_drive[16];
extern char dm_boot_drive[16];

char *videos[CFG_VIDEO_NUM] = 
{
	gts("System Def."),
	gts("Game Default"),
	gts("Force PAL50"),
	gts("Force PAL60"),
	gts("Force NTSC")
};

char *DML_videos[6] = 
{
	gts("Auto"),
	gts("Force PAL"),
	gts("Force NTSC"),
	gts("Force PAL60"),
	gts("Force NTSC 480p"),
	gts("Force PAL 480p")
};

char *languages[CFG_LANG_NUM] =
{
	gts("Auto"),
	gts("Japanese"),
	gts("English"),
	gts("German"),
	gts("French"),
	gts("Spanish"),
	gts("Italian"),
	gts("Dutch"),
	gts("S. Chinese"),
	gts("T. Chinese"),
	gts("Korean")
};

char *playlog_name[4] =
{
	gts("Off"),
	gts("On"),
	gts("Japanese Title"),
	gts("English Title")
};

char *str_wiird[3] =
{
	gts("Off"),
	gts("On"),
	gts("Paused Start")
};

char *str_dml[8] =
{
	gts("Auto"),
	gts("DEVO"),
    gts("r51-"),
	gts("r52+"),
	gts("1.2+"),
	gts("2.0+"),
	gts("2.1+"),
	gts("2.2+")
};

char *str_nand_emu[3] =
{
	gts("Off"),
	gts("Partial"),
	gts("Full")
};

char *str_channel_boot[2] =
{
	gts("Mighty Plugin"),
	gts("Neek2o Plugin")
};

char *str_gc_boot[4] =
{
	gts("Default"),
	gts("DIOS MIOS")
	gts("Devolution")
	gts("Nintendont")
};

int Menu_Global_Options();
int Menu_Game_Options();
void Switch_Favorites(bool enable);
void bench_io();

char action_string[40] = "";

#ifdef FAKE_GAME_LIST
// Debug Test mode with fake game list
#define WBFS_GetCount dbg_WBFS_GetCount
#define WBFS_GetHeaders dbg_WBFS_GetHeaders
#endif

char *__Menu_PrintTitle(char *name)
{
	//static char buffer[MAX_CHARACTERS + 4];
	static char buffer[400];
	int len = con_len(name);

	/* Clear buffer */
	memset(buffer, 0, sizeof(buffer));

	/* Check string length */
	if (len >= MAX_CHARACTERS) {
		//strncpy(buffer, name,  MAX_CHARACTERS - 4);
		STRCOPY(buffer, name);
		con_trunc(buffer, MAX_CHARACTERS - 4);
		strcat(buffer, "..");
		return buffer;
	}

	return name;
}

void __Chan_MoveList(s8 delta)
{
	/* No channellist */
	if (!mycount)
		return;

	#if 0
	/* Select next entry */
	chanSelected += delta;

	/* Out of the list? */
	if (chanSelected <= -1)
		chanSelected = (mycount - 1);
	if (chanSelected >= mycount)
		chanSelected = 0;
	#endif

	if(delta>0) {
		if(chanSelected == mycount - 1) {
			chanSelected = 0;
		}
		else {
			chanSelected +=delta;
			if(chanSelected >= mycount) {
				chanSelected = (mycount - 1);
			}
		}
	}
	else {
		if(!chanSelected) {
			chanSelected = mycount - 1;
		}
		else {
			chanSelected +=delta;
			if(chanSelected < 0) {
				chanSelected = 0;
			}
		}
	}

	/* List scrolling */
	s32 index = (chanSelected - chanStart);

	if (index >= ENTRIES_PER_PAGE)
		chanStart += index - (ENTRIES_PER_PAGE - 1);
	if (index <= -1)
		chanStart += index;
}

char *skip_sort_ignore(char *s)
{
	char tok[200], *p;
	int len;
	p = CFG.sort_ignore;
	while (p) {
		p = split_token(tok, p, ',', sizeof(tok));
		len = strlen(tok);
		if (len && strncasecmp(s, tok, strlen(tok)) == 0) {
			if (s[len] == ' ' || s[len] == '\'') {
				s += len + 1;
			}
		}
	}
	return s;
}

struct discHdr *getHeaderById(struct discHdr *list, int cnt, u8 *id, u8 disc) {
	u32 i = 0;
	for (i=0; i < cnt; i++) {
		struct discHdr *header = list+i;
		if (!memcmp(header->id, id, 6) && header->disc == disc) return header;
	}
	return NULL;
}

void Setup_NandEmu(u8 NandEmuMode, const char *NandEmuPath)
{
	if(NandEmuMode && strchr(NandEmuPath, '/'))
	{
		if (!strncmp(NandEmuPath, "usb", 3) || !strncmp(NandEmuPath, "sd", 2)) 
		{
			int partition = 0;

			dbg_printf("Enabling Nand Emulation on: %s\n", NandEmuPath);
			Set_FullMode(NandEmuMode == 2);
			Set_Path(strchr(NandEmuPath, '/'));

			//! Unmount devices to flush data before activating NAND Emu
			if(strncmp(NandEmuPath, "usb", 3) == 0)
			{
				//! Set which partition to use (USB only)
				partition = atoi(NandEmuPath+3)-1;
				Set_Partition(partition);
			}

			Enable_Emu(get_nand_device());

			//! Mount USB to start game, SD is not required
			if(strncmp(NandEmuPath, "usb", 3) == 0) {
				CFG_MountUSB();
				mount_find(USB_DRIVE);
			}
		}
	}
}


bool unwanted_channel(const char *d_name)
{
	if (!strncmp(d_name, ".", 1)			// current directory
	 || !strncmp(d_name, "..", 2)			// previous directory
	 || !strncmp(d_name, "48414141", 8)	// H A A A Photo channel 1.0
	 || !strncmp(d_name, "af1bf516", 8)	// B o o t M i i
	 || !strncmp(d_name, "4a4f4449", 8)	// J O D I home brew channel
	 || !strncmp(d_name, "48415858", 8)	// H A X X home brew channel
	 || !strncmp(d_name, "55435846", 8)	// U C X F cfg loader forwarder
	 || !strncmp(d_name, "4d415549", 8)	// M A U I backup homebrew channel
	 || !strncmp(d_name, "4e454f47", 8)	// N E O G backup disk channel
										)
		return true;
	 else
		return false;
}

s32 get_channel_list(void *outbuf, u32 size) 
{
	s32 ret = 0;
	
	static char path_buffer[255] ATTRIBUTE_ALIGN(32);
	struct dirent *entry;
	
	FILE *fp;
	DIR *sdir;
	
	u8 *buffer = allocate_memory(0x1E4+4);
	
	char gamePath[255];
	sprintf(gamePath, "%s/title/00010001", CFG.nand_emu_path);
	sdir = opendir(gamePath);
	if (sdir) do
	{
		entry = readdir(sdir);
		if (entry)
		{
//			sprintf(path, "%s/title/00010001/%s", CFG.nand_emu_path, entry->d_name);
			if (unwanted_channel(entry->d_name))
 			{
				continue;
 			}
			sprintf(path_buffer, "%s/title/00010001/%s/content/title.tmd", CFG.nand_emu_path, entry->d_name);
			fp = fopen(path_buffer, "rb");
			if (fp)
			{
				fread(buffer, 1, 0x1E4+4, fp);
				fclose(fp);
				u32 bannerContent = 0;
				memcpy(&bannerContent, buffer+0x1E4, 4);
				sprintf(path_buffer, "%s/title/00010001/%s/content/%08x.app", CFG.nand_emu_path, entry->d_name, bannerContent);
				fp = fopen(path_buffer, "rb");
				if (fp)
				{
					u8 *ptr = ((u8 *)outbuf) + (ret * sizeof(struct discHdr));
					struct discHdr *channel = (struct discHdr *)ptr;
					
					channel->magic = CHANNEL_MAGIC;
					//strncpy(channel->path, path, strlen(path));
					u32 title_low = TITLE_LOW(strtol(entry->d_name,NULL,16));
					memcpy(channel->id, &title_low, sizeof(u32));
					char *name = get_channel_name(TITLE_ID(0x00010001, title_low),fp);
					strncpy(channel->title, name, sizeof(channel->title));
					SAFE_FREE(name);
					
					ret++;
					fclose(fp);
				}
			}
		}
	} while (entry);
	if (sdir) closedir(sdir);

	SAFE_FREE(buffer);
	return ret;
}

s32 get_channel_list_cnt()
{	
	s32 ret = 0;
	
	struct dirent *entry;
	
	DIR *sdir;
	
	char gamePath[255];
	sprintf(gamePath, "%s/title/00010001", CFG.nand_emu_path);
	dbg_printf("\nSearching channel games in %s ...\n", gamePath);
	sdir = opendir(gamePath);
	if (sdir) do
	{
		entry = readdir(sdir);
		if (entry)
		{
			ret++;	//assume if the dir is their that it is a valid channel
					//No need to actually open the files here will be done during the actual load
					//This is much faster but uses slightly more mem while loading		
		}		
	} while (entry);
	if (sdir) closedir(sdir);

	return ret;
}

s32 get_DML_game_list(void *outbuf)
{
	FILE *fp;
	u32 DML_GameCount = 0;
	
	static char name_buffer[64] ATTRIBUTE_ALIGN(32);
    DIR *sdir;
    DIR *s2dir;
    struct dirent *entry;

	// 1st count the number of games on SD
	char gamePath[255];
	sprintf(gamePath, "%s/games", dm_boot_drive);
	dbg_printf("\nSearching GC games in %s ...\n", gamePath);
	sdir = opendir(gamePath);
	if (sdir) do
	{
		entry = readdir(sdir);
		if (entry)
		{
			sprintf(name_buffer, "%s/games/%s", dm_boot_drive, entry->d_name);
			if (!strncmp(entry->d_name, ".", 1) || !strncmp(entry->d_name, "..", 2))
 			{
				continue;
 			}
			s2dir =  opendir(name_buffer);
			if (s2dir)
			{
				sprintf(name_buffer, "%s/games/%s/game.iso", dm_boot_drive, entry->d_name);
				fp = fopen(name_buffer, "rb");
				if (fp)
				{
					fseek(fp, 0, SEEK_END);
					if (ftell(fp) > 1000000)
					{
						u8 *ptr = ((u8 *)outbuf) + (DML_GameCount * sizeof(struct discHdr));
						struct discHdr *dmlGame = (struct discHdr *)ptr;
						
						struct gc_discHdr header;
						fseek(fp, 0, SEEK_SET);
						fread(&header, 1, sizeof(struct gc_discHdr), fp);
						fclose(fp);
						
						if (header.magic == GC_MAGIC && !getHeaderById(outbuf, DML_GameCount, header.id, header.disc)) {
							memcpy(dmlGame->id, header.id, 6);
							dmlGame->disc = header.disc;
							memcpy(dmlGame->title, header.title, sizeof(dmlGame->title));
							
							sprintf(dmlGame->path, "%s/games/%s", dm_boot_drive, entry->d_name);
							dmlGame->magic = GC_GAME_ON_DRIVE;

							DML_GameCount++;
						}
					}
					fclose(fp);
				}
				else
				{
					sprintf(name_buffer, "%s/games/%s/sys/boot.bin", dm_boot_drive, entry->d_name);
					fp = fopen(name_buffer, "rb");
					if (fp)
					{
						u8 *ptr = ((u8 *)outbuf) + (DML_GameCount * sizeof(struct discHdr));
						struct discHdr *dmlGame = (struct discHdr *)ptr;
						
						struct gc_discHdr header;
						fseek(fp, 0, SEEK_SET);
						fread(&header, 1, sizeof(struct gc_discHdr), fp);
						fclose(fp);
						
						if (header.magic == GC_MAGIC && !getHeaderById(outbuf, DML_GameCount, header.id, header.disc)) {
							memcpy(dmlGame->id, header.id, 6);
							dmlGame->disc = header.disc;
							memcpy(dmlGame->title, header.title, sizeof(dmlGame->title));
							
							sprintf(dmlGame->path, "%s/games/%s", dm_boot_drive, entry->d_name);
							dmlGame->magic = GC_GAME_ON_DRIVE;

							DML_GameCount++;
						}
					}
				}
				closedir(s2dir);
			}
		}
	} while (entry);
	if (sdir) closedir(sdir);
	sdir = NULL;
	
	if (strncmp(wbfs_fs_drive, dm_boot_drive, strlen(dm_boot_drive))) {
		// 2st count the number of games on hdd+
		char filepath[0xFF];
		sprintf(filepath, "%s/games", wbfs_fs_drive);
		dbg_printf("\nSearching GC games in %s ...\n", filepath);
		sdir = opendir(filepath);
		if (sdir) do
		{
			entry = readdir(sdir);
			if (entry)
			{
				sprintf(name_buffer, "%s/games/%s", wbfs_fs_drive, entry->d_name);
				if (!strncmp(entry->d_name, ".", 1) || !strncmp(entry->d_name, "..", 2))
				{
					continue;
				}
				s2dir =  opendir(name_buffer);
				if(s2dir)
				{
					sprintf(name_buffer, "%s/games/%s/game.iso", wbfs_fs_drive, entry->d_name);
					fp = fopen(name_buffer, "rb");
					if(fp)
					{
						u8 *ptr = ((u8 *)outbuf) + (DML_GameCount * sizeof(struct discHdr));
						struct discHdr *dmlGame = (struct discHdr *)ptr;
						
						struct gc_discHdr header;
						fseek(fp, 0, SEEK_SET);
						fread(&header, 1, sizeof(struct gc_discHdr), fp);
						fclose(fp);
						
						if (header.magic == GC_MAGIC && !getHeaderById(outbuf, DML_GameCount, header.id, header.disc)) {
							memcpy(dmlGame->id, header.id, 6);
							dmlGame->disc = header.disc;
							memcpy(dmlGame->title, header.title, sizeof(dmlGame->title));
							
							sprintf(dmlGame->path, "%s/games/%s", wbfs_fs_drive, entry->d_name);
							dmlGame->magic = GC_GAME_ON_DRIVE;

							DML_GameCount++;
						}
					}
					else
					{
						sprintf(name_buffer, "%s/games/%s/sys/boot.bin", wbfs_fs_drive, entry->d_name);
						fp = fopen(name_buffer, "rb");
						if(fp)
						{
							u8 *ptr = ((u8 *)outbuf) + (DML_GameCount * sizeof(struct discHdr));
							struct discHdr *dmlGame = (struct discHdr *)ptr;
							
							struct gc_discHdr header;
							fseek(fp, 0, SEEK_SET);
							fread(&header, 1, sizeof(struct gc_discHdr), fp);
							fclose(fp);
							
							if (header.magic == GC_MAGIC && !getHeaderById(outbuf, DML_GameCount, header.id, header.disc)) {
								memcpy(dmlGame->id, header.id, 6);
								dmlGame->disc = header.disc;
								memcpy(dmlGame->title, header.title, sizeof(dmlGame->title));
								
								sprintf(dmlGame->path, "%s/games/%s", wbfs_fs_drive, entry->d_name);
								dmlGame->magic = GC_GAME_ON_DRIVE;

								DML_GameCount++;
							}
						}
					}
					closedir(s2dir);
				}
			}
		} while (entry);
		if (sdir) closedir(sdir);
		sdir = NULL;
	}
	
	if (strncmp(dm_boot_drive, "sd:", 3) && strncmp(wbfs_fs_drive, "sd:", 3)) {
		// 3th count the number of games on hdd
		char filepath[0xFF];
		sprintf(filepath, "sd:/games");
		dbg_printf("\nSearching GC games in %s ...\n", filepath);
		sdir = opendir(filepath);
		if (sdir) do
		{
			entry = readdir(sdir);
			if (entry)
			{
				sprintf(name_buffer, "sd:/games/%s", entry->d_name);
				if (!strncmp(entry->d_name, ".", 1) || !strncmp(entry->d_name, "..", 2))
				{
					continue;
				}
				s2dir =  opendir(name_buffer);
				if(s2dir)
				{
					sprintf(name_buffer, "sd:/games/%s/game.iso", entry->d_name);
					fp = fopen(name_buffer, "rb");
					if(fp)
					{
						u8 *ptr = ((u8 *)outbuf) + (DML_GameCount * sizeof(struct discHdr));
						struct discHdr *dmlGame = (struct discHdr *)ptr;
						
						struct gc_discHdr header;
						fseek(fp, 0, SEEK_SET);
						fread(&header, 1, sizeof(struct gc_discHdr), fp);
						fclose(fp);
						
						if (header.magic == GC_MAGIC && !getHeaderById(outbuf, DML_GameCount, header.id, header.disc)) {
							memcpy(dmlGame->id, header.id, 6);
							dmlGame->disc = header.disc;
							memcpy(dmlGame->title, header.title, sizeof(dmlGame->title));
							
							sprintf(dmlGame->path, "sd:/games/%s", entry->d_name);
							dmlGame->magic = GC_GAME_ON_DRIVE;

							DML_GameCount++;
						}
					}
					else
					{
						sprintf(name_buffer, "sd:/games/%s/sys/boot.bin", entry->d_name);
						fp = fopen(name_buffer, "rb");
						if(fp)
						{
							u8 *ptr = ((u8 *)outbuf) + (DML_GameCount * sizeof(struct discHdr));
							struct discHdr *dmlGame = (struct discHdr *)ptr;
							
							struct gc_discHdr header;
							fseek(fp, 0, SEEK_SET);
							fread(&header, 1, sizeof(struct gc_discHdr), fp);
							fclose(fp);
							
							if (header.magic == GC_MAGIC && !getHeaderById(outbuf, DML_GameCount, header.id, header.disc)) {
								memcpy(dmlGame->id, header.id, 6);
								dmlGame->disc = header.disc;
								memcpy(dmlGame->title, header.title, sizeof(dmlGame->title));
								
								sprintf(dmlGame->path, "sd:/games/%s", entry->d_name);
								dmlGame->magic = GC_GAME_ON_DRIVE;

								DML_GameCount++;
							}
						}
					}
					closedir(s2dir);
				}
			}
		} while (entry);
		dbg_printf("Closing dir\n");
		if (sdir) closedir(sdir);
		dbg_printf("Dir closed\n");
		sdir = NULL;
	}
	
	return DML_GameCount;
}

s32 __Menu_GetEntries(void)
{
	struct discHdr *buffer = NULL;
	struct discHdr *dmlBuffer = NULL;
	struct discHdr *channelBuffer = NULL;

	u32 cnt = 0, len = 0;
	s32 ret = 0;
	
	s32 dml = 0;
	s32 channelCnt = 0;

	Cache_Invalidate();

	Switch_Favorites(false);
	SAFE_FREE(fav_gameList);
	fav_gameCnt = 0;
	
	SAFE_FREE(filter_gameList);
	filter_gameCnt = 0;

	/* Free memory */
	if (all_gameList) {
		free(all_gameList);
		all_gameList = NULL;
		all_gameCnt = 0;
	}

	/* we just cleared all possable places gameList could be poinying so it must be clear */
	gameList = NULL;
	gameCnt = 0;

	if (MountWBFS) {
		/* Get list length */
		ret = WBFS_GetCount(&cnt);
		if (ret < 0)
			return ret;
	}
		
	/* Allocate memory */
	dmlBuffer = (struct discHdr *)allocate_memory(sizeof(struct discHdr) * 1000);
	if (!dmlBuffer)
		return -1;
	
	/* Clear buffer */
	memset(dmlBuffer, 0, sizeof(struct discHdr) * 1000);
	
	dml = get_DML_game_list(dmlBuffer);
	
	channelCnt = get_channel_list_cnt();
	
	/* Allocate memory */
	channelBuffer = (struct discHdr *)allocate_memory(sizeof(struct discHdr) * channelCnt);
	if (!channelBuffer)
		return -1;
	
	/* Clear buffer */
	memset(channelBuffer, 0, sizeof(struct discHdr) * channelCnt);
	
	s32 channelTemp = get_channel_list(channelBuffer, channelCnt);
	dbg_printf("Channel: %d\n", channelTemp);
	if (channelTemp > 0)
		channelCnt = channelTemp;
	else
		channelCnt = 0;

	/* Buffer length */
	len = sizeof(struct discHdr) * (cnt+dml+channelCnt);
	
	dbg_printf("Found %d games (%d wii games, %d gc games and %d channels)\n", len/sizeof(struct discHdr), cnt, dml, channelCnt);

	/* Allocate memory */
	buffer = (struct discHdr *)allocate_memory(len);
	if (!buffer)
		return -1;

	/* Clear buffer */
	memset(buffer, 0, len);
	
	memcpy(buffer, dmlBuffer, sizeof(struct discHdr) * dml);
	SAFE_FREE(dmlBuffer);
		
	memcpy(buffer+dml, channelBuffer, sizeof(struct discHdr) * channelCnt);
	SAFE_FREE(channelBuffer);

	/* Get header list */
	if (MountWBFS) {
		ret = WBFS_GetHeaders(buffer+(dml+channelCnt), cnt, sizeof(struct discHdr));
		if (ret < 0)
			goto err;
	}

	/* Set values */
	gameList = buffer;
	gameCnt  = cnt+dml+channelCnt;

	/* Reset variables */
	gameSelected = gameStart = 0;

	/* Sort entries */
	__set_default_sort();
	sortList_default();

	// hide and re-sort preffered
	if (!CFG.admin_lock || CFG.admin_mode_locked) {
		gameCnt = CFG_hide_games(gameList, gameCnt);
	}
	CFG_sort_pref(gameList, gameCnt);

	// init favorites
	all_gameList = gameList;
	all_gameCnt  = gameCnt;
	len = sizeof(struct discHdr) * all_gameCnt;
	fav_gameList = (struct discHdr *)allocate_memory(len);
	if (fav_gameList) {
		memcpy(fav_gameList, all_gameList, len);
		fav_gameCnt = all_gameCnt;
	}
	
	filter_gameList = (struct discHdr *)allocate_memory(len);
	if (filter_gameList) {
		memcpy(filter_gameList, all_gameList, len);
		filter_gameCnt = all_gameCnt;
	}
	

	return 0;

err:
	/* Free memory */
	SAFE_FREE(buffer);

	return ret;
}

void Switch_Favorites(bool enable)
{
	int i, len;
	u8 *id = NULL;

	// remember the selected game before we change the list
	if (gameSelected < gameCnt) {
		id = gameList[gameSelected].id;
	}
	// filter
	if (fav_gameList) {
		len = sizeof(struct discHdr) * all_gameCnt;
		memcpy(fav_gameList, all_gameList, len);
		fav_gameCnt = CFG_filter_favorite(fav_gameList, all_gameCnt);
	}
	// switch
	//printf("fav %d %p %d\n", enable, fav_gameList, fav_gameCnt); sleep(5);
	if (fav_gameList == NULL || fav_gameCnt == 0) enable = false;

	if (enable) {
		gameList = fav_gameList;
		gameCnt = fav_gameCnt;
	} else {
		gameList = all_gameList;
		gameCnt = all_gameCnt;
	}
	enable_favorite = enable;
	// find game selected
	gameStart = 0;
	gameSelected = 0;
	if (id) for (i=0; i<gameCnt; i++) {
		if (strncmp((char*)gameList[i].id, (char*)id, 6) == 0) {
			gameSelected = i;
			break;
		}
	}
	// scroll start list
	__Menu_ScrollStartList();
}

char *__Menu_WrapTitle(char *title, int len)
{
	//static char buffer[MAX_CHARACTERS + 4];
	static char buffer[400];
	memset(buffer, 0, sizeof(buffer));
	STRCOPY(buffer, title);
	/* if (len == 0) {
		int rows;
		CON_GetMetrics(&len, &rows);
		len--;
	}*/
	con_wordwrap(buffer, len, sizeof(buffer));
	// limit to 2 lines
	char *ln = strchr(buffer, '\n');
	if (ln) ln = strchr(ln+1, '\n');
	if (ln) *ln = 0;
	return buffer;
}

bool is_dual_layer(u64 real_size)
{
	//return true; // dbg
	u64 single = 4699979776LL;
	if (real_size > single) return true;
	return false;
}

int get_game_ios_idx(struct Game_CFG_2 *game_cfg)
{
	if (game_cfg == NULL) {
		return CURR_IOS_IDX;
	}
	return game_cfg->curr.ios_idx;
}

void warn_ios_bugs()
{
	bool warn = false;
	if (!is_ios_type(IOS_TYPE_WANIN)) {
		CFG.patch_dvd_check = 1;
		return;
	}
	// ios == 249
	if (wbfs_part_fs && IOS_GetRevision() < 18) {
		printf(
			gt("ERROR: cIOS249rev18, cIOS222v4,\n"
			   "cIOS223v4, cIOS224v5 or higher required\n"
			   "for starting games from a FAT partition!\n"
			   "Upgrade IOS249 or choose a different IOS."));
		printf("\n\n");
		warn = true;
	}
	if (IOS_GetRevision() < 14) {
		printf(
			gt("NOTE: CIOS249 before rev14:\n"
			"Possible error #001 is not handled!"));
		printf("\n\n");
		warn = true;
	}
	if (IOS_GetRevision() == 13) {
		printf(
			gt("NOTE: You are using CIOS249 rev13:\n"
			"To quit the game you must reset or\n"
			"power-off the Wii before you start\n"
			"another game or it will hang here!"));
		printf("\n\n");
		warn = true;
	}
	if (IOS_GetRevision() < 12) {
		CFG.patch_dvd_check = 1;
		/*printf(
			"NOTE: CIOS249 before rev12:\n"
			"Any disc in the DVD drive is required!\n\n");
		warn = true;*/
	}
	if (IOS_GetRevision() < 10
			&& wbfsDev == WBFS_DEVICE_SDHC) {
		printf(
			gt("NOTE: CIOS249 before rev10:\n"
			"Loading games from SDHC not supported!"));
		printf("\n\n");
		warn = true;
	}
	if (IOS_GetRevision() < 9
			&& wbfsDev == WBFS_DEVICE_USB) {
		printf(
			gt("NOTE: CIOS249 before rev9:\n"
			"Loading games from USB not supported!"));
		printf("\n\n");
		warn = true;
	}
	if (warn) sleep(1);
}

void print_dual_layer_note()
{
	printf(
		gt("NOTE: You are using CIOS249 rev14:\n"
		"It has known problems with dual layer\n"
		"games. It is highly recommended that\n"
		"you use a different IOS or revision\n"
		"for installation/playback of this game."));
	printf("\n\n");
}

bool check_device(struct Game_CFG_2 *game_cfg, bool print)
{
	int ii = get_game_ios_idx(game_cfg);
	if (wbfs_part_fs == PART_FS_WBFS && wbfs_part_idx > 1) {
		if (!is_ios_idx_mload(ii))
		{
			if (print) {
				printf(
					gt("ERROR: multiple wbfs partitions\n"
					"supported only with IOS222/223-mload"));
				printf("\n\n");
			}
			return false;
		}
	}
	return true;
}

bool check_dual_layer(u64 real_size, struct Game_CFG_2 *game_cfg)
{
	//return true; // dbg
	if (!is_dual_layer(real_size)) return false;
	if (game_cfg == NULL) {
		if (is_ios_type(IOS_TYPE_WANIN) && (IOS_GetRevision() == 14)) return true;
		return false;
	}
	if (get_ios_idx_type(game_cfg->curr.ios_idx) != IOS_TYPE_WANIN) return false; 
	if (is_ios_type(IOS_TYPE_WANIN)) {
	   	if (IOS_GetRevision() == 14) return true;
		return false;
	}
	// we are in 222 but cfg is 249, we can't know revision here
	// so we'll just return false
	return false;
}

void __Menu_GameSize(struct discHdr *header, u64 *comp_size, u64 *real_size)
{
	static u64 last_comp = 0, last_real = 0;
	static u8 last_id[8] = "";
	//f32 size = 0.0;
	*comp_size = 0;
	*real_size = 0;
	
	if (header->magic == CHANNEL_MAGIC) {
		*comp_size = *real_size = getChannelSize(header);
		return;
	}
	if (header->magic == GC_GAME_ON_DRIVE) {
		*comp_size = *real_size = getDMLGameSize(header);
		return;
	}

	/* Get game size */
	if (strncmp((char*)last_id, (char*)header->id, 6) == 0 && last_comp && last_real) {
		*comp_size = last_comp;
		*real_size = last_real;
	} else {
		WBFS_GameSize2(header->id, comp_size, real_size);
		last_comp = *comp_size;
		last_real = *real_size;
		strncpy((char*)last_id, (char*)header->id, 6);
	}
}

void Menu_GameInfoStr2(struct discHdr *header, char *str, u64 comp_size, u64 real_size)
{
	float size = (float)comp_size / 1024.0 / 1024.0 / 1024.0;
	char *s = str;
	sprintf(s, "[%.6s]", header->id);
	s += strlen(s);
	sprintf(s, " %.2f%s", size, gt("GB"));
	if (is_dual_layer(real_size)) {
		s += strlen(s);
		sprintf(s, " %s", "(dual-layer)");
	}
	// req. ios
	tmd game_tmd;
	memset(&game_tmd, 0, sizeof(game_tmd));
	if (header->magic == CHANNEL_MAGIC) {
		game_tmd.sys_version = getChannelReqIos(header); //store channel ios directly in sys_version for compatability
	}
	else if (header->magic == GC_GAME_ON_DRIVE) {
												//do nothing
	} else {
		wbfs_disc_t* d = WBFS_OpenDisc(header->id);
		if (d) {
			size = wbfs_extract_tmd(d, &game_tmd);
			WBFS_CloseDisc(d);
		}
	}
	if (game_tmd.sys_version) {
		s += strlen(s);
		sprintf(s, " IOS%d\n\n", TITLE_LOW(game_tmd.sys_version));
	} else {
		s += strlen(s);
		sprintf(s, "\n\n");
	}
}

void Menu_GameInfoStr(struct discHdr *header, char *str)
{
	u64 comp_size, real_size = 0;
	__Menu_GameSize(header, &comp_size, &real_size);
	Menu_GameInfoStr2(header, str, comp_size, real_size);
}

void __Menu_PrintInfo2(struct discHdr *header, u64 comp_size, u64 real_size)
{
	
	char *title1;
	//if (memcmp("G",header->id,1)==0 )
	//title1 = header->title;
	 //  if (strlen((char *)header->id)>6)
	  // 	title1=strcat(header->title,"(disc2)");
	//else	
	title1 = get_title(header);
	int len = con_len(title1);
	int pad = con_len(CFG.menu_plus_s);
	int cols, rows;
	CON_GetMetrics(&cols, &rows);
	if (pad + len < cols) {
		printf_("%s\n", title1);
	} else {
		printf("%s\n", __Menu_WrapTitle(title1, cols-1));
	}
	char info_str[64];
	Menu_GameInfoStr2(header, info_str, comp_size, real_size);
	printf_("%s", info_str);
}

void __Menu_PrintInfo(struct discHdr *header)
{
	u64 comp_size, real_size = 0;
	__Menu_GameSize(header, &comp_size, &real_size);
	__Menu_PrintInfo2(header, comp_size, real_size);
}

void __Menu_MoveList(s8 delta)
{
	/* No game list */
	if (!gameCnt)
		return;

	#if 0
	/* Select next entry */
	gameSelected += delta;

	/* Out of the list? */
	if (gameSelected <= -1)
		gameSelected = (gameCnt - 1);
	if (gameSelected >= gameCnt)
		gameSelected = 0;
	#endif

	if(delta>0) {
		if(gameSelected == gameCnt - 1) {
			gameSelected = 0;
		}
		else {
			gameSelected +=delta;
			if(gameSelected >= gameCnt) {
				gameSelected = (gameCnt - 1);
			}
		}
	}
	else {
		if(!gameSelected) {
			gameSelected = gameCnt - 1;
		}
		else {
			gameSelected +=delta;
			if(gameSelected < 0) {
				gameSelected = 0;
			}
		}
	}

	/* List scrolling */
	__Menu_ScrollStartList();
}

void __Menu_ScrollStartList()
{
	s32 index = (gameSelected - gameStart);

	if (index >= ENTRIES_PER_PAGE)
		gameStart += index - (ENTRIES_PER_PAGE - 1);
	if (index <= -1)
		gameStart += index;
}

void __Menu_ShowList(void)
{
	FgColor(CFG.color_header);
	if (enable_favorite) {
		printf_x(gt("Favorite Games"));
		printf(":\n");
	} else {
		if (!CFG.hide_header) {
			printf_x(gt("Select the game you want to boot"));
			printf(":\n");
		}
	}
	DefaultColor();
	if (CFG.console_mark_page && gameStart > 0) {
		printf(" %s +", CFG.cursor_space);
	}
	printf("\n");

	/* No game list*/
	if (gameCnt) {
		u32 cnt;

		/* Print game list */
		for (cnt = gameStart; cnt < gameCnt; cnt++) {
			struct discHdr *header = &gameList[cnt];

			/* Entries per page limit reached */
			if ((cnt - gameStart) >= ENTRIES_PER_PAGE)
				break;

			if (gameSelected == cnt) {
				FgColor(CFG.color_selected_fg);
				BgColor(CFG.color_selected_bg);
				Con_ClearLine();
			} else {
				DefaultColor();
			}

			/* Print entry */
			//printf(" %2s %s\n", (gameSelected == cnt) ? ">>" : "  ",
			char *title = __Menu_PrintTitle(get_title(header));
			// cursor
			printf(" %s", (gameSelected == cnt) ? CFG.cursor : CFG.cursor_space);
			// favorite mark
			printf("%s", (CFG.console_mark_favorite && is_favorite(header->id))
					? CFG.favorite : " ");
			// title
		//	if (memcmp("G",header->id,1)==0 )
		//	printf("%s",header->title);
		
		//		else
			printf("%s",title);
		  
			// saved mark
			if (CFG.console_mark_saved) {
				printf("%*s", (MAX_CHARACTERS - con_len(title)),
					(CFG_is_saved(header->id)) ? CFG.saved : " ");
			}
			printf("\n");
		}
		DefaultColor();
		if (CFG.console_mark_page && cnt < gameCnt) {
			printf(" %s +", CFG.cursor_space);
		} else {
			printf(" %s  ", CFG.cursor_space);
		}
		//if (CFG.hide_hddinfo) {
		FgColor(CFG.color_footer);
		BgColor(CONSOLE_BG_COLOR);
		int num_page = 1 + (gameCnt - 1) / ENTRIES_PER_PAGE;
		int cur_page = 1 + gameSelected / ENTRIES_PER_PAGE;
		printf(" %-*.*s %d/%d", MAX_CHARACTERS - 8, MAX_CHARACTERS - 8, action_string, cur_page, num_page);
		if (!CFG.hide_hddinfo) printf("\n");
		action_string[0] = 0;
		//}
	} else {
		printf(" %s ", CFG.cursor);
		printf("%s", gt("No games found!"));
		printf("\n");
	}

	/* Print free/used space */
	FgColor(CFG.color_footer);
	BgColor(CONSOLE_BG_COLOR);
	if (!CFG.hide_hddinfo) {
		// Get free space
		f32 free, used;
		//printf("\n");
		WBFS_DiskSpace(&used, &free);
		printf_x(gt("Used: %.1fGB Free: %.1fGB [%d]"), used, free, gameCnt);
		//int num_page = 1 + (gameCnt - 1) / ENTRIES_PER_PAGE;
		//int cur_page = 1 + gameSelected / ENTRIES_PER_PAGE;
		//printf(" %d/%d", cur_page, num_page);
	}
	if (!CFG.hide_footer) {
		printf("\n");
		// (B) GUI (1) Options (2) Favorites
		// B: GUI 1: Options 2: Favorites
		//char c_gui = 'B', c_opt = '1';
		//if (CFG.buttons == CFG_BTN_OPTIONS_B) {
		//	c_gui = '1'; c_opt = 'B';
		//}
		printf_("");
		if (CFG.gui && CFG.button_gui) {
			printf(gt("%s: GUI "), (button_names[CFG.button_gui]));
		}
		if (!CFG.disable_options && CFG.button_opt) {
			printf(gt("%s: Options "), (button_names[CFG.button_opt]));
		}
		if (CFG.button_fav) {
			printf(gt("%s: Favorites"), (button_names[CFG.button_fav]));
		}
	}
	if (CFG.db_show_info) {
		printf("\n");
		//load game info from XML - lustar
		__Menu_ShowGameInfo(false, gameList[gameSelected].id);
	}
	DefaultColor();
	__console_flush(0);
}

/* load game info from XML - lustar */
void __Menu_ShowGameInfo(bool showfullinfo, u8 *id)
{
	if (LoadGameInfoFromXML(id)) {
		FgColor(CFG.color_inactive);
		PrintGameInfo();
		if (showfullinfo) {
			printf("\n");
			PrintGameSynopsis();
		}
		//printf("Play Count: %d\n", getPlayCount(id));
		DefaultColor();
	}
}

void __Menu_ShowCover(void)
{
	struct discHdr *header = &gameList[gameSelected];

	/* No game list*/
	if (!gameCnt)
		return;

	/* Draw cover */
	Gui_DrawCover(header->id);
}

bool Save_ScreenShot(char *fn, int size)
{
	int i;
	struct stat st;
	for (i=1; i<100; i++) {
		snprintf(fn, size, "%s/screenshot%d.png", USBLOADER_PATH, i);
		if (stat(fn, &st)) break;
	}
	return ScreenShot(fn);
}

void Make_ScreenShot()
{
	bool ret;
	char fn[200];
	ret = Save_ScreenShot(fn, sizeof(fn));
	printf("\n%s: %s\n", ret ? "Saved" : "Error Saving", fn);
	sleep(1);
}

void Handle_Home()
{
	if (CFG.home == CFG_HOME_EXIT) {
		Con_Clear();
		printf("\n");
		printf_("Exiting...");
		__console_flush(0);
		Sys_Exit();
	} else if (CFG.home == CFG_HOME_SCRSHOT) {
		long long t1 = gettime();
		while (Wpad_HeldButtons() & CFG.button_exit.mask) {
			if (diff_msec(t1, gettime()) >= 1000) {
				__console_flush(0);
				Make_ScreenShot();
				return;
			}
			VIDEO_WaitVSync();
		}
		// loop ended before timeout - run hbc
		goto do_hbc;
	} else if (CFG.home == CFG_HOME_HBC && CFG.disable_options == 0) {
		do_hbc:
		Con_Clear();
		printf("\n");
		printf_("HBC...");
		__console_flush(0);
		Sys_HBC();
	} else if (CFG.home == CFG_HOME_REBOOT) { 
		Con_Clear();
		Restart();
	} else {
		// Priiloader magic words, and channels
		if ((CFG.home & 0xFF) < 'a') {
			// upper case final letter implies channel
			Con_Clear();
			Sys_Channel(CFG.home);
		} else {
			// lower case final letter implies magic word
			Con_Clear();
			*(vu32*)0x8132FFFB = CFG.home;
			Restart();
		}
	}
}

void Print_IOS_Info_str(char *str, int size)
{
	*str = 0;
	//int new_wanin = is_ios_type(IOS_TYPE_WANIN) && IOS_GetRevision() >= 18;
	snprintf(str, size, "IOS%u (r%u)\n",
			IOS_GetVersion(), IOS_GetRevision());
			//CFG.ios_mload||new_wanin ? "[FRAG]" : "");
	str_seek_end(&str, &size);
	print_mload_version_str(str);
}

void Print_IOS_Info()
{
	char str[200];
	FgColor(CFG.color_inactive);
	Print_IOS_Info_str(str, sizeof(str));
	printf_("%s", str);
	DefaultColor();
}

void Print_SYS_Info_str(char *str, int size, bool console)
{
	snprintf(str, size, gt("Loader Version: %s"), CFG_VERSION);
	str_seek_end(&str, &size);
	snprintf(str, size, "\n");
	str_seek_end(&str, &size);
	snprintf(str, size, gt("CFG base: %s"), USBLOADER_PATH);
	str_seek_end(&str, &size);
	snprintf(str, size, "\n");
	str_seek_end(&str, &size);
	if (strcmp(LAST_CFG_PATH, USBLOADER_PATH)) {
		// if last cfg differs, print it out
		snprintf(str, size, "%s\n", gt("Additional config:"));
		str_seek_end(&str, &size);
		snprintf(str, size, "  %s/config.txt\n", LAST_CFG_PATH);
		str_seek_end(&str, &size);
	}
	MountPrint_str(str, size);
	str_seek_end(&str, &size);
	Print_IOS_Info_str(str, size);
	
	if (!console) {
		snprintf(str, size, "\n");
		str_seek_end(&str, &size);
		
		char devopath[200];
		snprintf(devopath, sizeof(devopath), "%s/%s", USBLOADER_PATH, "loader.bin");
		
		FILE *f = NULL;
		f = fopen(devopath, "rb");
		if (f) {
			char buffer[72];
			u32 i = 0;
			fseek(f, 4, SEEK_SET);
			fread(&buffer, 1, sizeof(buffer), f);
			fclose(f);
			for (i = 0; i < sizeof(buffer); i++) {
				if (buffer[i] == '/' && i-5 > 0) {
					buffer[i-5] = '\n';
					break;
				}
			}
			snprintf(str, size, buffer);
			str_seek_end(&str, &size);
			snprintf(str, size, "\n");
			str_seek_end(&str, &size);
		}
		if (CFG.dml > 0) {
			snprintf(str, size, DIOS_MIOS_INFO);
			str_seek_end(&str, &size);
		}
		snprintf(str, size, gt("NAND Emu Path:\n%s\n"), CFG.nand_emu_path);
		str_seek_end(&str, &size);
	}
}

void Print_SYS_Info()
{
	char str[400];
	FgColor(CFG.color_inactive);
	Print_SYS_Info_str(str, sizeof(str), true);
	printf_("%s", str);
	DefaultColor();
}

// WiiMote to char map for admin mode unlocking
char get_unlock_buttons(buttons)
{
	switch (buttons)
	{
		case WPAD_BUTTON_UP:	return 'U';
		case WPAD_BUTTON_DOWN:	return 'D';
		case WPAD_BUTTON_LEFT:	return 'L';
		case WPAD_BUTTON_RIGHT:	return 'R';
		case WPAD_BUTTON_A:	return 'A';
		case WPAD_BUTTON_B:	return 'B';
		case WPAD_BUTTON_MINUS:	return 'M';
		case WPAD_BUTTON_PLUS:	return 'P';
		case WPAD_BUTTON_1:	return '1';
		case WPAD_BUTTON_2:	return '2';
		case WPAD_BUTTON_HOME:	return 'H';
	}
	return 'x';
}

void Admin_Unlock(bool unlock)
{
	if (unlock_init) {
		// save original settings
		disable_format = CFG.disable_format;
		disable_remove = CFG.disable_remove;
		disable_install = CFG.disable_install;
		disable_options = CFG.disable_options;
		confirm_start = CFG.confirm_start;
		unlock_init = false;
	}
	if (unlock) {
		// unlock
		//enable all "admin-type" screens
		CFG.disable_format = 0;
		CFG.disable_remove = 0;
		CFG.disable_install = 0;
		CFG.disable_options = 0;
		CFG.confirm_start = 1;
		CFG.admin_mode_locked = 0;
	} else {
		//set the lock back on
		CFG.disable_format = disable_format;
		CFG.disable_remove = disable_remove;
		CFG.disable_install = disable_install;
		CFG.disable_options = disable_options;
		CFG.confirm_start = confirm_start;
		CFG.admin_mode_locked = 1;
	}
	// reset the hidden games
	__Menu_GetEntries();
}

void Menu_Unlock() {
	u32 buttons;
	static long long t_start;
	long long t_now;
	unsigned ms_diff = 0;
	int i = 0;
	char buf[16];
	bool unlocked = false;
	memset(buf, 0, sizeof(buf));

	Con_Clear();
	printf(gt("Configurable Loader %s"), CFG_VERSION);
	printf("\n\n");

	if (CFG.admin_mode_locked) {
		printf(gt("Enter Code: "));
		t_start = gettime();
		
		while (ms_diff < 30000 && !unlocked) {
			buttons = Wpad_GetButtons();
			if (buttons) {
				printf("*");
				buf[i] = get_unlock_buttons(buttons);
				i++;
				if (stricmp(buf, CFG.unlock_password) == 0) {
					unlocked = true;
				}
				if (i >= 10) break;
			}
			VIDEO_WaitVSync();
			t_now = gettime();
			ms_diff = diff_msec(t_start, t_now);
		}
	}
	Admin_Unlock(unlocked);
	printf("\n\n");
	if (unlocked) {
		printf(gt("SUCCESS!"));
	} else {
		//set the lock back on
		printf(gt("LOCKED!"));
	}
	sleep(1);
}


int Menu_Views()
{
	struct discHdr *header = NULL;
	int redraw_cover = 0;
	struct Menu menu;

	if (gameCnt) {
		header = &gameList[gameSelected];
	}
	
	const int NUM_OPT = 8;
	char active[NUM_OPT];
	menu_init(&menu, NUM_OPT);

	for (;;) {

		menu_begin(&menu);
		menu_init_active(&menu, active, sizeof(active));

		if (CFG.disable_options) {
			active[1] = 0;
			active[2] = 0;
		}
		if (CFG.disable_remove) active[3] = 0;
		if (CFG.disable_install) active[4] = 0;
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Main Menu"));
		printf(":\n\n");
		
		DefaultColor();
		MENU_MARK();
		printf("<%s>\n", gt("Start Game"));
		MENU_MARK();
		printf("<%s>\n", gt("Game Options"));
		MENU_MARK();
		printf("<%s>\n", gt("Global Options"));
		MENU_MARK();
		printf("<%s>\n", gt("Delete Game"));
		MENU_MARK();
		printf("<%s>\n", gt("Install Game"));
		MENU_MARK();
		printf("<%s>\n", gt("Sort Games"));
		MENU_MARK();
		printf("<%s>\n", gt("Filter Games"));
		MENU_MARK();
		printf("<%s>\n", gt("Boot Disc"));
   
		DefaultColor();

		printf("\n");
		printf_h(gt("Press %s button to select."),
				(button_names[CFG.button_confirm.num]));
		printf("\n");
		DefaultColor();
		__console_flush(0);

		if (redraw_cover) {
			if (header) Gui_DrawCover(header->id);
			redraw_cover = 0;
		}
		
		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move_active(&menu, buttons);
		
		int change = -2;
		if (buttons & WPAD_BUTTON_LEFT) change = -1;
		if (buttons & WPAD_BUTTON_RIGHT) change = +1;
		if (buttons & CFG.button_confirm.mask) change = 0;
//		#define CHANGE(V,M) {V+=change;if(V>M)V=M;if(V<0)V=0;}

		if (change > -2) {
			switch (menu.current) {
			case 0:
				CFG.confirm_start = 0;
				Menu_Boot(0);
				break;
			case 1:
				Menu_Game_Options();
				break;
			case 2:
				Menu_Global_Options();
				break;
			case 3:
				Menu_Remove();
				break;
			case 4:
				Menu_Install();
				break;
			case 5:
				Menu_Sort();
				break;
			case 6:
				Menu_Filter();
				break;
			case 7:
				Menu_Boot(1);
				break;
			}
		}

		// HOME button
		if (buttons & CFG.button_exit.mask) {
			Handle_Home();
		}
		if (buttons & CFG.button_cancel.mask) break;
	}
	
	return 0;
}

int Menu_Game_Options() {
	return Menu_Boot_Options(&gameList[gameSelected], 0);
}

void Menu_Alt_Dol(struct discHdr *header, struct Game_CFG *game_cfg, int allow_back)
{
	int i;
	DOL_LIST dol_list;
	struct Menu menu;
	int num_opt;
	int rows, cols, win_size;
	CON_GetMetrics(&cols, &rows);
	if ((win_size = rows-5) < 3) win_size = 3;

	f32 size = 0.0;

	WBFS_GameSize(header->id, &size);
	WBFS_GetDolList(header->id, &dol_list);
	load_dolmenu((char*)header->id);
	int dm_num = dolmenubuffer ? dolmenubuffer[0].count : 0;
	int dm_count = dm_num ? 1+dm_num : 0;
	num_opt = ALT_DOL_DISC + 1 + dol_list.num + dm_count;
	char active[num_opt];
	
	menu_init(&menu, num_opt);
	
	// find out current
	if (game_cfg->alt_dol < ALT_DOL_DISC) {
		menu.current = game_cfg->alt_dol;
	} else if (game_cfg->alt_dol == ALT_DOL_DISC) {
		if (allow_back)
			menu.current = ALT_DOL_DISC;
		else if (dm_num)
			menu.current = ALT_DOL_DISC + 2 + dol_list.num;
		else if (dol_list.num)
			menu.current = ALT_DOL_DISC + 1;
		else
			menu.current = 0;

		for (i=0; i<dol_list.num; i++) {
			if (strcmp(game_cfg->dol_name, dol_list.name[i]) == 0) {
				menu.current = ALT_DOL_DISC + 1 + i;
				break;
			}
		}
	} else {
		// alt.dol plus
		menu.current = dol_list.num + game_cfg->alt_dol + 1;
	}

	for (;;) {

		menu_init_active(&menu, active, sizeof(active));
		active[ALT_DOL_DISC + 1 + dol_list.num] = 0;
		if (!allow_back) active[ALT_DOL_DISC] = 0;
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Selected Game"));
		printf(":");
		printf(" (%.6s) (%.2f%s)\n", header->id, size, gt("GB"));
		DefaultColor();
		printf(" %s %s\n", CFG.cursor_space, __Menu_PrintTitle(get_title(header)));
		FgColor(CFG.color_header);
		printf_x(gt("Launch Methods"));
		printf(":\n");
		DefaultColor();

		menu_begin(&menu);
		menu_window_begin(&menu, win_size, num_opt);
		if (menu_window_mark(&menu)) {
			if (allow_back)
				printf("%s\n", gt("Off"));
			else
				printf("main.dol\n");
		}
		if (menu_window_mark(&menu))
			printf("SD\n");
		if (menu_window_mark(&menu)) {
			if (allow_back)
				printf("%s\n", gt("Disc (Ask)"));
			else
				printf("%s\n", gt ("Disc"));
		}
		for (i=0; i<dol_list.num; i++) {
			if (menu_window_mark(&menu))
				printf("  %s\n", dol_list.name[i]);
		}
		DefaultColor();
		if (dm_num) {
			if (menu_window_mark(&menu))
				printf("Disc Plus:\n");
			for (i=0; i<dm_num; i++) {
				if (menu_window_mark(&menu))
					printf("  %s\n", dolmenubuffer[i+1].name);
			}
		}
		DefaultColor();
		menu_window_end(&menu, cols);
		printf_h(gt("Press %s button to select."),
				(button_names[CFG.button_confirm.num]));
		if (allow_back) {
			printf("\n");
			printf_h(gt("Press %s button to go back."),
					(button_names[CFG.button_cancel.num]));
		}
		__console_flush(0);

		u32 buttons = Wpad_WaitButtonsRpt();
		int change = 0;

		menu_move_active(&menu, buttons);

		if (buttons & CFG.button_exit.mask) Handle_Home();
		if ((buttons & CFG.button_cancel.mask) && allow_back) return;
		if (buttons & CFG.button_confirm.mask) change = 1;
//		if (buttons & WPAD_BUTTON_LEFT) change = 1;
//		if (buttons & WPAD_BUTTON_RIGHT) change = 1;
		if (change) {
			if (menu.current <= ALT_DOL_DISC) {
				game_cfg->alt_dol = menu.current;
				*game_cfg->dol_name = 0;
				return;
			}
			i = menu.current - ALT_DOL_DISC -1;
			if (i < dol_list.num) {
				game_cfg->alt_dol = ALT_DOL_DISC;
				STRCOPY(game_cfg->dol_name, dol_list.name[i]);
				//printf("DN: %s\n", game_cfg->dol_name); sleep(3);
				return;
			}
			i = menu.current - ALT_DOL_PLUS - dol_list.num - 1;
			game_cfg->alt_dol = ALT_DOL_PLUS + i;
			STRCOPY(game_cfg->dol_name, dolmenubuffer[i+1].dolname);
			return;
		}
	}
}

int Menu_Boot_Options(struct discHdr *header, bool disc) {

	int ret_val = 0;
	if (CFG.disable_options) return 0;
	if (!gameCnt && !disc) {
		// if no games, go directly to global menu
		return 1;
	}

	struct Game_CFG_2 *game_cfg2 = NULL;
	struct Game_CFG *game_cfg = NULL;
	int opt_saved;
	//int opt_ios_reload;
	int opt_language, opt_video, opt_video_patch, opt_vidtv, opt_padhook, opt_nand_emu;
	int opt_country_patch, opt_anti_002, opt_ocarina, opt_wide_screen, opt_nodisc, opt_ntsc_j_patch, opt_screenshot; 
	f32 size = 0.0;
	int redraw_cover = 0;
	int i;
	int rows, cols, win_size;
	CON_GetMetrics(&cols, &rows);
	if ((win_size = rows-9) < 3) win_size = 3;
	Con_Clear();
	FgColor(CFG.color_header);
	printf_x(gt("Selected Game"));
	printf(":");
	__console_flush(0);
	if (disc) {
		printf(" (%.6s)\n", header->id);
	} else {
		if (header->magic == GC_GAME_ON_DRIVE)
		{
			u64 tempSize = getDMLGameSize(header);
			if (tempSize > 0) {
				size = (float)tempSize / 1024.0 / 1024.0 / 1024.0;
			}
		}
		else if (header->magic == CHANNEL_MAGIC) {
			size = (float)getChannelSize(header) / 1024.0 / 1024.0 / 1024.0;
		}
		else 
		{
			WBFS_GameSize(header->id, &size);
		}
		printf(" (%.6s) (%.2f%s)\n", header->id, size, gt("GB"));
	}
	DefaultColor();
	printf(" %s %s\n\n", CFG.cursor_space, __Menu_PrintTitle(get_title(header)));
	__console_flush(0);
	load_dolmenu((char*)header->id);

	game_cfg2 = CFG_get_game(header->id);
	if (!game_cfg2) {
		printf(gt("ERROR game opt"));
		printf("\n");
		sleep(5);
		return 0;
	}
	game_cfg = &game_cfg2->curr;

	struct Menu menu;
	int NUM_OPT = 19;
	if (header->magic == GC_GAME_ON_DRIVE) NUM_OPT = 16;
	if (header->magic == CHANNEL_MAGIC) NUM_OPT = 19;
	char active[NUM_OPT];
	menu_init(&menu, NUM_OPT);

	for (;;) {
		/*
		// fat on 249?
		if (wbfs_part_fs && !disc) {
			if (!is_ios_idx_mload(game_cfg->ios_idx))
			{
				game_cfg->ios_idx = CFG_IOS_222_MLOAD;
			}
		}
		*/

		menu_init_active(&menu, active, sizeof(active));
		opt_saved = game_cfg2->is_saved;
		// block ios reload is supported with hermes and d2x
		// old: if not mload disable block ios reload opt
		/*
		opt_ios_reload = game_cfg->block_ios_reload;
		if (!is_ios_idx_mload(game_cfg->ios_idx)) {
			active[8] = 0;
			opt_ios_reload = 0;
		}
		*/
		// clean options
		opt_language = game_cfg->language;
		opt_video = game_cfg->video;
		opt_video_patch = game_cfg->video_patch;
		opt_vidtv = game_cfg->vidtv;
		opt_country_patch = game_cfg->country_patch;
		opt_anti_002 = game_cfg->fix_002;
		opt_wide_screen = game_cfg->wide_screen;
		opt_nodisc = game_cfg->nodisc;
		opt_screenshot = game_cfg->screenshot;
		opt_padhook = game_cfg->hooktype;
	 	opt_ocarina = game_cfg->ocarina;
		opt_ntsc_j_patch = game_cfg->ntsc_j_patch;
		opt_nand_emu = game_cfg->nand_emu;

		if (game_cfg->clean == CFG_CLEAN_ALL) {
			opt_language = CFG_LANG_CONSOLE;
			opt_video = CFG_VIDEO_GAME;
			opt_video_patch = CFG_VIDEO_PATCH_OFF;
			opt_vidtv = 0;
			opt_country_patch = 0;
			opt_anti_002 = 0;
			opt_ocarina = 0;
			opt_ntsc_j_patch = 0;
			opt_nand_emu = 0;
			opt_nodisc = 0;
			opt_screenshot = 0;
			active[1] = 0; // language
			active[2] = 0; // video
			active[3] = 0; // video_patch
			active[4] = 0; // vidtv
			active[5] = 0; // country
			active[6] = 0; // anti 002
			active[10] = 0; // ocarina
			active[11] = 0; // hook
		}
		
		// if not ocarina and not wiird, deactivate hooks
		//if (!game_cfg->ocarina && !CFG.wiird) {
		//	active[11] = 0;
		//}
		//if admin lock is off or they're not in admin 
		// mode then they can't hide/unhide games
	//	if (!CFG.admin_lock || CFG.admin_mode_locked) {
	//		active[14] = 0;
	//	}

		//These things shouldn't be changed if using a disc...maybe
	//	if ((disc) && memcmp("G",header->id,1)==0) {
			//active[3] = 0;
	//	 	active[8] = 0;
			//active[9] = 0;
			//active[10] = 0;
	//	}
		
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Selected Game"));
		printf(":");
		if (disc) printf(" (%.6s)\n", header->id);
		else printf(" (%.6s) (%.2fGB)\n", header->id, size);
		DefaultColor();
		printf(" %s %s\n\n", CFG.cursor_space, __Menu_PrintTitle(get_title(header)));
		FgColor(CFG.color_header);
		printf_x(gt("Game Options:  %s"),
				CFG_is_changed(header->id) ? gt("[ CHANGED ]") :
				opt_saved ? gt("[ SAVED ]") : "");
		printf("\n");
		DefaultColor();
		char c1 = '<', c2 = '>';
		//if (opt_saved) { c1 = '['; c2 = ']'; }
		const char *str_alt_dol = gt("Off");
		if (!disc) {
			if (game_cfg->alt_dol == ALT_DOL_OFF) {
				str_alt_dol = gt("Off");
			} else if (game_cfg->alt_dol == ALT_DOL_SD) {
				str_alt_dol = "SD";
			} else if (game_cfg->alt_dol == ALT_DOL_DISC) {
				str_alt_dol = "Disc";
				if (*game_cfg->dol_name) str_alt_dol = game_cfg->dol_name;
			} else {
				// alt.dol plus
				i = game_cfg->alt_dol - ALT_DOL_PLUS;
				str_alt_dol = dolmenubuffer[i+1].name;
			}
		}

		// start menu draw

		menu_begin(&menu);
		menu_jump_active(&menu);

		#define PRINT_OPT_S(N,V) \
			printf("%s%c %s %c\n", con_align(N,18), c1, V, c2)

		#define PRINT_OPT_A(N,V) \
			printf("%s%c%s%c\n", con_align(N,18), c1, V, c2)

		#define PRINT_OPT_B(N,V) \
			PRINT_OPT_S(N,(V?gt("On"):gt("Off"))) 
			
		if (header->magic == GC_GAME_ON_DRIVE) {
			int num_gc_boot = map_get_num(map_gc_boot);
			char *names_gc_boot[num_gc_boot];
			num_gc_boot = map_to_list(map_gc_boot, num_gc_boot, names_gc_boot);

			menu_window_begin(&menu, win_size, NUM_OPT);
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Favorite:"), is_favorite(header->id) ? gt("Yes") : gt("No"));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Language:"), gt(languages[opt_language]));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Video:"), gt(DML_videos[opt_video]));
			if (menu_window_mark(&menu)) {
				if (game_cfg->block_ios_reload == 2) {
					PRINT_OPT_S(gt("Video Patch:"), gt("Auto"));
				} else {	
					PRINT_OPT_B(gt("Video Patch:"), game_cfg->block_ios_reload);	
				}
			}
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("LED:"), opt_vidtv);
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("NMM:"), opt_country_patch);
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("Ocarina (cheats):"), opt_ocarina);
			if (menu_window_mark(&menu))
				PRINT_OPT_A(gt("Cheat Codes:"), gt("Manage"));
			if (menu_window_mark(&menu))
				printf("%s%s\n", con_align(gt("Cover Image:"), 18), 
					imageNotFound ? gt("< DOWNLOAD >") : gt("[ FOUND ]"));
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("Wide Screen:"), opt_wide_screen);
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("NTSC-J patch:"), opt_ntsc_j_patch);
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("NoDisc:"), opt_nodisc);	
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("PAD HOOK:"), opt_padhook);				
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Boot method:"), gt(names_gc_boot[game_cfg->channel_boot]));
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("Screenshot:"), opt_screenshot);
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("Alt Button Cfg:"), game_cfg->alt_controller_cfg);
				/*
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Write Playlog:"), gt(playlog_name[game_cfg->write_playlog]));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Clear Patches:"), gt(names_vpatch[game_cfg->clean]));
				*/
		} else if (header->magic == CHANNEL_MAGIC) {
			int num_channel_boot = map_get_num(map_channel_boot);
			char *names_channel_boot[num_channel_boot];
			num_channel_boot = map_to_list(map_channel_boot, num_channel_boot, names_channel_boot);
			
			menu_window_begin(&menu, win_size, NUM_OPT);
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Favorite:"), is_favorite(header->id) ? gt("Yes") : gt("No"));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Language:"), gt(languages[opt_language]));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Video:"), gt(videos[opt_video]));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Video Patch:"), gt(names_vpatch[opt_video_patch]));
			if (menu_window_mark(&menu))
				PRINT_OPT_B("VIDTV:", opt_vidtv);
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("Country Fix:"), opt_country_patch);
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("Anti 002 Fix:"), opt_anti_002);
			if (menu_window_mark(&menu))
				PRINT_OPT_S("IOS:", ios_str(game_cfg->ios_idx));
			if (menu_window_mark(&menu)) {
				if (game_cfg->block_ios_reload == 2) {
					PRINT_OPT_S(gt("Block IOS Reload:"), gt("Auto"));
				} else {
					PRINT_OPT_B(gt("Block IOS Reload:"), game_cfg->block_ios_reload);
				}
			}
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Alt dol:"), str_alt_dol);
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("Ocarina (cheats):"), opt_ocarina);
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Hook Type:"), hook_name[game_cfg->hooktype]);
			if (menu_window_mark(&menu))
				PRINT_OPT_A(gt("Cheat Codes:"), gt("Manage"));
			if (menu_window_mark(&menu))
				printf("%s%s\n", con_align(gt("Cover Image:"), 18), 
					imageNotFound ? gt("< DOWNLOAD >") : gt("[ FOUND ]"));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Hide Game:"), is_hide_game(header->id) ? gt("Yes") : gt("No"));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Write Playlog:"), gt(playlog_name[game_cfg->write_playlog]));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Clear Patches:"), gt(names_vpatch[game_cfg->clean]));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Plugin:"), gt(names_channel_boot[game_cfg->channel_boot]));
			if (menu_window_mark(&menu))
				PRINT_OPT_A(gt("Savegame:"), gt("Dump savegame"));
		} else {
			menu_window_begin(&menu, win_size, NUM_OPT);
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Favorite:"), is_favorite(header->id) ? gt("Yes") : gt("No"));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Language:"), gt(languages[opt_language]));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Video:"), gt(videos[opt_video]));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Video Patch:"), gt(names_vpatch[opt_video_patch]));
			if (menu_window_mark(&menu))
				PRINT_OPT_B("VIDTV:", opt_vidtv);
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("Country Fix:"), opt_country_patch);
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("Anti 002 Fix:"), opt_anti_002);
			if (menu_window_mark(&menu))
				PRINT_OPT_S("IOS:", ios_str(game_cfg->ios_idx));
			if (menu_window_mark(&menu)) {
				if (game_cfg->block_ios_reload == 2) {
					PRINT_OPT_S(gt("Block IOS Reload:"), gt("Auto"));
				} else {
					PRINT_OPT_B(gt("Block IOS Reload:"), game_cfg->block_ios_reload);
				}
			}
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Alt dol:"), str_alt_dol);
			if (menu_window_mark(&menu))
				PRINT_OPT_B(gt("Ocarina (cheats):"), opt_ocarina);
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Hook Type:"), hook_name[game_cfg->hooktype]);
			if (menu_window_mark(&menu))
				PRINT_OPT_A(gt("Cheat Codes:"), gt("Manage"));
			if (menu_window_mark(&menu))
				printf("%s%s\n", con_align(gt("Cover Image:"), 18), 
					imageNotFound ? gt("< DOWNLOAD >") : gt("[ FOUND ]"));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Hide Game:"), is_hide_game(header->id) ? gt("Yes") : gt("No"));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Write Playlog:"), gt(playlog_name[game_cfg->write_playlog]));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("Clear Patches:"), gt(names_vpatch[game_cfg->clean]));
			if (menu_window_mark(&menu))
				PRINT_OPT_S(gt("NAND Emu:"), gt(str_nand_emu[opt_nand_emu]));
			if (menu_window_mark(&menu))
				PRINT_OPT_A(gt("Savegame:"), gt("Dump savegame"));
		}
		
		DefaultColor();
		menu_window_end(&menu, cols);

		printf_h(gt("Press %s to start game"), (button_names[CFG.button_confirm.num]));
		printf("\n");
		bool need_save = !opt_saved || CFG_is_changed(header->id);
		if (need_save)
			printf_h(gt("Press %s to save options"), (button_names[CFG.button_save.num]));
		else
			printf_h(gt("Press %s to discard options"), (button_names[CFG.button_save.num]));
		printf("\n");
		printf_h(gt("Press %s for global options"), (button_names[CFG.button_other.num]));
		DefaultColor();
		__console_flush(0);

		if (redraw_cover) {
			Gui_DrawCover(header->id);
			redraw_cover = 0;
		}
		
		u32 buttons = Wpad_WaitButtonsRpt();
		int change = 0;

		menu_move_active(&menu, buttons);

		if (buttons & WPAD_BUTTON_LEFT) change = -1;
		if (buttons & WPAD_BUTTON_RIGHT) change = +1;

		if (change && ((header->magic == GC_GAME_ON_DRIVE) || header->magic == GC_MAGIC)) {
			switch (menu.current) {
			case 0:
				printf("\n\n");
				printf_x(gt("Saving Settings... "));
				__console_flush(0);
				if (set_favorite(header->id, change > 0)) {
					printf(gt("OK"));
				} else {
					printf(gt("ERROR"));
					sleep(1);
				}
				__console_flush(0);
				Gui_DrawCover(header->id);
				break;
			case 1:
				CHANGE(game_cfg->language, CFG_LANG_MAX);
				break;
			case 2:
				CHANGE(game_cfg->video, 5);
				break;
	    	case 3:
				CHANGE(game_cfg->block_ios_reload, 2);
				break;	
			case 4:
				CHANGE(game_cfg->vidtv, 1);
				break;
			case 5:
				CHANGE(game_cfg->country_patch, 1);
				break;
			case 6:
				CHANGE(game_cfg->ocarina, 1);
				break;
			case 7:
				Menu_Cheats(header);
				break;
			case 8:
				printf("\n\n");
				Download_Cover((char*)header->id, change > 0, true);
				Cache_Invalidate();
				Gui_DrawCover(header->id);
				Menu_PrintWait();
				break;
			case 9: // Wide Screen
				CHANGE(game_cfg->wide_screen, 1);
				break;
			case 10: // NoDisc+
				CHANGE(game_cfg->ntsc_j_patch, 1);
				break;
			case 11: // NoDisc+
				CHANGE(game_cfg->nodisc, 1);
				break;
			case 12: // PAD HOOK)
				CHANGE(game_cfg->hooktype, 1);
				break;	
			case 13: // gc Boot Method
				CHANGE(game_cfg->channel_boot, 3);
				break;
			case 14: // Screenshot
				CHANGE(game_cfg->screenshot, 1);
				break;
			case 15: // Alt Button Cfg
				CHANGE(game_cfg->alt_controller_cfg, 1);
				break;
			}
		} else if (change && (header->magic == CHANNEL_MAGIC)) {
			switch (menu.current) {
			case 0:
				printf("\n\n");
				printf_x(gt("Saving Settings... "));
				__console_flush(0);
				if (set_favorite(header->id, change > 0)) {
					printf(gt("OK"));
				} else {
					printf(gt("ERROR"));
					sleep(1);
				}
				__console_flush(0);
				Gui_DrawCover(header->id);
				break;
			case 1:
				CHANGE(game_cfg->language, CFG_LANG_MAX);
				break;
			case 2:
				CHANGE(game_cfg->video, CFG_VIDEO_MAX);
				break;
			case 3:
				CHANGE(game_cfg->video_patch, CFG_VIDEO_PATCH_MAX);
				break;
			case 4:
				CHANGE(game_cfg->vidtv, 1);
				break;
			case 5:
				CHANGE(game_cfg->country_patch, 1);
				break;
			case 6:
				CHANGE(game_cfg->fix_002, 1);
				break;
			case 7:
				CHANGE(game_cfg->ios_idx, CFG_IOS_MAX);
				break;
			case 8:
				CHANGE(game_cfg->block_ios_reload, 2);
				break;
			case 9:
				if (!disc) Menu_Alt_Dol(header, game_cfg, 1);
				break;
			case 10:
				CHANGE(game_cfg->ocarina, 1);
				break;
			case 11:
				CHANGE(game_cfg->hooktype, NUM_HOOK-1);
				break;
			case 12:
				Menu_Cheats(header);
				break;
			case 13:
				printf("\n\n");
				Download_Cover((char*)header->id, change > 0, true);
				Cache_Invalidate();
				Gui_DrawCover(header->id);
				Menu_PrintWait();
				break;
			case 14: // hide game
				printf("\n\n");
				printf_x(gt("Saving Settings... "));
				__console_flush(0);
				if (set_hide_game(header->id, change > 0)) {
					printf(gt("OK"));
				} else {
					printf(gt("ERROR"));
					sleep(1);
				}
				__console_flush(0);
				Gui_DrawCover(header->id);
				break;
			case 15:
				CHANGE(game_cfg->write_playlog, 3);
				break;
			case 16:
				CHANGE(game_cfg->clean, 2);
				break;
			case 17:
				CHANGE(game_cfg->channel_boot, 1);
				break;
			case 18:
				Menu_dump_savegame(header);
				break;
			}
		} else if (change) {
			switch (menu.current) {
			case 0:
				printf("\n\n");
				printf_x(gt("Saving Settings... "));
				__console_flush(0);
				if (set_favorite(header->id, change > 0)) {
					printf(gt("OK"));
				} else {
					printf(gt("ERROR"));
					sleep(1);
				}
				__console_flush(0);
				Gui_DrawCover(header->id);
				break;
			case 1:
				CHANGE(game_cfg->language, CFG_LANG_MAX);
				break;
			case 2:
				CHANGE(game_cfg->video, CFG_VIDEO_MAX);
				break;
			case 3:
				CHANGE(game_cfg->video_patch, CFG_VIDEO_PATCH_MAX);
				break;
			case 4:
				CHANGE(game_cfg->vidtv, 1);
				break;
			case 5:
				CHANGE(game_cfg->country_patch, 1);
				break;
			case 6:
				CHANGE(game_cfg->fix_002, 1);
				break;
			case 7:
				CHANGE(game_cfg->ios_idx, CFG_IOS_MAX);
				break;
			case 8:
				CHANGE(game_cfg->block_ios_reload, 2);
				break;
			case 9:
				if (!disc) Menu_Alt_Dol(header, game_cfg, 1);
				break;
			case 10:
				CHANGE(game_cfg->ocarina, 1);
				break;
			case 11:
				CHANGE(game_cfg->hooktype, NUM_HOOK-1);
				break;
			case 12:
				Menu_Cheats(header);
				break;
			case 13:
				printf("\n\n");
				Download_Cover((char*)header->id, change > 0, true);
				Cache_Invalidate();
				Gui_DrawCover(header->id);
				Menu_PrintWait();
				break;
			case 14: // hide game
				printf("\n\n");
				printf_x(gt("Saving Settings... "));
				__console_flush(0);
				if (set_hide_game(header->id, change > 0)) {
					printf(gt("OK"));
				} else {
					printf(gt("ERROR"));
					sleep(1);
				}
				__console_flush(0);
				Gui_DrawCover(header->id);
				break;
			case 15:
				CHANGE(game_cfg->write_playlog, 3);
				break;
			case 16:
				CHANGE(game_cfg->clean, 2);
				break;
			case 17:
				CHANGE(game_cfg->nand_emu, 2);
				break;
			case 18:
				Menu_dump_savegame(header);
				break;
			}
		}
		if (buttons & CFG.button_confirm.mask) {
			CFG.confirm_start = 0;
			Music_Pause();
			Menu_Boot(disc);
			break;
		}
		if (buttons & CFG.button_save.mask) {
			bool ret;
			printf("\n\n");
			if (need_save) {
				ret = CFG_save_game_opt(header->id);
				if (ret) {
					printf_x(gt("Options saved for this game."));
				} else printf(gt("Error saving options!")); 
			} else {
				ret = CFG_discard_game_opt(header->id);
				if (ret) printf_x(gt("Options discarded for this game."));
				else printf(gt("Error discarding options!")); 
			}
			sleep(1);
		}
		// HOME button
		if (buttons & CFG.button_exit.mask) {
			Handle_Home();
		}
		if (buttons & CFG.button_other.mask) { ret_val = 1; break; }
		if (buttons & CFG.button_cancel.mask) break;
	}
	CFG_release_game(game_cfg2);
	// refresh favorites list
	if (!disc) Switch_Favorites(enable_favorite);
	return ret_val;
}

void Save_Game_List()
{
	char name[200];
	FILE *f;
	int i;

	printf_x(gt("Saving gamelist.txt ... "));
	__console_flush(0);

	snprintf(D_S(name), "%s/%s", USBLOADER_PATH, "gamelist.txt");
	f = fopen(name, "wb");
	if (!f) goto error;
	fprintf(f, "# CFG USB Loader game list %s\n", CFG_VERSION);
	for (i=0; i<all_gameCnt; i++) {
		fprintf(f, "# %.6s %s\n", all_gameList[i].id, all_gameList[i].title);
		fprintf(f, "%.6s = %s\n", all_gameList[i].id, get_title(&all_gameList[i]));
	}
	fclose(f);
	printf("OK");
	return;

	error:
	printf(gt("ERROR"));
}

void print_debug_hdr(char *str, int size)
{
	snprintf(str, size, "# CFG USB Loader %s\n", CFG_VERSION);
	str_seek_end(&str, &size);
	snprintf(str, size, "\nIOS:\n\n");
	str_seek_end(&str, &size);
	print_all_ios_info_str(str, size);
	str_seek_end(&str, &size);
	snprintf(str, size, "\nMEM STATS:\n");
	str_seek_end(&str, &size);
	lib_mem_stat_str(str, size);
	str_seek_end(&str, &size);
	snprintf(str, size, "\nTIME STATS:\n\n");
	str_seek_end(&str, &size);
	time_statsf(NULL, str, size);
	str_seek_end(&str, &size);
	snprintf(str, size, "\nDEBUG LOG:\n\n");
	str_seek_end(&str, &size);
}

void Save_Debug()
{
	char *name = "debug.log";
	char path[200];
	char str[2000];
	FILE *f;

	printf("\n\n");
	snprintf(D_S(path), "%s/%s", USBLOADER_PATH, name);
	printf_x("%s\n%s\n", gt("Saving:"), path);
	__console_flush(0);
	f = fopen(path, "w");
	if (!f) {
		printf_(gt("ERROR"));
		return;
	}
	print_debug_hdr(str, sizeof(str));
	fprintf(f, "%s", str);
	fprintf(f, "%s\n", dbg_log_buf);
	fprintf(f, "\nEND\n");
	fclose(f);
	printf_("OK");
}

void Save_IOS_Hash()
{
	char *name = "ioshash.txt";
	char path[200];
	FILE *f;

	printf("\n\n");
	snprintf(D_S(path), "%s/%s", USBLOADER_PATH, name);
	printf_x("%s\n%s\n", gt("Saving:"), path);
	__console_flush(0);
	f = fopen(path, "a"); // append
	if (!f) {
		printf_(gt("ERROR"));
		return;
	}
	time_t t = time(NULL);
	fprintf(f, "\n# CFG %s %s", CFG_VERSION, ctime(&t));
	char *info = get_ios_info_from_tmd();
	if (!info) info = "??";
	fprintf(f, "IOS%d, ", IOS_GetVersion());
	char str[100];
	if (get_ios_tmd_hash_str(str)) {
		fprintf(f, "%s", str);
	} else {
		fprintf(f, "TMD HASH ERROR");
	}
	fprintf(f, " r%d Base: %s\n\n", IOS_GetRevision(), info);
	fclose(f);
	printf_("OK");
}

void Menu_Save_Settings()
{
	int ret;
	printf("\n");
	printf_x(gt("Saving Settings... "));
	printf("\n");
	__console_flush(0);
	FgColor(CFG.color_inactive);
	ret = CFG_Save_Global_Settings();
	DefaultColor();
	if (ret) {
		printf_(gt("OK"));
		printf("\n");
		Save_Game_List();
	} else {
		printf_(gt("ERROR"));
	}
	printf("\n");
	//sleep(2);
	printf_(gt("NOTE: may loader restart is required\nfor the settings to take effect."));
	printf("\n\n");
	Menu_PrintWait();
}

void Menu_All_IOS_Info()
{
	printf("\n\ncIOS Info:\n\n");
	print_all_ios_info(stdout);
	printf("\n");
	Menu_PrintWait();
}

int Menu_Global_Options()
{
	int rows, cols, win_size, info_size;
	CON_GetMetrics(&cols, &rows);
	//calc_rows: // debug
	info_size = 11;
	if (strcmp(LAST_CFG_PATH, USBLOADER_PATH)) info_size += 2;
	win_size = rows - info_size;
	if (win_size < 3) win_size = 3;

	if (CFG.disable_options) return 0;

	struct discHdr *header = NULL;
	int redraw_cover = 0;

	struct Menu menu;
	const int num_opt = 14;
	char active[num_opt];
	menu_init(&menu, num_opt);
	menu_init_active(&menu, active, sizeof(active));
	active[5] = usb_isgeckoalive(EXI_CHANNEL_1);

	for (;;) {

		menu_begin(&menu);

		if (gameCnt) {
			header = &gameList[gameSelected];
		} else {
			header = NULL;
		}

		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Global Options"));
		printf(":\n");
		DefaultColor();
		menu_window_begin(&menu, win_size, num_opt);
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Main Menu"));
		if (menu_window_mark(&menu))
			printf("%s%2d/%-2d< %s > (%d)\n", con_align(gt("Profile:"),8),
				CFG.current_profile + 1, CFG.num_profiles,
				CFG.profile_names[CFG.current_profile],
				CFG.num_favorite_game);
		if (menu_window_mark(&menu))
			printf("%s%2d/%2d < %s >\n", con_align(gt("Theme:"),7),
				cur_theme + 1, num_theme, *CFG.theme ? CFG.theme : gt("none"));
		if (menu_window_mark(&menu))
			printf("%s< %s >\n", con_align(gt("Device:"),13),
				(wbfsDev == WBFS_DEVICE_USB) ? "USB" : "SDHC");
		if (menu_window_mark(&menu))
			printf("%s< %s >\n", con_align(gt("Partition:"),13), CFG.partition);
		if (menu_window_mark(&menu))
			printf("%s< %s >\n", con_align("WiiRD:",13), gt(str_wiird[CFG.wiird]));
		if (menu_window_mark(&menu))
			printf("%s< %s >\n", con_align(gt("Default Gamecube Loader:"),24), gt(map_get_name(map_gc_boot, CFG.default_gc_loader+1)));
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Download All Missing Covers"));
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Download game information")); // download database - lustar
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Download Plugins"));
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Update themes"));
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Check For Updates"));
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Save debug.log"));
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Show cIOS info"));
		DefaultColor();
		menu_window_end(&menu, cols);
		
		printf_h(gt("Press %s for game options"), (button_names[CFG.button_other.num]));
		printf("\n");
		printf_h(gt("Press %s to save global settings"), (button_names[CFG.button_save.num]));
		printf("\n\n");
		Print_SYS_Info();
		DefaultColor();
		//Con_SetPosition(0, rows-1); printf(" -- %d %d --", rows, win_size); //calc_rows debug
		__console_flush(0);

		if (redraw_cover) {
			if (header) Gui_DrawCover(header->id);
			redraw_cover = 0;
		}
		
		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move(&menu, buttons);

		int change = 0;
		if (buttons & WPAD_BUTTON_LEFT) change = -1;
		if (buttons & WPAD_BUTTON_RIGHT) change = +1;
		if (buttons & CFG.button_confirm.mask) change = +1;

		if (change) {
			switch (menu.current) {
			case 0:
				Menu_Views();
				return 0;
			case 1:
				CHANGE(CFG.current_profile, CFG.num_profiles-1);
				// refresh favorites list
				Switch_Favorites(CFG.profile_start_favorites[CFG.current_profile]);

				int new_theme;
				if (CFG.profile_theme[CFG.current_profile] == -1)
					new_theme = CFG.profile_theme[0];
				else new_theme = CFG.profile_theme[CFG.current_profile];
				CFG_switch_theme(new_theme);
				Video_DrawBg();
				redraw_cover = 1;
				Cache_Invalidate();
				break;
			case 2:
				CFG_switch_theme(cur_theme + change);
				CFG.profile_theme[CFG.current_profile] = cur_theme;
				Video_DrawBg();
				redraw_cover = 1;
				Cache_Invalidate();
				break;
			case 3:
				Menu_Device();
				return 0;
			case 4:
				Menu_Partition(true);
				return 0;
			case 5:
				CHANGE(CFG.wiird, 2);
				break;
			case 6:
				CHANGE(CFG.default_gc_loader, 2);
				break;
			case 7:
				//cache_wait_idle();
				Download_All_Covers(change > 0);
				Cache_Invalidate();
				if (header) Gui_DrawCover(header->id);
				Menu_PrintWait();
				break;
			case 8:
				Download_XML();
				Download_Titles();
				break;
			case 9:
				Download_Plugins();
				break;
			case 10:
				Theme_Update();
				break;
			case 11:
				Online_Update();
				break;
			case 12:
				Save_Debug();
				Save_IOS_Hash();
				sleep(2);
				break;
			case 13:
				Menu_All_IOS_Info();
				break;
			}
		}
		
		// debug
		//if (buttons & WPAD_BUTTON_MINUS) { rows--; goto calc_rows; }
		//if (buttons & WPAD_BUTTON_PLUS) { rows++; goto calc_rows; }

		// HOME button
		if (buttons & CFG.button_exit.mask) {
			Handle_Home();
		}
		if (buttons & CFG.button_save.mask) {
			Menu_Save_Settings();
		}
		if (buttons & WPAD_BUTTON_PLUS) {
			printf("\n");
			mem_stat();
			time_stats();
			Menu_PrintWait();
		}
		/*
		if (buttons & WPAD_BUTTON_MINUS) {
			printf("\n");
			printf("Reloading ");
			Music_Pause();
			CURR_IOS_IDX = -1;
			ReloadIOS(1,1);
			printf("\n");
			Music_UnPause();
			sleep(1);
		}
		*/

		if (buttons & CFG.button_other.mask) return 1;
		if (buttons & CFG.button_cancel.mask) break;
	}
	return 0;
}

void Menu_Options()
{
	int ret = 1;
	while(ret)
	{
		if (gameCnt) {
			ret = Menu_Game_Options();
		}
		if (ret) {
			ret = Menu_Global_Options();
		}
	}
}

bool go_gui = false;
extern int action_alpha;

void DoAction(int action)
{
	if (action & CFG_BTN_REMAP) return;
	switch(action) {
		case CFG_BTN_NOTHING:
			break;
		case CFG_BTN_OPTIONS: 
			if (!CFG.disable_options) Menu_Options();
			break;
		case CFG_BTN_GUI:
			if (go_gui) {
				action_string[0] = 0;
				action_alpha=0;
			}
			if (CFG.gui) go_gui = !go_gui;
			break;
		case CFG_BTN_REBOOT:
			Con_Clear();
			Restart();
			break;
		case CFG_BTN_EXIT:
			Con_Clear();
			printf("\n");
			printf_("Exiting...");
			__console_flush(0);
			Sys_Exit();
			break;
		case CFG_BTN_SCREENSHOT:
			__console_flush(0);
			Make_ScreenShot();
			break;
		case CFG_BTN_INSTALL:
			Menu_Install();
			break;
		case CFG_BTN_REMOVE:
			Menu_Remove();
			break;
		case CFG_BTN_MAIN_MENU: 
			Menu_Views();
			break;
		case CFG_BTN_GLOBAL_OPS:
			if (!CFG.disable_options) Menu_Global_Options();
			break;
		case CFG_BTN_PROFILE: 
			if (CFG.current_profile == CFG.num_profiles-1)
				CFG.current_profile = 0;
			else
				CFG.current_profile++;
			Switch_Favorites(CFG.profile_start_favorites[CFG.current_profile]);
			
			int new_theme;
			if (CFG.profile_theme[CFG.current_profile] == -1)
				new_theme = CFG.profile_theme[0];
			else new_theme = CFG.profile_theme[CFG.current_profile];
			CFG_switch_theme(new_theme);
			Video_DrawBg();
			if (gameCnt) Gui_DrawCover(gameList[gameSelected].id);//redraw_cover = 1;
			Cache_Invalidate();

			sprintf(action_string, gt("Profile: %s"), CFG.profile_names[CFG.current_profile]);
			if (go_gui) action_alpha = 0xFF;
			
			break;
		case CFG_BTN_FAVORITES:
			{
				extern void reset_sort_default();
				reset_sort_default();
				Switch_Favorites(!enable_favorite);
			}
			break;
		case CFG_BTN_BOOT_GAME:
			Menu_Boot(0);
			break;
		case CFG_BTN_BOOT_DISC:
			Menu_Boot(1);
			break;
		case CFG_BTN_THEME:
			CFG_switch_theme(cur_theme + 1);
			CFG.profile_theme[CFG.current_profile] = cur_theme;
			Video_DrawBg();
			if (gameCnt) Gui_DrawCover(gameList[gameSelected].id);//redraw_cover = 1;
			Cache_Invalidate();
			
			sprintf(action_string, gt("Theme: %s"), theme_list[cur_theme]);
			if (go_gui) action_alpha = 0xFF;
			
			break;
		case CFG_BTN_UNLOCK:
			if (CFG.admin_lock) Menu_Unlock();
			break;
		case CFG_BTN_HBC:
			Con_Clear();
			printf("\n");
			printf_("HBC...");
			__console_flush(0);
			Sys_HBC();
			break;
		case CFG_BTN_SORT:
			if (sort_desc) {
				sort_desc = 0;
				if (sort_index == sortCnt - 1)
					sort_index = 0;
				else
					sort_index = sort_index + 1;
				sortList(sortTypes[sort_index].sortAsc);
			} else {
				sort_desc = 1;
				sortList(sortTypes[sort_index].sortDsc);
			}
			if (gameCnt) Gui_DrawCover(gameList[gameSelected].id);//redraw_cover = 1;

			
			sprintf(action_string, gt("Sort: %s-%s"), sortTypes[sort_index].name, (sort_desc) ? "DESC":"ASC");
			
			break;
		case CFG_BTN_FILTER:
			Menu_Filter();
			break;
		case CFG_BTN_RANDOM:
			gameSelected = (rand() >> 16) % gameCnt;
			__Menu_ScrollStartList();
			break;
		default:
			// Priiloader magic words, and channels
			if ((action & 0xFF) < 'a') {
				// upper case final letter implies channel
				Con_Clear();
				Sys_Channel(action);
			} else {
				// lower case final letter implies magic word
				Con_Clear();
				*(vu32*)0x8132FFFB = action;
				Restart();
			}
			break;
	}
}

int get_buttonmap_num(int buttons)
{
	int i;
	for (i = 4; i < MAX_BUTTONS; i++) {
			if (buttons & buttonmap[MASTER][i]) return i;
	}
	return -1;
}

int get_buttonmap_action(int button_num)
{
	int i = button_num;
	if (i < 4 || i >= MAX_BUTTONS) return CFG_BTN_NOTHING;
	return *(&CFG.button_M + (i - 4));
}

int get_button_action(int buttons)
{
	int i = get_buttonmap_num(buttons);
	return get_buttonmap_action(i);
}

void __Menu_Controls(void)
{
	if (go_gui) goto gui_mode;

	//u32 buttons = Wpad_WaitButtons();
	u32 buttons = Wpad_WaitButtonsRpt();

	/* UP/DOWN buttons */
	if (buttons & WPAD_BUTTON_UP)
		__Menu_MoveList(-1);

	if (buttons & WPAD_BUTTON_DOWN)
		__Menu_MoveList(1);

	/* LEFT/RIGHT buttons */
	if (buttons & WPAD_BUTTON_LEFT) {
		//__Menu_MoveList(-ENTRIES_PER_PAGE);
		if (CFG.cursor_jump) {
			__Menu_MoveList(-CFG.cursor_jump);
		} else {
			__Menu_MoveList((gameSelected-gameStart == 0) ? -ENTRIES_PER_PAGE : -(gameSelected-gameStart));
		}
	}

	if (buttons & WPAD_BUTTON_RIGHT) {
		//__Menu_MoveList(ENTRIES_PER_PAGE);
		if (CFG.cursor_jump) {
			__Menu_MoveList(CFG.cursor_jump);
		} else {
			__Menu_MoveList((gameSelected-gameStart == (ENTRIES_PER_PAGE - 1)) ? ENTRIES_PER_PAGE : ENTRIES_PER_PAGE - (gameSelected-gameStart) - 1);
		}
	}

	check_buttons:


	if (CFG.admin_lock) {
		if (buttons & CFG.button_other.mask) {

			static long long t_start;
			long long t_now;
			unsigned ms_diff = 0;
			bool display_unlock = false;

			Con_Clear();
			t_start = gettime();
			while (!display_unlock && (Wpad_Held() & CFG.button_other.mask)) {
				buttons = Wpad_GetButtons();
				VIDEO_WaitVSync();
				t_now = gettime();
				ms_diff = diff_msec(t_start, t_now);
				if (ms_diff > 5000)
					display_unlock = true;
			}
			if (display_unlock)
				Menu_Unlock();
			else
				buttons = buttonmap[MASTER][CFG.button_other.num];
		}
	}

	/* A button */
	//if (buttons & CFG.button_confirm.mask)
	//	Menu_Boot(0);

	int i;
	for (i = 4; i < MAX_BUTTONS; i++) {
			if (buttons & buttonmap[MASTER][i]) 
				DoAction(*(&CFG.button_M + (i - 4)));
	}
		
	if (go_gui) {
		gui_mode:;
		int prev_sel = gameSelected;
		CFG.gui = 1; // disable auto start
		buttons = Gui_Mode();
		if (prev_sel != gameSelected) {
			// List scrolling
			__Menu_ScrollStartList();
		}
		// if only returning to con mode, clear button status
		/*if (CFG.buttons == CFG_BTN_OPTIONS_1) {
			if (buttons & CFG.button_cancel.mask) buttons = 0;
		} else if (CFG.buttons == CFG_BTN_OPTIONS_B) {
			if (buttons & CFG.button_other.mask) buttons = 0;
		}
		*/
		// if action started from gui, process it then return to gui
		//if (buttons) {
		//	go_gui = true;
		goto check_buttons;
		//}
	}
}

const char *get_dev_name(int dev)
{
	switch (dev) {
		case WBFS_DEVICE_USB: return gt("USB Mass Storage Device");
		case WBFS_DEVICE_SDHC: return gt("SD/SDHC Card");
	}
	return gt("Unknown");
}

void print_part(int i, PartList *plist)
{
	partitionEntry *entry = &plist->pentry[i];
	PartInfo *pinfo = &plist->pinfo[i];
	f32 size = entry->size * (plist->sector_size / GB_SIZE);
	if (plist->num == 1) {
		printf(gt("RAW"));
	} else {
		printf("%s", i<4 ? "P" : "L");
		printf("#%d", i+1);
	}
	printf(" %7.2f ", size);
	
	if (pinfo->fs_index) {
		bool is_ext = part_is_extended(entry->type);
		if (is_ext) {
			printf("%s", part_type_name(entry->type));
			printf("/");
		}
		char pname[16];
		sprintf(pname, "%s%d", get_fs_name(pinfo->fs_type), pinfo->fs_index);
		printf("%-5s", pname);

		if (pinfo->fs_type == PART_FS_WBFS) {
			if (!is_ext) printf("      ");
			if (WBFS_Mounted() && wbfs_part_idx == pinfo->fs_index) {
				printf(gt("[USED]"));
			}
		} else {
			MountPoint *m = mount_find_part(wbfsDev, entry->sector);
			char mname[16] = "";
			if (m) {
				mount_name2drive(m->name, mname);
			}
			printf(" %-5s", mname);
			if (WBFS_Selected() && entry->sector == wbfs_part_lba) {
				printf(" ");
				printf(gt("[USED]"));
			}
		}
	} else {
		printf("%s", part_type_name(entry->type));
		char *pdata = part_type_data(entry->type);
		if (pdata) {
			if (strncmp(pdata,"FAT",3)==0
				|| strncmp(pdata,"NTFS",4)==0
				|| strncmp(pdata,"LINUX",5)==0)
			{
				printf("/?");
			}
		}
	}
}


void Menu_Format_Partition(partitionEntry *entry, bool delete_fs)
{
	int ret;

	if (CFG.disable_format) return;

	printf_x(gt("%s, please wait..."), delete_fs?gt("Deleting"):gt("Formatting"));
	__console_flush(0);

	// unmount if overwriting mounted fs
	MountPoint *mp;
	mp = mount_find_part(wbfsDev, entry->sector);
	if (mp) {
		Music_Pause();
		UnmountFS(mp->name);
	}

	// WBFS_Close will unmount fat/wbfs too
	WBFS_Close();
	all_gameCnt = gameCnt = 0;

	/* Format partition */
	bool ok;
	if (delete_fs) {
		char buf[512];
		memset(buf,0,sizeof(buf));
		ret = -2;
		ok = Device_WriteSectors(wbfsDev, entry->sector, 1, buf);
		if (ok) ret = 0;
	} else {
		ret = WBFS_Format(entry->sector, entry->size);
		ok = (ret == 0);
	}
	if (!ok) {
		printf("\n");
		printf_(gt("ERROR! (ret = %d)"), ret);
		printf("\n");
	} else {
		printf(" ");
		printf(gt("OK!"));
		printf("\n");
	}

	if (mp) {
		Music_UnPause();
	}

	printf("\n");
	Menu_PrintWait();
}

void Menu_Sync_FAT()
{
	extern MountTable mtab;
	MountPoint *m;
	struct statvfs vfs;
	struct statvfs vfs2;
	float free;
	int ret;
	int i;
	char drive[32];

	printf("\n");
	printf("\n");
	printf_x(gt("Sync FAT free space info?"));
	printf("\n");
	printf_(gt("(This can take a couple of minutes)"));
	printf("\n");
	printf("\n");
	for (i=0; i<mtab.num; i++)
	{
		m = &mtab.point[i];
		if (m->fstype == PART_FS_FAT) {
			free = 0;
			mount_name2drive(m->name, drive);
			memset(&vfs, 0, sizeof(vfs));
			ret = statvfs(drive, &vfs);
			if (ret == 0) {
				free = (f32)vfs.f_frsize * (f32)vfs.f_bfree / GB_SIZE;
			} else {
				printf(gt("ERROR!"));
				printf("\n");
			}
			printf_("[%s] %s %.3f %s %s\n",
				m->device == WBFS_DEVICE_USB ? "USB" : "SD",
				drive, free, gt("GB"), gt("free"));
		}
	}
	printf("\n");
	printf_h(gt("Press %s button to continue."), (button_names[CFG.button_confirm.num]));
	printf("\n");
	printf_h(gt("Press %s button to go back."), (button_names[CFG.button_cancel.num]));
	printf("\n");
	int button;
	while(1) {
		button = Wpad_WaitButtonsCommon();
		if (button & CFG.button_cancel.mask) return;
		if (button & CFG.button_confirm.mask) break;
	}
	printf("\n");
	printf_(gt("Synchronizing, please wait."));
	printf("\n");
	printf("\n");
	for (i=0; i<mtab.num; i++)
	{
		m = &mtab.point[i];
		if (m->fstype == PART_FS_FAT) {
			free = 0;
			mount_name2drive(m->name, drive);
			memset(&vfs, 0, sizeof(vfs));
			ret = statvfs(drive, &vfs);
			if (ret == 0) {
				free = (f32)vfs.f_frsize * (f32)vfs.f_bfree / GB_SIZE;
			}
			printf_("[%s] %s %.3f %s %s\n",
					m->device == WBFS_DEVICE_USB ? "USB" : "SD",
					drive, free, gt("GB"), gt("free"));
			printf_("...");
			memset(&vfs2, 0, sizeof(vfs2));
			memcpy(&vfs2.f_flag, "SCAN", 4);
			ret = statvfs(drive, &vfs2);
			if (ret == 0) {
				if (vfs.f_bfree == vfs2.f_bfree) {
					printf(gt("OK!"));
					printf("\n");
				} else {
					free = (f32)vfs2.f_frsize * (f32)vfs2.f_bfree / GB_SIZE;
					printf("%.3f %s %s\n", free, gt("GB"), gt("free"));
				}
			} else {
				printf(gt("ERROR!"));
				printf("\n");
			}
			printf("\n");
		}
	}
	Menu_PrintWait();
}

void Menu_Partition(bool must_select)
{
	if (!MountWBFS) {
		printf_x(gt("ERROR: WBFS not mounted!"));
		printf("\n");
		sleep(4);
		return;
	}
	
	PartList plist;
	partitionEntry *entry;
	PartInfo *pinfo;

	int i;
	s32 ret = 0;
	int is_raw = 0;

rescan:

	/* Get partition entries */
	ret = Partition_GetList(wbfsDev, &plist);
	if (ret < 0) {
		printf_x(gt("ERROR: No partitions found! (ret = %d)"), ret);
		printf("\n");
		sleep(4);
		return;
	}
	if (plist.num == 1) is_raw = 1; else is_raw = 0;

	struct Menu menu;
	char active[MAX_PARTITIONS_EX];
	menu_init(&menu, plist.num);
	menu_init_active(&menu, active, MAX_PARTITIONS_EX);

loop:
	menu_begin(&menu);
	/* Clear console */
	Con_Clear();

	FgColor(CFG.color_header);
	printf_x(gt("Select a partition"));
	printf(":\n\n");
	DefaultColor();
	printf_("[ %s ]\n\n", get_dev_name(wbfsDev));

	printf_("P# %s\n", gt("Size(GB) Type  Mount Used"));
	printf_("-----------------------------\n");
	//       P#1  400.00 FAT1  usb:  
	//       P#2  400.00 FAT2  game: [USED]
	//       P#3  400.00 WBFS1       
	//       P#4  400.00 NTFS        

	int invalid_ext = -1;
	for (i = 0; i < plist.num; i++) {
		partitionEntry *entry = &plist.pentry[i];
		active[i] = part_valid_data(entry);
		MENU_MARK();
		print_part(i, &plist);
		printf("\n");
		if (part_is_extended(entry->type)
				&& plist.pinfo[i].fs_type == PART_FS_WBFS
				&& plist.pinfo[i].fs_index > 0
				&& i < 4)
		{
			invalid_ext = i;
		}
	}
	printf("\n");
	printf_h(gt("Press %s button to select."), (button_names[CFG.button_confirm.num]));
	printf("\n");
	printf_h(gt("Press %s button to go back."), (button_names[CFG.button_cancel.num]));
	printf("\n");
	printf_h(gt("Press %s button to sync FAT."), (button_names[CFG.button_save.num]));
	printf("\n");
	if (!CFG.disable_format) {
		if (is_raw) {
			printf_("NOTE: RAW partition format\n"
					"and delete not supported");
		} else {
			printf_h(gt("Press %s button to format WBFS."),
					(button_names[NUM_BUTTON_PLUS]));
			printf("\n");
			printf_h(gt("Press %s button to delete FS."),
					(button_names[NUM_BUTTON_MINUS]));
		}
		printf("\n");
		if (invalid_ext >= 0) {
			printf_("\n");
			printf_(gt("NOTE: partition P#%d is type EXTEND but\n"
					"contains a WBFS filesystem. This is an\n"
					"invalid setup. Press %s to change the\n"
					"partition type from EXTEND to data."),
					invalid_ext+1, (button_names[CFG.button_other.num]));
			printf("\n");
			printf_h(gt("Press %s button to fix EXTEND/WBFS."),
					(button_names[CFG.button_other.num]));
			printf("\n");
		}
	}

	u32 buttons = Wpad_WaitButtonsCommon();

	menu_move(&menu, buttons);

	// B button
	if (buttons & CFG.button_cancel.mask) {
		if (must_select) {
			if (WBFS_Selected()) return;
			printf("\n");
			printf_(gt("No partition selected!"));
			printf("\n");
			sleep(2);
		} else {
			return;
		}
	}

	// A button
	if (buttons & CFG.button_confirm.mask) {
		i = menu.current;
		entry = &plist.pentry[i];
		pinfo = &plist.pinfo[i];
		if (pinfo->fs_index) {
			char pname[16];
			sprintf(pname, "%s%d", get_fs_name(pinfo->fs_type), pinfo->fs_index);
			printf(gt("Opening partition: %s"), pname);
			printf("\n");
			__console_flush(0);
			ret = WBFS_OpenPart(pinfo->fs_type, pinfo->fs_index, entry->sector,
					entry->size, CFG.partition);
			if (ret == 0 && WBFS_Selected()) {
				if (must_select) {
					// called from global options
					__Menu_GetEntries();
				}
				return;
			}
		}
	}

	// +/- button
	if ((buttons & WPAD_BUTTON_MINUS || buttons & WPAD_BUTTON_PLUS)
			&& !CFG.disable_format && !is_raw)
	{
		i = menu.current;
		entry = &plist.pentry[i];
		if (part_valid_data(entry)) {
			const char *act = gt("format");
			if (buttons & WPAD_BUTTON_MINUS) act = gt("delete");
			printf("\n");
			printf_x(gt("Are you sure you want to %s\n"
						"this partition?"), act);
			printf("\n\n");
			printf_("");
			print_part(i, &plist);
			printf("\n\n");
			if (Menu_Confirm(0)) {
				printf("\n");
				Menu_Format_Partition(entry, buttons & WPAD_BUTTON_MINUS);
				goto rescan;
			}
		}
	}
	// 2 button
	if (buttons & CFG.button_save.mask)
	{
		Menu_Sync_FAT();
	}
	// 1 button
	if (buttons & CFG.button_other.mask
		&& !CFG.disable_format
		&& invalid_ext >= 0)
	{
		printf("\n");
		printf_x(gt("Are you sure you want to FIX\n"
				"this partition?"));
		printf("\n\n");
		printf_("");
		print_part(invalid_ext, &plist);
		printf("\n\n");
		if (Menu_Confirm(0)) {
			printf("\n");
			printf_(gt("Fixing EXTEND partition..."));
			ret = Partition_FixEXT(wbfsDev, invalid_ext, plist.sector_size);
			printf("%s\n", ret ? gt("FAIL") : gt("OK"));
			printf_(gt("Press any button..."));
			Wpad_WaitButtonsCommon();
			goto rescan;
		}
	}

	goto loop;
}

void Menu_Device(void)
{
	static int first_time = 1;
	int save_wbfsDev = wbfsDev;
	s32 ret;

	if (CFG.device == CFG_DEV_USB) {
		wbfsDev = WBFS_DEVICE_USB;
		goto mount_dev;
	}
	if (CFG.device == CFG_DEV_SDHC) {
		wbfsDev = WBFS_DEVICE_SDHC;
		goto mount_dev;
	}
	restart:

	/* Ask user for device */
	for (;;) {
		//Enable the console
		Gui_Console_Enable();

		/* Clear console */
		Con_Clear();
			
		FgColor(CFG.color_header);
		printf_x(gt("Select WBFS device:"));
		printf("\n");
		DefaultColor();
		printf_("< %s >\n\n", get_dev_name(wbfsDev));

		FgColor(CFG.color_help);
		printf_(gt("Press %s/%s to select device."), (button_names[NUM_BUTTON_LEFT]), (button_names[NUM_BUTTON_RIGHT]));
		printf("\n");
		printf_(gt("Press %s button to continue."), (button_names[CFG.button_confirm.num]));
		printf("\n");
		if (!first_time) {
			printf_(gt("Press %s button to cancel."), (button_names[CFG.button_cancel.num]));
			printf("\n");
		}
		printf("\n");

		Print_SYS_Info();

		u32 buttons = Wpad_WaitButtonsCommon();

		/* LEFT/RIGHT buttons */
		if (buttons & WPAD_BUTTON_LEFT) {
			if ((--wbfsDev) < WBFS_MIN_DEVICE)
				wbfsDev = WBFS_MAX_DEVICE;
		}
		if (buttons & WPAD_BUTTON_RIGHT) {
			if ((++wbfsDev) > WBFS_MAX_DEVICE)
				wbfsDev = WBFS_MIN_DEVICE;
		}

		/* A button */
		if (buttons & CFG.button_confirm.mask)
			break;

		// B button
		if (buttons & CFG.button_cancel.mask) {
		   	if (!first_time && WBFS_Selected()) {
				wbfsDev = save_wbfsDev;
				return;
			}
		}
	}

	mount_dev:;

	CFG.device = CFG_DEV_ASK; // next time ask

	ret = Retry_Init_Dev(wbfsDev, 1);
	if (ret == WPAD_BUTTON_A) goto restart;
	
	// If 1 is pressed that means to skip the WBFS-mounting and use GC only
	MountWBFS = (ret != WPAD_BUTTON_1);

	//usleep(100000); // 100ms
	
	if (MountWBFS) {
		// Mount usb fat partition if not already
		// This is after device init, because of the timeout handling
		MountUSB();

		all_gameCnt = gameCnt = 0;
		/* Try to open device */
		//WBFS_Open();
		if (strcasecmp(CFG.partition, "ask") == 0) {
			Gui_Console_Enable();
			goto jump_to_selection;
		}
		WBFS_OpenNamed(CFG.partition);
		while (!WBFS_Selected()) {
		
			//Enable the console
			Gui_Console_Enable();

			/* Clear console */
			Con_Clear();

			printf_x(gt("WARNING:"));
			printf("\n\n");

			printf_(gt("Partition: %s not found!"), CFG.partition);
			printf("\n");
			printf_(gt("Select a different partition"));
			printf("\n");
			if (!CFG.disable_format) {
				printf_(gt("or format a WBFS partition."));
				printf("\n");
			}
			printf("\n");

			printf_h(gt("Press %s button to select a partition."), (button_names[CFG.button_confirm.num]));
			printf("\n");
			printf_h(gt("Press %s button to change device."), (button_names[CFG.button_cancel.num]));
			printf("\n");
			printf_h(gt("Press %s button to exit."), (button_names[CFG.button_exit.num]));
			printf("\n\n");

			// clear button states
			WPAD_Flush(WPAD_CHAN_ALL);

			/* Wait for user answer */
			for (;;) {
				u32 buttons = Wpad_WaitButtonsCommon();
				if (buttons & CFG.button_confirm.mask) break;
				if (buttons & CFG.button_cancel.mask) goto restart;
			}
				
			// Select or Format partition
			jump_to_selection:
			Menu_Partition(false);
		}

	}

	/* Get game list */
	get_time(&TIME.gamelist1);
	__Menu_GetEntries();
	get_time(&TIME.gamelist2);

	if (CFG.debug && first_time) {
		Menu_PrintWait();
	}
	first_time = 0;
}


int Retry_Init_Dev(int device, int a_select)
{
	static int ios_reloads = 0;
	int max_ios_reloads = 1;
	int timeout = 90;
	int retry_menu = 4;
	int ios_idx = CFG.game.ios_idx;
	int ret;
	int i;

	for (i=0; i<timeout; i++) {

		if (i == 1) {
			//Enable the console
			Gui_Console_Enable();
		}
		repaint:
		if (a_select || i > 0) {
			Con_Clear();
			//MountPrint();
			FgColor(CFG.color_header);
			if (!a_select) {
				printf("Searching for config.txt\n");
			}
			printf("\n");
			printf_("[ %s ]\n", get_dev_name(device));
			DefaultColor();
			printf("\n");
			printf_x(gt("Mounting device, please wait..."));
			printf("\n");
			printf_(gt("(%d seconds timeout)"), timeout);
			printf("\n\n");
			if (i >= retry_menu) {
				printf_("%s\n", gt("Device is not responding!"));
				if (device == WBFS_DEVICE_USB) {
					printf_("%s\n", gt("Make sure USB port 0 is used!\n"
								"(The one nearest to the edge)"));
				}
				printf_("%s\n", gt("You can also try unplugging\n"
							"and plugging back the device,\n"
							"or just wait some more"));
				if (a_select) {
					printf_h("%s\n", gt("Press A to select device"));
				} else {
					printf("\n\n");
					printf_h("%s\n", gt("Press A to continue without config.txt"));
				}
				printf_h("%s\n", gt("Press B to exit to HBC"));
				printf_h("%s\n", gt("Press HOME to reset"));
				// Press 1 to skip WBFS mounting and switch to GC-only mode
				printf_h("%s\n", gt("Press 1 to skip WBFS mounting"));
				//if (first_time && ios_reloads < max_ios_reloads) {
				if (ios_reloads < max_ios_reloads) {
					printf_h(gt("Press 2 to reload IOS"));
					printf(" < %s >\n", ios_str(ios_idx));
					printf_h("%s\n", gt("Press LEFT/RIGHT to select IOS"));
				} else {
					Print_IOS_Info();
					printf("\n");
				}
			}
		}
		if (i > 0) {
			printf("\n");
			printf_("Retry: %d ...\n", i);
		}
		fflush(stdout);
		__console_flush(0);

		/* Initialize WBFS */
		ret = WBFS_Init_Dev(device);
		if (ret >= 0) {
			printf("\n");
			printf_(gt("OK!"));
			return 0;
		}
		if (i < retry_menu) {
			sleep(1);
		} else {
			if (i == retry_menu) {
				Wpad_Init();
			}
			int button = Wpad_WaitButtonsTimeout(1000);
			switch (button) {
				case WPAD_BUTTON_A:
					return WPAD_BUTTON_A;
				case WPAD_BUTTON_B:
					Sys_HBC(0);
					Sys_Exit();
					break;
				case WPAD_BUTTON_HOME:
					Restart();
					break;
				case WPAD_BUTTON_1:
					return WPAD_BUTTON_1;
				case WPAD_BUTTON_2:
					if (ios_reloads >= max_ios_reloads) {
						goto repaint;
					}
					printf("\n");
					Music_Pause();
					cfg_ios_set_idx(ios_idx);
					CURR_IOS_IDX = -1;
					ReloadIOS(1,1);
					printf("\n");
					Music_UnPause();
					ios_reloads++;
					sleep(2);
					i = retry_menu; // reset retry count
					break;
				case WPAD_BUTTON_LEFT:
					ios_idx--;
					if (ios_idx < 0) ios_idx = 0;
					goto repaint;
				case WPAD_BUTTON_RIGHT:
					ios_idx++;
					if (ios_idx > CFG_IOS_MAX) ios_idx = CFG_IOS_MAX;
					goto repaint;
			}
		}
	}
	printf("\n");
	printf_(gt("ERROR! (ret = %d)"), ret);
	/* Restart wait */
	Restart_Wait();
	return -1;
}


u8 BCA_Data[64] ATTRIBUTE_ALIGN(32);

void Menu_DumpBCA(u8 *id)
{
	int ret;
	char fname[100];
	memset(BCA_Data, 0, 64);
	printf_("\n");
	printf_(gt("Reading BCA..."));
	printf("\n\n");
	ret = WDVD_Read_Disc_BCA(BCA_Data);
	hex_dump3(BCA_Data, 64);
	printf_("\n");
	if (ret) {
		printf_(gt("ERROR reading BCA!"));
		printf("\n\n");
		goto out;
	}
	// save
	snprintf(D_S(fname), "%s/%.6s.bca", USBLOADER_PATH, (char*)id);
	if (!Menu_Confirm(gt("save"))) return;
	printf("\n");
	printf_(gt("Writing: %s"), fname);
	printf("\n\n");
	FILE *f = fopen(fname, "wb");
	if (!f) {
		printf_(gt("ERROR writing BCA!"));
		printf("\n\n");
		goto out;
	}
	fwrite(BCA_Data, 64, 1, f);
	fclose(f);
	out:
	Menu_PrintWait();
}

void print_ntfs_write_error()
{
	printf_x(gt("ERROR: NTFS write disabled!\n(set ntfs_write=1)"));
	printf("\n");
}

void Menu_Install(void)
{
	static struct discHdr header ATTRIBUTE_ALIGN(32);

	s32 ret;
	int i;

#ifdef FAKE_GAME_LIST
	fake_games++;
	__Menu_GetEntries();
	return;
#endif

	if (CFG.disable_install) return;

	/* Clear console */
	Con_Clear();

	FgColor(CFG.color_header);
	printf_x(gt("Install game"));
	printf("\n\n");
	DefaultColor();
	
	char *nocover = "ZZZZZZ";
	Gui_DrawCover((u8 *)nocover);

	/* Disable WBFS mode */
	Disc_SetWBFS(0, NULL);

	// Wait for disc
	//printf_x("Checking game disc...");
	u32 cover = 0;
	ret = WDVD_GetCoverStatus(&cover);
	if (ret < 0) {
		err1:
		printf("\n");
		printf_(gt("ERROR! (ret = %d)"), ret);
		printf("\n");
		goto out;
	}
	if (!(cover & 0x2)) {
		printf_(gt("Please insert a game disc..."));
		printf("\n\n");
		printf_h(gt("Press %s to return"), (button_names[CFG.button_cancel.num]));
		printf("\n\n");
		do {
			ret = WDVD_GetCoverStatus(&cover);
			if (ret < 0) goto err1;
			u32 buttons = Wpad_GetButtons();
			if (buttons & CFG.button_cancel.mask) {
				header = gameList[gameSelected];
				Gui_DrawCover(header.id);
				return;
			}
			VIDEO_WaitVSync();
		} while(!(cover & 0x2));
	}

	// Open disc
	printf_x(gt("Opening DVD disc..."));

	ret = Disc_Open();
	if (ret < 0) {
		printf("\n");
		printf_(gt("ERROR! (ret = %d)"), ret);
		printf("\n");
		goto out;
	} else {
		printf(" ");
		printf(gt("OK!"));
		printf("\n");
	}
	
	if (Disc_IsGC() == 0) {
	
		Disc_ReadGCHeader(&header);
		u64 real_size = 0;

		Gui_DrawCover(header.id);
		
		if (getHeaderById(gameList, gameCnt, header.id, header.disc)) {
			FgColor(CFG.color_footer);
			__Menu_PrintInfo2(&header, GC_GAME_SIZE, real_size);
			DefaultColor();
			printf_x(gt("ERROR: Game already installed!!"));
			printf("\n\n");
			goto out;
		}
		
		// TODO Speicherplatz auch auf USB prüfen
		int installDevice = WBFS_MIN_DEVICE;
		/* Ask user for device */
		for (;;) {
			//Enable the console
			//Gui_Console_Enable();

			/* Clear console */
			Con_Clear();
				
			FgColor(CFG.color_header);
			printf_x(gt("Select device:"));
			printf("\n");
			DefaultColor();
			printf_("< %s >\n\n", get_dev_name(installDevice));

			FgColor(CFG.color_help);
			printf_(gt("Press %s/%s to select device."), (button_names[NUM_BUTTON_LEFT]), (button_names[NUM_BUTTON_RIGHT]));
			printf("\n");
			printf_(gt("Press %s button to continue."), (button_names[CFG.button_confirm.num]));
			printf("\n");
			printf_(gt("Press %s button to cancel."), (button_names[CFG.button_cancel.num]));
			printf("\n");
			printf("\n");

			//Print_SYS_Info();

			u32 buttons = Wpad_WaitButtonsCommon();

			/* LEFT/RIGHT buttons */
			if (buttons & WPAD_BUTTON_LEFT) {
				if ((--installDevice) < WBFS_MIN_DEVICE)
					installDevice = WBFS_MAX_DEVICE;
			}
			if (buttons & WPAD_BUTTON_RIGHT) {
				if ((++installDevice) > WBFS_MAX_DEVICE)
					installDevice = WBFS_MIN_DEVICE;
			}

			/* A button */
			if (buttons & CFG.button_confirm.mask)
				break;

			// B button
			if (buttons & CFG.button_cancel.mask) {
				goto out2;
			}
		}
		
		f32 free, used, total;
		if (installDevice == WBFS_DEVICE_USB) {
			WBFS_DiskSpace(&used, &free);
		} else if (installDevice == WBFS_DEVICE_SDHC) {
			SD_DiskSpace(&used, &free);
		}
		total = used + free;
		printf_x(installDevice == WBFS_DEVICE_USB ? "usb: " : "sd: ");
		printf(gt("%.1fGB free of %.1fGB"), free, total);
		printf("\n\n");

		bench_io();

		// require +128kb for operating safety
		if ((f32)GC_GAME_SIZE + (f32)128*1024 >= free * GB_SIZE) {
			printf_x(gt("ERROR: not enough free space!!"));
			printf("\n\n");
			goto out;
		}
		
		printf("\n");
		FgColor(CFG.color_footer);
		__Menu_PrintInfo2(&header, GC_GAME_SIZE, real_size);
		DefaultColor();
		printf("\n");
		printf("\n");
		printf_h(gt("Press %s button to continue."), (button_names[CFG.button_confirm.num]));
		printf("\n");
		printf_h(gt("Press %s button to go back."), (button_names[CFG.button_cancel.num]));
		printf("\n");
		DefaultColor();
		for (;;) {
			u32 buttons = Wpad_WaitButtonsCommon();
			if (buttons & CFG.button_confirm.mask) break;
			if (buttons & CFG.button_cancel.mask) {
				goto out2;
			}
		}

		printf_x(gt("Installing game, please wait..."));
		printf("\n\n");
		ret = Disc_DumpGCGame(installDevice == WBFS_DEVICE_SDHC);
		goto out_did_install;	//join common code after tried installing 
	}
	
	// GC installing is ok, Wii won't work because no HDD is mounted
	if (!MountWBFS) {
		printf_x(gt("ERROR: WBFS not mounted!"));
		printf("\n");
		printf_x(gt("Can't install Wii games!"));
		printf("\n");

		goto out;
	}

	/* Check disc */
	ret = Disc_IsWii();
	if (ret < 0) {
		printf_x(gt("ERROR: Not a Wii disc!!"));
		printf("\n");
		goto out;
	}

	/* Read header */
	Disc_ReadHeader(&header);
	u64 comp_size = 0, real_size = 0;
	WBFS_DVD_Size(&comp_size, &real_size);

	Gui_DrawCover(header.id);
	
	printf("\n");
	__Menu_PrintInfo2(&header, comp_size, real_size);

	// force fat freespace update when installing
	extern int wbfs_fat_vfs_have;
	wbfs_fat_vfs_have = 0;
	// Disk free space
	f32 free, used, total;
	WBFS_DiskSpace(&used, &free);
	total = used + free;
	//printf_x("WBFS: %.1fGB free of %.1fGB\n\n", free, total);
	printf_x("%s: ", CFG.partition);
	printf(gt("%.1fGB free of %.1fGB"), free, total);
	printf("\n\n");

	bench_io();

	if (check_dual_layer(real_size, NULL)) print_dual_layer_note();

	/* Check if game is already installed */
	int way_out = 0;
	ret = WBFS_CheckGame(header.id);
	if (ret) {
		printf_x(gt("ERROR: Game already installed!!"));
		printf("\n\n");
		//goto out;
		way_out = 1;
	}
	// require +128kb for operating safety
	if ((f32)comp_size + (f32)128*1024 >= free * GB_SIZE) {
		printf_x(gt("ERROR: not enough free space!!"));
		printf("\n\n");
		//goto out;
		way_out = 1;
	}
	// check ntfs write
	if (wbfs_part_fs == PART_FS_NTFS && CFG.ntfs_write == 0) {
		print_ntfs_write_error();
		printf("\n");
		//goto out;
		way_out = 1;
	}

	// get confirmation
	retry:
	if (!way_out) {
		printf_h(gt("Press %s button to continue."), (button_names[CFG.button_confirm.num]));
		printf("\n");
	}
	printf_h(gt("Press %s button to dump BCA."), (button_names[CFG.button_other.num]));
	printf("\n");
	printf_h(gt("Press %s button to go back."), (button_names[CFG.button_cancel.num]));
	printf("\n");
	DefaultColor();
	for (;;) {
		u32 buttons = Wpad_WaitButtonsCommon();
		if (!way_out)
			if (buttons & CFG.button_confirm.mask) break;
		if (buttons & CFG.button_cancel.mask) {
			goto out2;
		}
		if (buttons & CFG.button_other.mask) {
			Menu_DumpBCA(header.id);
			if (way_out) {
				goto out2;
			}
			Con_Clear();
			printf_x(gt("Install game"));
			printf("\n\n");
			__Menu_PrintInfo2(&header, comp_size, real_size);
			printf_x(gt("WBFS: %.1fGB free of %.1fGB"), free, total);
			printf("\n\n");
			goto retry;
		}
	}

	printf_x(gt("Installing game, please wait..."));
	printf("\n\n");

	// Pause the music
	Music_Pause();

	/* Install game */
	ret = WBFS_AddGame();

out_did_install:

	// UnPause the music
	Music_UnPause();

	if (ret < 0) {
		printf_x(gt("Installation ERROR! (ret = %d)"), ret);
		printf("\n");
		goto out;
	}

	/* Remember the game id because reload will free the memory its currently using */
	u8 new_game_id[6];
	memcpy(new_game_id, header.id, 6);

	/* Reload entries */
	__Menu_GetEntries();
	
	/* Set just installed game as selected */
	for (i=0; i<gameCnt; i++) {
		if (strncmp((char*)gameList[i].id, (char*)new_game_id, 6) == 0) {
			gameSelected = i;
			break;
		}
	}

	/* Turn on the Slot Light */
	wiilight(1);
	
	bool coversdone = false;
out:
	// clear buttons status
	WPAD_Flush(WPAD_CHAN_ALL);
	if (!coversdone) {
		printf("\n");
		printf_h(gt("Press button %s to download covers."), (button_names[CFG.button_other.num]));
	}
	printf("\n");
	printf_h(gt("Press button %s to eject DVD."), (button_names[CFG.button_confirm.num]));
	printf("\n");
	printf_h(gt("Press any button to continue..."));
	printf("\n\n");

	/* Wait for any button */
	u32 buttons = Wpad_WaitButtons();

	/* Turn off the Slot Light */
	wiilight(0);

	// button A
	if (buttons & CFG.button_confirm.mask) {
		printf_(gt("Ejecting DVD..."));
		printf("\n");
		WDVD_Eject();
		sleep(1);
	} else if (!coversdone && (buttons & CFG.button_other.mask)) {
		Download_Cover((char *)new_game_id, true, true);
		coversdone = true;
		Gui_DrawCover(header.id);
		goto out;
	}
out2:
	printf("\n");
	printf_(gt("Stopping DVD..."));
	WDVD_StopMotor();
	printf(gt("OK"));
	Gui_DrawCover(gameList[gameSelected].id);
}

void Menu_Remove(void)
{
	struct discHdr *header = NULL;

	s32 ret;

#ifdef FAKE_GAME_LIST
	fake_games--;
	__Menu_GetEntries();
	return;
#endif

	if (CFG.disable_remove) return;

	/* No game list */
	if (!gameCnt)
		return;

	/* Selected game */
	header = &gameList[gameSelected];

	/* Clear console */
	Con_Clear();

	FgColor(CFG.color_header);
	printf_x(gt("Are you sure you want to remove this\n"
			"game?"));
	printf("\n\n");
	DefaultColor();

	/* Show game info */
	__Menu_PrintInfo(header);

	if (!Menu_Confirm(0)) return;
	printf("\n\n");

	printf_x(gt("Removing game, please wait..."));
	fflush(stdout);

	/* Remove game */
	if (header->magic == GC_GAME_ON_DRIVE) {
		ret = DML_RemoveGame(*header);
	} else if (header->magic == CHANNEL_MAGIC) {
		ret = Channel_RemoveGame(header);
	} else {
		ret = WBFS_RemoveGame(header->id);
		if (ret < 0) {
			printf("\n");
			printf_(gt("ERROR! (ret = %d)"), ret);
			printf("\n");
			goto out;
		} else {
			printf(" ");
			printf(gt("OK!"));
		}
	}
	
	/* Reload entries */
	__Menu_GetEntries();

out:
	printf("\n");
	Menu_PrintWait();
}

extern s32 __WBFS_ReadDVD(void *fp, u32 lba, u32 len, void *iobuf);
extern int block_used(u8 *used,u32 i,u32 wblk_sz);

void bench_io()
{
	if (CFG.debug != 8) return;
	//int buf[64*1024/4];
	//int buf[32*1024/4];
	int size_buf = 128*1024;
	int size_total = 64*1024*1024;
	//int size_total = 256*1024;
	void *buf = allocate_memory(size_buf);
	int i, ret;
	int ms = 0;
	int count = 0;
	int count_max = size_total / size_buf;
	int size;
	wiidisc_t *d = 0;
	u8 *used = 0;
	int n_wii_sec_per_disc = 143432*2;//support for double layers discs..
	int wii_sec_sz = 0x8000;
	int n_wii_sec_per_buf = size_buf / wii_sec_sz;

	//printf("\nread: %d\n", n_wii_sec_per_buf); __console_flush(0);
	printf("\n");
	printf(gt("Running benchmark, please wait"));
	printf("\n");
	__console_flush(0);

	used = calloc(n_wii_sec_per_disc,1);
	d = wd_open_disc(__WBFS_ReadDVD, NULL);
	if (!d) { printf(gt("unable to open wii disc")); goto out; }
	wd_build_disc_usage(d,ALL_PARTITIONS,used);
	wd_close_disc(d);
	d = 0;
	printf(gt("linear...")); printf("\n"); __console_flush(0);
	dbg_time1();
	for (i=0; i<n_wii_sec_per_disc / n_wii_sec_per_buf; i++) {
		if (block_used(used,i,n_wii_sec_per_buf)) {
			u32 offset = i * (size_buf>>2);
			ret = __WBFS_ReadDVD(NULL,offset, size_buf,buf);
			if (ret) {
				printf(" %d = %d\n", i, ret); __console_flush(0);
				Wpad_WaitButtonsCommon();
			}
			count++;
			if (count >= count_max) break;
		}
	}
	ms = dbg_time2("64mb read time (ms):");
	size = count * size_buf;
	printf("\n");
	printf(gt("linear read speed: %.2f mb/s"),
			(float)size / ms * 1000 / 1024 / 1024);
	printf("\n");

	count = 0;
	printf(gt("random...")); printf("\n"); __console_flush(0);
	dbg_time1();
	for (i = n_wii_sec_per_disc / n_wii_sec_per_buf - 1; i>0; i--) {
		if (block_used(used,i,n_wii_sec_per_buf)) {
			u32 offset = i * (size_buf>>2);
			ret = __WBFS_ReadDVD(NULL,offset, size_buf,buf);
			if (ret) {
				printf(" %d = %d\n", i, ret); __console_flush(0);
				Wpad_WaitButtonsCommon();
			}
			count++;
			if (count >= count_max) break;
		}
	}
	ms = dbg_time2("64mb read time (ms):");
	size = count * size_buf;
	printf("\n");
	printf(gt("random read speed: %.2f mb/s"),
			(float)size / ms * 1000 / 1024 / 1024);
	printf("\n");
out:
	printf("\n");
	Menu_PrintWait();
	//exit(0);
}

void FmtGameInfoLong(u8 *id, int cols, char *game_desc, int size)
{
	*game_desc = 0;
	struct gameXMLinfo *g = get_game_info_id(id);
	if (!g) return;
	if (!LoadGameInfoFromXML(id)) return;
	FmtGameInfo(game_desc, cols, size);
	strappend(game_desc, "\n", size);
	int i, n;
	int req;
	const char *s;
	// required controllers
	n = 0;
	for (i=0; i<XML_NUM_ACCESSORY; i++) {
		s = getControllerName(g, i, &req);
		if (!s) break;
		if (!req) continue;
		if (n == 0) strappend(game_desc, "(", size);
		if (n > 0) strappend(game_desc, ", ", size);
		strappend(game_desc, s, size);
		n++;
	}
	if (n) strappend(game_desc, ")", size);
	// optional controllers
	n = 0;
	for (i=0; i<XML_NUM_ACCESSORY; i++) {
		s = getControllerName(g, i, &req);
		if (!s) break;
		if (req) continue;
		if (n == 0) strappend(game_desc, " [", size);
		if (n > 0) strappend(game_desc, ", ", size);
		strappend(game_desc, s, size);
		n++;
	}
	if (n) strappend(game_desc, "]", size);
	strappend(game_desc, "\n", size);

	// genre
	if (strcmp(g->genre,"") != 0) {
		char tok[100], *p;
		p = g->genre;
		while (p) {
			p = split_token(tok, p, ',', sizeof(tok));
			for (i=0; i<genreCnt; i++) {
				if (!strcmp(genreTypes[i][0],tok)) {		//if found right genre
					strappend(game_desc, genreTypes[i][1], size);
					break;
				}
				if (i == genreCnt -1)		//went through the whole list with no match
					strappend(game_desc, tok, size);	//add the origonal token
			}
			if (p)
				strappend(game_desc, ", ", size);	//add seperator
		}
		strappend(game_desc, "\n", size);
	}

	// synopsis
	if (g->synopsis) {
		strappend(game_desc, "\n", size);
		strappend(game_desc, g->synopsis, size);
		if (strlen(g->synopsis) == XML_MAX_SYNOPSIS - 1) {
			// mark that the text was cut and there's more
			strappend(game_desc, " ....", size);
		}
	}
	extern char * unescape(char *input, int size);
	unescape(game_desc, size);
	char dots[4] = {0xE2, 0x80, 0xA6, 0};
	str_replace_all(game_desc, dots, "...", size);
	str_replace_all(game_desc, "\t", " ", size); // tabs
	str_replace_all(game_desc, "  ", " ", size); // multiple spaces
}

void Menu_Boot(bool disc)
{
	/* Clear console */
	if (!CFG.direct_launch) {
		Con_Clear();
	}
	bool redraw_cover = false;
restart_menu_boot:;
	static struct discHdr *disc_header = NULL;
	struct discHdr *header;
	bool gc = false;
	if (disc) {
		char *nocover = "ZZZZZZ";
		Gui_DrawCover((u8 *)nocover);
		if (!disc_header) {
			disc_header = (struct discHdr *)allocate_memory(sizeof(struct discHdr));
		}
		memset(disc_header, 0, sizeof(struct discHdr));
		header = disc_header;
	} else {
		header = &gameList[gameSelected];
	}
	s32 ret;
	struct Game_CFG_2 *game_cfg = NULL;
	u64 comp_size = 0, real_size = 0;

	/* No game list */
	if (!disc && !gameCnt)
		return;

	if (disc) {
		Con_Clear();
		/* Disable WBFS mode */
		Disc_SetWBFS(0, NULL);
	
		// Wait for disc
		u32 cover = 0;
		ret = WDVD_GetCoverStatus(&cover);
		if (ret < 0) {
			err1:
			printf("\n");
			printf_(gt("ERROR! (ret = %d)"), ret);
			printf("\n");
			goto out;
		}
		if (!(cover & 0x2)) {
			printf_(gt("Please insert a game disc..."));
			printf("\n\n");
			printf_h(gt("Press %s to return"), (button_names[CFG.button_cancel.num]));
			printf("\n\n");
			do {
				ret = WDVD_GetCoverStatus(&cover);
				if (ret < 0) goto err1;
				u32 buttons = Wpad_GetButtons();
				if (buttons & CFG.button_cancel.mask) goto close;
				VIDEO_WaitVSync();
			} while(!(cover & 0x2));
		}
	
		// Open disc
		printf_x(gt("Opening DVD disc..."));
	
		ret = Disc_Open();
		if (ret < 0) {
			printf("\n");
			printf_(gt("ERROR! (ret = %d)"), ret);
			printf("\n");
			goto out;
		} else {
			printf(" ");
			printf("OK!");
			printf("\n");
		}
	
		/* Check disc */
		ret = Disc_IsWii();
		if (ret < 0) {
			ret = Disc_IsGC();
			if (ret < 0) {
				printf_x(gt("ERROR: Not a Wii disc!!"));
				printf("\n");
				goto out;
			} else {
				gc = true;
			}
		}
		
		/* Read header */
		Disc_ReadHeader(header);
		Gui_DrawCover(header->id);
	}
	game_cfg = CFG_find_game(header->id);

	// Get game size
	if (!disc && (header->magic != GC_GAME_ON_DRIVE)&& (header->magic != CHANNEL_MAGIC))
		WBFS_GameSize2(header->id, &comp_size, &real_size);
	bool dl_warn = check_dual_layer(real_size, game_cfg);
	bool can_skip = !CFG.confirm_start && !dl_warn && check_device(game_cfg, false);
	bool do_skip = (disc && !CFG.confirm_start) || (!disc && can_skip);


	SoundInfo snd;
	u8 banner_title[84];
	bool banner_playing = false;
	bool banner_parsed = false;
	memset(banner_title, 0, sizeof(banner_title));
	memset(&snd, 0, sizeof(snd));

	if (do_skip) {
		printf("\n");
		WBFS_Banner(header->id, &snd, banner_title, !do_skip,
				CFG_read_active_game_setting(header->id).write_playlog);
		/* Show game info */
		__Menu_PrintInfo(header);
		__console_flush(0);
		goto skip_confirm;
	}

	char game_desc[XML_MAX_SYNOPSIS * 2] = "";
	int page = 0;
	int num_pages = 0;
	int cols, rows;
	CON_GetMetrics(&cols, &rows);
	cols -= 1;
	rows -= 11;
	if (rows < 2) rows = 2;
	FmtGameInfoLong(header->id, cols, game_desc, sizeof(game_desc));
	con_wordwrap(game_desc, cols, sizeof(game_desc));

	Gui_Console_Enable();
	if (redraw_cover) {
		__Menu_ShowCover();
		redraw_cover = false;
	}
L_repaint:
	Con_Clear();
	FgColor(CFG.color_header);
	printf_x(gt("Start this game?"));
	printf("\n");
	DefaultColor();

	/* Show game info */
	if (disc) {
		printf_("DISC:\n");
		printf_("%s\n", get_title(header));
		printf_("(%.6s)\n\n", header->id);
	} else {
		printf("\n");
		__Menu_PrintInfo(header);
	}
	//__Menu_ShowGameInfo(false, header->id); /* load game info from XML */
	//printf("\n\n");
	FgColor(CFG.color_inactive);
	print_page(game_desc, rows, page, &num_pages);
	if (num_pages > 1) {
		const char *page_str = gt("page");
		int pad = cols - 8 - con_len(page_str);
		printf("%*s %s: %d/%d", pad, "", page_str, page + 1, num_pages );
	}
	printf("\n");
	DefaultColor();

	//Does DL warning apply to launching discs too? Not sure
	if (!disc && header->magic != GC_GAME_ON_DRIVE) {
		check_device(game_cfg, true);
		if (dl_warn) print_dual_layer_note();
	}
	
	// parse banner sound
	if (!banner_parsed) {
		__console_flush(0);
		if (header->magic == GC_GAME_ON_DRIVE) {
			parse_riff(&gc_wav, &snd);
		} else if (header->magic == CHANNEL_MAGIC) {
			CHANNEL_Banner(header, &snd);
		} else {
			WBFS_Banner(header->id, &snd, banner_title, !do_skip,
				CFG_read_active_game_setting(header->id).write_playlog);
		}
		banner_parsed = true;
	}

	printf_h(gt("Press %s button to continue."), (button_names[CFG.button_confirm.num]));
	printf("\n");
	printf_h(gt("Press %s button to go back."), (button_names[CFG.button_cancel.num]));
	printf("\n");
	printf_h(gt("Press %s button for options."), (button_names[CFG.button_other.num]));
	printf("\n");

	//printf_h(gt("Press %s/%s to scroll description."),
	//		button_names[NUM_BUTTON_UP], button_names[NUM_BUTTON_DOWN]);
	//printf("\n");
	__console_flush(0);

	// play banner sound
	if (snd.dsp_data && !banner_playing) {
		// pause mp3
		Music_PauseVoice(true);
		int fmt = (snd.channels == 2) ? VOICE_STEREO_16BIT : VOICE_MONO_16BIT;
		SND_SetVoice(1, fmt, snd.rate, 0,
			snd.dsp_data, snd.size,
			255,255, //volume,volume,
			NULL); //DataTransferCallback
		banner_playing = true;
	}

	/* Wait for user answer */
	u32 buttons;
	for (;;) {
		buttons = Wpad_WaitButtons();
		if (buttons & CFG.button_confirm.mask) break;
		if (buttons & CFG.button_cancel.mask) break;
		if (buttons & CFG.button_other.mask) break;
		if (buttons & CFG.button_exit.mask) break;
		if (buttons & WPAD_BUTTON_UP) {
			page--;
			if (page < 0) page = 0;
			goto L_repaint;
		}
		if (buttons & WPAD_BUTTON_DOWN) {
			page++;
			if (page >= num_pages) page = num_pages - 1;
			if (page < 0) page = 0;
			goto L_repaint;
		}
		if (!disc) {
			if (buttons & WPAD_BUTTON_LEFT) break;
			if (buttons & WPAD_BUTTON_RIGHT) break;
		}
	}

	// stop banner sound, resume mp3
	if (snd.dsp_data && banner_playing) {
		SND_StopVoice(1);
		SAFE_FREE(snd.dsp_data);
		if (buttons & CFG.button_confirm.mask) {
			// mute mp3
			Music_Mute(true);
		}
		if ( (buttons & WPAD_BUTTON_LEFT) || (buttons & WPAD_BUTTON_RIGHT) ) {
			// don't unpause mp3 when changing games
		} else {
			// unpause mp3
			Music_PauseVoice(false);
		}
		banner_playing = false;
	}

	if (buttons & CFG.button_cancel.mask) goto close;
	if (buttons & CFG.button_exit.mask) {
		Handle_Home();
		return;
	}
	if (buttons & CFG.button_other.mask) {
		if (disc) Menu_Boot_Options(header, 1);
		else Menu_Options();
		return;
	}
	// LEFT/RIGHT: prev/next game
	if (buttons & WPAD_BUTTON_LEFT) {
		__Menu_MoveList(-1);
		redraw_cover = true;
		goto restart_menu_boot;
	}
	if (buttons & WPAD_BUTTON_RIGHT) {
		__Menu_MoveList(+1);
		redraw_cover = true;
		goto restart_menu_boot;
	}

	// A button: continue to boot
	printf("\n");

	skip_confirm:

	get_time(&TIME.run1);

	if (game_cfg) {
		CFG.game = game_cfg->curr;
		if (CFG.game.ios_idx != CFG_IOS_AUTO) {
			cfg_ios_set_idx(CFG.game.ios_idx);
		} else {
			tmd game_tmd;
			memset(&game_tmd, 0, sizeof(game_tmd));
			if (header->magic == CHANNEL_MAGIC) {
				game_tmd.sys_version = getChannelReqIos(header); //store channel ios directly in sys_version for compatability
			}
			else if (header->magic == GC_GAME_ON_DRIVE) {
														//do nothing
			} else {
				wbfs_disc_t* d = WBFS_OpenDisc(header->id);
				if (d) {
					wbfs_extract_tmd(d, &game_tmd);
					WBFS_CloseDisc(d);
				}
			}
			if (game_tmd.sys_version) {
				set_recommended_cIOS_idx(TITLE_LOW(game_tmd.sys_version), false);
			}
		}
		//dbg_printf("set ios: %d idx: %d\n", CFG.ios, CFG.game.ios_idx);
		
	    /* Check for stub IOS */
        ret = checkIOS(CFG.ios);
        
        if (ret == -1) {
		    printf_x(gt("ERROR:"));
		    printf("\n");
		    printf_(gt("Custom IOS %d is a stub!\nPlease reinstall it."), CFG.ios);
		    printf("\n");
			printf_(gt("Press any button..."));
			printf("\n");

			/* Wait for button */
			Wpad_WaitButtonsCommon();
		    goto close;
        } else if (ret == -2) {
		    printf_x(gt("ERROR:"));
		    printf("\n");
			printf_(gt("Custom IOS %d could not be found!\nPlease install it."), CFG.ios);
		    printf("\n");
			printf_(gt("Press any button..."));
			printf("\n");

			/* Wait for button */
			Wpad_WaitButtonsCommon();
		    goto close;
	    }
	}

	if (!disc) {
		if (CFG.game.alt_dol == ALT_DOL_DISC && *CFG.game.dol_name == 0) {
			// show alt dol
			Menu_Alt_Dol(header, &CFG.game, 0);
		}
	}

	get_time(&TIME.playlog1);
	if (header->magic == GC_GAME_ON_DRIVE || header->magic == CHANNEL_MAGIC) CFG.game.write_playlog = 0;
	if (CFG.game.write_playlog && set_playrec(header->id, banner_title) < 0) {
		printf_(gt("Error storing playlog file.\nStart from the Wii Menu to fix."));
		printf("\n");
		printf_h(gt("Press %s button to exit."), (button_names[CFG.button_exit.num]));
		printf("\n");
		if (!Menu_Confirm(0)) return;
	}
	get_time(&TIME.playlog2);

	get_time(&TIME.gcard1);
	if (gamercard_update((char *)(header->id))) {
		printf_h(gt("Press %s button to exit."), (button_names[CFG.button_exit.num]));
		printf("\n");
		if (!Menu_Confirm(0)) return;
	}
	get_time(&TIME.gcard2);
	
	printf("\n");
	printf_x(gt("Booting game, please wait..."));
	printf("\n\n");
	
	if (header->magic == CHANNEL_MAGIC) {

		get_time(&TIME.playstat1);
		setPlayStat(header->id); //I'd rather do this after the check, but now you unmount fat before that ;)
		get_time(&TIME.playstat2);

		cfg_ios_set_idx(CFG.game.ios_idx);
		char args[255][255] = {{"\0"}};
		int i = 0;
		sprintf(args[i++], "--ios=%d", CFG.ios);
		if (strstr(CFG.nand_emu_path, "usb:")) {
			strcpy(args[i++], "--auto=USB");
		} else {
			strcpy(args[i++], "--auto=SD");
		}
		sprintf(args[i++], "--path=%s", strchr(CFG.nand_emu_path, '/'));
		sprintf(args[i++], "--gameIdLower=%02x%02x%02x%02x", header->id[0], header->id[1], header->id[2], header->id[3]);
		sprintf(args[i++], "--videoMode=%d", CFG.game.video);
		sprintf(args[i++], "--language=%d", CFG.game.language);
		strcpy(args[i++], "--bootMethod=0");
		if ((CFG.game.ocarina == 1) && (strncmp(USBLOADER_PATH,"usb",3) == 0))
			CFG.game.ocarina = 2;	// 1 is enabled on sd: , 2 is enabled on usb:
		sprintf(args[i++], "--ocarina=%d", CFG.game.ocarina);
		sprintf(args[i++], "--hooktype=%d", CFG.game.hooktype);
		strcpy(args[i++], "--videoPatch=0");
		strcpy(args[i++], "--debugger=0");
		Menu_Plugin(CFG.game.channel_boot, args, i);
		return;
	}
	
	//if we need to copy GC file
	if (header->magic == GC_GAME_ON_DRIVE && !is_gc_game_on_bootable_drive(header)) {
		char gamePath[255];
		char drive[255];
		
		sprintf(drive, "%s/", dm_boot_drive);
		if (!fsop_DirExist(drive)) {
			printf_x(gt("ERROR: the drive or partition %s/ is not connected!!"), dm_boot_drive);
			printf("\n\n");
			goto out;
		}
		
		sprintf(gamePath, "%s/games", dm_boot_drive);
		if (!fsop_DirExist(gamePath))
		{
			fsop_MakeFolder(gamePath);
		}
		
		s32 del = delete_Old_Copied_DML_Game();
		
		f32 free, used, total;
		if (!strncmp("sd:", dm_boot_drive, 3))
			SD_DiskSpace(&used, &free);
		else
			FAT_DiskSpace(&used, &free);
		
		total = used + free;
		printf_x("%s ", dm_boot_drive);
		printf(gt("%.1fGB free of %.1fGB"), free, total);
		printf("\n\n");

		bench_io();

		// require +128kb for operating safety
		if ((f32)getDMLGameSize(header) + (f32)128*1024 >= free * GB_SIZE) {
			printf_x(gt("ERROR: not enough free space!!"));
			printf("\n\n");
			if (del >= 0) __Menu_GetEntries();
			goto out;
		}
		
		copy_DML_Game_to_SD(header);
	}
	
	if (gc || (header->magic == GC_GAME_ON_DRIVE && is_gc_game_on_bootable_drive(header)))
	{
		get_time(&TIME.playstat1);
		setPlayStat(header->id); //I'd rather do this after the check, but now you unmount fat before that ;)
		get_time(&TIME.playstat2);

		memcpy((char *)0x80000000, header->id, 6);
	
		dbg_printf("Setting language...\n");
		if (CFG.game.language > 1 && CFG.game.language < 8)
			GC_SetLanguage(CFG.game.language-1);
		else
			GC_SetLanguage(0);
		
		char cheatPath[255];
		char newCheatPath[255];
		sprintf(newCheatPath, "%s/%.6s.gct", header->path, header->id);
		
		if (CFG.game.ocarina) {
			FILE *fp;
			char *fat_drive = FAT_DRIVE;
			int i;

			printf_x(gt("Ocarina: Searching codes..."));
			printf("\n");

			for (i=0; i<3; i++) {
				switch (i) {
				case 0:
					snprintf(cheatPath, sizeof(cheatPath), "%s/codes/%.6s.gct",
						USBLOADER_PATH, header->id);
					break;
				case 1:
					snprintf(cheatPath, sizeof(cheatPath), "%s/data/gecko/codes/%.6s.gct",
						fat_drive, header->id);
					break;
				case 2:
					snprintf(cheatPath, sizeof(cheatPath), "%s/codes/%.6s.gct",
						fat_drive, header->id);
					break;
				}
				printf("    %s\n", cheatPath);
				fp = fopen(cheatPath, "rb");
				if (fp) {
					fclose(fp);
					break;
				}
				if (i == 2 && strcmp(fat_drive, "usb:") == 0) {
					// retry on sd:
					fat_drive = "sd:";
					i = 0;
				}
			}
		}
		
		if ((CFG.game.channel_boot == 3)	 ||	//Boot GC using Nintendont
	        (CFG.game.channel_boot == 0 && CFG.default_gc_loader == 2))
		{
			Nintendont_set_options(header, cheatPath, newCheatPath);

			CFG.ios = 58;
			CFG.game.ios_idx = 58;
//			IOSPATCH_Apply();
			char args[255][255] = {{"\0"}};
			int i = 0;
			Menu_Plugin(PLUGIN_NINTENDONT, args, i);
			return;	// should never get here

		}

		if ((CFG.game.channel_boot == 2) || //boot GC using Devolution
		    (CFG.game.channel_boot == 1 && CFG.dml == CFG_MIOS) ||
	        (CFG.game.channel_boot == 0 && CFG.default_gc_loader == 1))
		{
			if (getDMLDisk1Size(header) != GC_GAME_SIZE) {
				printf_x(gt("Devolution only accepts clean dumps!\n"));
				Wpad_WaitButtons();
				goto out;
			}
			
			char devoPath[255];	
			char loaderPath[255];
			
			snprintf(devoPath, sizeof(devoPath), "%s/game.iso", header->path);	
			
			DEVO_SetOptions(devoPath, CFG.game.country_patch);
			
			snprintf(D_S(loaderPath), "%s/loader.bin", USBLOADER_PATH);
			sleep(1);
			u8 *loader_bin = NULL;
			FILE *fdev = fopen( loaderPath, "rb");
			if (fdev)
			{
				fseek(fdev, 0, SEEK_END);
				u32 size = ftell(fdev);
				fseek(fdev, 0, SEEK_SET);
				loader_bin = (u8*)mem2_alloc(size);
				fread(loader_bin, 1, size, fdev);
				fclose(fdev);
			}
			sleep(1);
			
			UnmountAll(NULL);
			Services_Close();
			Wpad_Disconnect();
			
			if(header->id[3] == 'P')
				GC_SetVideoMode(1, true);
			else
				GC_SetVideoMode(2, true);

			extern void __exception_closeall();
			  IRQ_Disable();
			__IOS_ShutdownSubsystems();
			__exception_closeall();
			#define LAUNCH() ((void(*)(void))loader_bin)()
			LAUNCH();
		} 
		
		//Start GC using DIOS MIOS
		dbg_printf("Setting video mode...\n");
		if(CFG.game.video == 0)
		{
			if(header->id[3] == 'P')
				GC_SetVideoMode(1, false);
			else
				GC_SetVideoMode(2, false);
		} else
			GC_SetVideoMode(CFG.game.video, false);
		
		dbg_printf("Check DIOS MIOS version...\n");
		if(header->magic == GC_GAME_ON_DRIVE)
		{
			if (CFG.dml == CFG_DML_R51) {
				DML_Old_SetOptions(header->path, cheatPath, newCheatPath, CFG.game.ocarina);
			} else {
				DML_New_SetOptions(header->path, cheatPath, newCheatPath, CFG.game.ocarina, false, CFG.game.country_patch, CFG.game.nodisc, CFG.game.vidtv, CFG.game.video, CFG.game.wide_screen, CFG.game.hooktype, CFG.game.block_ios_reload, CFG.game.screenshot);
			}
		} else if (CFG.dml >= CFG_DML_1_2) {
			DML_New_SetBootDiscOption(cheatPath, newCheatPath, CFG.game.ocarina, CFG.game.country_patch, CFG.game.vidtv, CFG.game.video);
		}

		if (CFG.game.ntsc_j_patch)
			*HW_PPCSPEED = 0x0002A9E0;
		
		dbg_printf("Launching GC game...\n");
		UnmountAll(NULL);
		Services_Close();
		Subsystem_Close();
		Wpad_Disconnect();

		WII_Initialize();
		WII_LaunchTitle(0x0000000100000100ULL);
		return;
	}

	// If fat, open wbfs disc and verfy id as a consistency check
	if (!disc) {
		if (wbfs_part_fs) {
			wbfs_disc_t *d = WBFS_OpenDisc(header->id);
			if (!d) {
				printf_(gt("ERROR: opening %.6s"), header->id);
				printf("\n");
				goto out;
			}
			WBFS_CloseDisc(d);
			/*
			if (!is_ios_idx_mload(CFG.game.ios_idx))
			{
				printf(gt("Switching to IOS222 for FAT support."));
				printf("\n");
				CFG.game.ios_idx = CFG_IOS_222_MLOAD;
				cfg_ios_set_idx(CFG.game.ios_idx);
			}
			*/
		}
	}

	// load stuff before ios reloads & services close
	if (CFG.game.clean != CFG_CLEAN_ALL)
		ocarina_load_code(header->id);
	load_wip_patches(header->id);
	load_bca_data(header->id);
	load_dolmenu((char*)header->id);

	if (!disc) {
		ret = get_frag_list(header->id);
		if (ret) {
			printf_(gt("ERROR: get_frag_list: %d"), ret);
			printf("\n");
			goto out;
		}
	}

	// stop services (music, gui)
	Services_Close();

	get_time(&TIME.playstat1);
	setPlayStat(header->id); //I'd rather do this after the check, but now you unmount fat before that ;)
	get_time(&TIME.playstat2);
	
	if (CFG.game.nand_emu > 0) {
		//! Create save game path and title.tmd for not existing saves and nand emulation is activated
		CreateSavePath(header);
	}
	
	if (CFG.game.alt_dol != 1) {
		// unless we're loading alt.dol from sd
		// unmount everything
		UnmountAll(NULL);
	}

	if (!disc) {
		get_time(&TIME.rios1);
		ret = ReloadIOS(1, 1);
		get_time(&TIME.rios2);
		if (ret < 0) goto out;

		if (wbfs_part_fs != PART_FS_WBFS) {
			load_dip_249();
		}

		Block_IOS_Reload();
		d2x_return_to_channel();
	
		// verify IOS version
		warn_ios_bugs();
		if (check_dual_layer(real_size, NULL)) print_dual_layer_note();
	
		//dbg_time1();
	
		/* Set WBFS mode */
		ret = Disc_SetWBFS(wbfsDev, header->id);
	 	if (ret) {
	 		printf_(gt("ERROR: SetWBFS: %d"), ret);
			printf("\n");
	 		goto out;
	 	}
		
		Setup_NandEmu(CFG.game.nand_emu, CFG.nand_emu_path);
		
		/* Open disc */
		ret = Disc_Open();
	
		//dbg_time2("\ndisc open");
		//Wpad_WaitButtonsCommon();
		bench_io();
	
		if (ret < 0) {
			printf_(gt("ERROR: Could not open game! (ret = %d)"), ret);
			printf("\n");
			goto out;
		}
	}

// printf("CFG language=%d,Wii language=%x\n",CFG.game.language,CONF_GetLanguage());
// sleep(5);
  
	if ((memcmp("SUK",header->id,3)!=0) && (CFG.game.language == 0))	//Kirby's Return to Dream Land 
	{
		if (((header->id[3]=='W') ||(header->id[3]=='J')) && (CONF_GetLanguage()== 1))
			CFG.game.language=1;
		if (((header->id[3]=='E') ||(header->id[3]=='P')) && (CONF_GetLanguage()== 0))
			CFG.game.language=2;
		// printf("GAME ID=%s, language=%d\n",header->id,CFG.game.language);
		// sleep(5);
	}
  
	if (CFG.game.wide_screen > 0)
	{
		wiilight(1);
	}
    
	switch(CFG.game.language)
	{
		// 0 = CFG_LANG_CONSOLE
		case 0: configbytes[0] = 0xCD; break; 
		case 1: configbytes[0] = 0x00; break; 
		case 2: configbytes[0] = 0x01; break; 
		case 3: configbytes[0] = 0x02; break; 
		case 4: configbytes[0] = 0x03; break; 
		case 5: configbytes[0] = 0x04; break; 
		case 6: configbytes[0] = 0x05; break; 
		case 7: configbytes[0] = 0x06; break; 
		case 8: configbytes[0] = 0x07; break; 
		case 9: configbytes[0] = 0x08; break; 
		case 10: configbytes[0] = 0x09; break;
	}

	/* Boot Wii disc */
	ret = Disc_WiiBoot(disc);

	printf_(gt("Returned! (ret = %d)"), ret);
	printf("\n");

out:
	printf("\n");
	printf_(gt("Press any button to exit..."));
	printf("\n");

	/* Wait for button */
	Wpad_Init();
	Wpad_WaitButtonsCommon();
	printf("Exiting...");
	exit(0);
close:
	if (disc) {
		printf("\n");
		printf_(gt("Stopping DVD..."));
		WDVD_StopMotor();
		printf(gt("OK"));
		header = &gameList[gameSelected];
		Gui_DrawCover(header->id);
	}
	return;
}

void Direct_Launch()
{
	int i;

	if (!CFG.direct_launch) return;

	// enable console in case there is some input
	// required when loading game, depends on options, like:
	// confirm ocarina, alt_dol=disk (asks for dol)
	// but this can be moved to those points and removed from here
	// so that console is shown only if needed...
	if (CFG.intro) {
		Gui_Console_Enable();
	}

	// confirm_direct_start
	//CFG.confirm_start = 0;

	//sleep(1);
	Con_Clear();
	printf(gt("Configurable Loader %s"), CFG_VERSION);
	printf("\n\n");
	//sleep(1);
	for (i=0; i<gameCnt; i++) {
		if (strncmp(CFG.launch_discid, (char*)gameList[i].id, 6) == 0) {
			gameSelected = i;
			Menu_Boot(0);
			goto out;
		}
	}
	Gui_Console_Enable();
	printf(gt("Auto-start game: %.6s not found!"), CFG.launch_discid);
	printf("\n");
	printf(gt("Press any button..."));
	printf("\n");
	Wpad_WaitButtons();
	out:
	CFG.direct_launch = 0;
}

void Menu_Loop(void)
{
	// enable the console if starting with console mode
	if (!CFG.gui_start) {
		if (!(CFG.direct_launch && !CFG.intro)) {
			Gui_Console_Enable();
		}
	}
	
	// Get MIOS info
	CFG.dml = get_miosinfo();
	
	fill_base_array();

	/* Device menu */
	Menu_Device();

	// Direct Launch?
	Direct_Launch();

	// Start Music
	get_time(&TIME.mp31);
	Music_Start();
	get_time(&TIME.mp32);

	get_time(&TIME.guitheme1);
	Grx_Init();
	get_time(&TIME.guitheme2);

	// Init Favorites
	Switch_Favorites(CFG.start_favorites || CFG.profile_start_favorites[CFG.current_profile]);

	switch(CFG.select) {
		case CFG_SELECT_PREVIOUS: 
			{
			int i;
			time_t ret, last=0;
			for (i = 0; i < gameCnt; i++) {
				if ((ret = getLastPlay(gameList[i].id)) >= last) {
					last = ret;
					gameSelected = i;
				}
			}
			break;
			}
		case CFG_SELECT_START:
			gameSelected = 0;
			break;
		case CFG_SELECT_MIDDLE:
			gameSelected = (gameCnt-1)/2;
			break;
		case CFG_SELECT_END:
			gameSelected = gameCnt - 1;
			break;
		case CFG_SELECT_MOST:
			{
			int i;
			u32 ret, last=0;
			for (i = 0; i < gameCnt; i++) {
				if ((ret = getPlayCount(gameList[i].id)) >= last) {
					last = ret;
					gameSelected = i;
				}
			}
			break;
			}
		case CFG_SELECT_LEAST:
			{
			int i;
			u32 ret, last=0xFFFFFFFF;
			for (i = 0; i < gameCnt; i++) {
				if ((ret = getPlayCount(gameList[i].id)) < last) {
					last = ret;
					gameSelected = i;
				}
			}
			break;
			}
		case CFG_SELECT_RANDOM:
			gameSelected = (rand() >> 16) % gameCnt;
			break;

	}
	// scroll start list
	__Menu_ScrollStartList();

	get_time(&TIME.boot2);

	//time_stats();
	if (CFG.debug) {
		Menu_PrintWait();
	}

	// Clear console
	// (so that it doesn't show when switching back from gui)
	Con_Clear();
	__console_scroll = 0;

	// Start GUI
	if (CFG.gui_start) {
		go_gui = true;
		goto skip_list;
	}

	/* Menu loop */
	for (;;) {
		/* Clear console */
		Con_Clear();

		/* Show gamelist */
		__Menu_ShowList();

		/* Show cover */
		__Menu_ShowCover();

		//memstat();

		skip_list:
		/* Controls */
		__Menu_Controls();
	}
}



// menu support routines

void menu_init(struct Menu *m, int num_opt)
{
	memset(m, 0, sizeof(*m));
	m->num_opt = num_opt;
}

void menu_begin(struct Menu *m)
{
	m->line_count = 0;
}

void menu_init_active(struct Menu *m, char *active, int active_size)
{
	m->active = active;
	m->active_size = active_size;
	if (!active) return;
	memset(active, 1, active_size);
}

char m_active(struct Menu *m, int i)
{
	if (m->active == NULL) return 1;
	if (i >= m->active_size) return 0;
	if (i < 0) return 0;
	return m->active[i];
}

void menu_jump_active(struct Menu *m)
{
	int i;
	if (m->current >= m->num_opt) m->current = m->num_opt - 1;
	if (m->current < 0) m->current = 0;
	if (m_active(m, m->current)) return;
	// move to first m->active
	for (i=0; i < m->num_opt; i++) {
		if (m_active(m, i)) {
			m->current = i;
			break;
		}
	}
	if (i == m->num_opt) m->current = 0;
}

void menu_move_cap(struct Menu *m)
{
	if (m->current >= m->num_opt) m->current = m->num_opt-1;
	if (m->current < 0) m->current = 0;
}

void menu_move_wrap(struct Menu *m)
{
	if (m->current >= m->num_opt) m->current = 0;
	if (m->current < 0) m->current = m->num_opt-1;
	if (m->current < 0) m->current = 0;
}

void menu_move_adir(struct Menu *m, int dir)
{
	int i;
	menu_move_cap(m);
	//int n = m->current;
	for (i=0; i<m->num_opt; i++) {
		m->current += dir;
		menu_move_wrap(m);
		if (m_active(m, m->current)) break;
	}
}

// only move on active lines
void menu_move_active(struct Menu *m, int buttons)
{
	if (buttons & WPAD_BUTTON_UP) {
		menu_move_adir(m, -1);
	}
	if (buttons & WPAD_BUTTON_DOWN) {
		menu_move_adir(m, 1);
	}
}

// simple, move on all lines
void menu_move(struct Menu *m, int buttons)
{
	int dir = 0;
	menu_move_cap(m);
	if (buttons & WPAD_BUTTON_UP) dir = -1;
	if (buttons & WPAD_BUTTON_DOWN) dir = 1;
	m->current += dir;
	menu_move_wrap(m);
}

void menu_print_mark(struct Menu *m)
{
	//if (m->active[m->line_count] && m->current == m->line_count) {
	if (m->current == m->line_count) {
		BgColor(CFG.color_selected_bg);
		FgColor(CFG.color_selected_fg);
		Con_ClearLine();
	} else if (m_active(m, m->line_count)) {
		DefaultColor();
	} else {
		DefaultColor();
		FgColor(CFG.color_inactive);
	}
	char *xx;
	//if (m->active[m->line_count] && m->current == m->line_count) {
	if (m->current == m->line_count) {
		xx = CFG.cursor;
	} else {
		xx = CFG.cursor_space;
	}
	printf(" %s ", xx);
}

int menu_mark(struct Menu *m)
{
	menu_print_mark(m);
	return m->line_count++;
}


// size is number of visible lines on screen
// num_items is number of items in the list
void menu_window_begin(struct Menu *m, int size, int num_items)
{
	if (num_items > size + 1)
		m->window_size = size;
	else
		m->window_size = size + 1;
	m->window_items = num_items;
	m->window_begin = m->line_count;
	// adjust window_pos so that selected line is inside the window
	// current_pos = position of selected line
	//   (relative offset from window_begin)
	int current_pos = m->current - m->window_begin;
	if (current_pos >= m->window_pos + m->window_size) {
		if (current_pos > num_items) {
			m->window_pos = num_items - m->window_size + 1;
		} else {
			m->window_pos = current_pos - m->window_size + 1;
		}
	} else if (current_pos < m->window_pos) {
		m->window_pos = current_pos;
	}
	if (m->window_pos < 0) m->window_pos = 0;
	// print page continuation marker
	// only if needed
	if (m->window_size < m->window_items) {
		if (m->window_pos > 0) {
			printf(" %s +\n", CFG.cursor_space);
		} else {
			printf("\n");
		}
	}
}

bool menu_window_mark(struct Menu *m)
{
	int pos = m->line_count - m->window_begin;
	if (pos < m->window_pos) {
		m->line_count++;
		return false;
	}

	if (pos >= m->window_pos + m->window_size) {
		m->line_count++;
		return false;
	}

	menu_mark(m);
	return true;
}

void menu_window_end(struct Menu *m, int cols)
{
	int pos = m->line_count - m->window_begin;
	char str[20] = "";
	int len;
	printf(" %s ", CFG.cursor_space);
	if (pos > m->window_pos + m->window_size) {
		printf("+");
	} else {
		printf(" ");
	}
	if (m->window_size < m->window_items)
		sprintf(str, "%s: %d/%d", gt("page"), 
			    (m->current - m->window_begin) / m->window_size + 1,
				(m->window_items-1) / m->window_size + 1);
	len = strlen(str);
	printf("%*.*s", cols-6-len, cols-6-len, str);
	printf("\n");
	// debug
	//printf("c:%d s:%d b:%d i:%d p:%d\n", m->current, m->window_size,
	//		m->window_begin, m->window_items, m->window_pos);
}


// indented printf
// will indent also each \n in string
void printf_a(const char *fmt, va_list argp)
{
	char strbuf[512];
	char *s, *n;
	int ret;
	ret = vsnprintf(strbuf, sizeof(strbuf), fmt, argp);
	if (ret >= sizeof(strbuf)) {
		// buffer too small, just print directly
		vprintf(fmt, argp);
	} else {
		// append space also to each \n
		s = strbuf;
		do {
			n = strchr(s, '\n');
			if (n) *n = 0; // terminate new line
			printf("%s", s);
			if (n) {
				printf("\n");
				s = n + 1;
				// only indent if there's more text to print
				// don't indent if \n is at the end of string
				if (*s == 0) break;
				printf("%s", CFG.menu_plus_s);
			}
		} while (n);
	}
}

void printf_(const char *fmt, ...)
{
	va_list argp;
	printf("%s", CFG.menu_plus_s);
	va_start(argp, fmt);
	printf_a(fmt, argp);
	va_end(argp);
}

void printf_x(const char *fmt, ...)
{
	va_list argp;
	printf("%s", CFG.menu_plus);
	va_start(argp, fmt);
	//vprintf(fmt, argp);
	printf_a(fmt, argp);
	va_end(argp);
}

void printf_h(const char *fmt, ...)
{
	va_list argp;
	DefaultColor();
	FgColor(CFG.color_help);
	printf("%s", CFG.menu_plus_s);
	va_start(argp, fmt);
	//vprintf(fmt, argp);
	printf_a(fmt, argp);
	va_end(argp);
	DefaultColor();
}

int Menu_PrintWait()
{
	const char *msg = gt("Press any button to continue...");
	printf_h("%s\n", msg);
	gecko_printf("%s\n", msg);
	// clear button states
	gecko_printf("WPAD_Flush(WPAD_CHAN_ALL)\n");
	WPAD_Flush(WPAD_CHAN_ALL);
	// wait for button press
	gecko_printf("Wpad_WaitButtonsCommon()\n");
	int button = Wpad_WaitButtonsCommon();
	// wait for button release
	while (Wpad_HeldButtons()) {
		gecko_printf(".");
		usleep(10000);
		VIDEO_WaitVSync();
	}
	return button;
}

bool Menu_Confirm(const char *msg)
{
	if (msg) {
		printf_h(gt("Press %s button to %s."), (button_names[CFG.button_confirm.num]), msg);
	} else {
		printf_h(gt("Press %s button to continue."), (button_names[CFG.button_confirm.num]));
	}
	printf("\n");
	printf_h(gt("Press %s button to go back."), (button_names[CFG.button_cancel.num]));
	printf("\n");
	DefaultColor();
	WPAD_Flush(WPAD_CHAN_ALL);
	for (;;) {
		u32 buttons = Wpad_WaitButtonsCommon();
		if (buttons & CFG.button_confirm.mask) return true;
		if (buttons & CFG.button_cancel.mask) return false;
	}
}

void Menu_Plugin(int plugin, char arguments[255][255], int argCnt) {
	FILE* file;
	u32 entryPoint;
	int i = 0;
	void *buffer = (void *)0x92000000;
		
	char fname[128];
	
	struct __argv args;
	bzero(&args, sizeof(args));
	args.argvMagic = ARGV_MAGIC;
	args.length = 1;
	
	for (i = 0; i < argCnt; i++) {
		args.length += strlen(arguments[i]) + 1;
	}
	
	args.commandLine = (char*)allocate_memory(args.length);
	if (!args.commandLine) dbg_printf("failed...\n");
	
	int pos = 0;
	
	for (i = 0; i < argCnt; i++) {
		strcpy(args.commandLine+pos, arguments[i]);
        pos += strlen(arguments[i]) + 1;
	}
	
	args.argc = argCnt;
	args.commandLine[args.length - 1] = '\0';
	args.argv = &args.commandLine;
	args.endARGV = args.argv + 1;
	
	switch (plugin) {
		case PLUGIN_MIGHTY:
			snprintf(fname, sizeof(fname), "%s/plugins/mighty.dol", USBLOADER_PATH);
			break;
		case PLUGIN_NEEK:
			snprintf(fname, sizeof(fname), "%s/plugins/neek2o.dol", USBLOADER_PATH);
			break;
		case PLUGIN_NINTENDONT:
			snprintf(fname, sizeof(fname), "/apps/nintendont/boot.dol");
			break;
		default:
			return;
	}
	
	printf_(gt("Loading..%s\n"), fname);

	file = fopen(fname, "rb");
	
	if(file == NULL) 
	{
		printf_(gt("Not Found boot.dol!"));
		printf("\n");
		sleep(5);
		return;
	}
	
	fseek (file, 0, SEEK_END);
	int len = ftell(file);
	fseek (file, 0, SEEK_SET);
	fread(buffer, 1, len, file);
	fclose (file);
	
	entryPoint = load_dol_image_args(buffer, &args);
	
	ReloadIOS(1,1);
	d2x_return_to_channel();
	
	UnmountAll(NULL);
	Services_Close();
	Subsystem_Close();
	Wpad_Disconnect();
	
	__IOS_ShutdownSubsystems();
	SYS_ResetSystem(SYS_SHUTDOWN,0,0);
	entrypoint exec = (entrypoint)entryPoint;
	exec();
	//__lwp_thread_stopmultitasking((entrypoint)entryPoint);
}

/*
Maybe:

Ocarina (cheats): < Off >

>> Compatibility  < Edit >
   Favorite:      < Yes >
   Hide Game:     < No >
   Cheat Codes:   < Manage >
   Cover Image:   < Download >
   < Start Game >
   < Remove Game >
   < Install Game >
   < Global Options >

*/

