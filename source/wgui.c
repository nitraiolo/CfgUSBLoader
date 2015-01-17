
// by oggzee

// wii gui

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>

#include "wpad.h"
#include "my_GRRLIB.h"
#include "gui.h"
#include "cfg.h"
#include "wgui.h"
#include "gettext.h"

#include "button_png.h"
#include "button_old_png.h"
#include "window_png.h"

/*

Resources:

button.png
window.png

*/

#define WGUI_IMG_NORMAL      0
#define WGUI_IMG_FLASH       1
#define WGUI_IMG_HOVER       2
#define WGUI_IMG_PRESS       3
#define WGUI_IMG_PRESS_HOVER 4
#define WGUI_IMG_INACTIVE    5

GRRLIB_texImg *tx_button;
GRRLIB_texImg *tx_checkbox;
GRRLIB_texImg *tx_radio;
GRRLIB_texImg *tx_window;
GRRLIB_texImg *tx_page;
GRRLIB_texImg *tx_custom[GUI_BUTTON_NUM];

FontColor wgui_fc = { 0xFFFFFFFF, 0xA0, 0x44444444 }; // GUI_TC_MENU
FontColor text_fc = { 0xFFFFFFFF, 0xA0, 0 }; // GUI_TC_INFO
FontColor about_fc = { 0xFFFFFFFF, 0xFF }; // GUI_TC_ABOUT
u32 disabled_color = 0xB0B0B0A0;

const Pos pos_auto =
{
	POS_AUTO, POS_AUTO,
	SIZE_AUTO, SIZE_AUTO,
	0, 0
};

const Pos pos_fill =
{
	POS_AUTO, POS_AUTO,
	SIZE_FULL, SIZE_FULL,
	0, 0
};

const Pos pos_full =
{
	0, 0,
	SIZE_FULL, SIZE_FULL,
	0, 0
};

#define P_ALL(P) P.x, P.y, P.w, P.h

struct Wgui_Input winput;

static struct {
	int buttons;
	float sx, sy;
	int smooth_valid;
	int valid;
} save_input;


int wgui_inited = 0;

void wgui_init_button_press(GRRLIB_texImg *texsrc, GRRLIB_texImg *texdest,
		int src_h, int dest_y)
{
	unsigned int x, y;
	u32 color;
	
	for(y=0; y < src_h; y++) {
		for(x=0; x < texsrc->w; x++) {
			color = GRRLIB_GetPixelFromtexImg(x, y, texsrc);
			GRRLIB_SetPixelTotexImg(x, dest_y + y, texdest, (0xFFFFFF00 | (color & 0xFF)));
		}
	}
	GRRLIB_FlushTex(texdest);
}

void wgui_init_button(GRRLIB_texImg *tx)
{
	if (!tx) return;
	// ratio w:h of a single button image is 2:1
	int h = tx->w / 2;
	GRRLIB_InitTileSet(tx, tx->w/4, h, 0);
	// nbtileh is number of button images inside the texture
	if (tx->nbtileh == 1) {
		// single button: resize and
		// create "flash" version of the button for button presses
		void *data = mem_realloc(tx->data, GRRLIB_DataSize(tx->w, tx->w, 0, 0));
		if (!data) return;
		tx->data = data;
		tx->h = tx->w;
		wgui_init_button_press(tx, tx, h, h);
		GRRLIB_InitTileSet(tx, tx->w/4, h, 0);
	}
}

// iw,ih: icon size on screen
void wgui_init_icon(GRRLIB_texImg *tx, int iw, int ih)
{
	if (!tx) return;
	// w:h of a single button has same proportions as iw:ih
	// w:h = iw:ih h=w/iw*ih
	float fh = (float)tx->w / (float)iw * (float)ih;
	int num = (int)roundf((float)tx->h / fh);
	if (num < 1) num = 1;
	if (num > 6) num = 6;
	int h = tx->h / num;
	// num is number of icon images inside the texture
	if (num == 1) {
		// single icon: resize and
		// create "flash" version of the button for button presses
		void *data = mem_realloc(tx->data, GRRLIB_DataSize(tx->w, h*2, 0, 0));
		if (!data) return;
		tx->data = data;
		tx->h = h * 2;
		wgui_init_button_press(tx, tx, h, h);
	}
	GRRLIB_InitTileSet(tx, tx->w, h, 0);
}

int wgui_load_texture1(GRRLIB_texImg **ptx, char *name, bool global)
{
	GRRLIB_texImg *tx = NULL;
	void *img_data = NULL;
	GRRLIB_FREE_TEX(*ptx);
	int ret = Load_Theme_Image2(name, &img_data, global);
	if (!img_data) return ret;
	tx = GRRLIB_LoadTexture(img_data);
	SAFE_FREE(img_data);
	if (!tx) return -1;
	// must be divisible by 4, min size: 8x8 max size: 512x512
	if ( (tx->w % 4) || (tx->h % 4)
		|| tx->w < 8 || tx->h < 8
		|| tx->w > 512 || tx->h > 512)
	{
		GRRLIB_FREE_TEX(tx);
		return -1;
	}
	GRRLIB_TextureToMEM2(tx);
	*ptx = tx;
	return ret;
}

int wgui_load_texture(GRRLIB_texImg **ptx, char *name, const u8 *img, bool global)
{
	int ret = wgui_load_texture1(ptx, name, global);
	if (!*ptx && img) {
		*ptx = GRRLIB_LoadTexture(img);
		GRRLIB_TextureToMEM2(*ptx);
	}
	return ret;
}

int wgui_load_button(GRRLIB_texImg **ptx, char *name, const u8 *img, bool global)
{
	int ret = wgui_load_texture(ptx, name, img, global);
	wgui_init_button(*ptx);
	return ret;
}

int wgui_load_icon(GRRLIB_texImg **ptx, char *name, const u8 *img, bool global, int w, int h)
{
	int ret = wgui_load_texture(ptx, name, img, global);
	wgui_init_icon(*ptx, w, h);
	//dbg_printf("icon: %s %d %d : %d\n", name, w, h, *ptx ? (*ptx)->nbtileh : -1);
	return ret;
}

void wgui_init_window(GRRLIB_texImg *tx)
{
	if (!tx) return;
	GRRLIB_InitTileSet(tx, tx->w/4, tx->h/4, 0);
}

int wgui_load_window(GRRLIB_texImg **ptx, char *name, const u8 *img, bool global)
{
	int ret = wgui_load_texture(ptx, name, img, global);
	wgui_init_window(*ptx);
	return ret;
}

void wgui_init()
{
	if (wgui_inited) return;
	bool global = true;
	int ret;

	// button
	if (CFG.old_button_color)
		ret = wgui_load_button(&tx_button, "button.png", button_old_png, global);
	else
		ret = wgui_load_button(&tx_button, "button.png", button_png, global);
	// if a themed button.png is found
	// then all other wgui images must be themed too
	if (ret == 0) global = false;
	// checkbox
	wgui_load_button(&tx_checkbox, "checkbox.png", NULL, global);
	// radio
	wgui_load_button(&tx_radio, "radio.png", NULL, global);
	// window
	wgui_load_window(&tx_window, "window.png", window_png, global);
	// page
	wgui_load_window(&tx_page, "page.png", NULL, global);

	// custom
	int i;
	struct CfgButton *bb;
	for (i=0; i<GUI_BUTTON_NUM; i++) {
		GRRLIB_FREE_TEX(tx_custom[i]);
		bb = &CFG.gui_button[i];
		if (bb->enabled) {
			if (bb->type == 0) {
				wgui_load_button(&tx_custom[i], bb->image, NULL, global);
			} else {
				wgui_load_icon(&tx_custom[i], bb->image, NULL, global,
						bb->pos.w, bb->pos.h);
			}
		}
	}

	wgui_inited = 1;
}

void wgui_DrawWindowBase(GRRLIB_texImg *tx, int x, int y, int w, int h, u32 color)
{
	int tw = tx->tilew;
	int th = tx->tileh;
	int ww = w - tw*2;
	int hh = h - th*2;

	// v:
	// 0 1 2
	// 3 4 5
	// 6 7 8

	guVector v1[] = {
		{x+tw,    y},
		{x+tw+ww, y},
		{x+tw+ww, y+th},
		{x+tw,    y+th}
	};
	guVector v3[] = {
		{x,    y+th},
		{x+tw, y+th},
		{x+tw, y+th+hh},
		{x,    y+th+hh}
	};
	guVector v4[] = {
		{x+tw,    y+th},
		{x+tw+ww, y+th},
		{x+tw+ww, y+th+hh},
		{x+tw,    y+th+hh}
	};
	guVector v5[] = {
		{x+tw+ww, y+th},
		{x+w,     y+th},
		{x+w,     y+th+hh},
		{x+tw+ww, y+th+hh}
	};
	guVector v7[] = {
		{x+tw,    y+th+hh},
		{x+tw+ww, y+th+hh},
		{x+tw+ww, y+h},
		{x+tw,    y+h}
	};

	// tile:
	//  0  1  2  3
	//  4  5  6  7
	//  8  9 10 11
	// 12 13 14 15
	GRRLIB_DrawTile(x, y, tx, 0,1,1, color, 0);
	GRRLIB_DrawTileRectQuad(v1, tx, color, 1, 2, 1);
	GRRLIB_DrawTile(x+w-tw, y, tx, 0,1,1, color, 3);

	GRRLIB_DrawTileRectQuad(v3, tx, color, 4, 1, 2);
	GRRLIB_DrawTileRectQuad(v4, tx, color, 5, 2, 2);
	GRRLIB_DrawTileRectQuad(v5, tx, color, 7, 1, 2);

	GRRLIB_DrawTile(x, y+th+hh, tx, 0,1,1, color, 12);
	GRRLIB_DrawTileRectQuad(v7, tx, color, 13, 2, 1);
	GRRLIB_DrawTile(x+tw+ww, y+th+hh, tx, 0,1,1, color, 15);
}

void wgui_DrawWindow(GRRLIB_texImg *tx, int x, int y, int w, int h,
		u32 window_color, u32 color, float txt_scale, char *title)
{
	int cx = x + w / 2;
	window_color = color_multiply(window_color, color);
	wgui_DrawWindowBase(tx, x, y, w, h, window_color);
	if (title) {
		FontColor fc;
		font_color_multiply(&CFG.gui_tc[GUI_TC_TITLE], &fc, color);
		Gui_PrintAlignZ(cx, y+PAD2, 0, 0, &tx_font, fc, txt_scale, title);
		/*char str[16];
		sprintf(str, "%.2f %.2fx%.2f", txt_scale,
				tx_font.tilew*txt_scale, tx_font.tileh*txt_scale);
		Gui_PrintAlignZ(x, y, -1, -1, &tx_font, fc, txt_scale, str);*/
	}
}

