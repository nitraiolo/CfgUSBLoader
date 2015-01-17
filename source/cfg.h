
// by oggzee & usptactical

#ifndef _CFG_H
#define _CFG_H

#include <gctypes.h>
#include "disc.h"
#include "util.h"
#include "cfgutil.h"
#include "strutil.h"

typedef struct PosCoords
{
	int x, y;
} PosCoords;

typedef struct RectCoords
{
	int x, y, w, h;
} RectCoords;


#define CFG_IOS_245       0
#define CFG_IOS_246       1
#define CFG_IOS_247       2
#define CFG_IOS_248       3
#define CFG_IOS_249       4
#define CFG_IOS_250       5
#define CFG_IOS_251       6
#define CFG_IOS_222_MLOAD 7
#define CFG_IOS_223_MLOAD 8
#define CFG_IOS_224_MLOAD 9
#define CFG_IOS_222_YAL  10
#define CFG_IOS_223_YAL  11
#define CFG_IOS_AUTO     12
extern int CFG_IOS_MAX;
extern int CURR_IOS_IDX;

#define CFG_MIOS	 	 0
#define CFG_DML_R51      1
#define CFG_DML_R52      2
#define CFG_DML_1_2      3
#define CFG_DM_2_0	 	 4
#define CFG_DM_2_1       5
#define CFG_DM_2_2       6

#include "version.h"

#define MAX_THEME 300
#define MAX_THEME_NAME 32
extern char theme_list[MAX_THEME][MAX_THEME_NAME];

extern int ENTRIES_PER_PAGE;
extern int MAX_CHARACTERS;
extern int CONSOLE_XCOORD;
extern int CONSOLE_YCOORD;
extern int CONSOLE_WIDTH;
extern int CONSOLE_HEIGHT;
extern int CONSOLE_BG_COLOR;
extern int CONSOLE_FG_COLOR;
extern int COVER_XCOORD;
extern int COVER_YCOORD;
extern int COVER_WIDTH;
extern int COVER_HEIGHT;
extern int COVER_WIDTH_FRONT;
extern int COVER_HEIGHT_FRONT;

#define CFG_LAYOUT_ORIG      0
#define CFG_LAYOUT_ORIG_12   1
#define CFG_LAYOUT_SMALL     2
#define CFG_LAYOUT_MEDIUM    3
#define CFG_LAYOUT_LARGE     4 // nixx
#define CFG_LAYOUT_LARGE_2   5 // usptactical
#define CFG_LAYOUT_LARGE_3   6 // oggzee
#define CFG_LAYOUT_ULTIMATE1 7 // Ultimate1: (WiiShizza)
#define CFG_LAYOUT_ULTIMATE2 8 // Ultimate2: (jservs7 / hungyip84)
#define CFG_LAYOUT_ULTIMATE3 9 // Ultimate3: (WiiShizza)
#define CFG_LAYOUT_KOSAIC   10 // Kosaic

#define CFG_VIDEO_SYS   0  // system default
#define CFG_VIDEO_AUTO  0
#define CFG_VIDEO_GAME  1  // game default
#define CFG_VIDEO_PAL50	2  // force PAL
#define CFG_VIDEO_PAL60	3  // force PAL60
#define CFG_VIDEO_NTSC	4  // force NTSC
#define CFG_VIDEO_MAX   4
#define CFG_VIDEO_NUM   (CFG_VIDEO_MAX+1)
//#define CFG_VIDEO_PATCH 5  // patch mode - moved to separate option

#define CFG_VIDEO_PATCH_OFF 0
#define CFG_VIDEO_PATCH_ON  1
#define CFG_VIDEO_PATCH_ALL 2
#define CFG_VIDEO_PATCH_SNEEK 3
#define CFG_VIDEO_PATCH_SNEEK_ALL 4
#define CFG_VIDEO_PATCH_MAX 4
#define CFG_VIDEO_PATCH_NUM (CFG_VIDEO_PATCH_MAX+1)

#define CFG_CLEAN_OFF 0
#define CFG_CLEAN_ON  1
#define CFG_CLEAN_ALL 2

#define CFG_HOME_REBOOT     0
#define CFG_HOME_EXIT       1
#define CFG_HOME_HBC        2
#define CFG_HOME_SCRSHOT    3
#define CFG_HOME_CHANNEL    4
#define CFG_HOME_PRIILOADER 0x4461636F
#define CFG_HOME_WII_MENU   0x50756E65

