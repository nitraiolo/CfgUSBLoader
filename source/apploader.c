#include <stdio.h>
#include <ogcsys.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <ctype.h>

#include "apploader.h"
#include "wdvd.h"
#include "video.h"
#include "wpad.h"
#include "cfg.h"
#include "patchcode.h" /*FISHEARS*/
#include "dol.h"
#include "wiip.h"
#include "gettext.h"
#include "menu.h"
#include "dolmenu.h"

/* Apploader function pointers */
typedef int   (*app_main)(void **dst, int *size, int *offset);
typedef void  (*app_init)(void (*report)(const char *fmt, ...));
typedef void *(*app_final)();
typedef void  (*app_entry)(void (**init)(void (*report)(const char *fmt, ...)), int (**main)(), void *(**final)());

/* Apploader pointers */
static u8 *appldr = (u8 *)0x81200000;


/* Constants */
#define APPLDR_OFFSET	0x2440

/* Variables */
static u32 buffer[0x20] ATTRIBUTE_ALIGN(32);

/* Forward Declarations */
void PatchCountryStrings(void *Address, int Size);
void maindolpatches(void *dst, int len);
bool cfg_PatchReturnTo(void *Address, int Size);
u32 Load_Dol_from_sd();
u32 Load_Dol_from_disc_menu();
u32 Load_Dol_from_disc(u32 doloffset);
void getdolnames();
u32 getdoloffsetbyname(char *name);
bool Remove_001_Protection(void *Address, int Size);
void Anti_002_fix(void *Address, int Size);
//u32 load_dol_image (void *dolstart, bool clear_bss);

u32 dolcount = 0;
u32 dolindex[10];

static void __noprint(const char *fmt, ...)
{
}

bool compare_videomodes(GXRModeObj* mode1, GXRModeObj* mode2)
{
	if (mode1->viTVMode != mode2->viTVMode || mode1->fbWidth != mode2->fbWidth ||	mode1->efbHeight != mode2->efbHeight || mode1->xfbHeight != mode2->xfbHeight ||
	mode1->viXOrigin != mode2->viXOrigin || mode1->viYOrigin != mode2->viYOrigin || mode1->viWidth != mode2->viWidth || mode1->viHeight != mode2->viHeight ||
	mode1->xfbMode != mode2->xfbMode || mode1->field_rendering != mode2->field_rendering || mode1->aa != mode2->aa || mode1->sample_pattern[0][0] != mode2->sample_pattern[0][0] ||
	mode1->sample_pattern[1][0] != mode2->sample_pattern[1][0] ||	mode1->sample_pattern[2][0] != mode2->sample_pattern[2][0] ||
	mode1->sample_pattern[3][0] != mode2->sample_pattern[3][0] ||	mode1->sample_pattern[4][0] != mode2->sample_pattern[4][0] ||
	mode1->sample_pattern[5][0] != mode2->sample_pattern[5][0] ||	mode1->sample_pattern[6][0] != mode2->sample_pattern[6][0] ||
	mode1->sample_pattern[7][0] != mode2->sample_pattern[7][0] ||	mode1->sample_pattern[8][0] != mode2->sample_pattern[8][0] ||
	mode1->sample_pattern[9][0] != mode2->sample_pattern[9][0] ||	mode1->sample_pattern[10][0] != mode2->sample_pattern[10][0] ||
	mode1->sample_pattern[11][0] != mode2->sample_pattern[11][0] || mode1->sample_pattern[0][1] != mode2->sample_pattern[0][1] ||
	mode1->sample_pattern[1][1] != mode2->sample_pattern[1][1] ||	mode1->sample_pattern[2][1] != mode2->sample_pattern[2][1] ||
	mode1->sample_pattern[3][1] != mode2->sample_pattern[3][1] ||	mode1->sample_pattern[4][1] != mode2->sample_pattern[4][1] ||
	mode1->sample_pattern[5][1] != mode2->sample_pattern[5][1] ||	mode1->sample_pattern[6][1] != mode2->sample_pattern[6][1] ||
	mode1->sample_pattern[7][1] != mode2->sample_pattern[7][1] ||	mode1->sample_pattern[8][1] != mode2->sample_pattern[8][1] ||
	mode1->sample_pattern[9][1] != mode2->sample_pattern[9][1] ||	mode1->sample_pattern[10][1] != mode2->sample_pattern[10][1] ||
	mode1->sample_pattern[11][1] != mode2->sample_pattern[11][1] || mode1->vfilter[0] != mode2->vfilter[0] ||
	mode1->vfilter[1] != mode2->vfilter[1] ||	mode1->vfilter[2] != mode2->vfilter[2] || mode1->vfilter[3] != mode2->vfilter[3] ||	mode1->vfilter[4] != mode2->vfilter[4] ||
	mode1->vfilter[5] != mode2->vfilter[5] ||	mode1->vfilter[6] != mode2->vfilter[6] )
	{
		return false;
	} else
	{
		return true;
	}
}


