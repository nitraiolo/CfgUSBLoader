/****************************************************************************
 * Copyright (C) 2011
 * by Dimok
 *
 * Modified for CFG USB Loader by R2-D2199
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 ***************************************************************************/
#include <gctypes.h>
#include <stdio.h>

#include "savegame.h"
#include "wbfs.h"
#include "fileOps.h"
#include "cfg.h"
#include "libwbfs/libwbfs.h"
#include "sys.h"
#include "nand.h"
#include "gettext.h"
#include "wpad.h"
#include "gc.h"

void CreateTitleTMD(const char *path, struct discHdr *hdr)
{
	wbfs_disc_t *disc = WBFS_OpenDisc(hdr->id);
	if (!disc)
		return;

	wiidisc_t *wdisc = wd_open_disc((int(*)(void *, u32, u32, void *)) wbfs_disc_read, disc);
	if (!wdisc)
	{
		WBFS_CloseDisc(disc);
		return;
	}

	tmd game_tmd;
	memset(&game_tmd, 0, sizeof(game_tmd));
	int tmd_size = wbfs_extract_tmd(disc, &game_tmd);
	WBFS_CloseDisc(disc);

	if(!tmd_size)
		return;

	FILE *f = fopen(path, "wb");
	if(f)
	{
		fwrite((u8*)&game_tmd, 1, tmd_size, f);
		fclose(f);
		dbg_printf("Written Game Title TDM to: %s\n", path);
	}

}

static void CreateNandPath(char *path)
{
	if(fsop_DirExist(path))
		return;

	dbg_printf("Creating Nand Path: %s\n", path);
	fsop_CreateFolderTree(path);
}

void CreateSavePath(struct discHdr *hdr)
{
	char nandPath[512];
	char buffer[512];

	snprintf(nandPath, sizeof(nandPath), "%s/import", CFG.nand_emu_path);
	CreateNandPath(nandPath);

	snprintf(nandPath, sizeof(nandPath), "%s/meta", CFG.nand_emu_path);
	CreateNandPath(nandPath);

	snprintf(nandPath, sizeof(nandPath), "%s/shared1", CFG.nand_emu_path);
	CreateNandPath(nandPath);

	snprintf(nandPath, sizeof(nandPath), "%s/shared2", CFG.nand_emu_path);
	CreateNandPath(nandPath);

	snprintf(nandPath, sizeof(nandPath), "%s/sys", CFG.nand_emu_path);
	CreateNandPath(nandPath);

	snprintf(nandPath, sizeof(nandPath), "%s/ticket", CFG.nand_emu_path);
	CreateNandPath(nandPath);

	snprintf(nandPath, sizeof(nandPath), "%s/tmp", CFG.nand_emu_path);
	CreateNandPath(nandPath);

	const char *titlePath = "title/00010000";

	if(	memcmp(hdr->id, "RGWX41", 6) == 0 || memcmp(hdr->id, "RGWP41", 6) == 0 ||
		memcmp(hdr->id, "RGWJ41", 6) == 0 || memcmp(hdr->id, "RGWE41", 6) == 0)
	{
		titlePath = "title/00010004";
	}

	snprintf(nandPath, sizeof(nandPath), "%s/%s/%02x%02x%02x%02x/data", CFG.nand_emu_path, titlePath, hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
	CreateNandPath(nandPath);

	snprintf(nandPath, sizeof(nandPath), "%s/%s/%02x%02x%02x%02x/content", CFG.nand_emu_path, titlePath, hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
	CreateNandPath(nandPath);

	strcat(nandPath, "/title.tmd");
	if(!fsop_FileExist(nandPath))
		CreateTitleTMD(nandPath, hdr);

	snprintf(nandPath, sizeof(nandPath), "%s/shared2/sys/SYSCONF", CFG.nand_emu_path);
	if(!fsop_FileExist(nandPath)) {
		snprintf(buffer, sizeof(buffer), "%s/shared2/sys", CFG.nand_emu_path);
		fsop_CreateFolderTree(buffer);
		dumpfile("/shared2/sys/SYSCONF", nandPath);
	}
		
	snprintf(nandPath, sizeof(nandPath), "%s/title/00000001/00000002/data/setting.txt", CFG.nand_emu_path);
	if(!fsop_FileExist(nandPath)) {
		snprintf(buffer, sizeof(buffer), "%s/title/00000001/00000002/data", CFG.nand_emu_path);
		fsop_CreateFolderTree(buffer);
		dumpfile("/title/00000001/00000002/data/setting.txt", nandPath);
	}
}

void Menu_dump_savegame(struct discHdr *hdr)
{
	if (hdr->magic == GC_GAME_ON_DRIVE) {
		printf_x(gt("Can not dump GC savegames.\n")); 
		printf_x(gt("Press any button.\n")); 
		Wpad_WaitButtons();
		return;
	}
	
	char nandPath[512];
	char emuNandPath[512];
	char buffer[512];
	
	const char *titlePath = "title/00010000";
	
	snprintf(nandPath, sizeof(nandPath), "/%s/%02x%02x%02x%02x", titlePath, hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
	snprintf(emuNandPath, sizeof(emuNandPath), "%s/", CFG.nand_emu_path);
	
	if (!isdir(nandPath)) {
		titlePath = "title/00010004";
		snprintf(nandPath, sizeof(nandPath), "/%s/%02x%02x%02x%02x", titlePath, hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
		if (!isdir(nandPath)) {
			printf_x(gt("Savegame not found.\n")); 
			printf_x(gt("Press any button.\n")); 
			Wpad_WaitButtons();
			return;
		}
	}
	dumpfolder(nandPath, emuNandPath);
	
	strcat(emuNandPath, "shared2/sys/SYSCONF");
	if(!fsop_FileExist(emuNandPath)) {
		snprintf(buffer, sizeof(buffer), "%s/shared2/sys", CFG.nand_emu_path);
		fsop_CreateFolderTree(buffer);
		dumpfile("/shared2/sys/SYSCONF", emuNandPath);
	}
	
	snprintf(emuNandPath, sizeof(emuNandPath), "%s/title/00000001/00000002/data/setting.txt", CFG.nand_emu_path);
	if(!fsop_FileExist(emuNandPath)) {
		snprintf(buffer, sizeof(buffer), "%s/title/00000001/00000002/data", CFG.nand_emu_path);
		fsop_CreateFolderTree(buffer);
		dumpfile("/title/00000001/00000002/data/setting.txt", emuNandPath);
	}
	sleep(5);
}