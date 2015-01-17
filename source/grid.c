// by oggzee

#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

#include "grid.h"
#include "cfg.h"
#include "gui.h"
#include "cache.h"
#include "wpad.h"
#include "my_GRRLIB.h"
#include "sys.h"
#include "gettext.h"


extern struct discHdr *gameList;
extern s32 gameCnt, gameSelected, gameStart;
extern int game_select;

extern float cam_f;
extern float cam_dir;
extern float cam_z;
extern guVector cam_look;

// maybe 12/16
float widescreen_ratio = 13.0 / 16.0;
float w_ratio = 1.0;

typedef struct Grid_State
{
	float max_w, max_h;
	float x, y, w, h;
	float scale, sx, sy;
	float zoom_step;
	float angle;
	int gi, state;
	int img_x, img_y;
	int center_x, center_y;
	GRRLIB_texImg tx;
} Grid_State;

struct Grid_State *grid_state = NULL;

//int spacing = 20; // between covers
//int spacing_over_y = 24; // overscan 5% of 480 
//int spacing_over_x = 32; // overscan 6.7% of 640 
//int spacing_text = 24; // max font height
//int title_y = 480-20*2+5; //BACKGROUND_HEIGHT-spacing*2+5;
#define spacing        20 // between covers
#define spacing_over_y 24 // overscan 5% of 480 
#define spacing_over_x 20 // overscan 6.7% of 640 = 42
#define spacing_text   24 // max font height

struct RectCoords cover_area;

int grid_columns = 5;
int grid_rows = 2;
int page_covers = 0;		// covers per page
int page_i;					// current page index
int page_gi = -1;			// first visible game index on page
int page_visible;			// num visible covers
int visible_add_rows = 0; 	// additional visible rows on each side of page
int num_pages;

float max_w, max_h;
float scroll_per_cover; // 1 cover w
float scroll_per_page;  // 1 page w
float scroll_max;   // full grid max scroll
float page_scroll = 0; // current scroll position
// 3 max sized grids (4*8)
// plus (5 visible_add_rows) * 2:
// 3 * 4*(8+5*2)
#define GRID_SIZE 216

int grid_alloc = 0;
int grid_covers = 0;



///// GRID stuff

float tran_f(float f1, float f2, float step)
{
	float f;
	f = f1 + (f2 - f1) * step;
	return f;
}

#define TRAN_F(F1, F2, S) F1 = tran_f(F1, F2, S)


void grid_allocate()
{
	memcheck();
	if (grid_alloc < gameCnt || grid_alloc == 0) {
		grid_alloc = gameCnt;
		if (grid_alloc < GRID_SIZE) grid_alloc = GRID_SIZE;
		grid_state = realloc(grid_state, grid_alloc * sizeof(Grid_State));
		if (grid_state == NULL) {
			printf(gt("FATAL: alloc grid(%d)"), grid_alloc);
			printf("\n");
			sleep(5);
			Sys_Exit();
		}
	}
}

void init_cover_area()
{
	if (CFG.gui_cover_area.w == 0) {
		cover_area.x = spacing_over_x;
		cover_area.y = spacing_over_y;
		if (CFG.gui_title_top) {
			cover_area.y += spacing_text;
		}
		cover_area.w = BACKGROUND_WIDTH - spacing_over_x * 2;
		cover_area.h = BACKGROUND_HEIGHT - spacing_over_y * 2 - spacing_text;
	} else {
		cover_area = CFG.gui_cover_area;
	}
}

void reset_grid_1(struct Grid_State *GS)
{
	GS->gi = -1;
	GS->x = 0;
	GS->y = 0;
	GS->w = 0;
	GS->h = 0;
	GS->zoom_step = 0;
	GS->angle = 0;
}

void reset_grid()
{
	int i;
	for (i=0; i<grid_covers; i++) {
		reset_grid_1(&grid_state[i]);
	}
}


bool is_visible(struct Grid_State *GS)
{
	if (GS->gi < page_gi || GS->gi > page_gi + page_visible) return false;
	return true;
}

void update_grid_scale(struct Grid_State *GS, float zoom)
{
	GS->scale = GS->max_w / (GS->tx.w * w_ratio);
	// scale: fit and zoom
	if (GS->tx.h * GS->scale <= GS->max_h) {
		// yes, adjust zoom
		GS->scale = (GS->max_w + zoom) / (GS->tx.w * w_ratio);
	} else {
		// no, fit H instead
		GS->scale = (GS->max_h + zoom) / GS->tx.h;
	}
	GS->sx = GS->scale * w_ratio;
	GS->sy = GS->scale;
	// adjust coords
	GS->w = (float)GS->tx.w * GS->sx;
	GS->h = (float)GS->tx.h * GS->sy;
	GS->x = (float)GS->center_x - (GS->w / 2.0);
	GS->y = (float)GS->center_y - (GS->h / 2.0);
}

/**
 * Checks the CoverCache to see if the cover image for the passed in game is 
 * loaded into the cache yet.  
 *   If the cover is loaded:  the image is saved to the Grid_State.
 *   If the cover isn't loaded: the default "loading" image is returned.
 *   If no cover is found: the default "no cover" image is returned.
 */
