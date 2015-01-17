
// by oggzee & usptactical

#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "cache.h"
#include "mem.h"
#include "cfg.h"
#include "gui.h"
#include "coverflow.h"
//#include "disc.h"
//#include "gettext.h"

//#define cc_dbg dbg_printf
#define cc_dbg(...)

//0x2000000 = 32 MB
//0x1700000 = 23 MB
#define COVER_CACHE_DATA_SIZE 0x1900000 // 25 MB

extern struct discHdr *all_gameList;
extern struct discHdr *gameList;
extern s32 gameCnt, all_gameCnt;

#if 0

struct Cover_State
{
	char used;
	char id[8];
	int gid;    // ccache.game[gid]
	GRRLIB_texImg tx;
	//int lru;
};

struct Game_State
{
	int request;
	int state; // idle, loading, present, missing
	int cid; // cache index
	int agi; // actual all_gameList[agi] (mapping gameList -> all_gameList)
	char id[8];
};

struct Cover_Cache
{
	void *data;
	int width4, height4;
	int csize;
	int num, cover_alloc, lru;
	struct Cover_State *cover;
	int num_game, game_alloc;
	struct Game_State *game;
	volatile int run;
	volatile int idle;
	volatile int restart;
	volatile int quit;
	lwp_t lwp;
	lwpq_t queue;
	mutex_t mutex;
};

struct Cover_Cache ccache;
int ccache_init = 0;
int ccache_inv = 0;

//****************************************************************************
//**************************  START CACHE CODE  ******************************
//****************************************************************************

void game_populate_ids()
{
	int i;
	char *id;

	for (i=0; i<all_gameCnt; i++) {
		memcheck_ptr(all_gameList, &all_gameList[i]);
		id = (char*)all_gameList[i].id;
		memcheck_ptr(ccache.game,&ccache.game[i]);
		STRCOPY(ccache.game[i].id, id);
	}
}

int cache_game_find_id(int gi)
{
	int i;
	bool getNewIds = true;
	char *id = (char*)gameList[gi].id;
	
	retry:;
	for (i=0; i<all_gameCnt; i++) {
		memcheck_ptr(ccache.game,&ccache.game[i]);
		if (strcmp(ccache.game[i].id, id) == 0) {
			ccache.game[gi].agi = i;
			return i;
		}
	}
	//didn't find the game so lets load up the 
	//ids in case they're not loaded yet
	if (getNewIds) {
		game_populate_ids();
		getNewIds = false;
		goto retry;
	}
	//should never get here...
	return -1;
}

// find gameList[gi] in all_gameList[]
int cache_game_find(int gi)
{
	// try if direct mapping exists
	int agi = ccache.game[gi].agi;
	if ( (strcmp(ccache.game[agi].id, (char*)gameList[gi].id) == 0) &&
	     (strcmp(ccache.game[agi].id, (char*)all_gameList[agi].id) == 0) )
	{
		return agi;
	}
	// direct mapping not found, search for id
	return cache_game_find_id(gi);
}

int cache_find(char *id)
{
	int i;
	for (i=0; i<ccache.num; i++) {
		if (strcmp(ccache.cover[i].id, id) == 0) {
			return i;
		}
	}
	return -1;
}

int cache_find_free()
{
	int i;
	for (i=0; i<ccache.num; i++) {
		if (*ccache.cover[i].id == 0) {
			return i;
		}
	}
	return -1;
}

int cache_find_lru()
{
	int i, idx;
	//int lru = -1;
	//int found = -1;
	// not a true lru, but good enough
	for (i=0; i<ccache.num; i++) {
		idx = (ccache.lru + i) % ccache.num;
		if (ccache.cover[idx].used == 0) {
			ccache.lru = (idx + 1) % ccache.num;
			return idx;
		}
	}
	return -1;
}

void cache_free(int i)
{
	memcheck_ptr(ccache.cover, &ccache.cover[i]);
	int gid = ccache.cover[i].gid;
	if (*ccache.cover[i].id && gid >= 0) {
		memcheck_ptr(ccache.game, &ccache.game[gid]);
		ccache.game[gid].state = CS_IDLE;
	}
	*ccache.cover[i].id = 0;
	ccache.cover[i].used = 0;
	//ccache.cover[i].lru = 0;
	ccache.cover[i].gid = -1;
}

int cache_alloc(char *id)
{
	int i;
	i = cache_find(id);
	if (i >= 0) goto found;
	i = cache_find_free();
	if (i >= 0) goto found;
	i = cache_find_lru();
	if (i >= 0) goto found;
	return -1;
	found:
	cache_free(i);
	return i;
}

#ifdef DEBUG_CACHE

struct Cache_Debug
{
	int cc_rq_prio;
	int cc_i;
	int cc_line;
	int cc_line_lock;
	int cc_line_unlock;
	int cc_locked;
	int cc_cid;
	void *cc_img;
	void *cc_buf;
};

struct Cache_Debug ccdbg;

#define ___ ccdbg.cc_line = __LINE__
#define CACHE_LOCK() do{ \
	ccdbg.cc_line_lock = __LINE__; \
	LWP_MutexLock(ccache.mutex); \
	ccdbg.cc_locked = 1; }while(0)

#define CACHE_UNLOCK() do{ \
	ccdbg.cc_line_unlock = __LINE__; \
	LWP_MutexUnlock(ccache.mutex); \
	ccdbg.cc_locked = 0; }while(0)

#define CCDBG(X) X

#else

#define ___
#define CACHE_LOCK() LWP_MutexLock(ccache.mutex)
#define CACHE_UNLOCK() LWP_MutexUnlock(ccache.mutex)
#define CCDBG(X) 

#endif


/**
 * This is the main cache thread. This method loops through all the game indexes  
 * (0 through ccache.num_game) and loads each requested cover. The covers are loaded
 * based on the priority level (rq_prio) -> 1 is highest priority, 3 is lowest.
 *
 */

