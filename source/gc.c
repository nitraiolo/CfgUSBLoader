#include <gccore.h>
#include <stdio.h>

#include <sdcard/wiisd_io.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <fat.h>

#include "gc.h"
#include "dir.h"
#include "util.h"
#include "gettext.h"
#include "wpad.h"
#include "debug.h"
#include "disc.h"
#include "fileOps.h"
#include "cfg.h"
#include "NintendontConfig.h"

#define MAX_FAT_PATH 1024

#define SRAM_ENGLISH 0
#define SRAM_GERMAN 1
#define SRAM_FRENCH 2
#define SRAM_SPANISH 3
#define SRAM_ITALIAN 4
#define SRAM_DUTCH 5

syssram* __SYS_LockSram();
u32 __SYS_UnlockSram(u32 write);
u32 __SYS_SyncSram(void);

extern char wbfs_fs_drive[16];
char dm_boot_drive[16];

void GC_SetVideoMode(u8 videomode, bool devo)
{
	syssram *sram;
	sram = __SYS_LockSram();
	//void *m_frameBuf;
	static GXRModeObj *rmode;
	int memflag = 0;
	
	if (devo) {
	
		if((VIDEO_HaveComponentCable() && (CONF_GetProgressiveScan() > 0)) && videomode > 3)
			sram->flags |= 0x80; //set progressive flag
		else
			sram->flags &= 0x7F; //clear progressive flag

		if(videomode == 1 || videomode == 3 || videomode == 5)
		{
			memflag = 1;
			sram->flags |= 0x01; // Set bit 0 to set the video mode to PAL
			sram->ntd |= 0x40; //set pal60 flag
		}
		else
		{
			sram->flags &= 0xFE; // Clear bit 0 to set the video mode to NTSC
			sram->ntd &= 0xBF; //clear pal60 flag
		}
		
		if(videomode == 1) {
			rmode = &TVPal528IntDf;
		} else if(videomode == 2) {
			rmode = &TVNtsc480IntDf;
		}
		
		__SYS_UnlockSram(1); // 1 -> write changes
		while(!__SYS_SyncSram());

		/* Set video mode */
		if (rmode != NULL)
			VIDEO_Configure(rmode);

		/* Setup video  */
		VIDEO_SetBlack(TRUE);
		VIDEO_Flush();
		VIDEO_WaitVSync();
		VIDEO_WaitVSync();
		return;
	}

	if((VIDEO_HaveComponentCable() && (CONF_GetProgressiveScan() > 0)) && videomode > 3)
		sram->flags |= 0x80; //set progressive flag
	else
		sram->flags &= 0x7F; //clear progressive flag

	if(videomode == 1 || videomode == 3 || videomode == 5)
	{
		memflag = 1;
		sram->flags |= 0x01; // Set bit 0 to set the video mode to PAL
		sram->ntd |= 0x40; //set pal60 flag
	}
	else
	{
		sram->flags &= 0xFE; // Clear bit 0 to set the video mode to NTSC
		sram->ntd &= 0xBF; //clear pal60 flag
	}

	if(videomode == 1)
		rmode = &TVPal528IntDf;
	else if(videomode == 2)
		rmode = &TVNtsc480IntDf;
	else if(videomode == 3)
	{
		rmode = &TVEurgb60Hz480IntDf;
		memflag = 5;
	}
	else if(videomode == 4)
		rmode = &TVNtsc480Prog;
	else if(videomode == 5)
	{
		rmode = &TVEurgb60Hz480Prog;
		memflag = 5;
	}

	__SYS_UnlockSram(1); // 1 -> write changes
	while(!__SYS_SyncSram());

	/* Set video mode to PAL or NTSC */
	*(vu32*)0x800000CC = memflag;
	DCFlushRange((void *)(0x800000CC), 4);
	//ICInvalidateRange((void *)(0x800000CC), 4);

	if (rmode != 0)
		VIDEO_Configure(rmode);
	
	//m_frameBuf = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	//VIDEO_ClearFrameBuffer(rmode, m_frameBuf, COLOR_BLACK);
	//VIDEO_SetNextFramebuffer(m_frameBuf);

 /* Setup video  */
	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) 
		VIDEO_WaitVSync();
}

u8 get_wii_language()
{
	switch (CONF_GetLanguage())
	{
		case CONF_LANG_GERMAN:
			return SRAM_GERMAN;
		case CONF_LANG_FRENCH:
			return SRAM_FRENCH;
		case CONF_LANG_SPANISH:
			return SRAM_SPANISH;
		case CONF_LANG_ITALIAN:
			return SRAM_ITALIAN;
		case CONF_LANG_DUTCH:
			return SRAM_DUTCH;
		default:
			return SRAM_ENGLISH;
	}
}

