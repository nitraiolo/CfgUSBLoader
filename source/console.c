
// Transparency and fixes by oggzee

#include <stdlib.h>
#include <string.h>
#include <reent.h>
#include <errno.h>
#include <sys/param.h>

#include "ogc/machine/asm.h"
#include "ogc/machine/processor.h"
#include "ogc/color.h"
#include "ogc/cache.h"
#include "ogc/video.h"
#include "ogc/system.h"

#include "console.h"
#include "ogc/consol.h"
#include "ogc/usbgecko.h"

#include <stdio.h>
#include <sys/iosupport.h>

#include "util.h"
#include "version.h"

#include "pngu/pngu.h"
#include "net.h"

extern void *bg_buf_rgba;
extern void *bg_buf_ycbr;

//---------------------------------------------------------------------------------
const devoptab_t dotab_stdout = {
//---------------------------------------------------------------------------------
	"stdout",	// device name
	0,			// size of file structure
	NULL,		// device open
	NULL,		// device close
	__console_write,	// device write
	NULL,		// device read
	NULL,		// device seek
	NULL,		// device fstat
	NULL,		// device stat
	NULL,		// device link
	NULL,		// device unlink
	NULL,		// device chdir
	NULL,		// device rename
	NULL,		// device mkdir
	0,			// dirStateSize
	NULL,		// device diropen_r
	NULL,		// device dirreset_r
	NULL,		// device dirnext_r
	NULL,		// device dirclose_r
	NULL		// device statvfs_r
};

//color table YcbYcr
const unsigned int color_table_YcbYcr[] =
{
  0x00800080,		// 30 normal black
  0x246A24BE,		// 31 normal red
  0x4856484B,		// 32 normal green
  0x6D416D8A,		// 33 normal yellow
  0x0DBE0D75,		// 34 normal blue
  0x32A932B4,		// 35 normal magenta
  0x56955641,		// 36 normal cyan
  0xC580C580,		// 37 normal white
  0x7B807B80,		// 30 bright black
  0x4C544CFF,		// 31 bright red
  0x95299512,		// 32 bright green
  0xE200E294,		// 33 bright yellow
  0x1CFF1C6B,		// 34 bright blue
  0x69D669ED,		// 35 bright magenta
  0xB2ABB200,		// 36 bright cyan
  0xFF80FF80,		// 37 bright white
};
// RGBA
const unsigned int color_table[] =
{
  0x000000FF,		// 30 normal black
  0x800000FF,		// 31 normal red
  0x008000FF,		// 32 normal green
  0x808000FF,		// 33 normal yellow
  0x000080FF,		// 34 normal blue
  0x800080FF,		// 35 normal magenta
  0x008080FF,		// 36 normal cyan
  0xB0B0B0FF,		// 37 normal white
  0x606060FF,		// 30 bright black
  0xFF0000FF,		// 31 bright red
  0x00FF00FF,		// 32 bright green
  0xFFFF00FF,		// 33 bright yellow
  0x0000FFFF,		// 34 bright blue
  0xFF00FFFF,		// 35 bright magenta
  0x00FFFFFF,		// 36 bright cyan
  0xFFFFFFFF,		// 37 bright white
};

#define RGBA_COLOR_WHITE 0xFFFFFFFF
#define RGBA_COLOR_BLACK 0x000000FF

struct unifont_header *unifont = NULL;
u8 *unifont_glyph = NULL;

inline u32 RGB8x2_TO_YCbYCr(u8 *c1, u8 *c2)
{
	return PNGU_RGB8_TO_YCbYCr(c1[0], c1[1], c1[2], c2[0], c2[1], c2[2]);
}


static u32 do_xfb_copy = FALSE;
static struct _console_data_s stdcon;
static struct _console_data_s *curr_con = NULL;
static void *_console_buffer = NULL;

// transparency
struct _c1
{
	//char c;
	wchar_t c;
	int fg, bg;
};
//static void *_bg_buffer = NULL;
//static unsigned int _bg_color = COLOR_BLACK;
static unsigned int _bg_color = RGBA_COLOR_BLACK;
static int _c_buffer_size = 0;
static struct _c1 *_c_buffer = NULL;
static int _tr_enable = 0;
int __console_disable = 0;
int __console_scroll = 0;

static s32 __gecko_status = -1;
static u32 __gecko_safe = 0;

extern u8 console_font_8x16[];
extern u8 console_font_512[];

int fb_change = 0;
int retrace_cnt = 0;
int con_exception_mode = 0;

void __console_vipostcb(u32 retraceCnt)
{
	if (retrace_cnt < 5) {
		retrace_cnt++;
		return;
	}
	if (!fb_change) return;
	do_xfb_copy = TRUE;
	__console_flush(0);
	do_xfb_copy = FALSE;
}

