// by usptactical

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <math.h>
#include <ogcsys.h>
#include <ogc/conf.h>
#include <gccore.h>
#include <fat.h>
#include <wiiuse/wpad.h>
#include "coverflow.h"
#include "cfg.h"
#include "gui.h"
#include "cache.h"
#include "wpad.h"
#include "my_GRRLIB.h"
#include "xml.h"
#include "gettext.h"

//defines for the mouseover detection
#define MOUSEOVER_COVER_OFFSCREEN 0xFFFFFFFF

//defines for default background alpha levels
#define BACKGROUND_REFLECTION_ALPHA 0xFFFFFF44
#define BACKGROUND_REFLECTION_ALPHA2 0xFFFFFFAA

//defines for how many Cover structs to allocate
#define MAX_COVERFLOW_COVERS 10
#define MAX_COVERFLOW_COVERS_TOTAL 12

//maximum length of the drawn title text
#define TITLE_MAX 54

//position at which the wiimote starts scrolling right/left on x axis
#define WIIMOTE_SIDE_SCROLL_WIDTH 100

//defines used for setting final cover positioning during transitions
#define COVERPOS_FLIP_TO_BACK 0
#define COVERPOS_CENTER_THEME 1
#define COVERPOS_CONSOLE_2D 2
#define COVERPOS_CONSOLE_3D 3
#define COVERPOS_GAME_LOAD 4

//defines for fading the background (for flip to back)
#define BACKGROUND_FADE_NONE 0
#define BACKGROUND_FADE_DARK 1
#define BACKGROUND_FADE_LIGHT 2

//easing type used for the last rotation in a right/left transition
#define EASING_TYPE_SLOW 3
#define EASING_TYPE_LINEAR 0

#define DML_MAGIC 0x444D4C00
#define DML_MAGIC_HDD DML_MAGIC + 1

extern struct discHdr *gameList;
extern s32 gameCnt;
extern bool loadGame;
extern bool suppressCoverDrawing;

//AA
extern struct GRRLIB_texImg aa_texBuffer[4];

bool coverflow_test_grid = false;

struct settings_cf_global CFG_cf_global;
struct settings_cf_theme CFG_cf_theme[CF_THEME_NUM];

//struct for the cover coords and color levels
typedef struct CoverPos {
	float x,y,z;			// screen co-ordinates 
	float xrot, yrot, zrot;	// rotation
	int alpha;				// alpha level of cover
	u32 reflection_bottom;	// color of the bottom portion of the reflection (bottom of the cover box)
	u32 reflection_top;		// color of the top portion of the reflection (top of the cover box)
} CoverPos;

//struct to store the game cover properties
typedef struct Cover {
	CoverPos themePos;
	CoverPos startPos;
	CoverPos endPos;
	CoverPos currentPos;
	int gi;					// game index
	int state;				// current loading state of the cover image
	bool selected;			// set to true if cover is being pointed at
	u32 stencil_color;		// the stencil cover (for pointer-over detection)
	int hidden;				// is cover is hidden?
	GRRLIB_texImg tx;		// cover image data
} Cover;

Cover coverCoords_center;
Cover coverCoords_left[MAX_COVERFLOW_COVERS_TOTAL];
Cover coverCoords_right[MAX_COVERFLOW_COVERS_TOTAL];

extern unsigned char cover_top[];
extern unsigned char cover_front[];
extern unsigned char cover_side[];
extern unsigned char coverImg_full[];

GRRLIB_texImg *tx_cover_top;
GRRLIB_texImg *tx_cover_front;
GRRLIB_texImg *tx_cover_side;
GRRLIB_texImg tx_nocover_full_CMPR;
GRRLIB_texImg tx_hourglass_full;
GRRLIB_texImg tx_screenshot;

GXTexObj texCoverTop;
GXTexObj texCoverFront;
GXTexObj texCoverSide;

WPADData wpad_data;

static int background_fade_status = 0;
static int grx_cover_init = 0;
static int cover_init = 0;
bool showingFrontCover = true;
static bool spinCover_dpad_used = false;
u32 selectedColor;

//vars for the floating cover functionality
f32 float_speed_increment = 0.03;
f32 float_max_xrot = 5; //10
f32 float_max_yrot = 5; //10
f32 float_max_zrot = 3; //3
f32 float_xrot;
f32 float_yrot;
f32 float_zrot;
bool float_xrot_up = true;
bool float_yrot_up = false;
bool float_zrot_up = true;

static int rotation_start_index = 0;
int rotating_with_wiimote = 0;

Mtx GXview2D;


/**
 * Retrieves the current data for wiimote 1
 *  @return void
 */
#if 0
void getWiiMoteInfo() {
	u32 ext;	//Extension type
	//u32 ret=0;	//remote probe return

	WPAD_ScanPads();
	ret=WPAD_Probe(WPAD_CHAN_0,&ext);			//probe remote 1 with extension
	WPADData *Data = WPAD_Data(WPAD_CHAN_0);	//store data from remote 1
	wpad_data = *Data;
	
	WPAD_IR(WPAD_CHAN_0, &wpad_data.ir);				//get IR data
	WPAD_Orientation(WPAD_CHAN_0, &wpad_data.orient);	//get rotation data
	WPAD_GForce(WPAD_CHAN_0, &wpad_data.gforce);		//get "speed" data
	WPAD_Accel(WPAD_CHAN_0, &wpad_data.accel);			//get accelerometer data
	WPAD_Expansion(WPAD_CHAN_0, &wpad_data.exp);		//get expansion data
}
#endif

/**
 * Initilaizes all the common cover images (top, side, front, back)
 *  @return void
 */
void Coverflow_Grx_Init() {
	GRRLIB_texImg tx_tmp;
	showingFrontCover = true;

	if (!grx_cover_init) {
		tx_cover_top = GRRLIB_LoadTexture(cover_top);
		tx_cover_front = GRRLIB_LoadTexture(cover_front);
		tx_cover_side = GRRLIB_LoadTexture(cover_side);

		GX_InitTexObj(&texCoverTop,
				tx_cover_top->data, tx_cover_top->w, tx_cover_top->h,
				GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_InitTexObj(&texCoverFront,
				tx_cover_front->data, tx_cover_front->w, tx_cover_front->h,
			   	GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_InitTexObj(&texCoverSide,
				tx_cover_side->data, tx_cover_side->w, tx_cover_side->h,
				GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);

		if (CFG.gui_compress_covers) {
			//store a CMPR version of the full cover
			tx_tmp = Gui_LoadTexture_CMPR(coverImg_full, 0, 0, NULL, NULL);
			GRRLIB_TextureMEM2(&tx_nocover_full_CMPR, &tx_tmp);
		}
	}
	grx_cover_init = 1;
}


/**
 * Frees image resources
 *  @return void
 */
void Coverflow_Close() {
	if (grx_cover_init) {
		GRRLIB_FREE_TEX(tx_cover_top);
		GRRLIB_FREE_TEX(tx_cover_front);
		GRRLIB_FREE_TEX(tx_cover_side);
	}
}


/**
 * Sets the image load state, and if the image is in the cache it adds it to
 * the cover struct.
 *
 *  @param *cover Cover struct representing a game
 *  @return void
 */
void update_cover_state2(Cover *cover, int cstyle)
{
	int gi = cover->gi;
	
	//make sure this is a valid cover index
	if (gi < 0) return;
	
	//set the current image state for this cover
	GRRLIB_texImg *tx;
	int fmt = CC_COVERFLOW_FMT;
	tx = cache_request_cover(gi, cstyle, fmt, CC_PRIO_MED, &cover->state);
	if (cover->state == CS_PRESENT) {
		cover->tx = *tx;
	} else if (cover->state == CS_IDLE) {
		cover->tx = tx_hourglass_full;
	} else if (cover->state == CS_LOADING) {
		cover->tx = tx_hourglass_full;
	} else { // CS_MISSING
		if (CFG.gui_compress_covers)
			cover->tx = tx_nocover_full_CMPR;
		else
			cover->tx = tx_nocover_full;
	}
}

void update_cover_state(Cover *cover)
{
	update_cover_state2(cover, CFG_COVER_STYLE_FULL);
}

/**
 * Checks the loading status of the covers.
 *  @return void
 */
void Coverflow_update_state() {
	int i;
	struct Cover *cover;

	//center cover
	if (showingFrontCover) {
		update_cover_state(&coverCoords_center);
	} else {
		update_cover_state2(&coverCoords_center, CFG_COVER_STYLE_HQ_OR_FULL);
	}

	//check left side
	for (i=0; i < CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers + 1; i++) {
		cover = &coverCoords_left[i];
		update_cover_state(cover);
	}

	//check right side
	for (i=0; i < CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers + 1; i++) {
		cover = &coverCoords_right[i];
		update_cover_state(cover);
	}
}


/**
 * Sets the projection matrix for 2D display
 *  @return void
 */
void set2DProjectionMatrix() {
	Mtx44 GXprojection2D;
	
	guMtxIdentity(GXmodelView2D);
	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -50.0F);
	GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);
	guOrtho(GXprojection2D, 0, 480, 0, 640, 0, 300.0F);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC);
}


/**
 * Used to set the camera position 
 *  @param aaStep int position in the array to store the created tex
 *  @return void
 */
void set_camera() {
	//set the view
	guVector	cam = {CFG_cf_theme[CFG_cf_global.theme].cam_pos_x, 
				CFG_cf_theme[CFG_cf_global.theme].cam_pos_y, 
				CFG_cf_theme[CFG_cf_global.theme].cam_pos_z},
				
			up = {0.0F, 1.0F, 0.0F},

			look = {CFG_cf_theme[CFG_cf_global.theme].cam_look_x, 
				CFG_cf_theme[CFG_cf_global.theme].cam_look_y, 
				CFG_cf_theme[CFG_cf_global.theme].cam_look_z};

	guLookAt(GXview2D, &cam, &up, &look);
}


//Easing routines from http://www.robertpenner.com/easing/
// t = time
// c = change in position
// d = duration
// b = beginning position
float linearOut(float t, float b , float c, float d) {
	return c*t/d + b;
}

float easeOutQuad(float t, float b , float c, float d) {
	t /= d;
	return -c * t*(t-2) + b;
}

float easeOutCubic(float t, float b , float c, float d) {
	t = t/d - 1;
	return c*(t*t*t + 1) + b;
}

float easeOutQuart(float t, float b , float c, float d) {
	t = t/d - 1;
	return -c * (t*t*t*t - 1) + b;
}

float easeOutQuint(float t, float b , float c, float d) {
	t = t/d - 1;
	return c*(t*t*t*t*t + 1) + b; //Quint
}

float easeOutExpo(float t, float b , float c, float d) {
	return (t==d) ? b+c : c * (-pow(2, -10 * t/d) + 1) + b;
}

float ease_x(int type, float t, float b, float c, float d)
{
	switch (type) {
		default:
		case 0: return linearOut(t, b, c, d);
		case 1: return easeOutQuad(t, b, c, d);
		case 2: return easeOutCubic(t, b, c, d);
		case 3: return easeOutQuart(t, b, c, d);
		case 4: return easeOutQuint(t, b, c, d);
		case 5: return easeOutExpo(t, b, c, d);
	}
}


/**
 * Method used to calculate the new position when moving an object.  Can be used for moving on any axis (x,y,z)
 *	E.g. moving a cover from the right side to the center.
 *	The basic equation is: 
 *		current pos = starting point - ((starting point - end point) * index / number of total frames)
 *
 *  @param startPos f32 representing the original starting point of the object
 *  @param endPos f32 representing the ending point (where you want the object to ultimately end up).
 *  @param index int representing the current index in the loop
 *  @param totalFrames int representing the total number of frames in your loop
 *  @param ease_type int representing the type of easing to use in the calculation
 *  @return f32 next position
 */
f32 calculateNewPosition(f32 startPos, f32 endPos, int index, int totalFrames, int ease_type) {
	if (index >= totalFrames || totalFrames==0 || startPos==endPos)
		return endPos;
	switch (ease_type) {
		case 0: return linearOut(index, startPos, endPos - startPos, totalFrames);
		case 1: return easeOutQuad(index, startPos, endPos - startPos, totalFrames);
		case 2: return easeOutCubic(index, startPos, endPos - startPos, totalFrames);
		case 3: return easeOutQuart(index, startPos, endPos - startPos, totalFrames);
		case 4: return easeOutQuint(index, startPos, endPos - startPos, totalFrames);
		case 5: return easeOutExpo(index, startPos, endPos - startPos, totalFrames);
	}
	return endPos;
}


/**
 * Method used to calculate the new alpha level when moving a color.
 *
 *  @param startPos u32 representing the original starting color of the object
 *  @param endPos u32 representing the ending color (where you want the color to ultimately end up).
 *  @param index int representing the current index in the loop
 *  @param totalFrames into representing the total number of frames in your loop
 *  @param ease_type int representing the type of easing to use in the calculation
 *  @return u32 next color in the iteration
 */
u32 calculateNewColor(u32 startPos, u32 endPos, int index, int totalFrames, int ease_type)
{
	u32 color;
	int a, a1, a2;
	int i;

	if (index > totalFrames || totalFrames==0 || startPos==endPos)
		return endPos;
	
	// each component
	color = 0;
	for (i=0; i<4; i++) {
		a1 = (startPos >> (i*8)) & 0xFF;
		a2 = (endPos >> (i*8)) & 0xFF;
		a = ease_x(ease_type, index, a1, a2 - a1, totalFrames);
		// range check, just in case..
		if (a < 0) a = 0;
		if (a > 0xFF) a = 0xFF;
		color |= (a << (i*8));
	}
	
	return color;
}

/**
 * Initializes the cover image layout in the current CFG_cf_theme[CFG_cf_global.theme].
 *  @param coverCount total number of covers
 *	@param index represents the cover index of the center cover
 *  @return int of the selected cover
 */
int setCoverIndexing(int coverCount, int index) {
	int i, img, selectedCover;

	img = index;
	//set the center to the first cover if the index is bad
	if (img < 1 || img >= coverCount) 
		img = 0;
	selectedCover = img;
	
	//set the right covers
	coverCoords_center.gi = img;
	img++;
	if (img >= coverCount)
		img = 0;
	for (i=0; i < CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers + 1; i++) {
		coverCoords_right[i].gi = img;
		img++;
		if (img >= coverCount)
			img = 0;
	}

	//set the left covers
	img = selectedCover-1;
	if (img < 0)
		img = coverCount - 1;
	if (img < 0) img = 0;
	for (i=0; i < CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers + 1; i++) {
		coverCoords_left[i].gi = img;
		img--;
		if (img < 0)
			img = coverCount - 1;
		if (img < 0) img = 0;
	}
	return selectedCover;
}


/**
 * Converts degrees to radians:
 *  	angle = index * ((Math.PI * 2) / numOfItems);
 *	@param degree the degree to convert
 *  @return f32 representing the radian
 */
f32 degToRad (f32 degree) {
	return (degree * M_PI) / 180.f;
}


/**
 * Initializes a cover positioned to the left of center
 *	@param *cover Cover struct object of the cover to set up
 *	@param index represents the cover's index in the coverCoords_left array
 *  @return void
 */
