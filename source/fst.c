/*
 *  Copyright (C) 2008 Nuke (wiinuke@gmail.com)
 *
 *  this file is part of GeckoOS for USB Gecko
 *  http://www.usbgecko.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <malloc.h>
#include <sys/unistd.h>
#include <fat.h>
#include "ogc/ipc.h"
#include "fst.h"
#include "dvd_broadway.h"
#include "wpad.h"
#include "cfg.h"
#include "fat.h"
#include "menu.h"
#include "video.h"

#include "patchcode.h"
#include "gettext.h"

#include "codehandler.h"
//#include "codehandlerslota.h"
#include "codehandleronly.h"
#include "multidol.h"

#define FSTDIRTYPE 1
#define FSTFILETYPE 0
#define ENTRYSIZE 0xC
//#define FILEDIR	"fat0:/codes"
//#define FILEDIR	"sd:/codes"
#define FILEDIR	"/codes"

#define MAX_FILENAME_LEN	128

// Globals
u32 hooktype;
int patched;
u8 configbytes[2];
u32 regionfree;

static u8 *codelistend;
void *codelist;

u32 gameconfsize = 0;
u32 *gameconf = NULL;

u8 debuggerselect = 0;
u8 pausedstartoption = 0;

extern const u32 viwiihooks[4];
extern const u32 kpadhooks[4];
extern const u32 joypadhooks[4];
extern const u32 gxdrawhooks[4];
extern const u32 gxflushhooks[4];
extern const u32 ossleepthreadhooks[4];
extern const u32 axnextframehooks[4];
extern const u32 wpadbuttonsdownhooks[4];
extern const u32 wpadbuttonsdown2hooks[4];

//static vu32 dvddone = 0;

//---------------------------------------------------------------------------------
void app_loadgameconfig(char *discid)
//---------------------------------------------------------------------------------
{
	gameconfsize = 0;
	
	if (gameconf == NULL)
	{
		gameconf = malloc(65536);
		if (gameconf == NULL)
		{
			printf("gameconfig: Out of memory\n");
			sleep(4);
			return;
		}
	}

	FILE* fp;
	u32 ret;
	u32 filesize;
	s32 gameidmatch, maxgameidmatch = -1, maxgameidmatch2 = -1;
	u32 i, numnonascii, parsebufpos;
	u32 codeaddr, codeval, codeaddr2, codeval2, codeoffset;
	u32 temp, tempoffset = 0;
	char parsebuffer[18];
	
	//if (config_bytes[2] == 8)
	//	hookset = 1;
	
	u8 *tempgameconf;
	u32 tempgameconfsize = 0;
	
	//memcpy(tempgameconf, defaultgameconfig, defaultgameconfig_size);
	//tempgameconf[defaultgameconfig_size] = '\n';
	//tempgameconfsize = defaultgameconfig_size + 1;

	fp = fopen("sd:/codes/gameconfig.txt", "rb");
		
	if (!fp) fp = fopen("sd:/gameconfig.txt", "rb");
			
	if (fp) {
		fseek(fp, 0, SEEK_END);
		filesize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		
		tempgameconf = malloc(filesize);
		if (tempgameconf == NULL)
		{
			printf("gameconfig.txt: Out of memory\n");
			sleep(4);
			return;
		}
		
		ret = fread((void*)tempgameconf, 1, filesize, fp);
		fclose(fp);
		if (ret != filesize)
		{
			printf("Error reading gameconfig.txt\n");
			sleep(4);
			return;
		}
		tempgameconfsize = filesize;
	} else
	{
		return;
	}
	
	// Remove non-ASCII characters
	numnonascii = 0;
	for (i = 0; i < tempgameconfsize; i++)
	{
		if (tempgameconf[i] < 9 || tempgameconf[i] > 126)
			numnonascii++;
		else
			tempgameconf[i-numnonascii] = tempgameconf[i];
	}
	tempgameconfsize -= numnonascii;
	
	*(tempgameconf + tempgameconfsize) = 0;
	//gameconf = (tempgameconf + tempgameconfsize) + (4 - (((u32) (tempgameconf + tempgameconfsize)) % 4));
	
	for (maxgameidmatch = 0; maxgameidmatch <= 6; maxgameidmatch++)
	{
		i = 0;
		while (i < tempgameconfsize)
		{
			maxgameidmatch2 = -1;
			while (maxgameidmatch != maxgameidmatch2)
			{
				while (i != tempgameconfsize && tempgameconf[i] != ':') i++;
				if (i == tempgameconfsize) break;
				while ((tempgameconf[i] != 10 && tempgameconf[i] != 13) && (i != 0)) i--;
				if (i != 0) i++;
				parsebufpos = 0;
				gameidmatch = 0;
				while (tempgameconf[i] != ':')
				{
					if (tempgameconf[i] == '?')
					{
						parsebuffer[parsebufpos] = discid[parsebufpos];
						parsebufpos++;
						gameidmatch--;
						i++;
					}
					else if (tempgameconf[i] != 0 && tempgameconf[i] != ' ')
						parsebuffer[parsebufpos++] = tempgameconf[i++];
					else if (tempgameconf[i] == ' ')
						break;
					else
						i++;
					if (parsebufpos == 8) break;
				}
				parsebuffer[parsebufpos] = 0;
				if (strncasecmp("DEFAULT", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 7)
				{
					gameidmatch = 0;
					goto idmatch;
				}
				if (strncmp(discid, parsebuffer, strlen(parsebuffer)) == 0)
				{
					gameidmatch += strlen(parsebuffer);
				idmatch:
					if (gameidmatch > maxgameidmatch2)
					{
						maxgameidmatch2 = gameidmatch;
					}
				}
				while ((i != tempgameconfsize) && (tempgameconf[i] != 10 && tempgameconf[i] != 13)) i++;
			}
			while (i != tempgameconfsize && tempgameconf[i] != ':')
			{
				parsebufpos = 0;
				while ((i != tempgameconfsize) && (tempgameconf[i] != 10 && tempgameconf[i] != 13))
				{
					if (tempgameconf[i] != 0 && tempgameconf[i] != ' ' && tempgameconf[i] != '(' && tempgameconf[i] != ':')
						parsebuffer[parsebufpos++] = tempgameconf[i++];
					else if (tempgameconf[i] == ' ' || tempgameconf[i] == '(' || tempgameconf[i] == ':')
						break;
					else
						i++;
					if (parsebufpos == 17) break;
				}
				parsebuffer[parsebufpos] = 0;
				//if (!autobootcheck)
				{
					//if (strncasecmp("addtocodelist(", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 14)
					//{
					//	ret = sscanf(tempgameconf + i, "%x %x", &codeaddr, &codeval);
					//	if (ret == 2)
					//		addtocodelist(codeaddr, codeval);
					//}
					if (strncasecmp("codeliststart", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 13)
					{
						unsigned int xx;
						sscanf((char *)(tempgameconf + i), " = %x", &xx);
						codelist = (void*)xx;
					}
					if (strncasecmp("codelistend", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
					{
						unsigned int xx;
						sscanf((char *)(tempgameconf + i), " = %x", &xx);
						codelistend = (void*)xx;
					}
					/*
					if (strncasecmp("hooktype", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 8)
					{
						if (hookset == 1)
						{
							ret = sscanf(tempgameconf + i, " = %u", &temp);
							if (ret == 1)
								if (temp >= 0 && temp <= 7)
									config_bytes[2] = temp;
						}
					}
					*/
					if (strncasecmp("poke", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 4)
					{
						ret = sscanf((char *)tempgameconf + i, "( %x , %x", &codeaddr, &codeval);
						if (ret == 2)
						{
							*(gameconf + (gameconfsize / 4)) = 0;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = 0;
							gameconfsize += 8;
							*(gameconf + (gameconfsize / 4)) = codeaddr;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeval;
							gameconfsize += 4;
							DCFlushRange((void *) (gameconf + (gameconfsize / 4) - 5), 20);
						}
					}
					if (strncasecmp("pokeifequal", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
					{
						ret = sscanf((char *)(tempgameconf + i), "( %x , %x , %x , %x", &codeaddr, &codeval, &codeaddr2, &codeval2);
						if (ret == 4)
						{
							*(gameconf + (gameconfsize / 4)) = 0;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeaddr;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeval;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeaddr2;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeval2;
							gameconfsize += 4;
							DCFlushRange((void *) (gameconf + (gameconfsize / 4) - 5), 20);
						}
					}
					if (strncasecmp("searchandpoke", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 13)
					{
						ret = sscanf((char *)(tempgameconf + i), "( %x%n", &codeval, &tempoffset);
						if (ret == 1)
						{
							gameconfsize += 4;
							temp = 0;
							while (ret == 1)
							{
								*(gameconf + (gameconfsize / 4)) = codeval;
								gameconfsize += 4;
								temp++;
								i += tempoffset;
								ret = sscanf((char *)(tempgameconf + i), " %x%n", &codeval, &tempoffset);
							}
							*(gameconf + (gameconfsize / 4) - temp - 1) = temp;
							ret = sscanf((char *)(tempgameconf + i), " , %x , %x , %x , %x", &codeaddr, &codeaddr2, &codeoffset, &codeval2);
							if (ret == 4)
							{
								*(gameconf + (gameconfsize / 4)) = codeaddr;
								gameconfsize += 4;
								*(gameconf + (gameconfsize / 4)) = codeaddr2;
								gameconfsize += 4;
								*(gameconf + (gameconfsize / 4)) = codeoffset;
								gameconfsize += 4;
								*(gameconf + (gameconfsize / 4)) = codeval2;
								gameconfsize += 4;
								DCFlushRange((void *) (gameconf + (gameconfsize / 4) - temp - 5), temp * 4 + 20);
							}
							else
								gameconfsize -= temp * 4 + 4;
						}
						
					}
					/*
					if (strncasecmp("hook", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 4)
					{
						ret = sscanf(tempgameconf + i, "( %x %x %x %x %x %x %x %x", customhook, customhook + 1, customhook + 2, customhook + 3, customhook + 4, customhook + 5, customhook + 6, customhook + 7);
						if (ret >= 3)
						{
							if (hookset != 1)
								configwarn |= 4;
							config_bytes[2] = 0x08;
							customhooksize = ret * 4;
						}
					}
					if (strncasecmp("002fix", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 6)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 0x1)
								fakeiosversion = temp;
					}
					if (strncasecmp("switchios", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 9)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								willswitchios = temp;
					}
					if (strncasecmp("videomode", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 9)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
						{
							if (temp == 0)
							{
								if (config_bytes[1] != 0x00)
									configwarn |= 1;
								config_bytes[1] = 0x00;
							}
							else if (temp == 1)
							{
								if (config_bytes[1] != 0x03)
									configwarn |= 1;
								config_bytes[1] = 0x03;
							}
							else if (temp == 2)
							{
								if (config_bytes[1] != 0x01)
									configwarn |= 1;
								config_bytes[1] = 0x01;
							}
							else if (temp == 3)
							{
								if (config_bytes[1] != 0x02)
									configwarn |= 1;
								config_bytes[1] = 0x02;
							}
						}
					}
					if (strncasecmp("language", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 8)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
						{
							if (temp == 0)
							{
								if (config_bytes[0] != 0xCD)
									configwarn |= 2;
								config_bytes[0] = 0xCD;
							}
							else if (temp > 0 && temp <= 10)
							{
								if (config_bytes[0] != temp-1)
									configwarn |= 2;
								config_bytes[0] = temp-1;
							}
						}
					}
					if (strncasecmp("diagnostic", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 10)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
						{
							if (temp == 0 || temp == 1)
								diagcreate = temp;
						}
					}
					if (strncasecmp("vidtv", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 5)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								vipatchon = temp;
					}
					if (strncasecmp("fwritepatch", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								applyfwritepatch = temp;
					}
					if (strncasecmp("dumpmaindol", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								dumpmaindol = temp;
					}
					*/
				}
				/*else
				{
					
					if (strncasecmp("autoboot", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 8)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								autoboot = temp;
					}
					if (strncasecmp("autobootwait", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 12)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 255)
								autobootwait = temp;
					}
					if (strncasecmp("autoboothbc", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								autoboothbc = temp;
					}
					if (strncasecmp("autobootocarina", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 15)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								config_bytes[4] = temp;
					}
					if (strncasecmp("autobootdebugger", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 16)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								config_bytes[7] = temp;
					}
					if (strncasecmp("rebootermenuitem", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 16)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 1)
								rebooterasmenuitem = temp;
					}
					if (strncasecmp("startupios", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 10)
					{
						ret = sscanf(tempgameconf + i, " = %u", &temp);
						if (ret == 1)
							if (temp >= 0 && temp <= 255)
							{
								sdio_Shutdown();
								IOS_ReloadIOS(temp);
								detectIOScapabilities();
								sd_init();
								startupiosloaded = 1;
							}
					}
					
				}*/
				if (tempgameconf[i] != ':')
				{
					while ((i != tempgameconfsize) && (tempgameconf[i] != 10 && tempgameconf[i] != 13)) i++;
					if (i != tempgameconfsize) i++;
				}
			}
			if (i != tempgameconfsize) while ((tempgameconf[i] != 10 && tempgameconf[i] != 13) && (i != 0)) i--;
		}
	}
	
	free(tempgameconf);
	//tempcodelist = ((u8 *) gameconf) + gameconfsize;
}


