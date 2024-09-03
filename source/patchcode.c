#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <sys/unistd.h>

#include "apploader.h"
#include "patchcode.h"
#include "cfg.h"

//#include "sd.h"
//#include "fwrite_patch.h"
//#include "fwrite_patch_slota.h"
//#include "main.h"
#define debug_printf(fmt, args...)

bool hookpatched = false;

extern void patchhook(u32 address, u32 len);
extern void patchhook2(u32 address, u32 len);
extern void patchhook3(u32 address, u32 len);

extern void multidolpatchone(u32 address, u32 len);
extern void multidolpatchtwo(u32 address, u32 len);

extern void regionfreejap(u32 address, u32 len);
extern void regionfreeusa(u32 address, u32 len);
extern void regionfreepal(u32 address, u32 len);

extern void removehealthcheck(u32 address, u32 len);

extern void copyflagcheck1(u32 address, u32 len);
extern void copyflagcheck2(u32 address, u32 len);
extern void copyflagcheck3(u32 address, u32 len);
extern void copyflagcheck4(u32 address, u32 len);
extern void copyflagcheck5(u32 address, u32 len);

extern void patchupdatecheck(u32 address, u32 len);

extern void movedvdhooks(u32 address, u32 len);

extern void multidolhook(u32 address);
extern void langvipatch(u32 address, u32 len, u8 langbyte);
extern void vipatch(u32 address, u32 len);

static const u32 multidolpatch1[2] = {
	0x3C03FFB4,0x28004F43
};	

static const u32 healthcheckhook[2] = {
	0x41810010,0x881D007D
};	

static const u32 updatecheckhook[3] = {
	0x80650050,0x80850054,0xA0A50058
};

static const u32 multidolpatch2[2] = {
	0x3F608000, 0x807B0018
};

static const u32 recoveryhooks[3] = {
	0xA00100AC,0x5400073E,0x2C00000F
};

static const u32 nocopyflag1[3] = {
	0x540007FF, 0x4182001C, 0x80630068
};

static const u32 nocopyflag2[3] = {
	0x540007FF, 0x41820024, 0x387E12E2
};

// this one is for the GH3 and VC saves
//static const u32 nocopyflag3[5] = {
//	0x2C030000, 0x40820010, 0x88010020, 0x28000002, 0x41820234
//};

static const u32 nocopyflag3[5] = {
	0x2C030000, 0x41820200,0x48000058,0x38610100
};
// this removes the display warning for no copy VC and GH3 saves
static const u32 nocopyflag4[4] = {
	0x80010008, 0x2C000000, 0x4182000C, 0x3BE00001
};

static const u32 nocopyflag5[3] = {
	0x801D0024,0x540007FF,0x41820024
};

static const u32 movedvdpatch[3] = {
	0x2C040000, 0x41820120, 0x3C608109
};

static const u32 regionfreehooks[5] = {
	0x7C600774, 0x2C000001, 0x41820030,0x40800010,0x2C000000
};

static const u32 cIOScode[16] = {
	0x7f06c378, 0x7f25cb78, 0x387e02c0, 0x4cc63182
};

static const u32 cIOSblock[16] = {
	0x2C1800F9, 0x40820008, 0x3B000024
};

static const u32 fwritepatch[8] = {
	0x9421FFD0,0x7C0802A6,0x90010034,0xBF210014,0x7C9B2378,0x7CDC3378,0x7C7A1B78,0x7CB92B78  // bushing fwrite
};

static const u32 vipatchcode[3] = {
0x4182000C,0x4180001C,0x48000018
};

const u32 viwiihooks[4] = {
	0x7CE33B78,0x38870034,0x38A70038,0x38C7004C 
};

const u32 kpadhooks[4] = {
	0x9A3F005E,0x38AE0080,0x389FFFFC,0x7E0903A6
};

const u32 kpadoldhooks[6] = {
	0x801D0060, 0x901E0060, 0x801D0064, 0x901E0064, 0x801D0068, 0x901E0068
};

const u32 joypadhooks[4] = {
	0x3AB50001, 0x3A73000C, 0x2C150004, 0x3B18000C
};

const u32 gxdrawhooks[4] = {
	0x3CA0CC01, 0x38000061, 0x3C804500, 0x98058000
};

const u32 gxflushhooks[4] = {
	0x90010014, 0x800305FC, 0x2C000000, 0x41820008
};

const u32 ossleepthreadhooks[4] = {
	0x90A402E0, 0x806502E4, 0x908502E4, 0x2C030000
};

const u32 axnextframehooks[4] = {
	0x3800000E, 0x7FE3FB78, 0xB0050000, 0x38800080
};

const u32 wpadbuttonsdownhooks[4] = {
	0x7D6B4A14, 0x816B0010, 0x7D635B78, 0x4E800020
};

const u32 wpadbuttonsdown2hooks[4] = {
	0x7D6B4A14, 0x800B0010, 0x7C030378, 0x4E800020
};

const u32 multidolhooks[4] = {
	0x7C0004AC, 0x4C00012C, 0x7FE903A6, 0x4E800420
};

const u32 multidolchanhooks[4] = {
	0x4200FFF4, 0x48000004, 0x38800000, 0x4E800020
};

const u32 langpatch[3] = {
	0x7C600775, 0x40820010, 0x38000000
};

static const u32 oldpatch002[3] = {
	0x2C000000, 0x40820214, 0x3C608000
};

static const u32 newpatch002[3] = {
	0x2C000000, 0x48000214, 0x3C608000
};