void* cache_thread(void *arg)
{
	int i, ret, actual_i;
	char *id;
	void *img;
	void *buf;
	int cid;
	int rq_prio;
	bool resizeToFullCover = false;
	int cover_height, cover_width;
	char path[200];
	//cc_dbg("thread started\n");

	___;
	while (!ccache.quit) {
		___;
		CACHE_LOCK();
		ccache.idle = 0;
		//cc_dbg("thread running\n");

		restart:

		ccache.restart = 0;
		___;
		//if (0) // disable
		for (rq_prio=1; rq_prio<=3; rq_prio++) {
			___;
			for (i=0; i<ccache.num_game; i++) {
				___;
				CCDBG(ccdbg.cc_rq_prio = rq_prio);
				CCDBG(ccdbg.cc_i = i);
				if (ccache.restart) goto restart;
				//get the actual game index, in case we're using favorites
				actual_i = cache_game_find(i);
				memcheck_ptr(ccache.game, &ccache.game[actual_i]);
				if (ccache.game[actual_i].request != rq_prio) continue;  //was "<"
				// load
				ccache.game[actual_i].state = CS_LOADING;
				memcheck_ptr(gameList, &gameList[i]);
				id = (char*)gameList[i].id;
				
				//cc_dbg("thread processing %d %s\n", i, id);
				img = NULL;
				___;
				CACHE_UNLOCK();
				___;

				//capture the current cover width and height
				cover_height = COVER_HEIGHT;
				cover_width = COVER_WIDTH;

				//load the cover image
				if (CFG.cover_style == CFG_COVER_STYLE_FULL) {
					//try to load the full cover
					ret = Gui_LoadCover_style((u8*)id, &img, false, CFG_COVER_STYLE_FULL, path);
					if (ret < 0) {
						//try to load the 2D cover
						ret = Gui_LoadCover_style((u8*)id, &img, true, CFG_COVER_STYLE_2D, path);
						if (ret && img) resizeToFullCover = true;
					}
				} else {
					ret = Gui_LoadCover_style((u8*)id, &img, false, CFG.cover_style, path);
				}
				//sleep(1);//dbg
				___;
				if (ccache.quit) goto quit;
				___;
				CACHE_LOCK();
				___;
				if (ret > 0 && img) {
					___;
					cid = cache_alloc(id);
					if (cid < 0) {
						// should not happen
						free(img);
						ccache.game[actual_i].state = CS_IDLE;
						goto end_request;
					}
					buf = ccache.data + ccache.csize * cid;
					___;
					CACHE_UNLOCK();
					___;
					memcheck_ptr(ccache.cover, &ccache.cover[cid]);
					CCDBG(ccdbg.cc_img = img);
					CCDBG(ccdbg.cc_buf = buf);
					CCDBG(ccdbg.cc_cid = cid);
					//sleep(3);//dbg

					LWP_MutexUnlock(ccache.mutex);

					//make sure the cover height and width didn't change on us before we resize it
					if ((cover_height == COVER_HEIGHT) &&
						(cover_width == COVER_WIDTH)) {

						if (resizeToFullCover) {
							ccache.cover[cid].tx = Gui_LoadTexture_fullcover(img, COVER_WIDTH, COVER_HEIGHT, COVER_WIDTH_FRONT, buf, path);
							resizeToFullCover = false;
						} else {
							if (CFG.cover_style == CFG_COVER_STYLE_FULL && CFG.gui_compress_covers) {
								ccache.cover[cid].tx = Gui_LoadTexture_CMPR(img, COVER_WIDTH, COVER_HEIGHT, buf, path);
							} else {
								ccache.cover[cid].tx = Gui_LoadTexture_RGBA8(img, COVER_WIDTH, COVER_HEIGHT, buf, path);
							}
							if (!ccache.cover[cid].tx.data && CFG.cover_style == CFG_COVER_STYLE_FULL) {
								//corrupted image - try to load the 2D cover
								ret = Gui_LoadCover_style((u8*)id, &img, true, CFG_COVER_STYLE_2D, path);
								if (ret && img)
									ccache.cover[cid].tx = Gui_LoadTexture_fullcover(img, COVER_WIDTH, COVER_HEIGHT, COVER_WIDTH_FRONT, buf, path);
							}
						}
					} else {
						CCDBG(ccdbg.cc_img = NULL);
						CCDBG(ccdbg.cc_buf = NULL);
						free(img);
						if (ccache.quit) goto quit;
						CACHE_LOCK();
						goto end_request;
					}
					
					CCDBG(ccdbg.cc_img = NULL);
					CCDBG(ccdbg.cc_buf = NULL);
					free(img);
					___;
					if (ccache.quit) goto quit;
					___;
					CACHE_LOCK();
					___;
					if (ccache.cover[cid].tx.data == NULL) {
						cache_free(cid);
						goto noimg;
					}
					// mark
					STRCOPY(ccache.cover[cid].id, id);
					// check if req change
					if (ccache.game[actual_i].request) {
						ccache.cover[cid].used = 1;
						ccache.cover[cid].gid = actual_i;
						ccache.game[actual_i].cid = cid;
						ccache.game[actual_i].state = CS_PRESENT;
						//cc_dbg("Load OK %d %d %s\n", i, cid, id);

					} else {
						ccache.game[actual_i].state = CS_IDLE;
					}
				} else {
					noimg:
					ccache.game[actual_i].state = CS_MISSING;
					//cc_dbg("Load FAIL %d %s\n", i, id);
				}
				end_request:
				// request processed.
				ccache.game[actual_i].request = 0;
				___;
			}
		}
		___;
		CCDBG(ccdbg.cc_rq_prio = 0);
		CCDBG(ccdbg.cc_i = -1);
		ccache.idle = 1;
		//cc_dbg("thread idle\n");
		// all processed. wait.
		while (!ccache.restart && !ccache.quit) {
			___;
			CACHE_UNLOCK();
			//cc_dbg("thread sleep\n");
			___;
			LWP_ThreadSleep(ccache.queue);
			//cc_dbg("thread wakeup\n");
			___;
			CACHE_LOCK();
			___;
		}
		___;
		ccache.restart = 0;
		CACHE_UNLOCK();
	}

	quit:
	___;
	ccache.quit = 0;

	return NULL;
}

#undef ___

void cache_wait_idle()
{
	int i;
	if (!ccache_init) return;
	// wait till idle
	i = 1000; // 1 second
	while (!ccache.idle && i) {
		usleep(1000);
		i--;
	}
}

/**
 * This method is used to set the caching priority of the passed in game index.
 * If the image is already cached or missing then the CoverCache is updated for
 * the cover.
 *
 * @param game_i the game index
 * @param rq_prio the cache priority: 1 = high priority
 */
void cache_request_1(int game_i, int rq_prio)
{
	int cid, actual_i;
	if (game_i < 0 || game_i >= ccache.num_game) return;

	actual_i = cache_game_find(game_i);
	if (ccache.game[actual_i].request>0) return;
	// check if already present
	if (ccache.game[actual_i].state == CS_PRESENT) {
		cid = ccache.game[actual_i].cid;
		ccache.cover[cid].used = 1;
		return;
	}
	// check if already missing
	if (ccache.game[actual_i].state == CS_MISSING) {
		return;
	}
	// check if already cached
	cid = cache_find((char*)gameList[game_i].id);
	if (cid >= 0) {
		ccache.cover[cid].used = 1;
		ccache.cover[cid].gid = actual_i;
		ccache.game[actual_i].cid = cid;
		ccache.game[actual_i].state = CS_PRESENT;
		return;
	}
	// add request
	ccache.game[actual_i].request = rq_prio;
}


/**
 * This method is used to set the caching priority of the passed in game index
 * as well as the next x number of games.
 *
 * @param game_i the starting game index
 * @param num the number of images to set the priority on
 * @param rq_prio the cache priority: 1 = high priority
 */
void cache_request(int game_i, int num, int rq_prio)
{
	int i, idle;
	// setup requests
	CACHE_LOCK();
	for (i=0; i<num; i++) {
		cache_request_1(game_i + i, rq_prio);
	}
	// restart thread
	ccache.restart = 1;
	idle = ccache.idle;
	CACHE_UNLOCK();
	//cc_dbg("cache restart\n");
	LWP_ThreadSignal(ccache.queue);
	if (idle) {
		//cc_dbg("thread idle wait restart\n");
		i = 10;
		while (ccache.restart && i) {
			usleep(10);
			LWP_ThreadSignal(ccache.queue);
			i--;
		}
		if (ccache.restart) {
			//cc_dbg("thread fail restart\n");
		}
	}
	//cc_dbg("cache restart done\n");
}


/**
 * This method is used to set the caching priority of the passed in game index
 * as well as the next x number of games before and after the index.  All the
 * rest of the covers are given a lower priority.
 *
 * @param game_i the starting game index
 * @param num the number of images before and after the game_i to set the priority on
 * @param rq_prio the cache priority: 1 = high priority
 */
void cache_request_before_and_after(int game_i, int num, int rq_prio)
{
	int i, idle;
	int nextRight, nextLeft;

	// setup requests
	CACHE_LOCK();

	// set the first one high
	cache_request_1(game_i, rq_prio);
	
	// alternate the rest
	nextRight = game_i + 1;
	nextLeft = game_i - 1;
	for (i=0; i<num; i++) {
		if (nextRight >= ccache.num_game)
			nextRight = 0;
		cache_request_1(nextRight, 2);
		
		if (nextLeft < 0)
			nextLeft = ccache.num_game - 1;
		cache_request_1(nextLeft, 2);
		
		nextRight++;
		nextLeft--;
	}
	
	// now set the priority to low for +-10 covers
	for (i=0; i<10; i++) {
		if (nextRight >= ccache.num_game)
			nextRight = 0;
		cache_request_1(nextRight, 3);
		
		if (nextLeft < 0)
			nextLeft = ccache.num_game - 1;
		cache_request_1(nextLeft, 3);
		
		nextRight++;
		nextLeft--;
	}
	
	// restart thread
	ccache.restart = 1;
	idle = ccache.idle;
	CACHE_UNLOCK();
	LWP_ThreadSignal(ccache.queue);
	if (idle) {
		//cc_dbg("thread idle wait restart\n");
		i = 10;
		while (ccache.restart && i) {
			usleep(10);
			LWP_ThreadSignal(ccache.queue);
			i--;
		}
		if (ccache.restart) {
			//cc_dbg("thread fail restart\n");
		}
	}
	//cc_dbg("cache restart done\n");
}