void GC_SetLanguage(u8 lang)
{
	if (lang == 0)
		lang = get_wii_language();
	else
		lang--;

	syssram *sram;
	sram = __SYS_LockSram();
	sram->lang = lang;

	__SYS_UnlockSram(1); // 1 -> write changes
	while(!__SYS_SyncSram());
}

s32 DML_RemoveGame(struct discHdr header)
{
	fsop_deleteFolder(header.path);
	return 0;
}

int DML_GameIsInstalled(char *folder)
{
	int ret = 0;
	char source[300];
	snprintf(source, sizeof(source), "%s/game.iso", folder);
	
	FILE *f = fopen(source, "rb");
	if(f) 
	{
		dbg_printf("Found on: %s\n", folder);
		fclose(f);
		ret = 1;
	}
	else
	{
		snprintf(source, sizeof(source), "%s/sys/boot.bin", folder);
		f = fopen(source, "rb");
		if(f) 
		{
			dbg_printf("Found on: %s\n", folder);
			fclose(f);
			ret = 2;
		}
	}
	
	return ret;
}

void DML_New_SetOptions(char *GamePath,char *CheatPath, char *NewCheatPath, bool cheats, bool debugger, u8 NMM, u8 nodisc, u8 LED, u8 DMLvideoMode, u8 W_SCREEN, u8 PHOOK, u8 V_PATCH, u8 screenshot)
{
	dbg_printf("DML: Launch game '%s/game.iso' through memory (new method)\n", GamePath);

	DML_CFG *DMLCfg = (DML_CFG*)memalign(32, sizeof(DML_CFG));
	memset(DMLCfg, 0, sizeof(DML_CFG));

	DMLCfg->Magicbytes = 0xD1050CF6;
	if (CFG.dml >= CFG_DM_2_2)
		DMLCfg->Version = 0x00000002;
	else
		DMLCfg->Version = 0x00000001;	
	
	if (V_PATCH == 1)
		DMLCfg->VideoMode |= DML_VID_FORCE;
	else if (V_PATCH == 2)
		DMLCfg->VideoMode |= DML_VID_DML_AUTO;
	else
		DMLCfg->VideoMode |= DML_VID_NONE;

	//DMLCfg->Config |= DML_CFG_ACTIVITY_LED; //Sorry but I like it lol, option will may follow
	//DMLCfg->Config |= DML_CFG_PADHOOK; //Makes life easier, l+z+b+digital down...

	if(GamePath != NULL)
	{
		if(DML_GameIsInstalled(GamePath) == 2)
			snprintf(DMLCfg->GamePath, sizeof(DMLCfg->GamePath), "/games/%s/", get_DM_Game_Folder(GamePath));
		else
			snprintf(DMLCfg->GamePath, sizeof(DMLCfg->GamePath), "/games/%s/game.iso", get_DM_Game_Folder(GamePath));
		DMLCfg->Config |= DML_CFG_GAME_PATH;
	}

	if(CheatPath != NULL && NewCheatPath != NULL && cheats)
	{
		char *ptr;
		if(strstr(CheatPath, dm_boot_drive) == NULL)
		{
			fsop_CopyFile(CheatPath, NewCheatPath);
			ptr = &NewCheatPath[strlen(dm_boot_drive)];
		}
		else
			ptr = &CheatPath[strlen(dm_boot_drive)];
		strncpy(DMLCfg->CheatPath, ptr, sizeof(DMLCfg->CheatPath));
		DMLCfg->Config |= DML_CFG_CHEAT_PATH;
	}

	if(cheats)
	{
		DMLCfg->Config |= DML_CFG_CHEATS;
		printf_x(gt("Ocarina: Searching codes..."));
		printf("\n");
		sleep(1);
	}
	if(debugger)
		DMLCfg->Config |= DML_CFG_DEBUGGER;
	if(NMM > 0)
		DMLCfg->Config |= DML_CFG_NMM;
	if(NMM > 1)
		DMLCfg->Config |= DML_CFG_NMM_DEBUG;
	if(nodisc > 0 && CFG.dml >= CFG_DM_2_1)
		DMLCfg->Config |= DML_CFG_NODISC;
	else if (nodisc > 0)
		DMLCfg->Config |= DML_CFG_FORCE_WIDE;
	if(LED > 0)
		DMLCfg->Config |= DML_CFG_ACTIVITY_LED;
	if(W_SCREEN > 0)
		DMLCfg->Config |= DML_CFG_FORCE_WIDE; 
	if(PHOOK > 0)  
		DMLCfg->Config |= DML_CFG_PADHOOK;
	if(screenshot > 0)
		DMLCfg->Config |= DML_CFG_SCREENSHOT;
	
	if(DMLvideoMode == 1 && V_PATCH == 1)
		DMLCfg->VideoMode |= DML_VID_FORCE_PAL50;
	else if(DMLvideoMode == 2 && V_PATCH == 1)
		DMLCfg->VideoMode |= DML_VID_FORCE_NTSC;
	else if(DMLvideoMode == 3 && V_PATCH == 1)
		DMLCfg->VideoMode |= DML_VID_FORCE_PAL60;
	else if(DMLvideoMode >= 4 && V_PATCH == 1)
		DMLCfg->VideoMode |= DML_VID_FORCE_PROG;


	if(DMLvideoMode > 3)
		DMLCfg->VideoMode |= DML_VID_PROG_PATCH;


	//Write options into memory
	memcpy((void *)0x80001700, DMLCfg, sizeof(DML_CFG));
	DCFlushRange((void *)(0x80001700), sizeof(DML_CFG));

	//DML v1.2+
	memcpy((void *)0x81200000, DMLCfg, sizeof(DML_CFG));
	DCFlushRange((void *)(0x81200000), sizeof(DML_CFG));

	
	free(DMLCfg);
}

