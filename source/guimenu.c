
#include <stdio.h>
#include <string.h>
#include <asndlib.h>
#include <ogc/lwp_threads.h>
#include <wctype.h>
#include <wchar.h>

#include "wgui.h"
#include "gui.h"
#include "guimenu.h"

#include "menu.h"
#include "sort.h"
#include "coverflow.h"
#include "wbfs.h"
#include "cache.h"
#include "console.h"
#include "grid.h"
#include "xml.h"
#include "dolmenu.h"
#include "gettext.h"
#include "net.h"
#include "sys.h"
#include "util.h"
#include "music.h"
#include "gc_wav.h"
#include "cfg.h"
#include "dol.h"
#include "gc.h"
#include "savegame.h"
#include "channel.h"

typedef void (*entrypoint) (void);

#if 1

extern int page_gi;		//used by jump
extern int page_covers; //juse by jump
/*
Q:

use "Back" or "Close" button?

when opening main menu and selecting an option,
should back return to main menu?

tabbed settings instead of separate windows?
(view, style, system, online)

separate info window with: about, loader info, ios info, debug ?

*/

/*

Used buttons:

by wgui.c:
A : confirm, select, hold
B : cancel, close, back
UP/DOWN : page up / down (text or anywhere a page switcher is used)

by guimenu.c:
LEFT/RIGHT : prev / next game in game selection
HOME : Home (Quit) Menu

*/

// desktop dialog stack
// XXX use Widget*
Widget wgui_desk;
Widget *d_custom;
Widget *d_top;
Widget *d_top5;		//used by jump
Widget *d_bottom;

void pos_desk(Pos *p)
{
	if (p->x == POS_AUTO) p->x = POS_CENTER;
	if (p->y == POS_AUTO) p->y = POS_CENTER;
	if (p->w == SIZE_AUTO) p->w = -PAD3;
	if (p->h == SIZE_AUTO) p->h = -PAD3;
}

Widget *desk_open_dialog(Pos p, char *name)
{
	pos_desk(&p);
	pos_init(&wgui_desk);
	Widget *dd = wgui_add_dialog(&wgui_desk, p, name);
	return dd;
}

Widget *desk_open_singular(Pos p, char *name, Widget **self_ptr)
{
	if (*self_ptr) return NULL;
	Widget *dd = desk_open_dialog(p, name);
	dd->self_ptr = self_ptr;
	*self_ptr = dd;
	return dd;
}


//////// 


void wgui_test()
{
	Widget dialog;
	int buttons;
	ir_t ir;
	Widget *btn_back;
	Widget *ret;
	Widget *ww;

	wgui_dialog_init(&dialog, pos(20, 20, 600, 440), "Title");
	pos_prefsize(&dialog, 0, 58);
	ww = wgui_add_checkbox(&dialog, pos_xy(400, 100), "Favorite", true);
	ww = wgui_add_button(&dialog, pos_xy(400, 180), "Options");
	ww = wgui_add_button(&dialog, pos_xy(400, 260), "Ab日本語cd");
	btn_back = wgui_add_button(&dialog, pos_xy(100, 350), "Back");
	ww = wgui_add_button(&dialog, pos_xy(300, 350), "Start");
	ww = wgui_add_button(&dialog, pos(50, 100, 64, 64), "+");
	ww = wgui_add_button(&dialog, pos(50, 200, 40, 64), "-");

	//Widget *radio;
	ww = wgui_add_radio(&dialog, NULL, pos(150, 100, 100, 64), "Radio1");
	ww = wgui_add_radio(&dialog, ww, pos(150, 190, 100, 64), "Radio2");
	ww = wgui_add_radio(&dialog, ww, pos(150, 280, 100, 64), "Radio3");

	do {
		buttons = Wpad_GetButtons();
		Wpad_getIR(&ir);
		wgui_input_set(&ir, &buttons, NULL);
		ret = wgui_handle(&dialog);
		GX_SetZMode (GX_FALSE, GX_NEVER, GX_TRUE);
		wgui_render(&dialog);
		wgui_input_restore(&ir, &buttons);
		Gui_draw_pointer(&ir);
		Gui_Render();
		if (buttons & WPAD_BUTTON_B) break;
	} while (ret != btn_back);

	wgui_close(&dialog);
}

void Switch_WGui_To_Console()
{
	Switch_Gui_To_Console(false);
}

void Switch_Console_To_WGui()
{
	Switch_Console_To_Gui();
	wgui_input_get();
	// clear buttons
	wgui_input_steal_buttons();
	wgui_input_save();
}

void banner_end(bool mute);

void action_Reboot(Widget *_ww)
{
	banner_end(true);
	Switch_Gui_To_Console(true);
	prep_exit();
	Sys_LoadMenu();
}

void action_HBC(Widget *_ww)
{
	banner_end(true);
	Switch_Gui_To_Console(true);
	dbg_printf("Sys_HBC\n");
	Sys_HBC();
}

void action_Exit(Widget *_ww)
{
	banner_end(true);
	Switch_Gui_To_Console(true);
	Sys_Exit();
}

void action_priiloader(Widget *_ww)
{
	if (CFG.disable_options == 1) return;
	*(vu32*)0x8132FFFB = 0x4461636F;
DCFlushRange((void*)0x8132FFFB,4);
SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0); 
}

void action_Shutdown(Widget *_ww)
{
	banner_end(true);
	Switch_Gui_To_Console(true);
	Sys_Shutdown();
}

// home = [reboot], exit, hbc, screenshot, priiloader, wii_menu, 
//        <magic word>, <channel ID>

Widget *w_Quit = NULL;

int handle_Quit(Widget *ww)
{
	if (winput.w_buttons & WPAD_BUTTON_HOME) {
		//if (CFG.home == CFG_HOME_SCRSHOT) CFG.home = CFG_HOME_REBOOT;
		banner_end(true);
		Switch_Gui_To_Console(true);
		Handle_Home();
		exit(0);
	}
	handle_B_close(ww);
	return 0;
}

void action_OpenQuit(Widget *parent)
{
	Widget *dd;
	Widget *ww;

	/*
	if (CFG.home == CFG_HOME_SCRSHOT) {
		char fn[200];
		extern bool Save_ScreenShot(char *fn, int size);
		Save_ScreenShot(fn, sizeof(fn));
		return;
	}
	*/

	if (w_Quit) return;
	parent = wgui_primary_parent(parent);
	if (!parent) parent = &wgui_desk;
	Pos p = pos(50, 50, 640-80, 480-80);
	w_Quit = dd = wgui_add_dialog(parent, p, gt("Home Menu"));
	dd->self_ptr = &w_Quit;
	dd->handle = handle_Quit;
	dd->ax = 50;
	dd->ay = 50;
	dd->dialog_color = GUI_COLOR_POPUP;

	pos_rows(dd, 7, SIZE_FULL);
	p = pos_auto;
	p.x = POS_CENTER;
	//p.w = dd->w * 2 / 3;
	p.w = dd->w * 46 / 100;		//any bigger messes up ES translation

	if (CFG.home == CFG_HOME_REBOOT) p.h = H_LARGE; else p.h = H_NORMAL;
	ww = wgui_add_button(dd, p, gt("Reboot"));
	ww->action = action_Reboot;
	ww->action2 = action_close_parent_dialog;
	if (p.h == H_LARGE) wgui_add_text(dd, pos_h(H_LARGE), gt("[HOME]"));
	pos_newline(dd);

	if (CFG.home == CFG_HOME_HBC) p.h = H_LARGE; else p.h = H_NORMAL;
	ww = wgui_add_button(dd, p, gt("Homebrew Channel"));
	ww->action = action_HBC;
	ww->action2 = action_close_parent_dialog;
	wgui_set_inactive(ww, CFG.disable_options);
	if (p.h == H_LARGE) wgui_add_text(dd, pos_h(H_LARGE), gt("[HOME]"));
	pos_newline(dd);

	if (CFG.home == CFG_HOME_EXIT) p.h = H_LARGE; else p.h = H_NORMAL;
	ww = wgui_add_button(dd, p, gt("Exit"));
	ww->action = action_Exit;
	ww->action2 = action_close_parent_dialog;
	if (p.h == H_LARGE) wgui_add_text(dd, pos_h(H_LARGE), gt("[HOME]"));
	pos_newline(dd);
	
	if (CFG.home == CFG_HOME_CHANNEL) p.h = H_LARGE; else p.h = H_NORMAL;
	ww = wgui_add_button(dd, p, gt("Priiloader"));
	ww->action = action_priiloader;
	ww->action2 = action_close_parent_dialog;
	wgui_set_inactive(ww, CFG.disable_options);
	if (p.h == H_LARGE) wgui_add_text(dd, pos_h(H_LARGE), gt("[HOME]"));
	pos_newline(dd);

	p.h = H_NORMAL;
	ww = wgui_add_button(dd, p, gt("Shutdown"));
	ww->action = action_Shutdown;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);

	// screenshot
	// priiloader 
	// priiloader wii sys menu

	p.w = dd->w * 46 / 100;
	p.y = POS_EDGE;
	ww = wgui_add_button(dd, p, gt("Back"));
	ww->action = action_close_parent_dialog;
	pos_newline(dd);
	
	//battery level
	static char battery_str[50];
	sprintf(battery_str, gt("P1 %3u%% | P2 %3u%% | P3 %3u%% | P4 %3u%%"), WPAD_BatteryLevel(0), WPAD_BatteryLevel(1), WPAD_BatteryLevel(2), WPAD_BatteryLevel(3)); 
	wgui_add_text(dd, pos(POS_CENTER, 322,	SIZE_AUTO, H_LARGE), battery_str);
}

/*
// text scale tests
void test_text_scale()
{
	x = ww->ax + PAD1;
	y = ww->ay + PAD1;
	char *str = "aaabcdABCD1234";
	float scale = 1.0;
	int i;
	for (i=0; i<10; i++) {
		scale = 1.0 + (float)(i/2) / 10.0;
		if (i%2==0) scale = text_round_scale_w(scale, -1);
		GRRLIB_Printf(x, y, &tx_font, 0xFFFFFFFF, scale, "%.4f", scale);
		GRRLIB_Print3(x+100, y, &tx_font, 0xFFFFFFFF, 0xFF, 0, scale, str);
		y += 30;
	}
	x = ww->ax;
	y = ww->ay;
	char *str = "aaaaaAAAAA22222WWWWW";
	static int f = 0;
	int i;
	f++;
	float scale = 1.2;
	if ((f/60)%3==0) scale = text_round_scale_w(scale, -1);
	if ((f/60)%3==2) scale = text_round_scale_w(scale, 1);
	GRRLIB_Rectangle(x-10, y-10, 400, 350, 0xFF, 1);
	for (i=0; i<10; i++) {
		GRRLIB_Printf(x, y, &tx_font, 0xFFFFFFFF, scale, "%.4f", scale);
		GRRLIB_Print3(x+100, y, &tx_font, 0xFFFFFFFF, 0, 0, scale, str);
		y += 30;
	}
}
*/




// ########################################################
//
// Game Options
//
// ########################################################





struct AltDolInfo
{
	int type;
	char name[64];
	char dol[64];
};

#define ALT_DOL_LIST_NUM 64

struct AltDolInfoList
{
	int num;
	struct AltDolInfo info[ALT_DOL_LIST_NUM];
	char *name[ALT_DOL_LIST_NUM];
};

int get_alt_dol_list(struct discHdr *header, struct AltDolInfoList *list)
{
	struct AltDolInfo *info;
	int i;
	DOL_LIST dol_list;

	WBFS_GetDolList(header->id, &dol_list);
	load_dolmenu((char*)header->id);
	int dm_num = dolmenubuffer ? dolmenubuffer[0].count : 0;
	memset(list, 0, sizeof(*list));

	info = &list->info[0];
	info->type = ALT_DOL_OFF;
	STRCOPY(info->name, gt("Off"));
	list->num++;

	info++;
	info->type = ALT_DOL_SD;
	STRCOPY(info->name, "SD");
	list->num++;

	info++;
	info->type = ALT_DOL_DISC;
	STRCOPY(info->name, gt("Disc (Ask)"));
	list->num++;

	for (i=0; i < dol_list.num; i++) {
		if (list->num >= ALT_DOL_LIST_NUM) break;
		info++;
		info->type = ALT_DOL_DISC;
		STRCOPY(info->name, dol_list.name[i]);
		STRCOPY(info->dol, dol_list.name[i]);
		list->num++;
	}

	for (i=0; i < dm_num; i++) {
		if (list->num >= ALT_DOL_LIST_NUM) break;
		info++;
		info->type = ALT_DOL_PLUS + i;
		STRCOPY(info->name, dolmenubuffer[i+1].name);
		STRCOPY(info->dol, dolmenubuffer[i+1].dolname);
		list->num++;
	}

	for (i=0; i < list->num; i++) {
		list->name[i] = list->info[i].name;
	}

	return list->num;
}

int get_alt_dol_list_index(struct AltDolInfoList *list, struct Game_CFG *gcfg)
{
	int i = 0;
	int alt_dol = gcfg->alt_dol;
	char *dol_name = gcfg->dol_name;
	if (alt_dol < ALT_DOL_DISC) return alt_dol;
	if (alt_dol == ALT_DOL_DISC) {
		if (!*dol_name) return alt_dol;
		for (i=3; i<list->num; i++) {
			if (list->info[i].type == ALT_DOL_DISC) {
				if (strcmp(list->info[i].dol, dol_name) == 0) return i;
			}
		}
	} else { // >= ALT_DOL_PLUS
		for (i=2; i<list->num; i++) {
			if (list->info[i].type == alt_dol) return i;
		}
	}
	return 0; // not found;
}