void cache_release_1(int game_i)
{
	int cid;
	if (game_i < 0 || game_i >= ccache.num_game) return;
	memcheck_ptr(ccache.game, &ccache.game[game_i]);
	ccache.game[game_i].request = 0;
	if (ccache.game[game_i].state == CS_PRESENT) {
		// release if already cached
		cid = ccache.game[game_i].cid;
		memcheck_ptr(ccache.cover, &ccache.cover[cid]);
		ccache.cover[cid].used = 0; //--
		ccache.game[game_i].state = CS_IDLE;
	}
}

void cache_release(int game_i, int num)
{
	int i;
	// cancel requests
	CACHE_LOCK();
	for (i=0; i<num; i++) {
		cache_release_1(game_i + i);
	}
	CACHE_UNLOCK();
}

void cache_release_all()
{
	int i;
	// cancel all requests
	CACHE_LOCK();
	for (i=0; i<ccache.num_game; i++) {
		cache_release_1(i);
	}
	CACHE_UNLOCK();
}

void cache_num_game()
{
	ccache.num_game = gameCnt;
}

/**
 * Reallocates cache tables, if gamelist resized
 */
int cache_resize_tables()
{
	bool resized = false;
	int size, new_num;

	//cc_dbg("\ncache_resize %p %d %d %p %d %d\n",
	//		ccache.game, ccache.game_alloc, ccache.num_game,
	//		ccache.cover, ccache.cover_alloc, ccache.num);


	memcheck();
	// game table
	// resize if games added
	if (all_gameCnt > ccache.game_alloc || ccache.game == NULL)
	{
		ccache.game_alloc = all_gameCnt;
		size = sizeof(struct Game_State) * ccache.game_alloc;
		ccache.game = realloc(ccache.game, size);
		if (ccache.game == NULL) goto err;
		resized = true;
	}
	cache_num_game();

	memcheck();
	// cover table
	// check cover size

	if (CFG.cover_style == CFG_COVER_STYLE_FULL && CFG.gui_compress_covers) {
		ccache.width4 = COVER_WIDTH;
		ccache.height4 = COVER_HEIGHT;
		ccache.csize = (ccache.width4 * ccache.height4)/2;
	} else {
		ccache.width4 = (COVER_WIDTH + 3) >> 2 << 2;
		ccache.height4 = (COVER_HEIGHT + 3) >> 2 << 2;
		ccache.csize = ccache.width4 * ccache.height4 * 4;
	}
	
	new_num = COVER_CACHE_DATA_SIZE / ccache.csize;
	if (new_num > ccache.cover_alloc || ccache.cover == NULL) {
		ccache.cover_alloc = new_num;
		size = sizeof(struct Cover_State) * ccache.cover_alloc;
		ccache.cover = realloc(ccache.cover, size);
		if (ccache.cover == NULL) goto err;
		resized = true;
	}
	ccache.num = new_num;

	memcheck();
	if (resized) {
		// clear both tables
		size = sizeof(struct Game_State) * ccache.game_alloc;
		memset(ccache.game, 0, size);
		size = sizeof(struct Cover_State) * ccache.cover_alloc;
		memset(ccache.cover, 0, size);
		ccache.lru = 0;
	}
	memcheck();

	//cc_dbg("cache_resize %p %d %d %p %d %d\n",
	//		ccache.game, ccache.game_alloc, ccache.num_game,
	//		ccache.cover, ccache.cover_alloc, ccache.num);

	return 0;

err:
	// fatal
	printf(gt("ERROR: cache: out of memory"));
	printf("\n");
	sleep(1);
	return -1;
}	

/**
 * Initializes the cover cache
 */
int Cache_Init()
{
	int ret;

	//update the cache tables size
	if (ccache_inv || !ccache_init) {
		// call invalidate again, for safety
		// this will cancel all requests and wait till idle
		Cache_Invalidate();
		ret = cache_resize_tables();
		if (ret) return -1;
		ccache_inv = 0;
	}
	ccache.num_game = gameCnt;

	if (ccache_init) return 0;
	ccache_init = 1;
	
	ccache.data = LARGE_memalign(32, COVER_CACHE_DATA_SIZE);

	ccache.lwp   = LWP_THREAD_NULL;
	ccache.queue = LWP_TQUEUE_NULL;
	ccache.mutex = LWP_MUTEX_NULL;

	// start thread
	LWP_InitQueue(&ccache.queue);
	LWP_MutexInit(&ccache.mutex, FALSE);
	ccache.run = 1;
	//cc_dbg("running thread\n");
	ret = LWP_CreateThread(&ccache.lwp, cache_thread, NULL, NULL, 32*1024, 40);
	if (ret < 0) {
		//cache_thread_stop();
		return -1;
	}
	//cc_dbg("lwp ret: %d id: %x\n", ret, ccache.lwp);
	return 0;
}

void Cache_Invalidate()
{
	if (!ccache_init) return;
	CACHE_LOCK();
	// clear requests
	memset(ccache.game, 0, sizeof(struct Game_State) * ccache.game_alloc);
	CACHE_UNLOCK();
	// wait till idle
	cache_wait_idle();
	if (!ccache.idle) {
		//printf("\nERROR: cache not idle\n"); sleep(3);
	}
	// clear all
	memset(ccache.game, 0, sizeof(struct Game_State) * ccache.game_alloc);
	memset(ccache.cover, 0, sizeof(struct Cover_State) * ccache.cover_alloc);

	ccache_inv = 1;
}

void Cache_Close()
{
	int i;
	if (!ccache_init) return;
	ccache_init = 0;
	CACHE_LOCK();
	ccache.quit = 1;
	CACHE_UNLOCK();
	LWP_ThreadSignal(ccache.queue);
	i = 1000; // 1 second
	while (ccache.quit && i) {
		usleep(1000);
		LWP_ThreadSignal(ccache.queue);
		i--;
	}
	if (ccache.quit) {
		printf("\n");
		printf(gt("ERROR: Cache Close"));
		printf("\n");
		sleep(5);
	} else {
		LWP_JoinThread(ccache.lwp, NULL);
	}
	LWP_CloseQueue(ccache.queue);
	LWP_MutexDestroy(ccache.mutex);
	ccache.lwp   = LWP_THREAD_NULL;
	ccache.queue = LWP_TQUEUE_NULL;
	ccache.mutex = LWP_MUTEX_NULL;
	SAFE_FREE(ccache.cover);
	SAFE_FREE(ccache.game);
}

