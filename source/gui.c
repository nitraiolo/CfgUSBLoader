
// Modified by oggzee & usptactical

#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "gui.h"
#include "sys.h"
#include "fat.h"
#include "video.h"
#include "cache.h"
#include "cfg.h"
#include "grid.h"
#include "coverflow.h"
#include "menu.h"
#include "console.h"
#include "png.h"
#include "gettext.h"
#include "sort.h"
#include "wgui.h"
#include "guimenu.h"

#include "intro4_jpg.h"

extern void *bg_buf_rgba;
extern void *bg_buf_ycbr;
extern bool imageNotFound;
extern int page_gi;

int gui_mode = 0;
int gui_style = GUI_STYLE_GRID;

bool loadGame;
bool suppressCoverDrawing;

/* Constants */
int COVER_WIDTH = 160;
int COVER_HEIGHT = 225;

#define SAFE_PNGU_RELEASE(CTX) if(CTX){PNGU_ReleaseImageContext(CTX);CTX=NULL;}

static int grr_init = 0;
static int grx_init = 0;
static int prev_cover_style = CFG_COVER_STYLE_2D;
static int prev_cover_height;
static int prev_cover_width;

int game_select = -1;

s32 __Gui_DrawPngA(void *img, u32 x, u32 y);
s32 __Gui_DrawPngRA(void *img, u32 x, u32 y, int width, int height);
void CopyRGBA(char *src, int srcWidth, int srcHeight,
		char *dest, int dstWidth, int dstHeight,
		int srcX, int srcY, int dstX, int dstY, int w, int h);
void CompositeRGBA(char *bg_buf, int bg_w, int bg_h,
		int x, int y, char *img, int width, int height);
void ResizeRGBA(char *img, int imgWidth, int imgHeight,
	   char *resize, int width, int height);
void RGB_to_RGBA(const unsigned char *src, unsigned char *dst,
		const int width, const int height, unsigned char alpha);
void* Load_JPG_RGB(const unsigned char my_jpg[], int *w, int *h);

// FLOW_Z camera
float cam_f = 0.0f;
float cam_dir = 1.0;
float cam_z = -578.0F;
guVector cam_look = {319.5F, 239.5F, 0.0F};

//AA
GRRLIB_texImg aa_texBuffer[4];

//Mtx GXmodelView2D;
extern unsigned char bgImg[];
//extern unsigned char bgImg_wide[];
extern unsigned char bg_gui[];
extern unsigned char introImg2[];
extern unsigned char introImg3[];
extern unsigned char introImg41[];
extern unsigned char coverImg[];
extern unsigned char coverImg_full[];
//extern unsigned char coverImg_wide[];
extern unsigned char pointer[];
extern unsigned char hourglass[];
extern unsigned char star_icon[];
extern unsigned char gui_font[];

GRRLIB_texImg tx_bg;
GRRLIB_texImg tx_bg_con;
GRRLIB_texImg tx_nocover;
GRRLIB_texImg tx_nocover_full;
extern GRRLIB_texImg tx_hourglass_full; // coverflow

GRRLIB_texImg tx_pointer;
GRRLIB_texImg tx_hourglass;
GRRLIB_texImg tx_star;
GRRLIB_texImg tx_font;
GRRLIB_texImg tx_font_clock;

s32 __Gui_GetPngDimensions(void *img, u32 *w, u32 *h)
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

	/* Set image width and height */
	*w = imgProp.imgWidth;
	*h = imgProp.imgHeight;

	/* Success */
	ret = 0;

out:
	/* Free memory */
	if (ctx)
		PNGU_ReleaseImageContext(ctx);

	return ret;
}

s32 __Gui_DrawPng(void *img, u32 x, u32 y)
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

	/* Draw image */
	Video_DrawPng(ctx, imgProp, x, y);

	/* Success */
	ret = 0;

out:
	/* Free memory */
	if (ctx)
		PNGU_ReleaseImageContext(ctx);

	return ret;
}


void Gui_InitConsole(void)
{
	/* Initialize console */
	Con_Init(CONSOLE_XCOORD, CONSOLE_YCOORD, CONSOLE_WIDTH, CONSOLE_HEIGHT);
}

void Gui_Console_Enable(void)
{
	if (!__console_disable) return;
	Video_DrawBg();
	// current frame buffer
	__console_enable(_Video_GetFB(-1));
}

IMGCTX Gui_OpenPNG(void *img, int *w, int *h)
{
	IMGCTX ctx = NULL;
	PNGUPROP imgProp;
	int ret;

	// Select PNG data
	ctx = PNGU_SelectImageFromBuffer(img);
	if (!ctx) {
		return NULL;
	}

	// Get image properties
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK
		|| imgProp.imgWidth == 0
		|| imgProp.imgWidth > 1024
		|| imgProp.imgHeight == 0
		|| imgProp.imgHeight > 1024)
	{
		PNGU_ReleaseImageContext(ctx);
		return NULL;
	}

	if (w) *w = imgProp.imgWidth;
	if (h) *h = imgProp.imgHeight;

	return ctx;
}

void* Gui_DecodePNG(IMGCTX ctx)
{
	PNGUPROP imgProp;
	void *img_buf = NULL;
	int ret;

	if (ctx == NULL) return NULL;

	// Get image properties
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK) {
		return NULL;
	}
	// alloc
	img_buf = memalign(32, imgProp.imgWidth * imgProp.imgHeight * 4);
	if (img_buf == NULL) {
		return NULL;
	}
	// decode
	ret = PNGU_DECODE_TO_COORDS_RGBA8(ctx, 0, 0,
			imgProp.imgWidth, imgProp.imgHeight, 255,
			imgProp.imgWidth, imgProp.imgHeight, img_buf);
	return img_buf;
}

int Gui_DecodePNG_scale_to(IMGCTX ctx, void *img_buf, int dest_w, int dest_h)
{
	PNGUPROP imgProp;
	int width, height;
	int ret;

	if (ctx == NULL) return -1;
	// Get image properties
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK) return -1;
	width = imgProp.imgWidth;
	height = imgProp.imgHeight;

	if (width != dest_w || height != dest_h) {
		// fix size
		char *resize_buf = NULL;
		// decode
		resize_buf = Gui_DecodePNG(ctx);
		if (!resize_buf) return -1;
		// resize
		ResizeRGBA(resize_buf, width, height, img_buf, dest_w, dest_h);
		// free
		SAFE_FREE(resize_buf);
	} else {
		// Decode image
		PNGU_DECODE_TO_COORDS_RGBA8(ctx, 0, 0, width, height, 255,
			dest_w, dest_h, img_buf);
	}
	return 0;
}

int Gui_DecodePNG_scaleBG_to(IMGCTX ctx, void *img_buf)
{
	PNGUPROP imgProp;
	int width, height;
	int ret;

	if (ctx == NULL) return -1;

	// Get image properties
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK) return -1;
	width = imgProp.imgWidth;
	height = imgProp.imgHeight;

	if (height != BACKGROUND_HEIGHT || width < BACKGROUND_WIDTH) {
		return -1;
	}

	if (width > BACKGROUND_WIDTH) {
		// fix size
		char *resize_buf = NULL;
		// decode
		resize_buf = Gui_DecodePNG(ctx);
		if (!resize_buf) {
			return -1;
		}
		if (CFG.widescreen) {
			// resize
			ResizeRGBA(resize_buf, width, height,
				   img_buf, BACKGROUND_WIDTH, BACKGROUND_HEIGHT);
		} else {
			// crop
			int x = (width - BACKGROUND_WIDTH) / 2;
			if (x<0) x = 0;
			CopyRGBA(resize_buf, width, height,
					img_buf, BACKGROUND_WIDTH, BACKGROUND_HEIGHT,
					x, 0, 0, 0, BACKGROUND_WIDTH, BACKGROUND_HEIGHT);
		}
		// free
		SAFE_FREE(resize_buf);
	} else {
		// Decode image
		PNGU_DECODE_TO_COORDS_RGBA8(ctx, 0, 0, width, height, 255,
			BACKGROUND_WIDTH, BACKGROUND_HEIGHT, img_buf);
	}
	return 0;
}

void* Gui_AllocBG()
{
	return memalign(32, BACKGROUND_WIDTH * BACKGROUND_HEIGHT * 4);
}

void *Gui_DecodePNG_scaleBG(IMGCTX ctx)
{
	void *img_buf = NULL;
	int ret;

	if (ctx == NULL) return NULL;

	img_buf = Gui_AllocBG();
	if (img_buf == NULL) return NULL;

	ret = Gui_DecodePNG_scaleBG_to(ctx, img_buf);
	if (ret) {
		SAFE_FREE(img_buf);
		return NULL;
	}
	return img_buf;
}

int Gui_LoadBG_data_to(void *data, void *dest_buf)
{
	IMGCTX ctx = NULL;
	int ret;

	ctx = Gui_OpenPNG(data, NULL, NULL);
	if (ctx == NULL) {
		return -1;
	}
	ret = Gui_DecodePNG_scaleBG_to(ctx, dest_buf);
	PNGU_ReleaseImageContext(ctx);
	return ret;
}

void* Gui_LoadBG_data(void *data)
{
	void *img_buf;
	int ret;

	img_buf = Gui_AllocBG();
	if (img_buf == NULL) return NULL;
	ret = Gui_LoadBG_data_to(data, img_buf);
	if (ret) {
		SAFE_FREE(img_buf);
	}
	return img_buf;
}

int Gui_LoadBG_to(char *path, void *dest_buf)
{
	void *data = NULL;
	int ret = -1;

	ret = Fat_ReadFile(path, &data);
	if (ret <= 0 || data == NULL) {
		return -1;
	}
	ret = Gui_LoadBG_data_to(data, dest_buf);
	SAFE_FREE(data);
	return ret;
}

void* Gui_LoadBG(char *path)
{
	void *data = NULL;
	void *img_buf = NULL;
	int ret = -1;

	ret = Fat_ReadFile(path, &data);
	if (ret <= 0 || data == NULL) {
		return NULL;
	}
	img_buf = Gui_LoadBG_data(data);
	SAFE_FREE(data);
	return img_buf;
}

int Gui_OverlayBg(char *path, void *bg_buf)
{
	void *img_buf = NULL;
	img_buf = Gui_LoadBG(path);
	if (img_buf == NULL) {
		return -1;
	}
	// overlay
	CompositeRGBA(bg_buf, BACKGROUND_WIDTH, BACKGROUND_HEIGHT,
			0, 0, img_buf, BACKGROUND_WIDTH, BACKGROUND_HEIGHT);
	SAFE_FREE(img_buf);
	return 0;
}

void Gui_LoadBackground(void)
{
	s32 ret = -1;

	Video_AllocBg();

	// Try to open background image from SD
	if (*CFG.background) {
		ret = Gui_LoadBG_to(CFG.background, bg_buf_rgba);
	}
	// if failed, use builtin
	if (ret) {
		ret = Gui_LoadBG_data_to(bgImg, bg_buf_rgba);
	}

	// Overlay
	char path[200];
	ret = -1;

	if (CFG.widescreen) {
		snprintf(D_S(path), "%s/bg_overlay_w.png", CFG.theme_path);
		ret = Gui_OverlayBg(path, bg_buf_rgba);
	}
	if (ret) {
		snprintf(D_S(path), "%s/bg_overlay.png", CFG.theme_path);
		ret = Gui_OverlayBg(path, bg_buf_rgba);
	}

	// save background to ycbr
	Video_SaveBgRGBA();
} 

void Gui_DrawBackground(void)
{
	if (CFG.direct_launch) return;

	// Load background
	Gui_LoadBackground();

	// Draw background
	Video_DrawBg();
}