struct W_GameCfg
{
	Widget *dialog;
	Widget *start;
	Widget *cover;
	Widget *cstyle;
	Widget *cctrl; // cover control
	Widget *cc_nohq;
	Widget *info;
	Widget *options;
	Widget *discard;
	Widget *save;
	struct discHdr *header;
	struct Game_CFG_2 *gcfg2;
	struct Game_CFG *gcfg;
	struct AltDolInfoList dol_info_list;
	Widget *favorite;
	Widget *hide;
	Widget *language;
	Widget *video;
	Widget *video_patch;
	Widget *vidtv;
	Widget *country_patch;
	Widget *ios_idx;
	Widget *block_ios_reload;
	Widget *fix_002;
	Widget *wide_screen;
	Widget *ntsc_j_patch;
	Widget *nodisc;
	Widget *screenshot;
	Widget *alt_dol;
	Widget *ocarina;
	Widget *hooktype;
	Widget *write_playlog;
	Widget *clean;
	Widget *nand_emu;
	Widget *channel_boot;
	Widget *alt_controller_cfg;
} wgame;


// cover style names
char *cstyle_names[] =
{
	"2D", "3D",
	gts("Disc"),
	gts("Full"),
	"HQ",
	"RGB",
	"Full RGB", "Full CMPR", "HQ CMPR"
};

int cstyle_val[] =
{
	CFG_COVER_STYLE_2D,
	CFG_COVER_STYLE_3D,
	CFG_COVER_STYLE_DISC,
	CFG_COVER_STYLE_FULL,
	//CFG_COVER_STYLE_HQ,
	CFG_COVER_STYLE_HQ_OR_FULL,
	CFG_COVER_STYLE_HQ_OR_FULL_RGB,
	// debug:
	CFG_COVER_STYLE_FULL_RGB,
	CFG_COVER_STYLE_FULL_CMPR,
	CFG_COVER_STYLE_HQ_CMPR,
};

int cstyle_num = 6;
int cstyle_num_debug = 9;

static int cover_hold = 0;
static float hold_x, hold_y; // base
static float hold_ax, hold_ay; // abs. delta vs base
static float hold_px, hold_py; // prev
static float hold_dx, hold_dy; // delta vs prev
static float hold_sx = 0.5, hold_sy; // average speed of last 10 frames
static float cover_z = 0;
static float cover_zd = 0; // dest
static bool over_cover = false;

int handle_game_start(Widget *ww)
{
	int ret = wgui_handle_button(ww);
	if (over_cover && ww->state == WGUI_STATE_NORMAL) {
		ww->state = WGUI_STATE_HOVER;
	}
	return ret;
}

int handle_game_cover(Widget *ww)
{
	wgame.cc_nohq->state = WGUI_STATE_DISABLED;
	int cstyle = cstyle_val[wgame.cstyle->value]; // remap
	if (cstyle > CFG_COVER_STYLE_FULL) {
		int cs = cstyle;
		int fmt = CC_COVERFLOW_FMT;
		int state = -1;
		cache_remap_style_fmt(&cs, &fmt);
		if (cs == CFG_COVER_STYLE_HQ || cs == CFG_COVER_STYLE_HQ_OR_FULL) {
			// if HQ selected then check missing
			cache_request_cover(gameSelected, CFG_COVER_STYLE_HQ, fmt, CC_PRIO_NONE, &state);
			if (state == CS_MISSING) {
				// show missing note
				wgame.cc_nohq->state = WGUI_STATE_NORMAL;
			}
		}
	}

	over_cover = false;
	if (wgui_over(ww)) {
		over_cover = true;
		if (winput.w_buttons & WPAD_BUTTON_A) {
			return WGUI_STATE_PRESS;
		}
	}

	if (cstyle < CFG_COVER_STYLE_FULL) return 0;

	int hold_buttons = Wpad_HeldButtons();

	if (winput.w_buttons & WPAD_BUTTON_1) {
		if (wgui_over(ww)) {
			cover_hold = 1;
			hold_x = winput.w_ir->sx;
			hold_y = winput.w_ir->sy;
			hold_px = hold_x;
			hold_py = hold_y;
			hold_sx = 0;
			hold_sy = 0;
		}
	}
	if (cover_hold) {
		if (!(hold_buttons & WPAD_BUTTON_1)) {
			// released, keep rotating
			if (fabsf(hold_sx) < 0.5) hold_sx = 0;
			if (fabsf(hold_sy) < 0.5) hold_sy = 0;
			// allow to rotate only on one axis
			if (fabsf(hold_sx) > fabsf(hold_sy)) hold_sy = 0; else hold_sx = 0;
			cover_hold = 0;
		}
	}
	if (cover_hold && winput.w_ir->smooth_valid) {
		hold_ax = winput.w_ir->sx - hold_x;
		hold_ay = winput.w_ir->sy - hold_y;
		hold_dx = winput.w_ir->sx - hold_px;
		hold_dy = winput.w_ir->sy - hold_py;
		hold_px = winput.w_ir->sx;
		hold_py = winput.w_ir->sy;
		hold_sx = (hold_sx * 9.0 + hold_dx) / 10.0;
		hold_sy = (hold_sy * 9.0 + hold_dy) / 10.0;
	}
	if (winput.w_buttons & WPAD_BUTTON_PLUS) {
		cover_zd += 5;
	}
	if (winput.w_buttons & WPAD_BUTTON_MINUS) {
		cover_zd -= 5;
	}
	float zd = 0.0;
	if (cover_z < cover_zd) zd = +0.2;
	if (cover_z > cover_zd) zd = -0.2;
	if (cover_z == cover_zd) {
		if (hold_buttons & WPAD_BUTTON_PLUS) zd = +0.2;
		if (hold_buttons & WPAD_BUTTON_MINUS) zd = -0.2;
		cover_zd += zd;
		cover_z = cover_zd;
	}
	if (fabsf(cover_z - cover_zd) > fabsf(zd)) {
		cover_z += zd;
	} else {
		cover_z = cover_zd;
	}

	return 0;
}

void render_game_cover(Widget *ww)
{
	int cstyle = cstyle_val[wgame.cstyle->value]; // remap
	//wgui_render_page(ww);
	float x, y;
	if (cstyle < CFG_COVER_STYLE_FULL) {
		// x,y: center
		x = ww->ax + ww->w / 2;
		y = ww->ay + ww->h / 2;
		draw_grid_cover_at(gameSelected, x, y, ww->w, ww->h, cstyle);
	} else {
		static float yrot = 0;
		static float xrot = 0;
		float z = cover_z;
		if (cover_hold) {
			z = cover_z + 5;
			yrot += hold_dx;
			xrot += hold_dy / 2.0;
		} else {
			yrot += hold_sx / 2.0;
			xrot += hold_sy / 2.0;
			if (yrot > 360) yrot -= 360;
			if (yrot < -360) yrot += 360;
			if (xrot > 360) xrot -= 360;
			if (xrot < -360) xrot += 360;
		}
		x = ww->ax + PAD3 * 3;
		y = ww->ay + PAD3 * 2;
		if (z > 0) x += z * 3.0;
		Coverflow_drawCoverForGameOptions(gameSelected,
				x, y, z, xrot, yrot, 0, cstyle);
	}
}

void init_cover_page(Widget *pp)
{
	Widget *ww;
	Widget *cc;
	pos_margin(pp, PAD00);
	// cover style
	int ns = cstyle_num;
	if (CFG.debug >= 3) ns = cstyle_num_debug;
	char *trans_names[ns];
	translate_array(ns, cstyle_names, trans_names);
	ww = wgui_add_listboxx(pp, pos(POS_AUTO, POS_EDGE, SIZE_AUTO, H_SMALL),
			gt("Cover Style"), ns, trans_names);
	ww->child[0]->action_button = WPAD_BUTTON_UP;
	ww->child[2]->action_button = WPAD_BUTTON_DOWN;
	wgui_set_value(ww, CFG.cover_style);
	wgame.cstyle = ww;
	ww = wgui_add_text(pp, pos_h(H_SMALL), gt("[No HQ]"));
	wgame.cc_nohq = ww;
	// cover image
	cc = wgui_add(pp, pos(0, 0, SIZE_FULL, -H_SMALL), NULL);
	cc->type = WGUI_TYPE_USER;
	cc->render = render_game_cover;
	cc->handle = handle_game_cover;
	wgame.cover = cc;
	// debug / test
	if (CFG.debug) {
		// fast aa
		ww = wgui_add_checkbox(cc, pos_h(H_SMALL), "fast ww", true);
		ww->val_ptr = &aa_method;
		ww->update = update_val_from_ptr_int;
		ww->action = action_write_val_ptr_int;
		pos_newline(cc);
		// ww level
		ww = wgui_add_numswitch(cc, pos_h(H_SMALL), NULL, 0, 4);
		ww->val_ptr = &CFG.gui_antialias;
		ww->update = update_val_from_ptr_int;
		ww->action = action_write_val_ptr_int;
		wgui_propagate_value(ww, SET_VAL_MAX, 4); 
		ww->child[0]->action_button = 0;
		ww->child[2]->action_button = 0;
		pos_newline(cc);
		// wide ww
		extern bool grrlib_wide_aa;
		ww = wgui_add_checkbox(cc, pos_h(H_SMALL), "wide ww", true);
		ww->val_ptr = &grrlib_wide_aa;
		ww->update = update_val_from_ptr_bool;
		ww->action = action_write_val_ptr_bool;
		pos_newline(cc);
		// lod bias
		extern int lod_bias_idx;
		char *lod_bias_v[] = { "-2.0", "-1.5", "-1.0", "-0.5", "-0.3", "0", "+0.5", "+1" };
		ww = wgui_add_listboxx(cc, pos_h(H_SMALL), "lod bias", 8, lod_bias_v);
		ww->val_ptr = &lod_bias_idx;
		ww->update = update_val_from_ptr_int;
		ww->action = action_write_val_ptr_int;
		pos_newline(cc);
		// grid
		ww = wgui_add_checkbox(cc, pos_h(H_SMALL), "grid", true);
		ww->val_ptr = &coverflow_test_grid;
		ww->update = update_val_from_ptr_bool;
		ww->action = action_write_val_ptr_bool;
	}
	// container for cover controls
	/*
	Widget *aa;
	Pos p;
	aa = wgui_add(cc, pos(0, POS_EDGE, (H_SMALL+4)*2, (H_SMALL+4)*4), NULL);
	pos_pad(aa, 4);
	wgame.cctrl = aa;
	p = pos_wh(H_SMALL, H_SMALL);
	ww = wgui_add_button(aa, p, "☻"); // F
	ww->text_scale *= 1.5;
	ww = wgui_add_button(aa, p, "↔"); // B =#↔↕�♦☺○o•☼ //
	ww->text_scale *= 1.6;
	pos_newline(aa);
	ww = wgui_add_button(aa, p, "+");
	ww = wgui_add_button(aa, p, "-");
	pos_newline(aa);
	ww = wgui_add_button(aa, p, "▲"); // ^
	ww = wgui_add_button(aa, p, "▼"); // v
	pos_newline(aa);
	*/
}

void gameopt_inactive(int cond, Widget *ww, int val)
{
	traverse(ww, wgui_set_inactive, cond);
	if (cond) wgui_set_value(ww, val);
}

void update_gameopt_state()
{
	int cond;
    struct discHdr *header = wgame.header;
	if (header->magic == GC_GAME_ON_DRIVE)
	{
		wgui_update(wgame.clean);
		cond = (wgame.clean->value == CFG_CLEAN_ALL);
		gameopt_inactive(cond, wgame.language, CFG_LANG_CONSOLE);
		gameopt_inactive(cond, wgame.video, CFG_VIDEO_GAME);
		gameopt_inactive(cond, wgame.vidtv, 0);
		gameopt_inactive(cond, wgame.country_patch, 0);
		gameopt_inactive(cond, wgame.channel_boot, 0);
		gameopt_inactive(cond, wgame.ocarina, 0);
		gameopt_inactive(cond, wgame.wide_screen, 0);		
		gameopt_inactive(cond, wgame.nodisc, 0);
		gameopt_inactive(cond, wgame.screenshot, 0);
		gameopt_inactive(cond, wgame.hooktype, 1);
		gameopt_inactive(cond, wgame.ntsc_j_patch, 0);
	}
	else if (header->magic == CHANNEL_MAGIC)
	{
		// clear
		wgui_update(wgame.clean);
		cond = (wgame.clean->value == CFG_CLEAN_ALL);
		gameopt_inactive(cond, wgame.language, CFG_LANG_CONSOLE);
		gameopt_inactive(cond, wgame.video, CFG_VIDEO_GAME);
		gameopt_inactive(cond, wgame.video_patch, CFG_VIDEO_PATCH_OFF);	
		gameopt_inactive(cond, wgame.vidtv, 0);
		gameopt_inactive(cond, wgame.country_patch, 0);
		gameopt_inactive(cond, wgame.fix_002, 0);
		gameopt_inactive(cond, wgame.ocarina, 0);
		gameopt_inactive(cond, wgame.wide_screen, 0);
				
		// ocarina hook type
		wgui_update(wgame.ocarina);
		cond = (cond || (!wgame.ocarina->value && !CFG.wiird));
		gameopt_inactive(cond, wgame.hooktype, 0);
	}
	else
	{
		// clear
		wgui_update(wgame.clean);
		cond = (wgame.clean->value == CFG_CLEAN_ALL);
		gameopt_inactive(cond, wgame.language, CFG_LANG_CONSOLE);
		gameopt_inactive(cond, wgame.video, CFG_VIDEO_GAME);
		gameopt_inactive(cond, wgame.video_patch, CFG_VIDEO_PATCH_OFF);	
		gameopt_inactive(cond, wgame.vidtv, 0);
		gameopt_inactive(cond, wgame.country_patch, 0);
		gameopt_inactive(cond, wgame.fix_002, 0);
		gameopt_inactive(cond, wgame.ocarina, 0);
		gameopt_inactive(cond, wgame.wide_screen, 0);
		gameopt_inactive(cond, wgame.nand_emu, 0);
				
		// ocarina hook type
		wgui_update(wgame.ocarina);
		cond = (cond || (!wgame.ocarina->value && !CFG.wiird));
		gameopt_inactive(cond, wgame.hooktype, 0);
	}
	// update values from val_ptr
	traverse1(wgame.dialog, wgui_update);

	// game opt save state:
	//
	// [default] <-reset [changed] save-> [saved] <-revert [changed]
	//           <------------------------discard
	// [default] /save/
	// (reset)   (save)
	// (discard) [saved]
	// (revert)  (save)

	bool changed = CFG_is_changed(wgame.header->id);
	int saved = wgame.gcfg2->is_saved;
	wgame.save->name = gt("save");
	wgame.save->render = wgui_render_button;
	wgame.save->text_opt = WGUI_TEXT_SCALE_FIT_BUTTON;
	wgame.discard->render = wgui_render_button;
	wgame.discard->text_opt = WGUI_TEXT_SCALE_FIT_BUTTON;
	if (!changed && !saved) {
		wgame.discard->name = gt("[default]");
		wgame.discard->render = wgui_render_text;
		wgame.discard->text_opt = WGUI_TEXT_SCALE_FIT;
		wgui_set_inactive(wgame.discard, true);
		wgui_set_inactive(wgame.save, true);
	} else if (changed && !saved) {
		wgame.discard->name = gt("reset");
		wgui_set_inactive(wgame.discard, false);
		wgui_set_inactive(wgame.save, false);
	} else if (!changed && saved) {
		wgame.discard->name = gt("discard");
		wgame.save->name = gt("[saved]");
		wgame.save->render = wgui_render_text;
		wgame.save->text_opt = WGUI_TEXT_SCALE_FIT;
		wgui_set_inactive(wgame.discard, false);
		wgui_set_inactive(wgame.save, true);
	} else { // changed && saved
		wgame.discard->name = gt("revert");
		wgui_set_inactive(wgame.discard, false);
		wgui_set_inactive(wgame.save, false);
	}
}