void wgui_DrawButtonBase(GRRLIB_texImg *tx,
		float x, float y, float w, float h, float zoom, u32 color, int state)
{
	float w2 = w / 2.0;
	float h2 = h / 2.0;
	bool do_middle = true;
	x = (x + w2) - w2 * zoom;
	y = (y + h2) - h2 * zoom;
	w *= zoom;
	h *= zoom;
	h2 = h / 2.0;
	if (fabsf(h) >= fabsf(w)) {
		h2 = w / 2.0;
		do_middle = false;
	}
	guVector v1[] = {
		{x,    y},
		{x+h2, y},
		{x+h2, y+h},
		{x,    y+h}
	};
	guVector v2[] = {
		{x+h2,   y},
		{x+w-h2, y},
		{x+w-h2, y+h},
		{x+h2,   y+h}
	};
	guVector v3[] = {
		{x+w-h2, y},
		{x+w,    y},
		{x+w,    y+h},
		{x+w-h2, y+h}
	};

	state *= 4;
	GRRLIB_DrawTileQuad(v1, tx, color, state + 0);
	if (do_middle) {
		GRRLIB_DrawTileRectQuad(v2, tx, color, state + 1, 2, 1);
	}
	GRRLIB_DrawTileQuad(v3, tx, color, state + 3);
}

void wgui_DrawIconBase(GRRLIB_texImg *tx,
		float x, float y, float w, float h, float zoom, u32 color, int state)
{
	float w2 = w / 2.0;
	float h2 = h / 2.0;
	x = (x + w2) - w2 * zoom;
	y = (y + h2) - h2 * zoom;
	w *= zoom;
	h *= zoom;
	guVector v1[] = {
		{x,   y},
		{x+w, y},
		{x+w, y+h},
		{x,   y+h}
	};
	GRRLIB_DrawTileQuad(v1, tx, color, state);
}

void wgui_DrawButtonX(GRRLIB_texImg *tx,
		float x, float y, float w, float h, float zoom, u32 color, int state)
{
	if (state >= tx->nbtileh) {
		if (state == WGUI_IMG_HOVER || state == WGUI_IMG_PRESS_HOVER) {
			color = color_multiply(color, 0xE0E0FFFF);
		}
		switch (state) {
			case WGUI_IMG_PRESS:
			case WGUI_IMG_PRESS_HOVER:
				color = color_multiply(color, 0xA0A0C0FF);
				zoom = -zoom;
				break;
			case WGUI_IMG_FLASH:
				color = color_multiply(color, 0x808080FF);
				break;
			case WGUI_IMG_INACTIVE:
				color = color_multiply(color, disabled_color);
				break;
		}
		state = 0;
	}
	if (tx->nbtilew == 4) {
		wgui_DrawButtonBase(tx, x, y, w, h, zoom, color, state);
	} else {
		wgui_DrawIconBase(tx, x, y, w, h, zoom, color, state);
	}
}

#if 0
float _wgui_DrawButton(GRRLIB_texImg *tx, int x, int y, int w, int h,
		u32 color, int state, float txt_scale, char *txt)
{
	float cx = (float)x + (float)w / 2.0;
	float cy = (float)y + (float)h / 2.0;
	float zoom;
	FontColor fc;
	if (state == WGUI_STATE_HOVER) {
		zoom = 1.1;
		txt_scale *= zoom;
	} else {
		zoom = 1.0;
	}
	if (state == WGUI_STATE_INACTIVE) {
		font_color_multiply(&disabled_fc, &fc, color);
		color = color_multiply(color, disabled_color);
	} else {
		font_color_multiply(&CFG.gui_tc[GUI_TC_MENU], &fc, color);
	}
	wgui_DrawButtonBase(tx, x, y, w, h, zoom, color, state);
	Gui_PrintAlignZ(cx, cy, 0, 0, &tx_font, fc, txt_scale, txt);
	/*char str[16];
	sprintf(str, "%.2f %.2f", txt_scale, tx_font.tilew*txt_scale);
	Gui_PrintAlignZ(cx, y+h, 0, -1, &tx_font, fc, 0.7, str);*/
	return zoom;
}
#endif

void wgui_input_save2(ir_t *ir, int *p_buttons)
{
	if (ir) {
		save_input.sx = ir->sx;
		save_input.sy = ir->sy;
		save_input.smooth_valid = ir->smooth_valid;
		save_input.valid = ir->valid;
	}
	if (p_buttons) {
		save_input.buttons = *p_buttons;
	}
}

void wgui_input_save()
{
	wgui_input_save2(winput.w_ir, winput.p_buttons);
}

void wgui_input_set(ir_t *ir, int *buttons, int *held)
{
	winput.w_ir = ir;
	winput.w_buttons = *buttons;
	winput.p_buttons = buttons;
	winput.p_held = held;
	wgui_input_save();
}

void wgui_input_steal2(ir_t *ir, int *buttons)
{
	// steal wpad
	ir->sx = -100;
	ir->sy = -100;
	ir->smooth_valid = 0;
	ir->valid = 0;
	if (*buttons & (WPAD_BUTTON_A | WPAD_BUTTON_B)) {
		*buttons = 0;
		winput.w_buttons = 0;
	}
}

void wgui_input_steal_buttons()
{
	if (winput.p_buttons) *winput.p_buttons = 0;
	if (winput.p_held) *winput.p_held = 0;
	winput.w_buttons = 0;
}

void wgui_input_steal()
{
	wgui_input_steal2(winput.w_ir, winput.p_buttons);
}

void wgui_input_restore(ir_t *ir, int *buttons)
{
	if (buttons) *buttons = save_input.buttons;
	winput.w_buttons = save_input.buttons;
	if (ir) {
		ir->sx = save_input.sx;
		ir->sy = save_input.sy;
		ir->smooth_valid = save_input.smooth_valid;
		ir->valid = save_input.valid;
	}
}

void wgui_input_get()
{
	winput.w_buttons = Wpad_GetButtons();
	Wpad_getIR(winput.w_ir);
	if (winput.p_buttons) *winput.p_buttons = winput.w_buttons;
	wgui_input_save();
}

bool wgui_over_widget(Widget *ww, ir_t *ir)
{
	if (!ir->smooth_valid) return false;
	return GRRLIB_PtInRect(ww->ax, ww->ay, ww->w, ww->h, ir->sx, ir->sy);
}

bool wgui_over(Widget *ww)
{
	return wgui_over_widget(ww, winput.w_ir);
}

void traverse_children1(Widget *ww, void (*fun)(Widget *))
{
	int i;
	Widget *cc;
	for (i=0; i < ww->num_child; i++) {
		cc = ww->child[i];
		fun(cc);
		if (cc->num_child) {
			traverse_children1(cc, fun);
		}
	}
}

void traverse1(Widget *ww, void (*fun)(Widget *))
{
	fun(ww);
	if (ww->num_child) {
		traverse_children1(ww, fun);
	}
}

void traverse_children(Widget *ww, void (*fun)(Widget *, int), int val)
{
	int i;
	Widget *cc;
	for (i=0; i < ww->num_child; i++) {
		cc = ww->child[i];
		fun(cc, val);
		if (cc->num_child) {
			traverse_children(cc, fun, val);
		}
	}
}

void traverse(Widget *ww, void (*fun)(Widget *, int), int val)
{
	fun(ww, val);
	if (ww->num_child) {
		traverse_children(ww, fun, val);
	}
}

// traverse linked only, not children
void traverse_linked(Widget *ww, void (*fun2)(Widget *, int), int val)
{
	while (ww) {
		fun2(ww, val);
		ww = ww->link_next;
	}
}

// traverse linked and children
void traverse_linked_children(Widget *ww, void (*fun2)(Widget *, int), int val)
{
	while (ww) {
		traverse(ww, fun2, val);
		ww = ww->link_next;
	}
}

void wgui_update(Widget *ww)
{
	if (ww->update) ww->update(ww);
}

void wgui_action(Widget *ww)
{
	if (ww->action) ww->action(ww);
	if (ww->action2) ww->action2(ww);
}

int wgui_set_value_local_simple(Widget *ww, int flags, int val)
{
	if (flags & SET_VAL) {
		if (!(flags & SET_VAL_FORCE)) {
			if (val > ww->val_max) val = ww->val_max;
			if (val < ww->val_min) val = ww->val_min;
		}
		ww->value = val;
	} else if (flags & SET_VAL_MIN) {
		ww->val_min = val;
	} else if (flags & SET_VAL_MAX) {
		// XXX maybe by default: increase only
		// so that linked page and radio can be constructed and linked
		// independently, without one decreasing the max limit of the other
		if (!(flags & SET_VAL_FORCE)) {
			ww->val_max = val;
		}
		ww->val_max = val;
		// XXX propagate fixed value?
		if (ww->value > ww->val_max) ww->value = ww->val_max;
	}
	return val;
}

// no set_action callback
int wgui_set_value_local_x(Widget *ww, int flags, int val)
{
	int old_val = ww->value;
	val = wgui_set_value_local_simple(ww, flags, val);
	if ((flags & SET_VAL) && (flags & SET_VAL_ACTION) && (ww->value != old_val)) {
		wgui_action(ww);
	}
	return val;
}

int wgui_set_value_local(Widget *ww, int flags, int val)
{
	if (!ww->set_value) {
		val = wgui_set_value_local_x(ww, flags, val);
	} else {
		val = ww->set_value(ww, flags, val);
	}
	return val;
}

// propagate value to all value-linked widgets
int wgui_propagate_value(Widget *ww, int flags, int val)
{
	if (!ww) return val;
	int old_val = ww->value;
	int error = 0;
restart:;
	Widget *link = ww;
	int ret;
	do {
		ret = wgui_set_value_local(link, flags, val);
		if (ret != val) {
			// value not accepted
			error++;
			if (error == 1) {
				// use returned value and restart loop
				val = ret;
				goto restart;
			} else if (error == 2) {
				// use old value and restart loop
				val = old_val;
				goto restart;
			} else {
				// ignore, let the loop through
			}
		}
		link = link->val_link;
	} while (link && link != ww);
	return val;
}

int wgui_set_value(Widget *ww, int val)
{
	// by default no action is called
	return wgui_propagate_value(ww, SET_VAL, val);
}

// a->x->y  b->q->w
// a->x->y->b->q->w
// a: existing, b: new
// copy val a->b
void wgui_link_value(Widget *a, Widget *b)
{
	if (!a && !b) return;
	if (a && b) {
		wgui_propagate_value(b, SET_VAL_FORCE | SET_VAL, a->value);
		wgui_propagate_value(b, SET_VAL_FORCE | SET_VAL_MIN, a->val_min);
		wgui_propagate_value(b, SET_VAL_FORCE | SET_VAL_MAX, a->val_max);
	}
	// if one of ptr is NULL make it link to self
	if (!a) a = b;
	if (!b) b = a;
	Widget *last_a = a;
	Widget *last_b = b;
	while (last_a->val_link && last_a->val_link != a) last_a = last_a->val_link;
	while (last_b->val_link && last_b->val_link != b) last_b = last_b->val_link;
	last_a->val_link = b;
	last_b->val_link = a;
}

void wgui_unlink_value(Widget *a)
{
	if (!a) return;
	Widget *last_a = a;
	while (last_a->val_link && last_a->val_link != a) last_a = last_a->val_link;
	last_a->val_link = a->val_link;
	a->val_link = NULL;
}