void DML_Old_SetOptions(char *GamePath, char *CheatPath, char *NewCheatPath, bool cheats)
{
	dbg_printf("DML: Launch game 'sd:/games/%s/game.iso' through boot.bin (old method)\n", GamePath);
	FILE *f;
	f = fopen("sd:/games/boot.bin", "wb");
	fwrite(get_DM_Game_Folder(GamePath), 1, strlen(get_DM_Game_Folder(GamePath)) + 1, f);
	fclose(f);

	if(cheats && strstr(CheatPath, NewCheatPath) == NULL)
		fsop_CopyFile(CheatPath, NewCheatPath);

	//Tell DML to boot the game from sd card
	*(vu32*)0x80001800 = 0xB002D105;
	DCFlushRange((void *)(0x80001800), 4);
	ICInvalidateRange((void *)(0x80001800), 4);

	*(vu32*)0xCC003024 |= 7;
}

void DML_New_SetBootDiscOption(char *CheatPath, char *NewCheatPath, bool cheats, u8 NMM, u8 LED, u8 DMLvideoMode)
{
	dbg_printf("Booting GC game\n");

	DML_CFG *DMLCfg = (DML_CFG*)malloc(sizeof(DML_CFG));
	memset(DMLCfg, 0, sizeof(DML_CFG));

	DMLCfg->Magicbytes = 0xD1050CF6;
	
	if (CFG.dml >= CFG_DM_2_2)
		DMLCfg->Version = 0x00000002;
	else
		DMLCfg->Version = 0x00000001;	
	
	DMLCfg->VideoMode |= DML_VID_DML_AUTO;
	DMLCfg->Config |= DML_CFG_PADHOOK;
	DMLCfg->Config |= DML_CFG_BOOT_DISC;
	
	if(CheatPath != NULL && NewCheatPath != NULL && cheats)
	{
		char *ptr;
		if(strstr(CheatPath, dm_boot_drive) == NULL)
		{
			fsop_CopyFile(CheatPath, NewCheatPath);
			ptr = &NewCheatPath[strlen(dm_boot_drive)];
		}
		else
			ptr = &CheatPath[strlen(dm_boot_drive)];
		strncpy(DMLCfg->CheatPath, ptr, sizeof(DMLCfg->CheatPath));
		DMLCfg->Config |= DML_CFG_CHEAT_PATH;
	}	

	if(cheats)
	{
		DMLCfg->Config |= DML_CFG_CHEATS;
		printf("cheat path=%s\n",DMLCfg->CheatPath);
		printf_x(gt("Ocarina: Searching codes..."));
		printf("\n");
		sleep(1);
	}
	
	if(NMM > 0)
		DMLCfg->Config |= DML_CFG_NMM;
	if(NMM > 1)
		DMLCfg->Config |= DML_CFG_NMM_DEBUG;
	if(LED > 0)
		DMLCfg->Config |= DML_CFG_ACTIVITY_LED;

	if(DMLvideoMode > 3)
		DMLCfg->VideoMode |= DML_VID_PROG_PATCH;

	//DML v1.2+
	memcpy((void *)0x81200000, DMLCfg, sizeof(DML_CFG));
	DCFlushRange((void *)(0x81200000), sizeof(DML_CFG));

	free(DMLCfg);
}

