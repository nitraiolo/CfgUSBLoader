#include <stdio.h>
#include <ogcsys.h>
#include <stdlib.h>
#include <malloc.h>

#include "sys.h"
#include "video.h"
#include "cfg.h"

#include "png.h"

/* Video variables */
static void *framebuffer0 = NULL;
static void *framebuffer1 = NULL;
static void *framebuffer = NULL;
static int framebuffer_size;
static GXRModeObj *vmode = NULL;
static GXRModeObj vmode_non_wide;
static int video_wide;

void *bg_buf_rgba = NULL;
void *bg_buf_ycbr = NULL;

int con_inited = 0;

void _Con_Clear(void);

void Con_Init(u32 x, u32 y, u32 w, u32 h)
{
	/* Create console in the framebuffer */
	if (CFG.console_transparent) {
		CON_InitTr(vmode, x, y, w, h, CONSOLE_BG_COLOR);
	} else {
		CON_InitEx(vmode, x, y, w, h);
	}
	_Con_Clear();
	con_inited = 1;
}

void _Con_Clear(void)
{
	DefaultColor();
	/* Clear console */
	printf("\x1b[2J");
	fflush(stdout);
}

void Con_Clear(void)
{
	__console_flush(0);
	_Con_Clear();
}

void Con_ClearLine(void)
{
	s32 cols, rows;
	u32 cnt;

	printf("\r");
	fflush(stdout);

	/* Get console metrics */
	CON_GetMetrics(&cols, &rows);

	/* Erase line */
	for (cnt = 1; cnt < cols; cnt++) {
		printf(" ");
		fflush(stdout);
	}

	printf("\r");
	fflush(stdout);
}

void Con_FgColor(u32 color, u8 bold)
{
	/* Set foreground color */
	printf("\x1b[%u;%um", color + 30, bold);
	fflush(stdout);
}

void Con_BgColor(u32 color, u8 bold)
{
	/* Set background color */
	printf("\x1b[%u;%um", color + 40, bold);
	fflush(stdout);
}

void Con_SetPosition(int col, int row)
{
	/* Move to specified pos */
	printf("\x1b[%u;%uH", row, col);
	fflush(stdout);
}

void Con_FillRow(u32 row, u32 color, u8 bold)
{
	s32 cols, rows;
	u32 cnt;

	/* Set color */
	printf("\x1b[%u;%um", color + 40, bold);
	fflush(stdout);

	/* Get console metrics */
	CON_GetMetrics(&cols, &rows);

	/* Save current row and col */
	printf("\x1b[s");
	fflush(stdout);

	/* Move to specified row */
	printf("\x1b[%u;0H", row);
	fflush(stdout);

	/* Fill row */
	for (cnt = 0; cnt < cols; cnt++) {
		printf(" ");
		fflush(stdout);
	}

	/* Load saved row and col */
	printf("\x1b[u");
	fflush(stdout);

	/* Set default color */
	Con_BgColor(0, 0);
	Con_FgColor(7, 1);
}