void action_save_gamecfg(Widget *ww)
{
	//int ret;
	CFG_save_game_opt(wgame.header->id);
	// XXX on error open info dialog
	// if (!ret) printf(gt("Error saving options!"));
	update_gameopt_state();
}

void action_reset_gamecfg(Widget *ww)
{
	bool changed = CFG_is_changed(wgame.header->id);
	int saved = wgame.gcfg2->is_saved;
	if (changed && saved) {
		// copy saved over current
		memcpy(&wgame.gcfg2->curr, &wgame.gcfg2->save, sizeof(wgame.gcfg2->curr));
	} else {
		//int ret;
		CFG_discard_game_opt(wgame.header->id);
		// XXX on error open info dialog
		//if (!ret) printf(gt("Error discarding options!")); 
	}
	if (wgame.header->magic != GC_GAME_ON_DRIVE)
	{
		if (wgame.alt_dol->update == NULL)
		{
			// if update == NULL, alt_dol and dol_info_list was initialized
			wgui_set_value(wgame.alt_dol, get_alt_dol_list_index(&wgame.dol_info_list, wgame.gcfg));
		}
	}
	traverse1(ww->parent, wgui_update);
	update_gameopt_state();
}

void action_game_opt_change(Widget *ww)
{
	action_write_val_ptr_int(ww);
	update_gameopt_state();
}

void action_gameopt_set_alt_dol(Widget *ww)
{
	int i = ww->value;
	if (i >= 0) {
		wgame.gcfg->alt_dol = wgame.dol_info_list.info[i].type;
		STRCOPY(wgame.gcfg->dol_name, wgame.dol_info_list.info[i].dol);
	}
	update_gameopt_state();
}

Widget* wgui_add_game_opt(Widget *parent, char *name, int num, char **values)
{
	Pos p = pos_auto;
	p.w = SIZE_FULL;
	char **val = values;
	char *tr_values[num];
	if (values) {
		translate_array(num, values, tr_values);
		val = tr_values;
	}
	wgui_Option opt = wgui_add_option(parent, p, 2, 0.45, name, num, val);
	pos_newline(parent);
	Widget *ww = opt.control;
	ww->action = action_game_opt_change;
	return ww;
}

void action_game_favorite(Widget *ww)
{
	if (!set_favorite(wgame.header->id, ww->value)) {
		// XXX pop-up
		//printf(gt("ERROR"));
	}
}

void action_game_hide(Widget *ww)
{
	if (!set_hide_game(wgame.header->id, ww->value)) {
		// XXX pop-up
		//printf(gt("ERROR"));
	}
}

#define BIND_OPT(NAME) ww->val_ptr = &wgame.gcfg->NAME; wgame.NAME = ww;

void init_alt_dol_if_parent_enabled(Widget *ww)
{
	// init alt dol list on demand
	// only when page 2 of game options is visible
	// because it takes some time to scan for alt dols
	Widget *parent = ww->parent->parent;
	// 2x parent because first parent is the option container
	// next parent is page
	if (parent->state != WGUI_STATE_DISABLED) {
		BIND_OPT(alt_dol);
		ww->val_ptr = NULL;
		ww->update = NULL; // remove call to this function
		ww->action = action_gameopt_set_alt_dol;
		get_alt_dol_list(wgame.header, &wgame.dol_info_list);
		wgui_listbox_set_values(ww, wgame.dol_info_list.num, wgame.dol_info_list.name);
		wgui_set_value(ww, get_alt_dol_list_index(&wgame.dol_info_list, wgame.gcfg));
	}
}

void InitGameOptionsPage(Widget *pp, int bh)
{
	struct discHdr *header = wgame.header;
	Widget *ww;
	Widget *op, *w_opt_page = NULL;

	pos_margin(pp, 0);
	pos_move_to(pp, 0, 0);
	
	w_opt_page = op = wgui_add_page(pp, w_opt_page, pos_wh(SIZE_FULL, -bh), "opt");
	op->render = NULL;
	if (!CFG.disable_options) {
		pos_pad(op, PAD0);
		pos_margin(op, PAD0);
		//if (memcmp("G",((char *)header->id),1)==0)
		//pos_rows(op, 8, SIZE_AUTO);
		//else
		pos_rows(op, 7, SIZE_AUTO);	

		wgame.gcfg2 = CFG_get_game(header->id);
		wgame.gcfg = NULL;
		if (!wgame.gcfg2) {
			wgui_add_text(op, pos_auto, gt("ERROR game opt"));
		} else if (header->magic == GC_GAME_ON_DRIVE) {
			wgame.gcfg = &wgame.gcfg2->curr;

			//int num_ios = map_get_num(map_ios);
			//char *names_ios[num_ios];
			//num_ios = map_to_list(map_ios, num_ios, names_ios);

			int num_gc_boot = map_get_num(map_gc_boot);
			char *names_gc_boot[num_gc_boot];
			num_gc_boot = map_to_list(map_gc_boot, num_gc_boot, names_gc_boot);

			ww = wgui_add_game_opt(op, gt("Language:"), CFG_LANG_NUM, languages);
			BIND_OPT(language);

			ww = wgui_add_game_opt(op, gt("Video:"), 6, DML_videos);
			BIND_OPT(video);
			
			char *str_block[3] = { gt("Off"), gt("On"), gt("Auto") };
			ww = wgui_add_game_opt(op, gt("Video Patch:"), 3, str_block);
			BIND_OPT(block_ios_reload);
			
			ww = wgui_add_game_opt(op, gt("NMM:"), 2, NULL);
			BIND_OPT(country_patch);
			     
     		ww = wgui_add_game_opt(op, gt("Wide Screen:"), 2, NULL);
			BIND_OPT(wide_screen);

			ww = wgui_add_game_opt(op, gt("Ocarina (cheats):"), 2, NULL);
			BIND_OPT(ocarina);
					
			ww = wgui_add_game_opt(op, gt("Boot Method:"), num_gc_boot, names_gc_boot);
			BIND_OPT(channel_boot);

			/////////////////
					
			op = wgui_add_page(pp, w_opt_page, pos_wh(SIZE_FULL, -bh), "opt");
			op->render = NULL;
			
			ww = wgui_add_game_opt(op, gt("PAD HOOK"), 2, NULL);
			BIND_OPT(hooktype);
			
     		ww = wgui_add_game_opt(op, gt("NoDisc:"), 2, NULL);
			BIND_OPT(nodisc);		
			      		
			ww = wgui_add_game_opt(op, gt("LED:"), 2, NULL);
			BIND_OPT(vidtv);
			
			ww = wgui_add_game_opt(op, gt("Clear Patches:"), 3, names_vpatch);
			BIND_OPT(clean);
			
			ww = wgui_add_game_opt(op, gt("NTSC-J patch:"), 2, NULL);
			BIND_OPT(ntsc_j_patch);
			
			ww = wgui_add_game_opt(op, gt("Screenshot:"), 2, NULL);
			BIND_OPT(screenshot);
			
			ww = wgui_add_game_opt(op, gt("Alt Button Cfg:"), 2, NULL);
			BIND_OPT(alt_controller_cfg);
			
			pos_move_to(pp, PAD0, -bh);
			pos_pad(pp, PAD0);
			pos_columns(pp, 4, SIZE_FULL);
			Pos p = pos_auto;
			p.h = bh;
			ww = wgui_add_button(pp, p, gt("reset"));
			ww->action = action_reset_gamecfg;
			wgame.discard = ww;

			ww = wgui_add_button(pp, p, gt("save"));
			ww->action = action_save_gamecfg;
			wgame.save = ww;

			pos_columns(pp, 0, SIZE_FULL);
			p.w = -PAD0;
			ww = wgui_add_pgswitchx(pp, w_opt_page, p, NULL, 0, "%d/%d", 2);
			
			
			update_gameopt_state();
		} else if (header->magic == CHANNEL_MAGIC) {
			wgame.gcfg = &wgame.gcfg2->curr;

			int num_ios = map_get_num(map_ios);
			char *names_ios[num_ios];
			num_ios = map_to_list(map_ios, num_ios, names_ios);
			
			int num_channel_boot = map_get_num(map_channel_boot);
			char *names_channel_boot[num_channel_boot];
			num_channel_boot = map_to_list(map_channel_boot, num_channel_boot, names_channel_boot);

			ww = wgui_add_game_opt(op, gt("Language:"), CFG_LANG_NUM, languages);
			BIND_OPT(language);

			ww = wgui_add_game_opt(op, gt("Video:"), CFG_VIDEO_NUM, videos);
			BIND_OPT(video);

			ww = wgui_add_game_opt(op, gt("Video Patch:"), CFG_VIDEO_PATCH_NUM, names_vpatch);
			BIND_OPT(video_patch);

			ww = wgui_add_game_opt(op, "VIDTV:", 2, NULL);
			BIND_OPT(vidtv);
			
			ww = wgui_add_game_opt(op, gt("LED:"), 2, NULL);
			BIND_OPT(wide_screen);

			ww = wgui_add_game_opt(op, gt("Country Fix:"), 2, NULL);
			BIND_OPT(country_patch);

			ww = wgui_add_game_opt(op, "IOS:", num_ios, names_ios);
			BIND_OPT(ios_idx);

			/////////////////
			op = wgui_add_page(pp, w_opt_page, pos_wh(SIZE_FULL, -bh), "opt");
			op->render = NULL;
			
			char *str_block[3] = { gt("Off"), gt("On"), gt("Auto") };
			ww = wgui_add_game_opt(op, gt("Block IOS Reload:"), 3, str_block);
			BIND_OPT(block_ios_reload);

			ww = wgui_add_game_opt(op, gt("Alt dol:"), 0, NULL);
			BIND_OPT(alt_dol);
			ww->val_ptr = NULL;
			ww->update = init_alt_dol_if_parent_enabled;

			ww = wgui_add_game_opt(op, gt("Anti 002 Fix:"), 2, NULL);
			BIND_OPT(fix_002);

			ww = wgui_add_game_opt(op, gt("Ocarina (cheats):"), 2, NULL);
			BIND_OPT(ocarina);

			ww = wgui_add_game_opt(op, gt("Hook Type:"), NUM_HOOK, hook_name);
			BIND_OPT(hooktype);

			ww = wgui_add_game_opt(op, gt("Write Playlog:"), 4, playlog_name);
			BIND_OPT(write_playlog);

			ww = wgui_add_game_opt(op, gt("Clear Patches:"), 3, names_vpatch);
			BIND_OPT(clean);
			
			/////////////////
			op = wgui_add_page(pp, w_opt_page, pos_wh(SIZE_FULL, -bh), "opt");
			op->render = NULL;
			
			ww = wgui_add_game_opt(op, gt("Plugin:"), num_channel_boot, names_channel_boot);
			BIND_OPT(channel_boot);

			pos_move_to(pp, PAD0, -bh);
			pos_pad(pp, PAD0);
			pos_columns(pp, 4, SIZE_FULL);
			Pos p = pos_auto;
			p.h = bh;
			ww = wgui_add_button(pp, p, gt("reset"));
			ww->action = action_reset_gamecfg;
			wgame.discard = ww;

			ww = wgui_add_button(pp, p, gt("save"));
			ww->action = action_save_gamecfg;
			wgame.save = ww;

			pos_columns(pp, 0, SIZE_FULL);
			p.w = -PAD0;
			ww = wgui_add_pgswitchx(pp, w_opt_page, p, NULL, 0, "%d/%d", 2);

			update_gameopt_state();
		} else {
			wgame.gcfg = &wgame.gcfg2->curr;

			int num_ios = map_get_num(map_ios);
			char *names_ios[num_ios];
			num_ios = map_to_list(map_ios, num_ios, names_ios);
			
			ww = wgui_add_game_opt(op, gt("Language:"), CFG_LANG_NUM, languages);
			BIND_OPT(language);

			ww = wgui_add_game_opt(op, gt("Video:"), CFG_VIDEO_NUM, videos);
			BIND_OPT(video);

			ww = wgui_add_game_opt(op, gt("Video Patch:"), CFG_VIDEO_PATCH_NUM, names_vpatch);
			BIND_OPT(video_patch);

			ww = wgui_add_game_opt(op, "VIDTV:", 2, NULL);
			BIND_OPT(vidtv);
			
			ww = wgui_add_game_opt(op, gt("LED:"), 2, NULL);
			BIND_OPT(wide_screen);

			ww = wgui_add_game_opt(op, gt("Country Fix:"), 2, NULL);
			BIND_OPT(country_patch);

			ww = wgui_add_game_opt(op, "IOS:", num_ios, names_ios);
			BIND_OPT(ios_idx);

			/////////////////
			op = wgui_add_page(pp, w_opt_page, pos_wh(SIZE_FULL, -bh), "opt");
			op->render = NULL;
			
			char *str_block[3] = { gt("Off"), gt("On"), gt("Auto") };
			ww = wgui_add_game_opt(op, gt("Block IOS Reload:"), 3, str_block);
			BIND_OPT(block_ios_reload);

			ww = wgui_add_game_opt(op, gt("Alt dol:"), 0, NULL);
			BIND_OPT(alt_dol);
			ww->val_ptr = NULL;
			ww->update = init_alt_dol_if_parent_enabled;

			ww = wgui_add_game_opt(op, gt("Anti 002 Fix:"), 2, NULL);
			BIND_OPT(fix_002);

			ww = wgui_add_game_opt(op, gt("Ocarina (cheats):"), 2, NULL);
			BIND_OPT(ocarina);

			ww = wgui_add_game_opt(op, gt("Hook Type:"), NUM_HOOK, hook_name);
			BIND_OPT(hooktype);

			ww = wgui_add_game_opt(op, gt("Write Playlog:"), 4, playlog_name);
			BIND_OPT(write_playlog);

			ww = wgui_add_game_opt(op, gt("Clear Patches:"), 3, names_vpatch);
			BIND_OPT(clean);
			
			/////////////////
			op = wgui_add_page(pp, w_opt_page, pos_wh(SIZE_FULL, -bh), "opt");
			op->render = NULL;
			
			ww = wgui_add_game_opt(op, gt("NAND Emu:"), 3, str_nand_emu);
			BIND_OPT(nand_emu);

			pos_move_to(pp, PAD0, -bh);
			pos_pad(pp, PAD0);
			pos_columns(pp, 4, SIZE_FULL);
			Pos p = pos_auto;
			p.h = bh;
			ww = wgui_add_button(pp, p, gt("reset"));
			ww->action = action_reset_gamecfg;
			wgame.discard = ww;

			ww = wgui_add_button(pp, p, gt("save"));
			ww->action = action_save_gamecfg;
			wgame.save = ww;

			pos_columns(pp, 0, SIZE_FULL);
			p.w = -PAD0;
			ww = wgui_add_pgswitchx(pp, w_opt_page, p, NULL, 0, "%d/%d", 2);

			update_gameopt_state();
		}
	}
}