void patch_videomode(GXRModeObj* mode1, GXRModeObj* mode2)
{
	mode1->viTVMode = mode2->viTVMode;
	mode1->fbWidth = mode2->fbWidth;
	mode1->efbHeight = mode2->efbHeight;
	mode1->xfbHeight = mode2->xfbHeight;
	mode1->viXOrigin = mode2->viXOrigin;
	mode1->viYOrigin = mode2->viYOrigin;
	mode1->viWidth = mode2->viWidth; 
	mode1->viHeight = mode2->viHeight;
	mode1->xfbMode = mode2->xfbMode; 
	mode1->field_rendering = mode2->field_rendering;
	mode1->aa = mode2->aa;
	mode1->sample_pattern[0][0] = mode2->sample_pattern[0][0];
	mode1->sample_pattern[1][0] = mode2->sample_pattern[1][0];
	mode1->sample_pattern[2][0] = mode2->sample_pattern[2][0];
	mode1->sample_pattern[3][0] = mode2->sample_pattern[3][0];
	mode1->sample_pattern[4][0] = mode2->sample_pattern[4][0];
	mode1->sample_pattern[5][0] = mode2->sample_pattern[5][0];
	mode1->sample_pattern[6][0] = mode2->sample_pattern[6][0];
	mode1->sample_pattern[7][0] = mode2->sample_pattern[7][0];
	mode1->sample_pattern[8][0] = mode2->sample_pattern[8][0];
	mode1->sample_pattern[9][0] = mode2->sample_pattern[9][0];
	mode1->sample_pattern[10][0] = mode2->sample_pattern[10][0];
	mode1->sample_pattern[11][0] = mode2->sample_pattern[11][0];
	mode1->sample_pattern[0][1] = mode2->sample_pattern[0][1];
	mode1->sample_pattern[1][1] = mode2->sample_pattern[1][1];	
	mode1->sample_pattern[2][1] = mode2->sample_pattern[2][1];
	mode1->sample_pattern[3][1] = mode2->sample_pattern[3][1];	
	mode1->sample_pattern[4][1] = mode2->sample_pattern[4][1];
	mode1->sample_pattern[5][1] = mode2->sample_pattern[5][1];	
	mode1->sample_pattern[6][1] = mode2->sample_pattern[6][1];
	mode1->sample_pattern[7][1] = mode2->sample_pattern[7][1];	
	mode1->sample_pattern[8][1] = mode2->sample_pattern[8][1];
	mode1->sample_pattern[9][1] = mode2->sample_pattern[9][1];	
	mode1->sample_pattern[10][1] = mode2->sample_pattern[10][1];
	mode1->sample_pattern[11][1] = mode2->sample_pattern[11][1]; 
	mode1->vfilter[0] = mode2->vfilter[0];
	mode1->vfilter[1] = mode2->vfilter[1];	
	mode1->vfilter[2] = mode2->vfilter[2];
	mode1->vfilter[3] = mode2->vfilter[3];	
	mode1->vfilter[4] = mode2->vfilter[4];
	mode1->vfilter[5] = mode2->vfilter[5];	
	mode1->vfilter[6] = mode2->vfilter[6];
}

GXRModeObj* vmodes[] = {
	&TVNtsc240Ds,
	&TVNtsc240DsAa,
	&TVNtsc240Int,
	&TVNtsc240IntAa,
	&TVNtsc480IntDf,
	&TVNtsc480IntAa,
	&TVNtsc480Prog,
	&TVMpal480IntDf,
	&TVPal264Ds,
	&TVPal264DsAa,
	&TVPal264Int,
	&TVPal264IntAa,
	&TVPal524IntAa,
	&TVPal528Int,
	&TVPal528IntDf,
	&TVPal574IntDfScale,
	&TVEurgb60Hz240Ds,
	&TVEurgb60Hz240DsAa,
	&TVEurgb60Hz240Int,
	&TVEurgb60Hz240IntAa,
	&TVEurgb60Hz480Int,
	&TVEurgb60Hz480IntDf,
	&TVEurgb60Hz480IntAa,
	&TVEurgb60Hz480Prog,
	&TVEurgb60Hz480ProgSoft,
	&TVEurgb60Hz480ProgAa
};

GXRModeObj* PAL2NTSC[]={
	&TVMpal480IntDf,		&TVNtsc480IntDf,
	&TVPal264Ds,			&TVNtsc240Ds,
	&TVPal264DsAa,			&TVNtsc240DsAa,
	&TVPal264Int,			&TVNtsc240Int,
	&TVPal264IntAa,			&TVNtsc240IntAa,
	&TVPal524IntAa,			&TVNtsc480IntAa,
	&TVPal528Int,			&TVNtsc480IntAa,
	&TVPal528IntDf,			&TVNtsc480IntDf,
	&TVPal574IntDfScale,	&TVNtsc480IntDf,
	&TVEurgb60Hz240Ds,		&TVNtsc240Ds,
	&TVEurgb60Hz240DsAa,	&TVNtsc240DsAa,
	&TVEurgb60Hz240Int,		&TVNtsc240Int,
	&TVEurgb60Hz240IntAa,	&TVNtsc240IntAa,
	&TVEurgb60Hz480Int,		&TVNtsc480IntAa,
	&TVEurgb60Hz480IntDf,	&TVNtsc480IntDf,
	&TVEurgb60Hz480IntAa,	&TVNtsc480IntAa,
	&TVEurgb60Hz480Prog,	&TVNtsc480Prog,
	&TVEurgb60Hz480ProgSoft,&TVNtsc480Prog,
	&TVEurgb60Hz480ProgAa,  &TVNtsc480Prog,
	0,0
};

GXRModeObj* NTSC2PAL[]={
	&TVNtsc240Ds,			&TVPal264Ds,		
	&TVNtsc240DsAa,			&TVPal264DsAa,
	&TVNtsc240Int,			&TVPal264Int,
	&TVNtsc240IntAa,		&TVPal264IntAa,
	&TVNtsc480IntDf,		&TVPal528IntDf,
	&TVNtsc480IntAa,		&TVPal524IntAa,
	&TVNtsc480Prog,			&TVPal528IntDf,
	0,0
};