void __console_flush(int retrace_min)
{
	u32 ycnt,xcnt, fb_stride;
	u32 *fb,*ptr;

	if (!fb_change && retrace_min >= 0) {
		if (retrace_min == 0) retrace_cnt = 0;
		return;
	}
	if (retrace_cnt < retrace_min) return;
	fb_change = 0;
	retrace_cnt = 0;

	if (__console_disable) return;

	ptr = curr_con->destbuffer;
	fb = VIDEO_GetCurrentFramebuffer()+(curr_con->target_y*curr_con->tgt_stride) + curr_con->target_x*VI_DISPLAY_PIX_SZ;
	fb_stride = curr_con->tgt_stride/4 - (curr_con->con_xres/VI_DISPLAY_PIX_SZ);

	for(ycnt=curr_con->con_yres;ycnt>0;ycnt--)
	{
		for(xcnt=curr_con->con_xres;xcnt>0;xcnt-=VI_DISPLAY_PIX_SZ)
		{
			*fb++ = *ptr++;
		}
		fb += fb_stride;
	}
}

void __console_bg_grab(void *x_fb)
{
	/*
	u32 ycnt,xcnt, fb_stride;
	u32 *fb,*ptr;
	if (x_fb == NULL) x_fb = VIDEO_GetCurrentFramebuffer();

	if (!_bg_buffer) return;
	ptr = _bg_buffer;
	fb = x_fb + (curr_con->target_y*curr_con->tgt_stride) + curr_con->target_x*VI_DISPLAY_PIX_SZ;
	fb_stride = curr_con->tgt_stride/4 - (curr_con->con_xres/VI_DISPLAY_PIX_SZ);

	for(ycnt=curr_con->con_yres;ycnt>0;ycnt--)
	{
		for(xcnt=curr_con->con_xres;xcnt>0;xcnt-=VI_DISPLAY_PIX_SZ)
		{
			*ptr++ = *fb++;
		}
		fb += fb_stride;
	}
	*/
}

int console_set_unifont(void *buf, int size)
{
	unifont_header *u = (unifont_header*) buf;
	char *end;
	unifont = NULL;
	unifont_glyph = NULL;
	if (!buf) return -1;
	if (size < sizeof(unifont_header) + 4) return -1;
	/*
	printf("head: %.8s\n", u->head_tag);
	printf("index: %.4s\n", u->index_tag);
	printf("glyph: %.4s\n", u->glyph_tag);
	printf("max i: %04x\n", u->max_idx);
	printf("siz g: %04x\n", u->glyph_size);
	*/
	if (memcmp(u->head_tag, "UNIFONT", 8)) return -1;
	if (memcmp(u->index_tag, "INDX", 4)) return -1;
	if (memcmp(u->glyph_tag, "GLYP", 4)) return -1;
	if (u->max_idx != 0xFFFF) return -1;
	if (size < sizeof(unifont_header) + u->glyph_size + 4) return -1;
	end = buf + sizeof(unifont_header) + u->glyph_size * 16;
	//printf("end: %.4s\n", end);
	if (memcmp(end, "END", 4)) return -1;
	unifont = u;
	unifont_glyph = buf + sizeof(unifont_header);
	return 0;
}

int console_load_unifont(char *fname)
{
	FILE *f = NULL;
	void *buf = NULL;
	struct stat st;
	u32 size;
	s32 ret;

	//printf("load %s\n", fname); Wpad_WaitButtons();
	ret = stat(fname, &st);
	if (ret != 0) return -1;
	size = st.st_size;
	buf = mem1_alloc(size);
	if (!buf) return -1;
	f = fopen(fname, "rb");
	if (!f) goto err;
	ret = fread(buf, 1, size, f);
	fclose(f);
	if (ret != size) goto err;
	//printf("unifont: %d\n", size); Wpad_WaitButtons();
	if (console_set_unifont(buf, size)) goto err;
	/*printf("unifont: OK\n");
	printf("機動戦士ガンダム MS戦線\n");
	printf("機 動 戦 士 ガ ン ダ ム MS 戦 線\n");
	Wpad_WaitButtons();*/
	extern void unifont_cache_init();
	unifont_cache_init();

	return 0;
err:
	SAFE_FREE(buf)
	return -1;
}

