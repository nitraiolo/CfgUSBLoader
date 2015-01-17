#ifndef __CONSOLE_H__
#define __CONSOLE_H__


#define FONT_XSIZE		8
#define FONT_YSIZE		16
#define FONT_XFACTOR	1
#define FONT_YFACTOR	1
#define FONT_XGAP			0
#define FONT_YGAP			0
#define TAB_SIZE			4

typedef struct _console_data_s {
	void *destbuffer;
	unsigned char *font;
	int con_xres,con_yres,con_stride;
	int target_x,target_y, tgt_stride;
	int cursor_row,cursor_col;
	int saved_row,saved_col;
	int con_rows, con_cols;

	unsigned int foreground,background;
} console_data_s;

extern int __console_write(struct _reent *r,int fd,const char *ptr,size_t len);
extern void __console_init(void *framebuffer,int xstart,int ystart,int xres,int yres,int stride);
extern void __console_flush(int retrace_min);
extern int __console_disable;

typedef struct unifont_header
{
	char head_tag[8];   // UNIFONT\0
	u32	 max_idx;       // 0xFFFF
	u32  reserve[2];
	char index_tag[4];  // INDX
	u32  index[0xFFFF+1]; // 24 bit: offset in 16 byte units; 8 bit: len (0,1,2)
	char glyph_tag[4];  // GLYP
	u32  glyph_size;
	// GLYPH DATA
	// char end_tag[4]  // END\0
} unifont_header;

extern struct unifont_header *unifont;
extern u8 *unifont_glyph;

int console_set_unifont(void *buf, int size);
int console_load_unifont(char *fname);

//extern const devoptab_t dotab_stdout;

#endif
