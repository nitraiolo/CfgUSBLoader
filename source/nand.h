#ifndef _NAND_H_
#define _NAND_H_

/* 'NAND Device' structure */
typedef struct {
	/* Device name */
	const char *name;

	/* Mode value */
	u32 mode;

	/* Un/mount command */
	u32 mountCmd;
	u32 umountCmd;
} nandDevice; 


#define REAL_NAND	0
#define EMU_SD		1
#define EMU_USB		2

typedef struct _dirent
{
	char name[ISFS_MAXPATH + 1];
	int type;
	u32 ownerID;
	u16 groupID;
	u8 attributes;
	u8 ownerperm;
	u8 groupperm;
	u8 otherperm;
} dirent_t;

typedef struct _dir
{
	char name[ISFS_MAXPATH + 1];
} dir_t;

typedef struct _list
{
	char name[ISFS_MAXPATH + 1];

} list_t;

/* Prototypes */
s32 Nand_Mount(nandDevice *);
s32 Nand_Unmount(nandDevice *);
s32 Nand_Enable(nandDevice *);
s32 Nand_Disable(void);
s32 Enable_Emu(int selection);
s32 Disable_Emu();

void Set_Partition(int);
void Set_Path(const char*);
void Set_FullMode(int);
const char* Get_Path(void);
bool dumpfolder(char source[1024], char destination[1024]);
s32 dumpfile(char source[1024], char destination[1024]);
int isdir(char *path);

s32 get_nand_dir(char *path, dirent_t **ent, s32 *cnt);
s32 get_dir_count(char *path, u32 *cnt);
u8 get_nand_device();

#endif