static void _bg_console_draw_glyph(unsigned char *pbits)
{
	console_data_s *con;
	int ay;
	unsigned int *ptr;
	//unsigned char bg[8] = {0,0,0,0,0,0,0,0};
	unsigned char *bg;
	unsigned char bits;
	unsigned int color;
	unsigned int fgcolor;
	//unsigned int bgcolor;
	unsigned int nextline;
	u8 *c1, *c2;

	if(do_xfb_copy==TRUE) return;
	if(!curr_con) return;
	con = curr_con;
	if(!bg_buf_rgba) return;

	ptr = (unsigned int*)(con->destbuffer + ( con->con_stride *  con->cursor_row * FONT_YSIZE ) + ((con->cursor_col * FONT_XSIZE / 2) * 4));
	//bg = (unsigned char*)(bg_buf_rgba + ( con->target_y + con->cursor_row * FONT_YSIZE )*640*4 + (con->target_x + con->cursor_col * FONT_XSIZE) * 4 );
	
	nextline = con->con_stride/4 - 4;
	fgcolor = con->foreground;
	//bgcolor = con->background;

	for (ay = 0; ay < FONT_YSIZE; ay++)
	{
		/* hard coded loop unrolling ! */
		/* this depends on FONT_XSIZE = 8*/
#if FONT_XSIZE == 8
		bits = *pbits++;
		bg = (unsigned char*)(bg_buf_rgba + ( con->target_y + con->cursor_row * FONT_YSIZE + ay)*640*4 + (con->target_x + con->cursor_col * FONT_XSIZE) * 4 );

		/* bits 1 & 2 */
		c1 = (bits & 0x80) ? (u8*)&fgcolor : &bg[0];
		c2 = (bits & 0x40) ? (u8*)&fgcolor : &bg[4];
		color = RGB8x2_TO_YCbYCr(c1, c2);
		*ptr++ = color;
		bg += 8;

		/* bits 3 & 4 */
		c1 = (bits & 0x20) ? (u8*)&fgcolor : &bg[0];
		c2 = (bits & 0x10) ? (u8*)&fgcolor : &bg[4];
		color = RGB8x2_TO_YCbYCr(c1, c2);
		*ptr++ = color;
		bg += 8;

		/* bits 5 & 6 */
		c1 = (bits & 0x08) ? (u8*)&fgcolor : &bg[0];
		c2 = (bits & 0x04) ? (u8*)&fgcolor : &bg[4];
		color = RGB8x2_TO_YCbYCr(c1, c2);
		*ptr++ = color;
		bg += 8;

		/* bits 7 & 8 */
		c1 = (bits & 0x02) ? (u8*)&fgcolor : &bg[0];
		c2 = (bits & 0x01) ? (u8*)&fgcolor : &bg[4];
		color = RGB8x2_TO_YCbYCr(c1, c2);
		*ptr++ = color;
		bg += 8;

		/* next line */
		ptr += nextline;
#else
#endif
	}
}

static void _nc_console_draw_glyph(unsigned char *pbits)
{
	console_data_s *con;
	int ay;
	unsigned int *ptr;
	unsigned char bits;
	unsigned int color;
	unsigned int fgcolor, bgcolor;
	unsigned int nextline;
	u8 *c1, *c2;

	if(do_xfb_copy==TRUE) return;
	if(!curr_con) return;
	
	con = curr_con;

	ptr = (unsigned int*)(con->destbuffer + ( con->con_stride *  con->cursor_row * FONT_YSIZE ) + ((con->cursor_col * FONT_XSIZE / 2) * 4));
	
	nextline = con->con_stride/4 - 4;
	fgcolor = con->foreground;
	bgcolor = con->background;

	for (ay = 0; ay < FONT_YSIZE; ay++)
	{
		/* hard coded loop unrolling ! */
		/* this depends on FONT_XSIZE = 8*/
#if FONT_XSIZE == 8
		bits = *pbits++;

		/* bits 1 & 2 */
		c1 = (bits & 0x80) ? (u8*)&fgcolor : (u8*)&bgcolor;
		c2 = (bits & 0x40) ? (u8*)&fgcolor : (u8*)&bgcolor;
		color = RGB8x2_TO_YCbYCr(c1, c2);
		*ptr++ = color;

		/* bits 3 & 4 */
		c1 = (bits & 0x20) ? (u8*)&fgcolor : (u8*)&bgcolor;
		c2 = (bits & 0x10) ? (u8*)&fgcolor : (u8*)&bgcolor;
		color = RGB8x2_TO_YCbYCr(c1, c2);
		*ptr++ = color;

		/* bits 5 & 6 */
		c1 = (bits & 0x08) ? (u8*)&fgcolor : (u8*)&bgcolor;
		c2 = (bits & 0x04) ? (u8*)&fgcolor : (u8*)&bgcolor;
		color = RGB8x2_TO_YCbYCr(c1, c2);
		*ptr++ = color;

		/* bits 7 & 8 */
		c1 = (bits & 0x02) ? (u8*)&fgcolor : (u8*)&bgcolor;
		c2 = (bits & 0x01) ? (u8*)&fgcolor : (u8*)&bgcolor;
		color = RGB8x2_TO_YCbYCr(c1, c2);
		*ptr++ = color;

		/* next line */
		ptr += nextline;
#else
#endif
	}

}