void setCoverPosition_Left(CoverPos *cover, int index) {
	f32 angle_deg, angle_rad, radius;
	int alpha;

	cover->x = CFG_cf_theme[CFG_cf_global.theme].cover_left_xpos + CFG_cf_theme[CFG_cf_global.theme].cover_distance_from_center_left_x;
	cover->y = CFG_cf_theme[CFG_cf_global.theme].cover_left_ypos + CFG_cf_theme[CFG_cf_global.theme].cover_distance_from_center_left_y;
	cover->z = CFG_cf_theme[CFG_cf_global.theme].cover_left_zpos + CFG_cf_theme[CFG_cf_global.theme].cover_distance_from_center_left_z;
	
	cover->xrot = CFG_cf_theme[CFG_cf_global.theme].cover_left_xrot;
	cover->yrot = CFG_cf_theme[CFG_cf_global.theme].cover_left_yrot;
	cover->zrot = CFG_cf_theme[CFG_cf_global.theme].cover_left_zrot;

	if (CFG_cf_theme[CFG_cf_global.theme].cover_rolloff_y != 0) {
		angle_deg = CFG_cf_theme[CFG_cf_global.theme].cover_rolloff_y * index;
		radius = CFG_cf_theme[CFG_cf_global.theme].cover_distance_between_covers_left_x * index;
		//convert rolloff degree to radians
		angle_rad = degToRad(angle_deg);
		
		// x = Math.cos(mc.angle) * radiusX + centerX;
		// y = Math.sin(mc.angle) * radiusY + centerY;	
		cover->x += cosf(angle_rad) * radius;
		cover->z -= sinf(angle_rad) * radius;
		cover->yrot += angle_deg;
	} else {
		cover->x += (CFG_cf_theme[CFG_cf_global.theme].cover_distance_between_covers_left_x * index);
		cover->y += (CFG_cf_theme[CFG_cf_global.theme].cover_distance_between_covers_left_y * index);
		cover->z += (CFG_cf_theme[CFG_cf_global.theme].cover_distance_between_covers_left_z * index);
	}
	
	cover->alpha = coverCoords_center.themePos.alpha - CFG_cf_theme[CFG_cf_global.theme].alpha_rolloff * (index+1);
	if (cover->alpha < 0) {
		cover->alpha = 0;
		cover->reflection_bottom = 0x00000000;
		cover->reflection_top = 0x00000000;
	} else {
		alpha = (CFG_cf_theme[CFG_cf_global.theme].reflections_color_bottom & 0xFF) - (CFG_cf_theme[CFG_cf_global.theme].alpha_rolloff/2) * (index+1);
		if (alpha <= 0) {
			cover->reflection_bottom = 0x00000000;
		} else {
			cover->reflection_bottom = ((CFG_cf_theme[CFG_cf_global.theme].reflections_color_bottom & 0xFFFFFF00) | alpha);
		}
		//cover->reflection_bottom = CFG_cf_theme[CFG_cf_global.theme].reflections_color_bottom;
	
		alpha = (CFG_cf_theme[CFG_cf_global.theme].reflections_color_top & 0xFF) - (CFG_cf_theme[CFG_cf_global.theme].alpha_rolloff/2) * (index+1);
		if (alpha <= 0) {
			cover->reflection_top = 0x00000000;
		} else {
			cover->reflection_top = ((CFG_cf_theme[CFG_cf_global.theme].reflections_color_top & 0xFFFFFF00) | alpha);
		}
		//cover->reflection_top = CFG_cf_theme[CFG_cf_global.theme].reflections_color_top;
	}
}


/**
 * Initializes a cover positioned to the right of center
 *	@param *cover CoverPos struct object of the cover to set up
 *	@param index represents the cover's index in the coverCoords_right array
 *  @return void
 */
void setCoverPosition_Right(CoverPos *cover, int index) {
	f32 angle_deg, angle_rad, radius;
	int alpha;
	
	cover->x = CFG_cf_theme[CFG_cf_global.theme].cover_right_xpos + CFG_cf_theme[CFG_cf_global.theme].cover_distance_from_center_right_x;
	cover->y = CFG_cf_theme[CFG_cf_global.theme].cover_right_ypos + CFG_cf_theme[CFG_cf_global.theme].cover_distance_from_center_right_y;
	cover->z = CFG_cf_theme[CFG_cf_global.theme].cover_right_zpos + CFG_cf_theme[CFG_cf_global.theme].cover_distance_from_center_right_z;
	
	cover->xrot = CFG_cf_theme[CFG_cf_global.theme].cover_right_xrot;
	cover->yrot = CFG_cf_theme[CFG_cf_global.theme].cover_right_yrot;
	cover->zrot = CFG_cf_theme[CFG_cf_global.theme].cover_right_zrot;

	if (CFG_cf_theme[CFG_cf_global.theme].cover_rolloff_y != 0) {
		angle_deg = CFG_cf_theme[CFG_cf_global.theme].cover_rolloff_y * index * -1;
		radius = CFG_cf_theme[CFG_cf_global.theme].cover_distance_between_covers_right_x * index;
		//convert rolloff degree to radians
		angle_rad = degToRad(angle_deg);
		
		// x = Math.cos(mc.angle) * radiusX + centerX;
		// y = Math.sin(mc.angle) * radiusY + centerY;	
		cover->x += cosf(angle_rad) * radius;
		cover->z -= sinf(angle_rad) * radius;
		cover->yrot += angle_deg;
	} else {
		cover->x += (CFG_cf_theme[CFG_cf_global.theme].cover_distance_between_covers_right_x * index);
		cover->y += (CFG_cf_theme[CFG_cf_global.theme].cover_distance_between_covers_right_y * index);
		cover->z += (CFG_cf_theme[CFG_cf_global.theme].cover_distance_between_covers_right_z * index);
	}
	
	cover->alpha = coverCoords_center.themePos.alpha - CFG_cf_theme[CFG_cf_global.theme].alpha_rolloff * (index+1);
	if (cover->alpha < 0) {
		cover->alpha = 0;
		cover->reflection_bottom = 0x00000000;
		cover->reflection_top = 0x00000000;
	} else {
		alpha = (CFG_cf_theme[CFG_cf_global.theme].reflections_color_bottom & 0xFF) - (CFG_cf_theme[CFG_cf_global.theme].alpha_rolloff/2) * (index+1);
		if (alpha <= 0) {
			cover->reflection_bottom = 0x00000000;
		} else {
			cover->reflection_bottom = ((CFG_cf_theme[CFG_cf_global.theme].reflections_color_bottom & 0xFFFFFF00) | alpha);
		}
		//cover->reflection_bottom = CFG_cf_theme[CFG_cf_global.theme].reflections_color_bottom;
	
		alpha = (CFG_cf_theme[CFG_cf_global.theme].reflections_color_top & 0xFF) - (CFG_cf_theme[CFG_cf_global.theme].alpha_rolloff/2) * (index+1);
		if (alpha <= 0) {
			cover->reflection_top = 0x00000000;
		} else {
			cover->reflection_top = ((CFG_cf_theme[CFG_cf_global.theme].reflections_color_top & 0xFFFFFF00) | alpha);
		}
		//cover->reflection_top = CFG_cf_theme[CFG_cf_global.theme].reflections_color_top;
	}
}


/**
 * Method to set GX Position and Color
 */
void drawPosCol(f32 x, f32 y, f32 z, u32 c) {
	GX_Position3f32(x, y, z);
	GX_Color1u32(c);
}

/**
 * Method to set GX Position, Color and Texture coords
 */
void drawPosColTex(f32 x, f32 y, f32 z, u32 c, f32 tx, f32 ty) {
	GX_Position3f32(x, y, z);
	GX_Color1u32(c);
	GX_TexCoord2f32(tx, ty);
}


/**
 * Method to draw a flat 2D cover image with perspective-based (3D) viewpoint.
 *
 *  @param tex cover image to draw
 *  @param xpos x axis position for the image
 *  @param ypos y axis position for the image
 *  @param zpos z axis position for the image
 *  @param xrot degrees to rotate the image on the x axis.  For example a value of 90 will rotate it 90 degrees so it would appear flat on it's cover to the viewer.
 *  @param yrot degrees to rotate the image on the y axis.  For example a value of 90 will rotate it 90 degrees on it's side so the front will be facing right to the viewer
 *  @param zrot degrees to rotate the image.  For example a value of 90 will rotate it 90 degrees counter-clockwise
 *  @param color the color to make the cover
 *  @param reflectionColorBottom the color (including alpha) of the bottom portion of the reflection (bottom of the cover).  255 will be a solid image.  0 will be fully transparent.
 *  @param reflectionColorTop the color (including alpha) of the top portion of the reflection (top of the cover).  255 will be a solid image.  0 will be fully transparent.
 *  @param isFavorite bool represents if the favorite image should be drawn over the cover
 *  @return void
 */