//char languages[11][22] =
#define CFG_LANG_CONSOLE   0
#define CFG_LANG_JAPANESE  1
#define CFG_LANG_ENGLISH   2
#define CFG_LANG_GERMAN    3
#define CFG_LANG_FRENCH    4
#define CFG_LANG_SPANISH   5
#define CFG_LANG_ITALIAN   6
#define CFG_LANG_DUTCH     7
#define CFG_LANG_S_CHINESE 8
#define CFG_LANG_T_CHINESE 9
#define CFG_LANG_KOREAN   10
#define CFG_LANG_MAX      10
#define CFG_LANG_NUM      (CFG_LANG_MAX+1)

/*#define CFG_BTN_ORIGINAL  0 // obsolete
#define CFG_BTN_ULTIMATE  1 // obsolete
#define CFG_BTN_OPTIONS_1 2
#define CFG_BTN_OPTIONS_B 3*/

#define CFG_DEV_ASK  0
#define CFG_DEV_USB  1
#define CFG_DEV_SDHC 2

#define CFG_WIDE_NO   0
#define CFG_WIDE_YES  1
#define CFG_WIDE_AUTO 2

#define CFG_COLORS_MONO   1
#define CFG_COLORS_DARK   2
#define CFG_COLORS_BRIGHT 3

#define GUI_STYLE_GRID 0
#define GUI_STYLE_FLOW 1
#define GUI_STYLE_FLOW_Z 2
#define GUI_STYLE_COVERFLOW 3

#define CFG_GUI_START 2
#define CFG_GUI_START_WGUI 4

#define CFG_COVER_STYLE_2D   0
#define CFG_COVER_STYLE_3D   1
#define CFG_COVER_STYLE_DISC 2
#define CFG_COVER_STYLE_FULL 3
#define CFG_COVER_STYLE_HQ   4
// style + format combos:
#define CFG_COVER_STYLE_FULL_MIPMAP     5
#define CFG_COVER_STYLE_HQ_MIPMAP       6
#define CFG_COVER_STYLE_FULL_CMPR       7
#define CFG_COVER_STYLE_HQ_CMPR         8
#define CFG_COVER_STYLE_FULL_RGB        9
#define CFG_COVER_STYLE_HQ_RGB         10
#define CFG_COVER_STYLE_HQ_OR_FULL     11
#define CFG_COVER_STYLE_HQ_OR_FULL_RGB 12
// used just for cache/.ccc path:
#define CFG_COVER_STYLE_CACHE 20

#define CFG_INSTALL_GAME 0
#define CFG_INSTALL_ALL  1
#define CFG_INSTALL_1_1  2
#define CFG_INSTALL_ISO  3

#define CFG_UNLOCK_PASSWORD "BUDAH12"

#define CFG_BTN_PRIILOADER 0x4461636F
#define CFG_BTN_WII_MENU   0x50756E65
#define CFG_BTN_REMAP      0x80
#define CFG_BTN_M (CFG_BTN_REMAP | NUM_BUTTON_MINUS)
#define CFG_BTN_P (CFG_BTN_REMAP | NUM_BUTTON_PLUS)
#define CFG_BTN_A (CFG_BTN_REMAP | NUM_BUTTON_A) 
#define CFG_BTN_B (CFG_BTN_REMAP | NUM_BUTTON_B)
#define CFG_BTN_H (CFG_BTN_REMAP | NUM_BUTTON_HOME)
#define CFG_BTN_1 (CFG_BTN_REMAP | NUM_BUTTON_1)
#define CFG_BTN_2 (CFG_BTN_REMAP | NUM_BUTTON_2)
#define CFG_BTN_OPTIONS     0
#define CFG_BTN_GUI         1
#define CFG_BTN_REBOOT      2
#define CFG_BTN_EXIT        3
#define CFG_BTN_SCREENSHOT  4
#define CFG_BTN_INSTALL     5
#define CFG_BTN_REMOVE      6
#define CFG_BTN_MAIN_MENU   7
#define CFG_BTN_GLOBAL_OPS  8
#define CFG_BTN_FAVORITES   9
#define CFG_BTN_BOOT_DISC  10
#define CFG_BTN_THEME      11
#define CFG_BTN_PROFILE    12
#define CFG_BTN_UNLOCK     13
#define CFG_BTN_HBC        14
#define CFG_BTN_NOTHING    15
#define CFG_BTN_BOOT_GAME  16
#define CFG_BTN_SORT       17
#define CFG_BTN_FILTER     18
#define CFG_BTN_RANDOM     19
#define CFG_BTN_CHANNEL    20
/* Warning by Clipper: if the CFG_BTN_* list ever grows longer than 48 actions
 * (bloody hell, if so), then start using 1 << 8, 2 << 8 and so on.  Any number
 * in the alpha range could get confused with the channels and/or magic words,
 * and 128-255 are reserved for button remapping. */