static void _console_draw_glyph(unsigned char *pbits)
{
	if (_tr_enable && curr_con->background == _bg_color) {
		_bg_console_draw_glyph(pbits);
	} else {
		_nc_console_draw_glyph(pbits);
	}
}

static void _nc_console_drawc(int c)
{
	if (!curr_con) return;
	unsigned char *pbits;
	if (c <= 512) {
		pbits = &curr_con->font[c * FONT_YSIZE];
		_console_draw_glyph(pbits);
	} else if (unifont && unifont->index[c]) {
		u32 off = unifont->index[c] >> 8;
		u32 len = unifont->index[c] & 0x0F;
		pbits = &unifont_glyph[off * 16];
		_console_draw_glyph(pbits);
		if (len == 2) {
			if (curr_con->cursor_col + 1 >= curr_con->con_cols) return;
			curr_con->cursor_col++;
			pbits = &unifont_glyph[(off+1) * 16];
			_console_draw_glyph(pbits);
		}
	}
}


#if 0
// YcbYcr
static void _nc_console_drawc(int c)
{
	console_data_s *con;
	int ay;
	unsigned int *ptr;
	unsigned char *pbits;
	unsigned char bits;
	unsigned int color;
	unsigned int fgcolor, bgcolor;
	unsigned int nextline;

	if(do_xfb_copy==TRUE) return;
	if(!curr_con) return;
	con = curr_con;

	if (_tr_enable && _bg_buffer && con->background == _bg_color) {
		_bg_console_drawc(c);
		return;
	}

	ptr = (unsigned int*)(con->destbuffer + ( con->con_stride *  con->cursor_row * FONT_YSIZE ) + ((con->cursor_col * FONT_XSIZE / 2) * 4));
	pbits = &con->font[c * FONT_YSIZE];
	
	nextline = con->con_stride/4 - 4;
	fgcolor = con->foreground;
	bgcolor = con->background;

	for (ay = 0; ay < FONT_YSIZE; ay++)
	{
		/* hard coded loop unrolling ! */
		/* this depends on FONT_XSIZE = 8*/
#if FONT_XSIZE == 8
		bits = *pbits++;

		/* bits 1 & 2 */
		if ( bits & 0x80)
			color = fgcolor & 0xFFFF00FF;
		else
			color = bgcolor & 0xFFFF00FF;
		if (bits & 0x40)
			color |= fgcolor  & 0x0000FF00;
		else
			color |= bgcolor  & 0x0000FF00;
		*ptr++ = color;

		/* bits 3 & 4 */
		if ( bits & 0x20)
			color = fgcolor & 0xFFFF00FF;
		else
			color = bgcolor & 0xFFFF00FF;
		if (bits & 0x10)
			color |= fgcolor  & 0x0000FF00;
		else
			color |= bgcolor  & 0x0000FF00;
		*ptr++ = color;

		/* bits 5 & 6 */
		if ( bits & 0x08)
			color = fgcolor & 0xFFFF00FF;
		else
			color = bgcolor & 0xFFFF00FF;
		if (bits & 0x04)
			color |= fgcolor  & 0x0000FF00;
		else
			color |= bgcolor  & 0x0000FF00;
		*ptr++ = color;

		/* bits 7 & 8 */
		if ( bits & 0x02)
			color = fgcolor & 0xFFFF00FF;
		else
			color = bgcolor & 0xFFFF00FF;
		if (bits & 0x01)
			color |= fgcolor  & 0x0000FF00;
		else
			color |= bgcolor  & 0x0000FF00;
		*ptr++ = color;

		/* next line */
		ptr += nextline;
#else
#endif
	}
}
#endif

static void __console_drawc(int c)
{
	if(!curr_con) return;
	if (_tr_enable && _c_buffer) {
		int cidx = curr_con->cursor_row * curr_con->con_cols + curr_con->cursor_col;
		_c_buffer[cidx].c = c;
		_c_buffer[cidx].fg = curr_con->foreground;
		_c_buffer[cidx].bg = curr_con->background;
	}
	_nc_console_drawc(c);
	//fb_change = 1;
}

void _c_repaint();


void _bg_repaint()
{
	/*
	if (!_bg_buffer) return;
	// clear
	unsigned int c;
	unsigned int *p;
	unsigned int *bg;
	c = (curr_con->con_xres * curr_con->con_yres)/2;
	p = (unsigned int*)curr_con->destbuffer;
	bg = _bg_buffer;
	while(c--) *p++ = *bg++;
	*/
	unsigned int y;
	unsigned int *p;
	unsigned int *bg;
	if (!bg_buf_ycbr) return;
	p = (unsigned int*)curr_con->destbuffer;
	bg = (unsigned int*)(bg_buf_ycbr + curr_con->target_y * 640 * 2 + curr_con->target_x * 2 );
	for (y=0; y<curr_con->con_yres; y++) {
		memcpy(p, bg, curr_con->con_xres * 2);
		p += curr_con->con_xres / 2;
		bg += 640/2;
	}

}