void wgui_set_value_list(Widget *ww, int n, char **values)
{
	ww->list_num = n;
	SAFE_FREE(ww->list_values);
	if (n > 0 && values) {
		ww->list_values = malloc(sizeof(char*) * n);
		memcpy(ww->list_values, values, sizeof(char*) * n);
	}
	wgui_propagate_value(ww, SET_VAL_MAX, n - 1);
}


void wgui_remove(Widget *ww);

void wgui_remove_children(Widget *ww)
{
	int i;
	// must be in reverse order
	for (i = ww->num_child - 1; i >= 0; i--) {
		wgui_remove(ww->child[i]);
	}
}

void wgui_close(Widget *ww)
{
	if (ww->cleanup) ww->cleanup(ww);
	if (ww->self_ptr && *ww->self_ptr == ww) {
		*ww->self_ptr = NULL;
	}
	wgui_unlink_value(ww);
	wgui_remove_children(ww);
	SAFE_FREE(ww->child);
	SAFE_FREE(ww->list_values);
	memset(ww, 0, sizeof(Widget));
}

void wgui_remove(Widget *ww)
{
	Widget *pp = ww->parent;
	wgui_close(ww);
	// remove ww from parent list of children
	if (!pp) return;
	int i;
	for (i=0; i<pp->num_child; i++) {
		if (pp->child[i] == ww) {
			SAFE_FREE(pp->child[i]);
			// move following over
			int n = pp->num_child - i - 1;
			if (n) memmove(&pp->child[i], &pp->child[i+1], n * sizeof(Widget*));
			// could also realloc, but we can just keep the mem
			pp->num_child--;
			break;
		}
	}
}

// handling order:
//
// root ... children
// update ->
//   set_val / no action
// handle <-
//   set_val / action
// render ->


Widget* wgui_handle(Widget *ww);

Widget* wgui_handle_children(Widget *ww)
{
	int i;
	Widget *cc;
	Widget *r;
	Widget *ret = NULL;
	// handle from last to first
	// (opposite direction than render)
	for (i = ww->num_child-1; i >= 0; i--) {
		cc = ww->child[i];
		r = wgui_handle(cc);
		// close marked dialogs
		if (cc->closing) {
			wgui_remove(cc);
		} else if (r) {
			// return this child
			// unless cc is a container
			if (cc->type & WGUI_TYPE_CONTAINER) {
				ret = r;
			} else {
				ret = cc;
			}
		}
	}
	return ret;
}

Widget* wgui_handle(Widget *ww)
{
	int state = 0;
	Widget *ret = NULL;
	wgui_update(ww);
	if (ww->state == WGUI_STATE_DISABLED) return NULL;
	// handle children first
	if (!ww->skip_child_handle) {
		ret = wgui_handle_children(ww);
	}
	if (ww->handle && ww->state != WGUI_STATE_INACTIVE) {
		state = ww->handle(ww);
	}
	// if ww->parent == NULL then we're the desktop,
	// so don't steal the input
	if (ww->lock_focus || (ww->parent && wgui_over(ww)
				&& ( ww->type == WGUI_TYPE_DIALOG || state == WGUI_STATE_PRESS)))
	{
		wgui_input_steal();
	}
	if (state == WGUI_STATE_PRESS) {
		ret = ww;
	}
	return ret;
}

void wgui_render(Widget *ww);

void wgui_render_children(Widget *ww)
{
	int i;
	for (i=0; i<ww->num_child; i++) {
		wgui_render(ww->child[i]);
	}
}

void wgui_render(Widget *ww)
{
	if (ww->state == WGUI_STATE_DISABLED) return;
	if (ww->render) ww->render(ww);
	if (!ww->skip_child_render) {
		wgui_render_children(ww);
	}
}

// SIZE_AUTO:
//  use parent's post suggested size or pos.auto calculated size

Pos pos(int x, int y, int w, int h)
{
	Pos p = pos_auto;
	p.x = x;
	p.y = y;
	p.w = w;
	p.h = h;
	return p;
}

Pos pos_xy(int x, int y)
{
	Pos p = pos_auto;
	p.x = x;
	p.y = y;
	return p;
}

Pos pos_x(int x)
{
	Pos p = pos_auto;
	p.x = x;
	return p;
}

Pos pos_y(int y)
{
	Pos p = pos_auto;
	p.y = y;
	return p;
}

Pos pos_wh(int w, int h)
{
	Pos p = pos_auto;
	p.w = w;
	p.h = h;
	return p;
}

Pos pos_w(int w)
{
	Pos p = pos_auto;
	p.w = w;
	return p;
}

Pos pos_h(int h)
{
	Pos p = pos_auto;
	p.h = h;
	return p;
}

bool pos_special(int x)
{
	return (x == POS_AUTO || x == POS_CENTER || x == POS_MIDDLE || x == POS_EDGE);
}

Pos pos_get(Widget *ww)
{
	Pos p = pos_auto;
	p.x = ww->x;
	p.y = ww->y;
	p.w = ww->w;
	p.h = ww->h;
	if (ww->parent) {
		p.x -= ww->parent->post.margin;
		p.y -= ww->parent->post.margin;
	}
	return p;
}

void pos_init(Widget *ww)
{
	PosState *p = &ww->post;
	memset(p, 0, sizeof(*p));
}

int pos_margin(Widget *ww, int margin)
{
	int old = ww->post.margin;
	if (margin < 0) {
		margin = (ww->w + margin) / 2;
	}
	ww->post.margin = margin;
	if (ww->post.x == old) ww->post.x = margin;
	if (ww->post.y == old) ww->post.y = margin;
	return old;
}

void pos_pad(Widget *ww, int pad)
{
	ww->post.pad = pad;
}

void pos_prefsize(Widget *ww, int w, int h)
{
	ww->post.w = w;
	ww->post.h = h;
}

void pos_reset(Widget *ww)
{
	ww->post.w = 0;
	ww->post.h = 0;
}

void pos_move(Widget *ww, int x, int y)
{
	ww->post.x += x;
	ww->post.y += y;
}

void pos_move_x(Widget *ww, int x)
{
	if (x == POS_AUTO) {
		// nothing
	} else if (x == POS_CENTER) {
		ww->post.x = ww->post.w / 2;
	} else if (x == POS_EDGE) {
		ww->post.x = ww->post.w;
	} else if (x >= 0) {
		ww->post.x = ww->post.margin + x;
	} else {
		ww->post.x = ww->w - ww->post.margin + x;
	}
}

void pos_move_y(Widget *ww, int y)
{
	if (y == POS_AUTO) {
		// nothing
	} else if (y == POS_CENTER) {
		ww->post.y = ww->post.h / 2;
	} else if (y == POS_EDGE) {
		ww->post.y = ww->post.h;
	} else if (y >= 0) {
		ww->post.y = ww->post.margin + y;
	} else {
		ww->post.y = ww->h - ww->post.margin + y;
	}
}
void pos_move_to(Widget *ww, int x, int y)
{
	pos_move_x(ww, x);
	pos_move_y(ww, y);
}

void pos_move_pos(Widget *ww, Pos p)
{
	pos_move_to(ww, p.x, p.y);
}

void pos_move_abs(Widget *ww, int x, int y)
{
	ww->post.x = x;
	ww->post.y = y;
}

int pos_avail_w(Widget *ww)
{
	return ww->w - ww->post.x - ww->post.margin;
}

int pos_avail_h(Widget *ww)
{
	return ww->h - ww->post.y - ww->post.margin;
}

void pos_newlines(Widget *ww, int lines)
{
	int i;
	int my = 0, y;
	Widget *cc;
	ww->post.x = ww->post.margin;
	if (lines >= 0) {
		// find max y
		for (i=0; i < ww->num_child; i++) {
			cc = ww->child[i];
			y = cc->y + cc->h;
			if (y > my) my = y;
		}
		if (my > ww->post.y) ww->post.y = my;
		ww->post.y += ww->post.pad * lines;
	}
}

void pos_newline(Widget *ww)
{
	pos_newlines(ww, 1);
}

int pos_simple_w(Widget *ww, int x, int pw)
{
	int w = pw;
	if (ww) {
		PosState *post = &ww->post;
		if (pw == SIZE_AUTO || pw == SIZE_FULL) {
			w = ww->w - x - post->margin;
		} else if (pw < 0) {
			w = ww->w - x - post->margin + pw;
		}
	} else {
		if (w == SIZE_FULL || w == SIZE_AUTO) w = 640;
		else if (w < 0) w = 640 + w;
	}
	if (w < PAD1) w = PAD1;
	return w;
}

int pos_simple_h(Widget *ww, int y, int ph)
{
	int h = ph;
	if (ww) {
		PosState *post = &ww->post;
		if (ph == SIZE_AUTO || ph == SIZE_FULL) {
			h = ww->h - y - post->margin;
		} else if (ph < 0) {
			h = ww->h - y - post->margin + ph;
		}
	} else {
		if (h == SIZE_FULL || h == SIZE_AUTO) h = 480;
		else if (h < 0) h = 480 + h;
	}
	if (h < PAD1) h = PAD1;
	return h;
}

int pos_simple_x(Widget *parent, int px, int *pw)
{
	int x = px;
	int w = *pw;
	if (parent) {
		PosState *post = &parent->post;
		if (pos_special(x)) {
			x = post->x;
			w = pos_simple_w(parent, x, *pw);
			int avail = pos_avail_w(parent) - w;
			if ((avail < 0 || w < 0) && x > post->margin) {
				pos_newline(parent);
				x = post->x;
				w = pos_simple_w(parent, x, *pw);
				avail = pos_avail_w(parent) - w;
			}
			if (px == POS_CENTER) {
				x = (parent->w - w) / 2;
			}
			if (px == POS_MIDDLE) {
				if (avail > 0) x += avail / 2;
			}
			if (px == POS_EDGE) {
				x = parent->w - post->margin - w;
			}
		} else if (x >= 0) {
			// left offset
			x = post->margin + x;
		} else { // x < 0
			// right offset
			x = parent->w - post->margin + x;
		}
	} else {
		if (pos_special(x)) {
			x = 0;
			w = pos_simple_w(parent, x, *pw);
			int avail = 640 - w;
			if (px == POS_CENTER) {
				x = avail / 2;
			}
			if (px == POS_MIDDLE) {
				if (avail > 0) x += avail / 2;
			}
			if (px == POS_EDGE) {
				if (avail > 0) x += avail;
			}
		} else if (x >= 0) {
			// left offset
		} else { // x < 0
			// right offset
			x = 640 + x;
		}
	}
	*pw = w;
	return x;
}