//---------------------------------------------------------------------------------
bool dogamehooks(void *addr, u32 len)
//---------------------------------------------------------------------------------
{
	// when using Ocarina check if a hook as patched (in ocarina_do_code)

	if (!CFG.game.ocarina) return false;

	//hooktype = 1;
	hooktype = CFG.game.hooktype;
	/*
	0 No Hook
	1 VBI
	2 KPAD read
	3 Joypad Hook
	4 GXDraw Hook
	5 GXFlush Hook
	6 OSSleepThread Hook
	7 AXNextFrame Hook
	*/

	void *addr_start = addr;
	void *addr_end = addr+len;

	while(addr_start < addr_end)
	{
		switch(hooktype)
		{
		
			case 0x00:	
				hookpatched = true;
			break;

			case 0x01:	
				if(memcmp(addr_start, viwiihooks, sizeof(viwiihooks))==0)
				{
					patchhook((u32)addr_start, len);
					hookpatched = true;
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0)
				{
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
					hookpatched = true;
				}
			break;

			case 0x02:
				
				if(memcmp(addr_start, kpadhooks, sizeof(kpadhooks))==0)
				{
					patchhook((u32)addr_start, len);
					hookpatched = true;
				}

				if(memcmp(addr_start, kpadoldhooks, sizeof(kpadoldhooks))==0)
				{
					patchhook((u32)addr_start, len);
					hookpatched = true;
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0)
				{
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
					hookpatched = true;
				}
			break;
		
			case 0x03:
				
				if(memcmp(addr_start, joypadhooks, sizeof(joypadhooks))==0)
				{
					patchhook((u32)addr_start, len);
					hookpatched = true;
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0)
				{
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
					hookpatched = true;
				}
			break;
				
			case 0x04:
				
				if(memcmp(addr_start, gxdrawhooks, sizeof(gxdrawhooks))==0)
				{
					patchhook((u32)addr_start, len);
					hookpatched = true;
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0)
				{
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
					hookpatched = true;
				}
			break;
				
			case 0x05:
				
				if(memcmp(addr_start, gxflushhooks, sizeof(gxflushhooks))==0)
				{
					patchhook((u32)addr_start, len);
					hookpatched = true;
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0)
				{
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
					hookpatched = true;
				}
			break;
				
			case 0x06:
				
				if(memcmp(addr_start, ossleepthreadhooks, sizeof(ossleepthreadhooks))==0)
				{
					patchhook((u32)addr_start, len);
					hookpatched = true;
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0)
				{
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
					hookpatched = true;
				}
			break;
				
			case 0x07:
				
				if(memcmp(addr_start, axnextframehooks, sizeof(axnextframehooks))==0)
				{
					patchhook((u32)addr_start, len);
					hookpatched = true;
				}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0)
				{
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
					hookpatched = true;
				}
			break;
				
			case 0x08:
				
				//if(memcmp(addr_start, customhook, customhooksize)==0)
				//{
				//	patchhook((u32)addr_start, len);
				//	hookpatched = true;
				//}
				if(memcmp(addr_start, multidolhooks, sizeof(multidolhooks))==0)
				{
					multidolhook((u32)addr_start+sizeof(multidolhooks)-4);
					hookpatched = true;
				}
			break;
		}
		addr_start += 4;
	}
	return hookpatched;
}

// Not used yet, for patching DOL once loaded into memory and befor execution
/*
void patchdol(void *addr, u32 len)
{
	
	void *addr_start = addr;
	void *addr_end = addr+len;

	while(addr_start < addr_end)
	{
		if(memcmp(addr_start, wpadlibogc, sizeof(wpadlibogc))==0) {
	//		printf("\n\n\n");
	//		printf("found at address %x\n", addr_start);
		//	sleep(10);
	//		patchhookdol((u32)addr_start, len);
			patched = 1;
			break;
		}
		addr_start += 4;
	}
}
*/
void langpatcher(void *addr, u32 len)
{
	
	void *addr_start = addr;
	void *addr_end = addr+len;

	while(addr_start < addr_end)
	{
		
		if(memcmp(addr_start, langpatch, sizeof(langpatch))==0) {
			if(configbytes[0] != 0xCD){
				langvipatch((u32)addr_start, len, configbytes[0]);
			}	
		}
		addr_start += 4;
	}
}
/*
void patchdebug(void *addr, u32 len)
{
	
	void *addr_start = addr;
	void *addr_end = addr+len;

	while(addr_start < addr_end)
	{
		
		if(memcmp(addr_start, fwritepatch, sizeof(fwritepatch))==0) {

			memcpy(addr_start,fwrite_patch_bin,fwrite_patch_bin_len);
			// apply patch	
		}
		addr_start += 4;
	}
}
*/
void vidolpatcher(void *addr, u32 len)
{
	
	void *addr_start = addr;
	void *addr_end = addr+len;

	while(addr_start < addr_end)
	{
		if(memcmp(addr_start, vipatchcode, sizeof(vipatchcode))==0) {
			vipatch((u32)addr_start, len);
		}
		addr_start += 4;
	}
}