#define ALT_DOL_OFF  0
#define ALT_DOL_SD   1
#define ALT_DOL_DISC 2
#define ALT_DOL_PLUS 3

#define CFG_SELECT_PREVIOUS 0
#define CFG_SELECT_START    1
#define CFG_SELECT_MIDDLE   2
#define CFG_SELECT_END      3
#define CFG_SELECT_MOST     4
#define CFG_SELECT_LEAST    5
#define CFG_SELECT_RANDOM   6

#define CFG_PLAYLOG_OFF      0
#define CFG_PLAYLOG_ON       1
#define CFG_PLAYLOG_JAPANESE 2
#define CFG_PLAYLOG_ENGLISH  3

extern char FAT_DRIVE[];
extern char USBLOADER_PATH[];
extern char APPS_DIR[];
extern char CFG_VERSION[];
extern char LAST_CFG_PATH[];
extern char DIOS_MIOS_INFO[];

typedef char GAMEID_t[8];

struct Game_CFG
{
	int language;
	int video;
	int video_patch;
	int vidtv;
	int wide_screen;
	int ntsc_j_patch;
	int nodisc;
	int screenshot;
	int country_patch;
	int fix_002;
	int ios_idx;
	int block_ios_reload;
	int ocarina;
	int alt_dol;
	char dol_name[64];
	int write_playlog;
	int hooktype;
	int clean;
	int nand_emu;
	int channel_boot;
	int alt_controller_cfg;
};

struct Game_CFG_2
{
	u8 id[8];
	struct Game_CFG curr;
	struct Game_CFG save;
	int is_saved;
};


typedef struct FontColor
{
	u32 color;
	u32 outline;
	u32 shadow;
} FontColor;

static inline FontColor Font_Color(u32 color, u32 outline, u32 shadow)
{
	FontColor fc;
	fc.color = color;
	fc.outline = outline;
	fc.shadow = shadow;
	return fc;
}

typedef struct FontColor_CFG
{
	u32 color;
	u32 outline;
	u32 outline_auto;
	u32 shadow;
	u32 shadow_auto;
} FontColor_CFG;

typedef struct MenuButton
{
	int mask;
	int num;
} MenuButton;

typedef struct CfgButton
{
	int enabled;
	RectCoords pos;
	FontColor fc;
	char image[64];
	int type;
	int hover_zoom; // in % default: 10%
} CfgButton;

#define GUI_BUTTON_MAIN      0
#define GUI_BUTTON_SETTINGS  1
#define GUI_BUTTON_QUIT      2
#define GUI_BUTTON_STYLE     3
#define GUI_BUTTON_VIEW      4
#define GUI_BUTTON_SORT      5
#define GUI_BUTTON_FILTER    6
#define GUI_BUTTON_FAVORITES 7
#define GUI_BUTTON_JUMP		 8
#define GUI_BUTTON_NUM       9

#define GUI_COLOR_NONE  0
#define GUI_COLOR_BASE  1
#define GUI_COLOR_POPUP 2
#define GUI_COLOR_NUM   3

#define GUI_TC_MENU     0
#define GUI_TC_INFO     1
#define GUI_TC_TITLE    2
#define GUI_TC_BUTTON   3
#define GUI_TC_CHECKBOX 4
#define GUI_TC_RADIO    5
#define GUI_TC_ABOUT    6
#define GUI_TC_CBUTTON  7 // custom button
#define GUI_TC_NUM      (GUI_TC_CBUTTON + GUI_BUTTON_NUM)