void Video_Configure(GXRModeObj *rmode)
{
	/* Configure the video subsystem */
	VIDEO_Configure(rmode);

	/* Setup video */
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if (rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
}

// allocate a MAX size frambuffer, so that it
// can accomodate all video mode changes
void *Video_Allocate_MAX_Framebuffer(GXRModeObj *vmode, bool mem2)
{
	int w, h;
	void *buf;
	w = vmode->fbWidth;
	h = MAX(vmode->xfbHeight, VI_MAX_HEIGHT_PAL);
	framebuffer_size = VIDEO_PadFramebufferWidth(w) * h * VI_DISPLAY_PIX_SZ;
	if (mem2) {
		buf = LARGE_memalign(32, framebuffer_size);
	} else {
		buf = memalign(32, framebuffer_size);
	}
	if (buf) {
		memset(buf, 0, framebuffer_size);
		DCFlushRange(buf, framebuffer_size);
	}
	return buf;
}


void Video_SetWide()
{
	if (CFG.widescreen && video_wide) return;
	if (!CFG.widescreen && !video_wide) return;
	if (!vmode) return;
	// change required
	if (CFG.widescreen) {
		vmode->viWidth = 678;
		vmode->viXOrigin = ((VI_MAX_WIDTH_NTSC - 678) / 2);
	} else {
		*vmode = vmode_non_wide;
	}
	video_wide = CFG.widescreen;
	Video_Configure(vmode);
}

void Video_SetMode(void)
{
	if (vmode) return; // already set?!

	/* Select preferred video mode */
	vmode = VIDEO_GetPreferredMode(NULL);

	// save non-wide vmode
	vmode_non_wide = *vmode;

	// widescreen mode
	video_wide = CONF_GetAspectRatio();
	if (video_wide) {
		vmode->viWidth = 678;
		vmode->viXOrigin = ((VI_MAX_WIDTH_NTSC - 678) / 2);
	}

	/* Allocate memory for the framebuffer */
	//framebuffer0 = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
	//framebuffer1 = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
	framebuffer0 = MEM_K0_TO_K1(Video_Allocate_MAX_Framebuffer(vmode, true));
	framebuffer1 = MEM_K0_TO_K1(Video_Allocate_MAX_Framebuffer(vmode, true));
	framebuffer = framebuffer0;

	/* Clear the screen */
	Video_Clear(COLOR_BLACK);

	// Set Next framebuffer
	VIDEO_SetNextFramebuffer(framebuffer);

	/* Configure the video subsystem */
	Video_Configure(vmode);

	// VIDEO_WaitVSync This is necessary!
	// Otherwise the VIDEO_GetCurrentFramebuffer()
	// returns an invalid ptr before an actual vsync happens
	// so __console_flush can crash
	VIDEO_WaitVSync();
}

void Video_Close()
{
	void *fb = Video_Allocate_MAX_Framebuffer(vmode, false);
	VIDEO_SetBlack(TRUE);
	if (fb) {
		framebuffer = MEM_K0_TO_K1(fb);
		Video_Clear(COLOR_BLACK);
		VIDEO_SetNextFramebuffer(framebuffer);
	}
	VIDEO_Flush();
	VIDEO_WaitVSync();
}

void Video_Clear(s32 color)
{
	VIDEO_ClearFrameBuffer(vmode, framebuffer, color);
}

void Video_DrawPng(IMGCTX ctx, PNGUPROP imgProp, u16 x, u16 y)
{
	PNGU_DECODE_TO_COORDS_YCbYCr(ctx, x, y, imgProp.imgWidth, imgProp.imgHeight, vmode->fbWidth, vmode->xfbHeight, framebuffer);
}

GXRModeObj* _Video_GetVMode()
{
	return vmode;
}

void* _Video_GetFB(int n)
{
	switch(n) {
		case 0: return framebuffer0;
		case 1: return framebuffer1;
	}
	return framebuffer;
}

void _Video_SetFB(void *fb)
{
	framebuffer = fb;
}

void _Video_SyncFB()
{
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (framebuffer != framebuffer0) {
		memcpy(framebuffer0, framebuffer, framebuffer_size);
		framebuffer = framebuffer0;
		VIDEO_SetNextFramebuffer(framebuffer);
		VIDEO_Flush();
		VIDEO_WaitVSync();
	}
}

void FgColor(int color)
{
	Con_FgColor(color & 7, color > 7 ? 1 : 0);
}

void BgColor(int color)
{
	Con_BgColor(color & 7, color > 7 ? 1 : 0);
}

void DefaultColor()
{
	FgColor(CONSOLE_FG_COLOR);
	BgColor(CONSOLE_BG_COLOR);
}

#define BACKGROUND_WIDTH	640
#define BACKGROUND_HEIGHT	480

/** (from GRRLIB)
 * Make a PNG screenshot on the SD card.
 * @return True if every thing worked, false otherwise.
 */
bool ScreenShot(char *fname)
{
    IMGCTX pngContext;
    int ErrorCode = -1;
	void *fb = VIDEO_GetCurrentFramebuffer();

    if((pngContext = PNGU_SelectImageFromDevice(fname)))
    {
        ErrorCode = PNGU_EncodeFromYCbYCr(pngContext,
				BACKGROUND_WIDTH, BACKGROUND_HEIGHT, fb, 0);
        PNGU_ReleaseImageContext(pngContext);
    }
    return !ErrorCode;
}

void Video_DrawBg()
{
	//Video_DrawRGBA(0, 0, bg_buf_rgba, vmode->fbWidth, vmode->xfbHeight);
	//Video_DrawRGBA(0, 0, bg_buf_rgba, BACKGROUND_WIDTH, BACKGROUND_HEIGHT);
	memcpy(framebuffer, bg_buf_ycbr, BACKGROUND_WIDTH * BACKGROUND_HEIGHT * 2);
}

int Video_AllocBg()
{
	if (bg_buf_rgba == NULL) {
		bg_buf_rgba = LARGE_memalign(32, BACKGROUND_WIDTH * BACKGROUND_HEIGHT * 4);
		if (!bg_buf_rgba) return -1;
	}

	if (bg_buf_ycbr == NULL) {
		bg_buf_ycbr = LARGE_memalign(32, BACKGROUND_WIDTH * BACKGROUND_HEIGHT * 2);
		if (!bg_buf_ycbr) return -1;
	}
	return 0;
}

void Video_SaveBgRGBA()
{
	RGBA_to_YCbYCr(bg_buf_ycbr, bg_buf_rgba, BACKGROUND_WIDTH, BACKGROUND_HEIGHT);
}

s32 Video_SaveBg(void *img)
{
	IMGCTX   ctx = NULL;
	PNGUPROP imgProp;

	s32 ret;

	/* Select PNG data */
	ctx = PNGU_SelectImageFromBuffer(img);
	if (!ctx) {
		ret = -1;
		goto out;
	}

	/* Get image properties */
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK) {
		ret = -1;
		goto out;
	}

	Video_AllocBg();

	/* Decode image */
	//PNGU_DECODE_TO_COORDS_RGBA8(ctx, 0, 0, imgProp.imgWidth, imgProp.imgHeight, 255,
	//		vmode->fbWidth, vmode->xfbHeight, bg_buf_rgba);
	PNGU_DECODE_TO_COORDS_RGBA8(ctx, 0, 0, imgProp.imgWidth, imgProp.imgHeight, 255,
			BACKGROUND_WIDTH, BACKGROUND_HEIGHT, bg_buf_rgba);

	Video_SaveBgRGBA();

	/* Success */
	ret = 0;

out:
	/* Free memory */
	if (ctx)
		PNGU_ReleaseImageContext(ctx);

	return ret;
}