s32 DML_write_size_info_file(struct discHdr *header, u64 size) {
	char filepath[0xFF];
	FILE *infoFile = NULL;
	
	snprintf(filepath, sizeof(filepath), "%s/size.bin", header->path);
	
	infoFile = fopen(filepath, "wb");
	fwrite(&size, 1, sizeof(u64), infoFile);
	fclose(infoFile);
	return 0;
}

u64 DML_read_size_info_file(struct discHdr *header) {
	char filepath[0xFF];
	FILE *infoFile = NULL;
	u64 result = 0;
	
	snprintf(filepath, sizeof(filepath), "%s/size.bin", header->path);
	
	infoFile = fopen(filepath, "rb");
	if (infoFile) {
		fread(&result, 1, sizeof(u64), infoFile);
		fclose(infoFile);
	}
	return result;
}

u64 getDMLDisk1Size(struct discHdr *header) {
	u64 result = 0;
	char filepath[0xFF];
	snprintf(filepath, sizeof(filepath), "%s/game.iso", header->path);
	FILE *fp = fopen(filepath, "r");
	if (!fp)
	{
		snprintf(filepath, sizeof(filepath), "%s/sys/boot.bin", header->path);
		FILE *fp = fopen(filepath, "r");
		if (!fp)
			return result;
		fclose(fp);
		result = DML_read_size_info_file(header);
		if (result > 0)
			return result;
		snprintf(filepath, sizeof(filepath), "%s/root/", header->path);
		result = fsop_GetFolderBytes(filepath);
		if (result > 0)
			DML_write_size_info_file(header, result);
	}
	else
	{
		fseek(fp, 0, SEEK_END);
		result = ftell(fp);
		fclose(fp);
	}
	return result;
}

u64 getDMLGameSize(struct discHdr *header) {
	u64 result = 0;
	result = getDMLDisk1Size(header);	//get the size of the first disk

	char filepath[0xFF];
	snprintf(filepath, sizeof(filepath), "%s/disc2.iso", header->path);
	FILE *fp = fopen(filepath, "r");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		result += ftell(fp);			//add the size of the second disk
		fclose(fp);
	}
	return result;
}

s32 delete_Old_Copied_DML_Game() {
	FILE *infoFile = NULL;
	struct discHdr header;
	char infoFilePath[255];
	sprintf(infoFilePath, "%s/games/lastCopied.bin", dm_boot_drive);
	infoFile = fopen(infoFilePath, "rb");
	if (infoFile) {
		fread(&header, 1, sizeof(struct discHdr), infoFile);
		fclose(infoFile);
		DML_RemoveGame(header);
		return 0;
	}
	return -1;
}

s32 copy_DML_Game_to_SD(struct discHdr *header) {
	char target[255];
	
	if(!strncmp(header->path, dm_boot_drive, strlen(dm_boot_drive))) return -1;
	
	sprintf(target, "%s/games/%s", dm_boot_drive, get_DM_Game_Folder(header->path));
	fsop_CopyFolder(header->path, target);
	strcpy(header->path, target);
	
	FILE *infoFile = NULL;
	char infoFilePath[255];
	sprintf(infoFilePath, "%s/games/lastCopied.bin", dm_boot_drive);
	infoFile = fopen(infoFilePath, "wb");
	fwrite((u8*)header, 1, sizeof(struct discHdr), infoFile);
	fclose(infoFile);
	return 0;
}

// Devolution
static gconfig *DEVO_CONFIG = (gconfig*)0x80000020;