struct CFG
{
	char background[200];
	char w_background[200];
	char bg_gui[200];
	char bg_gui_wide[200];
	char covers_path[200]; // covers path base
	char covers_path_2d[200];
	int  covers_path_2d_set;
	char covers_path_3d[200];
	char covers_path_disc[200];
	char covers_path_full[200];
	char covers_path_cache[200];
	int layout;
	int covers;
	// game options:
	struct Game_CFG game;
	// misc
    int home;
	int debug;
	int debug_gecko;
	int time_launch;
	int device;
	char partition[16];
	int hide_header;
	// simple variants:
	int confirm_start;
	int hide_hddinfo;
	int hide_footer;
	int console_mark_page;
	int console_mark_favorite;
	int console_mark_saved;
	int disable_format;
	int disable_remove;
	int disable_install;
	int disable_options;
	// end simple
	int install_partitions;
	int fat_install_dir;
	int fat_split_size;
	int ntfs_write;
	// text colors
	int color_header;
	int color_selected_fg;
	int color_selected_bg;
	int color_inactive;
	int color_footer;
	int color_help;
	// music
	int music;
	char music_file[200];
	// widescreen
	int widescreen;
	int W_CONSOLE_XCOORD, W_CONSOLE_YCOORD;
	int W_CONSOLE_WIDTH, W_CONSOLE_HEIGHT;
	int W_COVER_XCOORD, W_COVER_YCOORD;
	int W_COVER_WIDTH, W_COVER_HEIGHT;
	int N_COVER_WIDTH, N_COVER_HEIGHT;
	// hide, pref games
	#define MAX_HIDE_GAME 500
	int num_hide_game;
	char hide_game[MAX_HIDE_GAME][8];
	// preferred games change sort order
	#define MAX_PREF_GAME 64
	int num_pref_game;
	char pref_game[MAX_PREF_GAME][8];
	// favorite games filter the list
	// 32 = 4*8 - one full page with max rows in gui mode
	#define MAX_FAVORITE_GAME 100
	//int num_favorite_game;
	//char favorite_game[MAX_FAVORITE_GAME][8];
	// profiles
	#define MAX_PROFILES 10
	#define PROFILE_NAME_LEN 17
	#define num_favorite_game profile_num_favorite[CFG.current_profile]
	#define favorite_game profile_favorite[CFG.current_profile]
	int num_profiles;
	int current_profile;
	char current_profile_name[PROFILE_NAME_LEN];
	char profile_names[MAX_PROFILES][PROFILE_NAME_LEN];
	int profile_num_favorite[MAX_PROFILES];
	GAMEID_t profile_favorite[MAX_PROFILES][MAX_FAVORITE_GAME];
	int profile_theme[MAX_PROFILES];
	int profile_start_favorites[MAX_PROFILES];
	// sort order ignore list
	char sort_ignore[200];
	// cover urls
	int cover_style;
	//char cover_url[1000];
	char cover_url_2d[1000];
	char cover_url_3d[1000];
	char cover_url_disc[1000];
	char cover_url_full[1000];
	char cover_url_hq[1000];

	// database  options - Lustar
	char db_url[512];
	char db_language[50];
	int db_show_info;
	int db_ignore_titles;
	int write_playstats;
	char sort[20];
	char translation[50];
	
	int download_id_len;
	int download_all;
	char download_cc_pal[4];
	//
	int confirm_ocarina;
	int cursor_jump;
	int console_transparent;
	char cursor[8], cursor_space[8];
	char menu_plus[8], menu_plus_s[8];
	char favorite[8], saved[8];
	//admin lock
	char unlock_password[11];
	int admin_lock;
	int admin_mode_locked;
	//int ios_idx;
	int ios, ios_yal, ios_mload;
	// gui
	int gui;
	int gui_start;
	int gui_menu; // wgui
	int gui_transit;
	int gui_style;
	int gui_rows;
	int gui_lock;
	int gui_title_top;
	struct FontColor gui_text;
	struct FontColor gui_text2;
	struct FontColor_CFG gui_text_cfg;
	struct FontColor_CFG gui_text2_cfg;
	struct FontColor gui_tc[GUI_TC_NUM];
	u32 gui_window_color[GUI_COLOR_NUM];
	char gui_font[100];
	int start_favorites;
	int gui_antialias;
	int gui_compress_covers;
	int clock_style;
	struct RectCoords gui_cover_area;
	struct RectCoords gui_title_area;
	struct PosCoords gui_clock_pos;
	struct PosCoords gui_page_pos;
	int gui_pointer_scroll;
	struct CfgButton gui_button[GUI_BUTTON_NUM];
	int gui_bar;
	// global saved state
	int saved_global;
	char saved_theme[32];
	int saved_device;
	int saved_gui_style;
	int saved_gui_rows;
	char saved_partition[16];
	int saved_profile;
	// loadstructor
	int intro;
	int direct_launch;
	char launch_discid[8];
	//int current_partition;
	int patch_dvd_check;
	int disable_dvd_patch;
	char titles_url[100];
	int disable_nsmb_patch;
	int disable_pop_patch;
	int disable_bca;
	int disable_wip;
	int dml;
	int default_gc_loader;
	int old_button_color;
	// int write_playlog;
	// order of the following options (until specified point) is important
	int button_M;
	int button_P;
	int button_A;
	int button_B;
	int button_H;
	int button_1;
	int button_2;
	int button_X;
	int button_Y;
	int button_Z;
	int button_C;
	int button_L;
	int button_R;
	//order importance ends here