//giantpune's magic super patch to return to channels - Added by Dr. Clipper
bool PatchReturnTo(void *Address, int Size, u32 id) {
    if( !id )return 0;
    //new __OSLoadMenu() (SM2.0 and higher)
    u8 SearchPattern[ 12 ] =    { 0x38, 0x80, 0x00, 0x02, 0x38, 0x60, 0x00, 0x01, 0x38, 0xa0, 0x00, 0x00 };

    //old _OSLoadMenu() (used in launch games)
    u8 SearchPatternB[ 12 ] =   { 0x38, 0xC0, 0x00, 0x02, 0x38, 0xA0, 0x00, 0x01, 0x38, 0xE0, 0x00, 0x00 };

    //identifier for the safe place
    u8 SearchPattern2[ 12 ] =   { 0x4D, 0x65, 0x74, 0x72, 0x6F, 0x77, 0x65, 0x72, 0x6B, 0x73, 0x20, 0x54 };


    int found = 0;
    int patched = 0;
    u8 oldSDK = 0;
    u32 ad[ 4 ] = { 0, 0, 0, 0 };

    void *Addr = Address;
    void *Addr_end = Address+Size;

    while (Addr <= Addr_end - 12 ) {
        //find a safe place or the patch to hang out
        if ( ! ad[ 3 ] && memcmp( Addr, SearchPattern2, 12 )==0 ) {
            ad[ 3 ] = (u32)Addr + 0x30;
            debug_printf("found a safe place @ %08x\n", ad[ 3 ]);
            //hexdump( Addr, 0x50 );
        }
        //find __OSLaunchMenu() and remember some addresses in it
        else if ( memcmp( Addr, SearchPattern, 12 )==0 ) {
            ad[ found++ ] = (u32)Addr;
        }
        else if ( ad[ 0 ] && memcmp( Addr, SearchPattern, 8 )==0 ) //after the first match is found, only search the first 8 bytes for the other 2
        {
            if( !ad[ 1 ] ) ad[ found++ ] = (u32)Addr;
            else if( !ad[ 2 ] ) ad[ found++ ] = (u32)Addr;
            if( found >= 3 )break;
        }
        Addr += 4;
    }
    //check for the older-ass version of the SDK
    if( found < 3 && ad[ 3 ] )
    {
        Addr = Address;
        ad[ 0 ] = 0; 
		ad[ 1 ] = 0;
        ad[ 2 ] = 0;
        found = 0;
        oldSDK = 1;

        while (Addr <= Addr_end - 12 ) {
            //find __OSLaunchMenu() and remember some addresses in it
            if ( memcmp( Addr, SearchPatternB, 12 )==0 ) {
                ad[ found++ ] = (u32)Addr;
            }
            else if ( ad[ 0 ] && memcmp( Addr, SearchPatternB, 8 ) == 0 ) //after the first match is found, only search the first 8 bytes for the other 2
            {
                if( !ad[ 1 ] ) ad[ found++ ] = (u32)Addr;
                else if( !ad[ 2 ] ) ad[ found++ ] = (u32)Addr;
                if( found >= 3 )break;
            }
            Addr += 4;
        }
    }

    //if the function is found and if it is not too far into the main.dol
    if( found == 3 && ( ad[ 2 ] - ad[ 3 ] < 0x1000001 ) && ad[ 3 ] )
    {
        debug_printf("patch __OSLaunchMenu( 0x00010001, 0x%08x )\n", id);
        u32 nop = 0x60000000;

        //the magic that writes the TID to the registers
        u8 jump[ 20 ] = { 0x3C, 0x60, 0x00, 0x01, 0x60, 0x63, 0x00, 0x01,
                          0x3C, 0x80, 0x4A, 0x4F, 0x60, 0x84, 0x44, 0x49,
                          0x4E, 0x80, 0x00, 0x20 };
        if( oldSDK )
        {
            jump[ 1 ] = 0xA0; //3CA00001 60A50001
            jump[ 5 ] = 0xA5; //3CC04A4F 60C64449
            jump[ 9 ] = 0xC0;
            jump[ 13 ] = 0xC6;
        }
        //patch the thing to use the new TID
        jump[ 10 ] = (u8)( id >> 24 );
        jump[ 11 ] = (u8)( id >> 16 );
        jump[ 14 ] = (u8)( id >> 8 );
        jump[ 15 ] = (u8)id;

        void* addr = (u32*)ad[ 3 ];

        //write new stuff to memory main.dol in a unused part of the main.dol
        memcpy( addr, jump, sizeof( jump ) );

        //ES_GetTicketViews()
        u32 newval = ( ad[ 3 ] - ad[ 0 ] );
        newval &= 0x03FFFFFC;
        newval |= 0x48000001;
        addr = (u32*)ad[ 0 ];
        memcpy( addr, &newval, sizeof( u32 ) );
        memcpy( addr + 4, &nop, sizeof( u32 ) );
        debug_printf("\t%p -> %08x\n", addr, newval );

        //ES_GetTicketViews() again
        newval = ( ad[ 3 ] - ad[ 1 ] );
        newval &= 0x03FFFFFC;
        newval |= 0x48000001;
        addr = (u32*)ad[ 1 ];
        memcpy( addr, &newval, sizeof( u32 ) );
        memcpy( addr + 4, &nop, sizeof( u32 ) );
        debug_printf("\t%p -> %08x\n", addr, newval );

        //ES_LaunchTitle()
        newval = ( ad[ 3 ] - ad[ 2 ] );
        newval &= 0x03FFFFFC;
        newval |= 0x48000001;
        addr = (u32*)ad[ 2 ];
        memcpy( addr, &newval, sizeof( u32 ) );
        memcpy( addr + 4, &nop, sizeof( u32 ) );
        debug_printf("\t%p -> %08x\n", addr, newval );

        patched = 1;
    }
    else
    {
        debug_printf("not patched\n");
        debug_printf("found %d addresses\n", found);
        int i;
        for( i = 0; i< 4; i++)
            debug_printf("ad[ %d ]: %08x\n", i, ad[ i ] );
        debug_printf("offset : %08x\n", ad[ 2 ] - ad[ 3 ] );

    }
    return patched;
}