int pos_simple_y(Widget *parent, int py, int *ph)
{
	int y = py;
	int h = *ph;
	if (parent) {
		PosState *post = &parent->post;
		if (pos_special(y)) {
			y = post->y;
			h = pos_simple_h(parent, y, *ph);
			if (py == POS_CENTER) {
				y = (parent->h - h) / 2;
			}
			if (py == POS_MIDDLE) {
				int avail = pos_avail_h(parent) - h;
				if (avail > 0) y += avail / 2;
			}
			if (py == POS_EDGE) {
				y = parent->h - post->margin - h;
			}
		} else if (y >= 0) {
			// top offset
			y = post->margin + y;
		} else { // y < 0
			// bottom offset
			y = parent->h - post->margin + y;
		}
	} else {
		if (pos_special(y)) {
			y = 0;
			h = pos_simple_h(parent, y, *ph);
			int avail = 480 - h;
			if (py == POS_CENTER) {
				y = avail / 2;
			}
			if (py == POS_MIDDLE) {
				if (avail > 0) y += avail / 2;
			}
			if (py == POS_EDGE) {
				if (avail > 0) y += avail;
			}
		} else if (y >= 0) {
			// top offset
		} else { // y < 0
			// bottom offset
			y = 480 + y;
		}
	}
	*ph = h;
	return y;
}

// simplify pos
void pos_simple(Widget *ww, Pos *p)
{
	p->x = pos_simple_x(ww, p->x, &p->w);
	p->w = pos_simple_w(ww, p->x, p->w);
	p->y = pos_simple_y(ww, p->y, &p->h);
	p->h = pos_simple_h(ww, p->y, p->h);
}

int pos_auto_only_w(Widget *parent, Pos p)
{
	int w = p.w;
	if (parent && p.w == SIZE_AUTO) {
		PosState *post = &parent->post;
		if (post->w > PAD1) w = post->w;
		else w = p.auto_w;
	}
	return w;
}

int pos_auto_w(int x, Widget *parent, Pos p)
{
	int w = pos_auto_only_w(parent, p);
	w = pos_simple_w(parent, x, w);
	if (w < PAD1) w = PAD1;
	return w;
}

int pos_auto_x(int *pw, Widget *parent, Pos p)
{
	*pw = pos_auto_only_w(parent, p);
	return pos_simple_x(parent, p.x, pw);
}

int pos_auto_only_h(Widget *parent, Pos p)
{
	int h = p.h;
	if (parent && p.h == SIZE_AUTO) {
		PosState *post = &parent->post;
		if (post->h > PAD1) h = post->h;
		else h = p.auto_h;
	}
	return h;
}

int pos_auto_h(int y, Widget *parent, Pos p)
{
	int h = pos_auto_only_h(parent, p);
	h = pos_simple_h(parent, y, h);
	if (h < PAD1) h = PAD1;
	return h;
}

int pos_auto_y(int *ph, Widget *parent, Pos p)
{
	*ph = pos_auto_only_h(parent, p);
	return pos_simple_y(parent, p.y, ph);
}

void pos_auto_default(Pos *p, char *name)
{
	// calculate auto size if not specified
	if (name) {
		if (!p->auto_w) p->auto_w = tx_font.tilew * con_len(name);
		if (!p->auto_h) p->auto_h = tx_font.tileh;
	} else {
		if (!p->auto_w) p->auto_w = PAD1;
		if (!p->auto_h) p->auto_h = PAD1;
	}
}

void pos_auto_expand(Widget *parent, Pos *p)
{
	p->x = pos_auto_x(&p->w, parent, *p);
	if (p->w < PAD1) p->w = pos_auto_w(p->x, parent, *p);
	p->y = pos_auto_y(&p->h, parent, *p);
	if (p->h < PAD1) p->h = pos_auto_h(p->y, parent, *p);
}

void pos_columns(Widget *ww, int cols, int pw)
{
	int w = 0;
	if (cols > 0) {
		pw = pos_simple_w(ww, ww->post.x, pw);
		w = (pw + ww->post.pad) / cols - ww->post.pad;
	}
	ww->post.w = w;
}

void pos_rows(Widget *ww, int rows, int ph)
{
	int h = 0;
	if (rows > 0) {
		ph = pos_simple_h(ww, ww->post.y, ph);
		h = (ph + ww->post.pad) / rows - ww->post.pad;
	}
	ww->post.h = h;
}

static inline u32 get_gui_color(u32 color)
{
	if (color > 0 && color < GUI_COLOR_NUM) {
		return CFG.gui_window_color[color];
	}
	return color;
}

static inline FontColor get_text_color(u32 tc)
{
	if (tc >= GUI_TC_NUM) tc = GUI_TC_NUM - 1;
	return CFG.gui_tc[tc];
}

void Widget_init(Widget *ww, Widget *parent, Pos p, char *name)
{
	memset(ww, 0, sizeof(*ww));
	ww->type = WGUI_TYPE_WIDGET;
	ww->name = name;
	ww->parent = parent;
	ww->color = 0xFFFFFFFF;
	ww->dialog_color = GUI_COLOR_BASE;
	ww->zoom = 1.0;
	ww->max_zoom = 1.1; // +10%
	ww->text_scale = 1.0;
	ww->text_color = GUI_TC_MENU;

	pos_auto_default(&p, name);
	pos_auto_expand(parent, &p);
	
	if (parent) {
		ww->ax = parent->ax + p.x;
		ww->ay = parent->ay + p.y;
		parent->post.x = p.x + p.w + parent->post.pad;
		parent->post.y = p.y;
	} else {
		ww->ax = p.x;
		ww->ay = p.y;
	}
	ww->x = p.x;
	ww->y = p.y;
	ww->w = p.w;
	ww->h = p.h;
	
	//dbg_printf("%s: %d %d %d %d\n", name?name:"", x, y, w, h);

	pos_init(ww);
}

Widget* wgui_add(Widget *parent, Pos p, char *name)
{
	Widget *ww;
	ww = calloc(1, sizeof(Widget));
	if (ww) {
		if (parent) {
			Widget **pw;
			pw = realloc(parent->child, (parent->num_child+1) * sizeof(Widget*));
			if (!pw) return NULL;
			parent->child = pw;
			parent->child[parent->num_child] = ww;
			parent->num_child++;
		}
		Widget_init(ww, parent, p, name);
	}
	return ww;
}

// return index
int wgui_link(Widget *link, Widget *ww)
{
	int i = 0;
	if (link) {
		i++;
		// link can point to any list element, get first.
		if (link->link_first) link = link->link_first;
		ww->link_first = link;
		// add link to last element
		while (link->link_next) {
			link = link->link_next;
			i++;
		}
		link->link_next = ww;
	} else {
		// first link element.
		// point to self
		ww->link_first = ww;
	}
	ww->link_index = i;
	return i;
}

// return index
// should be same as ww->link_index
int wgui_link_count(Widget *ww)
{
	int i = 0;
	Widget *link = ww->link_first;
	while (link) {
		if (ww == link) return i;
		link = link->link_next;
		i++;
	}
	return -1;
}

Widget* wgui_link_get(Widget *ww, int index)
{
	int i = 0;
	Widget *link = ww->link_first;
	while (link) {
		if (i == index) return link;
		link = link->link_next;
		i++;
	}
	return NULL;
}

void wgui_set_state(Widget *ww, int state)
{
	if (!ww) return;
	if (state == WGUI_STATE_NORMAL) {
		// only set to normal if disabled or inactive
		// so that hover or pressed are not changed
		if (ww->state == WGUI_STATE_INACTIVE
			|| ww->state == WGUI_STATE_DISABLED) {
			ww->state = state;
		}
	} else {
		ww->state = state;
	}
}

void wgui_set_inactive(Widget *ww, int cond)
{
	if (!ww) return;
	if (cond) {
		ww->state = WGUI_STATE_INACTIVE;
	} else {
		if (ww->state == WGUI_STATE_INACTIVE)
			ww->state = WGUI_STATE_NORMAL;
	}
}

void wgui_set_disabled(Widget *ww, int cond)
{
	if (!ww) return;
	if (cond) {
		ww->state = WGUI_STATE_DISABLED;
	} else {
		if (ww->state == WGUI_STATE_DISABLED)
			ww->state = WGUI_STATE_NORMAL;
	}
}

Widget* wgui_primary_parent(Widget *parent)
{
	while (parent && parent->parent) parent = parent->parent;
	return parent;
}

Widget *wgui_find_child_type(Widget *ww, int type)
{
	int i;
	Widget *ret = NULL;
	if (ww->type == type) return ww;
	for (i=0; i<ww->num_child; i++) {
		if (ww->child[i]->type == type) return ww->child[i];
	}
	for (i=0; i<ww->num_child; i++) {
		ret = wgui_find_child_type(ww->child[i], type);
		if (ret) break;
	}
	return ret;
}

Widget *wgui_find_parent_type(Widget *ww, int type)
{
	Widget *parent = ww->parent;
	while (parent) {
		if (parent->type == type) break;
		parent = parent->parent;
	}
	return parent;
}

int handle_B_close(Widget *ww)
{
	if (winput.w_buttons & WPAD_BUTTON_B) {
		ww->closing = 1;
		wgui_input_steal();
	}
	return 0;
}

void action_close_parent(Widget *ww)
{
	// mark closure, don't remove just yet
	// because we are still in handle loop
	if (ww->parent) ww = ww->parent;
	ww->closing = 1;
}

void action_close_parent_dialog(Widget *ww)
{
	// mark closure, don't remove just yet
	// because we are still in handle loop
	Widget *parent = wgui_find_parent_type(ww, WGUI_TYPE_DIALOG);
	if (parent) parent->closing = 1;
}

void wgui_set_color(Widget *ww, int color)
{
	ww->color = color;
}

void adjust_position(Widget *ww)
{
	if (ww->parent) {
		ww->ax = ww->x + ww->parent->ax;
		ww->ay = ww->y + ww->parent->ay;
	}
}

void update_val_from_ptr_int(Widget *ww)
{
	if (ww->val_ptr) {
		int val = *(int*)ww->val_ptr;
		// update val, no action
		if (val != ww->value) wgui_set_value(ww, val);
	}
}

void update_val_from_ptr_bool(Widget *ww)
{
	if (ww->val_ptr) {
		int val = *(bool*)ww->val_ptr ? 1 : 0;
		// update val, no action
		if (val != ww->value) wgui_set_value(ww, val);
	}
}


void update_val_from_ptr_int_active(Widget *ww)
{
	if (ww->state != WGUI_STATE_INACTIVE && ww->state != WGUI_STATE_DISABLED) {
		update_val_from_ptr_int(ww);
	}
}

void action_write_val_ptr_int(Widget *ww)
{
	if (ww->val_ptr) {
		*(int*)ww->val_ptr = ww->value;
	}
}

void action_write_val_ptr_bool(Widget *ww)
{
	if (ww->val_ptr) {
		*(bool*)ww->val_ptr = ww->value ? true : false;
	}
}

void wgui_action_change_parent_val(Widget *ww)
{
	Widget *ll = ww->parent;
	int value = ll->value + ww->value;
	// propagate value and call action hooks
	wgui_propagate_value(ll, SET_VAL | SET_VAL_ACTION, value);
}


//////// Text


float text_round_scale_w(float scale, int dir)
{
	// round to an integral width
	int w;
	if (dir < 0) {
		// round down
		w = (int)((float)tx_font.tilew * scale);
	} else if (dir > 0) {
		// round up
		w = (int)ceilf((float)tx_font.tilew * scale);
	} else {
		// round nearest
		w = (int)roundf((float)tx_font.tilew * scale);
	}
	// recalculate scale
	return (float)w / (float)tx_font.tilew;
}