GXRModeObj* NTSC2PAL60[]={
	&TVNtsc240Ds,			&TVEurgb60Hz240Ds,		
	&TVNtsc240DsAa,			&TVEurgb60Hz240DsAa,
	&TVNtsc240Int,			&TVEurgb60Hz240Int,
	&TVNtsc240IntAa,		&TVEurgb60Hz240IntAa,
	&TVNtsc480IntDf,		&TVEurgb60Hz480IntDf,
	&TVNtsc480IntAa,		&TVEurgb60Hz480IntAa,
	&TVNtsc480Prog,			&TVEurgb60Hz480Prog,
	0,0
};

bool Search_and_patch_Video_Modes(void *Address, u32 Size, GXRModeObj* Table[])
{
	u8 *Addr = (u8 *)Address;
	bool found = 0;
	u32 i;

	while(Size >= sizeof(GXRModeObj))
	{
		for(i = 0; Table[i]; i+=2)
		{
			if(compare_videomodes(Table[i], (GXRModeObj*)Addr))
			{
				found = 1;
				patch_videomode((GXRModeObj*)Addr, Table[i+1]);
				Addr += (sizeof(GXRModeObj)-4);
				Size -= (sizeof(GXRModeObj)-4);
				break;
			}
		}

		Addr += 4;
		Size -= 4;
	}

	return found;
}

bool Search_and_patch_Video_To(void *Address, u32 Size,
		GXRModeObj* Table[], GXRModeObj* vmode)
{
	u8 *Addr = (u8 *)Address;
	bool found = 0;
	u32 i;

	while(Size >= sizeof(GXRModeObj))
	{
		for(i = 0; Table[i]; i++)
		{
			if(compare_videomodes(Table[i], (GXRModeObj*)Addr))
			{
				found = 1;
				patch_videomode((GXRModeObj*)Addr, vmode);
				Addr += (sizeof(GXRModeObj)-4);
				Size -= (sizeof(GXRModeObj)-4);
				break;
			}
		}

		Addr += 4;
		Size -= 4;
	}

	return found;
}

