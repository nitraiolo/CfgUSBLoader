/* Load the open source "return to channel" stub 
 *
 * Functions for manipulating the HBC stub by giantpune from PostLoader
 * Stub from USBLoader GX and FIX94 work
 * Adapted to Cfg USB Loader by NiTRo
 */

#ifndef _STUB_
#define _STUB_

#include "titles.h"

void stub_load ( void );
void stub_unload ( void );
s32 stub_set_tid ( u64 new_tid );
s32 stub_set_splitted_tid ( u32 type, const char* new_lower );
u8 stub_available ( void );
u64 stub_get_tid ( void );

#endif

