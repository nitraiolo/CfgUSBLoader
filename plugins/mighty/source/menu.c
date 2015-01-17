/* menu.c
 *
 * Triiforce mod by Marc
 *
 * Copyright (c) 2009 The Lemon Man, Nicksasa & WiiPower
 *
 * Distributed under the terms of the GNU General Public License (v2)
 * See http://www.gnu.org/licenses/gpl-2.0.txt for more info.
 *
 *********************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gccore.h>
#include <ogc/lwp_watchdog.h>
#include <wiiuse/wpad.h>
#include <asndlib.h>
#include <network.h> //for network
#include <sys/errno.h>
#include <sys/stat.h> //for mkdir

#include "tools.h"
#include "lz77.h"
#include "config.h"
#include "patch.h"
#include "codes/codes.h"
#include "codes/patchcode.h"
#include "nand.h"
#include "fat_mine.h"
#include "video.h"
#include "sonido.h"
#include "menu.h"
#include "fat.h"

// Constants
#define ARROWS_X		24
#define ARROWS_Y		210
#define ARROWS_WIDTH	20
#define MIGHTY_PATH			"fat:/config/mighty/"
#define IMAGES_PREFIX		"channels"
#define MIGHTY_CONFIG_FILE	"mighty_channels.cfg"
#define MAXGAMES		800
#define EMPTY			-1

#define BLACK	0x000000FF
#define YELLOW	0xFFFF00FF
#define WHITE	0xFFFFFFFF
#define ORANGE	0xeab000ff


// Variables for MIGHTY
static char tempString[128];
static int nandMode=0;

extern GXRModeObj *vmode;
extern u32* framebuffer;

const u8 COLS[]={3, 4};
#define ROWS 3
const u8 FIRSTCOL[]={136, 112};
#define FIRSTROW 96
const u8 SEPARACIONX[]={180, 136};
#define SEPARACIONY 112
const u8 ANCHOIMAGEN[]={154, 116};
#define ALTOIMAGEN 90



u8 Video_Mode;

u32 entryPoint;


void*	dolchunkoffset[64];		//TODO: variable size
u32		dolchunksize[64];		//TODO: variable size
u32		dolchunkcount;

void _unstub_start();


typedef void (*entrypoint) (void);

typedef struct _dolheader{
	u32 text_pos[7];
	u32 data_pos[11];
	u32 text_start[7];
	u32 data_start[11];
	u32 text_size[7];
	u32 data_size[11];
	u32 bss_start;
	u32 bss_size;
	u32 entry_point;
} dolheader;


bool __Check_HBC(void){
	u32 *stub = (u32 *)0x80001800;

	// Check HBC stub
	if (*stub)
		return true;
	return false;
}

s32 check_dol(u64 titleid, char *out, u16 bootcontent){
	s32 cfd;
	s32 ret;
	u32 num;
	dirent_t *list;
    char contentpath[ISFS_MAXPATH];
    char path[ISFS_MAXPATH];
    int cnt = 0;
	
	u8 LZ77_0x10 = 0x10;
    u8 LZ77_0x11 = 0x11;
	u8 *decompressed;
	u8 *compressed;
	u32 size_out = 0;
	u32 decomp_size = 0;
	
	

	u8 *buffer = memalign(32, 32);
	if (buffer == NULL){
		printf("Out of memory\n");
		return -1;
	}
	
    u8 check[6] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
 
    sprintf(contentpath, "/title/%08x/%08x/content", TITLE_UPPER(titleid), TITLE_LOWER(titleid));
    ret = getdir(contentpath, &list, &num);
    if (ret < 0){
		printf("Reading folder of the title failed\n");
		free(buffer);
		return ret;
	}
	for(cnt=0; cnt < num; cnt++){        
        if ((strstr(list[cnt].name, ".app") != NULL || strstr(list[cnt].name, ".APP") != NULL) && (strtol(list[cnt].name, NULL, 16) != bootcontent)){			
			memset(buffer, 0x00, 32);
            sprintf(path, "/title/%08x/%08x/content/%s", TITLE_UPPER(titleid), TITLE_LOWER(titleid), list[cnt].name);
  
            cfd = ISFS_Open(path, ISFS_OPEN_READ);
            if (cfd < 0)
			{
	    	    printf("ISFS_Open for %s failed %d\n", path, cfd);
				continue; 
			}

            ret = ISFS_Read(cfd, buffer, 32);
	        if (ret < 0)
	        {
	    	    printf("ISFS_Read for %s failed %d\n", path, ret);
		        ISFS_Close(cfd);
				continue;
	        }

            ISFS_Close(cfd);	

			if (buffer[0] == LZ77_0x10 || buffer[0] == LZ77_0x11)
			{
                if (buffer[0] == LZ77_0x10)
				{
					printf("Found LZ77 0x10 compressed content --> %s\n", list[cnt].name);
				} else
				{
					printf("Found LZ77 0x11 compressed content --> %s\n", list[cnt].name);
				}
				printf("This is most likely the main DOL, decompressing for checking\n");
				ret = read_file(path, &compressed, &size_out);
				if (ret < 0)
				{
					printf("Reading file failed\n");
					free(list);
					free(buffer);
					return ret;
				}
				printf("read file\n");
				ret = decompressLZ77content(compressed, 32, &decompressed, &decomp_size);
				if (ret < 0)
				{
					printf("Decompressing failed\n");
					free(list);
					free(buffer);
					return ret;
				}				
				memcpy(buffer, decompressed, 8);
 			}
			
	        ret = memcmp(buffer, check, 6);
            if(ret == 0)
            {
				printf("Found DOL --> %s\n", list[cnt].name);
				sprintf(out, "%s", path);
				free(buffer);
				free(list);
				return 0;
            } 
        }
    }
	
	free(buffer);
	free(list);
	
	printf("No .dol found\n");
	return -1;
}

void patch_dol(bool bootcontent){
	int i;
	bool hookpatched = false;
	
	for (i=0;i < dolchunkcount;i++)	{		
		if (!bootcontent){
			if (languageoption != -1){
				patch_language(dolchunkoffset[i], dolchunksize[i], languageoption);
			}
			
			if (videopatchoption != 0){
				search_video_modes(dolchunkoffset[i], dolchunksize[i]);
				patch_video_modes_to(vmode, videopatchoption);
			}
		}

		if (hooktypeoption != 0){
			// Before this can be done, the codehandler needs to be in memory, and the code to patch needs to be in the right pace
			if (dochannelhooks(dolchunkoffset[i], dolchunksize[i], bootcontent)){
				hookpatched = true;
			}			
		}
	}
	if (hooktypeoption != 0 && !hookpatched){
		printf("Error: Could not patch the hook\n");
		printf("Ocarina and debugger won't work\n");
		sleep(5);
	}
}  


u32 load_dol(u8 *buffer){
	dolchunkcount = 0;
	
	dolheader *dolfile;
	dolfile = (dolheader *)buffer;
	
	printf("Entrypoint: %08x\n", dolfile->entry_point);
	printf("BSS: %08x, size = %08x(%u)\n", dolfile->bss_start, dolfile->bss_size, dolfile->bss_size);

	memset((void *)dolfile->bss_start, 0, dolfile->bss_size);
	DCFlushRange((void *)dolfile->bss_start, dolfile->bss_size);
	
    printf("BSS cleared\n");
	
	u32 doloffset;
	u32 memoffset;
	u32 restsize;
	u32 size;

	int i;
	for (i = 0; i < 7; i++){	
		if(dolfile->text_pos[i] < sizeof(dolheader))
			continue;
	    
		dolchunkoffset[dolchunkcount] = (void *)dolfile->text_start[i];
		dolchunksize[dolchunkcount] = dolfile->text_size[i];
		dolchunkcount++;
		
		doloffset = (u32)buffer + dolfile->text_pos[i];
		memoffset = dolfile->text_start[i];
		restsize = dolfile->text_size[i];

		printf("Moving text section %u from %08x to %08x-%08x...", i, dolfile->text_pos[i], dolfile->text_start[i], dolfile->text_start[i]+dolfile->text_size[i]);
		fflush(stdout);
			
		while (restsize > 0){
			if (restsize > 2048){
				size = 2048;
			}else{
				size = restsize;
			}
			restsize -= size;
			ICInvalidateRange ((void *)memoffset, size);
			memcpy((void *)memoffset, (void *)doloffset, size);
			DCFlushRange((void *)memoffset, size);
			
			doloffset += size;
			memoffset += size;
		}

		printf("done\n");
		fflush(stdout);			
	}

	for(i = 0; i < 11; i++){
		if(dolfile->data_pos[i] < sizeof(dolheader))
			continue;
		
		dolchunkoffset[dolchunkcount] = (void *)dolfile->data_start[i];
		dolchunksize[dolchunkcount] = dolfile->data_size[i];
		dolchunkcount++;

		doloffset = (u32)buffer + dolfile->data_pos[i];
		memoffset = dolfile->data_start[i];
		restsize = dolfile->data_size[i];

		printf("Moving data section %u from %08x to %08x-%08x...", i, dolfile->data_pos[i], dolfile->data_start[i], dolfile->data_start[i]+dolfile->data_size[i]);
		fflush(stdout);
			
		while (restsize > 0){
			if (restsize > 2048){
				size = 2048;
			}else{
				size = restsize;
			}
			restsize -= size;
			ICInvalidateRange ((void *)memoffset, size);
			memcpy((void *)memoffset, (void *)doloffset, size);
			DCFlushRange((void *)memoffset, size);

			doloffset += size;
			memoffset += size;
		}

		printf("done\n");
		fflush(stdout);			
	} 
	return dolfile->entry_point;
}


s32 search_and_read_dol(u64 titleid, u8 **contentBuf, u32 *contentSize, bool skip_bootcontent){
	char filepath[ISFS_MAXPATH] ATTRIBUTE_ALIGN(0x20);
	int ret;
	u16 bootindex;
	u16 bootcontent;
	bool bootcontent_loaded;
	
	u8 *tmdBuffer = NULL;
	u32 tmdSize;
	tmd_content *p_cr;

	printf("Reading TMD...");

	sprintf(filepath, "/title/%08x/%08x/content/title.tmd", TITLE_UPPER(titleid), TITLE_LOWER(titleid));
	ret = read_file(filepath, &tmdBuffer, &tmdSize);
	if (ret < 0)
	{
		printf("Reading TMD failed\n");
		return ret;
	}
	printf("done\n");
	
	bootindex = ((tmd *)SIGNATURE_PAYLOAD((signed_blob *)tmdBuffer))->boot_index;
	p_cr = TMD_CONTENTS(((tmd *)SIGNATURE_PAYLOAD((signed_blob *)tmdBuffer)));
	bootcontent = p_cr[bootindex].cid;

	free(tmdBuffer);

	// Write bootcontent to filepath and overwrite it in case another .dol is found
	sprintf(filepath, "/title/%08x/%08x/content/%08x.app", TITLE_UPPER(titleid), TITLE_LOWER(titleid), bootcontent);

	if (skip_bootcontent)
	{
		bootcontent_loaded = false;
		printf("Searching for main DOL...\n");
			
		ret = check_dol(titleid, filepath, bootcontent);
		if (ret < 0)
		{
			printf("Searching for main.dol failed\n");
			printf("Press any button to load NAND loader instead...\n");
			bootcontent_loaded = true;
		}
	} else
	{
		bootcontent_loaded = true;
	}
	
    printf("Loading DOL: %s\n", filepath);
	
	ret = read_file(filepath, contentBuf, contentSize);
	if (ret < 0)
	{
		printf("Reading .dol failed\n");
		return ret;
	}
	
	if (isLZ77compressed(*contentBuf))
	{
		u8 *decompressed;
		ret = decompressLZ77content(*contentBuf, *contentSize, &decompressed, contentSize);
		if (ret < 0)
		{
			printf("Decompression failed\n");
			free(*contentBuf);
			return ret;
		}
		free(*contentBuf);
		*contentBuf = decompressed;
	}	
	
	if(bootcontent_loaded){
		return 1;
	}else{
		return 0;
	}
}


void determineVideoMode(u64 titleid){
	if (videooption == 0 || videooption == 1) {		
		// Get rmode and Video_Mode for system settings first
		u32 tvmode = CONF_GetVideo();

		// Attention: This returns &TVNtsc480Prog for all progressive video modes
		vmode = VIDEO_GetPreferredMode(0);
		
		switch (tvmode) 
		{
			case CONF_VIDEO_PAL:
				if (CONF_GetEuRGB60() > 0) 
				{
					Video_Mode = VI_EURGB60;
				}
				else 
				{
					Video_Mode = VI_PAL;
				}
				break;

			case CONF_VIDEO_MPAL:
				Video_Mode = VI_MPAL;
				break;

			case CONF_VIDEO_NTSC:
			default:
				Video_Mode = VI_NTSC;
				
		}

		// Overwrite rmode and Video_Mode when Default Video Mode is selected and Wii region doesn't match the channel region
		u32 low;
		low = TITLE_LOWER(titleid);
		char Region = low % 256;
		if (*(char *)&low != 'W') // Don't overwrite video mode for WiiWare
		{
			switch (Region) 
			{
				case 'P':
				case 'D':
				case 'F':
				case 'X':
				case 'Y':
					if (CONF_GetVideo() != CONF_VIDEO_PAL)
					{
						Video_Mode = VI_EURGB60;

						if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable())
						{
							vmode = &TVNtsc480Prog; // This seems to be correct!
						}
						else
						{
							vmode = &TVEurgb60Hz480IntDf;
						}				
					}
					break;

				case 'E':
				case 'J':
				case 'T':
					if (CONF_GetVideo() != CONF_VIDEO_NTSC)
					{
						Video_Mode = VI_NTSC;
						if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable())
						{
							vmode = &TVNtsc480Prog;
						}
						else
						{
							vmode = &TVNtsc480IntDf;
						}				
					}
			}
		}
	} else {

		if (videooption == 2){
			vmode = &TVPal528IntDf;
		} else if (videooption == 3){
			vmode = &TVEurgb60Hz480IntDf;
		} else if (videooption == 4){
			vmode = &TVNtsc480IntDf;
		} else if (videooption == 5){
			vmode = &TVNtsc480Prog;
		}
		Video_Mode = (vmode->viTVMode) >> 2;
	}
}

void setVideoMode(){	
	*(u32 *)0x800000CC = Video_Mode;
	DCFlushRange((void*)0x800000CC, sizeof(u32));
	
	// Overwrite all progressive video modes as they are broken in libogc
	if (videomode_interlaced(vmode) == 0){
		vmode = &TVNtsc480Prog;
	}

	VIDEO_Configure(vmode);
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	
	if (vmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
}


#define MAXAPPLOADERGAMES 7
const char games[MAXAPPLOADERGAMES][4]={
	"WAL",	//ArtSyle: light trax
	"WDH",	//ArtSyle: Rotohex
	"WOB",	//ArtSyle: ORBIENT
	"WPR",	//ArtSyle: CUBELLO
	"WA8",	//ArtSyle: Penta Tentacles
	"WB7",	//Midnight Pool
	"WSP"	//Pokemon Rumble
};

bool checkApploaderGame(const char* id){
	int i;

	for(i=0; i<MAXAPPLOADERGAMES; i++){
		if(memcmp(id, &games[i], 3)==0){
		//if(games[i][0]==id[0] && games[i][1]==id[1] && games[i][2]==id[2]){
			return true;
		}
	}
	return false;
}

int __CreateNullFile(const char* path, const char* fileName, int fileSize){
	int i;
	unsigned char* nullData;
	sprintf(tempString, "%s/%s", path, fileName);
	if(!Fat_CheckFile(tempString)){
		nullData=allocate_memory(fileSize);
		for(i=0;i<fileSize;i++)
			nullData[i]=0;
		Fat_SaveFile(tempString, (void *)&nullData, fileSize);
		free(nullData);
		return 1;
	}

	return 0;
}

int checkNeededSave(title* game){
	int ret=0;
	char dataPath[128];
	sprintf(dataPath, "fat:/%s/title/00010001/%08x/data", Get_Path(), TITLE_LOWER(game->idInt));
		

	if(memcmp(game->id, "WTR", 3)==0 || memcmp(game->id, "W8C", 3)==0 || memcmp(game->id, "WVB", 3)==0){ //Bit Trip BEAT/CORE/VOID
		Fat_MakeDir(dataPath);
		ret+=__CreateNullFile(dataPath, "banner.bin", 61600);
		ret+=__CreateNullFile(dataPath, "savefile.dat", 512);
	}else if(memcmp(game->id, "WRU", 3)==0){ //Bit Trip RUNNER
		Fat_MakeDir(dataPath);
		ret+=__CreateNullFile(dataPath, "banner.bin", 61600);
		ret+=__CreateNullFile(dataPath, "savefile.dat", 1024);
	}else if(memcmp(game->id, "WBF", 3)==0){ //Bit Trip FATE
		Fat_MakeDir(dataPath);
		ret+=__CreateNullFile(dataPath, "banner.bin", 61600);
		ret+=__CreateNullFile(dataPath, "savefile.dat", 192);
	}else if(memcmp(game->id, "WTF", 3)==0){ //Bit Trip FLUX
		Fat_MakeDir(dataPath);
		ret+=__CreateNullFile(dataPath, "banner.bin", 61600);
		ret+=__CreateNullFile(dataPath, "savefile.dat", 160);
	}

	return ret;
}



void __Start_Game(title game, int ios, int mode) {

	nandMode = mode;
	
	if(nandMode==EMU_USB)
		Fat_Mount(USB);
	else
		Fat_Mount(SD);
	
	int ret = 0;

	entrypoint appJump;
	u32 requested_ios;
	u8 *dolbuffer;
	u32 dolsize;
	bool bootcontentloaded;

	printf("Setting game info...\n");
	printf("GameID: %08x\n", TITLE_LOWER(game.idInt));

	u64 titleid=game.idInt;
	videooption=game.videoMode;
	videopatchoption=game.videoPatch;
	languageoption=game.language-1;
	hooktypeoption=game.hooktype;
	ocarinaoption=game.ocarina;
	debuggeroption=game.debugger;			
	bootmethodoption=game.bootMethod;

	printf("Check savegame...\n");
	
	if(nandMode!=REAL_NAND){
		ret=checkNeededSave(&game);
	}
	
	//Unmount FAT
	Fat_Unmount();

	//Initialize NAND
	if(nandMode!=REAL_NAND){
		ret=Enable_Emu(nandMode);
		if(ret<0){
			printf("ERROR: I can't enable NAND emulator (ret=%d).\n", ret);
			goto out;
		}
	}

	ISFS_Initialize();

	ret = search_and_read_dol(titleid, &dolbuffer, &dolsize, (bootmethodoption == 0));
	if (ret < 0){
		printf(".dol loading failed\n");
		return;
	}
	bootcontentloaded = (ret == 1);

	determineVideoMode(titleid);
	
	entryPoint = load_dol(dolbuffer);
	
	free(dolbuffer);

	printf(".dol loaded\n");

	ret = identify(titleid, &requested_ios);
	if (ret < 0)
	{
		printf("Identify failed\n");
		return;
	}
	
	ISFS_Deinitialize();
	
	// Set the clock
	settime(secs_to_ticks(time(NULL) - 946684800));

	if (entryPoint != 0x3400){
		printf("Setting bus speed\n");
		*(u32*)0x800000F8 = 0x0E7BE2C0;
		printf("Setting cpu speed\n");
		*(u32*)0x800000FC = 0x2B73A840;

		DCFlushRange((void*)0x800000F8, 0xFF);
	}
	
	// Remove 002 error
	printf("Fake IOS Version(%u)\n", requested_ios);
	*(u16 *)0x80003140 = requested_ios;
	*(u16 *)0x80003142 = 0xffff;
	*(u16 *)0x80003188 = requested_ios;
	*(u16 *)0x8000318A = 0xffff;
	
	DCFlushRange((void*)0x80003140, 4);
	DCFlushRange((void*)0x80003188, 4);
	
	/*ret = ES_SetUID(titleid);
	if (ret < 0){
		printf("ES_SetUID failed %d", ret);
		return;
	}*/
	printf("ES_SetUID successful\n");
	
	
	if(hooktypeoption != 0){
		do_codes(titleid);
	}
	
	patch_dol(bootcontentloaded);

	printf("Loading complete, booting...\n");

	appJump = (entrypoint)entryPoint;

	//sleep(5);

	setVideoMode();

	//---GREENSCREEN FIX---
	VIDEO_Configure(vmode);
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	//---------------------

	SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);

	if (entryPoint != 0x3400)
	{
		if (hooktypeoption != 0)
		{
			__asm__(
						"lis %r3, entryPoint@h\n"
						"ori %r3, %r3, entryPoint@l\n"
						"lwz %r3, 0(%r3)\n"
						"mtlr %r3\n"
						"lis %r3, 0x8000\n"
						"ori %r3, %r3, 0x18A8\n"
						"mtctr %r3\n"
						"bctr\n"
						);
						
		} else
		{
			appJump();	
		}
	} else
	{
		if (hooktypeoption != 0)
		{
			__asm__(
						"lis %r3, returnpoint@h\n"
						"ori %r3, %r3, returnpoint@l\n"
						"mtlr %r3\n"
						"lis %r3, 0x8000\n"
						"ori %r3, %r3, 0x18A8\n"
						"mtctr %r3\n"
						"bctr\n"
						"returnpoint:\n"
						"bl DCDisable\n"
						"bl ICDisable\n"
						"li %r3, 0\n"
						"mtsrr1 %r3\n"
						"lis %r4, entryPoint@h\n"
						"ori %r4,%r4,entryPoint@l\n"
						"lwz %r4, 0(%r4)\n"
						"mtsrr0 %r4\n"
						"rfi\n"
						);
		}else{
			_unstub_start();
		}
	}
	
	out:
	printf("ret=%d\n", ret);
	return;
}