void update_grid_state2(struct Grid_State *GS, int cstyle)
{
	int gi = GS->gi;
	if (gi < 0) return;
	GRRLIB_texImg *tx;
	if (!is_visible(GS)) goto idle;
	tx = cache_request_cover(gi, cstyle, CC_FMT_C4x4, CC_PRIO_NONE, &GS->state);
	if (GS->state == CS_PRESENT) {
		if (!tx) goto missing; // internal error
		GS->tx = *tx;
		GS->angle = 0.0;
	} else if (GS->state == CS_IDLE) {
		if (gi > 0) {
			// if previous is loading then mark this as loading too
			int state;
			tx = cache_request_cover(gi-1, cstyle, CC_FMT_C4x4, CC_PRIO_NONE, &state);
			if (state == CS_LOADING) goto loading;
		}
		;idle:;
		GS->tx = tx_hourglass;
		GS->angle = 0.0;
	} else if (GS->state == CS_LOADING) {
		;loading:;
		GS->tx = tx_hourglass;
		GS->angle += 6.0;
		if (GS->angle > 180.0) GS->angle -= 360.0;
	} else { // CS_MISSING
		;missing:;
		GS->tx = tx_nocover;
		GS->angle = 0.0;
	}
	update_grid_scale(GS, 0);
	GS->img_x = GS->center_x - GS->tx.w / 2;
	GS->img_y = GS->center_y - GS->tx.h / 2;
}

void update_grid_state(struct Grid_State *GS)
{
	update_grid_state2(GS, CFG.cover_style);
}

void calc_scroll_range()
{
	max_w = (float)((cover_area.w - spacing) / grid_columns - spacing);
	max_h = (float)((cover_area.h - spacing) / grid_rows - spacing);
	scroll_per_cover = max_w + spacing;
	scroll_per_page = scroll_per_cover * grid_columns;
	// last corner_x
	int last_corner_x = cover_area.x + spacing
		+ (int)(max_w+spacing) * ((grid_covers-1) / grid_rows);

	scroll_max = last_corner_x + max_w + spacing
		- (cover_area.x + cover_area.w);
	// - cover_area.w

	if (scroll_max < 0) scroll_max = 0;
}

float get_scroll_pos(int gi)
{
	return scroll_per_cover * ((int)(gi / grid_rows));
}

void grid_calc_i(int game_i)
{
	int i, gi;
	int ix, iy;
	int corner_x = 0, corner_y;
	struct Grid_State *GS;

	calc_scroll_range();

	int base_x = cover_area.x + spacing;
	int base_y = cover_area.y + spacing;

	for (i=0; i<grid_covers; i++) {

		GS = &grid_state[i];
		gi = game_i + i;
		GS->gi = gi;
		if (gi >= gameCnt) {
			GS->gi = -1;
			break;
		}

		if (gui_style == GUI_STYLE_GRID) {
			ix = i % grid_columns;
			iy = i / grid_columns;
		} else {
			ix = i / grid_rows;
			iy = i % grid_rows;
		}
		GS->max_w = max_w;
		GS->max_h = max_h;
		corner_x = base_x + (int)(max_w+spacing) * ix;
		corner_y = base_y + (int)(max_h+spacing) * iy;
		GS->center_x = corner_x + (int)(max_w / 2);
		GS->center_y = corner_y + (int)(max_h / 2);
	}
}


void grid_calc()
{
	if (gui_style == GUI_STYLE_GRID) {
		grid_calc_i(page_gi);
	} else { // (gui_style == GUI_STYLE_FLOW) {
		grid_calc_i(0);
	}
}


void grid_shift_state(int gi_shift)
{
	int i, j, dir, start, end;
	if (gi_shift > 0) {
		dir = 1;
		start = 0;
		end = grid_covers;
	} else {
		dir = -1;
		start = grid_covers - 1;
		end = -1;
	}
	for (i=start; i!=end; i+=dir) {
		j = i + gi_shift;
		if (j<0 || j >= grid_covers) {
			reset_grid_1(&grid_state[i]);
		} else {
			grid_state[i] = grid_state[j];
		}
	}
}

void draw_grid_1(struct Grid_State *GS, float screen_x, float screen_y)
{
	bool miss, loading;
	float sx, sy;
	u32 color;

	if (GS->gi < 0 || GS->gi >= gameCnt) return;

	miss = true;
	loading = false;
	if (GS->state == CS_PRESENT) miss = false;
	if (GS->state == CS_IDLE || GS->state == CS_LOADING) loading = true;

	if (!loading) {
		sx = GS->sx;
		sy = GS->sy;
	} else {
		sx = sy = 0.5 + GS->zoom_step/5;
	}

	// dim unselected, light up selected
	if (game_select >=0 && GS->gi == game_select) {
		color = 0xFFFFFFFF;
	} else {
		color = 0xDDDDDDFF;
	}

	if (CFG.debug) {
		if (GS->gi == page_gi) {
			GRRLIB_Rectangle(screen_x + GS->center_x - GS->max_w/2 - spacing,
					screen_y + GS->center_y - GS->max_h/2-spacing,
					GS->max_w + spacing*2, GS->max_h+spacing*2, 0x0000FF80, 1);
		}
		if (GS->gi == page_gi + page_visible - 1
				|| GS->gi == gameCnt - 1) {
			GRRLIB_Rectangle(screen_x + GS->center_x - GS->max_w/2 - spacing,
					screen_y + GS->center_y - GS->max_h/2-spacing,
					GS->max_w + spacing*2, GS->max_h+spacing*2, 0xFF000080, 1);
		}
	}

	GRRLIB_DrawImg(screen_x + GS->img_x, screen_y + GS->img_y,
		&GS->tx, GS->angle, sx, sy, color);

	// favorite
	if (is_favorite(gameList[GS->gi].id)) {
		float star_center_x, star_center_y;
		if (loading) {
			star_center_x = GS->center_x + GS->tx.w/2 * sx;
			star_center_y = GS->center_y - GS->tx.h/2 * sy;
		} else {
			star_center_x = GS->center_x + GS->tx.w/2 * sx - tx_star.w/2 * sy;
			star_center_y = GS->center_y - (GS->tx.h/2 - tx_star.h/2) * sy;
		}
		float star_x = star_center_x - tx_star.w/2;
		float star_y = star_center_y - tx_star.h/2;
		/*
		GRRLIB_Rectangle(screen_x + star_x, screen_y + star_y,
			tx_star.w, tx_star.h, 0x00FF0080, 1); 
		GRRLIB_Printf(50, 50, &tx_font, 0xFFFFFFFF, 1, "%d %d %d %d",
				tx_star.offsetx, tx_star.offsety,
				tx_star.handlex, tx_star.handley);
		*/
		GRRLIB_DrawImg( screen_x + star_x, screen_y + star_y,
			&tx_star, 0, sy, sy, color);
	}
	
	if (miss) {
		int x, y;
		x = screen_x + GS->center_x-16;
		y = screen_y + GS->center_y+35;
		FontColor fc;
		fc.color = 0xFFFFFFFF;
		fc.outline = 0x000000FF;
		fc.shadow = 0;
		if (loading) {
			GRRLIB_Rectangle(x-1,y-1, tx_font.tilew*4+2, tx_font.tileh+2, 0x00000040,1);
			fc.outline = 0;
		}
		Gui_PrintfEx(x, y, &tx_font, fc, "%.4s", gameList[GS->gi].id);
	}
}