void DEVO_SetOptions(const char *path, u8 NMM)
{
	struct stat st;
	char full_path[256];
	int data_fd;

	stat(path, &st);
	u8 *lowmem = (u8*)0x80000000;
	FILE *iso_file = fopen(path, "rb");
	if(iso_file)
	{
		printf("path=%s, iso found !!\n",path);
	}
	fread(lowmem, 1, 32, iso_file);
	fclose(iso_file);

	// fill out the Devolution config struct
	memset(DEVO_CONFIG, 0, sizeof(*DEVO_CONFIG));
	DEVO_CONFIG->signature = 0x3EF9DB23;
	DEVO_CONFIG->version = 0x00000110;
	DEVO_CONFIG->device_signature = st.st_dev;
	DEVO_CONFIG->disc1_cluster = st.st_ino;
	
	if (CFG.game.wide_screen)
		DEVO_CONFIG->options |= DEVO_CFG_WIDE;
	if (!CFG.game.vidtv)
		DEVO_CONFIG->options |= DEVO_CFG_NOLED;
	if (CFG.game.alt_controller_cfg) {
		DEVO_CONFIG->version = 0x00000112;
		DEVO_CONFIG->options |= DEVO_CFG_D_BUTTONS;
	}
	
	// For 2nd ios file
	struct stat st2;
	char disc2path[256];
	strcpy(disc2path, path);
	char *posz = (char *)NULL;
	posz = strstr(disc2path, "game.iso");
	if(posz != NULL)				
		strncpy(posz, "disc2.iso", 9);
		
	iso_file = fopen(disc2path, "rb");
	if(iso_file)
	{
		printf("path=%s, iso found !!\n",disc2path);
		//sleep(5);
		stat(disc2path, &st2);
		fread(lowmem, 1, 32, iso_file);
		fclose(iso_file);	
		DEVO_CONFIG->disc2_cluster = st2.st_ino;	
	}

	// make sure these directories exist ON THE DRIVE THAT HOLDS THE ISO FILE, they are required for Devolution to function correctly
	char iso_disk[5];
	if (path[0] == 'u')
		sprintf(iso_disk, "usb:");
	else
		sprintf(iso_disk, "sd:");
	snprintf(full_path, sizeof(full_path), "%s/apps", iso_disk);
	fsop_MakeFolder(full_path);
	snprintf(full_path, sizeof(full_path), "%s/apps/gc_devo", iso_disk);
	fsop_MakeFolder(full_path);

	// find or create a 16MB memcard file for emulation
	// this file can be located anywhere since it's passed by cluster, not name
	// it must be at least 512KB (smallest possible memcard = 59 blocks)

	// IT MUST BE LOCATED ON THE SAME DRIVE AS THE ISO FILE!
	// IF YOU FUCK THIS UP (I'M LOOKING AT YOU, CFG-LOADER) YOU RISK DATA CORRUPTION
	if (USBLOADER_PATH[0] == iso_disk[0])
		snprintf(full_path, sizeof(full_path), "%s/memcard.bin", USBLOADER_PATH);
	else
		snprintf(full_path, sizeof(full_path), "%s/apps/gc_devo/memcard.bin", iso_disk);


	// check if file doesn't exist or is less than 16MB
	if(stat(full_path, &st) == -1 || st.st_size < 16<<20)
	{
		// need to enlarge or create it
		data_fd = open(full_path, O_WRONLY|O_CREAT);
		if (data_fd>=0)
		{
			// make it 16MB
			printf("Resizing memcard file...\n");
			ftruncate(data_fd, 16<<20);
			if (fstat(data_fd, &st)==-1 || st.st_size < 16<<20)
			{
				// it still isn't big enough. Give up.
				st.st_ino = 0;
			}
			close(data_fd);
		}
		else
		{
			// couldn't open or create the memory card file
			st.st_ino = 0;
		}
	}

	// set FAT cluster for start of memory card file
	// if this is zero memory card emulation will not be used
	if(!NMM > 0)
	{
		st.st_ino = 0;
		printf("emu. memcard is disabled\n");
	}
	else {
		printf("emu. memcard is enabled\n");
		DEVO_CONFIG->memcard_cluster = st.st_ino;
	}

	// flush disc ID and Devolution config out to memory
	DCFlushRange(lowmem, 64);
}

char *get_DM_Game_Folder(char *path) {
	char *folder;
	
	folder = strstr(path, "/");
	folder = folder+1;
	while (strstr(folder, "/")) {
		folder = strstr(folder, "/");
		folder = folder+1;
	}
	return folder;
}

bool is_gc_game_on_bootable_drive(struct discHdr *header) {
	bool ret = false;

	if ((CFG.game.channel_boot == 3) ||		//use nintendont
	    (CFG.game.channel_boot == 0 && CFG.default_gc_loader == 2))
	{
		ret = strncmp(header->path, "sd:", 3) == 0 || strncmp(header->path, "usb:", 4) == 0;
	}
	else
	if ((CFG.game.channel_boot == 2) ||		//use devolution
	    (CFG.game.channel_boot == 1 && CFG.dml == CFG_MIOS) ||
	    (CFG.game.channel_boot == 0 && CFG.default_gc_loader == 1))
	{
		ret = strncmp(header->path, "sd:", 3) == 0 || strncmp(header->path, "usb:", 4) == 0;
	}
	else	//use DIOS MIOS
	{
		ret = strncmp(header->path, dm_boot_drive, strlen(dm_boot_drive)) == 0;
	}
	
	return ret;
}