void action_game_missing_covers(Widget *ww)
{
	Switch_WGui_To_Console();
	//cache_wait_idle();
	Download_Cover((char*)wgame.header->id, true, true);
	Cache_Invalidate();
	Menu_PrintWait();
	Switch_Console_To_WGui();
}

void action_game_all_covers(Widget *ww)
{
	Switch_WGui_To_Console();
	//cache_wait_idle();
	Download_Cover((char*)wgame.header->id, false, true);
	Cache_Invalidate();
	Menu_PrintWait();
	Switch_Console_To_WGui();
}

void action_game_manage_cheats(Widget *ww)
{
	Switch_WGui_To_Console();
	Menu_Cheats(wgame.header);
	Switch_Console_To_WGui();
}

void action_game_dump_savegame(Widget *ww) 
{
	Switch_WGui_To_Console();
	Menu_dump_savegame(wgame.header);
	Switch_Console_To_WGui();
}

struct BannerInfo
{
	int inited;
	SoundInfo snd;
	u8 title[84];
	bool playing;
	bool parsed;
	lwp_t lwp;
	mutex_t mutex;
	cond_t cond;
	struct discHdr *header;
	volatile struct discHdr *request;
	volatile bool waiting;
	volatile bool stop;
};

struct BannerInfo banner;

void banner_parse(struct discHdr *header)
{
	memset(&banner.snd, 0, sizeof(banner.snd));
	memset(&banner.title, 0, sizeof(banner.title));
	banner.parsed = false;
	if (wgame.header->magic == GC_GAME_ON_DRIVE) {
		parse_riff(&gc_wav, &banner.snd);
	} else if (wgame.header->magic == CHANNEL_MAGIC) {
		CHANNEL_Banner(header, &banner.snd);
	} else {
		WBFS_Banner(header->id, &banner.snd, banner.title, true, true);
	}
	// CFG_read_active_game_setting(header->id).write_playlog);
	banner.parsed = true;
}

void banner_play()
{
	// play banner sound
	if (banner.snd.dsp_data && !banner.playing) {
		int fmt = (banner.snd.channels == 2) ? VOICE_STEREO_16BIT : VOICE_MONO_16BIT;
		SND_SetVoice(1, fmt, banner.snd.rate, 0,
			banner.snd.dsp_data, banner.snd.size,
			255,255, //volume,volume,
			NULL); //DataTransferCallback
		banner.playing = true;
	}
}

void banner_stop()
{
	// stop banner sound, resume mp3	
	if (banner.snd.dsp_data && banner.playing) {
		SND_StopVoice(1);
		banner.playing = false;
	}
	SAFE_FREE(banner.snd.dsp_data);
}

void* banner_thread(void *arg)
{
	int state = 0;
	while (1) {
		// lock
		//dbg_printf("bt. lock\n");
		LWP_MutexLock(banner.mutex);
		if (!banner.stop) {
			if (banner.request) {
				dbg_printf("banner req: %.6s\n", banner.request->id);
				banner.header = (struct discHdr *)banner.request;
				banner.request = NULL;
				state = 1;
				//dbg_printf("bt. state: %d\n", state);
			}
			if (state == 0) {
				banner.waiting = true;
				//dbg_printf("bt. sleep\n");
				// LWP_CondWait unlocks mutex on enter
				LWP_CondWait(banner.cond, banner.mutex);
				// LWP_CondWait locks mutex on exit
				//dbg_printf("bt. wake\n");
				banner.waiting = false;
			}
		}
		// unlock
		LWP_MutexUnlock(banner.mutex);
		//dbg_printf("bt. unlock\n");

		if (banner.stop) {
			// exit
			//dbg_printf("bt. stop\n");
			banner_stop();
			break;
		}

		switch (state) {
			case 1:
				// parse
				//dbg_printf("bt. stop\n");
				banner_stop();
				//dbg_printf("bt. parse\n");
				if (banner.header) banner_parse(banner.header);
				banner.header = NULL;
				state = 2;
				//dbg_printf("bt. state: %d\n", state);
				break;
			case 2:
				// play
				banner_play();
				state = 0;
				//dbg_printf("bt. state: %d\n", state);
				break;
		}
	}
	dbg_printf("bt. exiting\n");

	return NULL;
}

void banner_thread_start()
{
	if (banner.inited) return;
	memset(&banner, 0, sizeof(banner));
	banner.lwp   = LWP_THREAD_NULL;
	banner.mutex = LWP_MUTEX_NULL;
	banner.cond  = LWP_COND_NULL;
	LWP_MutexInit(&banner.mutex, FALSE);
	LWP_CondInit(&banner.cond);
	// start thread
	LWP_CreateThread(&banner.lwp, banner_thread, NULL, NULL, 64*1024, 40);
	banner.inited = 1;
}

void banner_thread_play(struct discHdr *header)
{
	bool waiting;
	// lock 
	//dbg_printf("b. lock\n");
	LWP_MutexLock(banner.mutex);
	banner.request = header;
	waiting = banner.waiting;
	LWP_MutexUnlock(banner.mutex);
	//dbg_printf("b. unlock\n");
	// wake
	if (waiting) {
		//dbg_printf("b. signal\n");
		LWP_CondSignal(banner.cond);
	}
}

void banner_thread_stop()
{
	bool waiting;
	if (banner.inited == 0) return;
	if (banner.lwp == LWP_THREAD_NULL) return;
	// mutex
	LWP_MutexLock(banner.mutex);
	banner.stop = true;
	waiting = banner.waiting;
	LWP_MutexUnlock(banner.mutex);
	// wake
	if (waiting) {
		//dbg_printf("b. signal 2\n");
		LWP_CondSignal(banner.cond);
	}
	// wait for exit and cleanup
	LWP_JoinThread(banner.lwp, NULL);
	LWP_MutexDestroy(banner.mutex);
	LWP_CondDestroy(banner.cond);
	memset(&banner, 0, sizeof(banner));
	banner.lwp   = LWP_THREAD_NULL;
	dbg_printf("bt. stopped\n");
}

void banner_start()
{
	Music_PauseVoice(true);
	banner_thread_start();
}

void banner_end(bool mute)
{
	banner_thread_stop();
	Music_Mute(mute);
	Music_PauseVoice(false);
}

// bind selected game info and options to GameDialog
void BindGameDialog()
{
	static char game_desc[XML_MAX_SYNOPSIS * 2] = "";
	struct discHdr *header;
	gameSelected = game_select;
	header = &gameList[gameSelected];
	wgame.header = header;
	dbg_printf("game %.6s\n", header->id);
	// title
//if ((memcmp("G",header->id,1)==0 ) && (strlen((char *)header->id)>6))
	//wgame.dialog->name=header->title;
//	else
	wgame.dialog->name = get_title(header);
	text_scale_fit_dialog(wgame.dialog);
	// info
	int rows, cols;
	wgui_textbox_coords(wgame.info, NULL, NULL, &rows, &cols);
	Menu_GameInfoStr(header, game_desc);
	int len = strlen(game_desc);
	FmtGameInfoLong(header->id, cols, game_desc + len, sizeof(game_desc) - len);
	wgui_textbox_wordwrap(wgame.info, game_desc, sizeof(game_desc));
	// favorite, hide
	wgui_set_value(wgame.favorite, is_favorite(header->id) ? 1 : 0);
	wgui_set_value(wgame.hide, is_hide_game(header->id) ? 1 : 0);
	// options
	InitGameOptionsPage(wgame.options, H_NORMAL);
	// banner
	banner_thread_play(header);
}

void ReleaseGameDialog()
{
	// unbind game options
	if (wgame.gcfg2) {
		CFG_release_game(wgame.gcfg2);
		wgame.gcfg2 = NULL;
	}
	wgui_remove_children(wgame.options);
	memset(&wgame.dol_info_list, 0, sizeof(wgame.dol_info_list));
}

void action_next_game(Widget *ww)
{
	ReleaseGameDialog();
	game_select++;
	if (game_select > gameCnt - 1) game_select = 0;
	BindGameDialog();
}

void action_prev_game(Widget *ww)
{
	ReleaseGameDialog();
	game_select--;
	if (game_select < 0) game_select = gameCnt - 1;
	BindGameDialog();
}