void draw_grid(int selected, float screen_x, float screen_y)
{
	int i;
	struct Grid_State *GS, *post = NULL;

	for (i=0; i<grid_covers; i++) {
		GS = &grid_state[i];
		if (GS->gi < 0) break;
		if (GS->gi < page_gi || GS->gi > page_gi + page_visible) continue;
		if (GS->gi == selected) {
			post = GS;
		} else {
			draw_grid_1(GS, screen_x, screen_y);
		}
	}
	if (post) {
		draw_grid_1(post, screen_x, screen_y);
	}
	if (CFG.debug) {
		GRRLIB_Rectangle(cover_area.x, cover_area.y, cover_area.w, cover_area.h, 0x0000FFFF, 0);
	}
}

void draw_grid_sel(int selected, float screen_x, float screen_y)
{
	int i;
	struct Grid_State *GS;
	
	for (i=0; i<grid_covers; i++) {
		GS = &grid_state[i];
		if (GS->gi == selected) {
			draw_grid_1(GS, screen_x, screen_y);
		}
	}
}

void grid_draw(int selected)
{
	draw_grid(selected, -page_scroll, 0);
}

bool is_over(struct Grid_State *GS, ir_t *ir, float screen_x, float screen_y)
{
	bool over = false;
	float ir_sx = ir->sx;
	float ir_sy = ir->sy;
	float x1, y1, x2, y2;
	float x, y, w, h;

	if (!is_visible(GS)) return false;

	if (GS->w > 0 && GS->h > 0) {
		//over = GRRLIB_PtInRect(GS->x, GS->y, GS->w, GS->h, ir_sx, ir_sy);
		x1 = GS->x;
		y1 = GS->y;
		x2 = GS->x + GS->w;
		y2 = GS->y + GS->h;
	} else {
		float corner_x = GS->center_x - (int)(GS->max_w / 2);
		float corner_y = GS->center_y - (int)(GS->max_h / 2);
		//over = GRRLIB_PtInRect(corner_x, corner_y, max_w, max_h, ir_sx, ir_sy);
		x1 = corner_x;
		y1 = corner_y;
		x2 = corner_x + GS->max_w;
		y2 = corner_y + GS->max_h;
	}
	x1 += screen_x;
	x2 += screen_x;
	y1 += screen_y;
	y2 += screen_y;
	if (gui_style == GUI_STYLE_FLOW_Z) {
		guVector p1 = { x1, y1, 0.0 };
		guVector p2 = { x2, y1, 0.0 };
		guVector p3 = { x1, y2, 0.0 };
		guVector p4 = { x2, y2, 0.0 };
		gui_tilt_pos(&p1);
		gui_tilt_pos(&p2);
		gui_tilt_pos(&p3);
		gui_tilt_pos(&p4);
		x1 = p1.x;
		y1 = MIN(p1.y, p2.y);
		x2 = p2.x;
		y2 = MAX(p3.y, p4.y);
		/*Gui_set_camera(NULL, 0);
		GRRLIB_Rectangle(x1-10, y1-10, 20, 20, 0xFF0000FF, 1);
		Gui_set_camera(NULL, 1);*/
	}
	x = x1;
	y = y1;
	w = x2 - x1;
	h = y2 - y1; 

	over = GRRLIB_PtInRect(x, y, w, h, ir_sx, ir_sy);
	return over;
}

// return selected
int grid_update_state_s(ir_t *ir, float screen_x, float screen_y)
{
	int i, selected = -1;
	float zoom;
	bool over;
	struct Grid_State *GS;

	for (i=0; i<grid_covers; i++) {

		GS = &grid_state[i];
		if (GS->gi < 0 || GS->gi >= gameCnt) break;

		if (ir) {
			over = is_over(GS, ir, screen_x, screen_y);
		} else {
			over = false;
		}
		// zoom
		if (over) {
			if (GS->zoom_step < 1) GS->zoom_step += 0.2;
			if (GS->zoom_step > 1) GS->zoom_step = 1;
			selected = GS->gi;
		} else {
			if (GS->zoom_step > 0) GS->zoom_step -= 0.05;
			if (GS->zoom_step < 0) GS->zoom_step = 0;
		}
		zoom = (float)spacing * 1.5 * (GS->zoom_step);
		update_grid_state(GS);
		update_grid_scale(GS, zoom);
	}
	return selected;
}