void Nintendont_set_options(struct discHdr *header, char *CheatPath, char *NewCheatPath)
{
	NIN_CFG ncfg;
	memset(&ncfg, 0, sizeof(NIN_CFG));

	ncfg.Magicbytes = 0x01070CF6;
	ncfg.Version = NIN_CFG_VERSION;

	if (CFG.game.ocarina)
		ncfg.Config |= NIN_CFG_CHEATS;
	if (CFG.game.country_patch)		//country_patch contains NMM setting
		ncfg.Config |= NIN_CFG_MEMCARDEMU;
	if (CFG.game.ocarina)
		ncfg.Config |= NIN_CFG_CHEAT_PATH;
	if (CFG.game.wide_screen)
		ncfg.Config |= NIN_CFG_FORCE_WIDE;
	if (CFG.game.video == 4 || CFG.game.video == 5)
		ncfg.Config |= NIN_CFG_FORCE_PROG;
	ncfg.Config |= NIN_CFG_AUTO_BOOT;
	if (CFG.game.alt_controller_cfg)
		ncfg.Config |= NIN_CFG_HID;
	if (strncmp("sd:", header->path, 3))
		ncfg.Config |= NIN_CFG_USB;
	if (CFG.game.vidtv)			//vidtv contains gc led setting
		ncfg.Config |= NIN_CFG_LED;

	switch (CFG.game.video)
	{
		default:
		case 0:
			ncfg.VideoMode |= NIN_VID_AUTO;
			break;
		case 1:
			ncfg.VideoMode |= NIN_VID_FORCE_PAL50 | NIN_VID_FORCE;
			break;
		case 2:
			ncfg.VideoMode |= NIN_VID_FORCE_NTSC | NIN_VID_FORCE;
			break;
		case 3:
			ncfg.VideoMode |= NIN_VID_FORCE_PAL60 | NIN_VID_FORCE;
			break;
		case 4:
			ncfg.VideoMode |= NIN_VID_FORCE_NTSC | NIN_VID_FORCE | NIN_VID_PROG;
			break;
		case 5:
			ncfg.VideoMode |= NIN_VID_FORCE_PAL60 | NIN_VID_FORCE | NIN_VID_PROG;
			break;
	}
	
	switch (CFG.game.language)
	{
		case CFG_LANG_CONSOLE:
		default:
			ncfg.Language = NIN_LAN_AUTO;
			break;
		case CFG_LANG_ENGLISH:
			ncfg.Language = NIN_LAN_ENGLISH;
			break;
		case CFG_LANG_GERMAN:
			ncfg.Language = NIN_LAN_GERMAN;
			break;
		case CFG_LANG_FRENCH:
			ncfg.Language = NIN_LAN_FRENCH;
			break;
		case CFG_LANG_SPANISH:
			ncfg.Language = NIN_LAN_SPANISH;
			break;
		case CFG_LANG_ITALIAN:
			ncfg.Language = NIN_LAN_ITALIAN;
			break;
		case CFG_LANG_DUTCH:
			ncfg.Language = NIN_LAN_DUTCH;
			break;
	}
	
	if(DML_GameIsInstalled(header->path) == 2)	//if FST format
		snprintf(ncfg.GamePath, sizeof(ncfg.GamePath), "%s/", strstr(header->path, "/"));
	else
		snprintf(ncfg.GamePath, sizeof(ncfg.GamePath), "%s/game.iso", strstr(header->path, "/"));

	if(CFG.game.ocarina && strstr(CheatPath, NewCheatPath) == NULL)
		fsop_CopyFile(CheatPath, NewCheatPath);
	snprintf(ncfg.CheatPath, sizeof(ncfg.CheatPath), "%s", strstr(NewCheatPath, "/"));

	ncfg.MaxPads = NIN_CFG_MAXPAD;
	memcpy(&ncfg.GameID, header->id, 4);

	FILE *f;
	f = fopen("/nincfg.bin", "wb");
	fwrite(&ncfg, 1, sizeof(NIN_CFG), f);
	fclose(f);
}