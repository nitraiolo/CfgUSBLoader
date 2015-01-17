#ifndef _GUI_H_
#define _GUI_H_

#include "sys/stat.h"

#include "wpad.h"
#include "my_GRRLIB.h"
#include "cfg.h"

#define BACKGROUND_WIDTH	640
#define BACKGROUND_HEIGHT	480

extern int gui_mode;
extern int gui_style;
extern int grid_rows;

/*
struct M2_texImg
{
	GRRLIB_texImg tx;
	int size;
};
*/

extern GRRLIB_texImg tx_bg;
extern GRRLIB_texImg tx_bg_con;
extern GRRLIB_texImg tx_nocover;
extern GRRLIB_texImg tx_nocover_full;
extern GRRLIB_texImg tx_pointer;
extern GRRLIB_texImg tx_hourglass;
extern GRRLIB_texImg tx_star;
extern GRRLIB_texImg tx_font;


// FLOW_Z camera
extern float cam_f;
extern float cam_dir;
extern float cam_z;
extern guVector cam_look;

extern int game_select;


/* Prototypes */
void Gui_InitConsole(void);
void Gui_Console_Alert(void);
void Gui_DrawBackground(void);	//draws the console mode background
void Gui_DrawIntro(void);
void Gui_draw_background_alpha2(u32 color1, u32 color2);
void Gui_draw_background_alpha(u32);  //draws the GX mode (grid/coverflow/etc) background
void Gui_draw_background(void);	//draws the GX mode (grid/coverflow/etc) background
void Gui_draw_pointer(ir_t *ir);
void Gui_DrawCover(u8 *discid);
int Gui_LoadCover_style(u8 *discid, void **p_imgData, bool load_noimage, int cover_style, char *path);
s32 __Gui_DrawPng(void *img, u32 x, u32 y);
void Gui_LoadBackground(void);
void Gui_DrawThemePreview(char *name, int id);
void Gui_DrawThemePreviewLarge(char *name, int id);

int Load_Theme_Image2(char *name, void **img_buf, bool global);
int Load_Theme_Image(char *name, void **img_buf);
int find_cover_path(u8 *id, int cover_style, char *path, int size, struct stat *st);
GRRLIB_texImg Gui_LoadTexture_RGBA8(const unsigned char my_png[], int, int, void *dest, char *path);
GRRLIB_texImg Gui_LoadTexture_CMPR(const unsigned char my_png[], int, int, void *dest, char *path);
GRRLIB_texImg Gui_LoadTexture_fullcover(const unsigned char my_png[], int, int, int, void *dest, char *path);
GRRLIB_texImg Gui_paste_into_fullcover(void *src, int src_w, int src_h, void *dest, int dest_w, int dest_h);
u32 upperPower(u32 width);
GRRLIB_texImg Gui_LoadTexture_MIPMAP(const unsigned char my_png[],
		int width, int height, int maxlod, void *dest, char *path);

s32 __Gui_GetPngDimensions(void *img, u32 *w, u32 *h);
void Gui_set_camera(ir_t *ir, int);
void Gui_Wpad_IR(int, struct ir_t *ir);
void gui_tilt_pos(guVector *pos);
/*
void cache2_tex(struct M2_texImg *dest, GRRLIB_texImg *src);
void cache2_tex_alloc(struct M2_texImg *dest, int w, int h);
void cache2_tex_alloc_fullscreen(struct M2_texImg *dest);
void cache2_GRRLIB_tex_alloc(GRRLIB_texImg *dest, int w, int h);
void cache2_GRRLIB_tex_alloc_fullscreen(GRRLIB_texImg *dest);
*/

void Gui_PrintAlignZ(float x, float y, int alignx, int aligny,
		GRRLIB_texImg *font, FontColor font_color, float zoom, char *str);
void Gui_PrintAlign(int x, int y, int alignx, int aligny,
		GRRLIB_texImg *font, FontColor font_color, char *str);
void Gui_PrintEx2(int x, int y, GRRLIB_texImg *font,
		int color, int color_outline, int color_shadow, const char *str);
void Gui_PrintEx(int x, int y, GRRLIB_texImg *font, FontColor font_color, const char *str);
void Gui_PrintfEx(int x, int y, GRRLIB_texImg *font, FontColor font_color, char *fmt, ...);
void Gui_Printf(int x, int y, char *fmt, ...);
void Gui_Print2(int x, int y, const char *str);
void Gui_Print(int x, int y, char *str);
void Gui_Print_Clock(int x, int y, FontColor font_color, int align);

u32 color_add(u32 c1, u32 c2, int neg);
u32 color_multiply(u32 c1, u32 c2);
void font_color_multiply(FontColor *src, FontColor *dest, u32 color);

void Grx_Init();
void Gui_DrawImgFullScreen(GRRLIB_texImg *tex, const u32 color, bool antialias);
void Gui_RenderAAPass(int aaStep);
void Gui_Render();
void Gui_Render_Out(ir_t *ir);
void Gui_Init();
int  Gui_Mode();
void Gui_Close();

void Gui_Refresh_List();
void Gui_Action_Favorites();
void Gui_Sort_Set(int index, bool desc);
void Gui_Action_Sort();
int Gui_Switch_Style(int new_style, int sub_style);
int Gui_Shift_Style(int change);
void Gui_Action_Profile(int n);
int Switch_Console_To_Gui();
void Switch_Gui_To_Console(bool exiting);
void Gui_save_cover_style();
void Gui_reset_previous_cover_style();

#endif
