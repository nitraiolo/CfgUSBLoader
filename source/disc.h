#ifndef _DISC_H_
#define _DISC_H_

#define GC_GAME_SIZE 0x57058000
#define WII_MAGIC	0x5D1C9EA3
#define GC_MAGIC	0xC2339F3D
#define CHANNEL_MAGIC 0x4348414E

/* Disc header structure */
struct discHdr
{
	/* Game ID */
	u8 id[6];

	/* Disk number */
	u8 disc;
	
	/* Game version */
	u8 version;

	/* Audio streaming */
	u8 streaming;
	u8 bufsize;

	/* Padding */
	u8 unused1[14];

	/* Magic word */
	u32 magic;

	/* Padding */
	u8 unused2[4];

	/* Game title */
	char title[64];

	/* Encryption/Hashing */
	u8 encryption;
	u8 h3_verify;

	/* Padding */
	u8 unused3[30];
	
	char path[255];
} ATTRIBUTE_PACKED;

struct gc_discHdr
{
	/* Game ID */
	u8 id[6];

	/* Disk number */
	u8 disc;
	
	/* Game version */
	u8 version;

	/* Audio streaming */
	u8 streaming;
	u8 bufsize;

	/* Padding */
	u8 unused1[18];

	/* Magic word */
	u32 magic;

	/* Game title */
	char title[64];
		
	/* Padding */
	u8 unused2[64];
};

struct dml_Game
{
	/* Game ID */
	char id[6];

	/* Game version */
	u8 disc;

	/* Game title */
	char title[64];

	char folder[255];
};

/* Prototypes */
s32  Disc_Init(void);
s32  Disc_Open(void);
s32  Disc_Wait(void);
s32  Disc_SetWBFS(u32, u8 *);
s32  Disc_ReadHeader(void *);
s32  Disc_ReadGCHeader(void *);
s32  Disc_IsWii(void);
s32  Disc_IsGC(void);
s32  Disc_BootPartition(u64, bool dvd);
s32  Disc_WiiBoot(bool dvd);
s32  Disc_DumpGCGame(bool sd);

u32 appentrypoint;
#endif