	int button_gui;
	int button_opt;
	int button_fav;
	struct MenuButton button_confirm;
	struct MenuButton button_cancel;
	struct MenuButton button_exit;
	struct MenuButton button_other;
	struct MenuButton button_save;
	int load_unifont;
	int wiird; // wii remote debugger
	u32 return_to;
	int delay_patch;
	//themes
	char theme[32];
	char theme_path[200];
	char nand_emu_path[200];
	char wbfs_fat_dir[128];
	int adult_themes;
	int theme_previews;
	int theme_previewX;
	int theme_previewY;
	int theme_previewW;
	int theme_previewH;
	int w_theme_previewX;
	int w_theme_previewY;
	int w_theme_previewW;
	int w_theme_previewH;
	int select;

	//gamercards
	char gamercard_url[1000];
	char gamercard_key[200];
};

extern struct CFG CFG;

#define CF_TRANS_NONE 0
#define CF_TRANS_ROTATE_RIGHT 1
#define CF_TRANS_ROTATE_LEFT 2
#define CF_TRANS_FLIP_TO_BACK 3
#define CF_TRANS_MOVE_TO_CENTER 4
#define CF_TRANS_MOVE_TO_POSITION 5
#define CF_TRANS_MOVE_TO_CONSOLE 6
#define CF_TRANS_MOVE_TO_CONSOLE_3D 7
#define CF_TRANS_MOVE_FROM_CONSOLE 8
#define CF_TRANS_MOVE_FROM_CONSOLE_3D 9

//coverflow themes
enum coverflow_themes {
	coverflow3d = 0,
	coverflow2d,
	frontrow,
	vertical,
	carousel,
};

//global settings structure for coverflow themes
struct settings_cf_global {
	int number_of_covers;			// total number of games
	int selected_cover;				// currently selected cover
	enum coverflow_themes theme;	// currently selected coverflow theme
	int covers_3d;					// use 3D cover objects
	int screen_fade_alpha;			// alpha level of background when screen is faded
	int frameCount;					// total frame count of rotation
	int frameIndex;					// current frame index of rotation
	int transition;					// current type of transition (rotate right/left, move to front, etc)
	f32 cover_back_xpos;			// cover position when the back cover is being displayed
	f32 cover_back_ypos;			// ''
	f32 cover_back_zpos;			// ''
	f32 cover_back_xrot;			// cover rotation when the back cover is being displayed
	f32 cover_back_yrot;			// ''
	f32 cover_back_zrot;			// ''
};
extern struct settings_cf_global CFG_cf_global;


//structure for the coverflow themes
struct settings_cf_theme {
	int rotation_frames;			// normal-speed rotation frame count
	int rotation_frames_fast;		// fast-speed rotation frame count
	int number_of_side_covers;		// number of side covers for the theme
	u32 reflections_color_bottom;	// color (incl alpha) of the reflection closest to the bottom cover
	u32 reflections_color_top;		// color (incl alpha) of the reflection furthest from the bottom of the cover
	int alpha_rolloff;				// makes the side covers MORE transparent as they span out from the center: 0=disabled
	bool floating_cover;			// makes the center cover "float" as it's standing still
	
	f32 cam_pos_x;					// camera position in 3D space
	f32 cam_pos_y;
	f32 cam_pos_z;
	f32 cam_look_x;					// camera look point
	f32 cam_look_y;
	f32 cam_look_z;
	
