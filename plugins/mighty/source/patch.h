/*******************************************************************************
 * tools.h
 *
 * Copyright (c) 2009 The Lemon Man
 * Copyright (c) 2009 Nicksasa
 * Copyright (c) 2009 WiiPower
 *
 * Distributed under the terms of the GNU General Public License (v2)
 * See http://www.gnu.org/licenses/gpl-2.0.txt for more info.
 *
 * Description:
 * -----------
 *
 ******************************************************************************/

#include <gccore.h>

extern GXRModeObj TVNtsc480Int; // declared somewhere inside libogc, but missing in the correct .h file
extern GXRModeObj TVMpal480Prog;

int videomode_interlaced(GXRModeObj* mode);
void search_video_modes(u8 *file, u32 size);
void patch_video_modes_to(GXRModeObj* vmode, u32 videopatchselect);
s32 parser(u8 *file, u32 size, u8 *value, u32 valuesize, u8 *patch, u32 patchsize, u32 offset);
s32 patch_language(u8 *file, u32 size, u32 languageoption);