int grid_update_state(ir_t *ir)
{
	// update game cover state
	return grid_update_state_s(ir, -page_scroll, 0);
}

// cap scroll
int update_scroll()
{
	int cap = 0;
	if (page_scroll > scroll_max) {
		page_scroll = scroll_max;
		cap = 1;
	}
	if (page_scroll < 0) {
		page_scroll = 0;
		cap = 1;
	}
	if (gui_style == GUI_STYLE_GRID) {
		page_scroll = 0;
	}
	return cap;
}

int get_first_visible()
{
	return (int)(page_scroll / scroll_per_cover) * grid_rows;
}

void update_visible()
{
	// first visible, num visible
	if (gui_style == GUI_STYLE_GRID) {
		page_visible = page_covers;
	} else { // (gui_style == GUI_STYLE_FLOW) {
		page_gi = get_first_visible() - grid_rows * visible_add_rows;
		if (page_gi < 0) page_gi = 0;
		page_visible = page_covers + grid_rows * visible_add_rows * 2;
	}
}

void cache_visible()
{
	// todo: request only on change?
	cache_release_all();
	cache_request(page_gi, page_visible, 1);
}

// cap page_gi and calc page_i
void update_page_i()
{
	if (page_gi >= gameCnt) page_gi = gameCnt - 1;
	if (page_gi < 0) page_gi = 0;
	if (gui_style == GUI_STYLE_GRID) {
		page_i = page_gi / page_covers;
	} else { // if (gui_style == GUI_STYLE_FLOW) {
		// round up
		page_i = (page_scroll + scroll_per_page - scroll_per_cover/2) / scroll_per_page;
	}
	if (page_i >= num_pages) page_i = num_pages - 1;
	if (page_i < 0) page_i = 0;
}

int grid_update_nocache(ir_t *ir)
{
	// cap scroll
	update_scroll(); // dep: scroll_max
	update_visible();
	update_page_i();
	return grid_update_state(ir);
}

int grid_update_all(ir_t *ir)
{
	// cap scroll
	update_scroll(); // dep: scroll_max
	update_visible();
	cache_visible();
	update_page_i();
	return grid_update_state(ir);
}


void grid_transit(float screen_x, float screen_y, int selected, float tran)
{
	int i;
	struct Grid_State *GS;
	float dest_sx, dest_sy;
	float dest_cx, dest_cy;
	float dest_x, dest_y;
	
	for (i=0; i<grid_covers; i++) {
		GS = &grid_state[i];
		if (GS->gi < 0) break;
		if (GS->gi != selected) continue;

		// if loading, don't transit, leave it where it is
		if (GS->state == CS_IDLE || GS->state == CS_LOADING) break;
		
		// is missing force present to avoid printing game id
		if (GS->state == CS_MISSING) GS->state = CS_PRESENT;

		dest_sx = (float)COVER_WIDTH / (float)GS->tx.w;
		dest_sy = (float)COVER_HEIGHT / (float)GS->tx.h;
		dest_cx = COVER_XCOORD + COVER_WIDTH/2 + page_scroll;
		dest_cy = COVER_YCOORD + COVER_HEIGHT/2;
		dest_x = dest_cx - (float)GS->tx.w / 2;
		dest_y = dest_cy - (float)GS->tx.h / 2;
		TRAN_F( GS->img_x, dest_x, tran );
		TRAN_F( GS->img_y, dest_y, tran );
		GS->img_x -= (int)screen_x;
		GS->img_y -= (int)screen_y;
		TRAN_F( GS->center_x, dest_cx, tran );
		TRAN_F( GS->center_y, dest_cy, tran );
		GS->center_x -= (int)screen_x;
		GS->center_y -= (int)screen_y;
		TRAN_F( GS->sx, dest_sx, tran );
		TRAN_F( GS->sy, dest_sy, tran );
	}
}

void draw_grid_t(float screen_x, float screen_y, int game_i, ir_t *ir, float tran)
{
	grid_calc();
	grid_update_nocache(NULL);
	grid_transit(screen_x, screen_y, gameSelected, tran);
	draw_grid(-1, screen_x-page_scroll, screen_y);
}

void transition_scroll(int direction, int grid_i)
{
	int tran_steps = 15;
	int i, j;
	float step, tran;
	ir_t ir;
	for (i=0; i<=tran_steps; i++) {
		if (direction > 0) j = i; else j = tran_steps - i;
		tran = (float)j / (float)tran_steps;
		step = (float)BACKGROUND_HEIGHT / (float)tran_steps * (float)j;
		Wpad_getIR(&ir);
		GRRLIB_DrawSlice(0, 0, tx_bg_con, 0, 1, 1, 0xFFFFFFFF,
				0, step, BACKGROUND_WIDTH, BACKGROUND_HEIGHT - step);
		GRRLIB_DrawSlice(0, BACKGROUND_HEIGHT - step, tx_bg, 0, 1, 1, 0xFFFFFFFF,
				0, 0, BACKGROUND_WIDTH, step);
		draw_grid_t(0, BACKGROUND_HEIGHT - step, grid_i, NULL, 1-tran);
		Gui_draw_pointer(&ir);
		Gui_Render();
	}
}