void Gui_DrawIntro(void)
{
	GXRModeObj *rmode = _Video_GetVMode();
	int con_x = 32;
	int con_y = 16;
	int con_w = 640-32*2;
	int con_h = 480-16*2;
	int w = con_w/8; // 72
	int h = con_h/16; // 28
	int i, x, y;
	char *title = "Configurable USB Loader";
	void *img_buf = NULL;
	int i41 = 0;

	time_t t = time(NULL);
	struct tm *ti = localtime(&t);
	// tm_mon starts at 0
	// tm_mday starts at 1
	if ((ti->tm_mon == 3 && ti->tm_mday == 1) || CFG.intro == 41) {
		CFG.intro = 2;
		i41 = 1;
	} else {
		if (CFG.direct_launch && CFG.intro==0) {
			CON_InitEx(rmode, con_x, con_y, con_w, con_h);
			__console_disable = 1;
			return;
		}
	}

	if (CFG.intro == 1) {
		CON_InitEx(rmode, con_x, con_y, con_w, con_h);
		Con_SetPosition((w-strlen(title))/2, h/2);
		printf("%s", title);
		Con_SetPosition(0,0);
		goto print_ver;
	}
	
	// Draw the intro image
	//VIDEO_WaitVSync();
	//__Gui_DrawPng(introImg, 0, 0);

	get_time(&TIME.intro1);
	
	IMGCTX ctx = NULL;
	img_buf = memalign(32, rmode->fbWidth * rmode->xfbHeight * 4);
	if (!img_buf) return;

	if (CFG.intro == 4) {
		int imgw, imgh;
		void *rgb = NULL;
		void *rgba = NULL;
		rgb = Load_JPG_RGB(intro4_jpg, &imgw, &imgh);
		if (!rgb) goto out;
		if (imgw == rmode->fbWidth && imgh == rmode->xfbHeight) {
			RGB_to_RGBA(rgb, img_buf, imgw, imgh, 255);
		} else {
			rgba = memalign(32, imgw * imgh * 4);
			RGB_to_RGBA(rgb, rgba, imgw, imgh, 255);
			ResizeRGBA(rgba, imgw, imgh, img_buf, rmode->fbWidth, rmode->xfbHeight);
			SAFE_FREE(rgba);
		}
		SAFE_FREE(rgb);
	} else {
		if (CFG.intro == 2) {
			ctx = Gui_OpenPNG(introImg2, NULL, NULL);
		} else {
			ctx = Gui_OpenPNG(introImg3, NULL, NULL);
		}
		if (!ctx) goto out;
		Gui_DecodePNG_scale_to(ctx, img_buf, rmode->fbWidth, rmode->xfbHeight);
		PNGU_ReleaseImageContext(ctx);
		ctx = NULL;
	}
	//
	if (i41) {
		ctx = Gui_OpenPNG(introImg41, NULL, NULL);
		if (ctx) {
			PNGUPROP imgProp;
			int x, y;
			PNGU_GetImageProperties(ctx, &imgProp);
			x = rmode->fbWidth / 2 - imgProp.imgWidth / 2;
			y = rmode->xfbHeight / 2 - imgProp.imgHeight / 2;
			PNGU_DECODE_TO_COORDS_RGBA8(ctx, x, y,
				imgProp.imgWidth, imgProp.imgHeight, 255,
				rmode->fbWidth, rmode->xfbHeight, img_buf);
			PNGU_ReleaseImageContext(ctx);
			ctx = NULL;
		}
	}
	//
	//VIDEO_WaitVSync();
	Video_DrawRGBA(0, 0, img_buf, rmode->fbWidth, rmode->xfbHeight);

out:
	get_time(&TIME.intro2);
	
	Video_AllocBg();
	memcpy(bg_buf_rgba, img_buf, BACKGROUND_WIDTH * BACKGROUND_HEIGHT * 4);
	memcpy(bg_buf_ycbr, _Video_GetFB(-1), BACKGROUND_WIDTH * BACKGROUND_HEIGHT * 2);

	//CON_InitTr(rmode, 552, 424, 88, 48, CONSOLE_BG_COLOR);
	CON_InitTr(rmode, con_x, con_y, con_w, con_h, CONSOLE_BG_COLOR);
	Con_Clear();
print_ver:
	x = w - 12; // 60
	y = h - 3; // 25
	for (i=0;i<y;i++) printf("\n");
	printf("%*s", x, "");
	printf("v%s\n", CFG_VERSION);
	__console_flush(0);
	
	//VIDEO_WaitVSync();
	SAFE_FREE(img_buf);
}

// return 0 on success -1 on error
int find_cover_path(u8 *id, int cover_style, char *path, int size, struct stat *st)
{
	s32 ret = -1;
	char *coverdir;
	struct stat st1;
	if (!st) st = &st1;
	if (!id || !*id) return -1;
	if (path) *path = 0;
	memset(st, 0, sizeof(*st));
	coverdir = cfg_get_covers_path(cover_style);
L_retry:
	/* Generate cover filepath for full covers */
	snprintf(path, size, "%s/%.6s.png", coverdir, id);
	/* stat cover */
	ret = stat(path, st);
	if (ret != 0) {
		// if 6 character id not found, try 4 character id
		snprintf(path, size, "%s/%.4s.png", coverdir, id);
		ret = stat(path, st);
	}
	if (ret != 0) {
		if (cover_style == CFG_COVER_STYLE_2D && coverdir == CFG.covers_path) {
			coverdir = CFG.covers_path_2d;
			goto L_retry;
		}
	}
	return ret;
}

/**
 * Tries to read the game cover image from disk.  This method uses the passed
 *  in cover_style value to determine which file path to use.  
 * 
 *  @param *discid the disk id of the cover to load
 *  @param **p_imgData the image
 *  @param load_noimage bool representing if the noimage png should be loaded
 *  @param cover_style represents the cover style to load
 *  @param *path for returning the full image path - doesn't return builtin image path
 *  @return int
 */
int Gui_LoadCover_style(u8 *discid, void **p_imgData, bool load_noimage, int cover_style, char *path)
{
	s32 ret = -1;
	char filepath[200];
	char *coverpath;

	if (path) *path = 0;
	imageNotFound = true;
	ret = find_cover_path(discid, cover_style, filepath, sizeof(filepath), NULL);
	if (ret == 0) {
		ret = Fat_ReadFile(filepath, p_imgData);
	}
	if (ret > 0) {
		imageNotFound = false;
		if (path) strcopy(path, filepath, 200);
	}
	if (ret <= 0 && load_noimage) {
		coverpath = cfg_get_covers_path(cover_style);
		L_retry2:
		snprintf(filepath, sizeof(filepath), "%s/noimage.png", coverpath);
		ret = Fat_ReadFile(filepath, p_imgData);
		if (ret <= 0) {
			if (cover_style == CFG_COVER_STYLE_2D && coverpath == CFG.covers_path) {
				coverpath = CFG.covers_path_2d;
				goto L_retry2;
			}
		}
	}
	return ret;
}


void DrawCoverImg(u8 *discid, char *img_buf, int width, int height)
{
	char *bg_buf = NULL;
	char *star_buf = NULL;
	void *fav_buf = NULL;
	IMGCTX ctx = NULL;
	s32 ret;

	// get background
	bg_buf = memalign(32, width * height * 4);
	if (!bg_buf) return;
	Video_GetBG(COVER_XCOORD, COVER_YCOORD, bg_buf, width, height);
	// composite img to bg
	CompositeRGBA(bg_buf, width, height, 0, 0,
				img_buf, width, height);
	// favorite?
	if (is_favorite(discid)) {
		// star
		PNGUPROP imgProp;
		// Select PNG data
		ret = Load_Theme_Image("favorite.png", &fav_buf);
		if (ret == 0 && fav_buf) {
			ctx = PNGU_SelectImageFromBuffer(fav_buf);
		} else {
			ctx = PNGU_SelectImageFromBuffer(star_icon);
		}
		if (!ctx) goto out;
		// Get image properties
		ret = PNGU_GetImageProperties(ctx, &imgProp);
		if (ret != PNGU_OK) goto out;
		if (imgProp.imgWidth > width || imgProp.imgHeight > height) goto out;
		// alloc star img buf
		star_buf = memalign(32, imgProp.imgWidth * imgProp.imgHeight * 4);
		if (!star_buf) goto out;
		// decode
		ret = PNGU_DECODE_TO_COORDS_RGBA8(ctx, 0, 0,
				imgProp.imgWidth, imgProp.imgHeight, 255,
				imgProp.imgWidth, imgProp.imgHeight, star_buf);
		// composite star
		CompositeRGBA(bg_buf, width, height,
			   	COVER_WIDTH - imgProp.imgWidth, 0, // right upper corner
				star_buf, imgProp.imgWidth, imgProp.imgHeight);
	}

out:
	SAFE_PNGU_RELEASE(ctx);
	SAFE_FREE(fav_buf);
	SAFE_FREE(star_buf);
	// draw
	Video_DrawRGBA(COVER_XCOORD, COVER_YCOORD, bg_buf, width, height);
	SAFE_FREE(bg_buf);
}

void Gui_DrawCover(u8 *discid)
{
	void *builtin = coverImg;
	void *imgData;

	s32  ret, size;

	imageNotFound = true;

	if (CFG.covers == 0) return;

	imgData = builtin;

	ret = Gui_LoadCover_style(discid, &imgData, true, CFG.cover_style, NULL);
	if (ret <= 0) imgData = builtin;

	size = ret;

	u32 width=0, height=0;

	/* Get image dimensions */
	ret = __Gui_GetPngDimensions(imgData, &width, &height);

	/* If invalid image, use default noimage */
	if (ret) {
		imageNotFound = true;
		if (imgData != builtin) free(imgData);
		imgData = builtin;
		ret = __Gui_GetPngDimensions(imgData, &width, &height);
		if (ret) return;
	}

	char *img_buf = NULL;
	char *resize_buf = NULL;
	IMGCTX ctx = NULL;
	img_buf = memalign(32, COVER_WIDTH * COVER_HEIGHT * 4);
	if (!img_buf) { ret = -1; goto out; }

	// Select PNG data
	ctx = PNGU_SelectImageFromBufferX(imgData, size);
	if (!ctx) { ret = -1; goto out; }

	// resize needed?
	if ((width != COVER_WIDTH) || (height != COVER_HEIGHT)) {
		// alloc tmp resize buf
		resize_buf = memalign(32, width * height * 4);
		if (!resize_buf) { ret = -1; goto out; }
		// decode
		ret = PNGU_DECODE_TO_COORDS_RGBA8(ctx, 0, 0,
				width, height, 255, width, height, resize_buf);
		// resize
		ResizeRGBA(resize_buf, width, height, img_buf, COVER_WIDTH, COVER_HEIGHT);
		// discard tmp resize buf
		SAFE_FREE(resize_buf);
	} else {
		// decode
		ret = PNGU_DECODE_TO_COORDS_RGBA8(ctx, 0, 0,
				width, height, 255, width, height, img_buf);
	}

	SAFE_PNGU_RELEASE(ctx);

	DrawCoverImg(discid, img_buf, COVER_WIDTH, COVER_HEIGHT);

out:

	SAFE_FREE(img_buf);
	SAFE_FREE(resize_buf);
	SAFE_PNGU_RELEASE(ctx);

	/* Free memory */
	if (imgData != builtin)
		free(imgData);
}

// return:
// 0: theme image found
// 1: global image found
// -1: not founf
int Load_Theme_Image2(char *name, void **img_buf, bool global)
{
	char path[200];
	void *data = NULL;
	int ret;
   	u32	width, height;

	*img_buf = NULL;
	// try theme-specific file
	snprintf(D_S(path), "%s/%s", CFG.theme_path, name);
	ret = Fat_ReadFile(path, &data);
	if (ret > 0 && data) {
		ret = __Gui_GetPngDimensions(data, &width, &height);
		if (ret == 0) {
			*img_buf = data;
			dbg_printf("img: %s\n", path);
			return 0;
		}
		SAFE_FREE(data);
	}
	if (!global) return -1;
	// failed, try global
	snprintf(D_S(path), "%s/%s", USBLOADER_PATH, name);
	ret = Fat_ReadFile(path, &data);
	if (ret > 0 && data) {
		ret = __Gui_GetPngDimensions(data, &width, &height);
		if (ret == 0) {
			*img_buf = data;
			dbg_printf("img: %s\n", path);
			return 1;
		}
		SAFE_FREE(data);
	}
	// failed
	return -1;
}

int Load_Theme_Image(char *name, void **img_buf)
{
	int ret = Load_Theme_Image2(name, img_buf, true);
	if (ret > 0) ret = 0;
	return ret;
}

s32 __Gui_DrawPngA(void *img, u32 x, u32 y)
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

	char *img_buf;
	img_buf = memalign(32, imgProp.imgWidth * imgProp.imgHeight * 4);
	if (!img_buf) {
		ret = -1;
		goto out;
	}

	// decode
	ret = PNGU_DECODE_TO_COORDS_RGBA8(ctx, 0, 0, imgProp.imgWidth, imgProp.imgHeight, 255,
			imgProp.imgWidth, imgProp.imgHeight, img_buf);
	// combine
	Video_CompositeRGBA(x, y, img_buf, imgProp.imgWidth, imgProp.imgHeight);
	// draw
	Video_DrawRGBA(x, y, img_buf, imgProp.imgWidth, imgProp.imgHeight);

	SAFE_FREE(img_buf);

	/* Success */
	ret = 0;

out:
	/* Free memory */
	if (ctx)
		PNGU_ReleaseImageContext(ctx);

	return ret;
}

void CopyRGBA(char *src, int srcWidth, int srcHeight,
		char *dest, int dstWidth, int dstHeight,
		int srcX, int srcY, int dstX, int dstY, int w, int h)
{
	int x, y;
	for (y=0; y<h; y++) {
		for (x=0; x<w; x++) {
			int sx = srcX + x;
			int sy = srcY + y;
			int dx = dstX + x;
			int dy = dstY + y;
			if (sx < srcWidth && sy < srcHeight
				&& dx < dstWidth && dy < dstHeight)
			{
				((int*)dest)[dy*dstWidth + dx] = ((int*)src)[sy*srcWidth + sx];
			}
		}
	}
}

void PadRGBA(char *img, int imgWidth, int imgHeight,
	   char *resize, int width, int height)
{
	int x, y;
	for (y=0; y<height; y++) {
		for (x=0; x<width; x++) {
			if (x < imgWidth && y < imgHeight) {
				((int*)resize)[y*width + x] = ((int*)img)[y*imgWidth + x];
			} else {
				((int*)resize)[y*width + x] = 0x00000000;
			}
		}
	}
}