void draw_2dcover_image(GRRLIB_texImg *tex, f32 xpos, f32 ypos, f32 zpos, f32 xrot, f32 yrot, f32 zrot, u32 color, u32 reflectionColorBottom, u32 reflectionColorTop, bool isFavorite) {
	Mtx44 GXprojection2D;
	//Mtx GXview2D;
	Mtx GXmodel2D;
	Mtx GXModelTemp;
	//Mtx GXmodelView2D;
	GXTexObj texObj;
	f32 width, height;
	
	guVector xaxis = (guVector) {1, 0, 0};
	guVector yaxis = (guVector) {0, 1, 0};
	guVector zaxis = (guVector) {0, 0, 1};

	//set the view
	set_camera();

	guMtxIdentity(GXmodel2D);
	GX_LoadTexMtxImm(GXmodel2D, GX_TEXMTX0, GX_MTX2x4);
	
	//get the objects size
	//width = COVER_WIDTH * 0.0625;
    //height = COVER_HEIGHT * 0.0625;
	width = 10.0;
	height = 14.0;

	//set rotation
	if (xrot != 0) {
		guMtxRotAxisDeg(GXModelTemp, &xaxis, xrot);
		guMtxConcat (GXmodel2D, GXModelTemp, GXmodel2D);
	}
	if (yrot != 0) {
		guMtxRotAxisDeg(GXModelTemp, &yaxis, yrot);
		guMtxConcat (GXmodel2D, GXModelTemp, GXmodel2D);
	}
	if (zrot != 0) {
		guMtxRotAxisDeg(GXModelTemp, &zaxis, zrot);
		guMtxConcat (GXmodel2D, GXModelTemp, GXmodel2D);
	}

	// set the XYZ position
	guMtxTransApply(GXmodel2D, GXmodel2D, xpos, ypos, zpos);
	guMtxConcat (GXview2D, GXmodel2D, GXmodelView2D);
	
	// load the modelview matrix into matrix memory
	GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

	if (CFG.widescreen) {
		guPerspective(GXprojection2D, 45, 1.778F, 0.1F, 500.0F);
	} else {
		guPerspective(GXprojection2D, 45, (f32)4/3, 0.1F, 500.0F);
	}
	GX_LoadProjectionMtx(GXprojection2D, GX_PERSPECTIVE);

	GX_SetZMode(GX_ENABLE, GX_ALWAYS, GX_TRUE);

	//set blend mode
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);

	//don't enable this or images look crappy - disables aa
	//	GX_InitTexObjLOD(&texObjects[index], GX_NEAR, GX_NEAR, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);

    GX_InitTexObj(&texObj, tex->data, tex->w, tex->h, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);

	//front blank cover
	GX_LoadTexObj(&texCoverFront, GX_TEXMAP0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
	GX_SetCullMode (GX_CULL_BACK);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosColTex(-width, height, 0, color, 0.0f, 0.0f);
		drawPosColTex(width, height, 0, color, 1.0f, 0.0f);
		drawPosColTex(width, -height, 0, color, 1.0f, 1.0f);
		drawPosColTex(-width, -height, 0, color, 0.0f, 1.0f);
	GX_End();
	
	//front cover image
	GX_LoadTexObj(&texObj, GX_TEXMAP0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
	GX_SetCullMode (GX_CULL_BACK);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosColTex(-width, height-0.5, 0, color, 0.0f, 0.0f);
		drawPosColTex(width-0.5, height-0.5, 0, color, 1.0f, 0.0f);
		drawPosColTex(width-0.5, -height+0.5, 0, color, 1.0f, 1.0f);
		drawPosColTex(-width, -height+0.5, 0, color, 0.0f, 1.0f);
	GX_End();

	//draw the favorites star
	if (isFavorite) {
		//calculate the size
		f32 star_scale = .125;
		f32 starw = tx_star.w * star_scale;
		f32 starh = tx_star.h * star_scale;
		//init the texture
		GXTexObj texStar;
		GX_InitTexObj(&texStar, tx_star.data, tx_star.w, tx_star.h, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_LoadTexObj(&texStar, GX_TEXMAP0);
		GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
		GX_SetCullMode (GX_CULL_BACK);
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			drawPosColTex(width-starw, height, 0, color, 0.0f, 0.0f);
			drawPosColTex(width, height, 0, color, 1.0f, 0.0f);
			drawPosColTex(width, height-starh, 0, color, 1.0f, 1.0f);
			drawPosColTex(width-starw, height-starh, 0, color, 0.0f, 1.0f);
		GX_End();
	}
	
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);

	if (reflectionColorBottom > 0 || reflectionColorTop > 0) {

		//Draw the reflection image upside down.  Set the RGBA color 
		//near the bottom of the cover (top of the reflection) to the passed 
		//in alpha level.  The RGBA for the lowest part of the reflection is 
		//set to zero, which gives us a fade-to-black appearance.
		GX_LoadTexObj(&texObj, GX_TEXMAP0);
		GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
		GX_SetCullMode (GX_CULL_BACK);
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			drawPosColTex(-width, -height-1, 0, reflectionColorBottom, 0.0f, 1.0f);
			drawPosColTex(width, -height-1, 0, reflectionColorBottom, 1.0f, 1.0f);
			drawPosColTex(width, -height*2.5, 0, reflectionColorTop, 1.0f, 0.0f);
			drawPosColTex(-width, -height*2.5, 0, reflectionColorTop, 0.0f, 0.0f);
		GX_End();		
	}

	GX_LoadPosMtxImm (GXview2D, GX_PNMTX0);
	//set TevOp to GX_PASSCLR so that we get the color's alpha level
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
	
	//reset the viewpoint to 2D
	set2DProjectionMatrix();
}

	
/**
 * Method to draw cover image with perspective-based (3D) viewpoint.
 *
 *  @param tex cover image to draw
 *  @param xpos x axis position for the image
 *  @param ypos y axis position for the image
 *  @param zpos z axis position for the image
 *  @param xrot degrees to rotate the image on the x axis.  For example a value of 90 will rotate it 90 degrees so it would appear flat on it's cover to the viewer.
 *  @param yrot degrees to rotate the image on the y axis.  For example a value of 90 will rotate it 90 degrees on it's side so the front will be facing right to the viewer
 *  @param zrot degrees to rotate the image.  For example a value of 90 will rotate it 90 degrees counter-clockwise
 *  @param color the color to make the cover (where the game image is)
 *  @param edgecolor the color to make the boxcover edges
 *  @param isFavorite bool represents if the favorite image should be drawn over the cover
 *  @return void
 */

// lod bias: at -1.0 looks best, lower values get slow, higher loose quality
float lod_bias_a[] = { -2.0, -1.5, -1.0, -0.5, -0.3, 0, +0.5, +1 };
int lod_bias_idx = 2;

void draw_fullcover_image(GRRLIB_texImg *tex, f32 xpos, f32 ypos, f32 zpos, f32 xrot, f32 yrot, f32 zrot, u32 color, u32 edgecolor, bool isFavorite) {
	Mtx44 GXprojection2D;
	//Mtx GXview2D;
	Mtx GXmodel2D;
	Mtx GXModelTemp;
	//Mtx GXmodelView2D;
	GXTexObj texCoverImage;
	
	//cover width and height
	f32 width = 10.0;
	f32 height = 14.0;

	//fullcover tex positioning
	f32 front_start = 0.525;
	f32 spine_start = 0.475;

	//distance between front and back covers
	f32 COVER_BOX_DEPTH = 2.0;
	//depth of the edge
	f32 COVER_EDGE_DEPTH = 0.3;
	//depth of side panel (closest / farthest wrt the viewpoint)
	f32 COVER_SIDE_NEAR_DEPTH = COVER_BOX_DEPTH - COVER_EDGE_DEPTH;
	f32 COVER_SIDE_FAR_DEPTH = COVER_EDGE_DEPTH;

	guVector xaxis = (guVector) {1, 0, 0};
	guVector yaxis = (guVector) {0, 1, 0};
	guVector zaxis = (guVector) {0, 0, 1};

	set_camera();

	guMtxIdentity(GXmodel2D);
	GX_LoadTexMtxImm(GXmodel2D, GX_TEXMTX0, GX_MTX2x4);

	//get the objects size
	//width = COVER_WIDTH * 0.0625;
    //height = COVER_HEIGHT * 0.0625;

	//set rotation
	if (xrot != 0) {
		guMtxRotAxisDeg(GXModelTemp, &xaxis, xrot);
		guMtxConcat (GXmodel2D, GXModelTemp, GXmodel2D);
	}
	if (yrot != 0) {
		guMtxRotAxisDeg(GXModelTemp, &yaxis, yrot);
		guMtxConcat (GXmodel2D, GXModelTemp, GXmodel2D);
	}
	if (zrot != 0) {
		guMtxRotAxisDeg(GXModelTemp, &zaxis, zrot);
		guMtxConcat (GXmodel2D, GXModelTemp, GXmodel2D);
	}

	// set the XYZ position
	guMtxTransApply(GXmodel2D, GXmodel2D, xpos, ypos, zpos);
	guMtxConcat (GXview2D, GXmodel2D, GXmodelView2D);
	
	// load the modelview matrix into matrix memory
	GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

	if (CFG.widescreen) {
		guPerspective(GXprojection2D, 45, 1.778F, 0.1F, 500.0F);
	} else {
		guPerspective(GXprojection2D, 45, (f32)4/3, 0.1F, 500.0F);
	}
	GX_LoadProjectionMtx(GXprojection2D, GX_PERSPECTIVE);

	GX_SetZMode(GX_ENABLE, GX_ALWAYS, GX_TRUE);
	//GX_SetZMode (GX_ENABLE, GX_LEQUAL, GX_TRUE);
	
	//set blend mode
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);

	int tx_fmt = GX_TF_RGBA8;
	if (tex->tex_format) tx_fmt = tex->tex_format;
	GX_InitTexObj(&texCoverImage, tex->data, tex->w, tex->h, tx_fmt, GX_CLAMP, GX_CLAMP, GX_FALSE);
	
	//don't enable this or images look crappy - disables aa
	//GX_InitTexObjLOD(&texCoverImage, GX_NEAR, GX_NEAR, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);
	if (tex->tex_lod) {
		// mipmap
		float lod_bias = lod_bias_a[lod_bias_idx];
		GX_InitTexObjLOD(&texCoverImage, GX_LIN_MIP_LIN, GX_LINEAR,
				0.f, // lod min
				(float)tex->tex_lod, // lod max
				lod_bias, // lod bias
				GX_FALSE,
				GX_FALSE, // edge lod
				GX_ANISO_1); //GX_ANISO_2 GX_ANISO_4); // _1 seems sharper
	}

	//-------------------------------------------------
	// draw the edges
	//-------------------------------------------------

	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
    GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetCullMode (GX_CULL_BACK);
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);

	//top front edge
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(-width, height + COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(width, height + COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(width, height, COVER_BOX_DEPTH, edgecolor);
		drawPosCol(-width, height, COVER_BOX_DEPTH, edgecolor);
	GX_End();

	//top front corner
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
		drawPosCol(width, height, COVER_BOX_DEPTH, edgecolor);
		drawPosCol(width, height + COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(width + COVER_EDGE_DEPTH, height, COVER_SIDE_NEAR_DEPTH, edgecolor);
	GX_End();

	//right front edge (opening)
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(width, height, COVER_BOX_DEPTH, edgecolor);
		drawPosCol(width + COVER_EDGE_DEPTH, height, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(width + COVER_EDGE_DEPTH, -height, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(width, -height, COVER_BOX_DEPTH, edgecolor);
	GX_End();

	//bottom front corner (opening)
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
		drawPosCol(width, -height, COVER_BOX_DEPTH, edgecolor);
		drawPosCol(width + COVER_EDGE_DEPTH, -height, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(width, -height - COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor);
	GX_End();

	//bottom front edge
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(-width, -height, COVER_BOX_DEPTH, edgecolor);
		drawPosCol(width, -height, COVER_BOX_DEPTH, edgecolor);
		drawPosCol(width, -height - COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(-width, -height - COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor);
	GX_End();

	//-------------------------------------------
	
	//top right edge (between top and opening)
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(width, height + COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(width, height + COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(width + COVER_EDGE_DEPTH, height, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(width + COVER_EDGE_DEPTH, height, COVER_SIDE_NEAR_DEPTH, edgecolor);
	GX_End();

	//bottom right edge (between bottom and opening)
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(width + COVER_EDGE_DEPTH, -height, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(width + COVER_EDGE_DEPTH, -height, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(width, -height - COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(width, -height - COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor);
	GX_End();

	//-------------------------------------------

	//top left edge (between top and spine)
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
		drawPosCol(-width, height, 0.0f, edgecolor);
		drawPosCol(-width, height + COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(-width, height, COVER_SIDE_FAR_DEPTH, edgecolor);
	GX_End();

	//top left edge (between top and spine)
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(-width, height + COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(-width, height + COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(-width, height, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(-width, height, COVER_SIDE_FAR_DEPTH, edgecolor);
	GX_End();

	//top left edge (between top and spine)
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
		drawPosCol(-width, height + COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(-width, height, COVER_BOX_DEPTH, edgecolor);
		drawPosCol(-width, height, COVER_SIDE_NEAR_DEPTH, edgecolor);
	GX_End();

	//bottom left edge (between bottom and spine)
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
		drawPosCol(-width, -height, 0.0f, edgecolor);
		drawPosCol(-width, -height, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(-width, -height - COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor);
	GX_End();

	//bottom left edge (between bottom and spine)
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(-width, -height, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(-width, -height, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(-width, -height - COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(-width, -height - COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor);
	GX_End();

	//bottom left edge (between bottom and spine)
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
		drawPosCol(-width, -height, COVER_SIDE_NEAR_DEPTH, edgecolor);
		drawPosCol(-width, -height, COVER_BOX_DEPTH, edgecolor);
		drawPosCol(-width, -height - COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor);
	GX_End();

	//-------------------------------------------
	
	//top back edge
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(width, height + COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(-width, height + COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(-width, height, 0.0f, edgecolor);
		drawPosCol(width, height, 0.0f, edgecolor);
	GX_End();

	//top back corner
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
		drawPosCol(width, height, 0.0f, edgecolor);
		drawPosCol(width + COVER_EDGE_DEPTH, height, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(width, height + COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor);
	GX_End();

	//right back edge
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(width + COVER_EDGE_DEPTH, height, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(width, height, 0.0f, edgecolor);
		drawPosCol(width, -height, 0.0f, edgecolor);
		drawPosCol(width + COVER_EDGE_DEPTH, -height, COVER_SIDE_FAR_DEPTH, edgecolor);
	GX_End();

	//bottom back corner
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
		drawPosCol(width, -height, 0.0f, edgecolor);
		drawPosCol(width, -height - COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(width + COVER_EDGE_DEPTH, -height, COVER_SIDE_FAR_DEPTH, edgecolor);
	GX_End();

	//bottom back edge
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(width, -height, 0.0f, edgecolor);
		drawPosCol(-width, -height, 0.0f, edgecolor);
		drawPosCol(-width, -height - COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor);
		drawPosCol(width, -height - COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor);
	GX_End();

	
	//--------------------------------
	// draw generic no cover image top and right side (cover opening)
	//--------------------------------

	//top
	GX_LoadTexObj(&texCoverTop, GX_TEXMAP0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
	GX_SetCullMode (GX_CULL_FRONT);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosColTex(-width, height + COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor, 0.0f, 0.0f);
		drawPosColTex(width, height + COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor, 1.0f, 0.0f);
		drawPosColTex(width, height + COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor, 1.0f, 1.0f);
		drawPosColTex(-width, height + COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor, 0.0f, 1.0f);
	GX_End();

	//right - cover opening
	GX_LoadTexObj(&texCoverSide, GX_TEXMAP0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
	GX_SetCullMode (GX_CULL_BACK);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosColTex(width + COVER_EDGE_DEPTH, height, COVER_SIDE_NEAR_DEPTH, edgecolor, 0.0f, 0.0f);
		drawPosColTex(width + COVER_EDGE_DEPTH, height, COVER_SIDE_FAR_DEPTH, edgecolor, 1.0f, 0.0f);
		drawPosColTex(width + COVER_EDGE_DEPTH, -height, COVER_SIDE_FAR_DEPTH, edgecolor, 1.0f, 1.0f);
		drawPosColTex(width + COVER_EDGE_DEPTH, -height, COVER_SIDE_NEAR_DEPTH, edgecolor, 0.0f, 1.0f);
	GX_End();

	//bottom
	GX_LoadTexObj(&texCoverTop, GX_TEXMAP0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
	GX_SetCullMode (GX_CULL_BACK);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosColTex(-width, -height - COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor, 0.0f, 0.0f);
		drawPosColTex(width, -height - COVER_EDGE_DEPTH, COVER_SIDE_NEAR_DEPTH, edgecolor, 1.0f, 0.0f);
		drawPosColTex(width, -height - COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor, 1.0f, 1.0f);
		drawPosColTex(-width, -height - COVER_EDGE_DEPTH, COVER_SIDE_FAR_DEPTH, edgecolor, 0.0f, 1.0f);
	GX_End();


	//--------------------------------
	// draw cover image
	//--------------------------------

	//Wrap the full cover
	GX_LoadTexObj(&texCoverImage, GX_TEXMAP0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
	GX_SetCullMode (GX_CULL_BACK);

	//fullcover back
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosColTex(width, height, 0.0f, color, 0.0f, 0.0f);
		drawPosColTex(-width, height, 0.0f, color, spine_start, 0.0f);
		drawPosColTex(-width, -height, 0.0f, color, spine_start, 1.0f);
		drawPosColTex(width, -height, 0.0f, color, 0.0f, 1.0f);
	GX_End();

	//fullcover spine
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosColTex(-width, height, 0.0f, color, spine_start, 0.0f);
		drawPosColTex(-width, height, COVER_BOX_DEPTH, color, front_start, 0.0f);
		drawPosColTex(-width, -height, COVER_BOX_DEPTH, color, front_start, 1.0f);
		drawPosColTex(-width, -height, 0.0f, color, spine_start, 1.0f);
	GX_End();

	//fullcover front
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosColTex(-width, height, COVER_BOX_DEPTH, color, front_start, 0.0f);
		drawPosColTex(width, height, COVER_BOX_DEPTH, color, 1.0f, 0.0f);
		drawPosColTex(width, -height, COVER_BOX_DEPTH, color, 1.0f, 1.0f);
		drawPosColTex(-width, -height, COVER_BOX_DEPTH, color, front_start, 1.0f);
	GX_End();

	
	//--------------------------------
	// favorites star
	//--------------------------------
	
	if (isFavorite) {
		//calculate the size
		f32 star_scale = .125;
		f32 starw = tx_star.w * star_scale;
		f32 starh = tx_star.h * star_scale;
		//init the texture
		GXTexObj texStar;
		GX_InitTexObj(&texStar, tx_star.data, tx_star.w, tx_star.h, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_LoadTexObj(&texStar, GX_TEXMAP0);
		GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
		GX_SetCullMode (GX_CULL_BACK);
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			drawPosColTex(width-starw, height, COVER_BOX_DEPTH, color, 0.0f, 0.0f);
			drawPosColTex(width, height, COVER_BOX_DEPTH, color, 1.0f, 0.0f);
			drawPosColTex(width, height-starh, COVER_BOX_DEPTH, color, 1.0f, 1.0f);
			drawPosColTex(width-starw, height-starh, COVER_BOX_DEPTH, color, 0.0f, 1.0f);
		GX_End();
	}
	
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
    GX_LoadPosMtxImm (GXview2D, GX_PNMTX0);
    //set TevOp to GX_PASSCLR so that we get the color's alpha level
    GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
	
	//reset the viewpoint to 2D
	set2DProjectionMatrix();
}


/**
 * Method to draw cover image reflection with perspective-based (3D) viewpoint.
 *
 *  @param tex cover image to draw
 *  @param xpos x axis position for the image
 *  @param ypos y axis position for the image
 *  @param zpos z axis position for the image
 *  @param xrot degrees to rotate the image on the x axis.  For example a value of 90 will rotate it 90 degrees so it would appear flat on it's cover to the viewer.
 *  @param yrot degrees to rotate the image on the y axis.  For example a value of 90 will rotate it 90 degrees on it's side so the front will be facing right to the viewer
 *  @param zrot degrees to rotate the image.  For example a value of 90 will rotate it 90 degrees counter-clockwise
 *  @param reflectionColorBottom the color (including alpha) of the bottom portion of the reflection (bottom of the cover).  255 will be a solid image.  0 will be fully transparent.
 *  @param reflectionColorTop the color (including alpha) of the top portion of the reflection (top of the cover).  255 will be a solid image.  0 will be fully transparent.
 *  @param reflectionColorBottomEdge the color (including alpha) of the bottom portion of the reflection (bottom of the boxcover).  255 will be a solid image.  0 will be fully transparent.
 *  @param reflectionColorTopEdge the color (including alpha) of the top portion of the reflection (top of the boxcover).  255 will be a solid image.  0 will be fully transparent.
 *  @param isFavorite bool represents if the favorite image should be drawn over the cover
 *  @return void
 */
void draw_fullcover_image_reflection(GRRLIB_texImg *tex, f32 xpos, f32 ypos, f32 zpos, f32 xrot, f32 yrot, f32 zrot, 
		u32 reflectionColorBottom, u32 reflectionColorTop, u32 reflectionColorBottomEdge, u32 reflectionColorTopEdge, bool isFavorite) {
	Mtx44 GXprojection2D;
	//Mtx GXview2D;
	Mtx GXmodel2D;
	Mtx GXModelTemp;
	//Mtx GXmodelView2D;
	GXTexObj texCoverImage;
	
	//cover width and height
	f32 width = 10.0;
	f32 height = 14.0;

	//fullcover tex positioning
	f32 front_start = 0.525;
	f32 spine_start = 0.475;

	//distance between the cover and its reflection
	f32 COVER_REFLECTION_DISTANCE = 2.0;
	//distance between front and back covers
	f32 COVER_BOX_DEPTH = 2.0;
	//depth of the edge
	f32 COVER_EDGE_DEPTH = 0.3;

	guVector xaxis = (guVector) {1, 0, 0};
	guVector yaxis = (guVector) {0, 1, 0};
	guVector zaxis = (guVector) {0, 0, 1};

	set_camera();
	
	guMtxIdentity(GXmodel2D);
	GX_LoadTexMtxImm(GXmodel2D, GX_TEXMTX0, GX_MTX2x4);
	
	//get the objects size
	//width = COVER_WIDTH * 0.0625;
    //height = COVER_HEIGHT * 0.0625;

	//set rotation
	if (xrot != 0) {
		guMtxRotAxisDeg(GXModelTemp, &xaxis, xrot);
		guMtxConcat (GXmodel2D, GXModelTemp, GXmodel2D);
	}
	if (yrot != 0) {
		guMtxRotAxisDeg(GXModelTemp, &yaxis, yrot);
		guMtxConcat (GXmodel2D, GXModelTemp, GXmodel2D);
	}
	if (zrot != 0) {
		guMtxRotAxisDeg(GXModelTemp, &zaxis, zrot);
		guMtxConcat (GXmodel2D, GXModelTemp, GXmodel2D);
	}

	// set the XYZ position
	guMtxTransApply(GXmodel2D, GXmodel2D, xpos, ypos, zpos);
	guMtxConcat (GXview2D, GXmodel2D, GXmodelView2D);
	
	// load the modelview matrix into matrix memory
	GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

	if (CFG.widescreen) {
		guPerspective(GXprojection2D, 45, 1.778F, 0.1F, 500.0F);
	} else {
		guPerspective(GXprojection2D, 45, (f32)4/3, 0.1F, 500.0F);
	}
	GX_LoadProjectionMtx(GXprojection2D, GX_PERSPECTIVE);

	//GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetZMode (GX_ENABLE, GX_LEQUAL, GX_TRUE);
	//GX_SetZMode(GX_ENABLE, GX_ALWAYS, GX_TRUE);
	
	//set blend mode
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);

	int tx_fmt = GX_TF_RGBA8;
	if (tex->tex_format) tx_fmt = tex->tex_format;
	GX_InitTexObj(&texCoverImage, tex->data, tex->w, tex->h, tx_fmt, GX_CLAMP, GX_CLAMP, GX_FALSE);
	if (tex->tex_lod) {
		// mipmap
		float lod_bias = lod_bias_a[lod_bias_idx];
		GX_InitTexObjLOD(&texCoverImage, GX_LIN_MIP_LIN, GX_LINEAR,
				0.f, // lod min
				(float)tex->tex_lod, // lod max
				lod_bias, // lod bias
				GX_FALSE,
				GX_FALSE, // edge lod
				GX_ANISO_1); //GX_ANISO_2 GX_ANISO_4); // _1 seems sharper
	}

	//--------------------------------
	// Reflection
	//--------------------------------

	if (reflectionColorBottom > 0 || reflectionColorTop > 0) {
		//Draw the reflection image upside down.  Set the RGBA color 
		//near the bottom of the cover (top of the reflection) to the passed 
		//in alpha level.  The RGBA for the lowest part of the reflection is 
		//set to zero, which gives us a fade-to-black appearance.

		//right side - cover opening
		GX_LoadTexObj(&texCoverSide, GX_TEXMAP0);
		GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
		GX_SetCullMode (GX_CULL_BACK);
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			drawPosColTex(width + COVER_EDGE_DEPTH, -height - COVER_REFLECTION_DISTANCE, COVER_BOX_DEPTH, reflectionColorBottomEdge, 0.0f, 1.0f);
			drawPosColTex(width + COVER_EDGE_DEPTH, -height - COVER_REFLECTION_DISTANCE, 0.0, reflectionColorBottomEdge, 1.0f, 1.0f);
			drawPosColTex(width + COVER_EDGE_DEPTH, -height*2.5 - COVER_REFLECTION_DISTANCE, 0.0, reflectionColorTopEdge, 1.0f, 0.0f);
			drawPosColTex(width + COVER_EDGE_DEPTH, -height*2.5 - COVER_REFLECTION_DISTANCE, COVER_BOX_DEPTH, reflectionColorTopEdge, 0.0f, 0.0f);
		GX_End();

		//bottom
		GX_LoadTexObj(&texCoverTop, GX_TEXMAP0);
		GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
		GX_SetCullMode (GX_CULL_BACK);
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			drawPosColTex(-width - COVER_EDGE_DEPTH, -height - COVER_REFLECTION_DISTANCE, 0.0, reflectionColorBottomEdge, 0.0f, 0.0f);
			drawPosColTex(width + COVER_EDGE_DEPTH, -height - COVER_REFLECTION_DISTANCE, 0.0, reflectionColorBottomEdge, 1.0f, 0.0f);
			drawPosColTex(width + COVER_EDGE_DEPTH, -height - COVER_REFLECTION_DISTANCE, COVER_BOX_DEPTH, reflectionColorBottomEdge, 1.0f, 1.0f);
			drawPosColTex(-width - COVER_EDGE_DEPTH, -height - COVER_REFLECTION_DISTANCE, COVER_BOX_DEPTH, reflectionColorBottomEdge, 0.0f, 1.0f);
		GX_End();

		//draw the game cover image
		GX_LoadTexObj(&texCoverImage, GX_TEXMAP0);
		GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
		GX_SetCullMode (GX_CULL_BACK);

		//fullcover back
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			drawPosColTex(width + COVER_EDGE_DEPTH, -height - COVER_REFLECTION_DISTANCE, 0.0f, reflectionColorBottom, 0.0f, 1.0f);
			drawPosColTex(-width - COVER_EDGE_DEPTH, -height - COVER_REFLECTION_DISTANCE, 0.0f, reflectionColorBottom, spine_start, 1.0f);
			drawPosColTex(-width - COVER_EDGE_DEPTH, -height *2.5 - COVER_REFLECTION_DISTANCE, 0.0f, reflectionColorTop, spine_start, 0.0f);
			drawPosColTex(width + COVER_EDGE_DEPTH, -height *2.5 - COVER_REFLECTION_DISTANCE, 0.0f, reflectionColorTop, 0.0f, 0.0f);
		GX_End();

		//front cover reflection
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			drawPosColTex(-width - COVER_EDGE_DEPTH, -height - COVER_REFLECTION_DISTANCE, COVER_BOX_DEPTH, reflectionColorBottom, front_start, 1.0f);
			drawPosColTex(width + COVER_EDGE_DEPTH, -height - COVER_REFLECTION_DISTANCE, COVER_BOX_DEPTH, reflectionColorBottom, 1.0f, 1.0f);
			drawPosColTex(width + COVER_EDGE_DEPTH, -height*2.5 - COVER_REFLECTION_DISTANCE, COVER_BOX_DEPTH, reflectionColorTop, 1.0f, 0.0f);
			drawPosColTex(-width - COVER_EDGE_DEPTH, -height*2.5 - COVER_REFLECTION_DISTANCE, COVER_BOX_DEPTH, reflectionColorTop, front_start, 0.0f);
		GX_End();

		//left - cover spine
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			drawPosColTex(-width - COVER_EDGE_DEPTH, -height - COVER_REFLECTION_DISTANCE, 0.0f, reflectionColorBottom, spine_start, 1.0f);
			drawPosColTex(-width - COVER_EDGE_DEPTH, -height - COVER_REFLECTION_DISTANCE, COVER_BOX_DEPTH, reflectionColorBottom, front_start, 1.0f);
			drawPosColTex(-width - COVER_EDGE_DEPTH, -height*2.5 - COVER_REFLECTION_DISTANCE, COVER_BOX_DEPTH, reflectionColorTop, front_start, 0.0f);
			drawPosColTex(-width - COVER_EDGE_DEPTH, -height*2.5 - COVER_REFLECTION_DISTANCE, 0.0f, reflectionColorTop, spine_start, 0.0f);
		GX_End();
	}

	//--------------------------------
	// favorites star
	//--------------------------------
	
	if (isFavorite) {
		//calculate the size
		f32 starw = tx_star.w * .125;
		f32 starh = tx_star.h * .062;
		//init the texture
		GXTexObj texStar;
		GX_InitTexObj(&texStar, tx_star.data, tx_star.w, tx_star.h, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_LoadTexObj(&texStar, GX_TEXMAP0);
		GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
		GX_SetCullMode (GX_CULL_BACK);
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			drawPosColTex(width-starw, -height*2.5 + starh, COVER_BOX_DEPTH, reflectionColorTop, 0.0f, 1.0f);
			drawPosColTex(width, -height*2.5 + starh, COVER_BOX_DEPTH, reflectionColorTop, 1.0f, 1.0f);
			drawPosColTex(width, -height*2.5 - COVER_REFLECTION_DISTANCE, COVER_BOX_DEPTH, reflectionColorTop, 1.0f, 0.0f);
			drawPosColTex(width-starw, -height*2.5 - COVER_REFLECTION_DISTANCE, COVER_BOX_DEPTH, reflectionColorTop, 0.0f, 0.0f);
		GX_End();
	}
	
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
    GX_LoadPosMtxImm (GXview2D, GX_PNMTX0);
    //set TevOp to GX_PASSCLR so that we get the color's alpha level
    GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
	
	//reset the viewpoint to 2D
	set2DProjectionMatrix();
}	

	
/**
 * Method that draws the passed in Cover in the specified color.  This is used by the 
 * capture_cover_positions() method for the cover mouseover detection.
 *  @param *cover CoverPos object to draw
 *  @param color the color to make the entire cover object. (e.g. 0x888888FF) 
 *  @return void
 */
void draw_phantom_cover (CoverPos *cover, u32 color) {
	Mtx44 GXprojection2D;
	Mtx GXview2D;
	Mtx GXmodel2D;
	Mtx GXModelTemp;
	//Mtx GXmodelView2D;
	f32 width, height;
	f32 COVER_BOX_DEPTH = 1.7;	

	guVector xaxis = (guVector) {1, 0, 0};
	guVector yaxis = (guVector) {0, 1, 0};
	guVector zaxis = (guVector) {0, 0, 1};

	//set the view
	guVector cam = {CFG_cf_theme[CFG_cf_global.theme].cam_pos_x, CFG_cf_theme[CFG_cf_global.theme].cam_pos_y, CFG_cf_theme[CFG_cf_global.theme].cam_pos_z},
		up = {0.0F, 1.0F, 0.0F},
		look = {CFG_cf_theme[CFG_cf_global.theme].cam_look_x, CFG_cf_theme[CFG_cf_global.theme].cam_look_y, CFG_cf_theme[CFG_cf_global.theme].cam_look_z};
	guLookAt(GXview2D, &cam, &up, &look);

	if (CFG.widescreen) {
		guPerspective(GXprojection2D, 45, 1.778F, 0.1F, 500.0F);
	} else {
		guPerspective(GXprojection2D, 45, (f32)4/3, 0.1F, 500.0F);
	}
	GX_LoadProjectionMtx(GXprojection2D, GX_PERSPECTIVE);

	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);

	guMtxIdentity(GXmodel2D);
	
	//get the objects size
	width = 10.0;
	height = 14.0;

	//set rotation
	if (cover->xrot != 0) {
		guMtxRotAxisDeg(GXModelTemp, &xaxis, cover->xrot);
		guMtxConcat (GXmodel2D, GXModelTemp, GXmodel2D);
	}
	if (cover->yrot != 0) {
		guMtxRotAxisDeg(GXModelTemp, &yaxis, cover->yrot);
		guMtxConcat (GXmodel2D, GXModelTemp, GXmodel2D);
	}
	if (cover->zrot != 0) {
		guMtxRotAxisDeg(GXModelTemp, &zaxis, cover->zrot);
		guMtxConcat (GXmodel2D, GXModelTemp, GXmodel2D);
	}

	// set the XYZ position
	guMtxTransApply(GXmodel2D, GXmodel2D, cover->x, cover->y, cover->z);
	guMtxConcat (GXview2D, GXmodel2D, GXmodelView2D);
	
	// load the modelview matrix into matrix memory
	GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

	//top
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(-width, height, COVER_BOX_DEPTH, color);
		drawPosCol(width, height, COVER_BOX_DEPTH, color);
		drawPosCol(width, height, 0.0f, color);
		drawPosCol(-width, height, 0.0f, color);
	GX_End();

	//right - cover opening
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(width, height, COVER_BOX_DEPTH, color);
		drawPosCol(width, height, 0.0f, color);
		drawPosCol(width, -height, 0.0f, color);
		drawPosCol(width, -height, COVER_BOX_DEPTH, color);
	GX_End();

	//back
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(width, height, 0.0f, color);
		drawPosCol(-width, height, 0.0f, color);
		drawPosCol(-width, -height, 0.0f, color);
		drawPosCol(width, -height, 0.0f, color);
	GX_End();

	//spine
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(-width, height, 0.0f, color);
		drawPosCol(-width, height, COVER_BOX_DEPTH, color);
		drawPosCol(-width, -height, COVER_BOX_DEPTH, color);
		drawPosCol(-width, -height, 0.0f, color);
	GX_End();

	//front
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		drawPosCol(-width, height, COVER_BOX_DEPTH, color);
		drawPosCol(width, height, COVER_BOX_DEPTH, color);
		drawPosCol(width, -height, COVER_BOX_DEPTH, color);
		drawPosCol(-width, -height, COVER_BOX_DEPTH, color);
	GX_End();
	GX_LoadPosMtxImm (GXview2D, GX_PNMTX0);
    GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
}


/**
 * Method that sets the stencil colors
 *  @return void
 */
void set_cover_stencil_colors() {
	int i;
	for (i=1; i<=CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers; i++) {
		coverCoords_left[i-1].stencil_color = GRRLIB_GetColor((u8)i, 0xFF, 0xFF, 0xFF);
		coverCoords_right[i-1].stencil_color = GRRLIB_GetColor((u8)255-i, 0xFF, 0xFF, 0xFF);
	}
	coverCoords_center.stencil_color = MOUSEOVER_COVER_OFFSCREEN;
}


/**
 * Method that draws each cover in a different color and then takes a snapshot of the buffer
 *  @return void
 */
void capture_cover_positions() {
	int i;

	if (tx_screenshot.data == NULL) {
		// first time, allocate
		// only 8 bits per pixel (R8)
		int src_size = 128 * 128;  // 16384
		tx_screenshot.data = LARGE_alloc(src_size);
		tx_screenshot.w = 128;
		tx_screenshot.h = 128;
	}
	
	GRRLIB_prepareStencil();
	//draw the covers with different colors
	for (i=CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers-1; i>=0; i--) {
		draw_phantom_cover(&coverCoords_left[i].currentPos, coverCoords_left[i].stencil_color);
		draw_phantom_cover(&coverCoords_right[i].currentPos, coverCoords_right[i].stencil_color);
	}
	draw_phantom_cover(&coverCoords_center.currentPos, coverCoords_center.stencil_color);
	GRRLIB_renderStencil_buf(&tx_screenshot);
}


/**
 * Method that updates the Cover.selected attribute of all the covers.  The 
 *  selectedIndex param is used to tell which cover should be set to true - all
 *  others will be marked false.
 *
 *  Valid selectedIndex values:  
 *    Center cover index = 0
 *    Left covers are < 0 -> so the left cover next to the center is -1
 *    Right covers are > 0 -> so the right cover next to the center is 1
 *
 *  @param selectedIndex the index of the cover that should be marked as selected
 *  @return void
 */
void set_selected_state(int selectedIndex) {
	int i;
	for (i=0; i<MAX_COVERFLOW_COVERS; i++) {
		if (selectedIndex < 0 && selectedIndex == -i - 1)
			coverCoords_left[i].selected = true;
		else
			coverCoords_left[i].selected = false;

		if (selectedIndex > 0 && selectedIndex == i + 1)
			coverCoords_right[i].selected = true;
		else
			coverCoords_right[i].selected = false;
	}
	if (selectedIndex == 0)
		coverCoords_center.selected = true;
	else
		coverCoords_center.selected = false;
}


/**
 * Method that returns the game index (gi) of the currently selected cover
 *  based on the Cover.selected attribute
 *
 *  @return int representing game index
 */
int get_selected_cover_index() {
	int i, selection;

	for (i=0; i<MAX_COVERFLOW_COVERS; i++) {
		if (coverCoords_left[i].selected) {
			selection = coverCoords_left[i].gi;
			goto out;
		}
		if (coverCoords_right[i].selected) {
			selection = coverCoords_right[i].gi;
			goto out;
		}
	}
	selection = coverCoords_center.gi;
	set_selected_state(0);
	out:;
	return selection;
}


/**
 * Method that determines if the current WiiMote sx/sy coord is over a cover.  This method uses
 *  the screenshot taken in the capture_cover_positions() method and interrogates the color 
 *  under the pointer.  If the color matches one of the covers then we found a match.
 *
 *  @param *ir IR info from the WiiMote
 *  @return int representing the index of the currently selected cover
 */
int is_over_cover(ir_t *ir) {
	int selected_gi, i;
	
	capture_cover_positions();
	//grab the color under the pointer
	i = GRRLIB_stencilVal(ir->sx, ir->sy, tx_screenshot);
	selectedColor = GRRLIB_GetColor((u8)i, 0xFF, 0xFF, 0xFF);
	if (selectedColor == coverCoords_center.stencil_color) {
		set_selected_state(0);
		selected_gi = coverCoords_center.gi;
		goto out;
	}
	for (i=0; i<CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers; i++) {
		if (selectedColor == coverCoords_left[i].stencil_color) {
			set_selected_state(-i - 1);
			selected_gi = coverCoords_left[i].gi;
			goto out;
		}
		if (selectedColor == coverCoords_right[i].stencil_color) {
			set_selected_state(i + 1);
			selected_gi = coverCoords_right[i].gi;
			goto out;
		}			
	}
	
	selected_gi = get_selected_cover_index();
	out:;
	GRRLIB_ResetVideo();
	return selected_gi;
}


/**
 * Translates 2D x/y coords into 3D x/y coords based on the camera positioning and z position of the object.
 *
 *  @param x_2d the 2d x coord to convert
 *  @param y_2d the 2d y coord to convert
 *  @param zpos z axis position of the 3d object to convert
 *  @param x_3d the converted 3d x coord
 *  @param y_3d the converted 3d y coord
 *  @return void
 */
void convert_2dCoords_into_3dCoords(float x_2d, float y_2d, float zpos, f32 *x_3d, f32 *y_3d)
{
	Mtx44 GXprojection2D;
	Mtx GXview2D, GXView2D_Inverse;
	guVector v1, v1_direction;
	f32 k;

	//set the view
	guVector cam = {CFG_cf_theme[CFG_cf_global.theme].cam_pos_x, CFG_cf_theme[CFG_cf_global.theme].cam_pos_y, CFG_cf_theme[CFG_cf_global.theme].cam_pos_z},
		up = {0.0F, 1.0F, 0.0F},
		look = {CFG_cf_theme[CFG_cf_global.theme].cam_look_x, CFG_cf_theme[CFG_cf_global.theme].cam_look_y, CFG_cf_theme[CFG_cf_global.theme].cam_look_z};
	guLookAt(GXview2D, &cam, &up, &look);

	//get the view matrix inverse
	guMtxInverse (GXview2D, GXView2D_Inverse);

	if (CFG.widescreen) {
		guPerspective(GXprojection2D, 45, 1.778F, 0.1F, 500.0F);
	} else {
		guPerspective(GXprojection2D, 45, (f32)4/3, 0.1F, 500.0F);
	}
	GX_LoadProjectionMtx(GXprojection2D, GX_PERSPECTIVE);

	//calculate the 3d vector based on the passed in 2d points and the perspective
	v1.x = -(((2.0f * x_2d) / 640.0) - 1.0) / GXprojection2D[0][0];
	v1.y = (((2.0f * y_2d ) / 480.0) - 1.0) / GXprojection2D[1][1];
	v1.z = 1.0f;
	//calculate the vector direction into the inverse world
	v1_direction.x = v1.x * GXView2D_Inverse[0][0] + v1.y * GXView2D_Inverse[1][0] + v1.z * GXView2D_Inverse[2][0];
	v1_direction.y = v1.x * GXView2D_Inverse[0][1] + v1.y * GXView2D_Inverse[1][1] + v1.z * GXView2D_Inverse[2][1];
	v1_direction.z = v1.x * GXView2D_Inverse[0][2] + v1.y * GXView2D_Inverse[1][2] + v1.z * GXView2D_Inverse[2][2];
	guVecNormalize (&v1_direction);

	//calculate the final 2d position based on the z position of the 3d object
	// and the direction into the inverse world
	k = (zpos - cam.z) / v1_direction.z;
	*x_3d = cam.x + k * v1_direction.x;
	*y_3d = cam.y + k * v1_direction.y;
}


/**
 * Calculates the max difference between two u32 colors (comparing RGB separately) and 
 *  applies the max difference to the RGB values of the first u32.  Then the u32 val is 
 *  added to the end result.
 *
 *  @param col the color to change
 *  @param col2 the color to compare the difference and apply to col
 *  @param val the color to add to col
 *  @param neg (1 or -1) to determine if val should be added or subtracted
 */
u32 addU32Value(u32 col, u32 col2, u32 val, int neg)
{
	int r = (int)R(col);
	int g = (int)G(col);
	int b = (int)B(col);
	int rv = (int)R(val);
	int gv = (int)G(val);
	int bv = (int)B(val);
	//get diff between col and col2 RGB
	int rdiff = r - (int)R(col2);
	int gdiff = g - (int)G(col2);
	int bdiff = b - (int)B(col2);
	//who's got the largest diff?
	int diff1 = (rdiff > bdiff) ? rdiff : bdiff;
	int diff = (gdiff > diff1) ? gdiff : diff1;
	diff = (diff > 0) ? diff : 0;
	//subtract diff from each color and add val
	r = r - diff + (rv * neg);
	r = (r > 255) ? 255 : ((r < 0) ? 0 : r);
	g = g - diff + (gv * neg);
	g = (g > 255) ? 255 : ((g < 0) ? 0 : g);
	b = b - diff + (bv * neg);
	b = (b > 255) ? 255 : ((b < 0) ? 0 : b);
	return ((u8)r << 24) | ((u8)g << 16) | ((u8)b << 8) | A(col2);
}


/**
 * Method that calculates the boxcover edge color
 *
 *  @param gi the game image id number
 *  @param selected boolean that represents if the cover should be dimmed (selected=false) or normal (selected=true)
 *  @param alpha the alpha level of the cover.  255 will be a solid image.  0 will be fully transparent.
 *  @param color the color (including alpha) of the game image
 *  @param reflectionColorBottom the color (including alpha) of the bottom portion of the reflection (bottom of the game image).  255 will be a solid image.  0 will be fully transparent.
 *  @param reflectionColorTop the color (including alpha) of the top portion of the reflection (top of the game image).  255 will be a solid image.  0 will be fully transparent.
 *  @param edgecolor the color (including alpha) of the boxcover
 *  @param reflectionBottomEdge the color (including alpha) of the bottom portion of the reflection (bottom of the boxcover).
 *  @param reflectionTopEdge the color (including alpha) of the top portion of the reflection (top of the boxcover).
 *  @return void
 */
void get_boxcover_edge_color(int gi, bool selected, u8 alpha, u32 color, u32 reflectionColorBottom, u32 reflectionColorTop,
		u32 *edgecolor, u32 *reflectionBottomEdge, u32 *reflectionTopEdge) {
	u32 col;
	bool dbColorFound = false;
	char *gameid;
	gameid = (char*)gameList[gi].id;
	
	//check xml database for color
	if (xml_getCaseColor(&col, (u8 *)gameid)) {
		dbColorFound = true;
		col |= alpha;
	} else {
		col = color;
	}

	//New Super Mario Bros
	if (strncmp(gameid, "SMN", 3) == 0) col = 0xFF000000 | alpha;

	//GameCube Games
	if(gameList[gi].magic == DML_MAGIC || gameList[gi].magic == DML_MAGIC_HDD)  col = 0x32323200 | alpha;
	
	if (selected) {
		*edgecolor = col;
		*reflectionBottomEdge = addU32Value(col, reflectionColorBottom, 0x11111100, 1);
		*reflectionTopEdge = addU32Value(col, reflectionColorTop, 0x11111100, 1);	
	} else {
		if (dbColorFound)
			*edgecolor = addU32Value(col, col, 0x22222200, -1);
		else
			*edgecolor = col;
		*reflectionBottomEdge = addU32Value(col, reflectionColorBottom, 0x11111100, -1);
		*reflectionTopEdge = addU32Value(col, reflectionColorTop, 0x11111100, -1);
	}
}


/**
 * Method draws a cover image.  Used when the cover structure isn't fully populated (in rotation loops, etc.)
 *  and you need to pass in every value.
 *
 *  @param tex image of the game cover
 *  @param xpos x axis position for the image
 *  @param ypos y axis position for the image
 *  @param zpos z axis position for the image
 *  @param xrot degrees to rotate the image on the x axis.  For example a value of 90 will rotate it 90 degrees so it would appear flat on it's cover to the viewer.
 *  @param yrot degrees to rotate the image on the y axis.  For example a value of 90 will rotate it 90 degrees on it's side so the front will be facing right to the viewer
 *  @param zrot degrees to rotate the image.  For example a value of 90 will rotate it 90 degrees counter-clockwise
 *  @param alpha the alpha level of the cover.  255 will be a solid image.  0 will be fully transparent.
 *  @param reflectionColorBottom the color (including alpha) of the bottom portion of the reflection (bottom of the game image).  255 will be a solid image.  0 will be fully transparent.
 *  @param reflectionColorTop the color (including alpha) of the top portion of the reflection (top of the game image).  255 will be a solid image.  0 will be fully transparent.
 *  @param draw3DCover boolean to determine if we should we draw the 3D cover object
 *  @param favorite boolean that represents if the favorites image should be drawn on the cover
 *  @param selected boolean that represents if the cover should be dimmed (selected=false) or normal (selected=true)
 *  @param drawReflection if true the cover reflection is drawn, else the cover itself
 *  @param gi the game image id number
 *  @return void
 */
void draw_cover_image(GRRLIB_texImg *tex, 
		f32 xpos, f32 ypos, f32 zpos, f32 xrot, f32 yrot, f32 zrot, 
		u8 alpha, u32 reflectionColorBottom, u32 reflectionColorTop, 
		bool draw3DCover, bool favorite, bool selected, bool drawReflection, int gi)
{
	u32 color, edgecolor, reflectionBottom, reflectionTop, reflectionBottomEdge, reflectionTopEdge;
	
	//is this cover selected?
	if (selected) {
		//light up selected game image
		color = 0xFFFFFF00 | alpha;
		reflectionBottom = color_add(reflectionColorBottom, 0x11111100, 1);
		reflectionTop = color_add(reflectionColorTop, 0x11111100, 1);
	} else {
		//dim not selected game image
		color = 0xDDDDDD00 | alpha;
		reflectionBottom = color_add(reflectionColorBottom, 0x11111100, -1);
		reflectionTop = color_add(reflectionColorTop, 0x11111100, -1);
	}
	//boxcover color
	get_boxcover_edge_color(gi, selected, alpha, color, reflectionColorBottom, reflectionColorTop, &edgecolor, &reflectionBottomEdge, &reflectionTopEdge);

	if (draw3DCover) {
		if (drawReflection)
			draw_fullcover_image_reflection(tex, xpos, ypos, zpos, xrot, yrot, zrot, reflectionBottom, reflectionTop, reflectionBottomEdge, reflectionTopEdge, favorite);
		else
			draw_fullcover_image(tex, xpos, ypos, zpos, xrot, yrot, zrot, color, edgecolor, favorite);
	} else {
		draw_2dcover_image(tex, xpos, ypos, zpos, xrot, yrot, zrot, color, reflectionBottom, reflectionTop, favorite);
	}
}


/**
 * Method that fades the background
 *  @return void
 */
void fadeBackground() {
	int background_alpha = 0;
	switch (background_fade_status) {
		case BACKGROUND_FADE_DARK:
			if (CFG_cf_global.transition == CF_TRANS_FLIP_TO_BACK && CFG_cf_global.frameCount > 0 && CFG_cf_global.frameIndex <= CFG_cf_global.frameCount) {
				background_alpha = calculateNewPosition(0, CFG_cf_global.screen_fade_alpha, CFG_cf_global.frameIndex, CFG_cf_global.frameCount, EASING_TYPE_LINEAR);
			} else {
				background_alpha = CFG_cf_global.screen_fade_alpha;
			}
			break;
		case BACKGROUND_FADE_LIGHT:
			if (CFG_cf_global.transition == CF_TRANS_FLIP_TO_BACK && CFG_cf_global.frameCount > 0 && CFG_cf_global.frameIndex <= CFG_cf_global.frameCount) {
				background_alpha = calculateNewPosition(CFG_cf_global.screen_fade_alpha, 0, CFG_cf_global.frameIndex, CFG_cf_global.frameCount, EASING_TYPE_LINEAR);
			} else {
				background_fade_status = BACKGROUND_FADE_NONE;
				background_alpha = 0;
			}
			break;
		default:
		case BACKGROUND_FADE_NONE: return;
	}
	if (background_alpha > 0 && background_alpha <= 255) {
		GRRLIB_FillScreen(0x00000000 | background_alpha);
	}
}


/**
 * Method that gets the next position in an arc.
 * 		parabola calculation:
 *		y = a(x-h)2+k 
 *		a < 0 means opening is down (shaped like n)
 *		h = distance to vertex
 *		k = height
 *	for Excel: vertex = -0.8 * (POWER(B3 - (-10), 2)) + 10
 *
 * @param a represents the opening - usually generated from the getAparam() method
 * @param height represents the height of the arc.  So if you want the vertex to stop at 10 on the x axis, pass in a 10
 * @param vertex represents the distance to the vertex from the starting point
 * @param index represents the current index in your drawing loop
 *  @return f32 representing your next position
 */
f32 getArcPos(f32 a, f32 height, f32 vertex, f32 index) {
	return a * (pow(index - vertex, 2)) + height;
}


/**
 * Method that gets the A param for the getArcPos() method
 * 		parabola calculation:
 *		y = a(x-h)2+k 
 *		a < 0 means opening is down (shaped like n)
 *		h = distance to vertex
 *		k = height
 *
 * @param height represents the height of the arc.  So if you want the vertex to stop at 10 on the x axis, pass in a 10
 * @param vertex represents the distance to the vertex from the starting point
 * @param finalIndexPos your last index value in your drawing loop (max loop value)
 * @param finalArcPos represents where you want to end on the straight axis - so if you're arcing from left to right on the 
 *			screen and you want the arc to be on the y axis, you pass in where you want to end on the x axis.
 *  @return f32 representing the A value
 */
f32 getAparam(f32 height, f32 vertex, f32 finalIndexPos, f32 finalArcPos) {
	return (-height + finalArcPos) / (pow(finalIndexPos - vertex, 2));
}

extern char action_string[40];
extern int action_alpha;

/**
 * Draws the selected cover's title
 *
 * @param selectedCover int representing the currently selected cover index
 * @return void
 */
void Coverflow_draw_title(int selectedCover, int xpos, ir_t *ir) {
	f32 title_y, title_x;
	char gameTitle[TITLE_MAX] = "";
	static time_t last_time = 0;
	bool do_clock = false;
	FontColor font_color = CFG.gui_text2;
	time_t t = 0;
	
	//game title stub.....
	if (ir->smooth_valid || CFG_cf_global.frameCount || !CFG.clock_style) {
		last_time = 0;
		L_title:
		if (action_alpha) {
			font_color.color = (font_color.color & 0xFFFFFF00) | action_alpha;
			if (action_alpha > 0) action_alpha -= 3;
			if (action_alpha < 0) action_alpha = 0;
			last_time = 0;
			strncpy(gameTitle, action_string, TITLE_MAX);
		} else {
			if (!gameCnt) {
				strcpy(gameTitle, gt("No games found!"));
			} else if (selectedCover < 0 || selectedCover >= gameCnt) {
				sprintf(gameTitle, "error %d", selectedCover);
			} else {
				snprintf(gameTitle, TITLE_MAX, "%s", get_title(&gameList[selectedCover]));
			}
		}
	} else {
		// clock
		t = time(NULL);
		// wait 5 seconds before showing clock
		if (last_time == 0) { last_time = t; goto L_title; }
		if (t - last_time < 5) goto L_title;
		do_clock = true;
	}
	
	//if xpos>0 then we are overriding the theme's settings
	if (xpos > 0) {
		title_x = xpos;
	} else {
		//Get x pos -> -1 means center it
		if (CFG_cf_theme[CFG_cf_global.theme].title_text_xpos == -1)
			title_x = (BACKGROUND_WIDTH/2) - (strlen(gameTitle)*(tx_font.tilew))/2;
		else
			title_x = CFG_cf_theme[CFG_cf_global.theme].title_text_xpos;
	}
		
	//Get y pos -> -1 means center it
	if (CFG_cf_theme[CFG_cf_global.theme].title_text_ypos == -1) {
		title_y = BACKGROUND_HEIGHT/2;
	} else {
		title_y = CFG_cf_theme[CFG_cf_global.theme].title_text_ypos;
	}
	
	//check if we're showing the back cover
	if (!showingFrontCover) {
		title_y = 436.f;
	}
	if (CFG.gui_title_top) {
		title_y = 24.f + 2.f; // 24 = overscan
	}
	if (CFG.gui_title_area.w) {
		if (xpos > 0) {
			title_x = CFG.gui_title_area.x + CFG.gui_title_area.w/4;
		} else {
			title_x = CFG.gui_title_area.x + CFG.gui_title_area.w/2 
				- (strlen(gameTitle)*(tx_font.tilew))/2;
		}
		title_y = CFG.gui_title_area.y;
	}
	// clock
	if (CFG.gui_clock_pos.x >= 0) {
		Gui_Print_Clock(CFG.gui_clock_pos.x, CFG.gui_clock_pos.y, CFG.gui_text2, -1);
		do_clock = false;
	}
	if (do_clock) {
		int x = BACKGROUND_WIDTH/2;
		int y = title_y + tx_font.tileh/2;
		Gui_Print_Clock(x, y, CFG.gui_text2, 0);
	} else {
		Gui_PrintEx(title_x, title_y, &tx_font, font_color, gameTitle);
	}
}


/**
 * resets the floating cover postions
 */
void resetFloatingCover() {
	//reset the floating cover vars
	float_xrot = 0;
	float_yrot = 0;
	float_zrot = 0;
}


/**
 * Calculates next position for floating covers
 */
void calculate_floating_cover() {
	//do the floating cover thing if the theme wants it or we're showing the back cover
	if (CFG_cf_theme[CFG_cf_global.theme].floating_cover || !showingFrontCover) {
		//calculate xrot for float
		if (float_xrot_up && float_xrot >= float_max_xrot) {
			float_xrot -= float_speed_increment;
			float_xrot_up = false;
		} else if (float_xrot_up) {
			float_xrot += float_speed_increment;
		} else if (!float_xrot_up && float_xrot <= -float_max_xrot) {
			float_xrot += float_speed_increment;
			float_xrot_up = true;
		} else {
			float_xrot -= float_speed_increment;
		}

		//calculate yrot for float
		if (float_yrot_up && float_yrot >= float_max_yrot) {
			float_yrot -= float_speed_increment;
			float_yrot_up = false;
		} else if (float_yrot_up) {
			float_yrot += float_speed_increment;
		} else if (!float_yrot_up && float_yrot <= -float_max_yrot) {
			float_yrot += float_speed_increment;
			float_yrot_up = true;
		} else {
			float_yrot -= float_speed_increment;
		}

		//calculate zrot for float
		if (float_zrot_up && float_zrot >= float_max_zrot) {
			float_zrot -= float_speed_increment;
			float_zrot_up = false;
		} else if (float_zrot_up) {
			float_zrot += float_speed_increment;
		} else if (!float_zrot_up && float_zrot <= -float_max_zrot) {
			float_zrot += float_speed_increment;
			float_zrot_up = true;
		} else {
			float_zrot -= float_speed_increment;
		}
	}
}


/**
 * Main method used to draw rotated covers.
 *  The basic equation is: 
 *    current pos = starting point - ((starting point - end point) * index / number of total frames)
 *  @param *cover Cover object to rotate
 *  @param index int representing the current index in the rotation loop
 *  @param frames int representing the total frame count of the rotation 
 *  @param floating if true the cover floats
 *  @param drawReflection if true the cover reflection is drawn, else the cover itself
 *  @param ease int representing the easing level to use: e.g. 0=linear
 *  @return void
 */
void Coverflow_rotate(struct Cover *cover, int index, int frames, bool floating, bool drawReflection, int ease) {
	//calculate the new positions
	cover->currentPos.x = calculateNewPosition(cover->startPos.x, cover->endPos.x, index, frames, ease);
	cover->currentPos.y = calculateNewPosition(cover->startPos.y, cover->endPos.y, index, frames, ease);
	cover->currentPos.z = calculateNewPosition(cover->startPos.z, cover->endPos.z, index, frames, ease);
	cover->currentPos.xrot = calculateNewPosition(cover->startPos.xrot, cover->endPos.xrot, index, frames, ease);
	cover->currentPos.yrot = calculateNewPosition(cover->startPos.yrot, cover->endPos.yrot, index, frames, ease);
	cover->currentPos.zrot = calculateNewPosition(cover->startPos.zrot, cover->endPos.zrot, index, frames, ease);
	cover->currentPos.alpha = calculateNewPosition(cover->startPos.alpha, cover->endPos.alpha, index, frames, ease);
	cover->currentPos.reflection_bottom = calculateNewColor(cover->startPos.reflection_bottom, cover->endPos.reflection_bottom, index, frames, ease);
	cover->currentPos.reflection_top = calculateNewColor(cover->startPos.reflection_top, cover->endPos.reflection_top, index, frames, ease);
	bool favorite = is_favorite(gameList[cover->gi].id);

	if (floating) {
		//draw the cover image
		draw_cover_image(&cover->tx, cover->currentPos.x, cover->currentPos.y, cover->currentPos.z,
			cover->currentPos.xrot + float_xrot, cover->currentPos.yrot + float_yrot, cover->currentPos.zrot + float_zrot, 
			cover->currentPos.alpha, cover->currentPos.reflection_bottom, cover->currentPos.reflection_top, 
			CFG_cf_global.covers_3d, favorite, cover->selected, drawReflection, cover->gi);
	} else {
		//draw the cover image
		draw_cover_image(&cover->tx, cover->currentPos.x, cover->currentPos.y, cover->currentPos.z,
			cover->currentPos.xrot, cover->currentPos.yrot, cover->currentPos.zrot, 
			cover->currentPos.alpha, cover->currentPos.reflection_bottom, cover->currentPos.reflection_top, 
			CFG_cf_global.covers_3d, favorite, cover->selected, drawReflection, cover->gi);
	}
}


/**
 * Method that calculates the new frameIndex when switching easing types.  This gets called when rotation is stopping.
 *
 *  @param ease_type the easing type that is being switch to
 *  @param new_framecount int representing the new frame count
 *  @return void
 */
void calculate_new_easing_frameindex(int ease_type, int new_framecount) {
	int i = 0;
	f32 xpos = 0;
	
	CFG_cf_global.frameIndex = 1;
	CFG_cf_global.frameCount = new_framecount;
	//find the closest matching frame index for the new ease type
	if (coverCoords_center.startPos.x > coverCoords_center.endPos.x) {
		for (i=0; i<=CFG_cf_global.frameCount; i++) {
			xpos = calculateNewPosition(coverCoords_center.startPos.x, coverCoords_center.endPos.x, i, new_framecount, ease_type);
			if (xpos <= coverCoords_center.currentPos.x) {
				CFG_cf_global.frameIndex = i;
				break;
			}
		}
	} else {
		for (i=0; i<CFG_cf_global.frameCount; i++) {
			xpos = calculateNewPosition(coverCoords_center.startPos.x, coverCoords_center.endPos.x, i, new_framecount, ease_type);
			if (xpos >= coverCoords_center.currentPos.x) {
				CFG_cf_global.frameIndex = i;
				break;
			}
		}
	}
}


/**
 * Method that resets the startPos to the currentPos for all right and left covers.
 *
 *  @return void
 */
void reset_rotation_startPosition() {
	int i = 0;
	
	//let the side covers keep rotating...
	if (CFG_cf_global.transition==CF_TRANS_ROTATE_RIGHT) {
		coverCoords_left[0].startPos = coverCoords_left[0].currentPos;
		for (i=0; i<CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers; i++) {
			coverCoords_left[i+1].startPos = coverCoords_left[i+1].currentPos;
			coverCoords_right[CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers-i-1].startPos = coverCoords_right[CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers-i-1].currentPos;
		}
	} else if (CFG_cf_global.transition==CF_TRANS_ROTATE_LEFT) {
		coverCoords_right[0].startPos = coverCoords_right[0].currentPos;
		for (i=0; i<CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers; i++) {
			coverCoords_right[i+1].startPos = coverCoords_right[i+1].currentPos;
			coverCoords_left[CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers-i-1].startPos = coverCoords_left[CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers-i-1].currentPos;
		}
	}
}


/**
 * Method that repaints the static covers.  This gets called when nothing is really going on (no rotating)
 *
 *  @param *ir pointer to current wiimote data
 *  @param coverCount total number of games
 *  @param draw_title boolean to determine if the game title should be drawn
 *  @return int representing the index of the currently selected cover
 */
int Coverflow_drawCovers(ir_t *ir, int coverCount, bool draw_title) {
	int i, j, alpha;
	int ease;
	int selectedCover = -1;

	cache_release_all();
	cache_request_before_and_after(coverCoords_center.gi, 5, 1);

	//update the floating cover data
	calculate_floating_cover();

	//update the cover images from the cache
	Coverflow_update_state();

	//check if the pointer is over a cover
	if (showingFrontCover) {
		selectedCover = is_over_cover(ir);
	} else {
		//showing the back cover so force to the center cover
		set_selected_state(0);
		selectedCover = coverCoords_center.gi;
	}
	//set easing based on rotation speed
	if (CFG_cf_global.frameCount < CFG_cf_theme[CFG_cf_global.theme].rotation_frames)
		ease = EASING_TYPE_LINEAR;  //linear
	else
		ease = EASING_TYPE_SLOW;
		
	for(j=0; j<CFG.gui_antialias; j++) {
		
		if (CFG.gui_antialias > 1)
			GRRLIB_prepareAAPass(CFG.gui_antialias, j);
		else
			GRRLIB_ResetVideo();

		//if moving to/from console mode then fade the backgrounds
		if (CFG_cf_global.transition == CF_TRANS_MOVE_TO_CONSOLE || CFG_cf_global.transition == CF_TRANS_MOVE_FROM_CONSOLE) {
			alpha = 255 * CFG_cf_global.frameIndex / CFG_cf_global.frameCount;
			Gui_set_camera(NULL, 0);
			if (CFG_cf_global.transition == CF_TRANS_MOVE_TO_CONSOLE) {
				GRRLIB_DrawImg(0, 0, &tx_bg, 0, 1, 1, 0xFFFFFF00 | (255-alpha));
				GRRLIB_DrawImg(0, 0, &tx_bg_con, 0, 1, 1, 0xFFFFFF00 | (alpha));
			} else {
				GRRLIB_DrawImg(0, 0, &tx_bg_con, 0, 1, 1, 0xFFFFFF00 | (255-alpha));
				GRRLIB_DrawImg(0, 0, &tx_bg, 0, 1, 1, 0xFFFFFF00 | (alpha));
			}
			Gui_set_camera(NULL, 1);
		} else {
			Gui_draw_background();
		}

		// reflections
		Coverflow_rotate(&coverCoords_center, CFG_cf_global.frameIndex, CFG_cf_global.frameCount, false, true, ease);
		for (i=0; i<CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers + 1; i++) {
			Coverflow_rotate(&coverCoords_right[i], CFG_cf_global.frameIndex, CFG_cf_global.frameCount, false, true, ease);
			Coverflow_rotate(&coverCoords_left[i], CFG_cf_global.frameIndex, CFG_cf_global.frameCount, false, true, ease);
		}

		// covers
		for (i=CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers; i>=0; i--) {
			Coverflow_rotate(&coverCoords_left[i], CFG_cf_global.frameIndex, CFG_cf_global.frameCount, false, false, ease);
			Coverflow_rotate(&coverCoords_right[i], CFG_cf_global.frameIndex, CFG_cf_global.frameCount, false, false, ease);
		}
	
		fadeBackground();

		//center cover
		if (!showingFrontCover || CFG_cf_theme[CFG_cf_global.theme].floating_cover)
			//float the cover
			Coverflow_rotate(&coverCoords_center, CFG_cf_global.frameIndex, CFG_cf_global.frameCount, true, false, ease);
		else
			Coverflow_rotate(&coverCoords_center, CFG_cf_global.frameIndex, CFG_cf_global.frameCount, false, false, ease);
	
		if (CFG.gui_antialias > 1)
			Gui_RenderAAPass(j);
	}

	if (CFG.gui_antialias > 1) {
		GRRLIB_drawAAScene(CFG.gui_antialias, aa_texBuffer);
		GRRLIB_ResetVideo();
	}
	

	if (draw_title) {
		//if rotating fast then left align title for easier readability
		if ((CFG_cf_global.frameCount < CFG_cf_theme[CFG_cf_global.theme].rotation_frames)	
				&& (CFG_cf_global.transition==CF_TRANS_ROTATE_RIGHT || CFG_cf_global.transition==CF_TRANS_ROTATE_LEFT))
			Coverflow_draw_title(selectedCover, 180, ir);
		else
			Coverflow_draw_title(selectedCover, -1, ir);
	}

	//check if we're done transitioning
	if (CFG_cf_global.transition > 0) {
		if (CFG_cf_global.frameIndex >= CFG_cf_global.frameCount) {
			//rotations should always end at the default (slow) speed
			if (CFG_cf_global.frameCount < CFG_cf_theme[CFG_cf_global.theme].rotation_frames
					&& (CFG_cf_global.transition==CF_TRANS_ROTATE_RIGHT || CFG_cf_global.transition==CF_TRANS_ROTATE_LEFT)) {
				Coverflow_init_transition(CFG_cf_global.transition, 0, coverCount, true);
			} else {
				//done with this transition so set the cover positions
				coverCoords_center.currentPos = coverCoords_center.endPos;
				coverCoords_center.startPos = coverCoords_center.endPos;
				for (i=0; i < CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers + 1; i++) {
					coverCoords_left[i].currentPos = coverCoords_left[i].endPos;
					coverCoords_left[i].startPos = coverCoords_left[i].endPos;
					coverCoords_right[i].currentPos = coverCoords_right[i].endPos;
					coverCoords_right[i].startPos = coverCoords_right[i].endPos;
				}
				//reset the transition params
				CFG_cf_global.frameIndex = 0;
				CFG_cf_global.frameCount = 0;
				CFG_cf_global.transition = 0;
			}
		} else {
			//if we're rotating fast but we stopped pressing the dpad (or rotating via wiimote) then 
			// we need to recalculate the frameIndex so the easing switchover is smooth
			if (CFG_cf_global.frameCount < CFG_cf_theme[CFG_cf_global.theme].rotation_frames
					&& (CFG_cf_global.transition==CF_TRANS_ROTATE_RIGHT || CFG_cf_global.transition==CF_TRANS_ROTATE_LEFT)) {
				if (!rotating_with_wiimote && !Wpad_Held(0) && CFG_cf_global.frameIndex < CFG_cf_global.frameCount * .75) {
					calculate_new_easing_frameindex(EASING_TYPE_SLOW, CFG_cf_theme[CFG_cf_global.theme].rotation_frames);
				}
			}
			//not done so increase the frame index
			CFG_cf_global.frameIndex++;	
		}
		rotating_with_wiimote = 0;
	}

	if (CFG.debug == 3) {
		GRRLIB_Printf(50, 10, &tx_font, CFG.gui_text.color, 1, "center  start.x:%f end.x:%f", coverCoords_center.startPos.x, coverCoords_center.endPos.x);
		GRRLIB_Printf(50, 25, &tx_font, CFG.gui_text.color, 1, "center  start.y:%f end.y:%f", coverCoords_center.startPos.y, coverCoords_center.endPos.y);
		GRRLIB_Printf(50, 40, &tx_font, CFG.gui_text.color, 1, "center  start.z:%f end.z:%f", coverCoords_center.startPos.z, coverCoords_center.endPos.z);
		GRRLIB_Printf(50, 55, &tx_font, CFG.gui_text.color, 1, "center  start.yrot:%f end.yrot:%f", coverCoords_center.startPos.yrot, coverCoords_center.endPos.yrot);
		GRRLIB_Printf(50, 70, &tx_font, CFG.gui_text.color, 1, "trans? %i, framecount: %i, frameindex: %i  ease: %i", CFG_cf_global.transition, CFG_cf_global.frameCount, CFG_cf_global.frameIndex, ease);
		//to see the mouseover screenshot image:
		//GRRLIB_DrawImg_format(0, 0, tx_screenshot, GX_TF_I8, 0, 1, 1, 0xFFFFFFFF);
		//currently selected color and cover index:
		GRRLIB_Printf(200, 410, &tx_font, CFG.gui_text.color, 1.0, "cover(gi): %i  color: %X", selectedCover, selectedColor);
		//mouse pointer position:
		//GRRLIB_Printf(50, 430, &tx_font, CFG.gui_text.color, 1.0, "sx: %.2f  sy: %.2f  angle: %.2f v: %d %d %d", ir->sx, ir->sy, ir->angle,	ir->raw_valid, ir->smooth_valid, ir->valid);
		orient_t o;
		WPAD_Orientation(0, &o);
		GRRLIB_Printf(50, 450, &tx_font, CFG.gui_text.color, 1.0, "yaw:%.2f pitch: %.2f  roll: %.2f ", o.yaw, o.pitch, o.roll);
	}
	
	return selectedCover;
}


/**
 * Method that populates a CoverPos (cover position) struct based on the passed in position type.
 *
 *  @param type int representing the cover position type to use
 *  @param *coverPos the CoverPos to populate
 *  @return void
 */
void build_coverPos_type(int type, CoverPos *coverPos) {
	f32 x, y;

	switch(type){
		//position for when viewing the back cover
		case COVERPOS_FLIP_TO_BACK:
			coverPos->xrot = 0;
			coverPos->yrot = 180;
			coverPos->zrot = 0;
			coverPos->x = CFG_cf_global.cover_back_xpos;
			coverPos->y = CFG_cf_global.cover_back_ypos;
			coverPos->z = CFG_cf_global.cover_back_zpos;
			coverPos->reflection_bottom = CFG_cf_theme[CFG_cf_global.theme].reflections_color_bottom & 0xFFFFFF00;
			coverPos->reflection_top = CFG_cf_theme[CFG_cf_global.theme].reflections_color_top & 0xFFFFFF00;
			coverPos->alpha = 255;
			break;

		//position of the center cover based on the current theme
		case COVERPOS_CENTER_THEME:
			coverPos->xrot = coverCoords_center.themePos.xrot;
			coverPos->yrot = coverCoords_center.themePos.yrot;
			coverPos->zrot = coverCoords_center.themePos.zrot;
			coverPos->x = coverCoords_center.themePos.x;
			coverPos->y = coverCoords_center.themePos.y;
			coverPos->z = coverCoords_center.themePos.z;
			coverPos->reflection_bottom = coverCoords_center.themePos.reflection_bottom;
			coverPos->reflection_top = coverCoords_center.themePos.reflection_top;
			coverPos->alpha = coverCoords_center.themePos.alpha;
			break;
			
		//position of the console image (2D cover)
		case COVERPOS_CONSOLE_2D:
			//calculate the 3d x and y position of the console cover
			convert_2dCoords_into_3dCoords(COVER_XCOORD + 80, COVER_YCOORD + 112, -73, &x, &y);
			coverPos->xrot = 0;
			coverPos->yrot = 0;
			coverPos->zrot = 0;
			coverPos->x = x;
			coverPos->y = y;
			coverPos->z = -73;
			coverPos->reflection_bottom = 0x00000000;
			coverPos->reflection_top = 0x00000000;
			coverPos->alpha = 255;
			break;

		//position of the console image (fake 3D cover)
		case COVERPOS_CONSOLE_3D:
			//calculate the 3d x and y position of the console cover
			convert_2dCoords_into_3dCoords(COVER_XCOORD + 80, COVER_YCOORD + 112, -80, &x, &y);
			coverPos->xrot = 0;
			coverPos->yrot = 33;
			coverPos->zrot = 0;
			coverPos->x = x;
			coverPos->y = y;
			coverPos->z = -82;
			coverPos->reflection_bottom = 0x00000000;
			coverPos->reflection_top = 0x00000000;
			coverPos->alpha = 255;
			break;

		//position of the game loading screen cover
		case COVERPOS_GAME_LOAD:
			coverPos->xrot = 0;
			coverPos->yrot = 0; //33;
			coverPos->zrot = 0;
			coverPos->x = 0;
			coverPos->y = 0;
			coverPos->z = -50; //-73;
			coverPos->alpha = 255;
			coverPos->reflection_bottom = 0x00000000;
			coverPos->reflection_top = 0x00000000;
		break;
	}
}


/**
 * Method that draws the center cover for the game options screen
 *
 *  @param game_sel int representing the index of the selected game to draw
 *  @param x int X position of the cover
 *  @param y int Y position of the cover
 *  @param z int Z position of the cover
 *  @param xrot f32 the X-axis rotation of the cover
 *  @param yrot f32 the Y-axis rotation of the cover
 *  @param zrot f32 the Z-axis rotation of the cover
 *  @return void
 */
void Coverflow_drawCoverForGameOptions(int game_sel,
		float x, float y, float z, f32 xrot, f32 yrot, f32 zrot, int cstyle)
{
	Cover cover;
	bool favorite;
	int aa, j;
	GRRLIB_texImg bg_tx;
	long long t1, t11, t12, t2;

	if (!grx_cover_init) Coverflow_Grx_Init();

	t1 = gettime();
	// reset cover indexing to the selected cover
	setCoverIndexing(gameCnt, game_sel);
	cache_request_before_and_after(game_sel, 5, 1);
	// update the cover images from the cache
	//Coverflow_update_state();
	update_cover_state2(&coverCoords_center, cstyle);

	favorite = is_favorite(gameList[coverCoords_center.gi].id);
	// init the positioning
	build_coverPos_type(COVERPOS_GAME_LOAD, &cover.currentPos);
	// scale the passed in x and y coords to 3d space
	convert_2dCoords_into_3dCoords(x+80, y+112, -40, &cover.currentPos.x, &cover.currentPos.y);	

	t11 = gettime();
	//GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	//GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
	//GX_InvVtxCache();
	//GX_InvalidateTexAll();
	// capture the background for aa
	bg_tx = GRRLIB_AAScreen2Texture();
	GRRLIB_AAScreen2Texture_buf(&bg_tx, GX_FALSE);
	t12 = gettime();

	aa = (CFG.gui_antialias < 1) ? 1 : CFG.gui_antialias;
	for(j=0; j < aa; j++) {
		
		GRRLIB_prepareAAPass(aa, j);

		Gui_set_camera(NULL, 0);
		if (j) Gui_DrawImgFullScreen(&bg_tx, 0xFFFFFFFF, false);
		Gui_set_camera(NULL, 1);

		// debug bench
		if (coverflow_test_grid)
		{
			int mx=8, my=4;
			int nx, ny;
			for (ny = 0; ny < my; ny++) {
				for (nx = 0; nx < mx; nx++) {
					if (ny == my-1 && nx < 2) continue;
					float x, y, z;
					float px, py; //, pz;
					x = (0.5+nx + 0.1*ny) * 640 / mx;
					y = (0.5+ny) * 480 / my;
					z = -150;
					convert_2dCoords_into_3dCoords(x, y, z, &px, &py);
					draw_cover_image(&coverCoords_center.tx,
							px, py, z,
							//cover.currentPos.xrot, yrot, cover.currentPos.zrot, 
							xrot, yrot, zrot, 
							cover.currentPos.alpha,
							cover.currentPos.reflection_bottom,
							cover.currentPos.reflection_top, 
							true, favorite, true, false, coverCoords_center.gi);
				}
			}
		}

		//draw the cover image
		draw_cover_image(&coverCoords_center.tx,
				cover.currentPos.x, cover.currentPos.y, cover.currentPos.z + z,
				//cover.currentPos.xrot, yrot, cover.currentPos.zrot, 
				xrot, yrot, zrot, 
				cover.currentPos.alpha,
				cover.currentPos.reflection_bottom,
				cover.currentPos.reflection_top, 
				true, favorite, true, false, coverCoords_center.gi);

		Gui_RenderAAPass(j);
	}
	GRRLIB_drawAAScene(aa, aa_texBuffer);
	GRRLIB_ResetVideo();
	SAFE_FREE(bg_tx.data);
	t2 = gettime();
	if (CFG.debug == 3)
	{
		GRRLIB_Printf(20, 60, &tx_font, 0xFF00FFFF, 1,
				"ms:%6.2f %6.2f %6.2f %6.2f",
				(float)diff_usec(t1,t2)/1000.0,
				(float)diff_usec(t1,t11)/1000.0,
				(float)diff_usec(t11,t12)/1000.0,
				(float)diff_usec(t12,t2)/1000.0);
	}
}


/**
 * Method used to inititate a new coverflow transition (rotate right/left, spin, etc).
 *
 *  @param trans_type int representing the transition type to initiate (rotate right/left, spin, etc)
 *  @param speed int representing the transition speed.  Set to 0 for the "default" slow speed.
 *  @param coverCount total number of games
 *  @param speed_rampup bool representing if each iteration of the transition should increase in speed
 *  @return int representing the index of the currently selected cover
 */
int Coverflow_init_transition(int trans_type, int speed, int coverCount, bool speed_rampup) {
	int numSideCovers, covertype, idx;
	int i=0;
	ir_t ir;
	int slow_rotation_speed = 20;

	//set the framecount for the transition
	if (trans_type == CF_TRANS_ROTATE_RIGHT || trans_type == CF_TRANS_ROTATE_LEFT) {
		//are we already performing this action?
		if (CFG_cf_global.transition == trans_type && speed > 0) {	
			//check if we're allowed to perform the transition yet
			if (CFG_cf_global.frameIndex < rotation_start_index) {
				return -1;
			} else {
				if (speed_rampup) {
					CFG_cf_global.frameCount = CFG_cf_theme[CFG_cf_global.theme].rotation_frames_fast;
					if (rotation_start_index > CFG_cf_theme[CFG_cf_global.theme].rotation_frames_fast)
						rotation_start_index = CFG_cf_theme[CFG_cf_global.theme].rotation_frames_fast;
					else
						rotation_start_index = 7;  //final rotation start - increase to slow it down
				} else {
					rotation_start_index = (int)round(calculateNewPosition(slow_rotation_speed, CFG_cf_theme[CFG_cf_global.theme].rotation_frames_fast, speed, 100, EASING_TYPE_LINEAR));
					CFG_cf_global.frameCount = (int)round(calculateNewPosition(slow_rotation_speed, CFG_cf_theme[CFG_cf_global.theme].rotation_frames_fast, speed, 100, EASING_TYPE_LINEAR));
				}
				CFG_cf_global.frameIndex = 1;
			}
		} else {
			//new transition - reset the rotation vars
			if (speed_rampup) {
				//first iteration, start at the default (slow) speed.  We'll ramp up from here next time.
				rotation_start_index = 20;  //determines when fast rotation starts when holding down the dpad
				CFG_cf_global.frameCount = CFG_cf_theme[CFG_cf_global.theme].rotation_frames;
				CFG_cf_global.frameIndex = 1;
			} else {
				//calculate framerate based on passed in speed level
				rotation_start_index = (int)round(calculateNewPosition(slow_rotation_speed, CFG_cf_theme[CFG_cf_global.theme].rotation_frames_fast, speed, 100, EASING_TYPE_LINEAR));
				idx = (int)round(calculateNewPosition(slow_rotation_speed, CFG_cf_theme[CFG_cf_global.theme].rotation_frames_fast, speed, 100, EASING_TYPE_LINEAR));
				if (idx == CFG_cf_theme[CFG_cf_global.theme].rotation_frames) {
					//rotating from slow to fast so find our new position for linear easing
					calculate_new_easing_frameindex(EASING_TYPE_LINEAR, idx);
				} else {
					CFG_cf_global.frameCount = idx;
					CFG_cf_global.frameIndex = 1;
				}
			}
		}
	}
	
	//store the number of side covers
	numSideCovers = CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers;

	//set the cover start and end positions for the transition
	switch(trans_type){
		case CF_TRANS_ROTATE_RIGHT:
			//move the img index to the right
			setCoverIndexing(coverCount, coverCoords_right[0].gi);
			if (numSideCovers > 0) {
				//right[0] to center
				coverCoords_center.startPos = coverCoords_right[0].currentPos;
				//center to left[0]
				coverCoords_left[0].startPos = coverCoords_center.currentPos;
				//side covers
				for (i=0; i<numSideCovers; i++) {
					coverCoords_left[i+1].startPos = coverCoords_left[i].currentPos;
					coverCoords_right[numSideCovers-i-1].startPos = coverCoords_right[numSideCovers-i].currentPos;
				}
				coverCoords_right[numSideCovers].startPos = coverCoords_right[numSideCovers].themePos;
				coverCoords_right[numSideCovers].currentPos = coverCoords_right[numSideCovers].themePos;
			}
			CFG_cf_global.transition = CF_TRANS_ROTATE_RIGHT;
			break;

		case CF_TRANS_ROTATE_LEFT:
			//move the img index to the left
			setCoverIndexing(coverCount, coverCoords_left[0].gi);
			if (numSideCovers > 0) {
				//left[0] to center
				coverCoords_center.startPos = coverCoords_left[0].currentPos;
				//center to right[0]
				coverCoords_right[0].startPos = coverCoords_center.currentPos;
				//side covers
				for (i=0; i<numSideCovers; i++) {
					coverCoords_right[i+1].startPos = coverCoords_right[i].currentPos;
					coverCoords_left[numSideCovers-i-1].startPos = coverCoords_left[numSideCovers-i].currentPos;
				}
				coverCoords_left[numSideCovers].startPos = coverCoords_left[numSideCovers].themePos;
				coverCoords_left[numSideCovers].currentPos = coverCoords_left[numSideCovers].themePos;
			}
			CFG_cf_global.transition = CF_TRANS_ROTATE_LEFT;
			break;

		case CF_TRANS_FLIP_TO_BACK:
			rotation_start_index = 20;

			if (CFG_cf_global.transition == trans_type)
				CFG_cf_global.frameCount = CFG_cf_global.frameIndex;
			else
				CFG_cf_global.frameCount = 30;
			CFG_cf_global.frameIndex = 1;

			coverCoords_center.startPos = coverCoords_center.currentPos;
			//let the side covers keep rotating...
			reset_rotation_startPosition();
			//set the positions
			if (showingFrontCover) {
				build_coverPos_type(COVERPOS_FLIP_TO_BACK, &coverCoords_center.endPos);
				background_fade_status = BACKGROUND_FADE_DARK;
				showingFrontCover = false;
			} else {
				build_coverPos_type(COVERPOS_CENTER_THEME, &coverCoords_center.endPos);
				background_fade_status = BACKGROUND_FADE_LIGHT;
				showingFrontCover = true;
			}
			CFG_cf_global.transition = CF_TRANS_FLIP_TO_BACK;
			break;

		case CF_TRANS_MOVE_TO_CENTER:
			break;

		case CF_TRANS_MOVE_TO_POSITION:
			rotation_start_index = 20;
			CFG_cf_global.frameCount = 20;
			CFG_cf_global.frameIndex = 1;
			if (background_fade_status == BACKGROUND_FADE_DARK) 
				background_fade_status = BACKGROUND_FADE_LIGHT;
			CFG_cf_global.transition = CF_TRANS_MOVE_TO_POSITION;			
			break;
		
		case CF_TRANS_MOVE_TO_CONSOLE:
		case CF_TRANS_MOVE_TO_CONSOLE_3D:
			rotation_start_index = 16 - CFG.gui_antialias * 2;
			CFG_cf_global.frameCount = 16 - CFG.gui_antialias * 2;
			CFG_cf_global.frameIndex = 1;


			if (trans_type == CF_TRANS_MOVE_TO_CONSOLE)
				covertype = COVERPOS_CONSOLE_2D;
			else
				covertype = COVERPOS_CONSOLE_3D;
			
			//check if the pointer is over a cover
			Wpad_getIR(&ir);
			if (showingFrontCover) {
				i = is_over_cover(&ir);
			} else {
				//showing the back cover so force to the center cover
				set_selected_state(0);
			}
			//set the positions
			for (i=0; i<numSideCovers; i++) {
				if (coverCoords_left[i].selected) {
					build_coverPos_type(covertype, &coverCoords_left[i].endPos);
				} else {
					coverCoords_left[i].endPos.alpha = 0;
					coverCoords_left[i].endPos.reflection_bottom = coverCoords_left[i].currentPos.reflection_bottom & 0xFFFFFF00;
					coverCoords_left[i].endPos.reflection_top = coverCoords_left[i].currentPos.reflection_top & 0xFFFFFF00;
				}
				if (coverCoords_right[i].selected) {
					build_coverPos_type(covertype, &coverCoords_right[i].endPos);
				} else {
					coverCoords_right[i].endPos.alpha = 0;
					coverCoords_right[i].endPos.reflection_bottom = coverCoords_right[i].currentPos.reflection_bottom & 0xFFFFFF00;
					coverCoords_right[i].endPos.reflection_top = coverCoords_right[i].currentPos.reflection_top & 0xFFFFFF00;
				}
			}
			if (coverCoords_center.selected) {
				build_coverPos_type(covertype, &coverCoords_center.endPos);
			} else {
				coverCoords_center.endPos.alpha = 0;
				coverCoords_center.endPos.reflection_bottom = coverCoords_center.currentPos.reflection_bottom & 0xFFFFFF00;
				coverCoords_center.endPos.reflection_top = coverCoords_center.currentPos.reflection_top & 0xFFFFFF00;
			}
			
			if (background_fade_status == BACKGROUND_FADE_DARK) 
				background_fade_status = BACKGROUND_FADE_LIGHT;
			CFG_cf_global.transition = CF_TRANS_MOVE_TO_CONSOLE;
			//we have to loop the rendering here since we're leaving gui mode
			for (i=0; i<CFG_cf_global.frameCount; i++) {
				Wpad_getIR(&ir);
				Coverflow_drawCovers(&ir, coverCount, false);
				Gui_draw_pointer(&ir);
				Gui_Render();
			}
			//reset the transition params
			CFG_cf_global.frameIndex = 0;
			CFG_cf_global.frameCount = 0;
			break;

		case CF_TRANS_MOVE_FROM_CONSOLE:
		case CF_TRANS_MOVE_FROM_CONSOLE_3D:
			rotation_start_index = 16 - CFG.gui_antialias * 2;
			CFG_cf_global.frameCount = 16 - CFG.gui_antialias * 2;
			CFG_cf_global.frameIndex = 1;


			if (trans_type == CF_TRANS_MOVE_FROM_CONSOLE)
				covertype = COVERPOS_CONSOLE_2D;
			else
				covertype = COVERPOS_CONSOLE_3D;

			//set the positions
			for (i=0; i<numSideCovers; i++) {
				coverCoords_left[i].startPos.alpha = 0;
				coverCoords_left[i].startPos.reflection_bottom = coverCoords_left[i].currentPos.reflection_bottom & 0xFFFFFF00;
				coverCoords_left[i].startPos.reflection_top = coverCoords_left[i].currentPos.reflection_top & 0xFFFFFF00;
				coverCoords_right[i].startPos.alpha = 0;
				coverCoords_right[i].startPos.reflection_bottom = coverCoords_right[i].currentPos.reflection_bottom & 0xFFFFFF00;
				coverCoords_right[i].startPos.reflection_top = coverCoords_right[i].currentPos.reflection_top & 0xFFFFFF00;
			}
			build_coverPos_type(covertype, &coverCoords_center.startPos);
			CFG_cf_global.transition = CF_TRANS_MOVE_FROM_CONSOLE;
			//loop the transition rendering...
			for (i=0; i<CFG_cf_global.frameCount; i++) {
				Wpad_getIR(&ir);
				Coverflow_drawCovers(&ir, coverCount, false);
				Gui_draw_pointer(&ir);
				Gui_Render();
			}
			break;
			
		default:
		case CF_TRANS_NONE:
			break;
	}
	return coverCoords_center.gi;
}

/**
 * Method that flips the center cover 180 degrees from it's current position to either 
 *  a fullscreen view (on it's back cover) or back to where it should be (on it's front 
 *  cover).  If showBackCover is true, it will flip to the back.
 *
 *  @param showBackCover boolean to tell if back cover should be shown
 *  @param TypeUsed if 1 then wiimote if 2 then dpad if 0 then just flip it with no checks
 *  @return int representing the index of the currently selected cover
 */
int Coverflow_flip_cover_to_back(bool showBackCover, int typeUsed) {
	if (typeUsed == 2) {  //DPad
		if (showingFrontCover) {
			spinCover_dpad_used = true;
		} else {
			spinCover_dpad_used = false;
		}
		Coverflow_init_transition(CF_TRANS_FLIP_TO_BACK, 0, 0, true);
	} else if (typeUsed == 1) {  //wiimote
		if (spinCover_dpad_used) {
			if (showBackCover && !showingFrontCover) {
				spinCover_dpad_used = false;
			}
		} else {
			if (showingFrontCover && showBackCover) {
				Coverflow_init_transition(CF_TRANS_FLIP_TO_BACK, 0, 0, true);
			} else if (!showingFrontCover && !showBackCover) {
				Coverflow_init_transition(CF_TRANS_FLIP_TO_BACK, 0, 0, true);
			}
		}
	} else {
		//somebody just wants it flipped so flip it
		spinCover_dpad_used = false;
		Coverflow_init_transition(CF_TRANS_FLIP_TO_BACK, 0, 0, true);
	}
	return coverCoords_center.gi;
}


/**
 * Checks the current wiimote params to determine if an action needs to be performed
 *  e.g. rotate the center cover, scroll right/left, etc.
 *
 *  @param *ir IR info from the WiiMote
 *  @param coverCount total number of games
 *  @return int representing the index of the currently selected cover
 */
int Coverflow_process_wiimote_events(ir_t *ir, int coverCount) {
	int selected = coverCoords_center.gi;
	int speed = 0;
	int ret = 0;
	
	// check if pointing within screen
	if ((ir->sy >= 0 && ir->sy <= BACKGROUND_HEIGHT) &&
		(ir->sx >= -160 && ir->sx <= BACKGROUND_WIDTH+160)) {

		//get rotation data
		int ir_rot = (int)ir->angle;
		if (ir_rot < 0) ir_rot += 360;
		//flip the cover over if they're rotating the wiimote
		if (ir_rot > 80 && ir_rot < 280){
			selected = Coverflow_flip_cover_to_back(true, 1);
			goto out;
		} else {
			selected = Coverflow_flip_cover_to_back(false, 1);
		}

		// jump out if pointer scroll disabled
		if (!CFG.gui_pointer_scroll) goto out;
		if (CFG.gui_cover_area.h) {
			if (ir->sy < CFG.gui_cover_area.y
					|| ir->sy > CFG.gui_cover_area.y + CFG.gui_cover_area.h) {
				goto out;
			}
		}
		
		//scroll left or right
		if (ir->sx >= -80 && ir->sx < WIIMOTE_SIDE_SCROLL_WIDTH - 1) {
			//LEFT
			if (ir->sx > 20 && ir->sx < WIIMOTE_SIDE_SCROLL_WIDTH - 1) {
				//speed = (100 / (WIIMOTE_SIDE_SCROLL_WIDTH - 20)) * (WIIMOTE_SIDE_SCROLL_WIDTH - ir->sx);
				speed = (int)round(1.25 * (f32)(WIIMOTE_SIDE_SCROLL_WIDTH - ir->sx));
			} else if (ir->sx <= 20 && ir->sx >= -80) {
				//max speed
				speed = 100;
			}
			if (speed < 0) speed = 0;
			rotating_with_wiimote = 1;
			ret = Coverflow_init_transition(CF_TRANS_ROTATE_LEFT, speed, coverCount, false);
			if (ret > 0) {
				cache_release_all();
				cache_request_before_and_after(ret, 5, 1);
			}
			
		} else if ((ir->sx > BACKGROUND_WIDTH - WIIMOTE_SIDE_SCROLL_WIDTH + 1) && (ir->sx <= BACKGROUND_WIDTH + 80)) {
			//RIGHT
			if ((ir->sx > BACKGROUND_WIDTH - WIIMOTE_SIDE_SCROLL_WIDTH + 1) && (ir->sx < BACKGROUND_WIDTH - 20)) {
				speed = (int)round(1.25 * (f32)(ir->sx - (BACKGROUND_WIDTH - WIIMOTE_SIDE_SCROLL_WIDTH)));
			} else if ((ir->sx <= BACKGROUND_WIDTH + 80) && (ir->sx >= BACKGROUND_WIDTH - 20)) {
				//max speed
				speed = 100;
			}
			if (speed < 0) speed = 0;
			rotating_with_wiimote = 1;
			ret = Coverflow_init_transition(CF_TRANS_ROTATE_RIGHT, speed, coverCount, false);
			if (ret > 0) {
				cache_release_all();
				cache_request_before_and_after(ret, 5, 1);
			}
		}
	}
	out:;
	return selected;
}


/**
 * Initializes the cover coordinates based on the currently selected theme
 *  @param coverCount total number of games
 *  @param selectedCoverIndex index of the currently selected cover
 *  @param themeChange boolean to determine if a coverflow gui theme change is being performed
 *  @return int representing the index of the currently selected cover
 */
int Coverflow_initCoverObjects(int coverCount, int selectedCoverIndex, bool themeChange) {
	int selectedCover, i, numSideCovers;
	
	//set the global number of covers
	CFG_cf_global.number_of_covers = coverCount;
	
	if (!themeChange) {
		memset(coverCoords_left, 0, sizeof(coverCoords_left));
		memset(coverCoords_right, 0, sizeof(coverCoords_right));
		memset(&coverCoords_center, 0, sizeof(coverCoords_center));
	}
	
	//if they tried to get crazy with the number of side covers then max them out
	if (CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers > MAX_COVERFLOW_COVERS) {
		CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers = MAX_COVERFLOW_COVERS;
	}
	
	numSideCovers = CFG_cf_theme[CFG_cf_global.theme].number_of_side_covers;

	//center positioning
	coverCoords_center.themePos.x = CFG_cf_theme[CFG_cf_global.theme].cover_center_xpos;
	coverCoords_center.themePos.y = CFG_cf_theme[CFG_cf_global.theme].cover_center_ypos;
	coverCoords_center.themePos.z = CFG_cf_theme[CFG_cf_global.theme].cover_center_zpos;
	coverCoords_center.themePos.xrot = CFG_cf_theme[CFG_cf_global.theme].cover_center_xrot;
	coverCoords_center.themePos.yrot = CFG_cf_theme[CFG_cf_global.theme].cover_center_yrot;
	coverCoords_center.themePos.zrot = CFG_cf_theme[CFG_cf_global.theme].cover_center_zrot;
	coverCoords_center.themePos.alpha = 255;
	coverCoords_center.themePos.reflection_bottom = CFG_cf_theme[CFG_cf_global.theme].reflections_color_bottom;
	coverCoords_center.themePos.reflection_top = CFG_cf_theme[CFG_cf_global.theme].reflections_color_top;

	if (!themeChange) {
		coverCoords_center.currentPos = coverCoords_center.themePos;
		coverCoords_center.startPos = coverCoords_center.themePos;
	} else {
		coverCoords_center.startPos = coverCoords_center.currentPos;
	}
	coverCoords_center.endPos = coverCoords_center.themePos;

	//side covers
	for (i=0; i < numSideCovers; i++) {
		setCoverPosition_Left(&coverCoords_left[i].themePos, i);
		if (!themeChange) {
			coverCoords_left[i].currentPos = coverCoords_left[i].themePos;
			coverCoords_left[i].startPos = coverCoords_left[i].themePos;
		} else {
			coverCoords_left[i].startPos = coverCoords_left[i].currentPos;
		}
		coverCoords_left[i].endPos = coverCoords_left[i].themePos;
		coverCoords_left[i].selected = false;
		coverCoords_left[i].gi = 0;

		setCoverPosition_Right(&coverCoords_right[i].themePos, i);
		if (!themeChange) {
			coverCoords_right[i].currentPos = coverCoords_right[i].themePos;
			coverCoords_right[i].startPos = coverCoords_right[i].themePos;
		} else {
			coverCoords_right[i].startPos = coverCoords_right[i].currentPos;
		}
		coverCoords_right[i].endPos = coverCoords_right[i].themePos;
		coverCoords_right[i].selected = false;
		coverCoords_right[i].gi = 0;
	}

	//hide the far right and left covers - only shown when rotating
	setCoverPosition_Right(&coverCoords_right[numSideCovers].themePos, i);
	coverCoords_right[numSideCovers].hidden = 1;
	coverCoords_right[numSideCovers].themePos.alpha = 0;
	coverCoords_right[numSideCovers].themePos.reflection_bottom = coverCoords_right[numSideCovers].themePos.reflection_bottom & 0xFFFFFF00;
	coverCoords_right[numSideCovers].themePos.reflection_top = coverCoords_right[numSideCovers].themePos.reflection_top & 0xFFFFFF00;
	coverCoords_right[numSideCovers].currentPos = coverCoords_right[numSideCovers].themePos;
	coverCoords_right[numSideCovers].startPos = coverCoords_right[numSideCovers].themePos;
	coverCoords_right[numSideCovers].endPos = coverCoords_right[numSideCovers].themePos;

	setCoverPosition_Left(&coverCoords_left[numSideCovers].themePos, i);
	coverCoords_left[numSideCovers].hidden = 1;
	coverCoords_left[numSideCovers].themePos.alpha = 0;
	coverCoords_left[numSideCovers].themePos.reflection_bottom = coverCoords_left[numSideCovers].themePos.reflection_bottom & 0xFFFFFF00;
	coverCoords_left[numSideCovers].themePos.reflection_top = coverCoords_left[numSideCovers].themePos.reflection_top & 0xFFFFFF00;
	coverCoords_left[numSideCovers].currentPos = coverCoords_left[numSideCovers].themePos;
	coverCoords_left[numSideCovers].startPos = coverCoords_left[numSideCovers].themePos;
	coverCoords_left[numSideCovers].endPos = coverCoords_left[numSideCovers].themePos;

	//init the cover indexing
	if (!themeChange)
		//use the passed in cover index when coming from console or grid
		selectedCover = setCoverIndexing(coverCount, selectedCoverIndex);
	else
		//use the current center cover index if changing coverflow themes
		selectedCover = setCoverIndexing(coverCount, coverCoords_center.gi);

	//screenshot the new cover positions for the mouseover detection
	set_cover_stencil_colors();
	capture_cover_positions();
	
	//reset the floating cover vars
	resetFloatingCover();

	showingFrontCover = true;
	background_fade_status = BACKGROUND_FADE_NONE;
	cover_init = 1;

	return selectedCover;
}
