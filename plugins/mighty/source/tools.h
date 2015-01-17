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
typedef struct _dirent{
	char name[ISFS_MAXPATH + 1];
	int type;
} dirent_t;

#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <wiiuse/wpad.h>

#define TITLE_UPPER(x)		((u32)((x) >> 32))
#define TITLE_LOWER(x)		((u32)(x))
#define TITLE_ID(x,y)		(((u64)(x) << 32) | (y))

void *allocate_memory(u32 size);
s32 getdir(char *, dirent_t **, u32 *);
s32 read_file(char *filepath, u8 **buffer, u32 *filesize);
s32 identify(u64 titleid, u32 *ios);
//u32 Pad_WaitButtons(void);