void ResizeRGBA1(char *img, int imgWidth, int imgHeight,
	   char *resize, int width, int height)
{
	int x, y, ix, iy;
	for (y=0; y<height; y++) {
		for (x=0; x<width; x++) {
			ix = x * imgWidth / width;
			iy = y * imgHeight / height;
			((int*)resize)[y*width + x] =
				((int*)img)[iy*imgWidth + ix];
		}
	}
}

// resize img buf to resize buf
void ResizeRGBA(char *img, int imgWidth, int imgHeight,
	   char *resize, int width, int height)
{
	u32 x, y, ix, iy, i;
	u32 rx, ry, nx, ny;
	u32 fix, fnx, fiy, fny;

	for (y=0; y<height; y++) {
		for (x=0; x<width; x++) {
			rx = (x << 8) * imgWidth / width;
			fnx = rx & 0xff;
			fix = 0x100 - fnx;
			ix = rx >> 8;
			nx = ix+1;
			if (nx >= imgWidth) nx = imgWidth - 1;

			ry = (y << 8) * imgHeight / height;
			fny = ry & 0xff;
			fiy = 0x100 - fny;
			iy = ry >> 8;
			ny = iy+1;
			if (ny >= imgHeight) ny = imgHeight - 1;

			// source pixel pointers (syx i=index n=next)
			u8 *rp  = (u8*)&((int*)resize)[y*width + x];
			u8 *sii = (u8*)&((int*)img)[iy*imgWidth + ix];
			u8 *sin = (u8*)&((int*)img)[iy*imgWidth + nx];
			u8 *sni = (u8*)&((int*)img)[ny*imgWidth + ix];
			u8 *snn = (u8*)&((int*)img)[ny*imgWidth + nx];
			// pixel factors (fyx)
			u32 fii = fiy * fix;
			u32 fin = fiy * fnx;
			u32 fni = fny * fix;
			u32 fnn = fny * fnx;
			// pre-multiply factors with alpha
			fii *= (u32)sii[3];
			fin *= (u32)sin[3];
			fni *= (u32)sni[3];
			fnn *= (u32)snn[3];
			// compute alpha
			u32 fff;
			fff = fii + fin + fni + fnn;
			rp[3] = fff >> 16;
			// compute color
			for (i=0; i<3; i++) { // for each R,G,B
				if (fff == 0) {
					rp[i] = 0;
				} else {
					rp[i] = 
						( (u32)sii[i] * fii
						+ (u32)sin[i] * fin
						+ (u32)sni[i] * fni
						+ (u32)snn[i] * fnn
						) / fff;
				}
			}
		}
	}
}


// destination: bg_buf
void CompositeRGBA(char *bg_buf, int bg_w, int bg_h,
		int x, int y, char *img, int width, int height)
{
	int ix, iy;
	char *c, *bg, a;
	c = img;
	for (iy=0; iy<height; iy++) {
		bg = (char*)bg_buf + (y+iy)*(bg_w * 4) + x*4;
		for (ix=0; ix<width; ix++) {
			a = c[3];
			png_composite(bg[0], c[0], a, bg[0]); // r
			png_composite(bg[1], c[1], a, bg[1]); // g
			png_composite(bg[2], c[2], a, bg[2]); // b
			c += 4;
			bg += 4;
		}
	}
}

s32 __Gui_DrawPngRA(void *img, u32 x, u32 y, int width, int height)
{
	IMGCTX   ctx = NULL;
	PNGUPROP imgProp;
	char *img_buf = NULL, *resize_buf = NULL;

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

	img_buf = memalign(32, imgProp.imgWidth * imgProp.imgHeight * 4);
	if (!img_buf) {
		ret = -1;
		goto out;
	}
	resize_buf = memalign(32, width * height * 4);
	if (!resize_buf) {
		ret = -1;
		goto out;
	}

	// decode
	ret = PNGU_DECODE_TO_COORDS_RGBA8(ctx, 0, 0, imgProp.imgWidth, imgProp.imgHeight, 255,
			imgProp.imgWidth, imgProp.imgHeight, img_buf);
	// resize
	ResizeRGBA(img_buf, imgProp.imgWidth, imgProp.imgHeight, resize_buf, width, height);
	// combine
	Video_CompositeRGBA(x, y, resize_buf, width, height);
	// draw
	Video_DrawRGBA(x, y, resize_buf, width, height);

	/* Success */
	ret = 0;

out:
	if (img_buf) free(img_buf);
	if (resize_buf) free(resize_buf);

	/* Free memory */
	if (ctx)
		PNGU_ReleaseImageContext(ctx);

	return ret;
}


void RGB_to_RGBA(const unsigned char *src, unsigned char *dst,
		const int width, const int height, unsigned char alpha)
{
	int i, size;
	size = width * height;
	for (i=0; i<size; i++) {
		memcpy(dst, src, 3);
		dst[3] = alpha;
		dst += 4;
		src += 3;
	}
}

static void RGBA_to_4x4(const unsigned char *src, void *dst,
		const unsigned int width, const unsigned int height)
{
	unsigned int block;
	unsigned int i, c, j;
	unsigned int ar, gb;
	unsigned char *p = (unsigned char*)dst;

	if (!src || !dst) return;

	for (block = 0; block < height; block += 4) {
		for (i = 0; i < width; i += 4) {
			/* Alpha and Red */
			for (c = 0; c < 4; ++c) {
				for (ar = 0; ar < 4; ++ar) {
					j = ((i + ar) + ((block + c) * width)) * 4;
					/* Alpha pixels */
					//*p++ = 255;
					*p++ = src[j + 3];
					/* Red pixels */
					*p++ = src[j + 0];
				}
			}

			/* Green and Blue */
			for (c = 0; c < 4; ++c) {
				for (gb = 0; gb < 4; ++gb) {
					j = (((i + gb) + ((block + c) * width)) * 4);
					/* Green pixels */
					*p++ = src[j + 1];
					/* Blue pixels */
					*p++ = src[j + 2];
				}
			}
		} /* i */
	} /* block */
}

static void C4x4_to_RGBA(const unsigned char *src, unsigned char *dst,
		const unsigned int width, const unsigned int height)
{
	unsigned int block;
	unsigned int i, c, j;
	unsigned int ar, gb;
	unsigned char *p = (unsigned char*)src;

	for (block = 0; block < height; block += 4) {
		for (i = 0; i < width; i += 4) {
			/* Alpha and Red */
			for (c = 0; c < 4; ++c) {
				for (ar = 0; ar < 4; ++ar) {
					j = ((i + ar) + ((block + c) * width)) * 4;
					/* Alpha pixels */
					//*p++ = 255;
					dst[j + 3] = *p++;
					/* Red pixels */
					dst[j + 0] = *p++;
				}
			}

			/* Green and Blue */
			for (c = 0; c < 4; ++c) {
				for (gb = 0; gb < 4; ++gb) {
					j = (((i + gb) + ((block + c) * width)) * 4);
					/* Green pixels */
					dst[j + 1] = *p++;
					/* Blue pixels */
					dst[j + 2] = *p++;
				}
			}
		} /* i */
	} /* block */
}


/**
 * Bad Cover image handler:
 *  - the bad image path is renamed to [path].bad
 *  - deletes any old bad cover images (if exists)
 *  - renames bad cover so it will be redownloaded
 * @param path char * representing the full path to the bad image
 */
void Gui_HandleBadCoverImage(char *path)
{
	char tmppath[200];
	if (!path) return;
	STRCOPY(tmppath, path);
	STRAPPEND(tmppath, ".bad");
	remove(tmppath);
	rename(path, tmppath);
}


GRRLIB_texImg Gui_LoadTexture_RGBA8(const unsigned char my_png[],
		int width, int height, void *dest, char *path)
{
	PNGUPROP imgProp;
	IMGCTX ctx = NULL;
	GRRLIB_texImg my_texture;
	int ret, width4, height4;
	char *buf1 = NULL, *buf2 = NULL;

	memcheck();

	memset(&my_texture, 0, sizeof(GRRLIB_texImg));

	ctx = PNGU_SelectImageFromBuffer(my_png);
	if (ctx == NULL) goto out;
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK) goto out;
   
	if (width == 0 && height == 0 && dest == NULL) {
		width = imgProp.imgWidth;
		height = imgProp.imgHeight;
	}
	
   	buf1 = mem_alloc(imgProp.imgWidth * imgProp.imgHeight * 4);
	if (buf1 == NULL) goto out;

	ret = PNGU_DecodeToRGBA8 (ctx, imgProp.imgWidth, imgProp.imgHeight, buf1, 0, 255);
	if (ret != PNGU_OK) {
		if (ret == -666) Gui_HandleBadCoverImage(path);
		goto out;
	}
	
	if (imgProp.imgWidth != width || imgProp.imgHeight != height) {
	   	buf2 = mem_alloc(width * height * 4);
		if (buf2 == NULL) goto out;
		ResizeRGBA(buf1, imgProp.imgWidth, imgProp.imgHeight, buf2, width, height);
		SAFE_FREE(buf1);
		buf1 = buf2;
		buf2 = NULL;
	}

	width4 = (width + 3) >> 2 << 2;
	height4 = (height + 3) >> 2 << 2;
	if (width != width4 || height != height4) {
		buf2 = mem_alloc(width4 * height4 * 4);
		if (buf2 == NULL) goto out;
		PadRGBA(buf1, width, height, buf2, width4, height4);
		SAFE_FREE(buf1);
		buf1 = buf2;
		buf2 = NULL;
	}

	if (dest == NULL) {
		dest = memalign (32, width4 * height4 * 4);
		if (dest == NULL) goto out;
	}
	memcheck();

	RGBA_to_4x4((u8*)buf1, (u8*)dest, width4, height4);

	//dbg_printf("RGB8 %dx%d %dx%d %dx%d\n",
	//		imgProp.imgWidth, imgProp.imgHeight, width, height, width4, height4);

	my_texture.data = dest;
	my_texture.w = width;
	my_texture.h = height;
	my_texture.tex_format = GX_TF_RGBA8;
	GRRLIB_FlushTex(&my_texture);
	out:
	SAFE_FREE(buf1);
	SAFE_FREE(buf2);
	if (ctx) PNGU_ReleaseImageContext(ctx);
	return my_texture;
}


/**
 * Method used to convert a RGBA8 image into CMPR
 */
GRRLIB_texImg Convert_to_CMPR(void *img, int width, int height, void *dest)
{
	GRRLIB_texImg my_texture;
	int ret;
	void *buf1 = NULL;

	memset(&my_texture, 0, sizeof(GRRLIB_texImg));

	int size = GX_GetTexBufferSize(width, height, GX_TF_RGBA8, GX_FALSE, 0);
	buf1 = memalign (32, size);
	if (buf1 == NULL) goto out;
	RGBA_to_4x4((u8*)img, (u8*)buf1, width, height);

	if (dest == NULL) {
		size = GX_GetTexBufferSize(width, height, GX_TF_CMPR, GX_FALSE, 0);
		dest = memalign (32, size);
		if (dest == NULL) goto out;
	}
	
	ret = PNGU_4x4RGBA8_To_CMPR(buf1, width, height, dest);
	if (ret != PNGU_OK) goto out;
	
	my_texture.data = dest;
	my_texture.w = width;
	my_texture.h = height;
	my_texture.tex_format = GX_TF_CMPR;
	GRRLIB_FlushTex(&my_texture);

	out:
	SAFE_FREE(buf1);
	return my_texture;
}


/**
 * Decodes a PNG to CMPR
 *
 * @param my_png[] PNG image
 * @param width png image width
 * @param height png image height
 * @param *dest image destination (CMPR-encoded)
 */
GRRLIB_texImg Gui_LoadTexture_CMPR(const unsigned char my_png[],
		int width, int height, void *dest, char *path)
{
	PNGUPROP imgProp;
	IMGCTX ctx = NULL;
	GRRLIB_texImg my_texture;
	int ret;
	void *buf1 = NULL;
	void *buf2 = NULL;

	memset(&my_texture, 0, sizeof(GRRLIB_texImg));

	ctx = PNGU_SelectImageFromBuffer(my_png);
	if (ctx == NULL) goto out;
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK) goto out;
   
	if (width == 0 && height == 0 && dest == NULL) {
		width = imgProp.imgWidth;
		height = imgProp.imgHeight;
	}
	
	// CMPR allocation has to be a multiple of 8
	//u32 imgheight, imgwidth;
	//imgheight = imgProp.imgHeight & ~7u;
	//imgwidth = imgProp.imgWidth & ~7u;
	// instead of trimming we will pad.
	
	// check if we need to resize
	if ((imgProp.imgHeight != height) || (imgProp.imgWidth != width)) {
		//dbg_printf("cmpr resize %dx%d %dx%d %s\n",
		//	imgProp.imgWidth, imgProp.imgHeight, width, height, path);
		// get RGBA8 version of image
		buf1 = memalign (32, imgProp.imgWidth * imgProp.imgHeight * 4);
		if (buf1 == NULL) goto out;
		ret = PNGU_DecodeToRGBA8 (ctx, imgProp.imgWidth, imgProp.imgHeight, buf1, 0, 255);
		if (ret != PNGU_OK) {
			if (ret == -666) Gui_HandleBadCoverImage(path);
			goto out;
		}
		// allocate for resize to passed in dimensions
		buf2 = mem_alloc(width * height * 4);
		if (buf2 == NULL) goto out;		
		ResizeRGBA(buf1, imgProp.imgWidth, imgProp.imgHeight, buf2, width, height);
		SAFE_FREE(buf1);
		// convert to cmpr
		PNGU_RGBA8_To_CMPR(buf2, width, height, dest);
	} else {
		if (dest == NULL) {
			int size = GX_GetTexBufferSize(imgProp.imgWidth, imgProp.imgHeight,
					GX_TF_CMPR, GX_FALSE, 0);
			dest = mem_alloc(size);
			if (dest == NULL) goto out;
		}
		ret = PNGU_DecodeToCMPR_Pad(ctx, imgProp.imgWidth, imgProp.imgHeight, dest);
		if (ret != PNGU_OK) {
			if (ret == -666) Gui_HandleBadCoverImage(path);
			goto out;
		}
	}
	my_texture.w = width;
	my_texture.h = height;
	my_texture.data = dest;
	my_texture.tex_format = GX_TF_CMPR;
	GRRLIB_FlushTex(&my_texture);
	out:
	SAFE_FREE(buf1);
	SAFE_FREE(buf2);
	if (ctx) PNGU_ReleaseImageContext(ctx);
	return my_texture;
}