float text_h_to_scale(int h)
{
	float scale = (float)h / (float)tx_font.tileh;
	return text_round_scale_w(scale, -1); // round down
}

float pos_text_w(int len, float scale)
{
	return tx_font.tilew * len * scale;
}

float text_scale_fit(int w, int h, int len, float scale)
{
	float tw = pos_text_w(len, scale);
	if (tw > w) {
		scale = (float)w / pos_text_w(len, 1.0);
		scale = text_round_scale_w(scale, -1);
		if (scale < 0.8) scale = text_round_scale_w(0.8, 0);
	}
	return scale;
}

float get_text_scale_fit_button(Widget *ww, int len, float scale)
{
	int h = ww->h;
	int w = ww->w - h / 2;
	if (w < h) w = h;
	return text_scale_fit(w, h, len, scale);
}

float text_scale_fit_button(Widget *ww, int len, float scale)
{
	return (ww->text_scale = get_text_scale_fit_button(ww, len, scale));
}

void pos_auto_textlen(Pos *p, int len, float scale)
{
	p->auto_w = pos_text_w(len, scale);
	p->auto_h = tx_font.tileh * scale;
}

Pos pos_get_prefsize(Widget *parent, Pos p)
{
	int w = 0;
	if (p.w > 0) {
		w = p.w;
	} else if (p.w == SIZE_AUTO && parent && parent->post.w > 0) {
		w = parent->post.w;
	}
	int h = 0;
	if (p.h > 0) {
		h = p.h;
	} else if (p.h == SIZE_AUTO && parent && parent->post.h > 0) {
		h = parent->post.h;
	}
	p.w = w;
	p.h = h;
	return p;
}

float pos_auto_text_scale_h(int h)
{
	int th = TXT_H_MEDIUM;
	if (h > 0) {
		if (h > H_NORMAL)  th = TXT_H_LARGE;  //scale = 1.2;
		if (h == H_NORMAL) th = TXT_H_MEDIUM; //scale = 1.1;
		if (h < H_NORMAL)  th = TXT_H_NORMAL; //scale = 1.0;
		if (h <= H_SMALL)  th = TXT_H_SMALL;  //scale = 0.9;
		if (h <= H_TINY)   th = TXT_H_TINY;   //scale = 0.8;
	}
	float scale = text_h_to_scale(th);
	return scale;
}

float pos_auto_text_scale(Widget *parent, Pos p)
{
	int h = pos_get_prefsize(parent, p).h;
	return pos_auto_text_scale_h(h);
}

void wgui_render_str2(Widget *ww, char *str, float add_zoom, u32 color, FontColor fc)
{
	if (ww->state == WGUI_STATE_INACTIVE) {
		//font_color_multiply(&disabled_fc, &fc, color);
		fc.outline = color_multiply(fc.outline, 0xFFFFFFC0);
		fc.shadow = color_multiply(fc.shadow, 0xFFFFFFC0);
		color = color_multiply(color, disabled_color);
	}
	font_color_multiply(&fc, &fc, color);
	float scale = ww->text_scale;
	int cx, alignx;
	int cy = ww->ay + ww->h / 2.0;
	if (ww->text_opt & WGUI_TEXT_ALIGN_LEFT) {
		alignx = -1;
		cx = ww->ax;
	} else if (ww->text_opt & WGUI_TEXT_ALIGN_RIGHT) {
		alignx = 1;
		cx = ww->ax + ww->w;
	} else {
		alignx = 0;
		cx = ww->ax + ww->w / 2.0;
	}
	if (ww->text_opt & WGUI_TEXT_SCALE_FIT) {
		scale = text_scale_fit(ww->w, ww->h, con_len(str), scale);
	} else if (ww->text_opt & WGUI_TEXT_SCALE_FIT_BUTTON) {
		scale = get_text_scale_fit_button(ww, con_len(str), scale);
	}
	//GRRLIB_Rectangle(ww->ax, ww->ay, ww->w, ww->h, 0x40408040, 1);
	Gui_PrintAlignZ(cx, cy, alignx, 0, &tx_font, fc, scale * add_zoom, str);
}

void wgui_render_str(Widget *ww, char *str, float add_zoom, u32 color)
{
	wgui_render_str2(ww, str, add_zoom, color, get_text_color(ww->text_color));
}

void wgui_render_text(Widget *ww)
{
	wgui_render_str(ww, ww->name, 1.0, ww->color);
}

float pos_auto_text(Widget *parent, Pos *p, const char *name)
{
	int len = name ? con_len(name) : 0;
	float scale = pos_auto_text_scale(parent, *p);
	pos_auto_textlen(p, len, scale);
	return scale;
}

Widget* wgui_add_text(Widget *parent, Pos p, const char *name)
{
	Widget *ww;
	int len = name ? con_len(name) : 0;
	float scale = pos_auto_text(parent, &p, name);
	ww = wgui_add(parent, p, (char*)name);
	if (ww) {
		ww->type = WGUI_TYPE_TEXT;
		ww->render = wgui_render_text;
		ww->text_scale = text_scale_fit(ww->w, ww->h, len, scale);
	}
	return ww;
}


//////// Text Box


void wgui_textbox_coords(Widget *ww, int *fw, int *fh, int *rows, int *cols)
{
	// round nearest font width (scale was already rounded down)
	int w = (int)roundf((float)tx_font.tilew * ww->text_scale);
	// round up font height
	int h = (int)ceilf((float)tx_font.tileh * ww->text_scale);
	if (fw) *fw = w;
	if (fh) *fh = h;
	if (cols) *cols = ww->w / w;
	if (rows) *rows = ww->h / h;
}

void wgui_render_textbox(Widget *ww)
{
	FontColor fc;
	FontColor fc1 = get_text_color(ww->text_color);
	font_color_multiply(&fc1, &fc, ww->color);
	int fw, fh, rows, cols;
	wgui_textbox_coords(ww, &fw, &fh, &rows, &cols);
	if (ww->opt) {
		GRRLIB_Rectangle(ww->ax, ww->ay, ww->w, ww->h, 0x80808080, 1);
	}
	//GRRLIB_Printf(ww->ax, ww->ay - 20, &tx_font, 0xffffffff, 1.0,
	//		"%d/%d %.4f %.4f %dx%d", ww->value, ww->val_max,
	//		ww->text_scale, ww->text_scale * tx_font.tilew, fw, fh);
	int page = ww->value;
	char *s = skip_lines(ww->name, page * rows);
	if (!s) return;
	int y = ww->ay;
	while (rows && *s) {
		GRRLIB_Print4(ww->ax, y, &tx_font, fc.color, fc.outline, fc.shadow,
				ww->text_scale, s, cols);
		s = strchr(s, '\n');
		if (!s) break;
		s++;
		rows--;
		y += fh;
	}
}

void wgui_textbox_wordwrap(Widget *ww, char *text, int strsize)
{
	if (!strsize || !text) return;
	ww->name = text;
	// round up scale so that font width aligns to a pixel
	int fw, fh, rows, cols;
	wgui_textbox_coords(ww, &fw, &fh, &rows, &cols);
	con_wordwrap(ww->name, cols, strsize);
	int num_pages = (count_lines(ww->name) + rows - 1) / rows;
	wgui_propagate_value(ww, SET_VAL_MAX, num_pages - 1);
}

Widget* wgui_add_textbox(Widget *parent, Pos p, int font_h, char *text, int strsize)
{
	Widget *ww;
	p.auto_w = pos_avail_w(parent);
	p.auto_h = pos_avail_h(parent);
	ww = wgui_add(parent, p, text);
	if (ww) {
		ww->type = WGUI_TYPE_TEXTBOX;
		ww->render = wgui_render_textbox;
		ww->text_color = GUI_TC_INFO;
		ww->text_scale = text_h_to_scale(font_h);
		wgui_textbox_wordwrap(ww, text, strsize);
	}
	return ww;
}


//////// Number


void wgui_render_num(Widget *ww)
{
	char str[100];
	snprintf(str, sizeof(str), ww->name, ww->opt + ww->value, ww->opt + ww->val_max);
	wgui_render_str(ww, str, 1.0, ww->color);
}

Widget* wgui_add_num(Widget *parent, Pos p, char *fmt, int base)
{
	Widget *ww;
	if (!fmt) fmt = "%d";
	pos_auto_textlen(&p, con_len(fmt), 1.0);
	ww = wgui_add(parent, p, fmt);
	if (ww) {
		ww->type = WGUI_TYPE_NUM;
		ww->render = wgui_render_num;
		ww->opt = base;
	}
	return ww;
}


//////// Dialog


void wgui_render_dialog(Widget *ww)
{
	wgui_DrawWindow(tx_window, ww->ax, ww->ay, ww->w, ww->h,
			get_gui_color(ww->dialog_color), ww->color, ww->text_scale, ww->name);
}

void pos_init_dialog(Widget *ww)
{
	pos_init(ww);
	pos_pad(ww, PAD1);
	pos_margin(ww, PAD3);
	pos_prefsize(ww, 0, H_NORMAL);
}

void text_scale_fit_dialog(Widget *ww)
{
	float scale = text_h_to_scale(TXT_H_HUGE);
	if (ww->name) {
		int len = con_len(ww->name);
		scale = text_scale_fit(ww->w - PAD1*2, TXT_H_HUGE, len, scale);
	}
	ww->text_scale = scale;
}

void wgui_dialog_ini(Widget *dialog)
{
	dialog->type = WGUI_TYPE_DIALOG;
	dialog->handle = handle_B_close;
	dialog->render = wgui_render_dialog;
	dialog->lock_focus = true;
	text_scale_fit_dialog(dialog);
	pos_init_dialog(dialog);
	if (dialog->name) {
		pos_move(dialog, 0, PAD1);
	}
}

void wgui_dialog_init(Widget *dialog, Pos p, char *title)
{
	Widget_init(dialog, NULL, p, title);
	wgui_dialog_ini(dialog);
}

Widget* wgui_add_dialog(Widget *parent, Pos p, char *name)
{
	Widget *ww;
	ww = wgui_add(parent, p, name);
	if (ww) {
		wgui_dialog_ini(ww);
	}
	return ww;
}


//////// Button


int wgui_handle_button(Widget *ww)
{
	int state = WGUI_STATE_NORMAL;
	if (wgui_over(ww)) {
		state = WGUI_STATE_HOVER;
		if (winput.w_buttons & WPAD_BUTTON_A) {
			state = WGUI_STATE_PRESS;
			wgui_input_steal_buttons();
		}
	}
	if (winput.w_buttons & ww->action_button) {
		state = WGUI_STATE_PRESS;
		wgui_input_steal_buttons();
	}
	ww->state = state;
	if (state == WGUI_STATE_PRESS) {
		wgui_action(ww);
	}
	return state;
}