s32 Apploader_Run(entry_point *entry)
{
	app_entry appldr_entry;
	app_init  appldr_init;
	app_main  appldr_main;
	app_final appldr_final;

	u32 appldr_len;
	s32 ret;

	void* dst_array[64];
	int len_array[64];
	int last_index = -1;
	int fststart = 0;
	
	wipreset();

	get_time(&TIME.load1);
	/* Read apploader header */
	ret = WDVD_Read(buffer, 0x20, APPLDR_OFFSET);
	if (ret < 0)
		return ret;
	TIME.size += 0x20;

	/* Calculate apploader length */
	appldr_len = buffer[5] + buffer[6];

	/* Read apploader code */
	ret = WDVD_Read(appldr, appldr_len, APPLDR_OFFSET + 0x20);
	if (ret < 0)
		return ret;
	TIME.size += appldr_len;

	// used mem range by the loader
	//void *mem_start = (void*)0x80b00000; // as set in Makefile
	void *mem_start = (void*)0x80a80000; // as set in Makefile
	void *mem_end   = memalign(32,32);
	//printf("malloc = %p sta = %p\n", mem, &ret);

	/* Set apploader entry function */
	appldr_entry = (app_entry)buffer[4];

	/* Call apploader entry */
	appldr_entry(&appldr_init, &appldr_main, &appldr_final);

	/* Initialize apploader */
	appldr_init(__noprint);

	// dolStart, dolEnd added for giantpune's return-to patch by Dr. Clipper
	u32 dolStart = 0x90000000;
    u32 dolEnd = 0x0;

	//if (CFG.ios_yal)
	printf(".");
	for (;;) {
		void *dst = NULL;
		s32   len = 0, offset = 0;

		/* Run apploader main function */
		ret = appldr_main(&dst, &len, &offset);
		if (!ret)
			break;

		// check for overlap
		//printf("%p [%4x] %p\n", dst, len, dst+len);
		if ( ((dst > mem_start) && (dst < mem_end))
			|| ((dst+len > mem_start) && (dst+len < mem_end)) )
		{
			printf(gt("ERROR: memory overlap!"));
			printf("\n");
			printf(gt("dest: %p - %p"), dst, dst+len);
			printf("\n");
			printf(gt("used: %p - %p"), mem_start, mem_end);
			printf("\n");
			sleep(2);
			printf(gt("Press %s button to exit."), (button_names[CFG.button_exit.num]));
			printf("\n");
			Menu_PrintWait();
		}

		/* Read data from DVD */
		WDVD_Read(dst, len, (u64)(offset << 2));
		TIME.size += len;

		//if (CFG.ios_yal)
		printf(".");

		if (CFG.delay_patch) {
			// From NeoGamma: delay patches after load complete
			last_index++;
			dst_array[last_index] = dst;
			len_array[last_index] = len;
		} else {
			maindolpatches(dst, len);
		}

		// dolStart, dolEnd added for giantpune's return-to patch by Dr. Clipper
        if( (u32)dst < dolStart )dolStart = (u32)dst;
        if( (u32)dst + len > dolEnd ) dolEnd = (u32)dst + len;
		DCFlushRange(dst, len);
	}
	get_time(&TIME.load2);

	int j = 0;
	if (CFG.delay_patch) {
		dst_array[last_index+1] = (void *)0x81800000;
		
		fststart = 0;
		while ( j <= last_index && (u32)dst_array[last_index-j] + len_array[last_index-j] == (u32)dst_array[last_index-j+1]) 
		{
			fststart = last_index - j;
			j++;
		}		
		if (fststart == 0)
		{
			for (j = 4; j <= last_index; j++)
			{
				if ((u32)dst_array[j] == *(u32 *)0x80000038)
				{
					fststart = j;
				}
			}
			if (fststart == 0)
			{
				fststart = last_index;
			}
		}
	}

	


	/* Set entry point from apploader */
	*entry = appldr_final();
	//if (CFG.ios_yal)
	printf("\n\n");
	
	if (CFG.delay_patch) {
		// delayed patching (NeoGamma)
		for (j=3;j<fststart;j++)
		{
			maindolpatches(dst_array[j], len_array[j]);
		}	
	}
	
	do_wip_patches();
	wipreset();

	cfg_PatchReturnTo((void*)dolStart, dolEnd - dolStart);

	// alternative dol (WiiPower)
	if (CFG.game.alt_dol)
	{
		if (CFG.game.alt_dol >= ALT_DOL_DISC)
		{
			u32 doloffset = 0;
			getdolnames();
			if (CFG.game.alt_dol >= ALT_DOL_PLUS)
			{
				int dolm_i = CFG.game.alt_dol - ALT_DOL_PLUS + 1;
				if (dolm_i <= dolmenubuffer[0].count) {
					STRCOPY(CFG.game.dol_name, dolmenubuffer[dolm_i].dolname);
					dolparameter = dolmenubuffer[dolm_i].parameter;
					printf_x("alt.dol+: %s\n", dolmenubuffer[dolm_i].name);
					doloffset = getdoloffsetbyname(CFG.game.dol_name);
					printf_(">> %s (%d)\n", CFG.game.dol_name, dolparameter);
					printf_("");
					if (doloffset == 0) sleep(1);
				}
			} else {
				doloffset = Load_Dol_from_disc_menu();
				if (doloffset == 0)
				{
					printf_x(gt("Alternative .dol:"));
					printf("\n");
					printf_(gt("None found on disc"));
					printf("\n");
					sleep(2);
				}
			}
			if (doloffset)
			{
				*entry = (void*)Load_Dol_from_disc(doloffset);
				if (*entry == NULL) return -1;
				printf_(gt("Load OK!"));
				printf("\n");
			}
		} else if (CFG.game.alt_dol == ALT_DOL_SD)
		{
			u32 new_entry;
			printf_x(gt("Alternative .dol:"));
			printf("\n");
			new_entry = Load_Dol_from_sd();
			if (new_entry == 0) {
				// non-fatal error
				printf_(gt("Press any button..."));
				printf("\n");
				Wpad_WaitButtons();
				// continue without alt.dol
			} else if (new_entry == (u32)-1) {
				// fatal error
				return -1;
			} else {
				// ok.
				*entry = (void*)new_entry;
				printf_(gt("Load OK!"));
				printf("\n");
			}
		}
		__console_flush(0);
		usleep(500000);
		if (CFG.debug) {
			printf_(gt("Press any button..."));
			printf("\n");
			Wpad_WaitButtons();
		}
	}
	// printf("%p\n", *entry); Wpad_WaitButtons(); // exit(0);

	// cios 249 rev13 - 002 fix (by WiiPower)
	*(u32 *)0x80003140 = *(u32 *)0x80003188;
	// *(u32 *)0x80003188 = *(u32 *)0x80003140;
	/*
	u8 ios = CFG.ios;
	if (*(u8 *)0x80003189 == ios)
	{
		*(u32 *)0x80003140 = *(u32 *)0x80003188;
	} else
	{
		if (ios == IOS_GetVersion())
		{
			*(u32 *)0x80003188 = *(u32 *)0x80003140;
		} else
		{
			*(u8 *)0x80003141 = ios;
			*(u8 *)0x80003189 = ios;
			*(u16 *)0x80003142 = 0xffff;
			*(u16 *)0x8000318A = 0xffff;
		}
	}
	*/


	DCFlushRange((void*)0x80000000, 0x3f00);

	return 0;
}


extern GXRModeObj *disc_vmode;

// Based in Waninkoko patch
void __Patch_CoverRegister(void *buffer, u32 len)
{
	const u8 oldcode[] = {
		0x54, 0x60, 0xF7, 0xFF, 0x40, 0x82, 0x00, 0x0C,
		0x54, 0x60, 0x07, 0xFF, 0x41, 0x82, 0x00, 0x0C };
	const u8 newcode[] = {
		0x54, 0x60, 0xF7, 0xFF, 0x40, 0x82, 0x00, 0x0C,
		0x54, 0x60, 0x07, 0xFF, 0x48, 0x00, 0x00, 0x0C };
	int n;
	/* Patch cover register */
	for(n=0;n<(len-sizeof(oldcode));n+=4)
	{
		if (memcmp(buffer+n, (void *) oldcode, sizeof(oldcode)) == 0) 
		{
			memcpy(buffer+n, (void *) newcode, sizeof(newcode));
		}
	}
}