#define CONFIGITEMS 7
int __Config_Game(void){

	//Obtener informacion del juego
	//title *config = NULL;
	//char* gameId=config->id;

	/*u32 optioncount[CONFIGITEMS]={8,4,11,8,3,2,2};
	u32 optionselected[CONFIGITEMS] = {config->videoMode, config->videoPatch, config->language, config->hooktype, config->ocarina, config->debugger, config->bootMethod};
	char *videooptions[8]={"Default Video Mode", "Force NTSC480i", "Force NTSC480p", "Force PAL480i", "Force PAL480p", "Force PAL576i", "Force MPAL480i", "Force MPAL480p"};
	char *videopatchoptions[4]={"No Video patches", "Smart Video patching", "More Video patching", "Full Video patching" };
	char *languageoptions[11]={"Default Language", "Japanese", "English", "German", "French", "Spanish", "Italian", "Dutch", "S. Chinese", "T. Chinese", "Korean"};
	char *hooktypeoptions[8]={"No Ocarina&debugger", "Hooktype: VBI", "Hooktype: KPAD", "Hooktype: Joypad", "Hooktype: GXDraw", "Hooktype: GXFlush", "Hooktype: OSSleepThread", "Hooktype: AXNextFrame"};
	char *ocarinaoptions[3]={"No Ocarina", "Ocarina from SD", "Ocarina from USB"};
	char *debuggeroptions[2]={"No debugger", "Debugger enabled"};
	char *bootmethodoptions[2]={"Normal boot method", "Load apploader"};*/

	return MENU_SELECT_GAME;
}

int __Update_File_From_Real_Nand(char* filepath){
	u8 *inBuffer = NULL;
	u32 inSize;
	int ret=0;

	//ISFS init
	ret=ISFS_Initialize();
	if(ret<0){
		return ret;
	}

	ret=read_file(filepath, &inBuffer, &inSize);
	ISFS_Deinitialize();
	if(ret<0){
		return ret;
	}

	sprintf(tempString, "fat:/%s/%s", Get_Path(), filepath);
	ret=Fat_SaveFile(tempString, (void *)&inBuffer, inSize);

	return ret;
}