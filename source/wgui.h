#ifndef WGUI_H
#define WGUI_H 1

#include "my_GRRLIB.h"
#include "wpad.h"
#include "cfg.h" // FontColor

#define WGUI_STATE_NORMAL   0
#define WGUI_STATE_HOVER    1
#define WGUI_STATE_PRESS    2
#define WGUI_STATE_INACTIVE 3 // no handle, render gray
#define WGUI_STATE_DISABLED 4 // no handle, no render, no input steal

#define WGUI_TYPE_WIDGET     0
#define WGUI_TYPE_CONTAINER  0x100
#define WGUI_TYPE_TEXT       1
#define WGUI_TYPE_TEXTBOX    2
#define WGUI_TYPE_NUM        3
#define WGUI_TYPE_DIALOG     0x104
#define WGUI_TYPE_BUTTON     5
#define WGUI_TYPE_CHECKBOX   6
#define WGUI_TYPE_RADIO      7
#define WGUI_TYPE_LISTBOX    8
#define WGUI_TYPE_PAGE       0x109
#define WGUI_TYPE_NUMSWITCH 10
#define WGUI_TYPE_PGSWITCH  11
#define WGUI_TYPE_OPTION    12
#define WGUI_TYPE_USER      20

// use full parent area from pos to edge
#define SIZE_FULL  0
#define SIZE_AUTO -1 

#define POS_AUTO   -1
#define POS_CENTER -2
#define POS_MIDDLE -3 // from current pos to edge
#define POS_EDGE   -4 // right or bottom
// #define POS_ABS 0x1000

#define PAD00 8
#define PAD0 12 
#define PAD1 16
#define PAD2 24
#define PAD3 32

#define H_TINY   24
#define H_SMALL  32
#define H_NORMAL 40
#define H_MEDIUM 48
#define H_LARGE  56
#define H_HUGE   64

#define W_SMALL  100
#define W_NORMAL 160
#define W_MEDIUM 180
#define W_LARGE  200

/* text sizes and scaling:
default font image is 352 x 304 with 32x16 chars so one char is 11x19
scaled text looks best if scaled width is pixel aligned
multiline text uses rounded up height for positioning
Useful sizes: (ns = nearest simple scale)
w  ns  scale  h        hh ww
 8 0.7 0.7272 13.8181  14  8.1048
 9 0.8 0.8181 15.5454  16  9.2631
10 0.9 0.9090 17.2727  18 10.4203
11 1.0 1.0    19       19 11
12 1.1 1.0909 20.7272  21 12.1572
13 1.2 1.1818 22.4545  23 13.3155
14 1.3 1.2727 24.1818  25 14.4727
*/

#define TXT_H_TINY    16 // 0.8
#define TXT_H_SMALL   18 // 0.9
#define TXT_H_NORMAL  19 // 1.0
#define TXT_H_MEDIUM  21 // 1.1
#define TXT_H_LARGE   23 // 1.2
#define TXT_H_HUGE    25 // 1.3

#define WGUI_TEXT_ALIGN_CENTER 0
#define WGUI_TEXT_ALIGN_LEFT   1
#define WGUI_TEXT_ALIGN_RIGHT  2
#define WGUI_TEXT_SCALE_FIT    4
#define WGUI_TEXT_SCALE_FIT_BUTTON 8

#define SET_VAL          1 
#define SET_VAL_FORCE    2 
#define SET_VAL_ACTION   4 // call action on value change
#define SET_VAL_MAX      8
#define SET_VAL_MIN     16

struct Wgui_Input
{
	ir_t *w_ir;
	int *p_buttons;
	int *p_held;
	int w_buttons;
};

typedef struct Pos
{
	int x, y;
	int w, h;
	int auto_w, auto_h;
} Pos;

typedef struct PosState
{
	int x, y;
	int w, h; // pref size
	int pad;
	int margin;
} PosState;

typedef struct Widget Widget;

struct Widget
{
	int type;
	int x, y; // relative
	int ax, ay; // absolute
	int w, h;
	char *name;
	int state; // normal, hover, disabled, pressed
	u32 color;
	bool custom_tx;
	int tx_idx;
	bool closing; // mark close
	bool lock_focus;
	PosState post;
	float text_scale;
	//FontColor text_color;
	int text_color; // index
	int text_opt; // align, fit
	float zoom;   // pointer-over zoom amount
	float max_zoom; // max hover zoom (1.1)
	float click;  // button press flash amount
	
	// update internal state/value from external source
	void (*update)(Widget *ww);
	// handle input
	int (*handle)(Widget *ww);
	// action on input
	void (*action)(Widget *ww);
	void (*action2)(Widget *ww);
	void (*render)(Widget *ww);
	// local set value override
	int (*set_value)(Widget *ww, int val, int flags);
	void (*cleanup)(Widget *ww);
	bool skip_child_handle;
	bool skip_child_render;
	int action_button;

	int num_child;
	Widget **child;
	Widget *parent;
	Widget **self_ptr;
	// *self_ptr is set to NULL when closed
	// so that multiple instances can be prevented

	u32 dialog_color; // index or color
	//u32 base_color;

	int value; // cbox radio listbox page textbox
	int val_max; // list textbox
	int val_min; // radio
	Widget *val_link; // circular list of widget sharing same value
	void *val_ptr; // optional external value source read by update
	
	bool opt; // cbox(show name) num(base)

	Widget *link_first;
	Widget *link_next;
	int link_index;

	int list_num;
	char **list_values;

	void *data;
};

typedef struct wgui_Option
{
	Widget *base;
	Widget *name;
	Widget *value;
	Widget *control;
} wgui_Option;

#include "wgui_f.h"

#endif