void draw_Cache()
{
#ifdef DEBUG_CACHE
	unsigned int i, x, y, c, cid;

	// thread state
	x = 15;
	y = 30;
	c = ccache.idle ? 0x00FF00FF : 0xFF0000FF;
	GRRLIB_Rectangle(x, y, 8, 8, c, 1);
	x += 10;
	c = !ccache.restart ? 0x00FF00FF : 0xFF0000FF;
	GRRLIB_Rectangle(x, y, 8, 8, c, 1);
	x += 10;
	c = !ccache.quit ? 0x00FF00FF : 0xFF0000FF;
	GRRLIB_Rectangle(x, y, 8, 8, c, 1);
	x += 20;
	GRRLIB_Printf(x, y, tx_cfont, 0xFFFFFFFF, 1, "%d %d %d",
			ccdbg.cc_rq_prio, ccdbg.cc_i, ccdbg.cc_line);
	// lock state
	x = 15;
	y = 40;
	c = !ccdbg.cc_locked ? 0x00FF00FF : 0xFF0000FF;
	GRRLIB_Rectangle(x, y, 8, 8, c, 1);
	x += 40;
	GRRLIB_Printf(x, y, tx_cfont, 0xFFFFFFFF, 1, "%d %d %p %p %d",
			ccdbg.cc_line_lock, ccdbg.cc_line_unlock,
			ccdbg.cc_img, ccdbg.cc_buf, ccdbg.cc_cid);

	// cover state
	for (i=0; i<ccache.num; i++) {
		x = 15 + (i % 10) * 2;
		y = 50 + (i / 10) * 2;
		// free: black
		// used: blue
		// present: green
		// add lru to red?
		if (ccache.cover[i].id[0] == 0) {
			c = 0x000000FF;
		} else if (ccache.cover[i].used) {
			c = 0x8080FFFF;
		} else {
			c = 0x80FF80FF;
		}
		GRRLIB_Rectangle(x, y, 2, 2, c, 1);
	}

	// game state
	for (i=0; i<ccache.num_game; i++) {
		x = 15 + (i % 10)*2;
		y = 110 + (i / 10)*2;
		// idle: black
		// loading: yellow
		// used: blue
		// present: green
		// missing: red
		// request: violet
		if (ccache.game[i].request) {
			c = 0xFF00FFFF;
		} else
		switch (ccache.game[i].state) {
			case CS_LOADING: c = 0xFFFF80FF; break;
			case CS_PRESENT:
				cid = ccache.game[i].cid;
				if (ccache.cover[cid].used) {
					c = 0x8080FFFF;
				} else {
					c = 0x80FF80FF;
				}
				break;
			case CS_MISSING: c = 0xFF8080FF; break;
			default:
			case CS_IDLE: c = 0x000000FF; break;
		}
		//GRRLIB_Plot(x, y, c);
		GRRLIB_Rectangle(x, y, 2, 2, c, 1);
	}
#endif
}

GRRLIB_texImg* cache_request_cover(int game_index, int style, int format, int priority, int *state)
{
	GRRLIB_texImg *tx = NULL;
	int actual_i = cache_game_find(game_index);
	int cstate = ccache.game[actual_i].state;
	if (cstate == CS_PRESENT) {
		tx = &ccache.cover[ccache.game[actual_i].cid].tx;
		// mark used
	}
	if (state) *state = cstate;
	return tx;
}





#else // newcache



/*
sizes
2d:     64 kb  160x224 =  140 kb rgba
3d:     67 kb  176x248 =  170 kb rgba
full:  315 kb  512x340 =  680 kb rgba cmpr: 512x336/2 = 84 kb
hq:   1081 kb 1024x680 = 2720 kb rgba cmpr: 340 kb
mipmap full : 512x512 + LOD0 = 128 kb cmpr
mipmap full : 512x512 + LOD4 = 170 kb cmpr
mipmap hq : 1024x1024 + LOD5 = 682 kb cmpr

mipmap LOD 4 : 512, 256, 128, 64, 32

num of covers that fit in 25MB:
25MB / 140 = 128 2d rgba
25MB / 170 = 150 3d rgba
25MB /  84 = 304 full cmpr
25MB / 128 = 200 full mipmap lod0
25MB / 170 = 150 full mipmap lod4
25MB / 340 =  75 hq cmpr
25MB / 682 =  37 hq mipmap lod5

times: (seconds) (from SD)
load,decode,sum,covers, ms/c
3d:   1.114 + 1.228 =  2.342 / 71 =  32.9
full: 4.344 + 7.064 = 11.408 / 68 = 167.7
.ccc: 2.363 + 0     =  2.363 / 68 =  34.7
*/

// sizeof(GRRLIB_texImg) = 60


#define CACHE_STATE_NUM 500
#define LRU_MAX 0xFF

typedef struct Cover_Key
{
	u8 id[7];
	char style;   // 2d, 3d, disc, full, hq
	char format;  // RGBA, C4x4, CMPR, MIPMAP
} Cover_Key;

typedef struct Cover_State
{
	Cover_Key key;
	char used;    // 0=unused, 1=used
	char request; // request priority: 3=hi 2=med 1=lo
	char state;   // free, loading, present, missing
	char hq_available;
	u8 lru;       // least recently used
	int hnext;    // hash next
	GRRLIB_texImg tx;
} Cover_State;

struct Cover_Cache
{
	void *data;
	heap heap;
	Cover_State cstate[CACHE_STATE_NUM];
	HashTable hash;
	lwp_t lwp;
	mutex_t mutex;
	cond_t cond;
	bool stop;
	bool waiting;
	int new_request; // highest priority of a new request
	long long stat_load;
	long long stat_decode;
	int stat_n2d;
};

struct Cover_Cache ccache;
int ccache_init = 0;
int ccache_inv = 0;


#define CACHE_LOCK() LWP_MutexLock(ccache.mutex)
#define CACHE_UNLOCK() LWP_MutexUnlock(ccache.mutex)

u32 cover_key_hash(void *cb, void *key)
{
	Cover_Key *ck = key;
	return hash_string((char*)ck->id);
}

bool cover_key_cmp(void *cb, void *key, int handle)
{
	Cover_Key *k1 = key;
	Cover_Key *k2 = &ccache.cstate[handle].key;
	return memcmp(k1, k2, sizeof(*k1)) == 0;
}

int* cover_hnext(void *cb, int handle)
{
	return &ccache.cstate[handle].hnext;
}

void make_cover_key(Cover_Key *ck, u8 *id, int style, int format)
{
	memset(ck, 0, sizeof(*ck));
	strcopy((char*)ck->id, (char*)id, sizeof(ck->id));
	ck->style = style;
	ck->format = format;
}

Cover_State* cache_find_cover(u8 *id, int style, int format)
{
	Cover_Key ck;
	int i;
	make_cover_key(&ck, id, style, format);
	CACHE_LOCK();
	i = hash_get(&ccache.hash, &ck);
	CACHE_UNLOCK();
	if (i < 0) return NULL;
	return &ccache.cstate[i];
}

Cover_State* cache_find_free()
{
	int i;
	Cover_State *cs = NULL;
	for (i=0; i<CACHE_STATE_NUM; i++) {
		cs = &ccache.cstate[i];
		if (cs->used) continue;
		if (cs->request) continue;
		if (cs->state != CS_IDLE) continue;
		if (cs->key.id[0]) continue;
		return cs;
	}
	return NULL;
}

Cover_State* cache_find_lru(bool present)
{
	Cover_State *cs = NULL;
	Cover_State *lru_cs = NULL;
	Cover_State *hq_cs = NULL;
	int lru = -1;
	int hq_lru = -1;
	int hq_cnt = 0;
	int rgb_cnt = 0;
	int i;
	CACHE_LOCK();
	for (i=0; i<CACHE_STATE_NUM; i++) {
		cs = &ccache.cstate[i];
		// don't return a cover that is:
		// used or requested or being loaded
		// or the present flag doesn't match the present state
		if (cs->used) continue;
		if (cs->request) continue;
		if (cs->state == CS_LOADING) continue;
		if (present) {
			if (cs->state != CS_PRESENT) continue;
		} else {
			if (cs->state == CS_PRESENT) continue;
		}
		if (cs->lru > lru) {
			lru = cs->lru;
			lru_cs = cs;
			//if (lru >= LRU_MAX / 2) break;
		}
		if (present) {
			// limit FULL/HQ RGBA to 3
			if ( (cs->key.style == CFG_COVER_STYLE_FULL
					|| cs->key.style == CFG_COVER_STYLE_HQ)
					&& cs->key.format == CC_FMT_RGBA)
			{
				rgb_cnt++;
				if (rgb_cnt > 3) {
					lru_cs = cs;
					break;
				}
			}
			// limit HQ to 10
			if (cs->key.style == CFG_COVER_STYLE_HQ) {
				hq_cnt++;
				if (cs->lru > hq_lru) {
					hq_lru = cs->lru;
					hq_cs = cs;
				}
			}
			if (hq_cnt > 10) {
				lru_cs = hq_cs;
			}
		}
	}
	CACHE_UNLOCK();
	return lru_cs;
}