float wgui_DrawButton(Widget *ww, char *str)
{
	u32 color = ww->color;
	int img_state = WGUI_IMG_NORMAL;
	GRRLIB_texImg *tx = tx_button;

	if (ww->state == WGUI_STATE_HOVER) {
		if (ww->zoom < ww->max_zoom) ww->zoom += 0.02;
		if (ww->zoom > ww->max_zoom) ww->zoom = ww->max_zoom;
		img_state = WGUI_IMG_HOVER;
	} else {
		if (ww->zoom > 1.0) ww->zoom -= 0.02;
		if (ww->zoom < 1.0) ww->zoom = 1.0;
	}
	
	if (ww->type == WGUI_TYPE_RADIO) {
		if (tx_radio) tx = tx_radio;
		Widget *rr = ww->link_first;
		if (rr && rr->value == ww->link_index) {
			img_state = WGUI_IMG_PRESS;
		} 
	} else if (ww->type == WGUI_TYPE_CHECKBOX) {
		if (tx_checkbox) tx = tx_checkbox;
		if (ww->value) {
			img_state = WGUI_IMG_PRESS;
		}
	}
	if (ww->custom_tx) {
		if (tx_custom[ww->tx_idx]) tx = tx_custom[ww->tx_idx];
	}
	if (img_state == WGUI_IMG_PRESS && ww->state == WGUI_STATE_HOVER) {
		img_state = WGUI_IMG_PRESS_HOVER;
	}
	if (ww->state == WGUI_STATE_INACTIVE) {
		img_state = WGUI_IMG_INACTIVE;
	}
	wgui_DrawButtonX(tx, ww->ax, ww->ay, ww->w, ww->h, ww->zoom, color, img_state);
	if (ww->state == WGUI_STATE_PRESS) ww->click = 1.0;
	if (ww->click > 0 && img_state != WGUI_IMG_PRESS && img_state != WGUI_IMG_PRESS_HOVER) {
		color = 0xFFFFFF00 | (u8)(ww->click * 255);
		wgui_DrawButtonX(tx, ww->ax, ww->ay, ww->w, ww->h, ww->zoom, color, WGUI_IMG_PRESS);
	}
	wgui_render_str(ww, str, ww->zoom, ww->color);
	
	if (ww->click > 0) {
		color = 0xFFFFFF00 | (u8)(ww->click * 255);
		wgui_DrawButtonX(tx, ww->ax, ww->ay, ww->w, ww->h, ww->zoom, color, WGUI_IMG_FLASH);
		ww->click -= 0.06;
		if (ww->click < 0) ww->click = 0;
	}
		
	/*char str[16];
	sprintf(str, "%.2f %.2f", txt_scale, tx_font.tilew*txt_scale);
	Gui_PrintAlignZ(cx, y+h, 0, -1, &tx_font, fc, 0.7, str);*/
	return ww->zoom;
}

void wgui_render_button(Widget *ww)
{
	wgui_DrawButton(ww, ww->name);
}

int button_auto_w(Widget *parent, Pos p, char *name)
{
	return H_NORMAL + con_len(name) * tx_font.tilew * 1.1;
}

void pos_auto_button(Pos *p, char *name)
{
	p->auto_w = H_SMALL*2 + con_len(name) * tx_font.tilew * 1.1;
	p->auto_h = H_SMALL;
}

float pos_auto_button_scale_len(Widget *parent, Pos *p, int len)
{
	int h = pos_get_prefsize(parent, *p).h;
	if (!h) h = H_NORMAL;
	float scale = pos_auto_text_scale_h(h);
	int w = h + pos_text_w(len, scale);
	p->auto_w = w;
	p->auto_h = h;
	return scale;
}

float pos_auto_button_scale(Widget *parent, Pos *p, char *name)
{
	return pos_auto_button_scale_len(parent, p, con_len(name));
}

Widget* wgui_add_button(Widget *parent, Pos p, char *name)
{
	float scale = pos_auto_button_scale(parent, &p, name);
	Widget *ww = wgui_add(parent, p, name);
	if (ww) {
		ww->type = WGUI_TYPE_BUTTON;
		ww->handle = wgui_handle_button;
		ww->render = wgui_render_button;
		ww->text_color = GUI_TC_BUTTON;
		text_scale_fit_button(ww, con_len(name), scale);
	}
	return ww;
}


//////// Checkbox


int wgui_handle_checkbox(Widget *ww)
{
	int state = WGUI_STATE_NORMAL;
	if (wgui_over(ww)) {
		state = WGUI_STATE_HOVER;
		if (winput.w_buttons & WPAD_BUTTON_A) {
			state = WGUI_STATE_PRESS;
			// propagate value and call action hooks
			wgui_propagate_value(ww, SET_VAL | SET_VAL_ACTION, ww->value ? 0 : 1);
			wgui_input_steal_buttons();
		}
	}
	ww->state = state;
	return state;
}

void wgui_RenderCheckbox(Widget *ww)
{
	char *val_text;
	if (ww->list_values && ww->list_num == 2) {
		val_text = ww->value ? ww->list_values[1] : ww->list_values[0];
	} else {
		val_text = ww->value ? gt("On") : gt("Off");
	}
	char str[100];
	if (ww->opt) {
		snprintf(str, sizeof(str), "%s: %s", ww->name, val_text);
		val_text = str;
	}
	wgui_DrawButton(ww, val_text);
}

Widget* wgui_add_checkboxx(Widget *parent, Pos p, char *name, bool show_name,
		char *off, char *on)
{
	int len = con_len(gt("Off"));
	if (off && on) len = con_len(off);
	if (show_name) len = con_len(name?name:"") + 2 + len; // 2=": "
	float scale = pos_auto_button_scale_len(parent, &p, len);
	Widget *ww = wgui_add(parent, p, name);
	if (ww) {
		ww->type = WGUI_TYPE_CHECKBOX;
		ww->update = update_val_from_ptr_bool;
		ww->handle = wgui_handle_checkbox;
		ww->render = wgui_RenderCheckbox;
		ww->opt = show_name;
		ww->val_max = 1;
		ww->text_color = GUI_TC_CHECKBOX;
		text_scale_fit_button(ww, len, scale);
		if (off && on) {
			char *vlist[2] = { off, on };
			wgui_set_value_list(ww, 2, vlist);
		}
	}
	return ww;
}

Widget* wgui_add_checkbox(Widget *parent, Pos p, char *name, bool show_name)
{
	return wgui_add_checkboxx(parent, p, name, show_name, NULL, NULL);
}


//////// Radio


int wgui_handle_radio(Widget *ww)
{
	int state = WGUI_STATE_NORMAL;
	if (wgui_over(ww)) {
		Widget *rr = ww->link_first;
		// ignore hover and action if already selected
		if (rr->value != ww->link_index) {
			state = WGUI_STATE_HOVER;
			if (winput.w_buttons & WPAD_BUTTON_A) {
				state = WGUI_STATE_PRESS;
				// set radio value to instance index and call action hooks
				wgui_propagate_value(rr, SET_VAL | SET_VAL_ACTION, ww->link_index);
				wgui_input_steal_buttons();
			}
		}
	}
	ww->state = state;
	return state;
}

void wgui_render_radio(Widget *ww)
{
	//int i;
	//Widget *rr = ww->link_first;
	//char str[100]; // dbg
	//sprintf(str, "%d.%d %s", rr->value, ww->link_index, ww->name);
	//float zoom = 
	wgui_DrawButton(ww, ww->name);
	/*
	if (rr->value == ww->link_index) {
		float cx, cy, w2, h2;
		float pad = 1.5;
		int x, y, w, h;
		u32 color = color_multiply(0xEE2222FF, ww->color);
		if (ww->state == WGUI_STATE_INACTIVE) {
			color = color_multiply(color, disabled_color);
		}

		w2 = (float)ww->w / 2.0;
		h2 = (float)ww->h / 2.0;
		cx = (float)ww->ax + w2;
		cy = (float)ww->ay + h2;
		x = cx - w2 * zoom - pad;
		y = cy - h2 * zoom - pad;
		w = (float)ww->w * zoom + pad * 2.0 + 1;
		h = (float)ww->h * zoom + pad * 2.0 + 1;
		for (i=0; i<4; i++) {
			GRRLIB_Rectangle(x-i, y-i, w+i*2, h+i*2, color, 0);
			GRRLIB_Plot(x-i, y-i, color);
		}
	}
	*/
}

Widget* wgui_add_radio(Widget *parent, Widget *radio, Pos p, char *name)
{
	float scale = pos_auto_button_scale(parent, &p, name);
	Widget *ww = wgui_add(parent, p, name);
	if (ww) {
		ww->type = WGUI_TYPE_RADIO;
		ww->handle = wgui_handle_radio;
		ww->render = wgui_render_radio;
		ww->text_color = GUI_TC_RADIO;
		text_scale_fit_button(ww, con_len(name), scale);
		wgui_link(radio, ww);
		if (!radio) {
			ww->update = update_val_from_ptr_int;
			// -1 means no radio selected
			ww->val_min = -1;
		} else {
			radio = radio->link_first;
			// without force, this only increases max which is
			// handy when combined with page or other value links
			// in case max is already higher
			wgui_propagate_value(radio, SET_VAL_MAX, ww->link_index);
		}
	}
	return ww;
}

void wgui_radio_set(Widget *ww, int val)
{
	// no action
	wgui_set_value(ww, val);
}

Widget* wgui_auto_radio2(Widget *parent, Widget *rr, int cols, int n, char *names[])
{
	Widget *ww;
	int i;
	for (i=0; i<n; i++) {
		if (cols > 0 && i > 0 && i % cols == 0 ) {
			pos_newline(parent);
			pos_move_abs(parent, rr->x, parent->post.y);
		}
		ww = wgui_add_radio(parent, rr, pos_auto, names[i]);
		if (i == 0) rr = ww;
	}
	return rr;
}

Widget* wgui_auto_radio(Widget *parent, int cols, int n, char *names[])
{
	return wgui_auto_radio2(parent, NULL, cols, n, names);
}

Widget* wgui_auto_radio_a(Widget *parent, int cols, int n, ...)
{
	int i;
	va_list ap;
	char *names[n];
	va_start(ap, n);
	for (i=0; i<n; i++) names[i] = va_arg(ap, char *);
	va_end(ap);
	return wgui_auto_radio(parent, cols, n, names);
}

char* get_longest_str(int num, char *str[])
{
	int i, len, maxlen = 0;
	char *s = "";
	for (i=0; i < num; i++) {
		if (str[i]) {
			len = con_len(str[i]);
			if (len > maxlen) {
				maxlen = len;
				s = str[i];
			}
		}
	}
	return s;
}

void pos_arrange_radio(Widget *parent, Pos *p, int cols, int rows, int n, char *names[])
{
	Pos tmp;
	if (p->w == SIZE_AUTO) {
		// use max auto size of names
		char *s = get_longest_str(n, names);
		tmp = pos_auto;
		pos_auto_button_scale(parent, &tmp, s);
		pos_auto_expand(parent, &tmp);
		p->w = cols * (tmp.w + parent->post.pad) - parent->post.pad;
		int avail = pos_avail_w(parent);
		if (p->w > avail) p->w = avail;
	}
	if (p->h == SIZE_AUTO) {
		tmp = pos_auto;
		pos_auto_button_scale(parent, &tmp, "");
		pos_auto_expand(parent, &tmp);
		p->h = rows * (tmp.h + parent->post.pad) - parent->post.pad;
	}
}