u32 upperPower(u32 width)
{
	u32 i = 8;
	u32 maxWidth = 1024;
	while (i < width && i < maxWidth)
		i <<= 1;
	return i;
}

// For powers of two
void resizeD2x2(u8 *dst, const u8 *src, u32 srcWidth, u32 srcHeight)
{
	u32 i = 0, i0 = 0, i1 = 4;
	u32 i2 = srcWidth * 4;
	u32 i3 = (srcWidth + 1) * 4;
	u32 dstWidth = srcWidth >> 1;
	u32 dstHeight = srcHeight >> 1;
	u32 w4 = srcWidth * 4;
	u32 x, y;
	for (y = 0; y < dstHeight; ++y) {
		for (x = 0; x < dstWidth; ++x) {
			dst[i] = ((u32)src[i0] + src[i1] + src[i2] + src[i3]) >> 2;
			dst[i + 1] = ((u32)src[i0 + 1] + src[i1 + 1] + src[i2 + 1] + src[i3 + 1]) >> 2;
			dst[i + 2] = ((u32)src[i0 + 2] + src[i1 + 2] + src[i2 + 2] + src[i3 + 2]) >> 2;
			dst[i + 3] = ((u32)src[i0 + 3] + src[i1 + 3] + src[i2 + 3] + src[i3 + 3]) >> 2;
			i += 4;
			i0 += 8;
			i1 += 8;
			i2 += 8;
			i3 += 8;
		}
		i0 += w4;
		i1 += w4;
		i2 += w4;
		i3 += w4;
	}
}

void ColorizeImage(void *img, int w, int h, u32 color)
{
	u32 *p = (u32*)img;
	int n = w * h;
	int i;
	for (i=0; i<n; i++) {
		*p = color_multiply(*p, color);
		p++;
	}
}

GRRLIB_texImg Gui_LoadTexture_MIPMAP(const unsigned char my_png[],
		int width, int height, int maxlod, void *dest, char *path)
{
	GRRLIB_texImg my_texture;
	PNGUPROP imgProp;
	IMGCTX ctx = NULL;
	void *buf1 = NULL;
	void *buf2 = NULL;
	void *destbuf = NULL;
	int ret;
	int i;

	memset(&my_texture, 0, sizeof(GRRLIB_texImg));

	ctx = PNGU_SelectImageFromBuffer(my_png);
	if (ctx == NULL) goto out;
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK) goto out;
   
	// MIPMAP size has to be a power of 2
	if (width != upperPower(width) || height != upperPower(width)) {
		return my_texture;
	}
	
	buf1 = memalign (32, imgProp.imgWidth * imgProp.imgHeight * 4);
	if (buf1 == NULL) goto out;
	ret = PNGU_DecodeToRGBA8 (ctx, imgProp.imgWidth, imgProp.imgHeight, buf1, 0, 0xFF);
	if (ret != PNGU_OK) {
		if (ret == -666) Gui_HandleBadCoverImage(path);
		goto out;
	}

	int size = fixGX_GetTexBufferSize(width, height, GX_TF_RGBA8, GX_TRUE, maxlod);
	buf2 = mem_alloc(size);
	if (buf2 == NULL) goto out;		

	int dest_size = fixGX_GetTexBufferSize(width, height, GX_TF_CMPR, GX_TRUE, maxlod);
	if (!dest) {
		dest = destbuf = mem_alloc(dest_size);
		if (dest == NULL) goto out;
	}

	ResizeRGBA(buf1, imgProp.imgWidth, imgProp.imgHeight, buf2, width, height);

	u32 nWidth = width;
	u32 nHeight = height;
	u8 *rgb_src = buf2; // rgba
	u8 *rgb_dest = buf2; // rgba
	for (i=0; i<maxlod; i++) {
		rgb_src = rgb_dest;
		rgb_dest += nWidth * nHeight * 4;
		resizeD2x2(rgb_dest, rgb_src, nWidth, nHeight);
		nWidth >>= 1;
		nHeight >>= 1;
	}

	nWidth = width;
	nHeight = height;
	rgb_src = buf2;
	u8 *pdest = dest; // cmpr
	for (i=0; i<maxlod+1; i++) {
		// debug
		/*
		u32 color = 0xFFFFFFFF;
		switch (i) {
			case 0: color = 0xFF0000FF; break;
			case 2: color = 0x00FF00FF; break;
			case 4: color = 0x0000FFFF; break;
		}
		ColorizeImage(rgb_src, nWidth, nHeight, color);
		*/
		// debug
		PNGU_RGBA8_To_CMPR(rgb_src, nWidth, nHeight, pdest);
		rgb_src += nWidth * nHeight * 4;
		pdest += fixGX_GetTexBufferSize(nWidth, nHeight, GX_TF_CMPR, GX_FALSE, 0);
		nWidth >>= 1;
		nHeight >>= 1;
	}
	// done
	my_texture.w = width;
	my_texture.h = height;
	my_texture.data = dest;
	my_texture.tex_format = GX_TF_CMPR;
	my_texture.tex_lod = maxlod;
	GRRLIB_FlushTex(&my_texture);
	out:
	SAFE_FREE(buf1);
	SAFE_FREE(buf2);
	if (!my_texture.data) SAFE_FREE(destbuf);
	if (ctx) PNGU_ReleaseImageContext(ctx);
	return my_texture;
}


/**
 * Method used by the cache to paste 2d cover images into the nocover_full image
 */
GRRLIB_texImg Gui_LoadTexture_fullcover(const unsigned char my_png[],
		int width, int height, int frontCoverWidth, void *dest, char *path)
{
	PNGUPROP imgProp;
	IMGCTX ctx = NULL;
	GRRLIB_texImg my_texture;
	int ret;
	int frontCoverStart = tx_nocover_full.w - frontCoverWidth;
	void *buf1 = NULL;
	void *buf2 = NULL;
	void *buf3 = NULL;

	memset(&my_texture, 0, sizeof(GRRLIB_texImg));

	ctx = PNGU_SelectImageFromBuffer(my_png);
	if (ctx == NULL) goto out;
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK) goto out;
	
	//allocate for RGBA8 2D cover
   	buf1 = memalign (32, imgProp.imgWidth * imgProp.imgHeight * 4);
	if (buf1 == NULL) goto out;
	ret = PNGU_DecodeToRGBA8 (ctx, imgProp.imgWidth, imgProp.imgHeight, buf1, 0, 255);
	if (ret != PNGU_OK) {
		if (ret == -666) Gui_HandleBadCoverImage(path);
		goto out;
	}

	//check if we need to resize
	if (imgProp.imgWidth != frontCoverWidth || imgProp.imgHeight != height) {
	   	buf2 = memalign (32, frontCoverWidth * height * 4);
		if (buf2 == NULL) goto out;
		ResizeRGBA(buf1, imgProp.imgWidth, imgProp.imgHeight, buf2, frontCoverWidth, height);
		SAFE_FREE(buf1);
		buf1 = buf2;
		buf2 = NULL;
	}

	//allocate for RGBA8 nocover_full image
	buf2 = memalign (32, GRRLIB_TextureSize(&tx_nocover_full));
	if (buf2 == NULL) goto out;
	C4x4_to_RGBA(tx_nocover_full.data, buf2, tx_nocover_full.w, tx_nocover_full.h);
	
	//check if we need to resize
	if (tx_nocover_full.w != width || tx_nocover_full.h != height) {
		buf3 = memalign (32, height * width * 4);
		if (buf3 == NULL) goto out;
		ResizeRGBA(buf2, tx_nocover_full.w, tx_nocover_full.h, buf3, width, height);
		SAFE_FREE(buf2);
		buf2 = buf3;
		buf3 = NULL;
	}
	
	//the full cover image is layed out like this:  back | spine | front
	//so we want to paste the passed in image in the front section
	CompositeRGBA(buf2, width, height,
			frontCoverStart, 0, //place where front cover is located
			buf1, frontCoverWidth, height);

	if (CFG.gui_compress_covers) {
		my_texture = Convert_to_CMPR(buf2, width, height, dest);
		my_texture.tex_format = GX_TF_CMPR;
	} else {
		RGBA_to_4x4((u8*)buf2, (u8*)dest, width, height);
		my_texture.data = dest;
		my_texture.w = width;
		my_texture.h = height;
		my_texture.tex_format = GX_TF_RGBA8;
		GRRLIB_FlushTex(&my_texture);
	}
	
	out:
	SAFE_FREE(buf1);
	SAFE_FREE(buf2);
	if (ctx) PNGU_ReleaseImageContext(ctx);
	return my_texture;
}


/**
 * Method used to paste the src image into the dest image (fullcover size)
 */
GRRLIB_texImg Gui_paste_into_fullcover(void *src, int src_w, int src_h, 
		void *dest, int dest_w, int dest_h)
{
	int height, width;
	int width_front;
	void *buf1 = NULL;
	void *buf2 = NULL;
	GRRLIB_texImg my_texture;

	memset(&my_texture, 0, sizeof(GRRLIB_texImg));
	
	if (!dest) goto out;
	
	buf1 = memalign (32, src_w * src_h * 4);
	if (buf1 == NULL) goto out;
	C4x4_to_RGBA(src, buf1, src_w, src_h);

	buf2 = memalign (32, dest_w * dest_h * 4);
	if (buf2 == NULL) goto out;
	C4x4_to_RGBA(dest, buf2, dest_w, dest_h);
	
	//the full cover image is layed out like this:  back | spine | front
	//so we want to paste the passed in image in the front section
	//COVER_WIDTH_FRONT = (int)(COVER_HEIGHT / 1.4) >> 2 << 2;
	// 512x336 ; 240 = 336/1.4 ; 240+32+240=512
	width_front = (int)(dest_h / 1.4) >> 2 << 2;
	CompositeRGBA(buf2, dest_w, dest_h,
			//place where front cover is located
			dest_w - (width_front/2) - (src_w/2), (dest_h/2) - (src_h/2),
			buf1, src_w, src_h);
	
	SAFE_FREE(buf1);
	
	if (CFG.gui_compress_covers) {
		//CMPR is divisible by 8
		height = dest_h & ~7u;
		width = dest_w & ~7u;
		my_texture = Convert_to_CMPR(buf2, width, height, buf1);
	} else {
		buf1 = memalign(32, dest_w * dest_h * 4);
		RGBA_to_4x4((u8*)buf2, (u8*)buf1, dest_w, dest_h);
		my_texture.data = buf1;
		my_texture.w = dest_w;
		my_texture.h = dest_h;
		GRRLIB_FlushTex(&my_texture);
	}
	
	out:;
	SAFE_FREE(buf2);
	return my_texture;
}

/**
 * Reads a JPG file from disk and decompresses it into a 4x4 RGBA
 * @param path The absolute path to the jpg file
 * @return GRRLIB_texImg
 */
GRRLIB_texImg Gui_LoadJPGFromPath(char *path)
{
	s32  ret;
	void *imgData = NULL;
	GRRLIB_texImg tx;

	//init the tex
	memset(&tx, 0, sizeof(GRRLIB_texImg));
	
	//read in the file
	ret = Fat_ReadFile(path, &imgData);
	if (ret <= 0 || imgData==NULL) goto out;

	//load the image
	tx = my_GRRLIB_LoadTextureJPG(imgData);
	if (tx.w == -666) Gui_HandleBadCoverImage(path);
		
out:
	SAFE_FREE(imgData);
	return tx;
}

/**
 * Draws a 4x4 RGBA image onto the screen.
 * To be used in console mode only.
 * @param tx The image to draw
 * @param xpos The X position
 * @param ypos The Y position
 * @param dest_w The desired image width to draw
 * @param dest_h The desired image height to draw
 * @return void
 */