void OpenGameDialog()
{
	static int last_game_page = 0;
	Widget *dd;
	Widget *ww;
	Pos p = pos_auto;
	int bh = H_NORMAL;

	memset(&wgame, 0, sizeof(wgame));
	banner_start();

	// XXX bind to desk
	//dd = desk_open_dialog(pos_auto, title);
	pos_desk(&p);
	dd = wgui_add_dialog(NULL, p, "");
	wgame.dialog = dd;
	//dd->handle = NULL;// default: handle_B_close
	pos_margin(dd, PAD1);
	pos_prefsize(dd, dd->w/4, bh);

	Pos pb = pos_auto;
	pb.x = POS_EDGE;
	pb.h = bh + PAD0;

	Widget *w_start = wgui_add_button(dd, pb, gt("Start"));
	w_start->handle = handle_game_start;
	wgame.start = w_start;
	pos_newlines(dd, 1);
	// if wiimote not pointing to screen then
	// move pointer to start button so that btn A will start directly
	bool wpad_stick = Wpad_set_pos(winput.w_ir,
			w_start->ax + w_start->w / 2,
			w_start->ay + w_start->h / 2);
	
	// page radio
	pos_move_to(dd, pos_get(w_start).x, POS_AUTO);
	Widget *w_info_radio = wgui_auto_radio_a(dd, 1, 4,
			gt("Cover"), gt("Info"), gt("Manage"), gt("Options"));
	if (CFG.disable_options) {
		wgui_set_inactive(wgui_link_get(w_info_radio, 2), true);
		wgui_set_inactive(wgui_link_get(w_info_radio, 3), true);
		if (last_game_page > 1) last_game_page = 0;
	}
	// prev/next game
	pos_newline(dd);
	Pos pa = pos_auto;
	pos_move_x(dd, -w_start->w);
	pa.w = (w_start->w - PAD1) / 2;
	ww = wgui_add_button(dd, pa, "<");
	ww->action = action_prev_game;
	ww->action_button = WPAD_BUTTON_LEFT;
	ww = wgui_add_button(dd, pa, ">");
	ww->action = action_next_game;
	ww->action_button = WPAD_BUTTON_RIGHT;
	pos_newline(dd);
	// back
	p = pb;
	p.y = POS_EDGE;
	p.h = bh;
	Widget *w_back = wgui_add_button(dd, p, gt("Back"));


	// page cover
	p = pos_auto;
	p.x = 0;
	p.y = pos_get(w_start).y;
	p.w = pos_get(w_start).x - PAD1;
	p.h = SIZE_FULL;
	Widget *pp;
	Widget *w_game_page = pp = wgui_add_page(dd, NULL, p, "cover");
	wgui_link_page_ctrl(w_game_page, w_info_radio);
	init_cover_page(pp);

	// page: game info
	pp = wgui_add_page(dd, w_game_page, pos_auto, "info");
	pos_pad(pp, 0);
	wgame.info = wgui_add_textbox(pp, pos_wh(SIZE_FULL, -H_NORMAL), TXT_H_SMALL, NULL, 0);
	pos_newline(pp);
	ww = wgui_add_pgswitch(pp, wgame.info, pos_wh(SIZE_FULL, H_NORMAL), NULL);
	
	// page manage
	pp = wgui_add_page(dd, w_game_page, pos_auto, "manage");
	pos_margin(pp, PAD0);
	pos_prefsize(pp, pp->w - PAD0 * 2, H_NORMAL);

	ww = wgui_add_checkbox(pp, pos_auto, gt("Favorite"), true);
	ww->action = action_game_favorite;
	wgame.favorite = ww;
	pos_newline(pp);

	ww = wgui_add_button(pp, pos_auto, gt("Download Missing Covers"));
	ww->action = action_game_missing_covers;
	pos_newline(pp);

	ww = wgui_add_button(pp, pos_auto, gt("Download All Covers"));
	ww->action = action_game_all_covers;
	pos_newline(pp);

	ww = wgui_add_button(pp, pos_auto, gt("Manage Cheats"));
	ww->action = action_game_manage_cheats;
	pos_newline(pp);

	ww = wgui_add_button(pp, pos_auto, gt("Delete Game"));
	Widget *w_delete_game = ww;
	wgui_set_inactive(ww, CFG.disable_remove);
	pos_newline(pp);
	
	ww = wgui_add_button(pp, pos_auto, gt("Dump savegame"));
	ww->action = action_game_dump_savegame;
	pos_newline(pp);

	ww = wgui_add_checkbox(pp, pos_auto, gt("Hide"), true);
	ww->action = action_game_hide;
	wgame.hide = ww;
	wgui_set_inactive(ww, CFG.admin_mode_locked);

	// page options
	pp = wgui_add_page(dd, w_game_page, pos_auto, "options");
	wgame.options = pp;
	
	// select last used page
	// disabled.
	//wgui_set_value(w_info_radio, last_game_page);
	
	BindGameDialog();

	while (1) {
		wgui_input_get();
		
		//ww = wgui_handle(&wgui_desk);
		wgui_input_steal();
		traverse1(&wgui_desk, wgui_update);
		wgui_input_restore(winput.w_ir, winput.p_buttons);

		ww = wgui_handle(dd);
		wgui_input_restore(winput.w_ir, winput.p_buttons);

		if (winput.w_buttons & WPAD_BUTTON_HOME) {
			action_OpenQuit(dd);
		}
		
		Gui_draw_background();
		Gui_set_camera(NULL, 0);
		GX_SetZMode(GX_FALSE, GX_NEVER, GX_TRUE);
		wgui_render(&wgui_desk);
		wgui_render(dd);

		wgui_input_restore(winput.w_ir, winput.p_buttons);
		Gui_draw_pointer(winput.w_ir);
		Gui_Render();
		
		if (dd->closing) break;

		if (ww) {

			if (ww == w_back) break;

			if (ww == w_start || ww == wgame.cover) {
				banner_end(true);
				Switch_WGui_To_Console();
				CFG.confirm_start = 0;
				Menu_Boot(false);
				Switch_Console_To_WGui();
				Music_Mute(false);
				break;
			}

			if (ww == w_delete_game) {
				banner_end(false);
				Switch_WGui_To_Console();
				Menu_Remove();
				Switch_Console_To_WGui();
				Gui_Refresh_List();
				break;
			}
		}
	}
	// remember page so we get the same for next time
	last_game_page = w_game_page->value;
	ReleaseGameDialog();
	banner_end(false);
	wgui_remove(dd);
	if (!wpad_stick) Wpad_set_pos(NULL, 0, 0);
	wgui_input_get();
}



// ########################################################
//
// Global Options
//
// ########################################################



void action_sort_type(Widget *ww)
{
	Gui_Sort_Set(ww->value, sort_desc);
}

void action_sort_order(Widget *ww)
{
	Gui_Sort_Set(sort_index, ww->value);
}

void action_OpenSort(Widget *a_ww)
{
	Widget *dd;
	Widget *rr = NULL;
	Widget *ww;
	int i;
	char *name[sortCnt];
	
	dd = desk_open_dialog(pos_auto, gt("Sort"));

	for (i=0; i<sortCnt; i++) {
		name[i] = sortTypes[i].name;
		if (i == 1) name[i] = gt("Players");
		if (i == 2) name[i] = gt("Online Players");
	}
	pos_margin(dd, PAD1);
	rr = wgui_arrange_radio(dd, pos_wh(SIZE_FULL, SIZE_AUTO), 3, sortCnt, name);
	rr->val_ptr = &sort_index;
	rr->action = action_sort_type;
	// disable sort by install date on wbfs
	wgui_set_inactive(wgui_link_get(rr, sortCnt-2), wbfs_part_fs == PART_FS_WBFS);
	pos_margin(dd, PAD3);
	pos_newlines(dd, 2);
	
	rr = wgui_arrange_radio_a(dd, pos_xy(POS_CENTER, POS_AUTO),
			2, 2, gt("Ascending"), gt("Descending"));
	rr->val_ptr = &sort_desc;
	rr->update = update_val_from_ptr_bool;
	rr->action = action_sort_order;
	pos_newlines(dd, 2);

	pos_columns(dd, 3, SIZE_FULL);
	ww = wgui_add_button(dd, pos_xy(POS_EDGE, POS_EDGE), gt("Back"));
	ww->action = action_close_parent_dialog;
	ww->action_button = WPAD_BUTTON_B;
}

Widget *r_filter_group;
Widget *w_filter_page;
Widget *r_filter[6];

void action_search_char(Widget *ww)
{
	Widget *rr;
	int i;
	int len = strlen(search_str);
	
	for (i=0; i<6; i++) {		//must be the same size as r_filter
		if (i != 4)				//all but search
			wgui_radio_set(r_filter[i], -1);	//clear filter type
	}

	rr = ww->link_first;
	i = rr->value;

	if ((i >= 0) && (i <= 9))			// 0 - 9
		search_str[len] = '0' + i; 
	else if ((i >= 10) && (i <= 35))	// A = Z
		search_str[len] = 'A' + i - 10;
	else if (i == 36)					// space
		search_str[len] = ' '; 
	else if (i == 37) {					//backspace
		if (len > 0)
			search_str[len - 1] = 0;
	}
	else if (i == 38)					//clear
		search_str[0] = 0; 
	search_str[len + 1] = 0;	//make sure its null terminated
		
    rr->value = -1;		//show all keys as unpressed

	if (cur_search_field < 0) cur_search_field = 0;
	if (cur_search_field >= searchFieldCnt) cur_search_field = searchFieldCnt - 1;
	
	filter_games_set(FILTER_SEARCH, cur_search_field);
	Gui_Refresh_List();

}
void action_search_field(Widget *ww)
{
	Widget *rr;
	int i;

	for (i=0; i<6; i++) {		//must be the same size as r_filter
		if (i != 4)				//all but search
			wgui_radio_set(r_filter[i], -1);	//clear filter type
	}

	rr = ww->link_first;
	i = rr->value;
	if (i < 0) i = 0;

	if (cur_search_field <= 5)
		cur_search_compare_type = 0;	//only allow contains
	if ((cur_search_field > 5) && (cur_search_compare_type == 0))
		cur_search_compare_type = 4;	//default to >=

//	filter_games_set(FILTER_SEARCH, i);
	filter_games_set(FILTER_SEARCH, cur_search_field);
	Gui_Refresh_List();

}

void action_search_compare_type(Widget *ww)
{
	int i;
	for (i=0; i<6; i++) {		//must be the same size as r_filter
		if (i != 4)				//all but search
			wgui_radio_set(r_filter[i], -1);	//clear filter type
	}

	cur_search_compare_type = ww->value;
	filter_games_set(FILTER_SEARCH, cur_search_field);
	Gui_Refresh_List();

}

void Init_Search_Dilog(Widget *dd)
{
//	Widget *dd;
	Widget *ww;

//	dd = desk_open_dialog(pos(POS_CENTER,118,550,276), gt("Search"));
//	dd->update = update_search;

	r_filter[4] = ww = wgui_add_opt(dd, gt("Search for:"), searchFieldCnt, searchFields);
	ww->val_ptr = &cur_search_field;
	ww->action = action_write_val_ptr_int;
	ww->action2 = action_search_field;

//	pos_newlines(dd, 1);
//	pos_columns(dd, 2, SIZE_FULL);
//	pos_rows(dd, 8, -PAD0);
//	wgui_add_text(dd, pos_auto, gt("Search for: "));
	ww = wgui_add_superbox(dd, pos_auto, gt("Compare Type"), searchCompareTypeCnt, searchCompareTypes);
	ww->val_ptr = &cur_search_compare_type;
	ww->action = action_write_val_ptr_int;
	ww->action2 = action_search_compare_type;
//wgui_set_inactive(wgui_link_get(ww, 4), true);
	pos_move_y(ww, -20);

	wgui_add_label(dd, pos_auto, search_str);
	pos_newlines(dd, 1);

	pos_pad(dd, PAD0/2);
	pos_rows(dd, 8, -PAD0);
	pos_prefsize(dd, 0, 30);             //h=height of button 

	ww = wgui_arrange_radio_a(dd, pos_w(SIZE_AUTO), 10, 39, 
				"0","1","2","3","4","5","6","7","8","9",
				"A","B","C","D","E","F","G","H","I","J",
				"K","L","M","N","O","P","Q","R","S","T",
				"U","V","W","X","Y","Z"," ","\x11--",gt("Clear"));
	ww->action = action_search_char;
    ww->value = -1;		//show all keys as unpressed
/*	
	pos_columns(dd, 3, SIZE_FULL);
	ww = wgui_add_button(dd, pos_xy(POS_EDGE, POS_EDGE), gt("Back"));
	ww->action = action_close_parent_dialog;
	ww->action_button = WPAD_BUTTON_B;
*/

	// set initial radio values
//	dd->update(dd);
}

void action_filter(Widget *ww)
{
	Widget *rr;
	int i, r = -1, t;
	rr = ww->link_first;
	for (i=0; i<6; i++) {		//must be the same size as r_filter
		if (rr == r_filter[i]) {
			r = i;
		} else {
			wgui_radio_set(r_filter[i], -1);
		}
	}
	if (r < 0) return;
	i = rr->value;
	t = FILTER_ALL;
	switch (r) {
		case 0: t = FILTER_GENRE; break;
		case 1: t = FILTER_CONTROLLER; break;
		case 2:
			i--;
			if (i == -1) t = FILTER_ONLINE;
			else t = FILTER_FEATURES;
			break;
		case 3:
			t = FILTER_GAME_TYPE;
			break;
		case 4:
			t = FILTER_SEARCH;
			break;
		case 5:
			if (i == 0) t = FILTER_ALL;
			else if(i == 1) t = FILTER_UNPLAYED;
			break;
	}
	filter_games_set(t, i);
	Gui_Refresh_List();
}

// filter_type filter_index
char *get_filter_name(int type, int index)
{
	switch (type) {
		case FILTER_ALL:
			return gt("Show All");
		case FILTER_ONLINE:
			return gt("Online Play");
		case FILTER_UNPLAYED:
			return gt("Unplayed");
		case FILTER_GAMECUBE:
			return gt("GameCube");
		case FILTER_WII:
			return gt("Wii");
		case FILTER_CHANNEL:
			return gt("Channel");
		case FILTER_GENRE:
			return genreTypes[index][1];
		case FILTER_CONTROLLER:
			return gt(accessoryTypes[index][1]);
		case FILTER_FEATURES:
			return gt(featureTypes[index][1]);
		case FILTER_DUPLICATE_ID3:
			return gt("Duplicate ID3");
		case FILTER_GAME_TYPE:
			return gt(gameTypes[index][1]);
		case FILTER_SEARCH:
			return search_str;
	}
	return "-";
}

void action_OpenFilter(Widget *a_ww)
{
	Widget *dd;
	Widget *page = NULL;
	Widget *pp;
	Widget *rr;
	Widget *ww;
	int i;
	char *names[genreCnt + accessoryCnt + featureCnt];

	dd = desk_open_dialog(pos_auto, gt("Filter"));

	// groups (tabs)
	r_filter_group = rr = wgui_arrange_radio_a(dd, pos_w(SIZE_FULL),
			4, 4, gt("Genre"), gt("Controller"), gt("Online"), gt("Game Type"));

	// Genre
	pos_margin(dd, PAD1);
	pp = wgui_add_page(dd, page, pos_wh(SIZE_FULL, -H_NORMAL-PAD1), NULL);
	w_filter_page = page = pp;
	wgui_link_page_ctrl(page, r_filter_group);
	for (i=0; i<genreCnt; i++) {
		names[i] = gt(genreTypes[i][1]);
	}
	r_filter[0] = rr = wgui_paginate_radio(pp, pos_full, 3, 4, genreCnt, names);	//add multipagesd genre menu
	wgui_radio_set(rr, -1);
	rr->action = action_filter;
	
	// Controller 
	pp = wgui_add_page(dd, page, pos_auto, NULL);
	for (i=0; i<accessoryCnt; i++) {
		names[i] = gt(accessoryTypes[i][1]);
	}
	names[5] = gt("Classic");
	// -1 = omit last 1 (vitality)
	r_filter[1] = rr = wgui_paginate_radio(pp, pos_full, 3, 4, accessoryCnt-1, names);
	wgui_radio_set(rr, -1);
	rr->action = action_filter;

	// Online
	pp = wgui_add_page(dd, page, pos_auto, NULL);
	names[0] = gt("Online Play");
	for (i=0; i<featureCnt; i++) {
		names[i+1] = gt(featureTypes[i][1]);
	}
	r_filter[2] = rr = wgui_arrange_radio(pp,
			pos(POS_CENTER, POS_AUTO, SIZE_AUTO, SIZE_FULL),
			1, featureCnt+1, names);
	wgui_radio_set(rr, -1);
	rr->action = action_filter;

	// Game Type 
	pp = wgui_add_page(dd, page, pos_auto, NULL);
	for (i=0; i<gameTypeCnt; i++) {
		names[i] = gt(gameTypes[i][1]);
	}
	r_filter[3] = rr = wgui_arrange_radio(pp, pos_full, 3, gameTypeCnt, names);
	wgui_radio_set(rr, -1);
	rr->action = action_filter;

	//search page
	pp = wgui_add_page(dd, page, pos_auto, NULL);
	Init_Search_Dilog(pp);

	pos_margin(dd, PAD3);
	pos_newline(dd);

	// all, unplayed, search, Back
	pos_columns(dd, 4, SIZE_FULL);
	r_filter[5] = rr = wgui_auto_radio_a(dd, 2, 2, gt("Show All"), gt("Unplayed"));
	wgui_radio_set(rr, -1);
	rr->action = action_filter;

	// Search (only add the button here to get the positions correct)
	rr = wgui_add_radio(dd, r_filter_group, pos_auto, gt("Search"));

	// close	
	ww = wgui_add_button(dd, pos_auto, gt("Back"));
	ww->action = action_close_parent_dialog;
	ww->action_button = WPAD_BUTTON_B;

	// set current
	int r = -1;
	i = filter_index;
	switch (filter_type) {
		case FILTER_GENRE:		r = 0;	break;
		case FILTER_CONTROLLER:	r = 1;	break;
		case FILTER_ONLINE:		r = 2;	i = 0;	break;
		case FILTER_FEATURES:	r = 2;	i = filter_index + 1;	break;
		case FILTER_GAME_TYPE:	r = 3;	break;
		case FILTER_SEARCH:		r = 4;	break;
		case FILTER_ALL:		r = 5;	i = 0;	break;
		case FILTER_UNPLAYED:	r = 5;	i = 1;	break;
	}
	if (r >= 0) {
		if (r < 5) wgui_set_value(r_filter_group, r);
		wgui_set_value(r_filter[r], i);
		if ((filter_type == FILTER_GENRE) || (filter_type == FILTER_CONTROLLER))	//if multiple pages
			wgui_set_value(r_filter[r]->parent, i/12);	//set default page
	}
}