Widget* wgui_arrange_radio(Widget *parent, Pos p, int cols, int n, char *names[])
{
	Widget *rr = NULL;
	int pref_w = parent->post.w;
	int pref_h = parent->post.h;
	int ph = p.h;
	int rows = (n + cols - 1) / cols;
	pos_arrange_radio(parent, &p, cols, rows, n, names);
	pos_auto_expand(parent, &p);
	pos_columns(parent, cols, p.w);
	if (ph != SIZE_AUTO) {
		pos_rows(parent, rows, p.h);
	}
	pos_move_abs(parent, p.x, p.y);
	rr = wgui_auto_radio(parent, cols, n, names);
	pos_prefsize(parent, pref_w, pref_h);
	return rr;
}

Widget* wgui_arrange_radio_a(Widget *parent, Pos p, int cols, int n, ...)
{
	int i;
	va_list ap;
	char *names[n];
	va_start(ap, n);
	for (i=0; i<n; i++) names[i] = va_arg(ap, char *);
	va_end(ap);
	return wgui_arrange_radio(parent, p, cols, n, names);
}

// old:
/*
order:
 1 2 3
 4 5 6
*/
// return: bottom y coord (max y)
/*
int wgui_arrange(Widget *ww, int n,
		int ax, int ay, int aw, int ah, // area
		int cols, int maxh, int pad)
{
	Widget *pp = ww->parent;
	Widget **cc = pp->child;
	int i;
	int first = -1;
	int maxy;
	
	if (ay < 0) ay = pp->h + ay;
	if (aw <= 0) aw = aw + pp->w - ax * 2;
	if (ah <= 0) ah = ah + pp->h - ay;
	maxy = ay;

	for (i = 0; i < pp->num_child; i++) {
		if (cc[i] == ww) {
			first = i;
			break;
		}
	}
	if (first < 0) return maxy;
	int w, h;
	int rows = (n + cols - 1) / cols;
	w = (aw - pad) / cols - pad;
	h = (ah - pad) / rows - pad;
	if (h > maxh) h = maxh;
	int r, c;
	for (i = 0; i < n; i++) {
		ww = cc[first + i];
		r = i / cols;
		c = i % cols;
		ww->x = ax + pad + c * (w + pad);
		ww->y = ay + pad + r * (h + pad);
		ww->w = w;
		ww->h = h;
		if (ww->y + h > maxy) maxy = ww->y + h;
		adjust_position(ww);
	}
	return maxy;
}

Widget* auto_radio(Widget *parent, int x, int y, int cols, int n, char *names[], int *max_y)
{
	Widget *rr = NULL;
	Widget *ww;
	int i, my;
	for (i=0; i<n; i++) {
		ww = wgui_add_radio(parent, rr, 1, 1, 1, 1, names[i]);
		if (i == 0) rr = ww;
	}
	my = wgui_arrange(rr, n, x, y, 0, 0, cols, 40, PAD1);
	if (max_y) *max_y = my;
	return rr;
}
*/


//////// Page


int wgui_set_value_page(Widget *ww, int flags, int val)
{
	val = wgui_set_value_local_x(ww, flags, val);
	// update active page
	Widget *page = ww->link_first;
	int v = page->value;
	while (page) {
		wgui_set_disabled(page, page->link_index != v);
		page = page->link_next;
	}
	return val;
}

void wgui_render_page(Widget *ww)
{
	u32 wcolor = get_gui_color(ww->dialog_color);
	GRRLIB_texImg *tx = tx_page;
	if (!tx) {
		tx = tx_window;
		wcolor = color_multiply(wcolor, 0x80808080);
	}
	wgui_DrawWindow(tx, ww->ax, ww->ay, ww->w, ww->h,
			wcolor, ww->color, 0.0, NULL);
}

Widget* wgui_add_page(Widget *parent, Widget *page, Pos p, char *name)
{
	if (page) {
		if (p.x == POS_AUTO) p.x = page->x - parent->post.margin;
		if (p.y == POS_AUTO) p.y = page->y - parent->post.margin;
		if (p.w == SIZE_AUTO) p.w = page->w;
		if (p.h == SIZE_AUTO) p.h = page->h;
	}
	Widget *ww = wgui_add(parent, p, name);
	if (ww) {
		ww->type = WGUI_TYPE_PAGE;
		ww->handle = NULL;
		ww->render = wgui_render_page;
		ww->dialog_color = 0xFFFFFFFF;
		// link pages
		wgui_link(page, ww);
		if (!page) {
			// this is first page
			// link to radio value
			page = ww;
			page->set_value = wgui_set_value_page;
			pos_margin(ww, PAD1);
			pos_pad(ww, PAD1);
		} else {
			wgui_propagate_value(page->link_first, SET_VAL_MAX, ww->link_index);
			pos_margin(ww, page->post.margin);
			pos_pad(ww, page->post.pad);
			pos_prefsize(ww, page->post.w, page->post.h);
			ww->dialog_color = page->dialog_color;
		}
		// disable page if not selected
		wgui_set_disabled(ww, ww->link_index != page->value);
	}
	return ww;
}

Widget* wgui_add_pages(Widget *parent, Pos p, int num, char *name)
{
	int i;
	Widget *page = NULL;
	Widget *ww = NULL;
	for (i=0; i<num; i++) {
		ww = wgui_add_page(parent, page, p, name);
		if (!page) {
			page = ww;
			p = pos_auto;
		}
	}
	return page;
}

Widget* wgui_page_get(Widget *page, int index)
{
	return wgui_link_get(page, index);
}

void wgui_link_page_ctrl(Widget *page, Widget *ctrl)
{
	if (page->link_first) page = page->link_first;
	if (page && ctrl) wgui_link_value(page, ctrl);
}

// create as many pages as is ctrl max value (+1)
Widget* wgui_add_pages_ctrl(Widget *parent, Pos p, Widget *ctrl, char *name)
{
	if (!ctrl) return NULL;
	int num = ctrl->val_max + 1;
	Widget *page = wgui_add_pages(parent, p, num, name);
	wgui_link_page_ctrl(page, ctrl);
	return page;
}


//////// Num Switch


int wgui_num_expected_len(char *fmt, int expected_max)
{
	char tmp[100];
	snprintf(tmp, sizeof(tmp), fmt, expected_max, expected_max);
	return con_len(tmp);
}

Widget* wgui_add_numswitch(Widget *parent, Pos p, char *fmt, int base, int expected_max)
{
	Widget *ww;
	if (!fmt) fmt = "%2d / %-2d";
	int len = wgui_num_expected_len(fmt, expected_max);
	float scale = 1.0;
	pos_auto_textlen(&p, len + 4, scale);
	ww = wgui_add(parent, p, fmt);
	if (ww) {
		ww->type = WGUI_TYPE_NUMSWITCH;

		Widget *cc;
		int h = ww->h;
		int w = h;
		if (w * 3 > ww->w) w = ww->w / 3;
		
		cc = wgui_add_button(ww, pos(0, 0, w, h), "<");
		cc->action = wgui_action_change_parent_val;
		cc->action_button = WPAD_BUTTON_UP;
		cc->value = -1;
		
		cc = wgui_add_num(ww, pos(w, 0, -w, h), fmt, base);
		wgui_link_value(ww, cc);
		cc->text_scale = scale;

		cc = wgui_add_button(ww, pos(-w, 0, w, h), ">");
		cc->action = wgui_action_change_parent_val;
		cc->action_button = WPAD_BUTTON_DOWN;
		cc->value = 1;

		// resize container
		ww->w = cc->x + cc->w;
		parent->post.x = ww->x + ww->w + parent->post.pad;
	}
	return ww;
}

// dir: -1 button left, 0: num, 1: button right
Widget *wgui_numswitch_get_element(Widget *ww, int dir)
{
	ww = wgui_find_child_type(ww, WGUI_TYPE_NUMSWITCH);
	if (ww) return ww->child[dir + 1];
	return NULL;
}

//////// Page Switch


Widget* wgui_add_pgswitchx(Widget *parent, Widget *page, Pos p, char *name,
		int pad_len, char *fmt, int expected_max)
{
	Widget *ww;
	if (!name) name = gt("Page:");
	if (!fmt) fmt = "%2d / %-2d";
	int num_len = wgui_num_expected_len(fmt, expected_max);
	int name_len = con_len(name) + pad_len;
	pos_auto_textlen(&p, name_len + num_len, 1.0);
	ww = wgui_add(parent, p, name);
	if (ww) {
		ww->type = WGUI_TYPE_PGSWITCH;
		if (page) wgui_link_page_ctrl(page, ww);
		Widget *cc;
		int h = ww->h;
		pos_auto_textlen(&p, name_len, 1.0);
		int w1 = p.auto_w;
		pos_auto_textlen(&p, num_len, 1.0);
		int w2 = p.auto_w + h * 2;
		int w = w1 + w2;
		int x = ww->w - w;
		if (x < 0) x = 0;
		
		cc = wgui_add_text(ww, pos(x, 0, w1, h), name);
		cc->text_scale = 1.0;
		
		cc = wgui_add_numswitch(ww, pos(x+w1, 0, w2, h), fmt, 1, 9);
		wgui_link_value(ww, cc);

		// resize container
		ww->w = cc->x + cc->w;
		parent->post.x = ww->x + ww->w + parent->post.pad;
	}
	return ww;
}

Widget* wgui_add_pgswitch(Widget *parent, Widget *page, Pos p, char *name)
{
	return wgui_add_pgswitchx(parent, page, p, name, 1, NULL, 99);
}


//////// ListBox


int wgui_set_value_listbox(Widget *ww, int flags, int val)
{
	val = wgui_set_value_local_x(ww, flags, val);
	// update text value
	Widget *txt = ww->child[1];
	int v = ww->value;
	if (v >= 0 && v < ww->list_num) {
		txt->name = ww->list_values[v];
	} else {
		txt->name = "";
	}
	return val;
}

void wgui_render_listbox(Widget *ww)
{
	GRRLIB_Rectangle(ww->ax + ww->h / 2, ww->ay, ww->w - ww->h, ww->h, 0x60606080, 1);
}

void wgui_listbox_set_values(Widget *ww, int n, char **values)
{
	wgui_set_value_list(ww, n, values);
}

float pos_auto_listbox(Widget *parent, Pos *p, int n, char **values)
{
	char *s = get_longest_str(n, values);
	int maxlen = con_len(s) + 2;
	int h = pos_get_prefsize(parent, *p).h;
	if (!h) h = H_NORMAL;
	float scale = pos_auto_text_scale(parent, *p);
	p->auto_w = h * 2 + pos_text_w(maxlen, scale);
	p->auto_h = h;
	return scale;
}
		