void Gui_DrawImage_Console(GRRLIB_texImg *tx, int xpos, int ypos, int dest_w, int dest_h)
{
	void *buf1 = NULL;
	void *buf2 = NULL;
	int width4, height4;

	//anything to draw?
	if (tx->data == NULL) return;
	
	//make sure final dimensions are divisible by 4
	width4 = (dest_w + 3) >> 2 << 2;
	height4 = (dest_h + 3) >> 2 << 2;

	//allocate temp storage for resize
	buf1 = mem_alloc (tx->w * tx->h * 4);  
	if (buf1 == NULL) goto out;
	buf2 = memalign (32, width4 * height4 * 4);
	if (buf2 == NULL) goto out;

	//convert to RGBA, resize and draw
	C4x4_to_RGBA(tx->data, buf1, tx->w, tx->h);
	ResizeRGBA(buf1, tx->w, tx->h, buf2, width4, height4);
	Video_DrawRGBA(xpos, ypos, buf2, width4, height4);
out:
	SAFE_FREE(buf1);
	SAFE_FREE(buf2);
}

/**
 * Draws the theme preview image
 * @param id The id(name) of the theme to draw
 **/
void Gui_DrawThemePreviewX(char *name, int id, int X, int Y, int W, int H)
{
	char theme_path[200];
	GRRLIB_texImg tx;

	//did we get a name to draw?
	if ((name == NULL) || (strlen(name) < 1)) return;
	//create the image path - images are in ./theme_previews dir
	snprintf(D_S(theme_path), "%s/theme_previews/%d.jpg", USBLOADER_PATH, id);
	//try to read in the image
	tx = Gui_LoadJPGFromPath(theme_path);
	if (tx.data == NULL) {
		//draw nocover image
		tx = Gui_LoadTexture_RGBA8(coverImg, COVER_WIDTH, COVER_HEIGHT, NULL, NULL);
	}
		
	//draw the image
	if (CFG.debug == 3) printf("X:%d  Y:%d  W:%d  H:%d\n", X, Y, W, H);
	Gui_DrawImage_Console(&tx, X, Y, W, H);
	SAFE_FREE(tx.data);
}

void Gui_DrawThemePreview(char *name, int id)
{
	Gui_DrawThemePreviewX(name, id,
			CFG.theme_previewX, CFG.theme_previewY, CFG.theme_previewW, CFG.theme_previewH); 
}

void Gui_DrawThemePreviewLarge(char *name, int id)
{
	int con_h = 16 * 3;
	// 2 lines of pixels will be overwritten by console
	// intentional, because the jpg->rgba->c4x4->rgba conversion will
	// corrupt the bottom 2 lines because h (250) is not a multiple of 4
	// hence con_h+2
	int h = 480 - con_h + 2;
	int w = (640 * h / 480) / 2 * 2;
	int x = (640 - w) / 2 / 2 * 2;
	int y = 0;
	Video_Clear(COLOR_BLACK);
	Gui_DrawThemePreviewX(name, id, x, y, w, h); 
	CON_InitEx(_Video_GetVMode(), 0, 480-con_h, 640, con_h);
	printf("%15s %s: %s\n", " ", gt("Theme"), name);
	printf("%15s %s", " ", gt("Press any button..."));
	__console_flush(0);
	Wpad_WaitButtonsCommon();
	Gui_InitConsole();
	Video_DrawBg();
}

#if 0
void cache2_tex_alloc(struct M2_texImg *dest, int w, int h)
{
	int src_size = w * h * 4;
	// data exists and big enough?
	if ((dest->tx.data == NULL) || (dest->size < src_size)) {
		dest->tx.data = LARGE_realloc(dest->tx.data, src_size);
		//dest->tx.data = LARGE_alloc(src_size);
		dest->size = src_size;
	}
	dest->tx.w = w;
	dest->tx.h = h;
}

void cache2_tex(struct M2_texImg *dest, GRRLIB_texImg *src)
{
	//int src_size = src->w * src->h * 4;
	int fmt = src->tex_format ? src->tex_format : GX_TF_RGBA8;
	int src_size = GX_GetTexBufferSize(src->w, src->h, fmt, GX_FALSE, 0);
	if (src->data == NULL) return;
	// realloc
	cache2_tex_alloc(dest, src->w, src->h);
	void *data = dest->tx.data;
	if (!data) return;
	memcpy(data, src->data, src_size);
	memcpy(&dest->tx, src, sizeof (GRRLIB_texImg));
	dest->tx.data = data; // reset as above will overwrite it
	GRRLIB_FlushTex(&dest->tx);
	// free src texture
	SAFE_FREE(src->data);
	src->w = src->h = 0;
}

void cache2_tex_alloc_fullscreen(struct M2_texImg *dest)
{
	GXRModeObj *rmode = _Video_GetVMode();
	cache2_tex_alloc(dest, rmode->fbWidth, rmode->efbHeight);
}

void cache2_GRRLIB_tex_alloc(GRRLIB_texImg *dest, int w, int h)
{
	int src_size = w * h * 4;
	void *m2_buf;
	if (dest->data) return;
	// alloc new
	m2_buf = LARGE_memalign(32, src_size);
	if (m2_buf == NULL) return;
	dest->data = m2_buf;
	dest->w = w;
	dest->h = h;
}

void cache2_GRRLIB_tex_alloc_fullscreen(GRRLIB_texImg *dest)
{
	GXRModeObj *rmode = _Video_GetVMode();
	cache2_GRRLIB_tex_alloc(dest, rmode->fbWidth, rmode->efbHeight);
}
#endif

void Gui_AllocTextureFullscreen(GRRLIB_texImg *dest)
{
	GXRModeObj *rmode = _Video_GetVMode();
	GRRLIB_AllocTextureData(dest, rmode->fbWidth, rmode->efbHeight);
}

long long render_cpu; // usec
long long render_gpu; // usec
long long render_busy; // usec
long long render_idle; // usec

void Gui_Render()
{
	static long long t0 = 0;
	static long long t1 = 0;
	static long long t2 = 0;

	t0 = gettime();
	if (CFG.debug)
	{
		GRRLIB_Rectangle(15,15,620,30, 0x404040B0, 1);
		// frame = busy (cpu+gpu) + idle
		GRRLIB_Printf(20, 20, &tx_font, 0xB0B0FFFF, 1,
				"f:%5.2f = b:%5.2f (c:%5.2f+g:%5.2f) + i:%5.2f ms",
				(float)(render_busy + render_idle)/1000.0,
				(float)render_busy/1000.0,
				(float)render_cpu/1000.0,
				(float)render_gpu/1000.0,
				(float)render_idle/1000.0);
	}

	_GRRLIB_Render();
	GX_DrawDone();
	//__console_disable=0; __console_flush(-1); // debug
	t1 = gettime();
	_GRRLIB_VSync();
	if (t2) {
		render_cpu = diff_usec(t2, t0);
		render_gpu = diff_usec(t0, t1);
		render_busy = diff_usec(t2, t1);
	}
	t2 = gettime();
	render_idle = diff_usec(t1, t2);
	if (gui_style == GUI_STYLE_FLOW_Z) {
		GX_SetZMode (GX_FALSE, GX_NEVER, GX_TRUE);
	}
}

// outside of main gui loop
// renders also wgui and pointer
void Gui_Render_Out(ir_t *ir)
{
	wgui_input_save2(ir, NULL);
	wgui_desk_render(NULL, NULL);
	Gui_draw_pointer(ir);
	Gui_Render();
}

void Gui_DrawImgFullScreen(GRRLIB_texImg *tex, const u32 color, bool antialias)
{
	int save_aa = GRRLIB_Settings.antialias;
	GRRLIB_Settings.antialias = antialias;
	// even though tex could be 640x528 in 50 Hz mode
	// we use 640x480 coordinates because guortho is fixed at 640x480
	GRRLIB_DrawImgRect(0, 0, 640, 480, tex, color);
	GRRLIB_Settings.antialias = save_aa;
}

/**
 * Renders the current scene to a texture and stores it in the global aa_texBuffer array.
 *  @param aaStep int position in the array to store the created tex
 *  @return void
 */
void Gui_RenderAAPass(int aaStep)
{
	if (aa_method == 0) {
		if (aa_texBuffer[aaStep].data == NULL) {
			// first time, allocate
			Gui_AllocTextureFullscreen(&aa_texBuffer[aaStep]);
			if (aa_texBuffer[aaStep].data == NULL) return;
		}
		GRRLIB_AAScreen2Texture_buf(&aa_texBuffer[aaStep], GX_TRUE);
	} else {
		// new slightly faster method
		// also takes less memory - only 1 buffer required
		if (aa_texBuffer[0].data == NULL) {
			// first time, allocate
			Gui_AllocTextureFullscreen(&aa_texBuffer[0]);
			if (aa_texBuffer[0].data == NULL) return;
		}
		if (aaStep > 0) {
			u32 color, alpha;
			alpha = 0xFF - 0xFF / (aaStep + 1);
			color = 0xFFFFFF00 | alpha;
			Gui_DrawImgFullScreen(&aa_texBuffer[0], color, false);
		}
		if (aaStep < CFG.gui_antialias - 1) {
			GRRLIB_AAScreen2Texture_buf(&aa_texBuffer[0], GX_TRUE);
		}
	}
}

void Gui_Init()
{
	if (CFG.gui == 0) return;
	if (grr_init) return;

	VIDEO_WaitVSync();
	if (GRRLIB_Init_VMode(_Video_GetVMode(), _Video_GetFB(0), _Video_GetFB(1)))
	{
		printf(gt("Error GRRLIB init"));
		printf("\n");
		sleep(1);
		return;
	}
	VIDEO_WaitVSync();
	grr_init = 1;
}

bool Load_Theme_Texture_1(char *name, GRRLIB_texImg *tx)
{
	void *data = NULL;
	int ret;

	SAFE_FREE(tx->data);
	ret = Load_Theme_Image(name, &data);
	if (ret == 0 && data) {
		tx_store(tx, GRRLIB_LoadTexture(data));
		GRRLIB_SetHandle(tx, tx->w/2, tx->h/2);
		SAFE_FREE(data);
		if (tx->data) return true;
	}
	return false;
}

void Load_Theme_Texture(char *name, GRRLIB_texImg *tx, void *builtin)
{
	if (Load_Theme_Texture_1(name, tx)) return;
	// failed, use builtin
	tx_store(tx, GRRLIB_LoadTexture(builtin));
	GRRLIB_SetHandle(tx, tx->w/2, tx->h/2);
}

void GRRLIB_CopyTextureBlock(GRRLIB_texImg *src, int x, int y, int w, int h,
		GRRLIB_texImg *dest, int dx, int dy)
{
	int ix, iy;
	u32 c;
	for (ix=0; ix<w; ix++) {
		for (iy=0; iy<h; iy++) {
			c = GRRLIB_GetPixelFromtexImg(x+ix, y+iy, src);
			GRRLIB_SetPixelTotexImg(dx+ix, dy+iy, dest, c);
		}
	}
}

void GRRLIB_RearrangeFont128(GRRLIB_texImg *tx)
{
	int tile_w = tx->w / 128;
	int tile_h = tx->h;
	if (tx->w <= 1024) return;
	// split to max 1024 width stripes
	GRRLIB_texImg tt;
	int i;
	int c_per_stripe;
	int nstripe;
	int stripe_w;
	// max number of characters per stripe
	c_per_stripe = 1024 / tile_w;
	// round down to a multiple of 4, so that the texture size
	// is a multiple of 4 as well.
	c_per_stripe = c_per_stripe >> 2 << 2;
	// number of stripes
	nstripe = (128 + c_per_stripe - 1) / c_per_stripe;
	// stripe width
	stripe_w = c_per_stripe * tile_w;
	GRRLIB_AllocTextureData(&tt, stripe_w, tile_h * nstripe);
	for (i=0; i<nstripe; i++) {
		GRRLIB_CopyTextureBlock(tx, stripe_w * i, 0, stripe_w, tile_h,
				&tt, 0, tile_h * i);
	}
	SAFE_FREE(tx->data);
	*tx = tt;
	tt.data = NULL;
}

void GRRLIB_InitFont(GRRLIB_texImg *tx)
{
	int tile_w;
	int tile_h;

	if (!tx->data) return;

	// detect font image type: 128x1 or 32x16 (512)
	if (tx->w / tx->h >= 32) {
		tile_w = tx->w / 128;
		tile_h = tx->h;
		// 128x1 font
		if (tx->w > 1024) {
			// split to max 1024 width stripes
			GRRLIB_RearrangeFont128(tx);
		}
	} else {
		// 32x16 font
		tile_w = tx->w / 32;
		tile_h = tx->h / 16;
	}
	//Gui_Console_Enable();
	//printf("\n%d %d %d %d\n", tx->w, tx->h, tile_w, tile_h);
	//Wpad_WaitButtons();

	// init tile
	GRRLIB_InitTileSet(tx, tile_w, tile_h, 0);
	GRRLIB_FlushTex(tx);
}

void GRRLIB_TrimTile(GRRLIB_texImg *tx, int maxn)
{
	if (!tx || !tx->data) return;
	int n = tx->nbtilew * tx->nbtileh;
	if (n <= maxn) return;
	// roundup maxn to a full row
	int nh = (maxn + tx->nbtilew - 1) / tx->nbtilew;
	tx->h = tx->tileh * nh;
	int size = tx->w * tx->h * 4;
	// downsize
	tx->data = mem_realloc(tx->data, size);
	// re-init tile
	GRRLIB_InitTileSet(tx, tx->tilew, tx->tileh, 0);
}

