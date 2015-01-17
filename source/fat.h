
// Modified by oggzee

#ifndef _FAT_H_
#define _FAT_H_

#define MOUNT_MAX 5
// sd: usb: ntfs: game:

#define SDHC_MOUNT	"sd"
#define USB_MOUNT	"usb"
#define GAME_MOUNT	"game"
#define NTFS_MOUNT	"ntfs"
#define EXT_MOUNT	"ext"

#define SDHC_DRIVE SDHC_MOUNT":"
#define USB_DRIVE  USB_MOUNT":"
#define GAME_DRIVE GAME_MOUNT":"
#define NTFS_DRIVE NTFS_MOUNT":"
#define EXT_DRIVE  EXT_MOUNT":"

/* Prototypes */
s32 Fat_ReadFile(const char *, void **);
/*
s32 Fat_MountSDHC(void);
s32 Fat_UnmountSDHC(void);
s32 Fat_MountUSB(void);
s32 Fat_UnmountUSB(void);
s32 Fat_MountWBFS(u32 sector);
s32 Fat_UnmountWBFS(void);
s32 MountNTFS(u32 sector);
s32 UnmountNTFS(void);

void Fat_UnmountAll();
void Fat_print_sd_mode();

extern int   fat_sd_mount;
extern sec_t fat_sd_sec;
extern int   fat_usb_mount;
extern sec_t fat_usb_sec;
extern int   fat_GAME_MOUNT;
extern sec_t fat_wbfs_sec;
extern int   fs_ntfs_mount;
extern sec_t fs_ntfs_sec;
*/

typedef struct MountPoint
{
	char  name[16]; // "usb:"
	int   device;
	sec_t sector;
	int   fstype;
	int   flags;
} MountPoint;

typedef struct MountTable
{
	int num;
	MountPoint point[MOUNT_MAX];
} MountTable;

void mount_drive2name(char *drive, char *name);
void mount_name2drive(char *name, char *drive);

int mount_add(char *name, int device, sec_t sector, int fstype, int flags);
MountPoint* mount_find(char *name);
MountPoint* mount_find_part(int device, sec_t sector);
int mount_del(char *name);

int MountFS(char *name, int device, sec_t sector, int fstype, int verbose);
int UnmountFS(char *name);
int MountAll(MountTable *mt);
int UnmountAll(MountTable *save_mtab);
int MountSDHC();
int MountUSB();
void RemountNTFS();
void MountPrint_str(char *str, int size);
void MountPrint();

s32 FAT_DiskSpace(f32 *used, f32 *free);
#endif