void _bg_scroll()
{
	// clear
	_bg_repaint();
	// scroll
	memmove(_c_buffer, _c_buffer + curr_con->con_cols,
		sizeof(struct _c1) * (curr_con->con_rows-1) * curr_con->con_cols);
	// clear last
	memset(_c_buffer + (curr_con->con_rows-1) * curr_con->con_cols, 0,
		sizeof(struct _c1) * curr_con->con_cols);
	// redraw characters
	_c_repaint();
}

void _c_repaint()
{
	// repaint
	if (!_c_buffer) return;
	int save_row = curr_con->cursor_row;
	int save_col = curr_con->cursor_col;
	int save_fg = curr_con->foreground;
	int save_bg = curr_con->background;
	int x, y, i = 0;
	for (y=0; y < curr_con->con_rows - 1; y++) {
		for (x=0; x < curr_con->con_cols; x++) {
			curr_con->cursor_col = x;
			curr_con->cursor_row = y;
			curr_con->foreground = _c_buffer[i].fg;
			curr_con->background = _c_buffer[i].bg;
			if (_c_buffer[i].c) _nc_console_drawc(_c_buffer[i].c);
			i++;
		}
	}
	curr_con->cursor_row = save_row;
	curr_con->cursor_col = save_col;
	curr_con->foreground = save_fg;
	curr_con->background = save_bg;
}


static void __console_clear(void)
{
	console_data_s *con;
	unsigned int c;
	unsigned int color;
	unsigned int *p;

	if(!curr_con) return;
	con = curr_con;

	if (_tr_enable) {
		_bg_repaint();
		memset(_c_buffer, 0, _c_buffer_size);
	} else {
		c = (con->con_xres*con->con_yres)/2;
		p = (unsigned int*)con->destbuffer;
		color = RGB8x2_TO_YCbYCr((u8 *)&(con->background), (u8 *)&(con->background));
		while (c--) *p++ = color;
	}

	con->cursor_row = 0;
	con->cursor_col = 0;
	con->saved_row = 0;
	con->saved_col = 0;
}

void __console_enable(void *fb)
{
	if(_tr_enable) {
		__console_bg_grab(fb);
		_bg_repaint();
		_c_repaint();
	}
	__console_disable = 0;
	__console_flush(-1);
}

void __console_init(void *framebuffer,int xstart,int ystart,int xres,int yres,int stride)
{
	// note: this is called by the CODE DUMP handler (c_default_exceptionhandler)
	if (yres > 480) yres = 480;
	unsigned int level;
	console_data_s *con = &stdcon;

	_CPU_ISR_Disable(level);

	_tr_enable = 0;
	__console_disable = 0;

	con->destbuffer = framebuffer;
	con->con_xres = xres;
	con->con_yres = yres;
	con->con_cols = xres / FONT_XSIZE;
	con->con_rows = yres / FONT_YSIZE;
	con->con_stride = con->tgt_stride = stride;
	con->target_x = xstart;
	con->target_y = ystart;

	con->font = console_font_8x16;

	//con->foreground = COLOR_WHITE;
	//con->background = COLOR_BLACK;
	con->foreground = RGBA_COLOR_WHITE;
	con->background = RGBA_COLOR_BLACK;

	curr_con = con;

	__console_clear();
	fb_change = 1;

	devoptab_list[STD_OUT] = &dotab_stdout;
	devoptab_list[STD_ERR] = &dotab_stdout;
	_CPU_ISR_Restore(level);

	setvbuf(stdout, NULL , _IONBF, 0);
	setvbuf(stderr, NULL , _IONBF, 0);

	// print debug log
	if (con_exception_mode == 0) {
		void Music_PauseVoice(bool pause);
		Music_PauseVoice(true);
		__console_scroll = 1;
		con_exception_mode = 1;
		puts(dbg_log_buf);
		fputs("cfg", stdout);
		puts(CFG_VERSION_STR);
		//memcpy((void *)0xC1300000, dbg_log_buf, DBG_LOG_SIZE);
	}
	con_exception_mode = 2;
}