void transition_fade(int direction, int grid_i)
{
	int tran_steps = 15;
	int i, j, alpha;
	float tran;
	u32 color;
	ir_t ir;
	for (i=0; i<=tran_steps; i++) {
		if (direction > 0) j = i; else j = tran_steps - i;
		tran = (float)j / (float)tran_steps;
		alpha = 255 * j / tran_steps;
		Wpad_getIR(&ir);

		color = 0xFFFFFF00 | alpha;
		GRRLIB_DrawImg(0, 0, &tx_bg, 0, 1, 1, color);

		draw_grid_t(0, 0, grid_i, NULL, 1-tran);

		color = 0xFFFFFF00 | (255-alpha);
		GRRLIB_DrawImg(0, 0, &tx_bg_con, 0, 1, 1, color);

		draw_grid_sel(gameSelected, -page_scroll, 0);
		
		Gui_draw_pointer(&ir);
		Gui_Render();
	}
}

void grid_transition(int direction, int grid_i)
{
	cache_request(gameSelected, 1, 1);
	if (CFG.gui_transit == 0) {
		transition_scroll(direction, grid_i);
	} else {
		transition_fade(direction, grid_i);
	}
}


void grid_set_style(int style, int r)
{
	gui_style = style;
	grid_rows = r;
	switch (grid_rows) {
		case 1: grid_columns = 4; break;
		case 2: grid_columns = 5; break;
		case 3: grid_columns = 6; break;
		default:
		case 4: grid_columns = 8; break;
	}
	page_covers = grid_columns * grid_rows;
	grid_covers = grid_columns * grid_rows;
	visible_add_rows = 0;
	if (gui_style == GUI_STYLE_FLOW) {
		grid_covers = gameCnt;
		visible_add_rows = 2;
	}
	if (gui_style == GUI_STYLE_FLOW_Z) {
		grid_covers = gameCnt;
		visible_add_rows = 6;
	}
	grid_allocate();
	num_pages = (gameCnt + page_covers - 1) / page_covers;
}

void grid_change_style(int style, int r)
{
	if (style < 0 || style > 2) return;
	if (r < 1 || r > 4) return;

	// save index for alignment
	int mid_gi;   // index to the game in the middle of the page
	int first_gi; // first really visible 
	if (gui_style == GUI_STYLE_GRID) {
		first_gi = page_gi;
	} else { // GUI_STYLE_FLOW
		// idx to column 2
		//first_gi = page_gi + grid_rows * visible_add_rows;
		first_gi = get_first_visible();
	}
	mid_gi = first_gi + page_covers/2 - 1;

	grid_set_style(style, r);

	// align to previous index
	if (gui_style == GUI_STYLE_GRID) {
		// cap scroll
		update_scroll();
		// align to previous middle
		page_i = mid_gi / page_covers;
		if (page_i >= num_pages) page_i = num_pages - 1;
		page_gi = page_i * page_covers;
		// cap page indexes
		update_page_i();
	} else { //if (gui_style == GUI_STYLE_FLOW) {
		// recalc, to get scroll_per_cover
		grid_calc();
		// align to first visible
		page_scroll = get_scroll_pos(first_gi);
		// cap scroll
		update_scroll();
		// get first visible
		update_visible();
		// get page number
		update_page_i();
	}
	// camera
	if (gui_style == GUI_STYLE_FLOW_Z) {
		// 3D
		Gui_set_camera(NULL, 1);
	} else {
		// 2D
		Gui_set_camera(NULL, 0);
	}
	reset_grid();
}

void grid_transit_rows(int r)
{
	int new_style, new_rows;
	r += grid_rows;
	if (r > 4) {
		new_style = (gui_style + 1) % 3;
		new_rows = grid_rows;
	} else if (r < 1) {
		new_style = (gui_style + 1) % 3;
		new_rows = grid_rows;
	} else {
		new_style = gui_style;
		new_rows = r;
	}
	grid_transit_style(new_style, new_rows);
}

void _align_grid(
		struct Grid_State *grid, int gi, int n,
		struct Grid_State *grid1, int gi1, int n1,
		struct Grid_State *grid2, int gi2, int n2)
{
	int i, j1, j2;
	j1 = gi - gi1;
	j2 = gi - gi2;
	for (i=0; i<n; i++,j1++,j2++) {
		if (j1<0) {
			if (j2>=0 && j2<n2) {
				grid[i] = grid2[j2];
				if (gui_style == GUI_STYLE_GRID) {
					grid[i].center_y = -COVER_HEIGHT;
					grid[i].img_y = -COVER_HEIGHT;
				} else {
					grid[i].center_x = -COVER_WIDTH;
					grid[i].img_x = -COVER_WIDTH;
				}
			}
		} else if (j1<n1) {
			grid[i] = grid1[j1];
		} else { // j >= n1
			if (j2>=0 && j2<n2) {
				grid[i] = grid2[j2];
				if (gui_style == GUI_STYLE_GRID) {
					grid[i].center_y = BACKGROUND_HEIGHT + COVER_HEIGHT;
					grid[i].img_y = BACKGROUND_HEIGHT + COVER_HEIGHT;
				} else {
					grid[i].center_x = BACKGROUND_WIDTH + COVER_WIDTH;
					grid[i].img_x = BACKGROUND_WIDTH + COVER_WIDTH;
				}
			}
		}
	}
}

int align_grid(struct Grid_State *grid1, int gi1, int n1,
		struct Grid_State *grid2, int gi2, int n2)
{
	struct Grid_State a_grid1[GRID_SIZE];
	struct Grid_State a_grid2[GRID_SIZE];
	int gi, gl, n;

	gi = MIN(gi1, gi2); // first
	gl = MAX(gi1+n1, gi2+n2); // last
	n = gl - gi; // num
	if (n > GRID_SIZE) n = GRID_SIZE;

	memset(a_grid1, 0, sizeof(a_grid1));
	memset(a_grid2, 0, sizeof(a_grid2));