// PoP patch by Dr. Clipper (thanks giantpune, WiiCrazy)
bool PrinceOfPersiaPatch()
{
	if (memcmp("SPX", (char *)0x80000000, 3) == 0 || memcmp("RPW", (char *)0x80000000, 3) == 0) {
		u8 *p = (u8 *)0x807AEB6A;
		*p++ = 0x6F;
		*p++ = 0x6A;
		*p++ = 0x7A;
		*p++ = 0x6B;
		p = (u8 *)0x807AEB75;
		*p++ = 0x69;
		*p++ = 0x39;
		*p++ = 0x7C;
		*p++ = 0x7A;
		p = (u8 *)0x807AEB82;
		*p++ = 0x68;
		*p++ = 0x6B;
		*p++ = 0x73;
		*p++ = 0x76;
		p = (u8 *)0x807AEB92;
		*p++ = 0x75;
		*p++ = 0x70;
		*p++ = 0x80;
		*p++ = 0x71;
		p = (u8 *)0x807AEB9D;
		*p++ = 0x6F;
		*p++ = 0x3F;
		*p++ = 0x82;
		*p++ = 0x80;
		return true;
	}
	return false;
}

// NSMB patch by WiiPower
bool NewSuperMarioBrosPatch(void *Address, int Size)
{
	if (memcmp("SMN", (char *)0x80000000, 3) == 0)
	{
		u8 SearchPattern1[32] = { // PAL
			0x94, 0x21, 0xFF, 0xD0, 0x7C, 0x08, 0x02, 0xA6,
			0x90, 0x01, 0x00, 0x34, 0x39, 0x61, 0x00, 0x30,
			0x48, 0x12, 0xD9, 0x39, 0x7C, 0x7B, 0x1B, 0x78,
			0x7C, 0x9C, 0x23, 0x78, 0x7C, 0xBD, 0x2B, 0x78 };
		u8 SearchPattern2[32] = { // NTSC
			0x94, 0x21, 0xFF, 0xD0, 0x7C, 0x08, 0x02, 0xA6,
			0x90, 0x01, 0x00, 0x34, 0x39, 0x61, 0x00, 0x30,
			0x48, 0x12, 0xD7, 0x89, 0x7C, 0x7B, 0x1B, 0x78,
			0x7C, 0x9C, 0x23, 0x78, 0x7C, 0xBD, 0x2B, 0x78 };
		u8 PatchData[4] = { 0x4E, 0x80, 0x00, 0x20 };

		void *Addr = Address;
		void *Addr_end = Address+Size;

		while (Addr <= Addr_end-sizeof(SearchPattern1))
		{
			if (memcmp(Addr, SearchPattern1, sizeof(SearchPattern1))==0
					|| memcmp(Addr, SearchPattern2, sizeof(SearchPattern2))==0) 
			{
				memcpy(Addr,PatchData,sizeof(PatchData));
				return true;
			}
			Addr += 4;
		}
	}
	return false;
}

// from sneek by crediar
void sneek_video_patch(void *addr, u32 len)
{
	u8 *addr_start = addr;
	u8 *addr_end = addr+len;
	
	while(addr_start < addr_end)
	{
		if( *(vu32*)(addr_start) == 0x3C608000 )
		{
			if( ((*(vu32*)(addr_start+4) & 0xFC1FFFFF ) == 0x800300CC) && ((*(vu32*)(addr_start+8) >> 24) == 0x54 ) )
			{
				//dbgprintf("DIP:[patcher] Found VI pattern:%08X\n", (u32)(addr_start) | 0x80000000 );
				*(vu32*)(addr_start+4) = 0x5400F0BE | ((*(vu32*)(addr_start+4) & 0x3E00000) >> 5	);
			}
		}
		addr_start += 4;
	}
}

void patch_video_modes(void *dst, int len)
{
	GXRModeObj** table = NULL;
	if (CFG.game.video_patch == CFG_VIDEO_PATCH_ALL)
	{
		Search_and_patch_Video_To(dst, len, vmodes, disc_vmode);
	}
	else if (CFG.game.video_patch == CFG_VIDEO_PATCH_SNEEK)
	{
		sneek_video_patch(dst, len);
	}
	else if (CFG.game.video_patch == CFG_VIDEO_PATCH_SNEEK_ALL)
	{
		sneek_video_patch(dst, len);
		Search_and_patch_Video_To(dst, len, vmodes, disc_vmode);
	}
	else 
	{
		if (CFG.game.video_patch == CFG_VIDEO_PATCH_ON
			&& (CFG.game.video == CFG_VIDEO_SYS))
		{
			switch(CONF_GetVideo())
			{
			case CONF_VIDEO_PAL:
				if(CONF_GetEuRGB60() > 0) 
				{
					table = NTSC2PAL60;
				}	
				else
				{
					table = NTSC2PAL;
				}
				break;

			case CONF_VIDEO_MPAL:
				table = NTSC2PAL;
				break;

			default:
				table = PAL2NTSC;
				break;
			}
			Search_and_patch_Video_Modes(dst, len, table);
		}
		
		// force PAL50 (Narolez)
		if (CFG.game.video == CFG_VIDEO_PAL50) {
			Search_and_patch_Video_Modes(dst, len, NTSC2PAL);
		}
		if (CFG.game.video == CFG_VIDEO_PAL60) {
			Search_and_patch_Video_Modes(dst, len, NTSC2PAL60);
		}
		if (CFG.game.video == CFG_VIDEO_NTSC) {
			Search_and_patch_Video_Modes(dst, len, PAL2NTSC);
		}
	}
}