void __console_init_ex(void *conbuffer,int tgt_xstart,int tgt_ystart,int tgt_stride,int con_xres,int con_yres,int con_stride)
{
	unsigned int level;
	console_data_s *con = &stdcon;

	_CPU_ISR_Disable(level);

	con->destbuffer = conbuffer;
	con->target_x = tgt_xstart;
	con->target_y = tgt_ystart;
	con->con_xres = con_xres;
	con->con_yres = con_yres;
	con->tgt_stride = tgt_stride;
	con->con_stride = con_stride;
	con->con_cols = con_xres / FONT_XSIZE;
	con->con_rows = con_yres / FONT_YSIZE;
	con->cursor_row = 0;
	con->cursor_col = 0;
	con->saved_row = 0;
	con->saved_col = 0;

	//con->font = console_font_8x16;
	con->font = console_font_512;

	//con->foreground = COLOR_WHITE;
	//con->background = COLOR_BLACK;
	con->foreground = RGBA_COLOR_WHITE;
	con->background = RGBA_COLOR_BLACK;

	curr_con = con;

	if(_tr_enable) {
		__console_bg_grab(NULL);
		con->background = _bg_color;
	}

	__console_clear();
	fb_change = 1;
	retrace_cnt = 0;

	devoptab_list[STD_OUT] = &dotab_stdout;
	devoptab_list[STD_ERR] = &dotab_stdout;

	VIDEO_SetPostRetraceCallback(__console_vipostcb);

	_CPU_ISR_Restore(level);

	setvbuf(stdout, NULL , _IONBF, 0);
	setvbuf(stderr, NULL , _IONBF, 0);
}

static int __console_parse_escsequence(char *pchr)
{
	char chr;
	console_data_s *con;
	int i;
	int parameters[3];
	int para;

	if(!curr_con) return -1;
	con = curr_con;

	/* set default value */
	para = 0;
	parameters[0] = 0;
	parameters[1] = 0;
	parameters[2] = 0;

	/* scan parameters */
	i = 0;
	chr = *pchr;
	while( (para < 3) && (chr >= '0') && (chr <= '9') )
	{
		while( (chr >= '0') && (chr <= '9') )
		{
			/* parse parameter */
			parameters[para] *= 10;
			parameters[para] += chr - '0';
			pchr++;
			i++;
			chr = *pchr;
		}
		para++;

		if( *pchr == ';' )
		{
		  /* skip parameter delimiter */
		  pchr++;
			i++;
		}
		chr = *pchr;
	}

	/* get final character */
	chr = *pchr++;
	i++;
	switch(chr)
	{
		/////////////////////////////////////////
		// Cursor directional movement
		/////////////////////////////////////////
		case 'A':
		{
			curr_con->cursor_row -= parameters[0];
			if(curr_con->cursor_row < 0) curr_con->cursor_row = 0;
			break;
		}
		case 'B':
		{
			curr_con->cursor_row += parameters[0];
			if(curr_con->cursor_row >= curr_con->con_rows) curr_con->cursor_row = curr_con->con_rows - 1;
			break;
		}
		case 'C':
		{
			curr_con->cursor_col += parameters[0];
			if(curr_con->cursor_col >= curr_con->con_cols) curr_con->cursor_col = curr_con->con_cols - 1;
			break;
		}
		case 'D':
		{
			curr_con->cursor_col -= parameters[0];
			if(curr_con->cursor_col < 0) curr_con->cursor_col = 0;
			break;
		}
		/////////////////////////////////////////
		// Cursor position movement
		/////////////////////////////////////////
		case 'H':
		case 'f':
		{
			curr_con->cursor_col = parameters[1];
			curr_con->cursor_row = parameters[0];
			if(curr_con->cursor_row >= curr_con->con_rows) curr_con->cursor_row = curr_con->con_rows - 1;
			if(curr_con->cursor_col >= curr_con->con_cols) curr_con->cursor_col = curr_con->con_cols - 1;
			break;
		}
		/////////////////////////////////////////
		// Screen clear
		/////////////////////////////////////////
		case 'J':
		{
			/* different erase modes not yet supported, just clear all */
			__console_clear();
			break;
		}
		/////////////////////////////////////////
		// Line clear
		/////////////////////////////////////////
		case 'K':
		{
			break;
		}
		/////////////////////////////////////////
		// Save cursor position
		/////////////////////////////////////////
		case 's':
		{
			con->saved_col = con->cursor_col;
			con->saved_row = con->cursor_row;
			break;
		}
		/////////////////////////////////////////
		// Load cursor position
		/////////////////////////////////////////
		case 'u':
			con->cursor_col = con->saved_col;
			con->cursor_row = con->saved_row;
			break;
		/////////////////////////////////////////
		// SGR Select Graphic Rendition
		/////////////////////////////////////////
		case 'm':
		{
			// handle 30-37,39 for foreground color changes
			if( (parameters[0] >= 30) && (parameters[0] <= 39) )
			{
				parameters[0] -= 30;

				//39 is the reset code
				if(parameters[0] == 9){
					parameters[0] = 15;
				}
				else if(parameters[0] > 7){
					parameters[0] = 7;
				}

				if(parameters[1] == 1)
				{
					// Intensity: Bold makes color bright
					parameters[0] += 8;
				}
				con->foreground = color_table[parameters[0]];
			}
			// handle 40-47 for background color changes
			else if( (parameters[0] >= 40) && (parameters[0] <= 47) )
			{
				parameters[0] -= 40;

				if(parameters[1] == 1)
				{
					// Intensity: Bold makes color bright
					parameters[0] += 8;
				}
				con->background = color_table[parameters[0]];
			}
		  break;
		}
	}

	return(i);
}

