#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>

#include "dolmenu.h"
#include "cfg.h"
#include "util.h"

wdm_entry_t *dolmenubuffer = NULL;
u32 dolparameter = 1;

static char *p;
static char *pend;

char *getnextstring()
{
	char *temp = NULL;
	char *pos;
	char *posCR;
	
	while (true)
	{
		if (p >= pend)
		{
			return NULL;
		}
		pos = strchr(p, '\n');
		if (pos == NULL)
		{
			temp = p;
			p = pend;
			return temp;
		}
		if (pos >= pend)
		{
			return NULL;
		}
		pos[0] = 0;
		posCR = strchr(p, '\r');
		if (posCR != NULL && posCR < pos)
		{
			posCR[0] = 0;
		}

		temp = p;
		p = (char *)((u32)pos+1);
		
		if (temp[0] != '#') break;		// Skip comments which start with #
	}
	return temp;
}

u32 getnextnumber()
{
	char *temp = getnextstring();
	if (temp == NULL || temp[0] == 0 || temp[0] == '\n' || temp[0] == '\r' || strlen(temp) == 0)
	{
		return 0;
	} else
	if (strlen(temp) > 2 && strncmp(temp, "0x", 2) == 0)
	{
		return strtol((char *)((u32)temp+2), NULL, 16);
	} else
	{
		return strtol(temp, NULL, 10);
	}
}

void copynextstring(char *output)
{
	char *temp = getnextstring();
	if (temp == NULL || temp[0] == 0 || temp[0] == '\n' || temp[0] == '\r' || strlen(temp) == 0)
	{
		strcpy(output, "?");	
	} else
	{
		strcpy(output, temp);	
	}
}

void parse_dolmenubuffer(u32 index, u32 count, u32 parent)
{
	u32 i;
	for (i = 0;i < count;i++)
	{
		dolmenubuffer[index + i].count = getnextnumber();
		memset(dolmenubuffer[index + i].name, 0, 64);
		copynextstring(dolmenubuffer[index + i].name);
		memset(dolmenubuffer[index + i].dolname, 0, 32);
		copynextstring(dolmenubuffer[index + i].dolname);
		
		dolmenubuffer[index + i].parameter = getnextnumber();
		dolmenubuffer[index + i].parent = parent;
		if (dolmenubuffer[index + i].count != 0)
		{
			parse_dolmenubuffer(index + i + 1, dolmenubuffer[index + i].count, index + i);	
			i+=dolmenubuffer[index + i].count;
		}
	}	
}

s32 createdolmenubuffer(u32 count)
{
	u32 i;
	dolmenubuffer = memalign(32, sizeof(wdm_entry_t) * count);
	if (dolmenubuffer == NULL)
	{
		return -1;
		//error
	}
	
	for (i=0;i<count;i++)
	{
		dolmenubuffer[i].count = 0;
		dolmenubuffer[i].parameter = 0;
		dolmenubuffer[i].parent = 0;
		memset(dolmenubuffer[i].name, 0, 64);
		memset(dolmenubuffer[i].dolname, 0, 32);
	}
	dolmenubuffer[0].parent = -1;
	return 1;
}

s32 load_dolmenu(char *discid)
{
	static char buf[128];
	FILE *fp = NULL;
	int filesize = 0;

	dbg_printf("Loading menu file...\n");
	
	SAFE_FREE(dolmenubuffer);

	snprintf(buf, 128, "%s/%.6s.wdm", USBLOADER_PATH, discid);
	fp = fopen(buf, "rb");
	if (!fp)
	{
		snprintf(buf, 128, "%s/%.4s.wdm", USBLOADER_PATH, discid);
		fp = fopen(buf, "rb");
	}
	if (!fp)
	{
		snprintf(buf, 128, "%s/%.3s.wdm", USBLOADER_PATH, discid);
		fp = fopen(buf, "rb");
	}

	if (!fp)
	{
		dbg_printf("No menu file found\n");
		//sleep(2);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	u8 *buffer = malloc(filesize+1);
	if (buffer == NULL)
	{
		printf("Ouf of memory\n");
		sleep(2);
		return -1;
	}

	fread(buffer, 1, filesize, fp);
	fclose(fp);

	buffer[filesize] = 0; // zero terminate

	p = (char *)buffer;
	pend = p + filesize;

	u32 count = getnextnumber();

	dolmenubuffer = malloc(sizeof(wdm_entry_t) * (count + 1));
	if (dolmenubuffer == NULL)
	{
		printf("Ouf of memory\n");
		sleep(2);
		return -1;
	}

	dolmenubuffer[0].count = count;
	dolmenubuffer[0].parent = -1;
	dolmenubuffer[0].parameter = 1;
	memset(dolmenubuffer[0].name, 0, 64);
	memset(dolmenubuffer[0].dolname, 0, 32);

	parse_dolmenubuffer(1 , count, 0);
	
	free(buffer);

	return 1;
}
