#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <ogcsys.h>
#include "pngu/pngu.h"

/* Prototypes */
void Con_Init(u32, u32, u32, u32);
void Con_Clear(void);
void Con_ClearLine(void);
void Con_FgColor(u32, u8);
void Con_BgColor(u32, u8);
void Con_FillRow(u32, u32, u8);
void Con_SetPosition(int col, int row);

void Video_Configure(GXRModeObj *);
void Video_SetMode(void);
void Video_Clear(s32);
void Video_DrawPng(IMGCTX, PNGUPROP, u16, u16);
void Video_SetWide();

void FgColor(int color);
void BgColor(int color);
void DefaultColor();
bool ScreenShot(char *fname);
int Video_AllocBg();
void Video_SaveBgRGBA();
s32 Video_SaveBg(void *img);
void Video_DrawBg();
void Video_GetBG(int x, int y, char *img, int width, int height);
void Video_CompositeRGBA(int x, int y, char *img, int width, int height);
void Video_DrawRGBA(int x, int y, char *img, int width, int height);
void RGBA_to_YCbYCr(char *dest, char *img, int width, int height);
extern GXRModeObj* _Video_GetVMode();
void* _Video_GetFB(int n);
void _Video_SetFB(void *fb);
void _Video_SyncFB();

extern int __console_scroll;
void __console_enable(void *fb);
void __console_flush(int retrace_min);
void Gui_Console_Enable();
s32 CON_InitTr(GXRModeObj *rmode, s32 conXOrigin,s32 conYOrigin,s32 conWidth,s32 conHeight, s32 bgColor);

#endif