void cache_free_buf(void *ptr)
{
	CACHE_LOCK();
	if (ptr) heap_free(&ccache.heap, ptr);
	CACHE_UNLOCK();
}

#define CACHE_SAFE_FREE(ptr) if(ptr){cache_free_buf(ptr);ptr=NULL;}

void* cache_alloc_buf(int size)
{
	CACHE_LOCK();
	void *ptr = heap_alloc(&ccache.heap, size);
	CACHE_UNLOCK();
	return ptr;
}

void cache_free_cover(Cover_State *cs)
{
	int i = cs - ccache.cstate;
	CACHE_LOCK();
	hash_remove(&ccache.hash, &cs->key, i);
	CACHE_SAFE_FREE(cs->tx.data);
	memset(cs, 0, sizeof(*cs));
	CACHE_UNLOCK();
}

Cover_State* cache_free_lru(bool present)
{
	Cover_State *cs;
	cs = cache_find_lru(present);
	if (cs) {
		cache_free_cover(cs);
	}
	return cs;
}

void* cache_alloc_data(int size)
{
	void *ptr = NULL;
	CACHE_LOCK();
	do {
		ptr = cache_alloc_buf(size);
		if (!ptr) {
			// try to free
			// find lru with data
			Cover_State *cs = cache_free_lru(true);
			if (!cs) break;
		}
	} while (!ptr);
	CACHE_UNLOCK();
	if (!ptr) dbg_printf("cc: alloc err %d\n", size);
	return ptr;
}

Cover_State* cache_get_free_cover()
{
	Cover_State *cs = NULL;
	cs = cache_find_free();
	if (!cs) {
		cs = cache_free_lru(false);
	}
	if (!cs) {
		cs = cache_free_lru(true);
	}
	return cs;
}

Cover_State* cache_alloc_cover(u8 *id, int style, int format)
{
	Cover_State *cs = NULL;
	CACHE_LOCK();
	cs = cache_get_free_cover();
	if (cs) {
		make_cover_key(&cs->key, id, style, format);
		int i = cs - ccache.cstate;
		hash_add(&ccache.hash, &cs->key, i);
	}
	CACHE_UNLOCK();
	return cs;
}

// find existing mark as used and return
// or create new, set key, mark as used and return
// return NULL on error
Cover_State* cache_get_cover(u8 *id, int style, int format)
{
	Cover_State *cs = NULL;
	CACHE_LOCK();
	cs = cache_find_cover(id, style, format);
	if (!cs) {
		cs = cache_alloc_cover(id, style, format);
	}
	if (cs) {
		cs->used = 1;
	}
	CACHE_UNLOCK();
	return cs;
}

// 

struct CIMG_HDR
{
	char tag[4];
	char id[8];
	short version;
	short gxformat;
	short width;
	short height;
	short lod;
	short hq;
	short pad[4];
};

char* cache_image_path(u8 *id, char *path, int size)
{
	char *coverpath = cfg_get_covers_path(CFG_COVER_STYLE_CACHE);
	snprintf(path, size, "%s/%.6s.ccc", coverpath, (char*)id);
	return coverpath;
}

// the normal quality 512x512 mipmap is saved first followed by HQ
// so that it loads faster since HQ is used less frequently
int cache_save_image(u8 *id, GRRLIB_texImg *tx)
{
	char filepath[200];
	char *dir;
	struct CIMG_HDR hdr;
	int fd, ret;
	int size, hq_size = 0;
	u8 *data;

	if (!id || !*id || !tx || !tx->data) return -1;
	dir = cache_image_path(id, filepath, sizeof(filepath));
	dbg_printf("cc save: %s\n", filepath);
	mkdir(dir, 0777);
	fd = open(filepath, O_CREAT | O_WRONLY, 0333);
	if (fd < 0) return -1;

	memset(&hdr, 0, sizeof(hdr));
	memcpy(hdr.tag, "CIMG", 4);
	memcpy(hdr.id, id, 6);
	hdr.version = 1;
	hdr.gxformat = tx->tex_format;
	hdr.width = tx->w;
	hdr.height = tx->h;
	hdr.lod = tx->tex_lod;
	hdr.hq = 0;
	data = tx->data;
	if (tx->w > 512 && tx->tex_lod) {
		// hq
		hq_size = fixGX_GetTexBufferSize(hdr.width, hdr.height,
				hdr.gxformat, GX_FALSE, 1);
		data += hq_size;
		hdr.width /= 2;
		hdr.height /= 2;
		hdr.lod--;
		hdr.hq = 1;
	}

	ret = write(fd, &hdr, sizeof(hdr));
	if (ret != sizeof(hdr)) goto error;
	
	size = fixGX_GetTexBufferSize(hdr.width, hdr.height,
			hdr.gxformat, hdr.lod ? GX_TRUE : GX_FALSE, hdr.lod);
	ret = write(fd, data, size);
	if (ret != size) goto error;

	if (hdr.hq) {
		ret = write(fd, tx->data, hq_size);
		if (ret != hq_size) goto error;
	}
	close(fd);
	return 0;

error:
	if (fd >= 0) close(fd);
	remove(filepath);
	return -1;
}

// style: FULL OR HQ
// FULL is always loaded, HQ only if requested and available in file
// hqavail is set to what is available in the file
int cache_load_image(u8 *id, GRRLIB_texImg *tx, int style, bool *hqavail)
{
	char filepath[200];
	struct CIMG_HDR hdr;
	int fd, ret;
	int size, hq_size = 0, data_size;
	u8 *data = NULL;

	if (!id || !*id || !tx || tx->data) return -1;
	memset(tx, 0, sizeof(*tx));
	memset(&hdr, 0, sizeof(hdr));
	*hqavail = false;
	cache_image_path(id, filepath, sizeof(filepath));
	fd = open(filepath, O_RDONLY);
	if (fd < 0) return -1;
	dbg_printf("cc load: %s\n", filepath);
	ret = read(fd, &hdr, sizeof(hdr));
	if (ret != sizeof(hdr)) goto error;
	if (memcmp(hdr.tag, "CIMG", 4)) goto error;
	if (memcmp(hdr.id, id, 6)) goto error;
	if (hdr.version != 1) goto error;
	if (hdr.gxformat != GX_TF_CMPR) goto error;
	if (hdr.width > 512) goto error;
	if (hdr.height > 512) goto error;

	size = fixGX_GetTexBufferSize(hdr.width, hdr.height,
			hdr.gxformat, hdr.lod ? GX_TRUE : GX_FALSE, hdr.lod);

	if (style == CFG_COVER_STYLE_HQ && hdr.hq) {
		// load HQ too
		hdr.width *= 2;
		hdr.height *= 2;
		hdr.lod++;
		hq_size = fixGX_GetTexBufferSize(hdr.width, hdr.height,
				hdr.gxformat, GX_FALSE, 1);
	}

	data_size = size + hq_size;
	data = cache_alloc_data(data_size);
	if (!data) {
		close(fd);
		return -2; // alloc error
	}
	
	ret = read(fd, data + hq_size, size);
	if (ret != size) goto error;

	if (hq_size) {
		ret = read(fd, data, hq_size);
		if (ret != hq_size) goto error;
		//memset(data, 0xAA, 16000); // dbg red stripe
	}
	
	tx->data = data;
	tx->w = hdr.width;
	tx->h = hdr.height;
	tx->tex_lod = hdr.lod;
	tx->tex_format = hdr.gxformat;
	GRRLIB_FlushTex(tx);
	*hqavail = hdr.hq ? true : false;
	close(fd);
	return 0;

error:
	CACHE_SAFE_FREE(data);
	memset(tx, 0, sizeof(*tx));
	close(fd);
	return -1;
}


