/*******************************************************************************
 * patch.c
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

#include <string.h>
#include "patch.h"

GXRModeObj TVPal528Prog = 
{
    6,      		 // viDisplayMode
    640,             // fbWidth
    528,             // efbHeight
    528,             // xfbHeight
    (VI_MAX_WIDTH_PAL - 640)/2,         // viXOrigin
    (VI_MAX_HEIGHT_PAL - 528)/2,        // viYOrigin
    640,             // viWidth
    528,             // viHeight
    VI_XFBMODE_SF,   // xFBmode
    GX_FALSE,        // field_rendering
    GX_FALSE,        // aa

    // sample points arranged in increasing Y order
	{
		{6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
		{6,6},{6,6},{6,6},  // pix 1
		{6,6},{6,6},{6,6},  // pix 2
		{6,6},{6,6},{6,6}   // pix 3
	},
	
    // vertical filter[7], 1/64 units, 6 bits each
	{
		 0,         // line n-1
		 0,         // line n-1
		21,         // line n
		22,         // line n
		21,         // line n
		 0,         // line n+1
		 0          // line n+1
	}

};

GXRModeObj TVPal528ProgSoft = 
{
    6,      		 // viDisplayMode
    640,             // fbWidth
    528,             // efbHeight
    528,             // xfbHeight
    (VI_MAX_WIDTH_PAL - 640)/2,         // viXOrigin
    (VI_MAX_HEIGHT_PAL - 528)/2,        // viYOrigin
    640,             // viWidth
    528,             // viHeight
    VI_XFBMODE_SF,   // xFBmode
    GX_FALSE,        // field_rendering
    GX_FALSE,        // aa

    // sample points arranged in increasing Y order
	{
		{6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
		{6,6},{6,6},{6,6},  // pix 1
		{6,6},{6,6},{6,6},  // pix 2
		{6,6},{6,6},{6,6}   // pix 3
	},
	
    // vertical filter[7], 1/64 units, 6 bits each
	{
		 8,         // line n-1
		 8,         // line n-1
		10,         // line n
		12,         // line n
		10,         // line n
		 8,         // line n+1
		 8          // line n+1
	}

};

GXRModeObj TVPal528ProgUnknown = 
{
    6,      		 // viDisplayMode
    640,             // fbWidth
    264,             // efbHeight
    524,             // xfbHeight
    (VI_MAX_WIDTH_PAL - 640)/2,         // viXOrigin
    (VI_MAX_HEIGHT_PAL - 528)/2,        // viYOrigin
    640,             // viWidth
    524,             // viHeight
    VI_XFBMODE_SF,   // xFBmode
    GX_FALSE,        // field_rendering
    GX_TRUE,         // aa

    // sample points arranged in increasing Y order
	{
		{3,2},{9,6},{3,10},  // pix 0, 3 sample points, 1/12 units, 4 bits each
		{3,2},{9,6},{3,10},  // pix 1
		{9,2},{3,6},{9,10},  // pix 2
		{9,2},{3,6},{9,10}   // pix 3
	},
	
    // vertical filter[7], 1/64 units, 6 bits each
	{
		 4,         // line n-1
		 8,         // line n-1
		12,         // line n
		16,         // line n
		12,         // line n
		 8,         // line n+1
		 4          // line n+1
	}

};

/*GXRModeObj TVMpal480Prog =
{
    10,     		 // viDisplayMode
    640,             // fbWidth
    480,             // efbHeight
    480,             // xfbHeight
    (VI_MAX_WIDTH_NTSC - 640)/2,        // viXOrigin
    (VI_MAX_HEIGHT_NTSC - 480)/2,       // viYOrigin
    640,             // viWidth
    480,             // viHeight
    VI_XFBMODE_SF,   // xFBmode
    GX_FALSE,        // field_rendering
    GX_FALSE,        // aa

    // sample points arranged in increasing Y order
    {
		{6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
		{6,6},{6,6},{6,6},  // pix 1
		{6,6},{6,6},{6,6},  // pix 2
		{6,6},{6,6},{6,6}   // pix 3
    },

    // vertical filter[7], 1/64 units, 6 bits each
    {
          0,         // line n-1
          0,         // line n-1
         21,         // line n
         22,         // line n
         21,         // line n
          0,         // line n+1
          0          // line n+1
    }
};*/