	f32 cover_rolloff_x;			// used to make the cover positioning into a circle shape
	f32 cover_rolloff_y;
	f32 cover_rolloff_z;

	f32 title_text_xpos;			// game title position
	f32 title_text_ypos;
	
	f32 cover_center_xpos;			// center cover positioning
	f32 cover_center_ypos;
	f32 cover_center_zpos;
	f32 cover_center_xrot;			// center cover rotation
	f32 cover_center_yrot;
	f32 cover_center_zrot;
	bool cover_center_reflection_used;			// alpha (transparency) level of the center cover 
												//  reflection: 0=no reflection, 255=no transparency 

	f32 cover_distance_from_center_left_x;		// distance between the center cover and the first left side cover
	f32 cover_distance_from_center_left_y;
	f32 cover_distance_from_center_left_z;
	f32 cover_distance_between_covers_left_x;	// distance between the left side covers
	f32 cover_distance_between_covers_left_y;
	f32 cover_distance_between_covers_left_z;
	f32 cover_left_xpos;						// position of left side covers
	f32 cover_left_ypos;
	f32 cover_left_zpos;
	f32 cover_left_xrot;						// left side cover rotation
	f32 cover_left_yrot;
	f32 cover_left_zrot;

	f32 cover_distance_from_center_right_x;		// distance between the center cover and the first right side cover
	f32 cover_distance_from_center_right_y;
	f32 cover_distance_from_center_right_z;
	f32 cover_distance_between_covers_right_x;	// distance between the right side covers
	f32 cover_distance_between_covers_right_y;
	f32 cover_distance_between_covers_right_z;
	f32 cover_right_xpos;						// position of right side covers
	f32 cover_right_ypos;
	f32 cover_right_zpos;
	f32 cover_right_xrot;						// right side cover rotation
	f32 cover_right_yrot;
	f32 cover_right_zrot;
};
#define CF_THEME_NUM 5
extern struct settings_cf_theme CFG_cf_theme[CF_THEME_NUM];

extern int num_theme;
extern int cur_theme;

void CFG_Default();
void CFG_Load(int argc, char **argv);
void CFG_Setup(int argc, char **argv);
bool CFG_Load_Settings();
bool CFG_Save_Settings(int verbose);
bool CFG_Save_Global_Settings();
void cfg_parsearg_early(int argc, char **argv);
int  CFG_MountUSB();

char *cfg_get_title(u8 *id);
char *get_title(struct discHdr *header);

struct Game_CFG_2* CFG_find_game(u8 *id);
struct Game_CFG CFG_read_active_game_setting(u8 *id);
struct Game_CFG_2* CFG_get_game(u8 *id);
bool CFG_is_saved(u8 *id);
bool CFG_is_changed(u8 *id);
bool CFG_save_game_opt(u8 *id);
bool CFG_discard_game_opt(u8 *id);
void CFG_release_game(struct Game_CFG_2 *game);

int CFG_hide_games(struct discHdr *list, int cnt);
void CFG_sort_pref(struct discHdr *list, int cnt);
void CFG_switch_theme(int theme_i);
void cfg_set_cover_style(int style);
void cfg_setup_cover_style();

char *ios_str(int idx);
void cfg_ios(char *name, char *val);
void cfg_ios_set_idx(int ios_idx);
bool is_ios_idx_mload(int ios_idx);
int get_ios_idx_type(int ios_idx);

bool set_favorite(u8 *id, bool fav);
bool is_favorite(u8 *id);

bool set_hide_game(u8 *id, bool hide);
bool is_hide_game(u8 *id);

int CFG_filter_favorite(struct discHdr *list, int cnt);

char *get_clock_str(time_t t);

void set_recommended_cIOS_idx(u8 ios, bool onlyD2x);

int readPlayStats(void);
u32 getPlayCount(u8 *id);
time_t getLastPlay(u8 *id);
int setPlayStat(u8 *id);

char *cfg_get_covers_path(int style);

extern struct TextMap map_video_patch[];
extern struct TextMap map_gui_style[];
extern struct TextMap map_ios[];
extern struct TextMap map_nand_emu[];
extern struct TextMap map_channel_boot[];
extern struct TextMap map_gc_boot[];
extern char *names_vpatch[CFG_VIDEO_PATCH_NUM];
extern u8	  cIOS_base[];

#define NUM_HOOK 8
extern char *hook_name[NUM_HOOK];

#endif