void Gui_DumpUnicode(int xx, int yy)
{
	wchar_t wc;
	char mb[32*6+1];
	char line[10+32*6+1];
	int i, k, len=0, count=0, y=0;
	int fh = tx_font.tileh;
	for (i=0; i<512; i++) {
		wc = i;
		k = wctomb(mb+len, wc);
		if (k < 1 || mb[len] == 0) {
			mb[len] = wc < 128 ? '!' : '?';
			k = 1;
		}
		len += k;
		mb[len] = 0;
		count++;
		if (count >= 32) {
			sprintf(line, "%03x: %s |", (unsigned)i-31, mb);
			y = 10+(i/32*fh);
			Gui_Print2(xx+5, yy+y, line);
			len = 0;
			count = 0;
		}
	}
	for (i=0; ufont_map[i]; i+=2) {
		wc = ufont_map[i];
		k = wctomb(mb+len, wc);
		if (k < 1 || mb[len] == 0) {
			mb[len] = wc < 128 ? '!' : '?';
			k = 1;
		}
		len += k;
		mb[len] = 0;
		count++;
		if (count >= 32 || !ufont_map[i+2]) {
			sprintf(line, "map: %s |", mb);
			y += fh;
			Gui_Print2(xx+5, yy+y, line);
			len = 0;
			count = 0;
		}
	}
}

void Gui_TestUnicode()
{
	if (CFG.debug != 2) return;
	GRRLIB_FillScreen(0x000000FF);
	Gui_DumpUnicode(0, 0);
	Gui_Print2(5, 440, gt("Press any button..."));
	Gui_Render();
	Wpad_WaitButtons();
}

void Gui_FillAscii(int xx, int yy, u32 color, int method)
{
	char line[1000];
	int y=0;
	int cols = 640 / tx_font.tilew - 1;
	int rows = 480 / tx_font.tileh - 2;
	int r, i;
	int c = 33;
	for (r=0; r<rows; r++) {
		for (i=0; i<cols; i++) {
			line[i] = c;
			c++;
			if (c >= 128) c = 33;
		}
		line[i] = 0;
		y = 10 + r * tx_font.tileh;
		if (method == 0)
			GRRLIB_Printf(xx+5, yy+y, &tx_font, color, 1.0, "%s", line);
		else
			Gui_PrintEx2(xx+5, yy+y, &tx_font, color, 0, 0, line);
	}
}

void Gui_BenchText()
{
	int i, m;
	int n = 10;
	for (m=0; m<2; m++) {
		GX_SetZMode (GX_FALSE, GX_NEVER, GX_TRUE);
		GRRLIB_FillScreen(0x000000FF);
		dbg_time_usec();
		for (i=0; i<n; i++) {
			u32 r = 0xff - (i%10+1) * 0xff / 10;
			u32 g = (i%20+1) * 0xff / 20;
			u32 b = 0xff - (i%30+1) * 0xff / 30;
			u32 c = RGBA(r,g,b,0xff);
			if (i==n-1) c = 0xFFFFFFFF;
			Gui_FillAscii(i%10, i%10+(i/10)%10, c, m);
		}
		long long us = dbg_time_usec();
		Gui_Printf(5, 440, "ms: %5.2f", (float)us/1000.0);
		Gui_Print2(5, 460, gt("Press any button..."));
		Gui_Render();
		Wpad_WaitButtons();
	}
	// m=1 seems to be 5x faster
}

void Gui_PrintEx2(int x, int y, GRRLIB_texImg *font,
		int color, int color_outline, int color_shadow, const char *str)
{
	GRRLIB_Print2(x, y, font, color, color_outline, color_shadow, str);
}

u32 color_add(u32 c1, u32 c2, int neg)
{
	int i, x1, x2, c;
	u32 color = 0;
	// each component
	for (i=0; i<4; i++) {
		x1 = (c1 >> (i*8)) & 0xFF;
		x2 = (c2 >> (i*8)) & 0xFF;
		c = x1 + neg * x2;
		// range check, possible overflow 
		if (c < 0) c = 0;
		if (c > 0xFF) c = 0xFF;
		color |= (c << (i*8));
	}
	return color;
}

u32 color_multiply(u32 c1, u32 c2)
{
	int i, x1, x2;
	u32 color = 0;
	u32 c;
	// each component
	for (i=0; i<4; i++) {
		x1 = (c1 >> (i*8)) & 0xFF;
		x2 = (c2 >> (i*8)) & 0xFF;
		c = x1 * x2 / 0xFF;
		// range check, just in case, but it shouldn't overflow 
		if (c < 0) c = 0;
		if (c > 0xFF) c = 0xFF;
		color |= (c << (i*8));
	}
	return color;
}

void font_color_multiply(FontColor *src, FontColor *dest, u32 color)
{
#define FC_MUL(X) dest->X = color_multiply(src->X, color)
	FC_MUL(color);
	// AA^4 for outline and shadow
	// because they are drawn 4 times
	color = color_multiply(color, color | 0xFFFFFF00);
	color = color_multiply(color, color | 0xFFFFFF00);
	FC_MUL(outline);
	FC_MUL(shadow);
#undef FC_MUL
}

void Gui_PrintExx(float x, float y, GRRLIB_texImg *font, FontColor fc,
		float zoom, const char *str)
{
	GRRLIB_Print3(x, y, font, fc.color, fc.outline, fc.shadow, zoom, str);
}

void Gui_PrintEx(int x, int y, GRRLIB_texImg *font, FontColor font_color, const char *str)
{
	Gui_PrintExx(x, y, font, font_color, 1.0, str);
}

void Gui_PrintfEx(int x, int y, GRRLIB_texImg *font, FontColor font_color, char *fmt, ...)
{
	char str[200];
	va_list argp;
	va_start(argp, fmt);
	vsnprintf(str, sizeof(str), fmt, argp);
	va_end(argp);
	Gui_PrintEx(x, y, font, font_color, str);
}

// alignx: -1=left 0=centre 1=right
// aligny: -1=top  0=centre 1=bottom
void Gui_PrintAlignZ(float x, float y, int alignx, int aligny,
		GRRLIB_texImg *font, FontColor font_color, float zoom, char *str)
{
	float xx = x, yy = y;
	int len;
	float w, h;
	len = con_len(str);
	w = len * font->tilew;
	h = font->tileh;
	w *= zoom;
	h *= zoom;
	if (alignx == 0) xx = x - w/2.0;
	else if (alignx > 0) xx = x - w;
	if (aligny == 0) yy = y - h/2.0;
	else if (aligny > 0) yy = y - h;
	// align to pixel for sharper text
	xx = (int)xx;
	yy = (int)yy;
	Gui_PrintExx(xx, yy, font, font_color, zoom, str);
}

void Gui_PrintAlign(int x, int y, int alignx, int aligny,
		GRRLIB_texImg *font, FontColor font_color, char *str)
{
	Gui_PrintAlignZ(x, y, alignx, aligny, font, font_color, 1.0, str);
}

void Gui_Print2(int x, int y, const char *str)
{
	Gui_PrintEx(x, y, &tx_font, CFG.gui_text2, str);
}

void Gui_Print(int x, int y, char *str)
{
	Gui_PrintEx(x, y, &tx_font, CFG.gui_text, str);
}

void Gui_Printf(int x, int y, char *fmt, ...)
{
	char str[200];
	va_list argp;
	va_start(argp, fmt);
	vsnprintf(str, sizeof(str), fmt, argp);
	va_end(argp);
	Gui_Print(x, y, str);
}

void Gui_Print_Clock(int x, int y, FontColor font_color, int align)
{
	if (CFG.clock_style == 0) return;
	GRRLIB_texImg *tx = &tx_font_clock;
	if (!tx->data) tx = &tx_font;
	Gui_PrintAlign(x, y, align, align, tx, font_color, get_clock_str(time(NULL)));
}

void Grx_Load_BG_Gui()
{
	void *img_buf = NULL;

	// widescreen?
	if (CFG.widescreen && *CFG.bg_gui_wide) {
		img_buf = Gui_LoadBG(CFG.bg_gui_wide);
	}
	// or normal
	if (img_buf == NULL) {
		img_buf = Gui_LoadBG(CFG.bg_gui);
	}
	// or builtin
	if (img_buf == NULL) {
		img_buf = Gui_LoadBG_data(bg_gui);
	}

	// overlay
	int ret = -1;
	char path[200];

	if (CFG.widescreen) {
		snprintf(D_S(path), "%s/bg_gui_over_w.png", CFG.theme_path);
		ret = Gui_OverlayBg(path, img_buf);
	}
   	if (ret) {
		snprintf(D_S(path), "%s/bg_gui_over.png", CFG.theme_path);
		ret = Gui_OverlayBg(path, img_buf);
	}

	// texture
	GRRLIB_ReallocTextureData(&tx_bg, BACKGROUND_WIDTH, BACKGROUND_HEIGHT);
	RGBA_to_4x4((u8*)img_buf, (u8*)tx_bg.data, BACKGROUND_WIDTH, BACKGROUND_HEIGHT);
	SAFE_FREE(img_buf);
}


/**
 * Initilaizes all the common images (no cover, pointer, font, background...)
 *
 */
void Grx_Init()
{
	if (!CFG.gui) return;

	int ret;
	GRRLIB_texImg tx_tmp;

	// detect theme change
	extern int cur_theme;
	static int last_theme = -1;
	bool theme_change = (last_theme != cur_theme);
	last_theme = cur_theme;

	// on cover_style change, need to reload noimage cover
	// (changing cover style invalidates cache and sets ccache_inv)
	if (!grx_init || ccache_inv) {

		// nocover image
		void *img = NULL;
		tx_tmp.data = NULL;
		ret = Gui_LoadCover_style((u8*)"", &img, true, CFG.cover_style, NULL);
		if (ret > 0 && img) {
			//tx_tmp = Gui_LoadTexture_RGBA8(img, COVER_WIDTH, COVER_HEIGHT, NULL, NULL);
			tx_tmp = Gui_LoadTexture_RGBA8(img, 0, 0, NULL, NULL);
			SAFE_FREE(img);
		}
		if (tx_tmp.data == NULL) {
			tx_tmp = Gui_LoadTexture_RGBA8(coverImg, 0, 0, NULL, NULL);
		}
		GRRLIB_TextureMEM2(&tx_nocover, &tx_tmp);

		//full nocover image
		tx_store(&tx_tmp, GRRLIB_LoadTexture(coverImg_full));
		GRRLIB_TextureMEM2(&tx_nocover_full, &tx_tmp);
	}

	// on theme change we need to reload all gui resources
	if (!grx_init || theme_change) {

		// background gui
		Grx_Load_BG_Gui();

		// background console
		GRRLIB_ReallocTextureData(&tx_bg_con, BACKGROUND_WIDTH, BACKGROUND_HEIGHT);
		RGBA_to_4x4((u8*)bg_buf_rgba, (u8*)tx_bg_con.data,
				BACKGROUND_WIDTH, BACKGROUND_HEIGHT);

		Load_Theme_Texture("pointer.png", &tx_pointer, pointer);

		Load_Theme_Texture("hourglass.png", &tx_hourglass, hourglass);

		Load_Theme_Texture("favorite.png", &tx_star, star_icon);

		// Load the image file and initilise the tiles for gui font
		// default: font_uni.png
		if (!Load_Theme_Texture_1(CFG.gui_font, &tx_font)) {
			Load_Theme_Texture("font.png", &tx_font, gui_font);
		}
		GRRLIB_InitFont(&tx_font);

		// if font not found, tx_font_clock.data will be NULL
		Load_Theme_Texture_1("font_clock.png", &tx_font_clock);
		GRRLIB_InitFont(&tx_font_clock);
		GRRLIB_TrimTile(&tx_font_clock, 128);

		//store the hourglass image pasted into the noimage fullcover image
		tx_tmp = Gui_paste_into_fullcover(tx_hourglass.data, tx_hourglass.w, tx_hourglass.h,
				tx_nocover_full.data, tx_nocover_full.w, tx_nocover_full.h);
		GRRLIB_TextureMEM2(&tx_hourglass_full, &tx_tmp);
	}

	grx_init = 1;
}


void gui_tilt_pos(guVector *pos)
{
	if (gui_style == GUI_STYLE_FLOW_Z) {
		// tilt pos
		float deg_max = 45.0;
		float deg = cam_f * cam_dir * deg_max;
		float rad = DegToRad(deg);
		float zf;
		Mtx rot;

		pos->x -= cam_look.x;
		pos->y -= cam_look.y;

		guMtxRotRad(rot, 'y', rad);
		guVecMultiply(rot, pos, pos);

		zf = (cam_z + pos->z) / cam_z;
		pos->x /= zf;
		pos->y /= zf;

		pos->x += cam_look.x;
		pos->y += cam_look.y;
	}
}