Widget* wgui_add_listbox1(Widget *parent, Pos p, char *name, int n, char **values)
{
	float scale = pos_auto_listbox(parent, &p, n, values);
	Widget *ww = wgui_add(parent, p, name);
	Widget *aa;
	if (ww) {
		ww->type = WGUI_TYPE_LISTBOX;
		ww->update = update_val_from_ptr_int;
		ww->set_value = wgui_set_value_listbox;
		ww->render = wgui_render_listbox;
		int h = ww->h;
		// button <
		aa = wgui_add_button(ww, pos(0, 0, h, h), "<");
		aa->action = wgui_action_change_parent_val;
		aa->value = -1;
		// text value area
		aa = wgui_add_text(ww, pos(h, 0, -h, h), n > 0 ? values[0] : NULL);
		aa->text_scale = scale;
		aa->text_opt = WGUI_TEXT_SCALE_FIT;
		// button >
		aa = wgui_add_button(ww, pos(-h, 0, h, h), ">");
		aa->action = wgui_action_change_parent_val;
		aa->value = 1;
		// set list of values
		wgui_listbox_set_values(ww, n, values);
	}
	return ww;
}

Widget* wgui_add_listbox(Widget *parent, Pos p, int n, char **values)
{
	return wgui_add_listbox1(parent, p, NULL, n, values);
}

// extended listbox

void wgui_render_listboxx(Widget *ww)
{
	if (ww->state == WGUI_STATE_HOVER || ww->state == WGUI_STATE_PRESS) {
		//wgui_render_button(ww);
		wgui_DrawButtonX(tx_button, ww->ax, ww->ay, ww->w, ww->h,
				1.0, 0xFFFFFFFF, WGUI_IMG_HOVER);
		wgui_render_str2(ww, ww->name, 1.0, ww->color, CFG.gui_tc[GUI_TC_BUTTON]);
	} else {
		wgui_render_text(ww);
	}
}

bool wgui_action_listboxx_base(Widget *ww)
{
	Widget *ll = ww->parent;
	if (ll->list_num <= 1) return true;
	// if only 2 values, just switch as a checkbox
	if (ll->list_num == 2) {
		wgui_propagate_value(ll, SET_VAL | SET_VAL_ACTION, ll->value ? 0 : 1);
		return true;
	}
	// return false if action unhandled and a dialog needs to be opened
	return false;
}

Widget* wgui_listboxx_open_dialog(Widget *ww)
{
	Widget *ll = ww->parent;
	// find primary parent (the top level dialog)
	Widget *parent = wgui_primary_parent(ll);
	Pos p = pos(50, 50, 640-100, 480-50-PAD2);
	Widget *dd = wgui_add_dialog(parent, p, ll->name);
	dd->ax = 50;
	dd->ay = 50;
	dd->dialog_color = GUI_COLOR_POPUP;
	return dd;
}

void wgui_listboxx_init_dialog(Widget *dd, Widget *ll)
{
	// dialog was opened, fill up with radio buttons
	// find width of the longest value
	Pos p1 = pos_auto;
	pos_arrange_radio(dd, &p1, 1, 1, ll->list_num, ll->list_values);
	int cols = (ll->list_num + 5 - 1) / 5;
	Pos p = pos_auto;
	p.x = POS_CENTER;
	if (cols == 1 && p1.w < W_NORMAL) p.w = W_NORMAL;
	if (ll->list_num < 5) pos_newline(dd);
	Widget *rr = wgui_arrange_radio(dd, p, cols, ll->list_num, ll->list_values);
	wgui_link_value(ll, rr);
	Widget *cc = wgui_add_button(dd, pos(POS_EDGE, POS_EDGE, W_NORMAL, H_NORMAL), gt("Back"));
	cc->action = action_close_parent;
}

void wgui_action_listboxx(Widget *ww)
{
	if (wgui_action_listboxx_base(ww)) return;
	Widget *dd = wgui_listboxx_open_dialog(ww);
	Widget *ll = ww->parent;
	wgui_listboxx_init_dialog(dd, ll);
}

Widget* wgui_add_listboxx(Widget *parent, Pos p, char *name, int n, char **values)
{
	Widget *ww = wgui_add_listbox1(parent, p, name, n, values);
	ww->child[1]->handle = wgui_handle_button;
	ww->child[1]->action = wgui_action_listboxx;
	ww->child[1]->render = wgui_render_listboxx;
	return ww;
}


//////// super listbox

// as listboxx + auto paginate if too many values

Widget* wgui_paginate_radio(Widget *parent, Pos p, int cols, int rows, int n, char *names[])
{
	Widget *r1 = NULL;
	Widget *rr = NULL;
	Widget *pp = NULL;
	int pref_w = parent->post.w;
	int pref_h = parent->post.h;
	//int ph = p.h;
	//int rows = (n + cols - 1) / cols;
	pos_arrange_radio(parent, &p, cols, rows + 1, n, names);
	pos_auto_expand(parent, &p);
	int old_margin = pos_margin(parent, 0);
	p.h -= H_NORMAL;
	int per_page = cols * rows;
	int num_pages = (n + per_page - 1) / per_page;
	int page;
	int i;
	for (page = 0; n > 0 && page < num_pages; page++) {
		pp = wgui_add_page(parent, pp, p, NULL);
		pos_columns(pp, cols, SIZE_FULL);
		pos_rows(pp, rows, SIZE_FULL);
		i = page * per_page;
		if (per_page > n) per_page = n;
		rr = wgui_auto_radio2(pp, r1, cols, per_page, names + i);
		n -= per_page;
		if (!r1) r1 = rr;
	}
	pos_margin(parent, old_margin);
	pos_newline(parent);
	//Widget *pswitch = 
	wgui_add_pgswitch(parent, pp, pos_auto, NULL);
	pos_prefsize(parent, pref_w, pref_h);
	return r1;
}

void wgui_action_superbox(Widget *ww)
{
	if (wgui_action_listboxx_base(ww)) return;
	Widget *dd = wgui_listboxx_open_dialog(ww);
	// dialog was opened, fill up with radio buttons
	Widget *ll = ww->parent;
	Widget *rr;
	bool paginate = false;
	if (ll->list_num > 15) {
		paginate = true;
	} else {
		// find width of the longest value
		Pos p1 = pos_auto;
		pos_arrange_radio(dd, &p1, 1, 1, ll->list_num, ll->list_values);
		// allow 0.8 auto scale if that fits all to one page
		int w = p1.w * 0.8;
		int cols = pos_avail_w(dd) / w;
		if (cols < 1) cols = 1;
		int rows = (ll->list_num + cols - 1) / cols;
		if (rows > 5) paginate = true;
	}
	if (!paginate) {
		wgui_listboxx_init_dialog(dd, ll);
	} else {
		// auto paginate
		// max: 2 columns, 5 rows
		// 1 row for page and back
		rr = wgui_paginate_radio(dd, pos_fill, 2, 5, ll->list_num, ll->list_values);
		wgui_link_value(ll, rr);
		Widget *page = rr->parent;
		wgui_set_value(page, ll->value / (2*5));
		Widget *cc = wgui_add_button(dd, pos(POS_EDGE, POS_AUTO, W_NORMAL, H_NORMAL), gt("Back"));
		cc->action = action_close_parent;
	}
}

Widget* wgui_add_superbox(Widget *parent, Pos p, char *name, int n, char **values)
{
	Widget *ww = wgui_add_listbox1(parent, p, name, n, values);
	ww->child[1]->handle = wgui_handle_button;
	ww->child[1]->action = wgui_action_superbox;
	ww->child[1]->render = wgui_render_listboxx;
	return ww;
}


//////// Option

// name: (<) _value_ (>)


Widget* wgui_add_label(Widget *parent, Pos p, char *name)
{
	Widget *ww = wgui_add_text(parent, p, name);
	ww->text_opt = WGUI_TEXT_ALIGN_LEFT;
	return ww;
}

void pos_auto_opt(Widget *parent, Pos *p, int pad,
		char *name, int num, char **values)
{
	Pos p1 = *p;
	Pos p2 = *p;
	pos_auto_text(parent, &p1, name);
	pos_auto_listbox(parent, &p2, num, values);
	p->auto_w = p1.auto_w + pad + p2.auto_w;
	p->auto_h = p2.auto_h;
}

wgui_Option wgui_add_option_base(Widget *parent, Pos p, int pad, float w_ratio, char *name)
{
	wgui_Option opt;
	Pos pos;
	if (!p.auto_w || !p.auto_h) {
		pos_auto_text(parent, &p, name);
		if (w_ratio > 0) {
			p.auto_w = p.auto_w / w_ratio + pad;
		} else {
			p.auto_w = p.auto_w * 2 + pad;
		}
	}
	opt.base = wgui_add(parent, p, name);
	opt.base->type = WGUI_TYPE_OPTION;
	pos = pos_full;
	if (w_ratio > 0) {
		pos.w = (opt.base->w - pad) * w_ratio;
	} else {
		pos.w = SIZE_AUTO;
	}
	opt.name = wgui_add_label(opt.base, pos, name);
	pos_move(opt.base, pad, 0);
	return opt;
}

wgui_Option wgui_add_option(Widget *parent, Pos p, int pad, float w_ratio,
		char *name, int num, char **values)
{
	wgui_Option opt;
	char *on_off[2] = { gt("Off"), gt("On") };
	if (num == 2 && values == NULL) {
		values = on_off;
		// alternative: use checkbox or radio
	}
	bool use_button = false;
	if (num == -1) {
		use_button = true;
		num = 1;
	}
	pos_auto_opt(parent, &p, pad, name, num, values);
	opt = wgui_add_option_base(parent, p, pad, w_ratio, name);
	Pos pos = pos_wh(SIZE_FULL, SIZE_FULL);
	if (use_button) {
		opt.value = wgui_add_button(opt.base, pos, values ? values[0] : NULL);
	} else {
		//opt.value = wgui_add_listboxx(opt.base, pos, name, num, values);
		opt.value = wgui_add_superbox(opt.base, pos, name, num, values);
		opt.value->update = update_val_from_ptr_int_active;
	}
	opt.control = opt.value;
	return opt;
}

Widget* wgui_add_opt(Widget *parent, char *name, int num, char **values)
{
	Pos p = pos_auto;
	p.w = SIZE_FULL;
	// 40% name 60% value
	wgui_Option opt = wgui_add_option(parent, p, 2, 0.40, name, num, values);
	pos_newline(parent);
	return opt.control;
}

Widget* wgui_add_opt_button(Widget *parent, char *name, char *value)
{
	char *names[1];
	names[0] = value;
	return wgui_add_opt(parent, name, -1, names);
}

Widget* wgui_add_opt_a(Widget *parent, char *name, int num, ...)
{
	int i;
	va_list ap;
	char *names[num];
	va_start(ap, num);
	for (i=0; i<num; i++) names[i] = va_arg(ap, char *);
	va_end(ap);
	return wgui_add_opt(parent, name, num, names);
}

Widget* wgui_add_opt_map(Widget *parent, char *name, struct TextMap *map)
{
	int num = map_get_num(map);
	char *names[num];
	map_to_list(map, num, names);
	return wgui_add_opt(parent, name, num, names);
}

Widget* wgui_add_opt_array(Widget *parent, char *name, int num, int size, char array[][size])
{
	int i;
	char *names[num];
	for (i=0; i<num; i++) names[i] = array[i];
	return wgui_add_opt(parent, name, num, names);
}


//////// 



