#ifndef _SDHC_H_
#define _SDHC_H_

/* Constants */
#define SDHC_SECTOR_SIZE	0x200

/* Disc interfaces */
extern const DISC_INTERFACE my_io_sdhc;
extern const DISC_INTERFACE my_io_sdhc_ro;

/* Prototypes */
bool SDHC_Init(void);
bool SDHC_Close(void);
bool SDHC_ReadSectors(u32, u32, void *);
bool SDHC_WriteSectors(u32, u32, void *);
s32  SD_DiskSpace(f32 *used, f32 *free);
extern int sdhc_mode_sd;
extern int sdhc_inited;

#endif