void tilt_cam(guVector *cam)
{
	if (gui_style == GUI_STYLE_FLOW_Z) {
		// tilt cam
		float deg_max = 45.0;
		float deg = cam_f * cam_dir * deg_max;
		float rad = DegToRad(deg);
		Mtx rot;

		cam->x -= cam_look.x;
		cam->y -= cam_look.y;

		guMtxRotRad(rot, 'y', rad);
		guVecMultiply(rot, cam, cam);

		cam->x += cam_look.x;
		cam->y += cam_look.y;
	}
}

void Gui_set_camera(ir_t *ir, int enable)
{
	float hold_w = 50.0, out = 64; // same as get_scroll() params
	float sx;
	if (ir) {
		// is out?
		// allow x out of screen by pointer size
		if (ir->sy < 0 || ir->sy > BACKGROUND_HEIGHT
			|| ir->sx < -out || ir->sx > BACKGROUND_WIDTH+out)
		{
			if (cam_f > 0.0) cam_f -= 0.02;
			if (cam_f < 0.0) cam_f = 0.0;
		} else {
			sx = ir->sx;
			if (sx < 0.0) sx = 0.0;
			if (sx > 640.0) sx = 640.0;
			sx = sx - 320.0;
			// check hold position
			if (sx < 0) {
				cam_dir = -1.0;
				sx = -sx;
			} else {
				cam_dir = 1.0;
			}
			sx -= hold_w;
			if (sx < 0) sx = 0;
			cam_f = sx / (320.0 - hold_w);
		}
	}

	if (gui_style == GUI_STYLE_GRID || gui_style == GUI_STYLE_FLOW) {
		enable = 0;
	}
	if (enable == 0) {
		// 2D
		Mtx44 perspective;
		guOrtho(perspective,0, 480, 0, 640, 0, 300.0F);
		GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

		guMtxIdentity(GXmodelView2D);
		guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -50.0F);
		GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);
	} else { // (gui_style == GUI_STYLE_FLOW_Z)
		// 3D
		Mtx44 perspective;
		guPerspective(perspective, 45, 4.0/3.0, 0.1F, 2000.0F);
		GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

		//guVector cam  = {319.5F, 239.5F, -578.0F};
		//guVector look = {319.5F, 239.5F, 0.0F};
		guVector up   = {0.0F, -1.0F, 0.0F};
		guVector cam  = cam_look;
		cam.z = cam_z;

		// tilt cam
		tilt_cam(&cam);

		guLookAt(GXmodelView2D, &cam, &up, &cam_look);
		GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);
	}
	if (gui_style == GUI_STYLE_FLOW_Z) {
		GX_SetZMode (GX_FALSE, GX_NEVER, GX_TRUE);
	}
}


void Gui_save_cover_style()
{
	prev_cover_style = CFG.cover_style;
	prev_cover_height = COVER_HEIGHT;
	prev_cover_width = COVER_WIDTH;
	if (gui_style == GUI_STYLE_COVERFLOW) {
		if (CFG_cf_global.covers_3d) {
			if (CFG.cover_style != CFG_COVER_STYLE_FULL) {
				cfg_set_cover_style(CFG_COVER_STYLE_FULL);
			}
		} else {
			if (CFG.cover_style != CFG_COVER_STYLE_2D) {
				cfg_set_cover_style(CFG_COVER_STYLE_2D);
			}
		}
	}
}

/**
 * Resets the cover style to the previous one.  Primarily used when 
 *  switching from full covers.
 */
void Gui_reset_previous_cover_style() {
	if (gui_style == GUI_STYLE_COVERFLOW) {
		//flip the cover style mode back to whatever it was
		cache_release_all();
		cache_wait_idle();
		cfg_set_cover_style(prev_cover_style);
		//set cover width and height back to original
		COVER_HEIGHT = prev_cover_height;
		COVER_WIDTH = prev_cover_width;
	}
}


void Gui_draw_background_alpha2(u32 color1, u32 color2)
{
	Gui_set_camera(NULL, 0);
	//void GRRLIB_DrawSlice2(f32 xpos, f32 ypos, GRRLIB_texImg tex, float degrees,
	//	float scaleX, f32 scaleY, u32 color1, u32 color2,
	//	float x, float y, float w, float h)

	// top half
	GRRLIB_DrawSlice2(0, 0, tx_bg, 0,
		1, 1, color1, color1,
		0, 0, tx_bg.w, tx_bg.h / 2);

	// bottom half
	GRRLIB_DrawSlice2(0, tx_bg.h / 2, tx_bg, 0,
		1, 1, color1, color2,
		0, tx_bg.h / 2, tx_bg.w, tx_bg.h / 2);

	Gui_set_camera(NULL, 1);
}

void Gui_draw_background_alpha(u32 color)
{
	Gui_set_camera(NULL, 0);
	GRRLIB_DrawImg(0, 0, &tx_bg, 0, 1, 1, color);
	Gui_set_camera(NULL, 1);
}

void Gui_draw_background()
{
	Gui_draw_background_alpha(0xFFFFFFFF);
}

void Gui_draw_pointer(ir_t *ir)
{
	// reset camera
	Gui_set_camera(NULL, 0);
	GX_SetZMode (GX_FALSE, GX_NEVER, GX_TRUE);
	// draw pointer
	GRRLIB_DrawImg(ir->sx - tx_pointer.w / 2, ir->sy - tx_pointer.h / 2,
			&tx_pointer, ir->angle, 1, 1, 0xFFFFFFFF);
}

void Repaint_ConBg(bool exiting)
{
	int fb;
	void **xfb;
	GRRLIB_texImg tx;
	void *img_buf;

	xfb = _GRRLIB_GetXFB(&fb);
	_Video_SetFB(xfb[fb^1]);
	Video_Clear(COLOR_BLACK);

	if (exiting) return;
	
	// background
	Video_DrawBg();

	// enable console if it was disabled till now
	Gui_Console_Enable();

	// Cover
	if (CFG.covers == 0) return;
	if (gameCnt == 0) return;
	
	int state;
	GRRLIB_texImg *txp = cache_request_cover(gameSelected,
			CFG.cover_style, CC_FMT_C4x4, CC_PRIO_NONE, &state);
	if (txp) {
		if ((txp->w != COVER_WIDTH) || (txp->h != COVER_HEIGHT)) {
			txp = NULL;
		}
	}
	if (!txp) {
		extern void __Menu_ShowCover();
		__Menu_ShowCover();
		return;
	}
	tx = *txp;

	img_buf = memalign(32, tx.w * tx.h * 4);
	if (img_buf == NULL) return;
	C4x4_to_RGBA(tx.data, img_buf, tx.w, tx.h);
	DrawCoverImg(gameList[gameSelected].id, img_buf, tx.w, tx.h);
	//Video_CompositeRGBA(COVER_XCOORD, COVER_YCOORD, img_buf, tx.w, tx.h);
	//Video_DrawRGBA(COVER_XCOORD, COVER_YCOORD, img_buf, tx.w, tx.h);
	free(img_buf); 
}

void Gui_Refresh_List()
{
	cache_release_all();
	cache_num_game();
	if (gui_style == GUI_STYLE_COVERFLOW) {
		gameSelected = Coverflow_initCoverObjects(gameCnt, 0, true);
		//load the covers - alternate right and left
		cache_request_before_and_after(gameSelected, 5, 1);
	} else {
		//grid_init(page_gi);
		grid_init(0);
	}
}

void Gui_Action_Favorites()
{
	reset_sort_default();
	Switch_Favorites(!enable_favorite);
	Gui_Refresh_List();
}

extern char action_string[40];
extern int action_alpha;

void Gui_Sort_Set(int index, bool desc)
{
	sortList_set(index, desc);
	Gui_Refresh_List();
	sprintf(action_string, gt("Sort: %s-%s"), sortTypes[sort_index].name, (sort_desc) ? "DESC":"ASC");
	action_alpha = 0xFF;
}

void Gui_Action_Sort()
{
	if (sort_desc) {
		sort_desc = 0;
		if (sort_index == sortCnt - 1)
			sort_index = 0;
		else
			sort_index = sort_index + 1;
	} else {
		sort_desc = 1;
	}
	Gui_Sort_Set(sort_index, sort_desc);
}

void Gui_Action_Profile(int n)
{
	if (CFG.current_profile == n) return;
	CFG.current_profile = n;
	if (CFG.current_profile >= CFG.num_profiles) CFG.current_profile = CFG.num_profiles - 1;
	if (CFG.current_profile < 0 ) CFG.current_profile = 0;
	reset_sort_default();
	Switch_Favorites(CFG.profile_start_favorites[CFG.current_profile]);
	action_Theme_2(CFG.profile_theme[CFG.current_profile]);
	Gui_Refresh_List();
	sprintf(action_string, gt("Profile: %s"), CFG.profile_names[CFG.current_profile]);
	action_alpha = 0xFF;
}

int Gui_DoAction(int action, ir_t *ir)
{
	if (action & CFG_BTN_REMAP) return 0; 
	switch(action) {
		case CFG_BTN_NOTHING:
			return 0;
		case CFG_BTN_OPTIONS: 
		case CFG_BTN_GLOBAL_OPS:
			return !CFG.disable_options;
		case CFG_BTN_GUI:
		case CFG_BTN_INSTALL:
		case CFG_BTN_REMOVE:
		case CFG_BTN_MAIN_MENU: 
		case CFG_BTN_UNLOCK:
		case CFG_BTN_BOOT_DISC:
		case CFG_BTN_BOOT_GAME:
		case CFG_BTN_THEME:
		case CFG_BTN_FILTER:
			return 1;
		case CFG_BTN_REBOOT:
		case CFG_BTN_EXIT:
		case CFG_BTN_HBC:
			//exiting = true;
			__console_disable = 1;
			return -1;
		case CFG_BTN_SCREENSHOT:
			{ 
				char fn[200];
				extern bool Save_ScreenShot(char *fn, int size);
				Save_ScreenShot(fn, sizeof(fn));
				return 0;
			}
			
		case CFG_BTN_PROFILE: 
			{
				int n = CFG.current_profile + 1;
				if (n >= CFG.num_profiles) n = 0;
				Gui_Action_Profile(n);
			}
			return 0;
			
		case CFG_BTN_FAVORITES:
			Gui_Action_Favorites();
			return 0;
			
		case CFG_BTN_SORT:
			Gui_Action_Sort();
			return 0;
			
		case CFG_BTN_RANDOM:
			//pick a random cover and scroll to it
			if (gui_style == GUI_STYLE_COVERFLOW) {
				int i, newIdx, ret, buttons;
				//this is needed so the easing slowdown doesn't occur in Coverflow_drawCovers
				extern int rotating_with_wiimote;

				newIdx = (rand() >> 16) % gameCnt;
				i = abs(gameSelected - newIdx);
				if (i > gameCnt/2) {
					//wrap around scrolling
					i = (newIdx > gameSelected) ? CF_TRANS_ROTATE_LEFT : CF_TRANS_ROTATE_RIGHT;
				} else { 
					i = (newIdx < gameSelected) ? CF_TRANS_ROTATE_LEFT : CF_TRANS_ROTATE_RIGHT;
				}
				//init rotation
				while (1) {
					buttons = Wpad_GetButtons();
					rotating_with_wiimote = 1;
					ret = Coverflow_init_transition(i, 100, gameCnt, false);
					if (ret > 0) gameSelected = ret;
					cache_release_all();
					cache_request_before_and_after(gameSelected, 5, 1);
					Coverflow_drawCovers(ir, gameCnt, false);
					if (CFG.debug == 3) {
						GRRLIB_Printf(50, 360, &tx_font, CFG.gui_text.color, 1,
								"looking for: %d  gameCnt: %d", newIdx, gameCnt);
					}
					Gui_Render();
					if (ret == newIdx) break;
					if ((buttons & WPAD_BUTTON_B) || (buttons & WPAD_BUTTON_A)) break;
				}
				return 0;
			} else {
				return 1;
			}

		default:
			// Magic Word or Channel
			if ((action & 0xFF) < 'a') {
				__console_disable = 1;
				return -1;
			} else {
				return 1;
			}
	}
}


int Gui_Switch_Style(int new_style, int sub_style)
{
	if (game_select >= 0) gameSelected = game_select;

	if (gui_style < GUI_STYLE_COVERFLOW) {

		if (new_style < GUI_STYLE_COVERFLOW) {

			// 1. from grid/flow to grid/flow
			if (new_style == gui_style && sub_style == grid_rows) {
				// nothing to do
				return 0;
			}
			grid_transit_style(new_style, sub_style);
			return 1;

		} else {

			// 2. from grid/flow to coverflow
			gui_style = new_style;
			CFG_cf_global.theme = sub_style;

			//Coverflow uses full covers ONLY
			// invalidate the cache and reload
			cache_release_all();
			cache_wait_idle();
			Gui_save_cover_style();
			Cache_Invalidate();
			Cache_Init();
			Coverflow_Grx_Init();
			gameSelected = Coverflow_initCoverObjects(gameCnt, gameSelected, false);
			//load the covers - alternate right and left
			cache_request_before_and_after(gameSelected, 5, 1);
		}

	} else {
		
		if (new_style == GUI_STYLE_COVERFLOW) {

			// 3. from coverflow to coverflow
			if (CFG_cf_global.theme == sub_style) {
				// nothing to do
				return 0;
			}
			CFG_cf_global.theme = sub_style;
			//setup the new coverflow style
			gameSelected = Coverflow_initCoverObjects(gameCnt, game_select, true);
			if (gameSelected == -1) {
				//fatal error - couldn't allocate memory
				gui_style = GUI_STYLE_GRID;
				grid_init(0);
				return 1;
			}
			Coverflow_init_transition(CF_TRANS_MOVE_TO_POSITION, 100, gameCnt, false);

		} else {

			// 4. from coverflow to grid/flow 
			Gui_reset_previous_cover_style();
			gui_style = new_style;
			grid_rows = sub_style;
			Cache_Invalidate();
			Cache_Init();
			grid_init(gameSelected);
			return 1;
		}
	}


	return 0;
}

