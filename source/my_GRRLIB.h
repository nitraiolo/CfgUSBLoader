#ifndef __MY_GRR_H__
#define __MY_GRR_H__

#include "util.h"
#include "grrlib.h"

//#define tex_format nbtilew
//#define tex_lod    nbtileh

#define GRRLIB_GetColor RGBA

extern int aa_method;
extern Mtx GXmodelView2D;

GRRLIB_texImg my_GRRLIB_LoadTextureJPG(const unsigned char my_jpg[]);

//stencil routines
void GRRLIB_prepareStencil(void);
void GRRLIB_renderStencil_buf(GRRLIB_texImg *tx);
int GRRLIB_stencilVal(int x, int y, GRRLIB_texImg tx);
inline void GRRLIB_DrawImg_format(f32 xpos, f32 ypos, GRRLIB_texImg tex, u8 texFormat, float degrees, float scaleX, f32 scaleY, u32 color );

//anti-alias routines
GRRLIB_texImg GRRLIB_AAScreen2Texture();
void GRRLIB_AAScreen2Texture_buf(GRRLIB_texImg *tx, u8 gx_clear);
void GRRLIB_prepareAAPass(int, int);
void GRRLIB_drawAAScene(int, GRRLIB_texImg *texAABuffer);
void GRRLIB_ResetVideo(void);

void GRRLIB_DrawImgRect(float x, float y, float w, float h, GRRLIB_texImg *tex, const u32 color);
void GRRLIB_DrawImgNoAA(const f32 xpos, const f32 ypos, const GRRLIB_texImg *tex,
		const f32 degrees, const f32 scaleX, const f32 scaleY, const u32 color);
extern void GRRLIB_DrawSlice2(f32 xpos, f32 ypos, GRRLIB_texImg tex,
		float degrees, float scaleX, f32 scaleY, u32 color1, u32 color2,
		float x, float y, float w, float h);
extern void GRRLIB_DrawSlice(f32 xpos, f32 ypos, GRRLIB_texImg tex,
		float degrees, float scaleX, f32 scaleY, u32 color,
		float x, float y, float w, float h);
void  GRRLIB_DrawPartQuad (const guVector pos[4], const GRRLIB_texImg *tex,
		const f32 partx, const f32 party, const f32 partw, const f32 parth,
		const u32 color);
void  GRRLIB_DrawTileRectQuad (const guVector pos[4], GRRLIB_texImg *tex, const u32 color, const int frame, int nx, int ny);

void GRRLIB_Print4(f32 xpos, f32 ypos, struct GRRLIB_texImg *tex, u32 color, u32 outline, u32 shadow, f32 scale, const char *text, int maxlen);
void GRRLIB_Print3(f32 xpos, f32 ypos, struct GRRLIB_texImg *tex, u32 color, u32 outline, u32 shadow, f32 scale, const char *text);
void GRRLIB_Print2(f32 xpos, f32 ypos, struct GRRLIB_texImg *tex, u32 color, u32 outline, u32 shadow, const char *text);
void GRRLIB_Print(f32 xpos, f32 ypos, struct GRRLIB_texImg *tex, u32 color, const char *text);

extern int GRRLIB_Init_VMode(GXRModeObj *a_rmode, void *fb0, void *fb1);
extern void** _GRRLIB_GetXFB(int *cur_fb);
extern void _GRRLIB_SetFB(int cur_fb);
extern void _GRRLIB_Render();
extern void _GRRLIB_VSync();
extern void _GRRLIB_Exit();

void tx_store(struct GRRLIB_texImg *dest, struct GRRLIB_texImg *src);

#define GRRLIB_FREE_TEX(tex) do{ if (tex) GRRLIB_FreeTexture(tex); tex = NULL; }while(0)

u32 fixGX_GetTexBufferSize(u16 wd, u16 ht, u32 fmt, u8 mipmap, u8 maxlod);

int GRRLIB_DataSize(uint w, uint h, int fmt, int lod);
int GRRLIB_TextureSize(GRRLIB_texImg *tx);
bool GRRLIB_AllocTextureDataX(GRRLIB_texImg *tx, int w, int h, int fmt, int lod);
bool GRRLIB_AllocTextureData(GRRLIB_texImg *tx, const uint w, const uint h);
bool GRRLIB_ReallocTextureData(GRRLIB_texImg *tx, const uint w, const uint h);
void GRRLIB_FreeTextureData(GRRLIB_texImg *tex);
bool GRRLIB_CloneTexture(GRRLIB_texImg *dest, GRRLIB_texImg *src);
bool GRRLIB_TextureMEM2(GRRLIB_texImg *dest, GRRLIB_texImg *src);
bool GRRLIB_TextureToMEM2(GRRLIB_texImg *tx);

#endif

