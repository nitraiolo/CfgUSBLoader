/********************************************************************************************

PNGU libwiigui & grrlib additions

********************************************************************************************/
#include <stdio.h>
#include <malloc.h>
#include "png.h"
#include "pngu.h"
#include "pngu_impl.h"

#define _SHIFTL(v, s, w)	\
    ((PNGU_u32) (((PNGU_u32)(v) & ((0x01 << (w)) - 1)) << (s)))
#define _SHIFTR(v, s, w)	\
    ((PNGU_u32)(((PNGU_u32)(v) >> (s)) & ((0x01 << (w)) - 1)))


// Coded by Tantric for WiiMC (http://www.wiimc.org)
static inline PNGU_u32 coordsRGBA8(PNGU_u32 x, PNGU_u32 y, PNGU_u32 w)
{
	return ((((y >> 2) * (w >> 2) + (x >> 2)) << 5) + ((y & 3) << 2) + (x & 3)) << 1;
}

// Coded by Tantric for WiiMC (http://www.wiimc.org)
PNGU_u8 * PNGU_DecodeTo4x4RGBA8_EX (IMGCTX ctx, PNGU_u32 width, PNGU_u32 height, int * dstWidth, int * dstHeight, PNGU_u8 *dstPtr)
{
	PNGU_u8 default_alpha = 255;    // default alpha value, which is used if the source image doesn't have an alpha channel.
	PNGU_u8 *dst;
	int x, y, x2=0, y2=0, offset;
	int xRatio = 0, yRatio = 0;
	png_byte *pixel;

	if (pngu_decode (ctx, width, height, 0) != PNGU_OK)
		return NULL;

	int newWidth = width;
	int newHeight = height;

	if(width > 1024 || height > 1024)
	{
		float ratio = (float)width/(float)height;

		if(ratio > 1)
		{
			newWidth = 1024;
			newHeight = 1024/ratio;
		}
		else
		{
			newWidth = 1024*ratio;
			newHeight = 1024;
		}
		xRatio = (int)((width<<16)/newWidth)+1;
		yRatio = (int)((height<<16)/newHeight)+1;
	}

	int padWidth = newWidth;
	int padHeight = newHeight;
	if(padWidth%4) padWidth += (4-padWidth%4);
	if(padHeight%4) padHeight += (4-padHeight%4);

	int len = (padWidth * padHeight) << 2;
	if(len%32) len += (32-len%32);

	if(dstPtr)
		dst = dstPtr; // use existing allocation
	else
		dst = memalign (32, len);

	if(!dst)
		return NULL;

	for (y = 0; y < padHeight; y++)
	{
		for (x = 0; x < padWidth; x++)
		{
			offset = coordsRGBA8(x, y, padWidth);

			if(y >= newHeight || x >= newWidth)
			{
				dst[offset] = 0;
				dst[offset+1] = 255;
				dst[offset+32] = 255;
				dst[offset+33] = 255;
			}
			else
			{
				if(xRatio > 0)
				{
					x2 = ((x*xRatio)>>16);
					y2 = ((y*yRatio)>>16);
				}

				if (ctx->prop.imgColorType == PNGU_COLOR_TYPE_GRAY_ALPHA || 
					ctx->prop.imgColorType == PNGU_COLOR_TYPE_RGB_ALPHA)
				{
					if(xRatio > 0)
						pixel = &(ctx->row_pointers[y2][x2*4]);
					else
						pixel = &(ctx->row_pointers[y][x*4]);

					dst[offset] = pixel[3]; // Alpha
					dst[offset+1] = pixel[0]; // Red
					dst[offset+32] = pixel[1]; // Green
					dst[offset+33] = pixel[2]; // Blue
				}
				else
				{
					if(xRatio > 0)
						pixel = &(ctx->row_pointers[y2][x2*3]);
					else
						pixel = &(ctx->row_pointers[y][x*3]);

					dst[offset] = default_alpha; // Alpha
					dst[offset+1] = pixel[0]; // Red
					dst[offset+32] = pixel[1]; // Green
					dst[offset+33] = pixel[2]; // Blue
				}
			}
		}
	}

	// Free resources
	free (ctx->img_data);
	free (ctx->row_pointers);

	*dstWidth = padWidth;
	*dstHeight = padHeight;
	return dst;
}