void maindolpatches(void *dst, int len)
{
	DCFlushRange(dst, len);

	if (CFG.game.clean == CFG_CLEAN_ALL) return;

	wipregisteroffset((u32)dst, len);

	patch_video_modes(dst, len);

	if (CFG.game.ocarina) {
		dogamehooks(dst,len);
	}
	if (CFG.game.vidtv) {
		vidolpatcher(dst,len);
	}
	/*LANGUAGE PATCH - FISHEARS*/
	if (CFG.game.language != CFG_LANG_CONSOLE) {
		langpatcher(dst,len);
	}
	// Country Patch by WiiPower
	if(CFG.game.country_patch) {
		PatchCountryStrings(dst, len);
	}

	// 002b fix from NeoGammaR4 by WiiPower:
	if (CFG.game.fix_002) {
	   	Anti_002_fix(dst, len);
	}
	// disc in drive check
	if (CFG.patch_dvd_check  && !CFG.disable_dvd_patch && !CFG.game.clean) {
		__Patch_CoverRegister(dst, len);
	}
	// NSMB patch by WiiPower
	if (!CFG.disable_nsmb_patch) {
		NewSuperMarioBrosPatch(dst, len);
	}
	// PoP patch
	if (!CFG.disable_pop_patch) {
		PrinceOfPersiaPatch();
	}

	DCFlushRange(dst, len);
}

bool disable_return_to_patch = false;

bool cfg_PatchReturnTo(void *Address, int Size)
{
	bool ret = false;
	if (disable_return_to_patch) return ret;
	// giantpune's return-to patch added by Dr. Clipper
	if (CFG.return_to > 2 && !CFG.game.clean) {
		ret = PatchReturnTo(Address, Size, CFG.return_to);
		if (ret) {
			//gprintf("return-to patched\n" );
			DCFlushRange(Address, Size);
		}
	}
	return ret;
}


// ALT. DOL


typedef struct {
    u8 filetype;
    char name_offset[3];
    u32 fileoffset;
    u32 filelen;
} __attribute__((packed)) FST_ENTRY;


char *fstfilename(u32 index)
{
	FST_ENTRY *fst = (FST_ENTRY *)*(u32 *)0x80000038;
	u32 count = fst[0].filelen;
	u32 stringoffset;
	if (index < count)
	{
		stringoffset = *(u32 *)&(fst[index]) % (256*256*256);
		return (char *)(*(u32 *)0x80000038 + count*12 + stringoffset);
	} else
	{
		return NULL;
	}
}

u32 fstfileoffset(u32 index)
{
	FST_ENTRY *fst = (FST_ENTRY *)*(u32 *)0x80000038;
	u32 count = fst[0].filelen;
	if (index < count)
	{
		return fst[index].fileoffset;
	} else
	{
		return 0;
	}
}

u32 Load_Dol_from_disc(u32 doloffset)
{
	int ret;
	void *dol_header = NULL;	
	u32 entrypoint;

	dol_header = memalign(32, sizeof(dolheader));
	if (dol_header == NULL)
	{
		printf(gt("Out of memory"));
		printf("\n");
		sleep(2);
		return 0;
	}

	//dvddone = 0;
	//ret = bwDVD_LowRead(dol_header, sizeof(dolheader), doloffset, __dvd_readidcb);
	//DVD_CHECK();
	ret = WDVD_Read(dol_header, sizeof(dolheader), (doloffset<<2));
	if (ret < 0) goto error;

	entrypoint = load_dol_start(dol_header);

	if (entrypoint == 0)
	{
error:
		printf(gt("Invalid .dol"));
		printf("\n");
		sleep(2);
		free(dol_header);
		return 0;
	}
	
	void *offset;
	u32 pos;
	u32 len;

	// dolStart, dolEnd added by Dr. Clipper for giantpune's return-to patch
	u32 dolStart = 0x90000000;
    u32 dolEnd = 0x0;

	int sec_idx = 0;
	
	printf_("...");
	while (load_dol_image(&offset, &pos, &len))
	{
		if (len != 0)
		{
			//dvddone = 0;
			//ret = bwDVD_LowRead(offset, len, (doloffset+pos/4), __dvd_readidcb);
			//DVD_CHECK();
			dbg_printf("\rdol [%d] @ 0x%08x [%6x] 0x%08x\n", sec_idx,
						(int)offset, len, (int)offset + len);
			ret = WDVD_Read(offset, len, (doloffset<<2) + pos);
			if (ret < 0) goto error;

			maindolpatches(offset, len);

            if( (u32)offset < dolStart )
                dolStart = (u32)offset;

            if( (u32)offset + len > dolEnd )
                dolEnd = (u32)offset + len;

			Remove_001_Protection(offset, len);
		}
		sec_idx++;
		printf(".");
	}
	printf("\n");
	
	cfg_PatchReturnTo((void*)dolStart, dolEnd - dolStart);

	free(dol_header);

	return entrypoint;
}	

void getdolnames()
{
	FST_ENTRY *fst = (FST_ENTRY *)*(u32 *)0x80000038;
	u32 count = fst[0].filelen;
	int i;

	dolcount = 0;

	for (i=1;i<count;i++)
	{		
		if (strstr(fstfilename(i), ".dol") != NULL)
		{
			if (dolcount < 10)
			{
				dolindex[dolcount] = i;
				dolcount++;
			}
		}		
	}
}

u32 stringcompare(char *s1, char *s2)
{
	u32 index = 0;
	while (true)
	{
		if (s1[index] == 0 || s2[index] == 0)
		{
			return 0;
		}
		if (s1[index] != '?' && s2[index] != '?' && toupper((u8)s1[index]) != toupper((u8)s2[index]))
		{
			return -1;
		}
		index++;	
	}
}