void WFCPatch(void *addr, u32 len, const char* domain)
{
	if(strlen("nintendowifi.net") < strlen(domain))
		return;

	char *cur = (char *)addr;
	const char *end = cur + len - 16;
	
	do
	{
		if (memcmp(cur, "nintendowifi.net", 16) == 0)
		{
			int len = strlen(cur);
			u8 i;
			memcpy(cur, domain, strlen(domain));
			memmove(cur + strlen(domain), cur + 16, len - 16);
			for(i = 16 - strlen(domain); i > 0 ; i--)
				cur[len - i ] = 0;
			cur += len;
		}
	} while (++cur < end);
}

static inline int GetOpcode(unsigned int* instructionAddr)
{
	return ((*instructionAddr >> 26) & 0x3f);
}

static inline int GetImmediateDataVal(unsigned int* instructionAddr)
{
	return (*instructionAddr & 0xffff);
}

static inline int GetLoadTargetReg(unsigned int* instructionAddr)
{
	return (int)((*instructionAddr >> 21) & 0x1f);
}

static inline int GetComparisonTargetReg(unsigned int* instructionAddr)
{
	return (int)((*instructionAddr >> 16) & 0x1f);
}

u32 do_new_wiimmfi_nonMKWii() {
		// As of February 2021, Wiimmfi requires a special Wiimmfi patcher 
		// update which does a bit more than just patch the server adresses. 
		// This function is being called by apploader.c, right before 
		// jumping to the entry point (only for non-MKWii games on Wiimmfi), 
		// and applies all the necessary security fixes to the game. 

		// This function has been implemented by Leseratte. Please don't
		// try to modify it without speaking to the Wiimmfi team because
		// doing so could have unintended side effects. 

		int hasGT2Error = 0;
		int i = 0;
		int dynamic = 0;
        char gt2locator[] = { 0x38, 0x61, 0x00, 0x08, 0x38, 0xA0, 0x00, 0x14};        
        
    	unsigned char opCodeChainP2P_v1[22] =    { 32, 32, 21, 21, 21, 21, 20, 20, 31, 40, 21, 20, 20, 31, 31, 10, 20, 36, 21, 44, 36, 16 };
    	unsigned char opCodeChainP2P_v2[22] =    { 32, 32, 21, 21, 20, 21, 20, 21, 31, 40, 21, 20, 20, 31, 31, 10, 20, 36, 21, 44, 36, 16 };

    	unsigned char opCodeChainMASTER_v1[22] = { 21, 21, 21, 21, 40, 20, 20, 20, 20, 31, 31, 14, 31, 20, 21, 44, 21, 36, 36, 18, 11, 16 };
    	unsigned char opCodeChainMASTER_v2[22] = { 21, 21, 21, 21, 40, 20, 20, 20, 20, 31, 31, 14, 31, 20, 21, 36, 21, 44, 36, 18, 11, 16 };
       

        int MASTERopcodeChainOffset = 0;

		char * cur = (char *)0x80004000; 
		const char * end = (const char *)0x80900000;

		// Check if the game needs the new patch. 
		do {
			if (memcmp(cur, "<GT2> RECV-0x%02x <- [--------:-----] [pid=%u]", 0x2e) == 0) 
            {
                hasGT2Error++;
            }
		} while (++cur < end); 

		cur = (char *)0x80004000; 

		if (hasGT2Error > 1) return 1; 	// error, this either doesn't exist, or exists once. Can't exist multiple times. 

		int successful_patch_p2p = 0; 
		int successful_patch_master = 0;


		do {

			// Patch the User-Agent so Wiimmfi knows this game has been patched. 
			// This also identifies patcher (U=CfgUSBLoader) and patch version (=1), please
			// do not change this without talking to Leseratte first.
			if (memcmp(cur, "User-Agent\x00\x00RVL SDK/", 20) == 0) {

				if (hasGT2Error) 
					memcpy(cur + 12, "U-3-1\x00", 6); 
				else
					memcpy(cur + 12, "U-3-0\x00", 6); 
				
			}

			if (hasGT2Error)
			{
				if (memcmp(cur, &gt2locator, 8) == 0)
				{
					int found_opcode_chain_P2P_v1 = 1; 
					int found_opcode_chain_P2P_v2 = 1; 
		
						for (i = 0; i < 22; i++) {
							int offset = (i * 4) + 12;
							if (opCodeChainP2P_v1[i] != (unsigned char)(GetOpcode((unsigned int *)(cur + offset)))) {
								found_opcode_chain_P2P_v1 = 0; 
							}
							if (opCodeChainP2P_v2[i] != (unsigned char)(GetOpcode((unsigned int *)(cur + offset)))) {
								found_opcode_chain_P2P_v2 = 0; 
							}
						}
						int found_opcode_chain_MASTER;
						for (dynamic = 0; dynamic < 40; dynamic += 4) {
							found_opcode_chain_MASTER = 1; 
							int offset = 0; 
							for (i = 0; i < 22; i++) {
								offset = (i * 4) + 12 + dynamic;
								if (
									(opCodeChainMASTER_v1[i] != (unsigned char)(GetOpcode((unsigned int *)(cur + offset)))) && 
									(opCodeChainMASTER_v2[i] != (unsigned char)(GetOpcode((unsigned int *)(cur + offset))))
								) {
									found_opcode_chain_MASTER = 0; 
								}
							}

							if (found_opcode_chain_MASTER) {
								MASTERopcodeChainOffset = (int)(cur + 12 + dynamic);
								break;
							}

						}
						if (found_opcode_chain_P2P_v1 || found_opcode_chain_P2P_v2) {

							if (
								GetImmediateDataVal((unsigned int *)(cur + 0x0c)) == 0x0c && 
								GetImmediateDataVal((unsigned int *)(cur + 0x10)) == 0x18 &&
								GetImmediateDataVal((unsigned int *)(cur + 0x30)) == 0x12 &&
								GetImmediateDataVal((unsigned int *)(cur + 0x48)) == 0x5a &&
								GetImmediateDataVal((unsigned int *)(cur + 0x50)) == 0x0c && 
								GetImmediateDataVal((unsigned int *)(cur + 0x58)) == 0x12 && 
								GetImmediateDataVal((unsigned int *)(cur + 0x5c)) == 0x18 && 
								GetImmediateDataVal((unsigned int *)(cur + 0x60)) == 0x18
							)
							{
							
								int loadedDataReg = GetLoadTargetReg((unsigned int *)(cur + 0x14));
								int comparisonDataReg = GetComparisonTargetReg((unsigned int *)(cur + 0x48));
								
								if (found_opcode_chain_P2P_v1) {
									
									*(int *)(cur + 0x14) = (0x88010011 | (comparisonDataReg << 21)); 
									*(int *)(cur + 0x18) = (0x28000080 | (comparisonDataReg << 16)); 
									*(int *)(cur + 0x24) = 0x41810064;                               
									*(int *)(cur + 0x28) = 0x60000000;                               
									*(int *)(cur + 0x2c) = 0x60000000;                               
									*(int *)(cur + 0x34) = (0x3C005A00 | (comparisonDataReg << 21)); 
									*(int *)(cur + 0x48) = (0x7C000000 | (comparisonDataReg << 16) | (loadedDataReg << 11)); 
									successful_patch_p2p++;
								}
								if (found_opcode_chain_P2P_v2) {

									loadedDataReg = 12;

									*(int *)(cur + 0x14) = (0x88010011 | (comparisonDataReg << 21)); 
									*(int *)(cur + 0x18) = (0x28000080 | (comparisonDataReg << 16)); 
									*(int *)(cur + 0x1c) = 0x41810070; 
									*(int *)(cur + 0x24) = *(int *)(cur + 0x28); 
									*(int *)(cur + 0x28) = (0x8001000c | (loadedDataReg << 21)); 
									*(int *)(cur + 0x2c) = (0x3C005A00 | (comparisonDataReg << 21)); 
									*(int *)(cur + 0x34) = (0x7c000000 | (comparisonDataReg << 16) | (loadedDataReg << 11)); 
									*(int *)(cur + 0x48) = 0x60000000; 
									successful_patch_p2p++;
								}

							}
						}

						else if (found_opcode_chain_MASTER) {
							if (
								GetImmediateDataVal((unsigned int *)(MASTERopcodeChainOffset + 0x10)) == 0x12 &&
								GetImmediateDataVal((unsigned int *)(MASTERopcodeChainOffset + 0x2c)) == 0x04 &&
								
								GetImmediateDataVal((unsigned int *)(MASTERopcodeChainOffset + 0x48)) == 0x18 &&
								GetImmediateDataVal((unsigned int *)(MASTERopcodeChainOffset + 0x50)) == 0x00 &&
								GetImmediateDataVal((unsigned int *)(MASTERopcodeChainOffset + 0x54)) == 0x18
							)
							{


								int master_patch_version = 0; 

								// Check which version we have:
								if ((GetImmediateDataVal((unsigned int *)(MASTERopcodeChainOffset + 0x3c)) == 0x12 && 
								GetImmediateDataVal((unsigned int *)(MASTERopcodeChainOffset + 0x44)) == 0x0c) ) {
									master_patch_version = 1; 
								}
								else if ((GetImmediateDataVal((unsigned int *)(MASTERopcodeChainOffset + 0x3c)) == 0x0c && 
								GetImmediateDataVal((unsigned int *)(MASTERopcodeChainOffset + 0x44)) == 0x12) ) {
									master_patch_version = 2; 
								}


								if (master_patch_version == 2) {
									// Different opcode order ...
									*(int *)(MASTERopcodeChainOffset + 0x3c) = *(int *)(MASTERopcodeChainOffset + 0x44);
								}

								if (master_patch_version != 0) {
									int rY = GetComparisonTargetReg((unsigned int *)MASTERopcodeChainOffset);
									int rX = GetLoadTargetReg((unsigned int *)MASTERopcodeChainOffset); 

									*(int *)(MASTERopcodeChainOffset + 0x00) = 0x38000004 | (rX << 21); 
									*(int *)(MASTERopcodeChainOffset + 0x04) = 0x7c00042c | (rY << 21) | (3 << 16) | (rX << 11); 
									*(int *)(MASTERopcodeChainOffset + 0x14) = 0x9000000c | (rY << 21) | (1 << 16); 
									*(int *)(MASTERopcodeChainOffset + 0x18) = 0x88000011 | (rY << 21) | (1 << 16); 
									*(int *)(MASTERopcodeChainOffset + 0x28) = 0x28000080 | (rY << 16); 
									*(int *)(MASTERopcodeChainOffset + 0x38) = 0x60000000; 
									*(int *)(MASTERopcodeChainOffset + 0x44) = 0x41810014; 
									successful_patch_master++;

								}

							}
                    	}

				}
			}

		} while (++cur < end); 

		if (hasGT2Error) {
			if (successful_patch_master == 0 || successful_patch_p2p == 0) {
				return 2; 
			}
		}

		return 0;

}