Widget *r_style;
Widget *r_rows;
Widget *r_side;

void update_Style(Widget *_ww)
{
	int v;
	// style
	if (gui_style < GUI_STYLE_COVERFLOW) v = gui_style;
	else v = GUI_STYLE_COVERFLOW + CFG_cf_global.theme;
	wgui_radio_set(r_style, v);
	// rows
	wgui_radio_set(r_rows, grid_rows - 1);
	traverse_linked_children(r_rows, wgui_set_inactive, gui_style == GUI_STYLE_COVERFLOW);
	// side
	v = 0;
	if (gui_style == GUI_STYLE_COVERFLOW && !showingFrontCover) v = 1;
	wgui_radio_set(r_side, v);
	traverse_linked_children(r_side, wgui_set_inactive, gui_style != GUI_STYLE_COVERFLOW);
}

void action_Style(Widget *ww)
{
	int style = r_style->value;
	int subs;

	if (style < GUI_STYLE_COVERFLOW) {
		subs = r_rows->value + 1;
	} else {
		subs = style - GUI_STYLE_COVERFLOW;
		style = GUI_STYLE_COVERFLOW;
	}
	// style
	if (Gui_Switch_Style(style, subs)) {
		// re-fresh wpad
		wgui_input_get();
	}
	// coverflow side
	if (gui_style == GUI_STYLE_COVERFLOW) {
		bool front = r_side->value == 0 ? true : false;
		if (front != showingFrontCover) {
			game_select = Coverflow_flip_cover_to_back(true, 2);
		}
	}
	update_Style(NULL);
}

void action_Theme_2(int theme_i)
{
	int new_theme;
	
	if (theme_i == -1)
		new_theme = CFG.profile_theme[0];
	else new_theme = theme_i;
	if (new_theme == cur_theme) return;
	Cache_Invalidate();
	Gui_reset_previous_cover_style();
	CFG_switch_theme(new_theme);
	Gui_save_cover_style();
	Grx_Init();
	Cache_Init();
	if (gui_style != GUI_STYLE_COVERFLOW) {
		// cover area change
		grid_init(gameSelected);
	} else {
		// reflection change
		gameSelected = Coverflow_initCoverObjects(gameCnt, gameSelected, true);
	}
	wgui_init();
	void desk_custom_init();
	desk_custom_init();
	void desk_bar_update();
	desk_bar_update();
}

void action_Theme(Widget *ww)
{
	action_Theme_2(ww->value);
	CFG.profile_theme[CFG.current_profile] = cur_theme;
}

void action_OpenStyle(Widget *aa)
{
	Widget *dd;
	Widget *rr;
	Widget *ww;
	int i, n = 8;
	char *names[n];

	dd = desk_open_dialog(pos_auto, gt("Style"));
	dd->update = update_Style;
	pos_pad(dd, PAD0);
	pos_rows(dd, 7, -PAD0*2);

	// gui style
	for (i=0; i<n; i++) names[i] = map_gui_style[i].name;
	r_style = rr = wgui_arrange_radio(dd, pos_w(SIZE_FULL), 3, n, names);
	rr->action = action_Style;
	pos_newlines(dd, 2);

	// grid rows
	ww = wgui_add_text(dd, pos_auto, gt("Rows:"));
	r_rows = rr = wgui_arrange_radio_a(dd, pos_w(400), 4, 4, "1","2","3","4");
	rr->action = action_Style;
	pos_newlines(dd, 2);

	// coverflow side
	ww = wgui_add_text(dd, pos_auto, gt("Side:"));
	r_side = rr = wgui_arrange_radio_a(dd, pos_w(400), 2, 2,
			gt("Cover~~Front"), gt("Cover~~Back"));
	rr->action = action_Style;
	pos_newlines(dd, 2);
	
	// theme
	ww = wgui_add_text(dd, pos_auto, gt("Theme:"));
	char *theme_names[num_theme];
	for (i=0; i<num_theme; i++) theme_names[i] = theme_list[i];
	ww = wgui_add_superbox(dd, pos_auto, gt("Theme:"), num_theme, theme_names);
	ww->action = action_Theme;
	ww->val_ptr = &cur_theme;

	// close
	ww = wgui_add_button(dd, pos(POS_EDGE, POS_EDGE, W_MEDIUM, SIZE_AUTO), gt("Back"));
	ww->action = action_close_parent_dialog;
	ww->action_button = WPAD_BUTTON_B;

	// set initial radio values
	dd->update(dd);
}

void action_Favorites(Widget *ww)
{
	Gui_Action_Favorites();
}

void action_Profile(Widget *ww)
{
	Gui_Action_Profile(ww->value);
}

void action_OpenView(Widget *aww)
{
	Widget *dd;
	Widget *ww;

	dd = desk_open_dialog(pos_auto, gt("View"));

	void Init_View_Dialog(Widget *dd);
	Init_View_Dialog(dd);

	ww = wgui_add_button(dd, pos(POS_EDGE, POS_EDGE, W_MEDIUM, SIZE_AUTO), gt("Back"));
	ww->action = action_close_parent_dialog;
	ww->action_button = WPAD_BUTTON_B;
}

void action_MissingCovers(Widget *ww)
{
	Switch_WGui_To_Console();
	Download_All_Covers(true);
	Cache_Invalidate();
	Menu_PrintWait();
	Switch_Console_To_WGui();
}

void action_AllCovers(Widget *ww)
{
	// XXX ask for confirmation
	Switch_WGui_To_Console();
	Download_All_Covers(false);
	Cache_Invalidate();
	Menu_PrintWait();
	Switch_Console_To_WGui();
}

void action_DownloadWIITDB(Widget *ww)
{
	Switch_WGui_To_Console();
	Download_XML();
	Download_Titles();
	Menu_PrintWait();
	Switch_Console_To_WGui();
}

void action_DownloadTitles(Widget *ww)
{
	Switch_WGui_To_Console();
	Download_Titles();
	Switch_Console_To_WGui();
}

void action_DownloadPlugins(Widget *ww)
{
	Switch_WGui_To_Console();
	Download_Plugins();
	Switch_Console_To_WGui();
}

void action_DownloadThemes(Widget *ww)
{
	Switch_WGui_To_Console();
	Theme_Update();
	Switch_Console_To_WGui();
}

void action_ProgramUpdate(Widget *ww)
{
	Switch_WGui_To_Console();
	Online_Update();
	Switch_Console_To_WGui();
}

void Init_Online_Dialog(Widget *dd, bool back)
{
	Widget *ww;
	int rows = back ? 7 : 6;
	pos_rows(dd, rows, SIZE_FULL);
	Pos p = pos_auto;
	p.x = POS_CENTER;
	//p.w = dd->w * 2 / 3;
	if (pos_avail_w(dd) > 400) p.w = 400;
	else p.w = SIZE_FULL;

	ww = wgui_add_button(dd, p, gt("Download Missing Covers"));
	ww->action = action_MissingCovers;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);

	ww = wgui_add_button(dd, p, gt("Download All Covers"));
	ww->action = action_AllCovers;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);

	ww = wgui_add_button(dd, p, gt("Download game information"));
	ww->action = action_DownloadWIITDB;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);

	ww = wgui_add_button(dd, p, gt("Download Plugins"));
	ww->action = action_DownloadPlugins;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);

	ww = wgui_add_button(dd, p, gt("Download Themes"));
	ww->action = action_DownloadThemes;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);

	ww = wgui_add_button(dd, p, gt("Program Updates"));
	ww->action = action_ProgramUpdate;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);

	if (back) {
		ww = wgui_add_button(dd, p, gt("Back"));
		ww->action = action_close_parent_dialog;
	}
}

void action_OpenOnline(Widget *_ww)
{
	Widget *dd;
	dd = desk_open_dialog(pos_auto, gt("Online Updates"));
	Init_Online_Dialog(dd, true);
}

void action_Device(Widget *ww)
{
	Switch_WGui_To_Console();
	Menu_Device();
	Switch_Console_To_WGui();
	Gui_Refresh_List();
}

void action_Partition(Widget *ww)
{
	Switch_WGui_To_Console();
	Menu_Partition(true);
	Switch_Console_To_WGui();
	Gui_Refresh_List();
}

void action_SaveDebug(Widget *ww)
{
	Switch_WGui_To_Console();
	Save_Debug();
	Save_IOS_Hash();
	Menu_PrintWait();
	Switch_Console_To_WGui();
}

void action_IOS_Info(Widget *ww)
{
	Switch_WGui_To_Console();
	Menu_All_IOS_Info();
	Switch_Console_To_WGui();
}

void action_SaveSettings()
{
	Switch_WGui_To_Console();
	Menu_Save_Settings();
	Switch_Console_To_WGui();
}

void Init_Info_Dialog(Widget *dd)
{
	Widget *page, *pp, *rr, *tt, *ww;
	static char basic[400];
	static char iosinfo[500];
	static char debugstr[DBG_LOG_SIZE + 400 + 500];
	int i;

	page = wgui_add_pages(dd, pos_wh(SIZE_FULL, -H_NORMAL-PAD1), 3, NULL);
	for (i=0; i<3; i++) {
		pp = wgui_page_get(page, i);
		pp->render = NULL; // don't draw page on page
		pos_margin(pp, 0);
		pos_pad(pp, 0);
	}
	rr = wgui_arrange_radio_a(dd, pos_fill, 3, 3, gt("Basic"), "IOS", gt("Debug"));
	wgui_link_page_ctrl(page, rr);
	// basic
	Print_SYS_Info_str(basic, sizeof(basic), false);
	pp = wgui_page_get(page, 0);
	wgui_add_textbox(pp, pos_full, TXT_H_NORMAL, basic, sizeof(basic));
	// ios
	print_all_ios_info_str(iosinfo, sizeof(iosinfo));
	pp = wgui_page_get(page, 1);
	wgui_add_textbox(pp, pos_full, TXT_H_NORMAL, iosinfo, sizeof(iosinfo));
	// debug
	int size = sizeof(debugstr);
	char *str = debugstr;
	print_debug_hdr(str, size);
	str_seek_end(&str, &size);
	strcopy(str, dbg_log_buf, size);
	pp = wgui_page_get(page, 2);
	tt = wgui_add_textbox(pp, pos_wh(SIZE_FULL, -H_SMALL),
			TXT_H_TINY, debugstr, sizeof(debugstr));
	wgui_add_pgswitchx(pp, tt, pos_h(H_SMALL), NULL, 1, "%2d/%2d", 90);
	ww = wgui_add_button(pp, pos(POS_EDGE, POS_AUTO, SIZE_AUTO, H_SMALL), gt("Save Debug"));
	ww->action = action_SaveDebug;
}

void Init_Style_Dialog(Widget *dd)
{
	Widget *ww;

	dd->update = update_Style;

	// gui style
	r_style = ww = wgui_add_opt_map(dd, gt("Style:"), map_gui_style);
	ww->action = action_Style;

	// grid rows
	r_rows = ww = wgui_add_opt_a(dd, gt("Rows:"), 4, "1", "2", "3", "4");
	ww->action = action_Style;

	// coverflow side
	r_side = ww = wgui_add_opt_a(dd, gt("Side:"), 2,
			gt("Cover~~Front"), gt("Cover~~Back"));
	ww->action = action_Style;
	
	update_Style(NULL);

	// theme
	ww = wgui_add_opt_array(dd, gt("Theme:"), num_theme, sizeof(theme_list[0]), theme_list);
	ww->action = action_Theme;
	ww->val_ptr = &cur_theme;

	// pointer scroll
	ww = wgui_add_opt(dd, gt("Scroll:"), 2, NULL);
	ww->val_ptr = &CFG.gui_pointer_scroll;
	ww->action = action_write_val_ptr_int;

	// XXX pop-up Style
	ww = wgui_add_button(dd, pos_auto, gt("Open Style"));
	ww->action = action_OpenStyle;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);
}

void update_FilterName(Widget *ww)
{
	ww->name = get_filter_name(filter_type, filter_index);
}

