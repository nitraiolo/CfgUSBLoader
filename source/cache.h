
// by oggzee & usptactical

#ifndef _CACHE_H_
#define _CACHE_H_

#include <ogc/lwp.h>
#include <ogc/cond.h>
#include <ogc/mutex.h>

#include "my_GRRLIB.h"

#define CS_IDLE     0 // free
#define CS_LOADING  1
#define CS_PRESENT  2
#define CS_MISSING  3
#define CS_ERROR    4 // (invalid req or internal error)

#define CC_PRIO_NONE 0
#define CC_PRIO_LOW  1
#define CC_PRIO_MED  2
#define CC_PRIO_HIGH 3

#define CC_FMT_RGBA 0
#define CC_FMT_C4x4 1 // RGBA8 4x4
#define CC_FMT_CMPR 2
#define CC_FMT_MIPMAP 3
// default LOD:
#define CC_MIPMAP_LOD 4 // +1 if HQ
// default format for full and HQ covers:
#define CC_COVERFLOW_FMT CC_FMT_MIPMAP

extern int ccache_inv;

int cache_game_find(int gi);
void cache_request(int, int, int);
void cache_request_before_and_after(int, int, int);
void cache_release_all(void);
int  Cache_Init(void);
void Cache_Close(void);
void Cache_Invalidate(void);
void cache_wait_idle(void);
void cache_num_game();

//#define DEBUG_CACHE
void draw_Cache();

// request with CC_PRIO_NONE will get current state without creating a request
GRRLIB_texImg* cache_request_cover(int game_index, int style, int format, int priority, int *state);

void Cache_Invalidate_Cover(u8 *id, int style);
void cache_stats(char *str, int size);
void cache_remap_style_fmt(int *cstyle, int *fmt);

#endif