int Gui_Shift_Style(int change)
{
	int rows = grid_rows + change;
	int new_style = gui_style;

	if (game_select >= 0) gameSelected = game_select;

	if (CFG.gui_lock) {
		// if gui style locked, only allow to change number of rows
		if (gui_style < GUI_STYLE_COVERFLOW) {
			if (rows >= 1 && rows <= 4) {
				Gui_Switch_Style(gui_style, rows);
				return 1;
			}
		}
	} else {

		// transition gui style:
		// grid -> flow -> flow-z -> coverflow modes -> grid -> ...

		if (gui_style < GUI_STYLE_COVERFLOW) {
			// grid mode
			if (rows < 1 || rows > 4) {
				rows = grid_rows;
				new_style = gui_style + 1;
			}
			if (new_style < GUI_STYLE_COVERFLOW) {
				// grid - grid
				Gui_Switch_Style(new_style, rows);
			} else {
				// grid - coverflow
				Gui_Switch_Style(new_style, coverflow3d);
			}
		} else {
			// coverflow mode
			int subs = CFG_cf_global.theme + 1;
			if (subs >= 0 && subs <= carousel) {
				// coverflow - coverflow
				Gui_Switch_Style(gui_style, subs);
			} else {
				// coverflow - grid
				if (subs < 0) new_style = gui_style--;
				else new_style = GUI_STYLE_GRID;
				Gui_Switch_Style(new_style, grid_rows);
				return 1;
			}
		}

	} // CFG.gui_lock

	return 0;
}

/**
 * This is the main control loop for the 3D cover modes.
 */
int Gui_Mode()
{
	int buttons = 0;
	int buttons_held = 0;
	ir_t ir;
	int fr_cnt = 0;
	bool exiting = false;
	bool suppressWiiPointerChecking = false;
	int ret = 0;

	//check for console mode
	if (CFG.gui == 0) return 0;
	
	memcheck();
	//load all the commonly used images (background, pointers, etc)
	// although already called at startup it needs to be called again
	// in case theme changed
	Grx_Init();
	//memcheck();

	// setup gui style
	static int first_time = 1;
	if (first_time) {
		gui_style = CFG.gui_style;
		// CFG.gui_style has all the styles unified
		// split coverflow styles out:
		if (gui_style >= GUI_STYLE_COVERFLOW) {
			gui_style = GUI_STYLE_COVERFLOW;
			CFG_cf_global.theme = CFG.gui_style - GUI_STYLE_COVERFLOW;
		}
		grid_rows = CFG.gui_rows;
	}

	// start the cache thread
	if (Switch_Console_To_Gui()) {
		// cache error, return to console
		go_gui = 0;
		Switch_Gui_To_Console(false);
		return 0;
	}

	//initilaize the GRRLIB video subsystem
	Gui_Init();

	if (gui_style == GUI_STYLE_COVERFLOW) {

		Coverflow_Grx_Init();

		game_select = -1;
		
		gameSelected = Coverflow_initCoverObjects(gameCnt, gameSelected, false);

		//load the covers - alternate right and left
		cache_request_before_and_after(gameSelected, 5, 1);
		
		if (!first_time) {
			if (prev_cover_style == CFG_COVER_STYLE_3D)
				Coverflow_init_transition(CF_TRANS_MOVE_FROM_CONSOLE_3D, 100, gameCnt, false);
			else
				Coverflow_init_transition(CF_TRANS_MOVE_FROM_CONSOLE, 100, gameCnt, false);
		}
		
	} else {
	
		//allocate the grid and initialize the grid cover settings
		grid_init(gameSelected);
	
		game_select = -1;
	
		if (!first_time) {
			//fade in the grid
			grid_transition(1, page_gi);
		}
	}
	
	if (first_time) {
		first_time = 0;
		Gui_TestUnicode();
		//Gui_BenchText();
	}

	wgui_desk_init();

	while (1) {

		restart:

		dbg_time_usec();

		buttons = Wpad_GetButtons();
		buttons_held = Wpad_Held();
		Wpad_getIR(&ir);

		wgui_desk_handle(&ir, &buttons, &buttons_held);
		if (!go_gui) break;

		//----------------------
		//  HOME Button
		//----------------------

		//if ((buttons & CFG.button_exit.mask) && Gui_DoAction(CFG.button_H)) {
		//	/*if (CFG.home == CFG_HOME_SCRSHOT) {
		//		char fn[200];
		//		extern bool Save_ScreenShot(char *fn, int size);
		//		Save_ScreenShot(fn, sizeof(fn));
		//	} else {
		//		exiting = true;
		//		__console_disable = 1;
		//		break;
		//	}*/
		//	if (game_select >= 0) gameSelected = game_select;
		//	break;
		//}


		//----------------------
		//  A Button
		//----------------------

		if (loadGame) {
			buttons = buttonmap[MASTER][CFG.button_confirm.num];
			loadGame = false;
		}

		if (buttons & CFG.button_confirm.mask) {
			if (game_select >= 0) {
				gameSelected = game_select;
				break;
			}
		}


		//----------------------
		//  B Button
		//----------------------

		//if ((buttons & CFG.button_cancel.mask) && Gui_DoAction(CFG.button_B)) {
		//	if (game_select >= 0) gameSelected = game_select;
		//	break;
		//}


		//----------------------
		//  dpad RIGHT
		//----------------------

		if (buttons & WPAD_BUTTON_RIGHT) {
			if (gui_style == GUI_STYLE_COVERFLOW) {
				ret = Coverflow_init_transition(CF_TRANS_ROTATE_RIGHT, 0, gameCnt, true);
				if (ret > 0) {
					game_select = ret;
					cache_release_all();
					cache_request_before_and_after(game_select, 5, 1);
				}
				suppressWiiPointerChecking = true;
			} else {
				if (grid_page_change(1)) goto restart;
			}
		} else if (buttons_held & WPAD_BUTTON_RIGHT) {
			if (gui_style == GUI_STYLE_COVERFLOW) {
				ret = Coverflow_init_transition(CF_TRANS_ROTATE_RIGHT, 100, gameCnt, true);
				if (ret > 0) {
					game_select = ret;
					cache_release_all();
					cache_request_before_and_after(game_select, 5, 1);
				}
				suppressWiiPointerChecking = true;
			}
		}


		//----------------------
		//  dpad LEFT
		//----------------------

		if (buttons & WPAD_BUTTON_LEFT) {
			if (gui_style == GUI_STYLE_COVERFLOW) {
				ret = Coverflow_init_transition(CF_TRANS_ROTATE_LEFT, 0, gameCnt, true);
				if (ret > 0) {
					game_select = ret;
					cache_release_all();
					cache_request_before_and_after(game_select, 5, 1);
				}
				suppressWiiPointerChecking = true;
			} else {
				if (grid_page_change(-1)) goto restart;
			}
		} else if (buttons_held & WPAD_BUTTON_LEFT) {
			if (gui_style == GUI_STYLE_COVERFLOW) {
				ret = Coverflow_init_transition(CF_TRANS_ROTATE_LEFT, 100, gameCnt, true);
				if (ret > 0) {
					game_select = ret;
					cache_release_all();
					cache_request_before_and_after(game_select, 5, 1);
				}
				suppressWiiPointerChecking = true;
			}
		}


		//----------------------
		//  dpad UP
		//----------------------
		
		if (buttons & WPAD_BUTTON_UP) {
			if (gui_style == GUI_STYLE_COVERFLOW) {
				game_select = Coverflow_flip_cover_to_back(true, 2);
			} else {
				if (Gui_Shift_Style(1)) goto restart;
			}
		}
		
		//----------------------
		//  dpad DOWN
		//----------------------

		if (buttons & WPAD_BUTTON_DOWN) {
			if (Gui_Shift_Style(-1)) goto restart;
		}

		// need to return to menu for timeout
		if (CFG.admin_lock) {
			if (buttons & CFG.button_other.mask) {
				if (game_select >= 0) gameSelected = game_select;
				goto return_to_console;
			}
		}

		//----------------------
		//  (CFG) button
		//----------------------

		int i;
		for (i = 4; i < MAX_BUTTONS; i++) {
			if ((buttons & buttonmap[MASTER][i]) && (ret = Gui_DoAction(*(&CFG.button_M + (i - 4)), &ir))) {
				if (ret < 0) exiting = true;
				else if (game_select >= 0) gameSelected = game_select;
				goto return_to_console;
			}
		}

		//----------------------
		//  check for other wiimote events
		//----------------------
		if (gui_style == GUI_STYLE_FLOW
				|| gui_style == GUI_STYLE_FLOW_Z) {
			// scroll flow style
			grid_get_scroll(&ir);
		} else if (gui_style == GUI_STYLE_COVERFLOW) {
			if (!suppressWiiPointerChecking)
				game_select = Coverflow_process_wiimote_events(&ir, gameCnt);
			else
				suppressWiiPointerChecking = false;
		}
		
		//if we should load a game then do
		//that instead of drawing the next frame
		if (loadGame || suppressCoverDrawing) {
			suppressCoverDrawing = false;
			goto restart;
		}
		//draw the covers
		if (gui_style == GUI_STYLE_COVERFLOW) {
			game_select = Coverflow_drawCovers(&ir, gameCnt, true);
		} else {
			//draw the background
			Gui_draw_background();
			
			grid_calc();
			game_select = grid_update_all(&ir);
			Gui_set_camera(&ir, 1);
			grid_draw(game_select);
			// title
			grid_print_title(game_select);
		}

		wgui_desk_render(&ir, &buttons);

		int us = dbg_time_usec();
		if (CFG.debug == 3)
		{
			GRRLIB_Printf(20, 50, &tx_font, 0xFF00FFFF, 1,
					"%4d ms:%6.2f", fr_cnt, (float)us/1000.0);
		}
		/*
		Gui_Printf(50, 390, "r x:%6.1f y:%6.1f v: %d %d a:%6.1f",
				ir.ax, ir.ay, ir.raw_valid, ir.state, ir.angle);
		Gui_Printf(50, 410, "s x:%6.1f y:%6.1f v: %d", ir.sx, ir.sy, ir.smooth_valid);
		Gui_Printf(50, 430, "b x:%6.1f y:%6.1f v: %d", ir.x, ir.y, ir.valid);
		*/
		//draw_Cache();
		//GRRLIB_DrawImg(0, 50, tx_font, 0, 1, 1, 0xFFFFFFFF); // entire font
		//Gui_FillAscii(10, 10, 0xffffffff, 1);
		Gui_draw_pointer(&ir);
		//GRRLIB_Rectangle(fr_cnt % 640, 0, 5, 15, 0x000000FF, 1);
		Gui_Render();

		fr_cnt++;
	}
return_to_console:
	if (gui_style == GUI_STYLE_COVERFLOW) {
		// this is for when transitioning from coverflow to Console...
		if (!exiting) {
			if (prev_cover_style == CFG_COVER_STYLE_3D)
				Coverflow_init_transition(CF_TRANS_MOVE_TO_CONSOLE_3D, 100, gameCnt, false);
			else
				Coverflow_init_transition(CF_TRANS_MOVE_TO_CONSOLE, 100, gameCnt, false);
		}
	} else {
		// this is for when transitioning from Grid to Console...
		//transition(-1, page_i*grid_covers);
		if (!exiting)
			grid_transition(-1, page_gi);
	}
	wgui_desk_close();
	Switch_Gui_To_Console(exiting);
	
	return buttons;
}

int Switch_Console_To_Gui()
{
	__console_flush(0);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	Gui_save_cover_style();
	gui_mode = 1;
	__console_disable = 1;
	// reinit cache if it was invalidated
	int ret = Cache_Init();
	memcheck();
	return ret;
}

void Switch_Gui_To_Console(bool exiting)
{
	cache_release_all();
	//reset to the previous style before leaving
	Gui_reset_previous_cover_style();
	Con_Clear();
	__console_flush(0);
	Repaint_ConBg(exiting);
	// restore original framebuffer
	_Video_SyncFB();
	_GRRLIB_SetFB(0);
	gui_mode = 0;
	if (!exiting) __console_disable = 0;
}


void Gui_Close()
{
	Cache_Close();

	if (CFG.gui == 0) return;
	if (!grr_init) return;

	dbg_printf("Gui_Close\n");	
	__console_flush(0);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	_GRRLIB_Exit(); 
	

	SAFE_FREE(tx_pointer.data);
	SAFE_FREE(tx_font.data);

	Coverflow_Close();

	grr_init = 0;
}