void Init_View_Dialog(Widget *dd)
{
	Widget *ww;

	ww = wgui_add_opt(dd, gt("Favorites:"), 2, NULL);
	ww->val_ptr = &enable_favorite;
	ww->action = action_Favorites;

	ww = wgui_add_opt_array(dd, gt("Profile:"), CFG.num_profiles,
			sizeof(CFG.profile_names[0]), CFG.profile_names);
	ww->val_ptr = &CFG.current_profile;
	ww->action = action_Profile;

	// sort
	int i;
	char *name[sortCnt];
	for (i=0; i<sortCnt; i++) {
		name[i] = sortTypes[i].name;
		if (i == 1) name[i] = gt("Players");
		if (i == 2) name[i] = gt("Online Players");
	}
	ww = wgui_add_opt(dd, gt("Sort Type:"), sortCnt, name);
	ww->val_ptr = &sort_index;
	ww->action = action_sort_type;

	ww = wgui_add_opt_a(dd, gt("Sort Order:"), 2, gt("Ascending"), gt("Descending"));
	ww->val_ptr = &sort_desc;
	ww->update = update_val_from_ptr_bool;
	ww->action = action_sort_order;

	// filter
	ww = wgui_add_opt_button(dd, gt("Filter:"), "");
	ww->update = update_FilterName;
	ww->action = action_OpenFilter;
	ww->action2 = action_close_parent_dialog;
	update_FilterName(ww);

	ww = wgui_add_button(dd, pos_auto, gt("Open Sort"));
	ww->action = action_OpenSort;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);
}

static char unlock_str[16];
static char unlock_buf[16];

int handle_AdminUnlock(Widget *ww)
{
	if (winput.w_buttons) {
		int i = strlen(unlock_buf);
		unlock_str[i] = '*';
		unlock_buf[i] = get_unlock_buttons(winput.w_buttons);
		i++;
		if (stricmp(unlock_buf, CFG.unlock_password) == 0) {
			Admin_Unlock(true);
			//Gui_Refresh_List();
			ww->closing = true;
		}
		if (i >= 10) {
			ww->closing = true;
		}
	}
	wgui_input_steal_buttons();
	return 0;
}

void Open_AdminUnlock()
{
	Widget *dd;
	Widget *ww;
	memset(unlock_str, 0, sizeof(unlock_str));
	memset(unlock_buf, 0, sizeof(unlock_buf));
	STRCOPY(unlock_str, "..........");
	dd = desk_open_dialog(pos_wh(400, 300), gt("Admin Unlock"));
	dd->handle = handle_AdminUnlock;
	dd->dialog_color = GUI_COLOR_POPUP;
	pos_newlines(dd, 2);
	pos_columns(dd, 2, SIZE_FULL);
	wgui_add_text(dd, pos_auto, gt("Enter Code: "));
	wgui_add_text(dd, pos_auto, unlock_str);
	pos_newlines(dd, 2);
	ww = wgui_add_button(dd, pos(POS_EDGE, POS_EDGE, W_MEDIUM, SIZE_AUTO), gt("Back"));
	ww->action = action_close_parent_dialog;
}

void action_AdminLock(Widget *dd)
{
	if (CFG.admin_mode_locked) {
		Open_AdminUnlock();
	} else {
		Admin_Unlock(false);
	}
}

void Init_System_Dialog(Widget *dd)
{
	Widget *ww;

	char *dev_name = wbfsDev == WBFS_DEVICE_USB ? "USB" : "SD";
	ww = wgui_add_opt_button(dd, gt("Device:"), dev_name);
	ww->action = action_Device;
	ww->action2 = action_close_parent_dialog;

	ww = wgui_add_opt_button(dd, gt("Partition:"), CFG.partition);
	ww->action = action_Partition;
	ww->action2 = action_close_parent_dialog;

	char *wiird_val[3];
	translate_array(3, str_wiird, wiird_val);
	ww = wgui_add_opt(dd, "Wiird", 3, wiird_val);
	ww->val_ptr = &CFG.wiird;
	ww->action = action_write_val_ptr_int;

	// skip game card update option
	ww = wgui_add_opt(dd, gt("Gamer Card:"), 2, NULL);
	ww->val_ptr = &gamercard_enabled;
	ww->action = action_write_val_ptr_int;
	if (!*CFG.gamercard_key || !*CFG.gamercard_url) {
		gamercard_enabled = 0;
		wgui_set_inactive(ww, true);
	}

	// Admin Lock
	ww = wgui_add_opt(dd, gt("Admin Lock:"), 2, NULL);
	ww->val_ptr = &CFG.admin_mode_locked;
	ww->action = action_AdminLock;
	
	// Force devo
	ww = wgui_add_opt_a(dd, gt("Default Gamecube Loader:"), 3, gt("DIOS MIOS"),gt("Devolution"),gt("Nintendont"));
	ww->val_ptr = &CFG.default_gc_loader;
	ww->action = action_write_val_ptr_int;
	

	// Save Settings
	//pos_newline(dd);
	pos_columns(dd, 2, SIZE_FULL);
	ww = wgui_add_button(dd, pos_x(POS_CENTER), gt("Save Settings"));
	ww->action = action_SaveSettings;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);
}

static Widget *w_Settings = NULL;
static Widget *w_settings_radio = NULL;

static int settings_hold_button;
static long long settings_hold_t1;

int handle_Settings(Widget *ww)
{
	int buttons;
	if (CFG.admin_mode_locked && settings_hold_button) {
		buttons = Wpad_Held();
		if (buttons == settings_hold_button) {
			long long t2 = gettime();
			int ms = diff_msec(settings_hold_t1, t2);
			if (ms >= 5000) {
				Open_AdminUnlock();
				settings_hold_button = 0;
			}
		} else {
			settings_hold_button = 0;
		}
	} else {
		settings_hold_button = 0;
	}
	handle_B_close(ww);
	return 0;
}

void update_Settings(Widget *ww)
{
	Widget *rr = w_settings_radio;
	wgui_set_inactive(wgui_link_get(rr, 3), CFG.disable_options);
	wgui_set_inactive(wgui_link_get(rr, 4), CFG.disable_options);
}

void action_OpenSettings(Widget *_ww)
{
	Widget *dd;
	Widget *page;
	Widget *rr;
	Widget *ww;

	settings_hold_button = Wpad_Held();
	settings_hold_t1 = gettime();

	dd = desk_open_singular(pos_auto, gt("Settings"), &w_Settings);
	if (!dd) return;
	dd->handle = handle_Settings;
	dd->update = update_Settings;
	pos_margin(dd, PAD1);

	// 70% page 30% tabs
	Pos p = pos_wh(pos_avail_w(dd)*7/10, SIZE_FULL);
	page = wgui_add_pages(dd, p, 5, "Settings");
	// tabs container
	Widget *cc = wgui_add(dd, pos_fill, NULL);
	pos_pad(cc, PAD1);
	pos_columns(cc, 1, SIZE_FULL); 
	pos_rows(cc, 6, SIZE_FULL); 
	rr = wgui_auto_radio_a(cc, 1, 5,
			gt("Info"), gt("View"), gt("Style"), gt("System"), gt("Updates"));
	wgui_link_page_ctrl(page, rr);
	w_settings_radio = rr;
	ww = wgui_add_button(cc, pos_auto, gt("Back"));
	ww->action = action_close_parent_dialog;

	// Info
	Init_Info_Dialog(wgui_page_get(page, 0));
	// View
	Init_View_Dialog(wgui_page_get(page, 1));
	// Style
	Init_Style_Dialog(wgui_page_get(page, 2));
	// System
	Init_System_Dialog(wgui_page_get(page, 3));
	// Online
	Init_Online_Dialog(wgui_page_get(page, 4), false);
}

void action_Install(Widget *ww)
{
	Switch_WGui_To_Console();
	Menu_Install();
	Switch_Console_To_WGui();
	Gui_Refresh_List();
}

void action_BootDisc(Widget *ww)
{
	Switch_WGui_To_Console();
	//CFG.confirm_start = 0;
	Menu_Boot(true);
	Switch_Console_To_WGui();
}

void action_Console(Widget *ww)
{
	wgui_input_steal_buttons();
	go_gui = false;
	if (gui_style == GUI_STYLE_COVERFLOW) {
		// force transition of central cover
		// instead of the one under pointer
		showingFrontCover = false;
	}
}

char about_title[] = "Configurable SD/USB Loader";
char about_str2[] =
"by oggzee,Dr.Clipper,FIX94,R2-D2199,airline38,Howard"
"\n\n"
"CREDITS: "
"Waninkoko Kwiirk Hermes WiiGator Spaceman Spiff WiiPower "
"davebaol tueidj FIX94 Narolez dimok giantpune Hibern r-win "
"Miigotu fishears pccfatman fig2k4 wiimm Xflak lustar"
"\n\n"
"TRANSLATORS: "
"FIX94 Fox888 TyRaNtM JABE Cambo xxdimixx Hosigumayuugi "
"cherries4u Stigmatic mangojambo LeonLeao tarcis pplucky "
"Geridian Clamis kavid nhlay WiiNero 19872001 MysTiCy"
;

char about_str[sizeof(about_title) + sizeof(about_str2) * 2];

void action_OpenAbout(Widget *_ww)
{
	Widget *dd;
	Widget *ww;

	dd = desk_open_dialog(pos_auto, gt("About"));
	dd->dialog_color = GUI_COLOR_POPUP;
	int pad = PAD1;
	pos_pad(dd, pad);

	STRCOPY(about_str, about_title);
	char *s = about_str;
	s += strlen(s);
	sprintf(s, " v%s \n\n", CFG_VERSION);
	STRAPPEND(about_str, about_str2);

	//pos_newline(dd);
	ww = wgui_add_page(dd, NULL, pos_wh(SIZE_FULL, -H_NORMAL-pad), NULL);
	ww = wgui_add_textbox(ww, pos_full, TXT_H_NORMAL, about_str, sizeof(about_str));
	ww->text_color = GUI_TC_ABOUT;
	//ww->opt = 1; // background

	ww = wgui_add_button(dd, pos_xy(POS_CENTER, POS_AUTO), gt("Back"));
	ww->action = action_close_parent_dialog;
	//ww = wgui_add_button(dd, pos_auto, "Test");
}


static Widget *w_MainMenu = NULL;

void action_OpenMain(Widget *_ww)
{
	Widget *dd;
	Widget *ww;

	dd = desk_open_singular(pos_auto, gt("Main Menu"), &w_MainMenu);
	if (!dd) return; // already open
	pos_margin(dd, PAD3);
	pos_pad(dd, PAD1);
	pos_columns(dd, 2, SIZE_FULL);
	pos_rows(dd, 6, SIZE_FULL);

	ww = wgui_add_button(dd, pos_auto, gt("View"));
	ww->action = action_OpenView;
	ww->action2 = action_close_parent_dialog;

	ww = wgui_add_button(dd, pos_auto, gt("Style"));
	ww->action = action_OpenStyle;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);

	ww = wgui_add_button(dd, pos_auto, gt("Updates"));
	ww->action = action_OpenOnline;
	ww->action2 = action_close_parent_dialog;
	wgui_set_inactive(ww, CFG.disable_options);

	ww = wgui_add_button(dd, pos_auto, gt("Settings"));
	ww->action = action_OpenSettings;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);

	ww = wgui_add_button(dd, pos_auto, gt("Install"));
	ww->action = action_Install;
	ww->action2 = action_close_parent_dialog;
	wgui_set_inactive(ww, CFG.disable_install);

	ww = wgui_add_button(dd, pos_auto, gt("Boot disc"));
	ww->action = action_BootDisc;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);
	
	ww = wgui_add_button(dd, pos_auto, gt("Console"));
	ww->action = action_Console;
	ww->action2 = action_close_parent_dialog;

	ww = wgui_add_button(dd, pos_auto, gt("About"));
	ww->action = action_OpenAbout;
	ww->action2 = action_close_parent_dialog;
	pos_newline(dd);

	ww = wgui_add_button(dd, pos_auto, gt("Quit"));
	ww->action = action_OpenQuit;
	ww->action2 = action_close_parent_dialog;

	ww = wgui_add_button(dd, pos_auto, gt("Back"));
	ww->action = action_close_parent_dialog;
}

static Widget *w_Konami = NULL;

void action_OpenKonami(Widget *_ww)
{
	Widget *dd;
	Widget *ww;

	dd = desk_open_singular(pos_wh(480, 320), "!!!", &w_Konami);
	if (!dd) return; // already open
	
	pos_margin(dd, PAD3);
	pos_newline(dd);
	wgui_add_text(dd, pos_auto, "Debug:");
	ww = wgui_add_numswitch(dd, pos_w(100), " %d ", 0, 9);
	wgui_propagate_value(ww, SET_VAL_MAX, 8);
	ww->val_ptr = &CFG.debug;
	ww->update = update_val_from_ptr_int;
	ww->action = action_write_val_ptr_int;
}


//new actions for alphabetical radio buttons
char *string = "#ABCDEFGHIJKLMNOPQRSTUVWXYZ";
Widget *_jump_rr;

void update_jump()
{
//set default button to current game
    int i, _jump_index;
	char *_jump_selected;
    if (gui_style == GUI_STYLE_COVERFLOW) {
    	game_select = Coverflow_flip_cover_to_back(true, 2);	//flip twice, just to get current pos ??
	    game_select = Coverflow_flip_cover_to_back(true, 2);	
		_jump_index = game_select;
    } else {
        if (gui_style == 0) {
            _jump_index = page_gi;
        } else {
//offset-ed for grid flow mode; problem if gamelist begining is <2 with same letter
            _jump_index = page_gi + page_covers/2;           }
    }
    _jump_selected = skip_sort_ignore(get_title(&gameList[_jump_index]));

	//dbg_printf("Jump %02x%02x = %s \n", _jump_selected[0], _jump_selected[1], _jump_selected);
	if ((_jump_selected[0] == 0xC5) && (_jump_selected[1] == 0x8C))	//Okami
		_jump_selected = "O";		//make it look like non accented character

    for (i = 1; i <= 27; i++){     	//search loop
        if (strncasecmp(_jump_selected, &string[i], 1) == 0) break;  
    }
    if (i >= 27) i=0; 
    _jump_rr->value = i;
}