int cache_convert_image(u8 *id)
{
	char path[200];
	void *img = NULL;
	GRRLIB_texImg tx;
	u32 width=0, height=0;
	int gx_lod = CC_MIPMAP_LOD;
	int ret;
	memset(&tx, 0, sizeof(tx));
	ret = Gui_LoadCover_style(id, &img, false, CFG_COVER_STYLE_HQ, path);
	if (ret <= 0) return -1;
	ret = __Gui_GetPngDimensions(img, &width, &height);
	if (ret < 0) goto out;
	width = upperPower(width);
	height = upperPower(height);
	if (width > 512) gx_lod++; // +1 if HQ
	tx = Gui_LoadTexture_MIPMAP(img, width, height, gx_lod, NULL, path);
	if (!tx.data) { ret = -1; goto out; }
	ret = cache_save_image(id, &tx);
out:
	SAFE_FREE(img);
	SAFE_FREE(tx.data);
	return ret;
}

void cache_check_convert_image(u8 *id)
{
	struct stat st1, st2;
	char path1[200], path2[200];
	int ret1, ret2;
	ret1 = find_cover_path(id, CFG_COVER_STYLE_FULL, path1, sizeof(path1), &st1);
	cache_image_path(id, path2, sizeof(path2));
	ret1 = stat(path1, &st1);
	if (ret1 == 0) {
		ret2 = stat(path2, &st2);
		if (ret2 == 0 && st1.st_mtime <= st2.st_mtime) return;
		// ignore if modify time in future
		if (ret2 == 0 && st1.st_mtime > time(NULL)) return;
		cache_convert_image(id);
	}
}


void cache_cover_load(Cover_State *cs)
{
	char path[200];
	void *img = NULL;
	void *buf = NULL;
	int size;
	bool resizeToFullCover = false;
	GRRLIB_texImg tx;
	int ret;
	int state = CS_IDLE;
	long long t1, tload = 0, tdecode = 0;
	bool hqavail = false;
	Cover_State *csfull = NULL;
	memset(&tx, 0, sizeof(tx));

	if (cs->key.style == CFG_COVER_STYLE_HQ) {
		//sleep(2);//dbg
		// if FULL is present it will have the info if HQ is available
		CACHE_LOCK();
		csfull = cache_find_cover(cs->key.id, CFG_COVER_STYLE_FULL, cs->key.format);
		if (csfull) hqavail = csfull->hq_available;
		CACHE_UNLOCK();
		if (csfull && !hqavail) {
			state = CS_MISSING;
			goto out;
		}
	}

	// use cached converted image
	if (cs->key.style == CFG_COVER_STYLE_FULL || cs->key.style == CFG_COVER_STYLE_HQ) {
		if (cs->key.format == CC_FMT_MIPMAP) {
			t1 = gettime();
			cache_check_convert_image(cs->key.id);
			tdecode = diff_usec(t1, gettime());
			t1 = gettime();
			ret = cache_load_image(cs->key.id, &tx, cs->key.style, &hqavail);
			tload = diff_usec(t1, gettime());
			if (ret == 0 && tx.data) goto out;
			if (ret == -2) {
				// alloc error
				state = CS_IDLE;
				goto out;
			}
		}
	}
	
	t1 = gettime();
	//load the cover image
	ret = Gui_LoadCover_style((u8*)cs->key.id, &img, false, cs->key.style, path);
	tload = diff_usec(t1, gettime());
	//cc_dbg("cc ld: %.6s %d %d = %d %p\n",
	//		cs->key.id, cs->key.style, cs->key.format, ret, img);
	if (ret <= 0 && cs->key.style == CFG_COVER_STYLE_FULL) {
		//try to load the 2D cover
		;overlay_2d:;
		tload = 0;
		SAFE_FREE(img);
		ret = Gui_LoadCover_style((u8*)cs->key.id, &img, true, CFG_COVER_STYLE_2D, path);
		//cc_dbg("cc ld 2d: %.6s %d %d = %d %p\n",
		//		cs->key.id, cs->key.style, cs->key.format, ret, img);
		resizeToFullCover = true;
	}
	if (ret <= 0 || !img) {
		state = CS_MISSING;
		goto out;
	}
	if (resizeToFullCover) {
		int cover_width_front = (int)(tx_nocover_full.h / 1.4) >> 2 << 2;
		size = tx_nocover_full.w * tx_nocover_full.h * 4;
		if (CFG.gui_compress_covers) {
			size /= 8;
		}
		buf = cache_alloc_data(size);
		if (!buf) {
			state = CS_IDLE;
			goto out;
		}
		// XXX format depends on CFG.gui_compress_covers
		// needs to be an arg.
		tx = Gui_LoadTexture_fullcover(img,
				tx_nocover_full.w, tx_nocover_full.h,
				cover_width_front, buf, path);
		if (!tx.data) {
			CACHE_SAFE_FREE(buf);
		} else {
			ccache.stat_n2d++;
		}
	} else {
		u32 width=0, height=0;
		// Get image dimensions
		ret = __Gui_GetPngDimensions(img, &width, &height);
		if (ret < 0) goto invalid_img;
		//cc_dbg("cc sz %d x %d / %d x %d\n", width, height, COVER_WIDTH, COVER_HEIGHT);
		if (width > 512) hqavail = true;
		if (cs->key.style == CFG_COVER_STYLE_FULL) {
			// scale down HQ if FULL requested
			if (width > 512) {
				width = 512;
				height = 340;
				//if (cs->key.format == CC_FMT_CMPR) height = 336;
			}
			//if (width > 64) { width = 64; height = 64; } // dbg
		}
		int gx_fmt = GX_TF_RGBA8;
		int gx_mipmap = GX_FALSE;
		int gx_lod = 0;
		switch (cs->key.format) {
			case CC_FMT_CMPR:
				gx_fmt = GX_TF_CMPR;
				// 512x512 CMPR looks better than 512x340
				// but is slower - old AA with 512x512 is too slow
				//height = width;
				break;
			case CC_FMT_MIPMAP:
				gx_fmt = GX_TF_CMPR;
				gx_mipmap = GX_TRUE;
				gx_lod = CC_MIPMAP_LOD;
				width = upperPower(width);
				height = upperPower(height);
				if (width > 512) gx_lod++; // +1 if HQ
				break;
		}
		size = fixGX_GetTexBufferSize(width, height, gx_fmt, gx_mipmap, gx_lod);
		buf = cache_alloc_data(size);
		if (!buf) {
			state = CS_IDLE;
			goto out;
		}
		t1 = gettime();
		switch (cs->key.format) {
			default:
			case CC_FMT_C4x4:
				tx = Gui_LoadTexture_RGBA8(img, width, height, buf, path);
				break;
			case CC_FMT_CMPR:
				tx = Gui_LoadTexture_CMPR(img, width, height, buf, path);
				break;
			case CC_FMT_MIPMAP:
				tx = Gui_LoadTexture_MIPMAP(img, width, height, gx_lod, buf, path);
				break;
		}
		tdecode = diff_usec(t1, gettime());
		//cc_dbg("cc tx %d (%p %d %d %p) = %p\n", cs->key.format,
		//		img, COVER_WIDTH, COVER_HEIGHT, buf, tx.data);
		if (!tx.data) {
			;invalid_img:;
			tdecode = 0;
			state = CS_MISSING;
			CACHE_SAFE_FREE(buf);
			// corrupted image or out of mem?
			dbg_printf("cc error %.6s\n", cs->key.id);
			if (cs->key.style == CFG_COVER_STYLE_FULL) {
				// try to load the 2D cover
				goto overlay_2d;
			}
		}
	}
out:
	SAFE_FREE(img);
	CACHE_LOCK();
	if (tx.data) {
		state = CS_PRESENT;
		if (cs->key.style == CFG_COVER_STYLE_HQ && !hqavail) {
			// HQ requested but only full found
			// if full exists then discard currently loaded
			// else store as full and mark HQ as not available
			csfull = cache_find_cover(cs->key.id, CFG_COVER_STYLE_FULL, cs->key.format);
			if (!csfull) {
				csfull = cache_get_cover(cs->key.id, CFG_COVER_STYLE_FULL, cs->key.format);
				csfull->used = 0;
			}
			if (csfull) {
				if (csfull->state != CS_PRESENT) {
					csfull->state = CS_PRESENT;
					csfull->tx = tx;
					csfull->request = 0;
					buf = NULL;
				}
				csfull->hq_available = false;
			}
			CACHE_SAFE_FREE(buf);
			memset(&tx, 0, sizeof(tx));
			state = CS_MISSING;
		}
	}
	cs->state = state;
	cs->request = 0;
	cs->tx = tx;
	cs->hq_available = hqavail;
	if (tload && tx.data) {
		ccache.stat_load += tload;
		ccache.stat_decode += tdecode;
	}
	CACHE_UNLOCK();
	if (tx.data) cc_dbg("cc load: %.6s %d %d = %p %d\n",
			cs->key.id, cs->key.style, cs->key.format, tx.data, state);
}

