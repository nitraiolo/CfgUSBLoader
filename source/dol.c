// this code was contributed by shagkur of the devkitpro team, thx!

#include <stdio.h>
#include <string.h>

#include <gccore.h>
#include <ogcsys.h>

#include "dol.h"


static dolheader *dolfile;
static int i;
static int phase;

u32 load_dol_start(void *dolstart)
{
	if (dolstart)
	{
		dolfile = (dolheader *)dolstart;
        return dolfile->entry_point;
	} else
	{
		return 0;
	}
	
    memset((void *)dolfile->bss_start, 0, dolfile->bss_size);
    DCFlushRange((void *)dolfile->bss_start, dolfile->bss_size);
	
	phase = 0;
	i = 0;
}

bool load_dol_image(void **offset, u32 *pos, u32 *len) 
{
	if (phase == 0)
	{
		if (i == 7)
		{
			phase = 1;
			i = 0;
		} else
		{
			if ((!dolfile->text_size[i]) || (dolfile->text_start[i] < 0x100))
			{
				*offset = 0;
				*pos = 0;
				*len = 0;
			} else
			{
				*offset = (void *)dolfile->text_start[i];
				*pos = dolfile->text_pos[i];
				*len = dolfile->text_size[i];
			}
			i++;
			return true;
		}
	}

	if (phase == 1)
	{
		if (i == 11)
		{
			phase = 2;
			return false;
		}	
		
		if ((!dolfile->data_size[i]) || (dolfile->data_start[i] < 0x100))
		{
			*offset = 0;
			*pos = 0;
			*len = 0;
		} else
		{
			*offset = (void *)dolfile->data_start[i];
			*pos = dolfile->data_pos[i];
			*len = dolfile->data_size[i];
		}
		i++;
		return true;
	}
	return false;
}

u32 load_dol_image_args(void *dolstart, struct __argv *argv)
{
	u32 i;
	dolheader *dolfile;

	if (dolstart)
	{
		dolfile = (dolheader *) dolstart;
		for (i = 0; i < 7; i++)
		{
			if ((!dolfile->text_size[i]) || (dolfile->text_start[i] < 0x100))
				continue;

			memmove((void *) dolfile->text_start[i], dolstart
				+ dolfile->text_pos[i], dolfile->text_size[i]);

			DCFlushRange ((void *) dolfile->text_start[i], dolfile->text_size[i]);
			ICInvalidateRange((void *) dolfile->text_start[i], dolfile->text_size[i]);
		}

		for (i = 0; i < 11; i++)
		{
			if ((!dolfile->data_size[i]) || (dolfile->data_start[i] < 0x100))
				continue;

			memmove((void *) dolfile->data_start[i], dolstart
					+ dolfile->data_pos[i], dolfile->data_size[i]);

			DCFlushRange((void *) dolfile->data_start[i],
					dolfile->data_size[i]);
		}

		if (argv && argv->argvMagic == ARGV_MAGIC)
		{
			void *new_argv = (void *) (dolfile->entry_point + 8);
			memmove(new_argv, argv, sizeof(*argv));
			DCFlushRange(new_argv, sizeof(*argv));
		}
		return dolfile->entry_point;
	}
	return 0;
}