bool compare_videomodes(GXRModeObj* mode1, GXRModeObj* mode2)
{
	if (mode1->viTVMode != mode2->viTVMode || mode1->fbWidth != mode2->fbWidth ||	mode1->efbHeight != mode2->efbHeight || mode1->xfbHeight != mode2->xfbHeight ||
	mode1->viXOrigin != mode2->viXOrigin || mode1->viYOrigin != mode2->viYOrigin || mode1->viWidth != mode2->viWidth || mode1->viHeight != mode2->viHeight ||
	mode1->xfbMode != mode2->xfbMode || mode1->field_rendering != mode2->field_rendering || mode1->aa != mode2->aa || mode1->sample_pattern[0][0] != mode2->sample_pattern[0][0] ||
	mode1->sample_pattern[1][0] != mode2->sample_pattern[1][0] ||	mode1->sample_pattern[2][0] != mode2->sample_pattern[2][0] ||
	mode1->sample_pattern[3][0] != mode2->sample_pattern[3][0] ||	mode1->sample_pattern[4][0] != mode2->sample_pattern[4][0] ||
	mode1->sample_pattern[5][0] != mode2->sample_pattern[5][0] ||	mode1->sample_pattern[6][0] != mode2->sample_pattern[6][0] ||
	mode1->sample_pattern[7][0] != mode2->sample_pattern[7][0] ||	mode1->sample_pattern[8][0] != mode2->sample_pattern[8][0] ||
	mode1->sample_pattern[9][0] != mode2->sample_pattern[9][0] ||	mode1->sample_pattern[10][0] != mode2->sample_pattern[10][0] ||
	mode1->sample_pattern[11][0] != mode2->sample_pattern[11][0] || mode1->sample_pattern[0][1] != mode2->sample_pattern[0][1] ||
	mode1->sample_pattern[1][1] != mode2->sample_pattern[1][1] ||	mode1->sample_pattern[2][1] != mode2->sample_pattern[2][1] ||
	mode1->sample_pattern[3][1] != mode2->sample_pattern[3][1] ||	mode1->sample_pattern[4][1] != mode2->sample_pattern[4][1] ||
	mode1->sample_pattern[5][1] != mode2->sample_pattern[5][1] ||	mode1->sample_pattern[6][1] != mode2->sample_pattern[6][1] ||
	mode1->sample_pattern[7][1] != mode2->sample_pattern[7][1] ||	mode1->sample_pattern[8][1] != mode2->sample_pattern[8][1] ||
	mode1->sample_pattern[9][1] != mode2->sample_pattern[9][1] ||	mode1->sample_pattern[10][1] != mode2->sample_pattern[10][1] ||
	mode1->sample_pattern[11][1] != mode2->sample_pattern[11][1] || mode1->vfilter[0] != mode2->vfilter[0] ||
	mode1->vfilter[1] != mode2->vfilter[1] ||	mode1->vfilter[2] != mode2->vfilter[2] || mode1->vfilter[3] != mode2->vfilter[3] ||	mode1->vfilter[4] != mode2->vfilter[4] ||
	mode1->vfilter[5] != mode2->vfilter[5] ||	mode1->vfilter[6] != mode2->vfilter[6] )
	{
		return false;
	} else
	{
		return true;
	}
}