u32 getdoloffsetbyname(char *name)
{
	if (stringcompare(name, "main") == 0 && strlen(name) == 4)
	{
		return 0;
	}

	u32 i;
	u32 index = 0;
	
	for (i = 0; i < dolcount;i++)
	{
		if (stringcompare(fstfilename(dolindex[i]), name) == 0)
		{
			if (index != 0)
			{
				return 0;
			}
			index = dolindex[i];
		}
	}
	//memset(name, 0, 32);
	//strncpy(name, fstfilename(index), strlen(fstfilename(index)));
	strcpy(name, fstfilename(index));
	return fstfileoffset(index);
}

u32 Load_Dol_from_disc_menu()
{
	int i;
	u32 pressed;
	int dolselect = 0;

	if (dolcount == 0) return 0;
	
	if (*CFG.game.dol_name) {
		printf_x(gt("Using Saved Alternative .dol:"));
		printf("\n");
		printf_("[%s]\n", CFG.game.dol_name);
		sleep(1);
		if (strcmp(CFG.game.dol_name, "main.dol") == 0) {
			dolselect = 0;
			goto start;
		}
		for (i=0; i<dolcount; i++) {
			if (strcmp(CFG.game.dol_name, fstfilename(dolindex[i])) == 0) {
				dolselect = i + 1;
				goto start;
			}
		}
		printf_(gt("Not Found!"));
		printf("\n");
	}

	Gui_Console_Enable();
	printf_x(gt("Select Alternative .dol:"));
	printf("\n");
	printf_(gt("Press %s to save selection"), (button_names[CFG.button_save.num]));
	printf("\n");
	printf_(gt("Press %s to start game"), (button_names[CFG.button_confirm.num]));
	printf("\n");

	while (true)
	{
		//Con_Clear();
		//printf_x("Select Alternative .dol:\n");

		if (dolselect == 0)
		{
			printf("\r");
			printf_("<main.dol>");
		} else
		{
			printf("\r");
			printf_("<%s>", fstfilename(dolindex[dolselect-1]));
		}
		printf("          ");
		
		pressed = 0;
		
		pressed = Wpad_WaitButtonsCommon();
		
		if (pressed == WPAD_BUTTON_LEFT)
		{
			if (dolselect > 0)
			{
				dolselect--;
			} else
			{
				dolselect = dolcount;
			}
		}

		if (pressed == WPAD_BUTTON_RIGHT)
		{
			if (dolselect < dolcount)
			{
				dolselect++;
			} else
			{
				dolselect = 0;
			}
		}

		if (pressed & CFG.button_save.mask)
		{
			//Con_Clear();
			printf("\n");

			// remember alt.dol name
			u8 *gameid = (u8*)0x80000000;
			int ret;

			if (dolselect == 0) {
				STRCOPY(CFG.game.dol_name, "main.dol");
			} else {
				STRCOPY(CFG.game.dol_name, fstfilename(dolindex[dolselect-1]));
			}
			printf_(gt("Saving settings..."));
			printf("  ");
			ret = CFG_save_game_opt(gameid);
			if (ret) printf(gt("OK!"));
			else {
				printf("\n");
				printf_(gt("Error saving settings!"));
			}
			printf("\n");
			*CFG.game.dol_name = 0;

			sleep(1);
			goto start;
		}		

		if (pressed & CFG.button_confirm.mask)
		{
	start:
			printf("\n");
			//Con_Clear();
			if (dolselect == 0)
			{
				return 0;
			} else
			{
				return fstfileoffset(dolindex[dolselect-1]);			
			}			
		}		
	}		
}



u32 Load_Dol_from_sd()
{
	int ret;
	FILE* file;
	void *dol_header;	
	u32 entrypoint;
	
	char fname[128];
	char gameidbuffer4[5];
	memset(gameidbuffer4, 0, 5);
	memcpy(gameidbuffer4, (char*)0x80000000, 4);		
	//snprintf(buf, 128, "sd:/NeoGamma/%s.dol", gameidbuffer4);
	snprintf(fname, sizeof(fname), "%s/%s.dol", USBLOADER_PATH, gameidbuffer4);
	printf_("%s\n", fname);

	file = fopen(fname, "rb");
	
	if(file == NULL) 
	{
		printf_(gt("Not Found!"));
		printf("\n");
		sleep(4);
		return 0;
	}
	
	int filesize;
	fseek(file, 0, SEEK_END);
	filesize = ftell(file);
	fseek(file, 0, SEEK_SET);

	dol_header = memalign(32, sizeof(dolheader));
	if (dol_header == NULL)
	{
		printf(gt("Out of memory"));
		printf("\n");
		sleep(2);
		fclose(file);
		return 0;
	}

	ret = fread( dol_header, 1, sizeof(dolheader), file);
	if(ret != sizeof(dolheader))
	{
		printf(gt("Error reading dol header"));
		printf("\n");
		sleep(2);
		free(dol_header);
		fclose(file);
		return 0;
	}
	
	entrypoint = load_dol_start(dol_header);
	
	if (entrypoint == 0)
	{
		printf(gt("Invalid .dol"));
		printf("\n");
		sleep(2);
		free(dol_header);
		fclose(file);
		return 0;
	}
	
	void *offset;
	u32 pos;
	u32 len;

	// dolStart, dolEnd added by Dr. Clipper for giantpune's return-to patch
	u32 dolStart = 0x90000000;
    u32 dolEnd = 0x0;

	int sec_idx = 0;
	
	printf_("...");
	while (load_dol_image(&offset, &pos, &len))
	{
		if(pos+len > filesize)
		{
			printf(gt(".dol too small"));
			printf("\n");
			sleep(2);
			free(dol_header);
			fclose(file);
			return -1;
		}		
		
		if (len != 0)
		{
			dbg_printf("\rdol [%d] @ 0x%08x [%6x] 0x%08x\n", sec_idx,
						(int)offset, len, (int)offset + len);
			fseek(file, pos, 0);
			ret = fread( offset, 1, len, file);
			if(ret != len)
			{
				printf(gt("Error reading .dol"));
				printf("\n");
				sleep(2);
				free(dol_header);
				fclose(file);
				return -1;
			}
			maindolpatches(offset, len);

            if( (u32)offset < dolStart )
                dolStart = (u32)offset;

            if( (u32)offset + len > dolEnd )
                dolEnd = (u32)offset + len;

			Remove_001_Protection(offset, len);
		}	
		sec_idx++;
		printf(".");
	}
	printf("\n");
	
	cfg_PatchReturnTo((void*)dolStart, dolEnd - dolStart);

	free(dol_header);
	fclose(file);

	return entrypoint;
}	