int __console_write(struct _reent *r,int fd,const char *ptr,size_t len)
{
	int i = 0;
	char *tmp = (char*)ptr;
	console_data_s *con;
	//char chr;
	wchar_t chr;

	if(__gecko_status>=0) {
		if(__gecko_safe)
			usb_sendbuffer_safe(__gecko_status,ptr,len);
		else
			usb_sendbuffer(__gecko_status,ptr,len);
	}

	if(!curr_con) return -1;
	con = curr_con;
	if(!tmp || len<=0) return -1;

	// custom formatting of exception
	switch (con_exception_mode) {
		default:
		case 0:
		case 1:
			break;
		case 2:
			if (strcasestr(ptr, "Exception")) {
				while (*tmp == '\n') tmp++;
				con_exception_mode++;
			}
			break;
		case 3:
			if (strcasestr(ptr, "STACK DUMP")) {
				con_exception_mode++;
				break;
			}
			return len;
		case 4:
			if (strcasestr(ptr, "CODE DUMP")) {
				con_exception_mode++;
				tmp = "\n\n";
				len = 2;
			}
			break;
		case 5:
			if (strcasestr(ptr, "Reload")) {
				con_exception_mode = 1;
				//WII_LaunchTitle(TITLE_ID(0x00010001,0xAF1BF516)); // HBC 1.07
				break;
			}
			return len;
	}

	i = 0;
	while(*tmp!='\0' && i<len)
	{
		int k = 1;
		chr = *tmp;
		if (chr >= 0x80) {
			// UTF-8 decode
			k = mbtowc(&chr, tmp, MIN(6,len-i));
			// ignore utf8 parse errors
			if (k < 1) k = 1;
			// font only contains the first 512 chars
			if ((unsigned)chr >= 512) {
				int map_chr = map_ufont(chr);
				if (map_chr == 0 && (unsigned)chr <= 0xFFFF) {
					if (unifont && unifont->index[(unsigned)chr]) {
						// unifont containst almost all unicode chars
						map_chr = chr;
						// if len==2 and right border reached: wrap around
						int len = unifont->index[(unsigned)chr] & 0x0F;
						if (len > 1 && con->cursor_col + len > con->con_cols) {
							// insert fake space
							chr = ' ';
							goto just_print;
						}
					}
				}
				chr = map_chr;
			}
		}
		tmp += k;
		i += k;
		if ( (chr == 0x1b) && (*tmp == '[') )
		{
			/* escape sequence found */
			int k;

			tmp++;
			i++;
			k = __console_parse_escsequence(tmp);
			tmp += k;
			i += k;
		}
		else
		{
			switch(chr)
			{
				case '\n':
					con->cursor_row++;
					con->cursor_col = 0;
					break;
				case '\r':
					con->cursor_col = 0;
					break;
				case '\b':
					con->cursor_col--;
					if(con->cursor_col < 0)
					{
						con->cursor_col = 0;
					}
					break;
				case '\f':
					con->cursor_row++;
					break;
				case '\t':
					if(con->cursor_col%TAB_SIZE) con->cursor_col += (con->cursor_col%TAB_SIZE);
					else con->cursor_col += TAB_SIZE;
					break;
				default:
					just_print:
					__console_drawc(chr);
					con->cursor_col++;

					if( con->cursor_col >= con->con_cols)
					{
						/* if right border reached wrap around */
						con->cursor_row++;
						con->cursor_col = 0;
					}
			}
		}

		if( con->cursor_row >= con->con_rows)
		{
			/* if bottom border reached scroll */
			if (_tr_enable) {
				_bg_scroll();
			} else {
				memcpy(con->destbuffer,
					con->destbuffer+con->con_stride*(FONT_YSIZE*FONT_YFACTOR+FONT_YGAP),
					con->con_stride*con->con_yres-FONT_YSIZE);

				unsigned int cnt = (con->con_stride * (FONT_YSIZE * FONT_YFACTOR + FONT_YGAP))/4;
				unsigned int *bptr = (unsigned int*)(con->destbuffer + con->con_stride * (con->con_yres - FONT_YSIZE));
				unsigned int color = RGB8x2_TO_YCbYCr((u8 *)&(con->background), (u8 *)&(con->background));
				while(cnt--)
					*bptr++ = color;
			}
			if (__console_scroll)
			{
				// flush on screen
				__console_flush(0);
				VIDEO_WaitVSync();
			}
			con->cursor_row--;
		}
	}
	fb_change = 1;

	return i;
}

