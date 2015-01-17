#include <gccore.h>

typedef struct
{
	u32 count;
	char name[64];
	char dolname[32];
	u32 parameter;
	u32 parent;
} wdm_entry_t;

extern wdm_entry_t *dolmenubuffer;
extern u32 dolparameter; // 1

s32 createdolmenubuffer(u32 count);
s32 load_dolmenu(char *discid);