void action_Jump(Widget *ww)
{
	int _jump_char_offset = _jump_rr->value;
    int i;      		//starts from 0
    char *_yan_name;
    if (_jump_char_offset == 0){
        i =0;
    } else {
        char *_selected_char = &string[_jump_char_offset];
//		char *_selected_char = mbs_align(&string[_jump_char_offset],1);
 
//search loop. Title only
        for (i = 0; i <= gameCnt; i++){     //search loop
            _yan_name = skip_sort_ignore(get_title(&gameList[i]));  //I would deal with title only
//			_yan_name = mbs_align(_yan_name,1);
			if (strncmp(_yan_name,"z",1)>0) continue;		//skip names begining with multi byte chars
            if (!sort_desc){             
                if (strncasecmp(_yan_name,_selected_char,1)>=0) break;
//                if (mbs_coll(_yan_name,_selected_char)>=0) break;
           } else {
                if (strncasecmp(_yan_name,_selected_char,1)<=0) break;
//                if (mbs_coll(_yan_name,_selected_char)<=0) break;
            }
        }
    }

    if (gui_style == GUI_STYLE_COVERFLOW) {
        int _jump_dir = CF_TRANS_ROTATE_LEFT;
    	game_select = Coverflow_flip_cover_to_back(true, 2);    //flip twice, just to get current pos
	    game_select = Coverflow_flip_cover_to_back(true, 2);    
        int _jump_shift_no = i - game_select;
        if (_jump_shift_no == 0) return;
        if (_jump_shift_no > 0) {
            _jump_dir = CF_TRANS_ROTATE_RIGHT;
        } else {
            _jump_shift_no = - _jump_shift_no;
        }
        while (_jump_shift_no > 0) {
            Coverflow_init_transition(_jump_dir, 0, gameCnt, true);
            _jump_shift_no--; 
		}
    } else {
        grid_init_jump(i); //grid_init goes to the page that contains the game not to the game;
//      page_move_to_jump(i);  //does not seems to have any speed improvement so better use init?
	}
}

void action_OpenJump(Widget *ww)
{
	Widget *dd;

	dd = desk_open_dialog(pos(POS_CENTER,100,480,170), gt("Jump"));
	dd->update = update_jump;
	pos_pad(dd, PAD0/2);
	pos_rows(dd, 7, -PAD0);
	pos_prefsize(dd, 0, 30);             //h=height of button 

	_jump_rr = wgui_arrange_radio_a(dd, pos_w(SIZE_AUTO), 9, 27, "#","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z");
	_jump_rr->action = action_Jump;

	// set initial radio values
	dd->update(dd);
}

void desk_dialog_update(Widget *ww)
{
	int alpha = ww->color & 0xFF;
	int pad_y = winput.w_ir->sy;
	int dir = 1;
	int speed = 3;
	int speed2 = 5;
	if (ww->y > 480/2) {
		// bottom - reverse dir.
		dir = -1;
		speed *= dir;
		speed2 *= dir;
		pad_y = 480 - pad_y;
	}
	int delta = (ww->ay - ww->y) * dir;
	if (winput.w_ir->smooth_valid) {
		int y1 = 480/5;
		int y2 = 480/3;
		if (wgui_over(ww) || pad_y < 0) {
			alpha += 8;
			if (delta < 0) ww->ay += speed2;
		} else if (pad_y < y1) {
			if (alpha > 0x80) alpha -= 2; else alpha += 2;
			if (delta  < -ww->h/2) ww->ay += speed;
		} else if (pad_y < y2) {
			if (delta  > -ww->h/2) ww->ay -= speed;
			if (alpha > 0x80) alpha -= 2;
		} else {
			goto hide;
		}
	} else {
		hide:
		alpha -= 8;
		if (delta > -ww->h) ww->ay -= speed;
	}
	delta = (ww->ay - ww->y) * dir;
	if (delta > 0) ww->ay = ww->y;
	if (delta < -ww->h) ww->ay = ww->y - dir * ww->h;
	if (alpha < 0) alpha = 0;
	if (alpha > 0xFF) alpha = 0xFF;
	traverse(ww, wgui_set_color, 0xFFFFFF00 | alpha);
	traverse_children1(ww, adjust_position);

    if (sort_index != 0) {		//if not sorting by name
    	traverse_linked_children(d_top5, wgui_set_inactive, true);				//disable jump  
	} else {
    	traverse_linked_children(d_top5, wgui_set_state, WGUI_STATE_NORMAL);	//enable jump  
    }

}

struct CustomButton
{
	char *name;
	void (*action)(Widget *ww);
};

struct CustomButton custom_button[GUI_BUTTON_NUM] =
{
	{ gts("Main"), action_OpenMain },
	{ gts("Settings"), action_OpenSettings },
	{ gts("Quit"), action_OpenQuit },
	{ gts("Style"), action_OpenStyle },
	{ gts("View"), action_OpenView },
	{ gts("Sort"), action_OpenSort },
	{ gts("Filter"), action_OpenFilter },
	{ "Favorites", action_Favorites },
	{ gts("Jump"), action_OpenJump }
};

char *custom_button_name(int i)
{
	return custom_button[i].name;
}

void desk_custom_update(Widget *ww)
{
	// set custom buttons inactive if any window is opened
	traverse_children(ww, wgui_set_inactive, ww->parent->num_child > 3);
}

// custom buttons
void desk_custom_init()
{
	struct CfgButton *bb;
	char *name;
	Widget *ww;
	int i;
	Pos p;
	if (!d_custom) {
		d_custom = wgui_add(&wgui_desk, pos_full, NULL);
		d_custom->update = desk_custom_update;
	}
	wgui_remove_children(d_custom);
	for (i=0; i<GUI_BUTTON_NUM; i++) {
		bb = &CFG.gui_button[i];
		if (bb->enabled) {
			p = pos(bb->pos.x, bb->pos.y, bb->pos.w, bb->pos.h);
			name = gt(custom_button[i].name);
			if (i == GUI_BUTTON_FAVORITES) {
				ww = wgui_add_checkboxx(d_custom, p, name,
						false, gt("Fav: Off"), gt("Fav: On"));
				ww->val_ptr = &enable_favorite;
			} else {
				ww = wgui_add_button(d_custom, p, name);
			}
			ww->action = custom_button[i].action;
			ww->text_color = GUI_TC_CBUTTON + i;
			ww->max_zoom = 1.0 + (float)bb->hover_zoom / 100.0;
			if (*bb->image && tx_custom[i]) {
				ww->custom_tx = true;
				ww->tx_idx = i;
			}
		}
	}
}

void desk_bar_update()
{
	switch (CFG.gui_bar) {
		case 0:
			d_top->state = d_bottom->state = WGUI_STATE_DISABLED;
			break;
		default:
		case 1:
			d_top->state = d_bottom->state = WGUI_STATE_NORMAL;
			break;
		case 2: // top
			d_top->state = WGUI_STATE_NORMAL;
			d_bottom->state = WGUI_STATE_DISABLED;
			break;
		case 3: // bottom
			d_top->state = WGUI_STATE_DISABLED;
			d_bottom->state = WGUI_STATE_NORMAL;
			break;
	}
}

void desk_dialog_init()
{
	Widget *dd;
	Widget *ww;
	int h = H_LARGE;
	int dh = h + PAD1*2 + PAD2;

	// custom
	desk_custom_init();

//comment pos=x,y,width,heigh
	// top
	dd = d_top = wgui_add_dialog(&wgui_desk, pos(POS_CENTER, 0, 600, dh), NULL);
	dd->update = desk_dialog_update;
	dd->handle = NULL; // disable handle_B_close;
	dd->y -= PAD1;                      //diaglog y offset
	dd->ay -= dh/2;
	dd->color = 0xFFFFFF80;
	dd->lock_focus = false;
	pos_margin(dd, PAD1);
	pos_prefsize(dd, 0, h);             //h=height of button =56
	pos_columns(dd, 5, SIZE_FULL);      //5 View, Sort, Fliter, Fav, Jump
	pos_move_to(dd, 0, -h);             //move buttons x,y

	ww = wgui_add_button(dd, pos_auto, gt("View"));
	ww->action = action_OpenView;

	ww = wgui_add_button(dd, pos_auto, gt("Sort"));
	ww->action = action_OpenSort;
	
	ww = wgui_add_button(dd, pos_auto, gt("Filter"));
	ww->action = action_OpenFilter;
	
	ww = wgui_add_checkboxx(dd, pos_auto, gt("Fav"),
			false, gt("Fav: Off"), gt("Fav: On"));
	ww->val_ptr = &enable_favorite;
	ww->action = action_Favorites;

    ww = d_top5 = wgui_add_button(dd, pos_auto, gt("Jump")); 
    ww->action = action_OpenJump;

	// bottom
	dd = d_bottom = wgui_add_dialog(&wgui_desk, pos(POS_CENTER, -dh, 600, dh), NULL);
	dd->update = desk_dialog_update;
	dd->handle = NULL; // disable handle_B_close;
	dd->y += PAD1;
	dd->ay += dh/2;
	dd->color = 0xFFFFFF80;
	dd->lock_focus = false;
	pos_margin(dd, PAD1);
	pos_prefsize(dd, 0, h);
	pos_columns(dd, 4, SIZE_FULL);

	ww = wgui_add_button(dd, pos_auto, gt("Main"));
	ww->action = action_OpenMain;

	ww = wgui_add_button(dd, pos_auto, gt("Style"));
	ww->action = action_OpenStyle;

	ww = wgui_add_button(dd, pos_auto, gt("Settings"));
	ww->action = action_OpenSettings;

	ww = wgui_add_button(dd, pos_auto, gt("Quit"));
	ww->action = action_OpenQuit;

	desk_bar_update();
}

void wgui_desk_close_dialogs(Widget *except)
{
	// close all except top, bottom and arg
	Widget *cc;
	int i;
	// must be reverse
	for (i = wgui_desk.num_child - 1; i >= 0; i--) {
		cc = wgui_desk.child[i];
		if (cc == d_custom) continue;
		if (cc == d_top) continue;
		if (cc == d_bottom) continue;
		if (cc == except) continue;
		wgui_remove(cc);
	}
}

static int wgui_code = 0;
static int wgui_konami[] = 
{
	WPAD_BUTTON_UP,
	WPAD_BUTTON_UP,
	WPAD_BUTTON_DOWN,
	WPAD_BUTTON_DOWN,
	WPAD_BUTTON_LEFT,
	WPAD_BUTTON_RIGHT,
	WPAD_BUTTON_LEFT,
	WPAD_BUTTON_RIGHT,
	WPAD_BUTTON_B,
	WPAD_BUTTON_A
};

void wgui_konami_handle(int *buttons)
{
	int b = *buttons;
	if (b) {
		if (b & wgui_konami[wgui_code]) {
			wgui_code++;
		} else if (wgui_code == 2 && (b & wgui_konami[0])) {
			wgui_code = 2; // keep same
		} else {
			wgui_code = 0;
		}
		if (wgui_code > 8) {
			*buttons = 0;
		}
		if (wgui_code == 10) {
			if (!CFG.gui_menu) {
				CFG.gui_menu = 1;
				wgui_desk_init();
			} else {
				action_OpenKonami(NULL);
			}
			wgui_code = 0;
		}
	}
}

void wgui_desk_close()
{
	if (!CFG.gui_menu) return;
	wgui_close(&wgui_desk);
	d_top = d_bottom = d_custom = NULL;
}

void wgui_desk_init()
{
	if (!CFG.gui_menu) return;
	wgui_init();
	wgui_desk_close();
	Widget_init(&wgui_desk, NULL, pos_full, NULL);
	desk_dialog_init();
}

void wgui_desk_handle(struct ir_t *ir, int *buttons, int *held)
{
	wgui_konami_handle(buttons);
	if (!CFG.gui_menu) return;

	// set and save input
	wgui_input_set(ir, buttons, held);
	// testing
	/*
	if (*buttons & WPAD_BUTTON_PLUS) {
		*buttons = 0;
		wgui_test();
	}
	*/
	// handle dialogs
	wgui_handle(&wgui_desk);

	if (winput.w_buttons) {
		if (winput.w_buttons & WPAD_BUTTON_A) {
			if (game_select >= 0) {
				// game selected
				OpenGameDialog();
			} else {
				wgui_input_steal_buttons();
			}
		} else {
			int btn_action = get_button_action(winput.w_buttons);
			if ((winput.w_buttons & WPAD_BUTTON_HOME)
					|| btn_action == CFG_BTN_EXIT
					|| btn_action == CFG_BTN_REBOOT )
			{
				action_OpenQuit(NULL);
				wgui_input_steal_buttons();
			}
			if (btn_action == CFG_BTN_MAIN_MENU) {
				wgui_desk_close_dialogs(w_MainMenu);
				action_OpenMain(NULL);
				wgui_input_steal_buttons();
			}
			if (btn_action == CFG_BTN_GLOBAL_OPS || btn_action == CFG_BTN_OPTIONS) {
				wgui_desk_close_dialogs(w_Settings);
				action_OpenSettings(NULL);
				wgui_input_steal_buttons();
			}
		}
	}
}

void wgui_desk_render(struct ir_t *ir, int *buttons)
{
	if (!CFG.gui_menu) return;
	// render
	GX_SetZMode (GX_FALSE, GX_NEVER, GX_TRUE);
	wgui_render(&wgui_desk);
	// restore wpad state for pointer
	wgui_input_restore(ir, buttons);
}


#else 

// STUB
void wgui_desk_init() { }
void wgui_desk_handle(struct ir_t *ir, int *buttons, int *held) { }
void wgui_desk_render(struct ir_t *ir, int *buttons) { }
void wgui_desk_close() { }

#endif