void cache_process_requests()
{
	Cover_State *cs;
	int prio;
	int i;
	do {
		ccache.new_request = 0;
		for (prio = 3; prio > 0; prio--) {
			;restart:;
			for (i=0; i<CACHE_STATE_NUM; i++) {
				if (ccache.stop) break;
				cs = &ccache.cstate[i];
				CACHE_LOCK();
				if (cs->request == prio) {
					cs->state = CS_LOADING;
				}
				CACHE_UNLOCK();
				if (cs->state == CS_LOADING) {
					// load
					//sleep(1);//dbg
					cache_cover_load(cs);
					// need to restart loop at higher priority?
					if (ccache.new_request > prio) {
						//cc_dbg("new rq %d %d\n", ccache.new_request, prio);
						prio = ccache.new_request;
						ccache.new_request = 0;
						goto restart;
					}
				}
			}
		}
		//if (ccache.new_request) cc_dbg("new rq %d\n", ccache.new_request);
	} while (ccache.new_request);
}

void* cache_thread(void *arg)
{
	cc_dbg("cc started\n");
	while (1) {
		// lock
		CACHE_LOCK();
		if (!ccache.stop && ccache.new_request == 0) {
			cc_dbg("cc waiting\n");
			ccache.waiting = true;
			// LWP_CondWait unlocks mutex on enter
			LWP_CondWait(ccache.cond, ccache.mutex);
			// LWP_CondWait locks mutex on exit
			ccache.waiting = false;
			cc_dbg("cc -> running\n");
		}
		// unlock
		CACHE_UNLOCK();
		if (ccache.stop) {
			break;
		}
		cache_process_requests();
	}
	return NULL;
}

int Cache_Init()
{
	if (ccache_init) return 0;
	int ret;
	ccache.data = mem2_alloc(COVER_CACHE_DATA_SIZE);
	if (!ccache.data) return -1;
	heap_init(&ccache.heap, ccache.data, COVER_CACHE_DATA_SIZE);
	hash_init(&ccache.hash, 0, NULL, cover_key_hash, cover_key_cmp, cover_hnext);
	ccache.lwp   = LWP_THREAD_NULL;
	ccache.mutex = LWP_MUTEX_NULL;
	ccache.cond  = LWP_COND_NULL;
	LWP_MutexInit(&ccache.mutex, true);
	LWP_CondInit(&ccache.cond);
	// start thread
	dbg_printf("cc. start\n");
	ret = LWP_CreateThread(&ccache.lwp, cache_thread, NULL, NULL, 32*1024, 40);
	if (ret < 0) {
		return -1;
	}
	//cc_dbg("lwp ret: %d id: %x\n", ret, ccache.lwp);
	ccache_init = 1;
	return 0;
}

void Cache_Wakeup()
{
	//cc_dbg("cc wakeup\n");
	CACHE_LOCK();
	bool waiting = ccache.waiting;
	CACHE_UNLOCK();
	// wake
	if (waiting) {
		cc_dbg("cc signal\n");
		LWP_CondSignal(ccache.cond);
	}
}

void Cache_Close()
{
	dbg_printf("cc close\n");
	if (!ccache_init) return;
	if (ccache.lwp == LWP_THREAD_NULL) return;
	CACHE_LOCK();
	ccache.stop = true;
	CACHE_UNLOCK();
	Cache_Wakeup();
	// wait for exit and cleanup
	LWP_JoinThread(ccache.lwp, NULL);
	LWP_MutexDestroy(ccache.mutex);
	LWP_CondDestroy(ccache.cond);
	hash_close(&ccache.hash);
	//heap_close
	SAFE_FREE(ccache.data);
	memset(&ccache, 0, sizeof(ccache));
	ccache.lwp = LWP_THREAD_NULL;
	ccache_init = 0;
	dbg_printf("cc stop\n");
}

GRRLIB_texImg* cache_request_cover1(int game_index, int style, int format, int priority, int *state)
{
	Cover_State *cs = NULL;
	GRRLIB_texImg *tx = NULL;
	bool need_wake = false;
	u8 *id = NULL;
	if (game_index < 0 || game_index >= gameCnt) goto error;
	if (style > CFG_COVER_STYLE_HQ) goto error;
	id = gameList[game_index].id;
	//cc_dbg("cc req: %d %.6s %d %d %d\n", game_index, id, style, format, priority);
	if (priority) {
		cs = cache_get_cover(id, style, format);
		if (!cs) goto error;
		// cs will already be marked as used by cache_get_cover
	} else {
		// if priority == CC_PRIO_NONE then
		// return the cover only if already loaded
		// but don't updte LRU
		CACHE_LOCK();
		cs = cache_find_cover(id, style, format);
		if (cs) cs->used = 1;
		CACHE_UNLOCK();
		if (!cs) {
			if (state) *state = CS_IDLE;
			return NULL;
		}
			
	}
	if (priority > 0) {
		CACHE_LOCK();
		if (cs->state == CS_IDLE) {
			if (cs->request == 0) {
				need_wake = true;
				//cc_dbg("cc rq: %d %.6s %d %d %d\n", game_index, id, style, format, priority);
			}
			if (priority > cs->request) {
				cs->request = priority;
				// mark new request
				if (priority > ccache.new_request) {
					ccache.new_request = priority;
				}
			}
		}
		cs->lru = 0;
		CACHE_UNLOCK();
	}
	if (state) *state = cs->state;
	tx = &cs->tx;
	if (need_wake) Cache_Wakeup();
	//cc_dbg("cc req = %d %p\n", cs->state, tx);
	return tx;
error:
	// out of cover slots or invalid index
	if (state) *state = CS_ERROR;
	return NULL;
}

