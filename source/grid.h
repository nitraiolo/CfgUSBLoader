#ifndef _GRID_H_
#define _GRID_H_

#include "wpad.h"

void grid_init(int);
void grid_init_jump(int);
void grid_transition(int, int);
int grid_page_change(int);
void grid_transit_style(int new_style, int new_rows);
void grid_transit_rows(int r);
void grid_get_scroll(ir_t *ir);
void grid_calc(void);
int grid_update_all(ir_t *ir);
void grid_draw(int);
void grid_print_title(int);
void draw_grid_cover_at(int game_sel, int x, int y, int maxw, int maxh, int cstyle);

#endif