// Coded by Tantric for libwiigui (http://code.google.com/p/libwiigui)
int PNGU_EncodeFromRGB (IMGCTX ctx, PNGU_u32 width, PNGU_u32 height, void *buffer, PNGU_u32 stride)
{
	png_uint_32 rowbytes;
	PNGU_u32 y;

	// Erase from the context any readed info
	pngu_free_info (ctx);
	ctx->propRead = 0;

	// Check if the user has selected a file to write the image
	if (ctx->source == PNGU_SOURCE_BUFFER);	

	else if (ctx->source == PNGU_SOURCE_DEVICE)
	{
		// Open file
		if (!(ctx->fd = fopen (ctx->filename, "wb")))
			return PNGU_CANT_OPEN_FILE;
	}

	else
		return PNGU_NO_FILE_SELECTED;

	// Allocation of libpng structs
	ctx->png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!(ctx->png_ptr))
	{
		if (ctx->source == PNGU_SOURCE_DEVICE)
			fclose (ctx->fd);
        return PNGU_LIB_ERROR;
	}

    ctx->info_ptr = png_create_info_struct (ctx->png_ptr);
    if (!(ctx->info_ptr))
    {
		png_destroy_write_struct (&(ctx->png_ptr), (png_infopp)NULL);
		if (ctx->source == PNGU_SOURCE_DEVICE)
			fclose (ctx->fd);
        return PNGU_LIB_ERROR;
    }

	if (ctx->source == PNGU_SOURCE_BUFFER)
	{
		// Installation of our custom data writer function
		ctx->cursor = 0;
		png_set_write_fn (ctx->png_ptr, ctx, pngu_write_data_to_buffer, pngu_flush_data_to_buffer);
	}
	else if (ctx->source == PNGU_SOURCE_DEVICE)
	{
		// Default data writer uses function fwrite, so it needs to use our FILE*
		png_init_io (ctx->png_ptr, ctx->fd);
	}

	// Setup output file properties
    png_set_IHDR (ctx->png_ptr, ctx->info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, 
				PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	// Allocate memory to store the image in RGB format
	rowbytes = width * 3;
	if (rowbytes % 4)
		rowbytes = ((rowbytes >>2) + 1) <<2; // Add extra padding so each row starts in a 4 byte boundary
		
	ctx->img_data = malloc(rowbytes * height);
	memset(ctx->img_data, 0, rowbytes * height);
	
	if (!ctx->img_data)
	{
		png_destroy_write_struct (&(ctx->png_ptr), (png_infopp)NULL);
		if (ctx->source == PNGU_SOURCE_DEVICE)
			fclose (ctx->fd);
		return PNGU_LIB_ERROR;
	}

	ctx->row_pointers = malloc (sizeof (png_bytep) * height);
	memset(ctx->row_pointers, 0, sizeof (png_bytep) * height);
	
	if (!ctx->row_pointers)
	{
		png_destroy_write_struct (&(ctx->png_ptr), (png_infopp)NULL);
		if (ctx->source == PNGU_SOURCE_DEVICE)
			fclose (ctx->fd);
		return PNGU_LIB_ERROR;
	}

	for (y = 0; y < height; ++y)
	{
		ctx->row_pointers[y] = buffer + (y * rowbytes);
	}

	// Tell libpng where is our image data
	png_set_rows (ctx->png_ptr, ctx->info_ptr, ctx->row_pointers);

	// Write file header and image data
	png_write_png (ctx->png_ptr, ctx->info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	// Tell libpng we have no more data to write
	png_write_end (ctx->png_ptr, (png_infop) NULL);

	// Free resources
	free (ctx->img_data);
	free (ctx->row_pointers);
	png_destroy_write_struct (&(ctx->png_ptr), &(ctx->info_ptr));
	if (ctx->source == PNGU_SOURCE_DEVICE)
		fclose (ctx->fd);

	// Success
	return ctx->cursor;
}

// Coded by Tantric for libwiigui (http://code.google.com/p/libwiigui)
int PNGU_EncodeFromGXTexture (IMGCTX ctx, PNGU_u32 width, PNGU_u32 height, void *buffer, PNGU_u32 stride)
{
	int res;
	PNGU_u32 x,y, tmpy1, tmpy2, tmpyWid, tmpxy;

	unsigned char * ptr = (unsigned char*)buffer;
	unsigned char * tmpbuffer = (unsigned char *)malloc(width*height*3);
	memset(tmpbuffer, 0, width*height*3);
	png_uint_32 offset;
	
	for(y=0; y < height; y++)
	{
		tmpy1 = y * 640*3;
		tmpy2 = y%4 << 2;
		tmpyWid = (((y >> 2)<<4)*width);

		for(x=0; x < width; x++)
		{
			offset = tmpyWid + ((x >> 2)<<6) + ((tmpy2+ x%4 ) << 1);
			tmpxy = x * 3 + tmpy1;

			tmpbuffer[tmpxy  ] = ptr[offset+1]; // R
			tmpbuffer[tmpxy+1] = ptr[offset+32]; // G
			tmpbuffer[tmpxy+2] = ptr[offset+33]; // B
		}
	}
	
	res = PNGU_EncodeFromRGB (ctx, width, height, tmpbuffer, stride);
	free(tmpbuffer);
	return res;
}

// Coded by Crayon for GRRLIB (http://code.google.com/p/grrlib)
int PNGU_EncodeFromEFB (IMGCTX ctx, PNGU_u32 width, PNGU_u32 height, PNGU_u32 stride)
{
    int res;
    PNGU_u32 x,y, tmpy, tmpxy, regval, val;
    unsigned char * tmpbuffer = (unsigned char *)malloc(width*height*3);
    memset(tmpbuffer, 0, width*height*3);

    for(y=0; y < height; y++)
    {
        tmpy = y * 640*3;
        for(x=0; x < width; x++)
        {
            regval = 0xc8000000|(_SHIFTL(x,2,10));
            regval = (regval&~0x3FF000)|(_SHIFTL(y,12,10));
            val = *(PNGU_u32*)regval;
            tmpxy = x * 3 + tmpy;
            tmpbuffer[tmpxy  ] = _SHIFTR(val,16,8); // R
            tmpbuffer[tmpxy+1] = _SHIFTR(val,8,8);  // G
            tmpbuffer[tmpxy+2] = val&0xff;          // B
        }
    }

    res = PNGU_EncodeFromRGB (ctx, width, height, tmpbuffer, stride);
    free(tmpbuffer);
    return res;
}


