/* Load the open source "return to channel" stub 
 *
 * Functions for manipulating the HBC stub by giantpune from PostLoader
 * Stub from USBLoader GX and FIX94 work
 * Adapted to Cfg USB Loader by NiTRo
 */

#include <stdio.h>
#include <ogcsys.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <gccore.h>
#include <string.h>
#include <time.h>

#include "rcstub.h"
#include "debug.h"

extern const u8 stub_bin[];
extern const u32 stub_bin_size;

void stub_load ( void )
{
	dbg_printf("stub_load()\n");
	
	// Clear potential homebrew channel stub
	memset((void*)0x80001800, 0, 0x1800);
	// Copy our own stub into memory
	memcpy((void*)0x80001800, stub_bin, stub_bin_size);
	DCFlushRange((void*)0x80001800, 0x1800);
	
	return;	
}

void stub_unload ( void )
{
	dbg_printf("stub_unload()\n");

	memset((void*)0x80001800, 0, 0x1800);
	DCFlushRange((void*)0x80001800, 0x1800);
	
	return;
}

static char* stub_find_tid_addr ( void )
{
	u32 *stub_id = (u32*) 0x80001818;

	//HBC stub 1.0.6 and lower, and stub.bin
	if (stub_id[0] == 0x480004c1 && stub_id[1] == 0x480004f5)
		return (char *) 0x800024C6;
	//HBC stub changed @ version 1.0.7.  this file was last updated for HBC 1.0.8
	else if (stub_id[0] == 0x48000859 && stub_id[1] == 0x4800088d)
		return (char *) 0x8000286A;
	return NULL;
}

s32 stub_set_tid ( u64 new_tid )
{
	char *stub_tid_addr = stub_find_tid_addr();
	if (!stub_tid_addr) return -68;

	stub_tid_addr[0]  = TITLE_7( new_tid );
	stub_tid_addr[1]  = TITLE_6( new_tid );
	stub_tid_addr[8]  = TITLE_5( new_tid );
	stub_tid_addr[9]  = TITLE_4( new_tid );
	stub_tid_addr[4]  = TITLE_3( new_tid );
	stub_tid_addr[5]  = TITLE_2( new_tid );
	stub_tid_addr[12] = TITLE_1( new_tid );
	stub_tid_addr[13] = ((u8) (new_tid));

	DCFlushRange(stub_tid_addr, 0x10);

	return 0;
}

s32 stub_set_splitted_tid ( u32 type, const char* new_lower )
{
	const u32 lower = ((u32)new_lower[0] << 24) |
					  ((u32)new_lower[1] << 16) |
					  ((u32)new_lower[2] << 8) |
					  ((u32)new_lower[3]);

	return stub_set_tid(TITLE_ID( type, lower ));
}


u8 stub_available ( void )
{
	char * sig = (char *) 0x80001804;
	return (strncmp(sig, "STUBHAXX", 8) == 0);
}

u64 stub_get_tid ( void )
{
	if (!stub_available()) return 0;

	char ret[8];
	u64 retu = 0;

	char *stub = stub_find_tid_addr();
	if (!stub) return 0;

	ret[0] = stub[0];
	ret[1] = stub[1];
	ret[2] = stub[8];
	ret[3] = stub[9];
	ret[4] = stub[4];
	ret[5] = stub[5];
	ret[6] = stub[12];
	ret[7] = stub[13];

	memcpy(&retu, ret, 8);

	return retu;
}