u32 do_new_wiimmfi() {

	// As of November 2018, Wiimmfi requires a special Wiimmfi patcher 
	// update which does a bit more than just patch the server adresses. 
	// This function is being called by apploader.c, right before 
	// jumping to the entry point (only for Mario Kart Wii & Wiimmfi), 
	// and applies all the necessary new patches to the game. 
	// This includes support for the new patcher update plus
	// support for StaticR.rel patching. 
	
	// This function has been implemented by Leseratte. Please don't
	// try to modify it without speaking to the Wiimmfi team because
	// doing so could have unintended side effects. 

	// Updated in 2021 to add the 51420 error fix.
	
	// check region: 
	char region = *((char *)(0x80000003)); 
	char * patched; 
	void * patch1_offset, *patch2_offset, *patch3_offset; 
	void * errorfix_offset;
	
	// define some offsets and variables depending on the region:
	switch (region) {
		case 'P': 
			patched = (char*)0x80276054; 
			patch1_offset = (void*)0x800ee3a0;
			patch2_offset = (void*)0x801d4efc; 
			patch3_offset = (void*)0x801A72E0; 
			errorfix_offset = (void*)0x80658ce4;
			break; 
		case 'E':
			patched = (char*)0x80271d14; 
			patch1_offset = (void*)0x800ee300;
			patch2_offset = (void*)0x801d4e5c; 
			patch3_offset = (void*)0x801A7240; 
			errorfix_offset = (void*)0x8065485c;
			break; 
		case 'J': 
			patched = (char*)0x802759f4;
			patch1_offset = (void*)0x800ee2c0;
			patch2_offset = (void*)0x801d4e1c; 
			patch3_offset = (void*)0x801A7200; 
			errorfix_offset = (void*)0x80658360;
			break; 
		case 'K': 
			patched = (char*)0x80263E34;
			patch1_offset = (void*)0x800ee418; 
			patch2_offset = (void*)0x801d5258;
			patch3_offset = (void*)0x801A763c;
			errorfix_offset = (void*)0x80646ffc;
			break;
		default: 
			return -1; 
	}
	
	if (*patched != '*') return -2; 	// ISO already patched
	
	// This RAM address is set (no asterisk) by all officially
	// updated patchers, so if it is modified, the image is already 
	// patched with a new patcher and we don't need to patch anything.
	
	// For statistics and easier debugging in case of problems, Wiimmfi
	// wants to know what patcher a game has been patched with, thus, 
	// let the game know the exact USB-Loader version. Max length 42 
	// chars, padded with whitespace, without null terminator
	char * fmt = "%s v%-50s";
	char patcher[100] = {0}; 
	snprintf((char *)&patcher, 99, fmt, APP_NAME, APP_VERSION); 	
	strncpy(patched, (char *)&patcher, 42); 
	
	// Do the plain old patching with the string search
	WFCPatch((void*)0x80004000, 0x385200, PRIVSERV_WIIMMFI);
	
	// Replace some URLs for Wiimmfi's new update system
	char newURL1[] = "http://ca.nas.wiimmfi.de/ca";
	char newURL2[] = "http://naswii.wiimmfi.de/ac";
	char newURL3P[] = "https://main.nas.wiimmfi.de/pp";
	char newURL3E[] = "https://main.nas.wiimmfi.de/pe";
	char newURL3J[] = "https://main.nas.wiimmfi.de/pj";
	char newURL3K[] = "https://main.nas.wiimmfi.de/pk";
	
	
	// Write the URLs to the proper place and do some other patching.
	switch (region) {
		case 'P': 
			memcpy((void*)0x8027A400, newURL1, sizeof(newURL1));
			memcpy((void*)0x8027A400 + 0x28, newURL2, sizeof(newURL2));
			memcpy((void*)0x8027A400 + 0x4C, newURL3P, sizeof(newURL3P));
			*(u32*)0x802a146c = 0x733a2f2f; 
			*(u32*)0x800ecaac = 0x3bc00000; 
			break; 
		case 'E': 
			memcpy((void*)0x802760C0, newURL1, sizeof(newURL1));
			memcpy((void*)0x802760C0 + 0x28, newURL2, sizeof(newURL2));
			memcpy((void*)0x802760C0 + 0x4C, newURL3E, sizeof(newURL3E));
			*(u32*)0x8029D12C = 0x733a2f2f; 
			*(u32*)0x800ECA0C = 0x3bc00000; 
			break; 
		case 'J':
			memcpy((void*)0x80279DA0, newURL1, sizeof(newURL1));
			memcpy((void*)0x80279DA0 + 0x28, newURL2, sizeof(newURL2));
			memcpy((void*)0x80279DA0 + 0x4C, newURL3J, sizeof(newURL3J));
			*(u32*)0x802A0E0C = 0x733a2f2f; 
			*(u32*)0x800EC9CC = 0x3bc00000; 
			break; 
		case 'K':
			memcpy((void*)0x802682B0, newURL1, sizeof(newURL1));
			memcpy((void*)0x802682B0 + 0x28, newURL2, sizeof(newURL2));
			memcpy((void*)0x802682B0 + 0x4C, newURL3K, sizeof(newURL3K));
			*(u32*)0x8028F474 = 0x733a2f2f; 
			*(u32*)0x800ECB24 = 0x3bc00000; 
			break; 
	}

	// Make some space on heap (0x500) for our custom code. 
	u32 old_heap_ptr = *(u32*)0x80003110; 
	*((u32*)0x80003110) = (old_heap_ptr - 0x500); 
	u32 heap_space = old_heap_ptr-0x500; 
	memset((void*)old_heap_ptr-0x500, 0xed, 0x500); 
	
	// Binary blobs with Wiimmfi patches. Do not modify. 
	// Provided by Leseratte on 2018-12-14.		
	
	int binary[] = { 0x37C849A2, 0x8BC32FA4, 0xC9A34B71, 0x1BCB49A2, 
					 0x2F119304, 0x5F402684, 0x3E4FDA29, 0x50849A21, 
					 0xB88B3452, 0x627FC9C1, 0xDC24D119, 0x5844350F, 
					 0xD893444F, 0x19A588DC, 0x16C91184, 0x0C3E237C, 
					 0x75906CED, 0x6E68A55E, 0x58791842, 0x072237E9, 
					 0xAB24906F, 0x0A8BDF21, 0x4D11BE42, 0x1AAEDDC8, 
					 0x1C42F908, 0x280CF2B2, 0x453A1BA4, 0x9A56C869, 
					 0x786F108E, 0xE8DF05D2, 0x6DB641EB, 0x6DFC84BB, 
					 0x7E980914, 0x0D7FB324, 0x23442185, 0xA7744966, 
					 0x53901359, 0xBF2103CC, 0xC24A4EB7, 0x32049A02, 
					 0xC1683466, 0xCA93689D, 0xD8245106, 0xA84987CF, 
					 0xEC9B47C9, 0x6FA688FE, 0x0A4D11A6, 0x8B653C7B, 
					 0x09D27E30, 0x5B936208, 0x5DD336DE, 0xCD092487, 
					 0xEF2C6D36, 0x1E09DF2D, 0x75B1BE47, 0xE68A7F22, 
					 0xB0E5F90D, 0xEC49F216, 0xAD1DCC24, 0xE2B5C841, 
					 0x066F6F63, 0xF4D90926, 0x299F42CD, 0xA3F125D6, 
					 0x077B093C, 0xB5721268, 0x1BE424D1, 0xEBC30BF0, 
					 0x77867BED, 0x4F0C9BCA, 0x3E195930, 0xDC32DE2C, 
					 0x1865D189, 0x70C67E7A, 0x71FA7329, 0x532233D3, 
					 0x06D2E87B, 0x6CBEBA7F, 0x99F08532, 0x52FA601C, 
					 0x05F4B82C, 0x4B64839C, 0xB5C65009, 0x1B8396E3, 
					 0x0A8B2DAF, 0x0DB85BE6, 0x12F1B71D, 0x186F6E4D, 
					 0x2870DC2E, 0x5960B8E6, 0x8F4D71BD, 0x0614E3C3, 
					 0x05E8C725, 0x365D8E3D, 0x74351CDE, 0xE1AB3930, 
					 0xFEDA721B, 0xE53AE4E9, 0xC3B4C9A6, 0xBAE59346, 
					 0x6D45269D, 0x634E4D1A, 0x2FD99A30, 0x26393449, 
					 0xE49768D1, 0x81E1D1A1, 0xFCE1A34A, 0x7EB44697, 
					 0xEB2F8D2D, 0xCECFE5AF, 0x81BD34B6, 0xB1F1696E, 
					 0x5E6ED2B2, 0xA473A4A0, 0x41664B70, 0xBF40968A, 
					 0x662F2CCB, 0xC5DF5B8C, 0xB632B772, 0x74EB6F39, 
					 0xE017DC71, 0xFDA3B890, 0xE3C9713D, 0xCE53E397, 
					 0xA12BC743, 0x5AD98EA5, 0xBC721C9F, 0x4568395A, 
					 0x925E72B4, 0x2D7DE4D7, 0x6777C9C7, 0xD6619396, 
					 0xA502268A, 0x77884D75, 0xF79E9AF0, 0xE6FC3461, 
					 0xF07468A5, 0xF866D11D, 0xF90CA342, 0xCF9546FF, 
					 0x87A48D81, 0x06881A51, 0x309C34D1, 0x79B669CE, 
					 0xFAADD2D7, 0xC8D7A5D1, 0x89214BE5, 0x1B8396EF, 
					 0x0A8B2DE9, 0x0D985B06, 0x12F1B711, 0x186F6E57, 
					 0x2850DC0E, 0x5960B8EA, 0x8F4D71AC, 0x0614E3E3, 
					 0x05E8C729, 0x365D8E39, 0x74351CFE, 0x518E3943, 
					 0x4A397268, 0x9D58E4B8, 0xD394C9A2, 0x0E069344, 
					 0xB522268B, 0x636E4D77, 0x2FF99A37, 0xF6DC346D, 
					 0xE49268B4, 0x2001D1A0, 0x4929A365, 0x7B764691, 
					 0xFFC68D49, 0x16A81A53, 0x247A34D2, 0xA1D16967, 
					 0x4B6DD2D5, 0xDDF4A5B7, 0x454A4B70, 0x0FAE96E2, 
					 0x0A8A2DC7, 0x0D98A47A, 0x06DCB71D, 0x0CCC6E38, 
					 0x55F25CFB, 0xB08C1E88, 0xDF4259C9, 0x0714E387, 
					 0xB00D47AF, 0x7B722975, 0x48BE349A, 0x29CC393C, 
					 0xEA797228, 0x98986471, 0x3778E1A3, 0xD7626D06, 
					 0x1567268D, 0x668ECD00, 0xD614F5C8, 0x133037CF, 
					 0x92F26CF2, 0x00000000, 0x00000000, 0x00000000, 
					 0x00000000, 0x00000000, 0x00000000, 0x00000000};	

	// Fix for error 51420:
	int patchCodeFix51420[] = { 
		0x4800000d, 0x00000000,
		0x00000000, 0x7cc803a6,
		0x80860000, 0x7c041800,
		0x4182004c, 0x80a60004,
		0x38a50001, 0x2c050003,
		0x4182003c, 0x90a60004,
		0x90660000, 0x38610010,
		0x3ca08066, 0x38a58418,
		0x3c808066, 0x38848498,
		0x90a10010, 0x90810014,
		0x3ce08066, 0x38e78ce4,
		0x38e7fef0, 0x7ce903a6,
		0x4e800420, 0x3c80801d,
		0x388415f4, 0x7c8803a6,
		0x4e800021, 0x00000000
		};

	
	// Prepare patching process ....
	int i = 3; 
	int idx = 0; 
	for (; i < 202; i++) {
		if (i == 67 || i == 82) idx++; 
		binary[i] = binary[i] ^ binary[idx]; 
		binary[idx] = ((binary[idx] << 1) | ((binary[idx] >> (32 - 1)) & ~(-1 << 1))); 
	}
	

	// Binary blob needs some changes for regions other than PAL ...		 
	switch (region) {
		case 'E': 
			binary[29] = binary[67]; 	
			binary[37] = binary[68]; 	
			binary[43] = binary[69];	
			binary[185] = 0x61295C74; 	
			binary[189] = 0x61295D40;	
			binary[198] = 0x61086F5C; 	

			patchCodeFix51420[14] = 0x3ca08065;
			patchCodeFix51420[15] = 0x38a53f90;
			patchCodeFix51420[16] = 0x3c808065;
			patchCodeFix51420[17] = 0x38844010;
			patchCodeFix51420[20] = 0x3ce08065;
			patchCodeFix51420[21] = 0x38e7485c;
			patchCodeFix51420[26] = 0x38841554;

			break; 
		case 'J': 
			binary[29] = binary[70]; 
			binary[37] = binary[71]; 
			binary[43] = binary[72];
			binary[185] = 0x612997CC; 	
			binary[189] = 0x61299898;	
			binary[198] = 0x61086F1C; 

			patchCodeFix51420[14] = 0x3ca08065;
			patchCodeFix51420[15] = 0x38a57a84;
			patchCodeFix51420[16] = 0x3c808065;
			patchCodeFix51420[17] = 0x38847b04;
			patchCodeFix51420[20] = 0x3ce08065;
			patchCodeFix51420[21] = 0x38e78350;
			patchCodeFix51420[26] = 0x38841514;
	
			break; 
		case 'K': 
			binary[6] = binary[73]; 	
			binary[9] = binary[74]; 
			binary[11] = binary[75]; 
			binary[23] = binary[76]; 
			binary[29] = binary[77]; 
			binary[33] = binary[78]; 
			binary[37] = binary[79]; 
			binary[43] = binary[80]; 
			binary[63] = binary[81]; 
			binary[184] = 0x3D208088;	
			binary[185] = 0x61298AA4; 	
			binary[188] = 0x3D208088;	
			binary[189] = 0x61298B58;	
			binary[198] = 0x61087358; 

			patchCodeFix51420[14] = 0x3ca08064;
			patchCodeFix51420[15] = 0x38a56730;
			patchCodeFix51420[16] = 0x3c808064;
			patchCodeFix51420[17] = 0x388467b0;
			patchCodeFix51420[20] = 0x3ce08064;
			patchCodeFix51420[21] = 0x38e76ffc;
			patchCodeFix51420[26] = 0x38841950;
	
			break; 
	}

	// Installing all the patches.
	
	memcpy((void*)heap_space, (void*)binary, 820); 	
	u32 code_offset_1 = heap_space + 12; 	
	u32 code_offset_2 = heap_space + 88; 	
	u32 code_offset_3 = heap_space + 92;	
	u32 code_offset_4 = heap_space + 264; 	
	u32 code_offset_5 = heap_space + 328;	
	
	
	*((u32*)patch1_offset) = 0x48000000 + (((u32)(code_offset_1) - ((u32)(patch1_offset))) & 0x3ffffff); 
	*((u32*)code_offset_2) = 0x48000000 + (((u32)(patch1_offset + 4) - ((u32)(code_offset_2))) & 0x3ffffff); 
	*((u32*)patch2_offset) = 0x48000000 + (((u32)(code_offset_3) - ((u32)(patch2_offset))) & 0x3ffffff); 
	*((u32*)code_offset_4) = 0x48000000 + (((u32)(patch2_offset + 4) - ((u32)(code_offset_4))) & 0x3ffffff); 
	*((u32*)patch3_offset) = 0x48000000 + (((u32)(code_offset_5) - ((u32)(patch3_offset))) & 0x3ffffff); 

	// Add the 51420 fix: 
	memcpy((void*)heap_space + 0x400, (void*)patchCodeFix51420, 0x78);
	*((u32*)errorfix_offset) = 0x48000000 + (((u32)(heap_space + 0x400) - ((u32)(errorfix_offset))) & 0x3ffffff);
	*((u32*)heap_space + 0x400 + 0x74) = 0x48000000 + (((u32)(errorfix_offset + 4) - ((u32)(heap_space + 0x400 + 0x74))) & 0x3ffffff);

	
	// Patches successfully installed
	// returns 0 when all patching is done and game is ready to be booted. 
	return 0; 	
}