void CON_Init(void *framebuffer,int xstart,int ystart,int xres,int yres,int stride)
{
	__console_init(framebuffer,xstart,ystart,xres,yres,stride);
}


/*
void _con_free_bg_buf()
{
	if(_bg_buffer) free(_bg_buffer);
	_bg_buffer = NULL;
	if(_c_buffer) free(_c_buffer);
	_c_buffer = NULL;
}
*/

/*
void _con_print_buf()
{
	printf("\ncon_buf: %p\n", _console_buffer);
	printf("_bg_buf: %p\n", _bg_buffer);
	printf("_c_buf:  %p\n", _c_buffer);
}
*/

void _con_alloc_buf(s32 *conW, s32 *conH)
{
	int w = 640;
	int h = 480;
	int size;

	if (conW && *conW > w) *conW = w;
	if (conH && *conH > h) *conH = h;
	if (_console_buffer) return;

	size = w * h * VI_DISPLAY_PIX_SZ;
	_c_buffer_size = sizeof(struct _c1) * (w / FONT_XSIZE) * (h / FONT_YSIZE);
	_console_buffer = LARGE_memalign(32, size);
	//_bg_buffer = LARGE_memalign(32, size);
	_c_buffer = LARGE_memalign(32, _c_buffer_size);
}

s32 CON_InitEx(GXRModeObj *rmode, s32 conXOrigin,s32 conYOrigin,s32 conWidth,s32 conHeight)
{
	VIDEO_SetPostRetraceCallback(NULL);

	_con_alloc_buf(&conWidth, &conHeight);

	/*
	if(_console_buffer)
		free(_console_buffer);
	
	_console_buffer = malloc(conWidth*conHeight*VI_DISPLAY_PIX_SZ);
	if(!_console_buffer) return -1;

	_con_free_bg_buf();
	*/
	_tr_enable = 0;

	__console_init_ex(_console_buffer,conXOrigin,conYOrigin,rmode->fbWidth*VI_DISPLAY_PIX_SZ,conWidth,conHeight,conWidth*VI_DISPLAY_PIX_SZ);

	return 0;
}

s32 CON_InitTr(GXRModeObj *rmode, s32 conXOrigin,s32 conYOrigin,s32 conWidth,s32 conHeight, s32 bgColor)
{
	VIDEO_SetPostRetraceCallback(NULL);

	_con_alloc_buf(&conWidth, &conHeight);
	/*
	if(_console_buffer) free(_console_buffer);
	_console_buffer = malloc(conWidth*conHeight*VI_DISPLAY_PIX_SZ);
	if(!_console_buffer) return -1;

	_con_free_bg_buf();

	_bg_buffer = malloc(conWidth*conHeight*VI_DISPLAY_PIX_SZ);
	if(!_bg_buffer) return -1;

	_c_buffer_size = sizeof(struct _c1) * (conWidth / FONT_XSIZE) * (conHeight/FONT_YSIZE);
	_c_buffer = malloc(_c_buffer_size);
	if(!_c_buffer) return -1;
	*/
	memset(_c_buffer, 0, _c_buffer_size);

	if (bgColor < 0 || bgColor > 15) bgColor = 0;
	_bg_color = color_table[bgColor];

	_tr_enable = 1;

	__console_init_ex(_console_buffer,conXOrigin,conYOrigin,rmode->fbWidth*VI_DISPLAY_PIX_SZ,conWidth,conHeight,conWidth*VI_DISPLAY_PIX_SZ);

	return 0;
}

void CON_GetMetrics(int *cols, int *rows)
{
	if(curr_con) {
		*cols = curr_con->con_cols;
		*rows = curr_con->con_rows;
	}
}

void CON_GetPosition(int *col, int *row)
{
	if(curr_con) {
		*col = curr_con->cursor_col;
		*row = curr_con->cursor_row;
	}
}

void CON_EnableGecko(int channel,int safe)
{
	if(channel && (channel>1 || !usb_isgeckoalive(channel))) channel = -1;

	__gecko_status = channel;
	__gecko_safe = safe;

	if(__gecko_status!=-1) {
		devoptab_list[STD_OUT] = &dotab_stdout;
		devoptab_list[STD_ERR] = &dotab_stdout;

		// line buffered output for threaded apps when only using the usbgecko
		if(!curr_con) {
			setvbuf(stdout, NULL, _IOLBF, 0);
			setvbuf(stderr, NULL, _IOLBF, 0);
		}
	}
}