void patch_videomode(GXRModeObj* mode1, GXRModeObj* mode2)
{
	mode1->viTVMode = mode2->viTVMode;
	mode1->fbWidth = mode2->fbWidth;
	mode1->efbHeight = mode2->efbHeight;
	mode1->xfbHeight = mode2->xfbHeight;
	mode1->viXOrigin = mode2->viXOrigin;
	mode1->viYOrigin = mode2->viYOrigin;
	mode1->viWidth = mode2->viWidth; 
	mode1->viHeight = mode2->viHeight;
	mode1->xfbMode = mode2->xfbMode; 
	mode1->field_rendering = mode2->field_rendering;
	mode1->aa = mode2->aa;
	mode1->sample_pattern[0][0] = mode2->sample_pattern[0][0];
	mode1->sample_pattern[1][0] = mode2->sample_pattern[1][0];
	mode1->sample_pattern[2][0] = mode2->sample_pattern[2][0];
	mode1->sample_pattern[3][0] = mode2->sample_pattern[3][0];
	mode1->sample_pattern[4][0] = mode2->sample_pattern[4][0];
	mode1->sample_pattern[5][0] = mode2->sample_pattern[5][0];
	mode1->sample_pattern[6][0] = mode2->sample_pattern[6][0];
	mode1->sample_pattern[7][0] = mode2->sample_pattern[7][0];
	mode1->sample_pattern[8][0] = mode2->sample_pattern[8][0];
	mode1->sample_pattern[9][0] = mode2->sample_pattern[9][0];
	mode1->sample_pattern[10][0] = mode2->sample_pattern[10][0];
	mode1->sample_pattern[11][0] = mode2->sample_pattern[11][0];
	mode1->sample_pattern[0][1] = mode2->sample_pattern[0][1];
	mode1->sample_pattern[1][1] = mode2->sample_pattern[1][1];	
	mode1->sample_pattern[2][1] = mode2->sample_pattern[2][1];
	mode1->sample_pattern[3][1] = mode2->sample_pattern[3][1];	
	mode1->sample_pattern[4][1] = mode2->sample_pattern[4][1];
	mode1->sample_pattern[5][1] = mode2->sample_pattern[5][1];	
	mode1->sample_pattern[6][1] = mode2->sample_pattern[6][1];
	mode1->sample_pattern[7][1] = mode2->sample_pattern[7][1];	
	mode1->sample_pattern[8][1] = mode2->sample_pattern[8][1];
	mode1->sample_pattern[9][1] = mode2->sample_pattern[9][1];	
	mode1->sample_pattern[10][1] = mode2->sample_pattern[10][1];
	mode1->sample_pattern[11][1] = mode2->sample_pattern[11][1]; 
	mode1->vfilter[0] = mode2->vfilter[0];
	mode1->vfilter[1] = mode2->vfilter[1];	
	mode1->vfilter[2] = mode2->vfilter[2];
	mode1->vfilter[3] = mode2->vfilter[3];	
	mode1->vfilter[4] = mode2->vfilter[4];
	mode1->vfilter[5] = mode2->vfilter[5];	
	mode1->vfilter[6] = mode2->vfilter[6];
}

int videomode_region(GXRModeObj* mode)
{
	if ( compare_videomodes(&TVNtsc480Int, mode)
	||   compare_videomodes(&TVNtsc480IntDf, mode)
	||   compare_videomodes(&TVNtsc480Prog, mode) )
	{
		return 0;
	}
	if ( compare_videomodes(&TVPal528Int, mode)
	||   compare_videomodes(&TVPal528IntDf, mode)
	||   compare_videomodes(&TVPal528Prog, mode) 
	||   compare_videomodes(&TVPal528ProgSoft, mode)
	||   compare_videomodes(&TVPal528ProgUnknown, mode) )
	{
		return 1;
	}
	if ( compare_videomodes(&TVMpal480IntDf, mode)
	||   compare_videomodes(&TVNtsc480Prog, mode) )
	{
		return 4;
	}
	if ( compare_videomodes(&TVEurgb60Hz480Int, mode)
	||   compare_videomodes(&TVEurgb60Hz480IntDf, mode)
	||   compare_videomodes(&TVEurgb60Hz480Prog, mode) )
	{
		return 5;
	}
	return -1;
}