void Video_GetBG(int x, int y, char *img, int width, int height)
{
	int ix, iy;
	char *c, *bg;
	if (!bg_buf_rgba) return;
	c = img;
	for (iy=0; iy<height; iy++) {
		bg = (char*)bg_buf_rgba + (y+iy)*(vmode->fbWidth * 4) + x*4;
		for (ix=0; ix<width; ix++) {
			*((int*)c) = *((int*)bg);
			c += 4;
			bg += 4;
		}
	}
}

// destination: img
void Video_CompositeRGBA(int x, int y, char *img, int width, int height)
{
	int ix, iy;
	char *c, *bg, a;
	if (!bg_buf_rgba) return;
	c = img;
	for (iy=0; iy<height; iy++) {
		bg = (char*)bg_buf_rgba + (y+iy)*(vmode->fbWidth * 4) + x*4;
		for (ix=0; ix<width; ix++) {
			a = c[3];
			png_composite(c[0], c[0], a, bg[0]); // r
			png_composite(c[1], c[1], a, bg[1]); // g
			png_composite(c[2], c[2], a, bg[2]); // b
			c += 4;
			bg += 4;
		}
	}
}

void RGBA_to_YCbYCr(char *dest, char *img, int width, int height)
{
	int ix, iy, buffWidth;
	char *ip = img;
	buffWidth = width / 2;
	for (iy=0; iy<height; iy++) {
		for (ix=0; ix<width/2; ix++) {
			((PNGU_u32 *)dest)[iy*buffWidth+ix] =
			   	PNGU_RGB8_TO_YCbYCr (ip[0], ip[1], ip[2], ip[4], ip[5], ip[6]);
			ip += 8;
		}
	}
}

void Video_DrawRGBA(int x, int y, char *img, int width, int height)
{
	int ix, iy, buffWidth;
	char *ip = img;
	buffWidth = vmode->fbWidth / 2;
	for (iy=0; iy<height; iy++) {
		for (ix=0; ix<width/2; ix++) {
			((PNGU_u32 *)framebuffer)[(y+iy)*buffWidth+x/2+ix] =
			   	PNGU_RGB8_TO_YCbYCr (ip[0], ip[1], ip[2], ip[4], ip[5], ip[6]);
			ip += 8;
		}
	}
}