	_align_grid(a_grid1, gi, n, grid1, gi1, n1, grid2, gi2, n2);
	_align_grid(a_grid2, gi, n, grid2, gi2, n2, grid1, gi1, n1);

	memcpy(grid1, a_grid1, n * sizeof(struct Grid_State));
	memcpy(grid2, a_grid2, n * sizeof(struct Grid_State));
	return n;
}

void transit_grid2(struct Grid_State *grid1, struct Grid_State *grid2, int n, float step)
{
	int i;
	struct Grid_State *GS;
	for (i=0; i<n; i++) {
		update_grid_state(&grid1[i]);
		update_grid_state(&grid2[i]);
		GS = &grid_state[i];
		*GS = grid2[i];

#define TRAN_GRID_F(V) GS->V = tran_f(grid1[i].V, grid2[i].V, step)
#define TRAN_GRID_I(V) GS->V = (int)tran_f((float)grid1[i].V, (float)grid2[i].V, step)

		TRAN_GRID_F(sx);
		TRAN_GRID_F(sy);
		TRAN_GRID_I(img_x);
		TRAN_GRID_I(img_y);
		TRAN_GRID_I(center_x);
		TRAN_GRID_I(center_y);
	}
}

void grid_move(struct Grid_State *grid, int gnum, float x, float y)
{
	int i;
	struct Grid_State *GS;
	for (i=0; i<gnum; i++) {
		GS = &grid[i];
		GS->img_x += (int)x;
		GS->img_y += (int)y;
		GS->center_x += (int)x;
		GS->center_y += (int)y;
	}
}

void grid_copy_vis(struct Grid_State *grid, int *gi, int *gnum)
{
	*gi = page_gi;
	if (gui_style == GUI_STYLE_GRID) {
		if (page_visible < *gnum) *gnum = page_visible;
		memcpy(grid, grid_state, *gnum * sizeof(Grid_State));
	} else {
		int my_gi;
		// take a bit more than visible.
		// take GRID_SIZE/2 from middle of visible
		my_gi = (page_gi + page_visible / 2) - GRID_SIZE / 4;
		// cap start
		if (my_gi < 0) my_gi = 0;
		// cap size to GRID_SIZE/2
		if (*gnum > GRID_SIZE/2) *gnum = GRID_SIZE/2;
		// cap to actual numebr of covers
		if (my_gi + *gnum > grid_covers) {
			*gnum = grid_covers -my_gi;
		}
		// safety check
		if (*gnum < 0) {
			*gnum = 0;
		}
		*gi = my_gi;
		memcpy(grid, grid_state + my_gi, *gnum * sizeof(Grid_State));
		grid_move(grid, *gnum, -page_scroll, 0);
	}
}

int action_alpha = 0x00;
extern char action_string[40];

static int style_alpha = 0x00;
//static int style_x;
static int page_x, page_y;

void print_style(int change)
{
	char *style_name[] = { "grid", "flow", "flow-z", "coverflow" };
	char str[16] = "";
	FontColor font_color = CFG.gui_text;
	if (change) {
		style_alpha = font_color.color & 0xFF;
	} else {
		if (style_alpha > 0) style_alpha -= 3;
		if (style_alpha < 0) style_alpha = 0;
	}
	font_color.color = (font_color.color & 0xFFFFFF00) | style_alpha;
	// reset camera
	Gui_set_camera(NULL, 0);
	if (CFG.gui_lock) {
		// print only rows
		sprintf(str, "[%d]", grid_rows);
	} else {
		// print style and rows
		sprintf(str, "[%s %d]", style_name[gui_style], grid_rows);
	}
	/*
	int text_w = tx_font.tilew * (strlen(str) + 1);
	style_x = page_x - text_w;
	Gui_PrintfEx(style_x, page_y, tx_font, font_color, "%s", str);
	*/
	if (change) {
		strcpy(action_string, str);
		action_alpha = 0xFF;
		style_alpha = 0;
	}
}