int videomode_interlaced(GXRModeObj* mode)
{
	if ( compare_videomodes(&TVNtsc480Int, mode)
	||   compare_videomodes(&TVNtsc480IntDf, mode)
	||   compare_videomodes(&TVPal528Int, mode)
	||   compare_videomodes(&TVPal528IntDf, mode)
	||   compare_videomodes(&TVMpal480IntDf, mode)
	||   compare_videomodes(&TVEurgb60Hz480Int, mode)
	||   compare_videomodes(&TVEurgb60Hz480IntDf, mode) )
	{
		return 1;
	}	
	if ( compare_videomodes(&TVNtsc480Prog, mode)
	||   compare_videomodes(&TVPal528Prog, mode) 
	||   compare_videomodes(&TVPal528ProgSoft, mode)
	||   compare_videomodes(&TVPal528ProgUnknown, mode)
	||   compare_videomodes(&TVNtsc480Prog, mode) 
	||   compare_videomodes(&TVEurgb60Hz480Prog, mode) )
	{
		return 0;
	}
	return -1;
}

int videomode_480(GXRModeObj* mode)
{
	switch (videomode_region(mode))
	{
		case 0:
		case 4:
		case 5:
		return 1;
		
		case 1:
		return 0;
		
		default:
		return -1;		
	}	
}

static GXRModeObj*			videomodes[50];
static u32					videomodecount;

void search_video_modes(u8 *file, u32 size)
{
	videomodecount = 0;
	u8 *Addr = file;

	while ((u32)Addr < (u32)file+size-sizeof(GXRModeObj))
	{
		if (videomode_region((GXRModeObj*)Addr) != -1)
		{
			videomodes[videomodecount] = (GXRModeObj*)Addr;
			videomodecount++;
			Addr += sizeof(GXRModeObj);
		} else
		{
			Addr += 4;
		}
	}
}


void patch_video_modes_to(GXRModeObj* vmode, u32 videopatchselect)
{
	bool patch;
	if (videopatchselect > 0)
	{
		unsigned int i;
		for (i=0;i < videomodecount;i++)
		{
			patch = false;
			if (videopatchselect == 3)
			{
				patch = true;
			} else
			{
				if (videomode_interlaced(videomodes[i]) == videomode_interlaced(vmode))
				{
					if (videopatchselect == 2)
					{
						patch = true;
					} else
					{
						if (videomode_480(videomodes[i]) == videomode_480(vmode))
						{
							patch = true;
						}
					}				
				}
			}
			if (patch == true)
			{
				patch_videomode(videomodes[i], vmode);
			}
		}
	}		
}


s32 search_offset(u8 *file, u32 size, u8 *value, u32 valuesize, u32 offset, u32 *outoffset)
{
    int i = 0;
	for (i = offset; i < size; i+=4)
	{
        if (memcmp(file + i, value, valuesize) == 0) 
		{
            *outoffset = i;
			return 0;
        }
	}
	return -1;
}

s32 parser(u8 *file, u32 size, u8 *value, u32 valuesize, u8 *patch, u32 patchsize, u32 shift)
{  
	u32 cnt = 0;
	u32 maxsize = size - valuesize;
	if (patchsize + shift > valuesize)
	{
		maxsize = size - patchsize - shift;
	}

	u32 offset = 0;
	while (search_offset(file, maxsize-offset, value, valuesize, offset, &offset) >= 0)
	{
		memcpy(file + offset + shift, patch, patchsize);
		offset = offset + shift + patchsize;
		cnt++;
	}
	
	if (cnt > 0)
	{
		return cnt;
	} else
	{
		return -1;
	}
}

s32 patch_language(u8 *file, u32 size, u32 languageoption)
{
	u8 languagesearchpattern1[12] = { 0x7C, 0x60, 0x07, 0x75, 0x40, 0x82, 0x00, 0x10, 0x38, 0x00, 0x00, 0x00 };
	u8 languagesearchpattern2[4]  = { 0x88, 0x61, 0x00, 0x08 };
	u8 languagepatch[4] = { 0x38, 0x60, 0x00, languageoption };

	u32 offset;
	if (search_offset(file, size - 16, languagesearchpattern1, 12, 0, &offset) >= 0)
	{
		if (search_offset(file, size -4 -offset, languagesearchpattern2, 4, offset, &offset) >= 0)
		{
			memcpy(file + offset, languagepatch, 4);
			return 0;
		}	
	}
	return -1;
}