#if 0
// Real basic 
u32 do_sd_code(char *filename)
{
	FILE *fp;
	u8 *filebuff;
	u32 filesize;
	u32 ret;
	u32 ret_val = 0;
	char filepath[128];
	char *fat_drive = FAT_DRIVE;
	int i;
	
	/*
	// fat already initialized in main()
	ret = fatInitDefault();
	if (!ret) {
		printf_x("SD Error\n");
		sleep (2);
		return 0;
	}
	*/

	fflush(stdout);
	
	printf_x(gt("Ocarina: Searching codes..."));
	printf("\n");

	for (i=0; i<3; i++) {
		switch (i) {
		case 0:
			snprintf(filepath, sizeof(filepath), "%s%s/%s.gct",
				USBLOADER_PATH, FILEDIR, filename);
			break;
		case 1:
			snprintf(filepath, sizeof(filepath), "%s/data/gecko%s/%s.gct",
				fat_drive, FILEDIR, filename);
			break;
		case 2:
			snprintf(filepath, sizeof(filepath), "%s%s/%s.gct",
				fat_drive, FILEDIR, filename);
			break;
		}
		printf("    %s\n", filepath);
		fp = fopen(filepath, "rb");
		if (fp) break;
		if (i == 2 && strcmp(fat_drive, "usb:") == 0) {
			// retry on sd:
			fat_drive = "sd:";
			i = 0;
		}
	}
	if (!fp) {
		printf_x(gt("Ocarina: No codes found"));
		printf("\n");
		sleep(3);
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	filebuff = (u8*) malloc (filesize);
	if(filebuff == 0){
		printf_x(gt("Ocarina: Out Of Memory Error"));
		printf("\n");
		fclose(fp);
		sleep(2);
		return 0;
	}

	ret = fread(filebuff, 1, filesize, fp);
	if(ret != filesize){	
		printf_x(gt("Ocarina: Code Error"));
		printf("\n");
		free(filebuff);
		fclose(fp);
		sleep(2);
		return 0;
	}
    printf_x(gt("Ocarina: Codes found."));
	printf("\n");

	// ocarina config options are done elswhere, confirmation optional
	if (!CFG.confirm_ocarina) goto no_confirm;

	printf("    ");
	printf(gt("Press %s button to apply codes."), (button_names[CFG.button_confirm.num]));
	printf("\n    ");
	printf(gt("Press %s button to skip codes."), (button_names[CFG.button_cancel.num]));
	printf("\n");
	/* Wait for user answer */
	for (;;) {
		u32 buttons = Wpad_WaitButtons();

		/* A button */
		if (buttons & CFG.button_confirm.mask) {
			no_confirm:
			// HOOKS STUFF - FISHEARS
			if (CFG.game.ocarina) {
				memset((void*)0x80001800,0,kenobiwii_size);
				memcpy((void*)0x80001800,kenobiwii,kenobiwii_size);
				DCFlushRange((void*)0x80001800,kenobiwii_size);
				hooktype = 1;
				memcpy((void*)0x80001800, (char*)0x80000000, 6);	// For WiiRD
			}
			// copy codes over
			memcpy((void*)0x800027E8,filebuff,filesize);
			// enable flag
			*(vu8*)0x80001807 = 0x01;
			// hooks are patched in dogamehooks()
			ret_val = 1;
			break;
		}

		/* B button */
		if (buttons & CFG.button_cancel.mask)
				break;
	}

	free(filebuff);
	fclose(fp);
	sleep(2);
	return ret_val;
}

#endif

u8 *code_buf = NULL;
int code_size = 0;

int ocarina_load_code(u8 *id)
{
	// option for the debugger
	if (CFG.wiird) debuggerselect = 1;
	if (CFG.wiird == 2) pausedstartoption = 1;
	if (debuggerselect == 0x00)
		codelist = (u8 *) 0x800022A8;
	else
		codelist = (u8 *) 0x800028B8;
	codelistend = (u8 *) 0x80003000;

	if (!CFG.game.ocarina) return 0;

	app_loadgameconfig((char *)id);
	
	char filename[8];
	char filepath[128];
	char *fat_drive = FAT_DRIVE;
	int i;
	memset(filename, 0, sizeof(filename));
	memcpy(filename, id, 6);
	
	printf_x(gt("Ocarina: Searching codes..."));
	printf("\n");

	for (i=0; i<3; i++) {
		switch (i) {
		case 0:
			snprintf(filepath, sizeof(filepath), "%s%s/%s.gct",
				USBLOADER_PATH, FILEDIR, filename);
			break;
		case 1:
			snprintf(filepath, sizeof(filepath), "%s/data/gecko%s/%s.gct",
				fat_drive, FILEDIR, filename);
			break;
		case 2:
			snprintf(filepath, sizeof(filepath), "%s%s/%s.gct",
				fat_drive, FILEDIR, filename);
			break;
		}
		printf_("%s\n", filepath);
		code_size = Fat_ReadFile(filepath, (void*)(&code_buf));
		if (code_size > 0 && code_buf) break;
		SAFE_FREE(code_buf);
		code_size = 0;
		if (i == 2 && strcmp(fat_drive, "usb:") == 0) {
			// retry on sd:
			fat_drive = "sd:";
			i = 0;
		}
	}

	if (!code_buf) {
		printf_x(gt("Ocarina: No codes found"));
		printf("\n");
		sleep(2);
		return 0;
	}

	if (code_size > (u32)codelistend - (u32)codelist)
	{
		printf_x(gt("Ocarina: Too many codes"));
		printf("\n");
		sleep(2);
		SAFE_FREE(code_buf);
		code_size = 0;
		return 0;
	}

    printf_x(gt("Ocarina: Codes found."));
	printf("\n");

	// optional confirmation
	if (CFG.confirm_ocarina) {
		Gui_Console_Enable();
		printf_h(gt("Press %s button to apply codes."), (button_names[CFG.button_confirm.num]));
		printf("\n");
		printf_h(gt("Press %s button to skip codes."), (button_names[CFG.button_cancel.num]));
		printf("\n");
		/* Wait for user answer */
		for (;;) {
			u32 buttons = Wpad_WaitButtons();
			if (buttons & CFG.button_confirm.mask) break;
			if (buttons & CFG.button_cancel.mask) {
				SAFE_FREE(code_buf);
				code_size = 0;
				break;
			}
		}
	}

	return code_size;
}

//---------------------------------------------------------------------------------
void app_pokevalues()
//---------------------------------------------------------------------------------
{
	u32 i, *codeaddr, *codeaddr2, *addrfound = NULL;
	
	if (gameconfsize != 0)
	{
		for (i = 0; i < gameconfsize/4; i++)
		{
			if (*(gameconf + i) == 0)
			{
				if (((u32 *) (*(gameconf + i + 1))) == NULL ||
					*((u32 *) (*(gameconf + i + 1))) == *(gameconf + i + 2))
				{
					*((u32 *) (*(gameconf + i + 3))) = *(gameconf + i + 4);
					DCFlushRange((void *) *(gameconf + i + 3), 4);
				}
				i += 4;
			}
			else
			{
				codeaddr = (u32 *)*(gameconf + i + *(gameconf + i) + 1);
				codeaddr2 = (u32 *)*(gameconf + i + *(gameconf + i) + 2);
				if (codeaddr == 0 && addrfound != NULL)
					codeaddr = addrfound;
				else if (codeaddr == 0 && codeaddr2 != 0)
					codeaddr = (u32 *) ((((u32) codeaddr2) >> 28) << 28);
				else if (codeaddr == 0 && codeaddr2 == 0)
				{
					i += *(gameconf + i) + 4;
					continue;
				}
				if (codeaddr2 == 0)
					codeaddr2 = codeaddr + *(gameconf + i);
				addrfound = NULL;
				while (codeaddr <= (codeaddr2 - *(gameconf + i)))
				{
					if (memcmp(codeaddr, gameconf + i + 1, (*(gameconf + i)) * 4) == 0)
					{
						*(codeaddr + ((*(gameconf + i + *(gameconf + i) + 3)) / 4)) = *(gameconf + i + *(gameconf + i) + 4);
						if (addrfound == NULL) addrfound = codeaddr;
					}
					codeaddr++;
				}
				i += *(gameconf + i) + 4;
			}
		}
	}
}


//---------------------------------------------------------------------------------
void load_handler()
//---------------------------------------------------------------------------------
{
	if (hooktype != 0x00)
	{
		if (debuggerselect == 0x01)
		{
			/*switch(gecko_channel)
			{
				case 0: // Slot A
					
					memset((void*)0x80001800,0,codehandlerslota_size);
					memcpy((void*)0x80001800,codehandlerslota,codehandlerslota_size);
					if (pausedstartoption == 0x01)
						*(u32*)0x80002774 = 1;
					memcpy((void*)0x80001CDE, &codelist, 2);
					memcpy((void*)0x80001CE2, ((u8*) &codelist) + 2, 2);
					memcpy((void*)0x80001F7E, &codelist, 2);
					memcpy((void*)0x80001F82, ((u8*) &codelist) + 2, 2);
					DCFlushRange((void*)0x80001800,codehandlerslota_size);
					break;
				
				case 1:	// slot B
				*/	memset((void*)0x80001800,0,codehandler_size);
					memcpy((void*)0x80001800,codehandler,codehandler_size);
					// option for paused start, debugging related
					if (pausedstartoption == 0x01)
						*(u32*)0x80002774 = 1;
					memcpy((void*)0x80001CDE, &codelist, 2);
					memcpy((void*)0x80001CE2, ((u8*) &codelist) + 2, 2);
					memcpy((void*)0x80001F5A, &codelist, 2);
					memcpy((void*)0x80001F5E, ((u8*) &codelist) + 2, 2);
					DCFlushRange((void*)0x80001800,codehandler_size);
				/*	break;
					
				case 2:
					memset((void*)0x80001800,0,codehandler_size);
					memcpy((void*)0x80001800,codehandler,codehandler_size);
					if (pausedstartoption == 0x01)
						*(u32*)0x80002774 = 1;
					memcpy((void*)0x80001CDE, &codelist, 2);
					memcpy((void*)0x80001CE2, ((u8*) &codelist) + 2, 2);
					memcpy((void*)0x80001F5A, &codelist, 2);
					memcpy((void*)0x80001F5E, ((u8*) &codelist) + 2, 2);
					DCFlushRange((void*)0x80001800,codehandler_size);
					break;
			}*/
		}
		else
		{
			memset((void*)0x80001800,0,codehandleronly_size);
			memcpy((void*)0x80001800,codehandleronly,codehandleronly_size);
			memcpy((void*)0x80001906, &codelist, 2);
			memcpy((void*)0x8000190A, ((u8*) &codelist) + 2, 2);
			DCFlushRange((void*)0x80001800,codehandleronly_size);
		}
		// Load multidol handler
		memset((void*)0x80001000,0,multidol_size);
		memcpy((void*)0x80001000,multidol,multidol_size); 
		DCFlushRange((void*)0x80001000,multidol_size);
		switch(hooktype)
		{
			case 0x01:
				memcpy((void*)0x8000119C,viwiihooks,12);
				memcpy((void*)0x80001198,viwiihooks+3,4);
				break;
			case 0x02:
				memcpy((void*)0x8000119C,kpadhooks,12);
				memcpy((void*)0x80001198,kpadhooks+3,4);
				break;
			case 0x03:
				memcpy((void*)0x8000119C,joypadhooks,12);
				memcpy((void*)0x80001198,joypadhooks+3,4);
				break;
			case 0x04:
				memcpy((void*)0x8000119C,gxdrawhooks,12);
				memcpy((void*)0x80001198,gxdrawhooks+3,4);
				break;
			case 0x05:
				memcpy((void*)0x8000119C,gxflushhooks,12);
				memcpy((void*)0x80001198,gxflushhooks+3,4);
				break;
			case 0x06:
				memcpy((void*)0x8000119C,ossleepthreadhooks,12);
				memcpy((void*)0x80001198,ossleepthreadhooks+3,4);
				break;
			case 0x07:
				memcpy((void*)0x8000119C,axnextframehooks,12);
				memcpy((void*)0x80001198,axnextframehooks+3,4);
				break;
			case 0x08:
				//if (customhooksize == 16)
				//{
				//	memcpy((void*)0x8000119C,customhook,12);
				//	memcpy((void*)0x80001198,customhook+3,4);
				//}
				break;
			case 0x09:
				//memcpy((void*)0x8000119C,wpadbuttonsdownhooks,12);
				//memcpy((void*)0x80001198,wpadbuttonsdownhooks+3,4);
				break;
			case 0x0A:
				//memcpy((void*)0x8000119C,wpadbuttonsdown2hooks,12);
				//memcpy((void*)0x80001198,wpadbuttonsdown2hooks+3,4);
				break;
		}
		DCFlushRange((void*)0x80001198,16);
	}
	memcpy((void *)0x80001800, (void*)0x80000000, 6);
}

int ocarina_do_code()
{
	if (!CFG.game.ocarina && !CFG.wiird) return 0;

	if (!hookpatched) {
		// when using Ocarina check if a hook as patched
		printf_("WARNING: ocarina/wiird hooks not set!");
		sleep(3);
	}

	if (CFG.wiird) {
		if (usb_isgeckoalive(EXI_CHANNEL_1)){
			printf_(gt("USB Gecko found. Debugging is enabled."));
			printf_("\n");
		}
	}

	memset((void *)0x80001800, 0, 0x1800);

	load_handler();
	memset(codelist, 0, (u32)codelistend - (u32)codelist);
	
	//Copy the codes
	if (code_size > 0 && code_buf)
	{
		memcpy(codelist, code_buf, code_size);
		DCFlushRange(codelist, (u32)codelistend - (u32)codelist);
		SAFE_FREE(code_buf);
	}

	// TODO What's this???
	// enable flag
	//*(vu8*)0x80001807 = 0x01;

	//This needs to be done after loading the .dol into memory
	app_pokevalues();

	// hooks are patched in dogamehooks()
	return 1;
}