void grid_print_title(int selected)
{
	int title_x = spacing_over_x + spacing;
	int title_y;
	int title_w = BACKGROUND_WIDTH - spacing_over_x - title_x;
	char title[80] = "";
	FontColor font_color = CFG.gui_text;
	int w;
	// reset camera
	Gui_set_camera(NULL, 0);
	// position
	if (CFG.gui_title_top) {
		title_y = spacing_over_y;
	} else {
		title_y = BACKGROUND_HEIGHT - spacing_over_y - spacing_text + 2;
	}

	if (CFG.gui_title_area.w) {
		title_x = CFG.gui_title_area.x;
		title_y = CFG.gui_title_area.y;
		title_w = CFG.gui_title_area.w;
	}
	
	// page
	char page_str[16];
	snprintf(page_str, sizeof(page_str), "%d/%d", page_i+1, num_pages);
	w = tx_font.tilew * strlen(page_str);
	if (CFG.gui_page_pos.x < 0) {
		//page_x = BACKGROUND_WIDTH - title_x - w;
		page_x = title_x + title_w - w;
		page_y = title_y;
		title_w -= w;
	} else {
		page_x = CFG.gui_page_pos.x;
		page_y = CFG.gui_page_pos.y;
	}
	Gui_Print(page_x, page_y, page_str);
	
	// style
	print_style(0);

	// clock
	if (CFG.gui_clock_pos.x >= 0) {
		Gui_Print_Clock(CFG.gui_clock_pos.x, CFG.gui_clock_pos.y, CFG.gui_text, -1);
	}

	int x, center, max_len, len;

	center = title_x + title_w / 2;

	static time_t last_time = 0;

	// action
	if (action_alpha) {
		font_color.color = (font_color.color & 0xFFFFFF00) | action_alpha;
		if (action_alpha > 0) action_alpha -= 3;
		if (action_alpha < 0) action_alpha = 0;
		last_time = 0;
		/*
		max_len = title_w / tx_font.tilew;
		len = strlen(action_string);
		if (len > max_len) len = max_len;
		x = center - len * tx_font.tilew / 2;
		if (style_alpha) {
			title_w = style_x - x;
			max_len = title_w / tx_font.tilew;
			if (len > max_len) len = max_len;
		}
		//GRRLIB_Rectangle(title_x, title_y-spacing, 600, spacing, 0xFF0000FF, 1);
		Gui_PrintfEx(x, title_y, tx_font, font_color, "%.*s", len, action_string);
		return;
		*/
		STRCOPY(title, action_string);

	} else {

		// clock
		if (CFG.gui_clock_pos.x < 0) {
			if (selected < 0) {
				if (!CFG.clock_style) return;
				time_t t = time(NULL);
				// wait 2 seconds before showing clock
				if (last_time == 0) { last_time = t; return; }
				if (t - last_time < 2) return;
				int x = BACKGROUND_WIDTH/2;
				int y = title_y + tx_font.tileh/2;
				Gui_Print_Clock(x, y, CFG.gui_text, 0);
				return;
			}
			last_time = 0;
		}

		// title
		if (selected < 0) return;
		STRCOPY(title, get_title(&gameList[selected]));
	}
	max_len = title_w / tx_font.tilew;
	len = con_len(title);
	if (len > max_len) len = max_len;
	x = center - len * tx_font.tilew / 2;
	/*
	if (style_alpha) {
		title_w = style_x - x;
		max_len = title_w / tx_font.tilew;
		if (len > max_len) len = max_len;
	}*/
	//GRRLIB_Rectangle(title_x, title_y, 600, spacing_text, 0x0000FFFF, 1);
	//GRRLIB_Rectangle(title_x, title_y+spacing_text, 600, spacing, 0xFF0000FF, 1);
	//GRRLIB_Rectangle(title_x, title_y-spacing, 600, spacing, 0xFF0000FF, 1);
	//Gui_Printf(x, title_y, "%.*s", len, title);
	char trunc_title[strlen(title)+1];
	STRCOPY(trunc_title, title);
	con_trunc(trunc_title, len);
	//Gui_Printf(x, title_y, "%s", trunc_title);
	Gui_PrintfEx(x, title_y, &tx_font, font_color, "%s", trunc_title);
}


void grid_transit_style(int new_style, int new_rows)
{
	int tran_steps = 15;
	float step;
	ir_t ir;
	int old_gi, new_gi;
	int old_n, new_n;
	int gi, gj, max_n, old_covers;
	int i;
	struct Grid_State grid1[GRID_SIZE], grid2[GRID_SIZE];

	reset_grid();
	grid_calc();
	grid_update_nocache(NULL);
	old_n = GRID_SIZE;
	grid_copy_vis(grid1, &old_gi, &old_n);

	grid_change_style(new_style, new_rows);

	new_gi = page_gi;
	reset_grid();
	grid_calc();
	grid_update_nocache(NULL);
	new_n = GRID_SIZE;
	grid_copy_vis(grid2, &new_gi, &new_n);

	gi = MIN(old_gi, new_gi);
	gj = MAX(old_gi+old_n, new_gi+new_n);
	max_n = gj - gi;

	cache_release_all();
	cache_request(gi, max_n, 1);

	align_grid(grid1, old_gi, old_n, grid2, new_gi, new_n);

	// transition
	old_covers = grid_covers;
	grid_covers = max_n;
	reset_grid();
	for (i=1; i<tran_steps; i++) {
		Wpad_getIR(&ir);
		Gui_draw_background();

		step = (float)i / (float)tran_steps;
		transit_grid2(grid1, grid2, max_n, step);

		draw_grid(-1, 0, 0);
		print_style(1);
		Gui_Render_Out(&ir);
	}
	grid_covers = old_covers;
	reset_grid();
	grid_calc();
	grid_update_all(NULL);

	//cache_release_all();
	//cache_request(page_i * grid_covers, grid_covers, 1);
}

void grid_get_scroll(ir_t *ir)
{
	float w2, sx, dir = -1, s = 0;
	float scroll_w, hold_w = 50, out = 64;
	float dx; // distance from hold
	float speed = 20;
	
	// is pointer scroll enabled?
	if (!CFG.gui_pointer_scroll) return;
	// if y outside defined cover area don't scroll
	if (CFG.gui_cover_area.h) {
		if (ir->sy < CFG.gui_cover_area.y
				|| ir->sy > CFG.gui_cover_area.y + CFG.gui_cover_area.h) {
			return;
		}
	}
	// if y out of screen, don't scroll
	if (ir->sy < 0 || ir->sy > BACKGROUND_HEIGHT) return;
	// allow x out of screen by pointer size
	if (ir->sx < -out || ir->sx > BACKGROUND_WIDTH+out) return;
	sx = ir->sx;
	// slow down if out of screen?
	//if (sx < 0) sx = -sx;
	//if (sx > BACKGROUND_WIDTH) sx = BACKGROUND_WIDTH - (sx-BACKGROUND_WIDTH);
	// cap
	if (sx < 0) sx = 0;
	if (sx > BACKGROUND_WIDTH) sx = BACKGROUND_WIDTH;
	// direction
	w2 = BACKGROUND_WIDTH / 2;
	scroll_w = w2 - hold_w;
	if (sx > w2) {
		sx = BACKGROUND_WIDTH - sx;
		dir = 1;
	}
	dx = w2 - sx;
	if (dx > hold_w) {
		// in scroll zone
		s = (dx - hold_w) / scroll_w;
		s = s * s * speed * dir;
	}
	
	page_scroll += s;
}