static u8  *diskid = (u8  *)0x80000000;

void PatchCountryStrings(void *Address, int Size)
{
    u8 SearchPattern[4]    = { 0x00, 0x00, 0x00, 0x00 };
    u8 PatchData[4]        = { 0x00, 0x00, 0x00, 0x00 };
    u8 *Addr            = (u8*)Address;

    int wiiregion = CONF_GetRegion();

    switch (wiiregion)
    {
        case CONF_REGION_JP:
            SearchPattern[0] = 0x00;
            SearchPattern[1] = 0x4A; // J
            SearchPattern[2] = 0x50; // P
            break;
        case CONF_REGION_EU:
            SearchPattern[0] = 0x02;
            SearchPattern[1] = 0x45; // E
            SearchPattern[2] = 0x55; // U
            break;
        case CONF_REGION_KR:
            SearchPattern[0] = 0x04;
            SearchPattern[1] = 0x4B; // K
            SearchPattern[2] = 0x52; // R
            break;
        case CONF_REGION_CN:
            SearchPattern[0] = 0x05;
            SearchPattern[1] = 0x43; // C
            SearchPattern[2] = 0x4E; // N
            break;
        case CONF_REGION_US:
        default:
            SearchPattern[0] = 0x01;
            SearchPattern[1] = 0x55; // U
            SearchPattern[2] = 0x53; // S
    }

    switch (diskid[3])
    {
        case 'J':
            PatchData[1] = 0x4A; // J
            PatchData[2] = 0x50; // P
            break;

        case 'D':
        case 'F':
        case 'P':
        case 'X':
        case 'Y':
            PatchData[1] = 0x45; // E
            PatchData[2] = 0x55; // U
            break;

        case 'E':
        default:
            PatchData[1] = 0x55; // U
            PatchData[2] = 0x53; // S
    }

    while (Size >= 4)
    {
        if (Addr[0] == SearchPattern[0] && Addr[1] == SearchPattern[1] && Addr[2] == SearchPattern[2] && Addr[3] == SearchPattern[3])
        {
            //*Addr = PatchData[0];
            Addr += 1;
            *Addr = PatchData[1];
            Addr += 1;
            *Addr = PatchData[2];
            Addr += 1;
            //*Addr = PatchData[3];
            Addr += 1;
            Size -= 4;
        } else
        {
            Addr += 4;
            Size -= 4;
        }
    }
}

bool Remove_001_Protection(void *Address, int Size)
{
	u8 SearchPattern[16] = 	{ 0x40, 0x82, 0x00, 0x0C, 0x38, 0x60, 0x00, 0x01, 0x48, 0x00, 0x02, 0x44, 0x38, 0x61, 0x00, 0x18 };
	u8 PatchData[16] = 		{ 0x40, 0x82, 0x00, 0x04, 0x38, 0x60, 0x00, 0x01, 0x48, 0x00, 0x02, 0x44, 0x38, 0x61, 0x00, 0x18 };

	void *Addr = Address;
	void *Addr_end = Address+Size;

	while(Addr <= Addr_end-sizeof(SearchPattern))
	{
		if(memcmp(Addr, SearchPattern, sizeof(SearchPattern))==0) 
		{
			memcpy(Addr,PatchData,sizeof(PatchData));
			return true;
		}
		Addr += 4;
	}
	return false;
}

void Anti_002_fix(void *Address, int Size)
{
	u8 SearchPattern[12] = 	{ 0x2C, 0x00, 0x00, 0x00, 0x48, 0x00, 0x02, 0x14, 0x3C, 0x60, 0x80, 0x00 };
	u8 PatchData[12] = 		{ 0x2C, 0x00, 0x00, 0x00, 0x40, 0x82, 0x02, 0x14, 0x3C, 0x60, 0x80, 0x00 };

	void *Addr = Address;
	void *Addr_end = Address+Size;

	while(Addr <= Addr_end-sizeof(SearchPattern))
	{
		if(memcmp(Addr, SearchPattern, sizeof(SearchPattern))==0) 
		{
			memcpy(Addr,PatchData,sizeof(PatchData));
		}
		Addr += 4;
	}
}

