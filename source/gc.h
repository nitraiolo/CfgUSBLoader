#ifdef __cplusplus
extern "C"
{
#endif

#ifndef GC_H_
#define GC_H_

#include "disc.h"

#define GC_GAME_ON_DRIVE 0x444D4C00

typedef struct DML_CFG
{
	u32 Magicbytes;			//0xD1050CF6
	u32 Version;			//0x00000002
	u32 VideoMode;
	u32 Config;
	char GamePath[255];
	char CheatPath[255];
} DML_CFG;

enum dmlconfig
{
	DML_CFG_CHEATS		= (1<<0),
	DML_CFG_DEBUGGER	= (1<<1),
	DML_CFG_DEBUGWAIT	= (1<<2),
	DML_CFG_NMM			= (1<<3),
	DML_CFG_NMM_DEBUG	= (1<<4),
	DML_CFG_GAME_PATH	= (1<<5),
	DML_CFG_CHEAT_PATH	= (1<<6),
	DML_CFG_ACTIVITY_LED= (1<<7),
	DML_CFG_PADHOOK		= (1<<8),
	DML_CFG_FORCE_WIDE	= (1<<9),
	DML_CFG_BOOT_DISC	= (1<<10),
	DML_CFG_BOOT_DISC2	= (1<<11),
	DML_CFG_NODISC		= (1<<12),
	DML_CFG_SCREENSHOT	= (1<<13),
};

enum dmlvideomode
{
	DML_VID_DML_AUTO	= (0<<16),
	DML_VID_FORCE		= (1<<16),
	DML_VID_NONE		= (2<<16),

	DML_VID_FORCE_PAL50	= (1<<0),
	DML_VID_FORCE_PAL60	= (1<<1),
	DML_VID_FORCE_NTSC	= (1<<2),
	DML_VID_FORCE_PROG	= (1<<3),
	DML_VID_PROG_PATCH	= (1<<4),
};

// Devolution
typedef struct global_config
{
	u32 signature;			//0x3EF9DB23
	u16 version;			//0x00000100
	u16 device_signature;
	u32 memcard_cluster;
	u32 disc1_cluster;
	u32 disc2_cluster;
	u32 options;
} gconfig;

#define DEVO_CFG_WIFILOG    (1<<0) // added in Devo r100, config version 0x0110
#define DEVO_CFG_WIDE       (1<<1) // added in Devo r142 
#define DEVO_CFG_NOLED      (1<<2)
#define DEVO_CFG_FZERO_AX	(1<<3) // added in Devo r196, config version x0111
#define DEVO_CFG_TIMER_FIX  (1<<4)
#define DEVO_CFG_D_BUTTONS  (1<<5) // added in Devo r200, config version 0x0112
   
void DEVO_SetOptions(const char* path,u8 NMM);

void GC_SetVideoMode(u8 videomode, bool devo);
void GC_SetLanguage(u8 lang);
s32 DML_RemoveGame(struct discHdr header);
int DML_GameIsInstalled(char *folder);
void DML_New_SetOptions(char *GamePath, char *CheatPath, char *NewCheatPath, bool cheats, bool debugger, u8 NMM, u8 nodisc, u8 DMLvideoMode, u8 LED, u8 W_SCREEN, u8 PHOOK, u8 V_PATCH, u8 screenshot);
void DML_Old_SetOptions(char *GamePath, char *CheatPath, char *NewCheatPath, bool cheats);
void DML_New_SetBootDiscOption(char *CheatPath, char *NewCheatPath, bool cheats, u8 NMM, u8 LED, u8 DMLvideoMode);
u64 getDMLDisk1Size(struct discHdr *header);
u64 getDMLGameSize(struct discHdr *header);
s32 copy_DML_Game_to_SD(struct discHdr *header);
s32 delete_Old_Copied_DML_Game();
char *get_DM_Game_Folder(char *path);
bool is_gc_game_on_bootable_drive(struct discHdr *header);
void Nintendont_set_options(struct discHdr *header, char *CheatPath, char *NewCheatPath);
#endif //GC_H_

#ifdef __cplusplus
}
#endif