void cache_remap_style_fmt(int *cstyle, int *fmt)
{
	switch (*cstyle) {
		case CFG_COVER_STYLE_HQ_OR_FULL_RGB:
			*cstyle = CFG_COVER_STYLE_HQ_OR_FULL;
			*fmt = CC_FMT_C4x4;
			break;
		case CFG_COVER_STYLE_FULL_RGB:
			*cstyle = CFG_COVER_STYLE_FULL;
			*fmt = CC_FMT_C4x4;
			break;
		case CFG_COVER_STYLE_HQ_RGB:
			*cstyle = CFG_COVER_STYLE_HQ;
			*fmt = CC_FMT_C4x4;
			break;
		case CFG_COVER_STYLE_FULL_CMPR:
			*cstyle = CFG_COVER_STYLE_FULL;
			*fmt = CC_FMT_CMPR;
			break;
		case CFG_COVER_STYLE_HQ_CMPR:
			*cstyle = CFG_COVER_STYLE_HQ;
			*fmt = CC_FMT_CMPR;
			break;
		case CFG_COVER_STYLE_FULL_MIPMAP:
			*cstyle = CFG_COVER_STYLE_FULL;
			*fmt = CC_FMT_MIPMAP;
			break;
		case CFG_COVER_STYLE_HQ_MIPMAP:
			*cstyle = CFG_COVER_STYLE_HQ;
			*fmt = CC_FMT_MIPMAP;
			break;
	}
}

GRRLIB_texImg* cache_request_cover(int game_index, int style, int format, int priority, int *state)
{
	GRRLIB_texImg *tx;
	int my_state;
	cache_remap_style_fmt(&style, &format);
	if (style == CFG_COVER_STYLE_HQ_OR_FULL) {
		// HQ or FULL requested, try HQ first:
		tx = cache_request_cover1(game_index, CFG_COVER_STYLE_HQ,
				format, priority, &my_state);
		if (my_state == CS_PRESENT) {
			// HQ present: return it
			if (state) *state = my_state;
			return tx;
		}
		if ((my_state == CS_IDLE && priority > 0) || my_state == CS_LOADING) {
			// HQ requested or loading: return FULL if present
			// but don't add a request for FULL (CC_PRIO_NONE)
			return cache_request_cover1(game_index, CFG_COVER_STYLE_FULL,
					format, CC_PRIO_NONE, state);
		}
		// HQ not found: request FULL
		style = CFG_COVER_STYLE_FULL;
	}
	return cache_request_cover1(game_index, style, format, priority, state);
}

/**
 * This method is used to set the caching priority of the passed in game index
 * as well as the next x number of games.
 *
 * @param game_i the starting game index
 * @param num the number of images to set the priority on
 * @param rq_prio the cache priority: 1 = high priority
 */
void cache_request(int game_i, int num, int rq_prio)
{
	int i;
	//cc_dbg("cc_req(%d %d %d)\n", game_i, num, rq_prio);
	for (i = game_i; i < game_i + num; i++) {
		cache_request_cover(i, CFG.cover_style, CC_FMT_C4x4, CC_PRIO_HIGH, NULL);
	}
}


/**
 * This method is used to set the caching priority of the passed in game index
 * as well as the next x number of games before and after the index.  All the
 * rest of the covers are given a lower priority.
 *
 * @param game_i the starting game index
 * @param num the number of images before and after the game_i to set the priority on
 * @param rq_prio the cache priority: 1 = high priority
 */
void cache_request_before_and_after(int game_i, int num, int rq_prio)
{
	int i;
	int format = CC_FMT_C4x4;
	if (CFG.gui_compress_covers) format = CC_COVERFLOW_FMT;
	//cc_dbg("cc_rqba(%d %d %d)\n", game_i, num, format);
	if (showingFrontCover) {
		cache_request_cover(game_i, CFG_COVER_STYLE_FULL, format, CC_PRIO_HIGH, NULL);
	} else {
		cache_request_cover(game_i, CFG_COVER_STYLE_HQ_OR_FULL, format, CC_PRIO_HIGH, NULL);
	}
	for (i = game_i - num; i < game_i + num; i++) {
		if (i == game_i) continue;
		cache_request_cover(i, CFG_COVER_STYLE_FULL, format, CC_PRIO_MED, NULL);
	}
	num += 5;
	for (i = game_i - num; i < game_i + num; i++) {
		if (i == game_i) continue;
		cache_request_cover(i, CFG_COVER_STYLE_FULL, format, CC_PRIO_LOW, NULL);
	}
}

void cache_release_cover(int game_index, int style, int format)
{
	Cover_State *cs = NULL;
	u8 *id = gameList[game_index].id;
	cs = cache_get_cover(id, style, format);
	if (cs) {
		CACHE_LOCK();
		cs->used = 0;
		if (cs->request > 0) cs->request--;
		if (cs->lru < LRU_MAX) cs->lru++;
		CACHE_UNLOCK();
	}
}

void cache_release_full(bool full)
{
	Cover_State *cs;
	int i;
	CACHE_LOCK();
	for (i=0; i<CACHE_STATE_NUM; i++) {
		cs = &ccache.cstate[i];
		cs->used = 0;
		if (full) {
			// completely release
			cs->request = 0;
		} else {
			// decrease priority
			if (cs->request > 0) cs->request--;
		}
		if (cs->lru < LRU_MAX) cs->lru++;
	}
	CACHE_UNLOCK();
}

void cache_release_all()
{
	cache_release_full(false);
}

void Cache_Invalidate()
{
}

void cache_wait_idle()
{
	cache_release_full(true);
	while (!ccache.waiting) {
		LWP_YieldThread();
		usleep(1000);
	}
}

void Cache_Invalidate_Cover(u8 *id, int style)
{
	int i;
	bool do_free;
	if (!ccache_init) return;
	Cover_State *cs;
	cache_wait_idle();
	CACHE_LOCK();
	for (i = 0; i < CACHE_STATE_NUM; i++) {
		cs = &ccache.cstate[i];
		if (strcmp((char*)cs->key.id, (char*)id) == 0) {
			do_free = false;
			if (cs->key.style == style) do_free = true;
			if (cs->key.style == CFG_COVER_STYLE_FULL
					&& (style == CFG_COVER_STYLE_2D || style == CFG_COVER_STYLE_HQ))
			{
				do_free = true;
			}
			if (do_free) cache_free_cover(cs);
		}
	}
	if (style == CFG_COVER_STYLE_FULL || style == CFG_COVER_STYLE_HQ) {
		cache_check_convert_image(id);
	}
	CACHE_UNLOCK();
}

void cache_num_game()
{
}

void cache_stats(char *str, int size)
{
	int i;
	int present = 0;
	int missing = 0;
	int nfree = 0;
	int nstyle[5];
	int style;
	memset(nstyle, 0, sizeof(nstyle));
	Cover_State *cs;
	for (i = 0; i < CACHE_STATE_NUM; i++) {
		cs = &ccache.cstate[i];
		if (cs->state == CS_MISSING) missing++;
		if (cs->state == CS_PRESENT) {
			present++;
			style = cs->key.style;
			if (style >= 0 && style < 5) {
				nstyle[style]++;
			}
			
		}
	}
	heap_stats hs;
	heap_stat(&ccache.heap, &hs);
#define MBf (1024.0 * 1024.0)
	nfree = CACHE_STATE_NUM - (present + missing);
	snprintf(str, size,
			"ccache p: %d m: %d f: %d / %d\n"
			"cc mem: s:%.1f u:%.1f f:%.1f [%d,%d]\n"
			"cc tm: load: %.3f decode: %.3f\n"
			"cc 2d:%d 3d:%d d:%d f:%d hq:%d f2d:%d\n",
			present, missing, nfree, CACHE_STATE_NUM,
			hs.size / MBf, hs.used / MBf, hs.free / MBf,
			ccache.heap.used_list.num,
			ccache.heap.free_list.num,
			(float)ccache.stat_load / 1000000.0,
			(float)ccache.stat_decode / 1000000.0,
			nstyle[0], nstyle[1], nstyle[2], nstyle[3], nstyle[4],
			ccache.stat_n2d
			);
}

#endif

//****************************************************************************
//****************************  END CACHE CODE  ******************************
//****************************************************************************
