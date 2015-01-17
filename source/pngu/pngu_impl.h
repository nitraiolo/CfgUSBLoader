#ifndef __PNGU_I__
#define __PNGU_I__

#include "png.h"

// Constants
#define PNGU_SOURCE_BUFFER			1
#define PNGU_SOURCE_DEVICE			2

// Prototypes of helper functions
int pngu_info (IMGCTX ctx);
int pngu_decode (IMGCTX ctx, PNGU_u32 width, PNGU_u32 height, PNGU_u32 stripAlpha);
int pngu_decode_add_alpha (IMGCTX ctx, PNGU_u32 width, PNGU_u32 height, PNGU_u32 stripAlpha, int force32bit);
void pngu_free_info (IMGCTX ctx);
void pngu_read_data_from_buffer (png_structp png_ptr, png_bytep data, png_size_t length);
void pngu_write_data_to_buffer (png_structp png_ptr, png_bytep data, png_size_t length);
void pngu_flush_data_to_buffer (png_structp png_ptr);
int pngu_clamp (int value, int min, int max);


// PNGU Image context struct
struct _IMGCTX
{
	int source;
	void *buffer;
	char *filename;
	PNGU_u32 cursor;
	PNGU_u32 buf_size; // buffer size

	PNGU_u32 propRead;
	PNGUPROP prop;

	PNGU_u32 infoRead;
	png_structp png_ptr;
	png_infop info_ptr;
	FILE *fd;
	
	png_bytep *row_pointers;
	png_bytep img_data;
};

#endif