void transition_page_grid(int gi_1, int gi_2)
{
	int tran_steps = 20;
	int i;
	float step, dir;
	ir_t ir;
	
	reset_grid();
	for (i=1; i<tran_steps; i++) {
		Wpad_getIR(&ir);
		if (gi_2 > gi_1) dir = 1; else dir = -1;
		step = (float)BACKGROUND_WIDTH / (float)tran_steps * (float)i;
		Gui_draw_background();
		// page 1
		grid_calc_i(gi_1);
		grid_update_nocache(NULL);
		draw_grid(-1, -step * dir, 0);
		// page 2
		grid_calc_i(gi_2);
		grid_update_nocache(NULL);
		draw_grid(-1, (BACKGROUND_WIDTH - step) * dir, 0);
		// done
		Gui_Render_Out(&ir);
	}
}

void transition_page_flow(float new_scroll)
{
	int i, tran_steps = 20;
	float step, dir;
	int end = 0;
	ir_t ir;
	if (new_scroll > page_scroll) {
		dir = 1;
	} else {
		dir = -1;
	}
	step = scroll_per_page / tran_steps * dir;
	for (i=0; i<tran_steps && !end; i++) {
		Wpad_getIR(&ir);
		Gui_draw_background();
		page_scroll += step;
		end = update_scroll();
		grid_update_nocache(&ir);
		grid_draw(-1);
		// done
		Gui_Render_Out(&ir);
	}
}

int grid_page_change(int pg_change)
{
	int new_page = 0;
	if (gui_style == GUI_STYLE_GRID) {
		new_page = page_i + pg_change;
		if (new_page >= num_pages) new_page = num_pages - 1;
		if (new_page < 0) new_page = 0;
		if (new_page != page_i) {
			cache_request(new_page * grid_covers, grid_covers, 1);
			transition_page_grid(page_i * grid_covers, new_page * grid_covers);
			cache_release_all();
			cache_request(new_page * grid_covers, grid_covers, 1);
			page_i = new_page;
			page_gi = page_i * grid_covers;
			return 1;
		}
	} else { // if (gui_style == GUI_STYLE_FLOW) {
		int new_gi;
		float new_scroll;
		new_scroll = page_scroll + scroll_per_page * pg_change;
		//page_scroll = new_scroll;
		//update_scroll();
		new_gi = page_gi + page_covers * pg_change;
		cache_request(new_gi, page_visible, 1);
		transition_page_flow(new_scroll);
		update_visible();
		cache_visible();
		update_page_i();
		return 1;
	}
	return 0;
}

void page_move_to(int gi)
{
	if (gui_style == GUI_STYLE_GRID) {
		page_i = gi / grid_covers;
		page_gi = page_i * grid_covers;
	} else {
		float gi_scroll;
		calc_scroll_range();
		gi_scroll = get_scroll_pos(gi);
		// if not on page, center on screen 
		if (page_scroll < gi_scroll - scroll_per_page) {
			page_scroll = gi_scroll - scroll_per_page / 2;
		}
		if (page_scroll > gi_scroll) {
			page_scroll = gi_scroll - scroll_per_page / 2;
		}
	}
	update_scroll();
	update_page_i();
}

void grid_init_ratio()
{
	if (CFG.widescreen) w_ratio = widescreen_ratio;
	else w_ratio = 1.0;
}


void page_move_to_jump(int gi)
{
	if (gui_style == GUI_STYLE_GRID) {
		page_i = gi / grid_covers;
		page_gi = gi;

	} else {
		float gi_scroll;
		calc_scroll_range();
		gi_scroll = get_scroll_pos(gi);
		// if not on page, center on screen 
		if (page_scroll < gi_scroll - scroll_per_page) {
			page_scroll = gi_scroll - scroll_per_page / 2;
		}
		if (page_scroll > gi_scroll) {
			page_scroll = gi_scroll - scroll_per_page / 2;
		}
	}
	update_scroll();
	update_page_i();
}

void grid_init_jump(int game_sel)
{
	grid_init_ratio();
	grid_allocate();
	init_cover_area();
	grid_set_style(gui_style, grid_rows);
	// move page to currently selected
	page_move_to_jump(game_sel);
	reset_grid();
	grid_calc();
	// update all params and cache current page covers
	grid_update_all(NULL);
}

void grid_init(int game_sel)
{
	grid_init_ratio();
	grid_allocate();
	init_cover_area();
	grid_set_style(gui_style, grid_rows);
	// move page to currently selected
	page_move_to(game_sel);
	reset_grid();
	grid_calc();
	// update all params and cache current page covers
	grid_update_all(NULL);
}

void draw_grid_cover_at(int game_sel, int x, int y, int maxw, int maxh, int cstyle)
{
	struct Grid_State GS1;
	struct Grid_State *GS = &GS1;
	cache_release_all();
	cache_request_cover(game_sel, cstyle, CC_FMT_C4x4, CC_PRIO_HIGH, NULL);
	grid_init_ratio();
	page_gi = game_sel;
	page_visible = 1;
	GS->gi = game_sel;
	GS->center_x = x;
	GS->center_y = y;
	GS->max_w = maxw;
	GS->max_h = maxh;
	update_grid_state2(GS, cstyle);
	if (GS->sy > 1.2) {
		GS->sy = 1.2;
		GS->sx = 1.2 * w_ratio;
	}
	draw_grid_1(GS, 0, 0);
}


