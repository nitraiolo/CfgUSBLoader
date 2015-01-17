
// by oggzee, usptactical & Dr. Clipper

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <ogcsys.h>
#include <dirent.h>
#include <malloc.h>
#include <sys/types.h>
#include <fcntl.h>

#include "dir.h"
#include "cfg.h"
#include "wpad.h"
#include "gui.h"
#include "video.h"
#include "fat.h"
#include "net.h"
#include "xml.h"
#include "gettext.h"
#include "menu.h"
#include "console.h"
#include "sys.h"
#include "menu.h"
#include "wbfs.h"
#include "wgui.h"

char FAT_DRIVE[8] = SDHC_DRIVE;
char USBLOADER_PATH[200] = "sd:/usb-loader";
char APPS_DIR[200] = "";
char LAST_CFG_PATH[200];
char direct_start_id_buf[] = "#GAMEID\0\0\0\0\0CFGUSB0000000000";
char DIOS_MIOS_INFO[200] = "";

/* configurable fields */

int ENTRIES_PER_PAGE = 12;
int MAX_CHARACTERS   = 30;

int CONSOLE_XCOORD   = 258;
int CONSOLE_YCOORD   = 112;
int CONSOLE_WIDTH    = 354;
int CONSOLE_HEIGHT   = 304;

int CONSOLE_FG_COLOR = 15;
int CONSOLE_BG_COLOR = 0;

int COVER_XCOORD     = 24;
int COVER_YCOORD     = 104;

struct CFG CFG;

int COVER_WIDTH_FRONT;
int COVER_HEIGHT_FRONT;

#define TITLE_MAX 64

struct ID_Title
{
	u8 id[8];
	char title[TITLE_MAX];
	int hnext; // linked list next with same hash
};

// renamed titles
int num_title = 0; //number of titles
struct ID_Title *cfg_title = NULL;
HashTable title_hash_id6;
HashTable title_hash_id4;

#define MAX_CFG_GAME 500
int num_cfg_game = 0;
struct Game_CFG_2 cfg_game[MAX_CFG_GAME];

struct TextMap map_video[] =
{
	{ "system", CFG_VIDEO_SYS },
	{ "game",   CFG_VIDEO_GAME },
	{ "auto",   CFG_VIDEO_AUTO },
	{ "pal50",  CFG_VIDEO_PAL50 },
	{ "pal60",  CFG_VIDEO_PAL60 },
	{ "ntsc",   CFG_VIDEO_NTSC },
	{ "pal480p",CFG_VIDEO_NUM },
	{ NULL, -1 }
};

struct TextMap map_video_patch[] =
{
	{ "0", CFG_VIDEO_PATCH_OFF },
	{ "1", CFG_VIDEO_PATCH_ON  },
	{ "all", CFG_VIDEO_PATCH_ALL },
	{ "sneek", CFG_VIDEO_PATCH_SNEEK },
	{ "sneek+all", CFG_VIDEO_PATCH_SNEEK_ALL },
	{ NULL, -1 }
};

char *names_vpatch[CFG_VIDEO_PATCH_NUM] =
{
	gts("Off"),
	gts("On"),
	gts("All"),
	"sneek",
	"sneek+all",
};

struct TextMap map_language[] =
{
	{ "console",   CFG_LANG_CONSOLE },
	{ "japanese",  CFG_LANG_JAPANESE },
	{ "english",   CFG_LANG_ENGLISH },
	{ "german",    CFG_LANG_GERMAN },
	{ "french",    CFG_LANG_FRENCH },
	{ "spanish",   CFG_LANG_SPANISH },
	{ "italian",   CFG_LANG_ITALIAN },
	{ "dutch",     CFG_LANG_DUTCH },
	{ "s.chinese", CFG_LANG_S_CHINESE },
	{ "t.chinese", CFG_LANG_T_CHINESE },
	{ "korean",    CFG_LANG_KOREAN },
	{ NULL, -1 }
};

struct TextMap map_layout[] =
{
	{ "original",  CFG_LAYOUT_ORIG },
	{ "original1", CFG_LAYOUT_ORIG },
	{ "original2", CFG_LAYOUT_ORIG_12 },
	{ "small",     CFG_LAYOUT_SMALL },
	{ "medium",    CFG_LAYOUT_MEDIUM },
	{ "large",     CFG_LAYOUT_LARGE },
	{ "large2",    CFG_LAYOUT_LARGE_2 },
	{ "large3",    CFG_LAYOUT_LARGE_3 },
	{ "ultimate1", CFG_LAYOUT_ULTIMATE1 },
	{ "ultimate2", CFG_LAYOUT_ULTIMATE2 },
	{ "ultimate3", CFG_LAYOUT_ULTIMATE3 },
	{ "kosaic",    CFG_LAYOUT_KOSAIC },
	{ NULL, -1 }
};

struct TextMap map_ios[] =
{
	{ "245",        CFG_IOS_245 },
	{ "246",        CFG_IOS_246 },
	{ "247",        CFG_IOS_247 },
	{ "248",        CFG_IOS_248 },
	{ "249",        CFG_IOS_249 },
	{ "250",        CFG_IOS_250 },
	{ "251",        CFG_IOS_251 },
	{ "222-mload",  CFG_IOS_222_MLOAD },
	{ "223-mload",  CFG_IOS_223_MLOAD },
	{ "224-mload",  CFG_IOS_224_MLOAD },
	{ "222-yal",    CFG_IOS_222_YAL },
	{ "223-yal",    CFG_IOS_223_YAL },
	{ "Auto",       CFG_IOS_AUTO },
	{ NULL, -1 }
};

u8 cIOS_base[10];

struct TextMap map_gui_style[] =
{
	{ "grid",        GUI_STYLE_GRID },
	{ "flow",        GUI_STYLE_FLOW },
	{ "flow-z",      GUI_STYLE_FLOW_Z },
	{ "coverflow3d", GUI_STYLE_COVERFLOW + coverflow3d },
	{ "coverflow2d", GUI_STYLE_COVERFLOW + coverflow2d },
	{ "frontrow",    GUI_STYLE_COVERFLOW + frontrow },
	{ "vertical",    GUI_STYLE_COVERFLOW + vertical },
	{ "carousel",    GUI_STYLE_COVERFLOW + carousel },
	{ NULL, -1 }
};

struct TextMap map_button[] =
{
	{ "-",          CFG_BTN_M },
	{ "+",          CFG_BTN_P },
	{ "A",          CFG_BTN_A },
	{ "B",          CFG_BTN_B },
	{ "H",          CFG_BTN_H },
	{ "1",          CFG_BTN_1 },
	{ "2",          CFG_BTN_2 },
	{ "nothing",    CFG_BTN_NOTHING },
	{ "options",    CFG_BTN_OPTIONS },
	{ "gui",        CFG_BTN_GUI },
	{ "reboot",     CFG_BTN_REBOOT },
	{ "exit",       CFG_BTN_EXIT },
	{ "hbc",        CFG_BTN_HBC },
	{ "screenshot", CFG_BTN_SCREENSHOT },
	{ "install",    CFG_BTN_INSTALL },
	{ "remove",     CFG_BTN_REMOVE },
	{ "main_menu",  CFG_BTN_MAIN_MENU },
	{ "global_ops", CFG_BTN_GLOBAL_OPS },
	{ "favorites",  CFG_BTN_FAVORITES },
	{ "boot_game",  CFG_BTN_BOOT_GAME },
	{ "boot_disc",  CFG_BTN_BOOT_DISC },
	{ "theme",      CFG_BTN_THEME },
	{ "profile",    CFG_BTN_PROFILE },
	{ "unlock",     CFG_BTN_UNLOCK },
	{ "sort",       CFG_BTN_SORT },
	{ "filter",     CFG_BTN_FILTER },
	{ "priiloader", CFG_BTN_PRIILOADER },
	{ "channel",    CFG_BTN_CHANNEL },
	{ "wii_menu",   CFG_BTN_WII_MENU },
	{ "random",     CFG_BTN_RANDOM },
	{ NULL, -1 }
};

struct TextMap map_button_menu[] =
{
	//{ "A", NUM_BUTTON_A },
	{ "B", NUM_BUTTON_B },
	{ "1", NUM_BUTTON_1 },
	{ "2", NUM_BUTTON_2 },
	{ "-", NUM_BUTTON_MINUS },
	{ "M", NUM_BUTTON_MINUS },
	{ "+", NUM_BUTTON_PLUS },
	{ "P", NUM_BUTTON_PLUS },
	{ "H", NUM_BUTTON_HOME },
	{ "X", NUM_BUTTON_X },
	{ "Y", NUM_BUTTON_Y },
	{ "Z", NUM_BUTTON_Z },
	{ "C", NUM_BUTTON_C },
	{ "L", NUM_BUTTON_L },
	{ "R", NUM_BUTTON_R },
	{ NULL, -1 }
};

struct TextMap map_hook[] =
{
	{ "nohooks", 0 },
	{ "vbi",     1 },
	{ "wiipad",  2 },
	{ "gcpad",   3 },
	{ "gxdraw",  4 },
	{ "gxflush", 5 },
	{ "ossleep", 6 },
	{ "axframe", 7 },
	{ NULL, -1 }
};

char *hook_name[NUM_HOOK] =
{
	"No Hooks",
	"VBI",
	"Wii Pad",
	"GC Pad",
	"GXDraw",
	"GXFlush",
	"OSSleepThread",
	"AXNextFrame"
};

struct TextMap map_nand_emu[] =
{
	{ "off", 0 },
	{ "partitial", 1  },
	{ "full", 2 },
	{ NULL, -1 }
};

struct TextMap map_boot_method[] =	//combination of map_channel_boot and map_gc_boot
{
	{ "Mighty Plugin", 0 },
	{ "Neek2o Plugin", 1  },
	{ "Default", 0 },
	{ "DIOS MIOS", 1  },
	{ "Devolution", 2  },
	{ "Nintendont", 3  },
	{ NULL, -1 }
};

struct TextMap map_channel_boot[] =
{
	{ "Mighty Plugin", 0 },
	{ "Neek2o Plugin", 1  },
	{ NULL, -1 }
};

struct TextMap map_gc_boot[] =
{
	{ "Default", 0 },
	{ "DIOS MIOS", 1  },
	{ "Devolution", 2  },
	{ "Nintendont", 3  },
	{ NULL, -1 }
};

struct playStat {
	char id[7];
	s32 playCount;
	time_t playTime;
};

bool playStatsRead = false;
int playStatsSize = 0;
struct playStat *playStats;

int CFG_IOS_MAX = MAP_NUM(map_ios) - 1;
int CURR_IOS_IDX = -1;



// theme
//#define MAX_THEME 100
int num_theme = 0;
int cur_theme = -1;
char theme_list[MAX_THEME][MAX_THEME_NAME];
char theme_path[200];

void game_set(char *name, char *val);
void load_theme(char *theme);
void cfg_setup1();
void cfg_setup2();
void cfg_setup3();
void cfg_direct_start(int argc, char **argv);


void cfg_layout()
{
	// CFG_LAYOUT_ORIG (1.0 - base for most)
	ENTRIES_PER_PAGE = 6;
	MAX_CHARACTERS   = 30;
	CONSOLE_XCOORD   = 260;
	CONSOLE_YCOORD   = 115;
	CONSOLE_WIDTH    = 340;
	CONSOLE_HEIGHT   = 218;
	CONSOLE_FG_COLOR = 15;
	CONSOLE_BG_COLOR = 0;
	// orig 1.2
	COVER_XCOORD     = 24;
	COVER_YCOORD     = 104;
	//COVER_WIDTH      = 160; // set by cover_style
	//COVER_HEIGHT     = 225; // set by cover_style
	// widescreen
	CFG.W_CONSOLE_XCOORD = CONSOLE_XCOORD;
	CFG.W_CONSOLE_YCOORD = CONSOLE_YCOORD;
	CFG.W_CONSOLE_WIDTH  = CONSOLE_WIDTH;
	CFG.W_CONSOLE_HEIGHT = CONSOLE_HEIGHT;
	CFG.W_COVER_XCOORD   = COVER_XCOORD;
	CFG.W_COVER_YCOORD   = COVER_YCOORD;
	//CFG.W_COVER_WIDTH  = 0; // auto
	//CFG.W_COVER_HEIGHT = 0; // auto 
	switch (CFG.layout) {

		case CFG_LAYOUT_ORIG: // 1.0+
			ENTRIES_PER_PAGE = 6;
			MAX_CHARACTERS   = 30;
			CONSOLE_XCOORD   = 260;
			CONSOLE_YCOORD   = 115;
			CONSOLE_WIDTH    = 340;
			CONSOLE_HEIGHT   = 218;
			CONSOLE_FG_COLOR = 15;
			CONSOLE_BG_COLOR = 0;
			break;

		case CFG_LAYOUT_ORIG_12: // 1.2+
			ENTRIES_PER_PAGE = 12;
			MAX_CHARACTERS   = 30;
			CONSOLE_XCOORD   = 258;
			CONSOLE_YCOORD   = 112;
			CONSOLE_WIDTH    = 354;
			CONSOLE_HEIGHT   = 304;
			CONSOLE_FG_COLOR = 15;
			CONSOLE_BG_COLOR = 0;
			COVER_XCOORD     = 24;
			COVER_YCOORD     = 104;
			break;

		case CFG_LAYOUT_SMALL: // same as 1.0 + use more space
			ENTRIES_PER_PAGE = 9;
			MAX_CHARACTERS   = 38;
			break;

		case CFG_LAYOUT_MEDIUM:
			ENTRIES_PER_PAGE = 19; //17;
			MAX_CHARACTERS   = 38;
			CONSOLE_YCOORD   = 50;
			CONSOLE_HEIGHT   = 380;
			COVER_XCOORD     = 28;
			COVER_YCOORD     = 175;
			break;

		// nixx:
		case CFG_LAYOUT_LARGE:
			ENTRIES_PER_PAGE = 21;
			MAX_CHARACTERS   = 38;
			CONSOLE_YCOORD   = 40;
			CONSOLE_HEIGHT   = 402;
			COVER_XCOORD     = 28;
			COVER_YCOORD     = 175;
			break;

		// usptactical:
		case CFG_LAYOUT_LARGE_2:
			ENTRIES_PER_PAGE = 21;
			MAX_CHARACTERS   = 38;
			CONSOLE_YCOORD   = 40;
			CONSOLE_HEIGHT   = 402;
			COVER_XCOORD     = 30;
			COVER_YCOORD     = 180;
			break;

		// oggzee
		case CFG_LAYOUT_LARGE_3:
			ENTRIES_PER_PAGE = 21;
			MAX_CHARACTERS   = 38;
			CONSOLE_YCOORD   = 40;
			CONSOLE_WIDTH    = 344;
			CONSOLE_HEIGHT   = 402;
			COVER_XCOORD     = 42;
			COVER_YCOORD     = 102;
			CFG.W_CONSOLE_XCOORD = CONSOLE_XCOORD;
			CFG.W_CONSOLE_YCOORD = CONSOLE_YCOORD;
			CFG.W_CONSOLE_WIDTH  = CONSOLE_WIDTH;
			CFG.W_CONSOLE_HEIGHT = CONSOLE_HEIGHT;
			CFG.W_COVER_XCOORD   = COVER_XCOORD+14;
			CFG.W_COVER_YCOORD   = COVER_YCOORD;
			break;

		// Ultimate1: (WiiShizza) white background
		case CFG_LAYOUT_ULTIMATE1:
			ENTRIES_PER_PAGE = 12;
			MAX_CHARACTERS   = 37;
			CONSOLE_YCOORD   = 30;
			CONSOLE_HEIGHT   = 290;
			CONSOLE_FG_COLOR = 0;
			CONSOLE_BG_COLOR = 15;
			COVER_XCOORD     = 28;
			COVER_YCOORD     = 105;
			break;

		// Ultimate2: (jservs7 / hungyip84)
		case CFG_LAYOUT_ULTIMATE2:
			ENTRIES_PER_PAGE = 12;
			MAX_CHARACTERS   = 37;
			CONSOLE_YCOORD   = 30;
			CONSOLE_HEIGHT   = 290;
			CONSOLE_FG_COLOR = 15;
			CONSOLE_BG_COLOR = 0;
			COVER_XCOORD     = 28;
			COVER_YCOORD     = 105;
			break;

		// Ultimate3: (WiiShizza) white background, covers on right
		case CFG_LAYOUT_ULTIMATE3:
			ENTRIES_PER_PAGE = 12;
			MAX_CHARACTERS   = 37;
			CONSOLE_XCOORD   = 40;
			CONSOLE_YCOORD   = 71;
			CONSOLE_WIDTH    = 344;
			CONSOLE_HEIGHT   = 290;
			CONSOLE_FG_COLOR = 0;
			CONSOLE_BG_COLOR = 15;
			COVER_XCOORD     = 446;
			COVER_YCOORD     = 109;
			CFG.W_CONSOLE_XCOORD = CONSOLE_XCOORD;
			CFG.W_CONSOLE_YCOORD = CONSOLE_YCOORD;
			CFG.W_CONSOLE_WIDTH  = CONSOLE_WIDTH;
			CFG.W_CONSOLE_HEIGHT = CONSOLE_HEIGHT;
			CFG.W_COVER_XCOORD   = 482;
			CFG.W_COVER_YCOORD   = 110;
			break;

		// Kosaic (modified U3)
		case CFG_LAYOUT_KOSAIC:
			ENTRIES_PER_PAGE = 12;
			MAX_CHARACTERS   = 37;
			CONSOLE_XCOORD   = 40;
			CONSOLE_YCOORD   = 71;
			CONSOLE_WIDTH    = 344;
			CONSOLE_HEIGHT   = 290;
			CONSOLE_FG_COLOR = 15;
			CONSOLE_BG_COLOR = 0;
			COVER_XCOORD     = 446;
			COVER_YCOORD     = 109;
			CFG.W_CONSOLE_XCOORD = 34;
			CFG.W_CONSOLE_YCOORD = 71;
			CFG.W_CONSOLE_WIDTH  = CONSOLE_WIDTH;
			CFG.W_CONSOLE_HEIGHT = CONSOLE_HEIGHT;
			CFG.W_COVER_XCOORD   = 444;
			CFG.W_COVER_YCOORD   = 109;
			break;

	}
}


void set_colors(int x)
{
	switch(x) {
		case CFG_COLORS_MONO:
			CFG.color_header =
			CFG.color_inactive =
			CFG.color_footer =
			CFG.color_help =
			CFG.color_selected_bg = CONSOLE_FG_COLOR;
			CFG.color_selected_fg = CONSOLE_BG_COLOR;
			break;

		case CFG_COLORS_DARK:
			CFG.color_header = 14;
			CFG.color_selected_fg = 11;
			CFG.color_selected_bg = 12;
			CFG.color_inactive = 8;
			CFG.color_footer = 6;
			CFG.color_help = 2;
			break;

		case CFG_COLORS_BRIGHT:
			CFG.color_header = 6;
			CFG.color_selected_fg = 11;
			CFG.color_selected_bg = 12;
			CFG.color_inactive = 7;
			CFG.color_footer = 4;
			CFG.color_help = 2;
			break;
	}
}

void cfg_set_covers_path()
{
	//STRCOPY(CFG.covers_path_2d, CFG.covers_path);
	snprintf(D_S(CFG.covers_path_2d), "%s/%s", CFG.covers_path, "2d");
	snprintf(D_S(CFG.covers_path_3d), "%s/%s", CFG.covers_path, "3d");
	snprintf(D_S(CFG.covers_path_disc), "%s/%s", CFG.covers_path, "disc");
	snprintf(D_S(CFG.covers_path_full), "%s/%s", CFG.covers_path, "full");
	snprintf(D_S(CFG.covers_path_cache), "%s/%s", CFG.covers_path, "cache");
	CFG.covers_path_2d_set = 0;
}

char *cfg_get_covers_path(int style)
{
	switch (style) {
		default:
		case CFG_COVER_STYLE_2D:
			if (CFG.covers_path_2d_set) {
				return CFG.covers_path_2d;
			} else {
				return CFG.covers_path;
			}

		case CFG_COVER_STYLE_3D:
			return CFG.covers_path_3d;

		case CFG_COVER_STYLE_DISC:
			return CFG.covers_path_disc;

		case CFG_COVER_STYLE_FULL:
		case CFG_COVER_STYLE_HQ:
			return CFG.covers_path_full;

		case CFG_COVER_STYLE_CACHE:
			return CFG.covers_path_cache;
	}
	return NULL;
}


void cfg_set_cover_style(int style)
{
	CFG.cover_style = style;
	CFG.W_COVER_WIDTH  = 0; // auto
	CFG.W_COVER_HEIGHT = 0; // auto 
	switch (style) {
		case CFG_COVER_STYLE_2D:
			COVER_WIDTH = 160;
			COVER_HEIGHT = 225;
			break;
		case CFG_COVER_STYLE_3D:
			COVER_WIDTH = 160;
			COVER_HEIGHT = 225;
			break;
		case CFG_COVER_STYLE_DISC:
			COVER_WIDTH = 160;
			COVER_HEIGHT = 160;
			break;
		case CFG_COVER_STYLE_FULL:
			COVER_WIDTH = 512;
			if (CFG.gui_compress_covers)
				COVER_HEIGHT = 336;
			else
				COVER_HEIGHT = 340;
			//Calculate the width of the front cover only, given the cover dimensions above.
			// Divide the height by 1.4, which is the 2d cover height:width ratio (225x160 = 1.4)
			// and then divide by 4 and multiply by 4 so it's divisible.  This makes it easier if
			// we want to change the fullcover dimensions in the future.
			COVER_WIDTH_FRONT = (int)(COVER_HEIGHT / 1.4) >> 2 << 2;
			COVER_HEIGHT_FRONT = COVER_HEIGHT;
			break;
	}
}

void cfg_set_cover_style_str(char *val)
{
	if (strcmp(val, "standard")==0) {
		cfg_set_cover_style(CFG_COVER_STYLE_2D);
	} else if (strcmp(val, "3d")==0) {
		cfg_set_cover_style(CFG_COVER_STYLE_3D);
	} else if (strcmp(val, "disc")==0) {
		cfg_set_cover_style(CFG_COVER_STYLE_DISC);
	} else if (strcmp(val, "full")==0) {
		cfg_set_cover_style(CFG_COVER_STYLE_FULL);
	}
}

// setup cover style: path & url
/*
void cfg_setup_cover_style()
{
	switch (CFG.cover_style) {
		case CFG_COVER_STYLE_3D:
			STRCOPY(CFG.covers_path, CFG.covers_path_3d); 
			STRCOPY(CFG.cover_url, CFG.cover_url_3d);
			break;
		case CFG_COVER_STYLE_DISC:
			STRCOPY(CFG.covers_path, CFG.covers_path_disc); 
			STRCOPY(CFG.cover_url, CFG.cover_url_disc);
			break;
		case CFG_COVER_STYLE_FULL:
			STRCOPY(CFG.covers_path, CFG.covers_path_full);
			STRCOPY(CFG.cover_url, CFG.cover_url_full);
			break;
		default:
		case CFG_COVER_STYLE_2D:
			STRCOPY(CFG.covers_path, CFG.covers_path_2d); 
			STRCOPY(CFG.cover_url, CFG.cover_url_2d);
			break;
	}
}
*/

void cfg_default_url()
{
	CFG.download_id_len = 6;
	CFG.download_all = 1;
	//strcpy(CFG.download_cc_pal, "EN");
	*CFG.download_cc_pal = 0; // means AUTO - console language

	*CFG.cover_url_2d = 0;
	*CFG.cover_url_3d = 0;
	*CFG.cover_url_disc = 0;
	*CFG.cover_url_full = 0;
	*CFG.cover_url_hq = 0;

	STRCOPY(CFG.cover_url_2d,
		" http://art.gametdb.com/wii/cover/{CC}/{ID6}.png"
		//" http://www.wiiboxart.com/artwork/cover/{ID6}.png"
		//" http://boxart.rowdyruff.net/flat/{ID6}.png"
		//" http://www.muntrue.nl/covers/ALL/160/225/boxart/{ID6}.png"
		//" http://wiicover.gateflorida.com/sites/default/files/cover/2D%20Cover/{ID6}.png"
		//" http://awiibit.com/BoxArt160x224/{ID6}.png"
		);

	STRCOPY(CFG.cover_url_3d,
		" http://art.gametdb.com/wii/cover3D/{CC}/{ID6}.png"
		//" http://www.wiiboxart.com/artwork/cover3D/{ID6}.png"
		//" http://boxart.rowdyruff.net/3d/{ID6}.png"
		//" http://www.muntrue.nl/covers/ALL/160/225/3D/{ID6}.png"
		//" http://wiicover.gateflorida.com/sites/default/files/cover/3D%20Cover/{ID6}.png"
		//" http://awiibit.com/3dBoxArt176x248/{ID6}.png"
		);

	STRCOPY(CFG.cover_url_disc,
		" http://art.gametdb.com/wii/disc/{CC}/{ID6}.png"
		" http://art.gametdb.com/wii/disccustom/{CC}/{ID6}.png"
		//" http://www.wiiboxart.com/artwork/disc/{ID6}.png"
		//" http://www.wiiboxart.com/artwork/disccustom/{ID6}.png"
		//" http://boxart.rowdyruff.net/fulldisc/{ID6}.png"
		//" http://www.muntrue.nl/covers/ALL/160/160/disc/{ID6}.png"
		//" http://wiicover.gateflorida.com/sites/default/files/cover/Disc%20Cover/{ID6}.png"
		//" http://awiibit.com/WiiDiscArt/{ID6}.png"
		);

	STRCOPY(CFG.cover_url_full,
		" http://art.gametdb.com/wii/coverfull/{CC}/{ID6}.png"
		//" http://www.wiiboxart.com/artwork/coverfull/{ID6}.png"
		//" http://www.muntrue.nl/covers/ALL/512/340/fullcover/{ID6}.png"
		//" http://wiicover.gateflorida.com/sites/default/files/cover/Full%20Cover/{ID6}.png"
		);

	STRCOPY(CFG.cover_url_hq,
		" http://art.gametdb.com/wii/coverfullHQ/{CC}/{ID6}.png"
		);

	STRCOPY(CFG.gamercard_url,
		" http://www.wiinnertag.com/wiinnertag_scripts/update_sign.php?key={KEY}&game_id={ID6}"
		" http://www.messageboardchampion.com/ncard/API/?cmd=tdbupdate&key={KEY}&game={ID6}"
		" http://tag.darkumbra.net/{KEY}.update={ID6}"
		);

	*CFG.gamercard_key = 0;
}


/**
 * Method that populates the coverflow global settings structure
 *  @return void
 */
void CFG_Default_Coverflow()
{
	CFG_cf_global.covers_3d = true;
	CFG_cf_global.number_of_covers = 7;
	CFG_cf_global.selected_cover = 1;
	CFG_cf_global.theme = coverflow3d;		// default to coverflow 3D
	
	CFG_cf_global.cover_back_xpos = 0;
	CFG_cf_global.cover_back_ypos = 0;
	CFG_cf_global.cover_back_zpos = -45;
	CFG_cf_global.cover_back_xrot = 0;
	CFG_cf_global.cover_back_yrot = 0;
	CFG_cf_global.cover_back_zrot = 0;
	CFG_cf_global.screen_fade_alpha = 210;
}

/**
 * Method that populates the coverflow theme settings structure
 *  @return void
 */
void CFG_Default_Coverflow_Themes()
{
	int i = 0;
	
	//allocate the structure array
	//SAFE_FREE(CFG_cf_theme);
	//CFG_cf_theme = malloc(5 * sizeof *CFG_cf_theme);
	
	// coverflow 3D
		CFG_cf_theme[i].rotation_frames = 40;
		CFG_cf_theme[i].rotation_frames_fast = 7;
		CFG_cf_theme[i].cam_pos_x = 0.0;
		CFG_cf_theme[i].cam_pos_y = 0.0;
		CFG_cf_theme[i].cam_pos_z = 0.0;
		CFG_cf_theme[i].cam_look_x = 0.0;
		CFG_cf_theme[i].cam_look_y = 0.0;
		CFG_cf_theme[i].cam_look_z = -1.0;
		if (CFG.widescreen)
			CFG_cf_theme[i].number_of_side_covers = 4;
		else
			CFG_cf_theme[i].number_of_side_covers = 3;
		CFG_cf_theme[i].reflections_color_bottom = 0x666666FF;
		CFG_cf_theme[i].reflections_color_top = 0xAAAAAA33;
		CFG_cf_theme[i].alpha_rolloff = 0;
		CFG_cf_theme[i].floating_cover = false;
		CFG_cf_theme[i].title_text_xpos = -1;
		CFG_cf_theme[i].title_text_ypos = 434;
		CFG_cf_theme[i].cover_rolloff_x = 0;
		CFG_cf_theme[i].cover_rolloff_y = 0;
		CFG_cf_theme[i].cover_rolloff_z = 0;
		CFG_cf_theme[i].cover_center_xpos = 0;
		CFG_cf_theme[i].cover_center_ypos = 0;
		CFG_cf_theme[i].cover_center_zpos = -70;
		CFG_cf_theme[i].cover_center_xrot = 0;
		CFG_cf_theme[i].cover_center_yrot = 0;
		CFG_cf_theme[i].cover_center_zrot = 0;
		CFG_cf_theme[i].cover_center_reflection_used = true;
		CFG_cf_theme[i].cover_distance_from_center_left_x = -21;
		CFG_cf_theme[i].cover_distance_from_center_left_y = 0;
		CFG_cf_theme[i].cover_distance_from_center_left_z = 0;
		CFG_cf_theme[i].cover_distance_between_covers_left_x = -10;
		CFG_cf_theme[i].cover_distance_between_covers_left_y = 0;
		CFG_cf_theme[i].cover_distance_between_covers_left_z = 0;
		CFG_cf_theme[i].cover_left_xpos = 0;
		CFG_cf_theme[i].cover_left_ypos = 0;
		CFG_cf_theme[i].cover_left_zpos = -93;
		CFG_cf_theme[i].cover_left_xrot = 0;
		CFG_cf_theme[i].cover_left_yrot = 70;
		CFG_cf_theme[i].cover_left_zrot = 0;
		CFG_cf_theme[i].cover_distance_from_center_right_x = 21;
		CFG_cf_theme[i].cover_distance_from_center_right_y = 0;
		CFG_cf_theme[i].cover_distance_from_center_right_z = 0;
		CFG_cf_theme[i].cover_distance_between_covers_right_x = 10;
		CFG_cf_theme[i].cover_distance_between_covers_right_y = 0;
		CFG_cf_theme[i].cover_distance_between_covers_right_z = 0;
		CFG_cf_theme[i].cover_right_xpos = 0;
		CFG_cf_theme[i].cover_right_ypos = 0;
		CFG_cf_theme[i].cover_right_zpos = -93;
		CFG_cf_theme[i].cover_right_xrot = 0;
		CFG_cf_theme[i].cover_right_yrot = -70;
		CFG_cf_theme[i].cover_right_zrot = 0;
			
	
	// coverflow 2D
		i++;
		CFG_cf_theme[i].rotation_frames = 40;
		CFG_cf_theme[i].rotation_frames_fast = 7;
		CFG_cf_theme[i].cam_pos_x = 0.0;
		CFG_cf_theme[i].cam_pos_y = 0.0;
		CFG_cf_theme[i].cam_pos_z = 0.0;
		CFG_cf_theme[i].cam_look_x = 0.0;
		CFG_cf_theme[i].cam_look_y = 0.0;
		CFG_cf_theme[i].cam_look_z = -1.0;
		if (CFG.widescreen)
			CFG_cf_theme[i].number_of_side_covers = 4;
		else
			CFG_cf_theme[i].number_of_side_covers = 3;
		CFG_cf_theme[i].reflections_color_bottom = 0x666666FF;
		CFG_cf_theme[i].reflections_color_top = 0xAAAAAA33;
		CFG_cf_theme[i].alpha_rolloff = 0;
		CFG_cf_theme[i].floating_cover = false;
		CFG_cf_theme[i].title_text_xpos = -1;
		CFG_cf_theme[i].title_text_ypos = 434;
		CFG_cf_theme[i].cover_rolloff_x = 0;
		CFG_cf_theme[i].cover_rolloff_y = 0;
		CFG_cf_theme[i].cover_rolloff_z = 0;
		CFG_cf_theme[i].cover_center_xpos = 0;
		CFG_cf_theme[i].cover_center_ypos = 0;
		CFG_cf_theme[i].cover_center_zpos = -70;
		CFG_cf_theme[i].cover_center_xrot = 0;
		CFG_cf_theme[i].cover_center_yrot = 0;
		CFG_cf_theme[i].cover_center_zrot = 0;
		CFG_cf_theme[i].cover_center_reflection_used = true;
		CFG_cf_theme[i].cover_distance_from_center_left_x = -36;
		CFG_cf_theme[i].cover_distance_from_center_left_y = 0;
		CFG_cf_theme[i].cover_distance_from_center_left_z = 0;
		CFG_cf_theme[i].cover_distance_between_covers_left_x = -22;
		CFG_cf_theme[i].cover_distance_between_covers_left_y = 0;
		CFG_cf_theme[i].cover_distance_between_covers_left_z = 0;
		CFG_cf_theme[i].cover_left_xpos = 0;
		CFG_cf_theme[i].cover_left_ypos = 0;
		CFG_cf_theme[i].cover_left_zpos = -167;
		CFG_cf_theme[i].cover_left_xrot = 0;
		CFG_cf_theme[i].cover_left_yrot = 0;
		CFG_cf_theme[i].cover_left_zrot = 0;
		CFG_cf_theme[i].cover_distance_from_center_right_x = 36;
		CFG_cf_theme[i].cover_distance_from_center_right_y = 0;
		CFG_cf_theme[i].cover_distance_from_center_right_z = 0;
		CFG_cf_theme[i].cover_distance_between_covers_right_x = 22;
		CFG_cf_theme[i].cover_distance_between_covers_right_y = 0;
		CFG_cf_theme[i].cover_distance_between_covers_right_z = 0;
		CFG_cf_theme[i].cover_right_xpos = 0;
		CFG_cf_theme[i].cover_right_ypos = 0;
		CFG_cf_theme[i].cover_right_zpos = -167;
		CFG_cf_theme[i].cover_right_xrot = 0;
		CFG_cf_theme[i].cover_right_yrot = 0;
		CFG_cf_theme[i].cover_right_zrot = 0;
		
	// frontrow
		i++;
		CFG_cf_theme[i].rotation_frames = 40;
		CFG_cf_theme[i].rotation_frames_fast = 7;
		CFG_cf_theme[i].cam_pos_x = 0.0;
		CFG_cf_theme[i].cam_pos_y = 0.0;
		CFG_cf_theme[i].cam_pos_z = 0.0;
		CFG_cf_theme[i].cam_look_x = 0.0;
		CFG_cf_theme[i].cam_look_y = 0.0;
		CFG_cf_theme[i].cam_look_z = -1.0;
		CFG_cf_theme[i].number_of_side_covers = 4;
		CFG_cf_theme[i].reflections_color_bottom = 0x666666FF;
		CFG_cf_theme[i].reflections_color_top = 0xAAAAAA33;
		CFG_cf_theme[i].alpha_rolloff = 0;
		CFG_cf_theme[i].floating_cover = false;
		CFG_cf_theme[i].title_text_xpos = -1;
		CFG_cf_theme[i].title_text_ypos = 434;
		CFG_cf_theme[i].cover_rolloff_x = 0;
		CFG_cf_theme[i].cover_rolloff_y = 0;
		CFG_cf_theme[i].cover_rolloff_z = 0;
		CFG_cf_theme[i].cover_center_xpos = 0;
		CFG_cf_theme[i].cover_center_ypos = 0;
		CFG_cf_theme[i].cover_center_zpos = -70;
		CFG_cf_theme[i].cover_center_xrot = 0;
		CFG_cf_theme[i].cover_center_yrot = 0;
		CFG_cf_theme[i].cover_center_zrot = 0;
		CFG_cf_theme[i].cover_center_reflection_used = true;
		CFG_cf_theme[i].cover_distance_from_center_left_x = -27;
		CFG_cf_theme[i].cover_distance_from_center_left_y = 0;
		CFG_cf_theme[i].cover_distance_from_center_left_z = 47;
		CFG_cf_theme[i].cover_distance_between_covers_left_x = -35;
		CFG_cf_theme[i].cover_distance_between_covers_left_y = 0;
		CFG_cf_theme[i].cover_distance_between_covers_left_z = 80;
		CFG_cf_theme[i].cover_left_xpos = 0;
		CFG_cf_theme[i].cover_left_ypos = 0;
		CFG_cf_theme[i].cover_left_zpos = -87;
		CFG_cf_theme[i].cover_left_xrot = 0;
		CFG_cf_theme[i].cover_left_yrot = 0;
		CFG_cf_theme[i].cover_left_zrot = 0;
		if (CFG.widescreen) {
			CFG_cf_theme[i].cover_distance_from_center_right_x = 37;
			CFG_cf_theme[i].cover_distance_from_center_right_y = 0;
			CFG_cf_theme[i].cover_distance_from_center_right_z = -30;
			CFG_cf_theme[i].cover_distance_between_covers_right_x = 45;
			CFG_cf_theme[i].cover_distance_between_covers_right_y = 0;
			CFG_cf_theme[i].cover_distance_between_covers_right_z = -50;
		} else {
			CFG_cf_theme[i].cover_distance_from_center_right_x = 34;
			CFG_cf_theme[i].cover_distance_from_center_right_y = 0;
			CFG_cf_theme[i].cover_distance_from_center_right_z = -37;
			CFG_cf_theme[i].cover_distance_between_covers_right_x = 50;
			CFG_cf_theme[i].cover_distance_between_covers_right_y = 0;
			CFG_cf_theme[i].cover_distance_between_covers_right_z = -80;
		}
		CFG_cf_theme[i].cover_right_xpos = 0;
		CFG_cf_theme[i].cover_right_ypos = 0;
		CFG_cf_theme[i].cover_right_zpos = -87;
		CFG_cf_theme[i].cover_right_xrot = 0;
		CFG_cf_theme[i].cover_right_yrot = 0;
		CFG_cf_theme[i].cover_right_zrot = 0;
		
	// vertical
		i++;
		CFG_cf_theme[i].rotation_frames = 40;
		CFG_cf_theme[i].rotation_frames_fast = 7;
		CFG_cf_theme[i].cam_pos_x = 0.0;
		CFG_cf_theme[i].cam_pos_y = 0.0;
		CFG_cf_theme[i].cam_pos_z = 0.0;
		CFG_cf_theme[i].cam_look_x = 0.0;
		CFG_cf_theme[i].cam_look_y = 0.0;
		CFG_cf_theme[i].cam_look_z = -1.0;
		CFG_cf_theme[i].number_of_side_covers = 4;
		CFG_cf_theme[i].reflections_color_bottom = 0xFFFFFF00;
		CFG_cf_theme[i].reflections_color_top = 0xFFFFFF00;
		CFG_cf_theme[i].alpha_rolloff = 0;
		CFG_cf_theme[i].floating_cover = true;
		CFG_cf_theme[i].title_text_xpos = -1;
		CFG_cf_theme[i].title_text_ypos = 434;
		CFG_cf_theme[i].cover_rolloff_x = 0;
		CFG_cf_theme[i].cover_rolloff_y = 0;
		CFG_cf_theme[i].cover_rolloff_z = 0;
		CFG_cf_theme[i].cover_center_xpos = 8;
		CFG_cf_theme[i].cover_center_ypos = 0;
		CFG_cf_theme[i].cover_center_zpos = -85;
		CFG_cf_theme[i].cover_center_xrot = 0;
		CFG_cf_theme[i].cover_center_yrot = 0;
		CFG_cf_theme[i].cover_center_zrot = 0;
		CFG_cf_theme[i].cover_center_reflection_used = false;
		CFG_cf_theme[i].cover_distance_from_center_left_x = 0;
		CFG_cf_theme[i].cover_distance_from_center_left_y = -25;
		CFG_cf_theme[i].cover_distance_from_center_left_z = 0;
		CFG_cf_theme[i].cover_distance_between_covers_left_x = -25;
		CFG_cf_theme[i].cover_distance_between_covers_left_y = -5;
		CFG_cf_theme[i].cover_distance_between_covers_left_z = 0;
		CFG_cf_theme[i].cover_left_xpos = -15;
		CFG_cf_theme[i].cover_left_ypos = 0;
		CFG_cf_theme[i].cover_left_zpos = -130;
		CFG_cf_theme[i].cover_left_xrot = 0;
		CFG_cf_theme[i].cover_left_yrot = 0;
		CFG_cf_theme[i].cover_left_zrot = 0;
		CFG_cf_theme[i].cover_distance_from_center_right_x = 0;
		CFG_cf_theme[i].cover_distance_from_center_right_y = 25;
		CFG_cf_theme[i].cover_distance_from_center_right_z = 0;
		CFG_cf_theme[i].cover_distance_between_covers_right_x = -25;
		CFG_cf_theme[i].cover_distance_between_covers_right_y = 5;
		CFG_cf_theme[i].cover_distance_between_covers_right_z = 0;
		CFG_cf_theme[i].cover_right_xpos = -15;
		CFG_cf_theme[i].cover_right_ypos = 0;
		CFG_cf_theme[i].cover_right_zpos = -130;
		CFG_cf_theme[i].cover_right_xrot = 0;
		CFG_cf_theme[i].cover_right_yrot = 0;
		CFG_cf_theme[i].cover_right_zrot = 0;

	// carousel
		i++;
		CFG_cf_theme[i].rotation_frames = 40;
		CFG_cf_theme[i].rotation_frames_fast = 7;
		CFG_cf_theme[i].cam_pos_x = 0.0;
		CFG_cf_theme[i].cam_pos_y = 0.0;
		CFG_cf_theme[i].cam_pos_z = 0.0;
		CFG_cf_theme[i].cam_look_x = 0.0;
		CFG_cf_theme[i].cam_look_y = 0.0;
		CFG_cf_theme[i].cam_look_z = -1.0;
		CFG_cf_theme[i].number_of_side_covers = 4;
		CFG_cf_theme[i].reflections_color_bottom = 0x666666FF;
		CFG_cf_theme[i].reflections_color_top = 0xAAAAAA33;
		CFG_cf_theme[i].alpha_rolloff = 0;
		CFG_cf_theme[i].floating_cover = false;
		CFG_cf_theme[i].title_text_xpos = -1;
		CFG_cf_theme[i].title_text_ypos = 434;
		CFG_cf_theme[i].cover_rolloff_x = 0;
		CFG_cf_theme[i].cover_rolloff_y = 17;
		CFG_cf_theme[i].cover_rolloff_z = 0;
		CFG_cf_theme[i].cover_center_xpos = 0;
		CFG_cf_theme[i].cover_center_ypos = 0;
		CFG_cf_theme[i].cover_center_zpos = -122;
		CFG_cf_theme[i].cover_center_xrot = 0;
		CFG_cf_theme[i].cover_center_yrot = 0;
		CFG_cf_theme[i].cover_center_zrot = 0;
		CFG_cf_theme[i].cover_center_reflection_used = true;
		CFG_cf_theme[i].cover_distance_from_center_left_x = -22;
		CFG_cf_theme[i].cover_distance_from_center_left_y = 0;
		CFG_cf_theme[i].cover_distance_from_center_left_z = 0;
		CFG_cf_theme[i].cover_distance_between_covers_left_x = -22;
		CFG_cf_theme[i].cover_distance_between_covers_left_y = 0;
		CFG_cf_theme[i].cover_distance_between_covers_left_z = 0;
		CFG_cf_theme[i].cover_left_xpos = 0;
		CFG_cf_theme[i].cover_left_ypos = 0;
		CFG_cf_theme[i].cover_left_zpos = -120;
		CFG_cf_theme[i].cover_left_xrot = 0;
		CFG_cf_theme[i].cover_left_yrot = 12;
		CFG_cf_theme[i].cover_left_zrot = 0;
		CFG_cf_theme[i].cover_distance_from_center_right_x = 22;
		CFG_cf_theme[i].cover_distance_from_center_right_y = 0;
		CFG_cf_theme[i].cover_distance_from_center_right_z = 0;
		CFG_cf_theme[i].cover_distance_between_covers_right_x = 22;
		CFG_cf_theme[i].cover_distance_between_covers_right_y = 0;
		CFG_cf_theme[i].cover_distance_between_covers_right_z = 0;
		CFG_cf_theme[i].cover_right_xpos = 0;
		CFG_cf_theme[i].cover_right_ypos = 0;
		CFG_cf_theme[i].cover_right_zpos = -120;
		CFG_cf_theme[i].cover_right_xrot = 0;
		CFG_cf_theme[i].cover_right_yrot = -12;
		CFG_cf_theme[i].cover_right_zrot = 0;
/*
	// Bookshelf
		i++;
		CFG_cf_theme[i].rotation_frames = 17;
		CFG_cf_theme[i].rotation_frames_fast = 6;
		CFG_cf_theme[i].cam_pos_x = 0.0;
		CFG_cf_theme[i].cam_pos_y = 0.0;
		CFG_cf_theme[i].cam_pos_z = 0.0;
		CFG_cf_theme[i].cam_look_x = 0.0;
		CFG_cf_theme[i].cam_look_y = 0.0;
		CFG_cf_theme[i].cam_look_z = -1.0;
		if (CFG.widescreen)
			CFG_cf_theme[i].number_of_side_covers = 5;
		else
			CFG_cf_theme[i].number_of_side_covers = 5;
		CFG_cf_theme[i].reflections_color_bottom = 0x666666FF;
		CFG_cf_theme[i].reflections_color_top = 0xAAAAAA33;
		CFG_cf_theme[i].alpha_rolloff = 0;
		CFG_cf_theme[i].floating_cover = false;
		CFG_cf_theme[i].title_text_xpos = -1;
		CFG_cf_theme[i].title_text_ypos = 434;
		CFG_cf_theme[i].cover_rolloff_x = 0;
		CFG_cf_theme[i].cover_rolloff_y = 0;
		CFG_cf_theme[i].cover_rolloff_z = 0;
		CFG_cf_theme[i].cover_center_xpos = 1;
		CFG_cf_theme[i].cover_center_ypos = 0;
		CFG_cf_theme[i].cover_center_zpos = -75;
		CFG_cf_theme[i].cover_center_xrot = 0;
		CFG_cf_theme[i].cover_center_yrot = 0;
		CFG_cf_theme[i].cover_center_zrot = 0;
		CFG_cf_theme[i].cover_center_reflection_used = true;
		CFG_cf_theme[i].cover_distance_from_center_left_x = -13;
		CFG_cf_theme[i].cover_distance_from_center_left_y = 0;
		CFG_cf_theme[i].cover_distance_from_center_left_z = 0;
		CFG_cf_theme[i].cover_distance_between_covers_left_x = -2;
		CFG_cf_theme[i].cover_distance_between_covers_left_y = 0;
		CFG_cf_theme[i].cover_distance_between_covers_left_z = 0;
		CFG_cf_theme[i].cover_left_xpos = 0;
		CFG_cf_theme[i].cover_left_ypos = 0;
		CFG_cf_theme[i].cover_left_zpos = -75;
		CFG_cf_theme[i].cover_left_xrot = 0;
		CFG_cf_theme[i].cover_left_yrot = 90;
		CFG_cf_theme[i].cover_left_zrot = 0;
		CFG_cf_theme[i].cover_distance_from_center_right_x = 13;
		CFG_cf_theme[i].cover_distance_from_center_right_y = 0;
		CFG_cf_theme[i].cover_distance_from_center_right_z = 0;
		CFG_cf_theme[i].cover_distance_between_covers_right_x = 2;
		CFG_cf_theme[i].cover_distance_between_covers_right_y = 0;
		CFG_cf_theme[i].cover_distance_between_covers_right_z = 0;
		CFG_cf_theme[i].cover_right_xpos = 0;
		CFG_cf_theme[i].cover_right_ypos = 0;
		CFG_cf_theme[i].cover_right_zpos = -75;
		CFG_cf_theme[i].cover_right_xrot = 0;
		CFG_cf_theme[i].cover_right_yrot = 90;
		CFG_cf_theme[i].cover_right_zrot = 0;
*/
}

		
// theme controls the looks, not behaviour

void CFG_Default_Theme()
{
	CFG_Default_Coverflow_Themes();
	*CFG.theme = 0;
	snprintf(D_S(CFG.background), "%s/%s", USBLOADER_PATH, "background.png");
	snprintf(D_S(CFG.w_background), "%s/%s", USBLOADER_PATH, "background_wide.png");
	snprintf(D_S(CFG.bg_gui), "%s/%s", USBLOADER_PATH, "bg_gui.png");
	STRCOPY(CFG.theme_path, USBLOADER_PATH);
	*CFG.bg_gui_wide = 0;
	CFG.covers   = 1;
	CFG.hide_header  = 0;
	CFG.hide_hddinfo = CFG_HIDE_HDDINFO;
	CFG.hide_footer  = 0;
	CFG.console_transparent = 0;
	//CFG.buttons  = CFG_BTN_OPTIONS_1;
	strcpy(CFG.cursor, ">>");
	strcpy(CFG.cursor_space, "  ");
	strcpy(CFG.menu_plus,   "[+] ");
	strcpy(CFG.menu_plus_s, "    ");
	strcpy(CFG.favorite, "*");
	strcpy(CFG.saved, "#");

	CFG.gui_text_cfg.color = 0x000000FF; // black
	CFG.gui_text_cfg.outline = 0xFF; //0;
	CFG.gui_text_cfg.outline_auto = 1; //0;
	CFG.gui_text_cfg.shadow = 0;
	CFG.gui_text_cfg.shadow_auto = 0;

	CFG.gui_text2_cfg.color = 0xFFFFFFFF; // white
	CFG.gui_text2_cfg.outline = 0xFF;
	CFG.gui_text2_cfg.outline_auto = 1;
	CFG.gui_text2_cfg.shadow = 0;
	CFG.gui_text2_cfg.shadow_auto = 0;

	CFG.gui_tc[GUI_TC_MENU] = wgui_fc;
	CFG.gui_tc[GUI_TC_TITLE] =
		CFG.gui_tc[GUI_TC_BUTTON] =
		CFG.gui_tc[GUI_TC_RADIO] =
		CFG.gui_tc[GUI_TC_CHECKBOX] =
		CFG.gui_tc[GUI_TC_MENU];
	CFG.gui_tc[GUI_TC_INFO] = text_fc;
	CFG.gui_tc[GUI_TC_ABOUT] = about_fc;
	CFG.gui_window_color[GUI_COLOR_BASE] = 0xFFFFFF80;
	CFG.gui_window_color[GUI_COLOR_POPUP] = 0xFFFFFFB0;
	memset(CFG.gui_button, 0, sizeof(CFG.gui_button));
	CFG.gui_bar = 1;

	CFG.gui_title_top = 0;

	//set cover and console postition defaults
	CFG.layout = CFG_LAYOUT_LARGE_3;
	cfg_layout();

	//set cover height and width
	cfg_set_cover_style(CFG_COVER_STYLE_2D);
	set_colors(CFG_COLORS_DARK);

	//set theme preview image size and pos
	CFG.theme_previewX = -1;
	CFG.theme_previewY = -1;
	CFG.theme_previewW = 0;
	CFG.theme_previewH = 0;
	CFG.w_theme_previewX = -1;
	CFG.w_theme_previewY = -1;
	CFG.w_theme_previewW = 0;
	CFG.w_theme_previewH = 0;

	//set up button mappings
	CFG.button_A = CFG_BTN_BOOT_GAME;
	CFG.button_B = CFG_BTN_GUI;
	CFG.button_1 = CFG_BTN_OPTIONS;
	CFG.button_2 = CFG_BTN_FAVORITES;
	CFG.button_H = CFG_BTN_REBOOT;
	CFG.button_P = CFG_BTN_INSTALL;
	CFG.button_M = CFG_BTN_MAIN_MENU;
	CFG.button_X = CFG_BTN_2;
	CFG.button_Y = CFG_BTN_1;
	CFG.button_Z = CFG_BTN_B;
	CFG.button_C = CFG_BTN_A;
	CFG.button_L = CFG_BTN_M;
	CFG.button_R = CFG_BTN_P;
	CFG.home = CFG_HOME_REBOOT;

	CFG.button_gui = NUM_BUTTON_B;
	CFG.button_opt = NUM_BUTTON_1;
	CFG.button_fav = NUM_BUTTON_2;

	CFG.button_confirm.mask = WPAD_BUTTON_A;
	CFG.button_confirm.num  = NUM_BUTTON_A;
	CFG.button_cancel.mask = WPAD_BUTTON_B;
	CFG.button_cancel.num  = NUM_BUTTON_B;
	CFG.button_exit.mask = WPAD_BUTTON_HOME;
	CFG.button_exit.num  = NUM_BUTTON_HOME;
	CFG.button_other.mask = WPAD_BUTTON_1;
	CFG.button_other.num  = NUM_BUTTON_1;
	CFG.button_save.mask = WPAD_BUTTON_2;
	CFG.button_save.num  = NUM_BUTTON_2;	

	memset(&CFG.gui_cover_area, 0, sizeof(CFG.gui_cover_area));
	memset(&CFG.gui_title_area, 0, sizeof(CFG.gui_title_area));
	CFG.gui_clock_pos.x = -1;
	CFG.gui_page_pos.x = -1;
}

void CFG_default_path()
{
	snprintf(D_S(CFG.covers_path), "%s/%s", USBLOADER_PATH, "covers");
	cfg_set_covers_path();
	CFG_Default_Theme();
	snprintf(D_S(CFG.bg_gui_wide), "%s/%s", USBLOADER_PATH, "bg_gui_wide.png");
}

void CFG_Default()
{
	memset(&CFG, 0, sizeof(CFG));

	// set coverflow defaults
	CFG_Default_Coverflow();

	CFG_default_path();
	cfg_default_url();
	//STRCOPY(CFG.gui_font, "font.png");
	STRCOPY(CFG.gui_font, "font_uni.png");
	
	//CFG.home     = CFG_HOME_REBOOT;
#ifdef BUILD_DBG
	CFG.debug = BUILD_DBG;
	CFG.home = CFG_HOME_SCRSHOT;
#else
	CFG.debug = 0;
	CFG.home = CFG_HOME_REBOOT;
#endif
	CFG.device   = CFG_DEV_ASK;
	STRCOPY(CFG.partition, CFG_DEFAULT_PARTITION);
	CFG.confirm_start = 1;
	CFG.install_partitions = CFG_INSTALL_GAME; //CFG_INSTALL_ALL;
	CFG.disable_format = 0;
	CFG.disable_remove = 0;
	CFG.disable_install = 0;
	CFG.disable_options = 0;
	CFG.music = 1;
	CFG.widescreen = CFG_WIDE_AUTO;
	CFG.gui = CFG_GUI_START_WGUI;
	CFG.gui_menu = 1;
	CFG.gui_start = 1;
	CFG.gui_rows = 2;
	CFG.admin_lock = 1;
	CFG.admin_mode_locked = 1;
	STRCOPY(CFG.unlock_password, CFG_UNLOCK_PASSWORD);
	CFG.gui_antialias = 4;
	CFG.gui_compress_covers = 1;
	CFG.gui_pointer_scroll = 1;
	// default game settings
	CFG.game.video    = CFG_VIDEO_AUTO;
	CFG.game.hooktype = 1; // VBI
	CFG.game.wide_screen = 0; // WIDE IS OFF
	CFG.game.ntsc_j_patch = 0;
	CFG.game.channel_boot = 0;
	CFG.game.nodisc = 0;
	CFG.game.screenshot = 0;
	CFG.game.block_ios_reload = 2; // 2=auto
	CFG.game.alt_controller_cfg = 0;
	cfg_ios_set_idx(DEFAULT_IOS_IDX);
	// all other game settings are 0 (memset(0) above)
	STRCOPY(CFG.sort_ignore, "A,An,The");
	CFG.clock_style = 24;
	// profiles
	CFG.num_profiles = 1;
	//CFG.current_profile = 0;
	STRCOPY(CFG.profile_names[0], "default");
	STRCOPY(CFG.titles_url, "http://www.gametdb.com/titles.txt?LANG={DBL}");
	CFG.intro = 4;
	CFG.fat_install_dir = 1;
	CFG.fat_split_size = 4;
	CFG.db_show_info = 1;
	//CFG.db_ignore_titles = 0;
	//CFG.write_playlog = 0;
	CFG.write_playstats = 1;
	STRCOPY(CFG.db_url, "http://www.gametdb.com/wiitdb.zip?LANG={DBL}&FALLBACK=true&GAMECUBE=true&WIIWARE=true");
	STRCOPY(CFG.db_language, auto_cc());
	STRCOPY(CFG.translation, getLang(CONF_GetLanguage()));
	STRCOPY(CFG.sort, "title-asc");
	CFG.delay_patch = 1;
	CFG.theme_previews = 1;
	// dvd slot check is handled properly now by all cios
	// so the patch is disabled by default
	CFG.disable_dvd_patch = 1;
	//CFG.dml = CFG_DM_2_2;
	STRCOPY(CFG.nand_emu_path, "usb:/nand");
	STRCOPY(CFG.wbfs_fat_dir, "/wbfs");
	CFG.game.nand_emu = 0;
}

bool map_auto_token(char *name, char *name2, char *val, struct TextMap *map, struct MenuButton *var)
{
	int single, all;
	char *next;
	char buf[12];
	if (strcmp(name, name2) != 0) return 0;
	next = val;
	all = 0;
	while ((next = split_tokens(buf, next, ",", 12)))
	{
		int res;
		buf[1]=0;
		res = map_auto_i(name, name2, buf, map, &single);
		if (res >= 0 && !all) var->num = single;
		if (res >= 0) all |= buttonmap[MASTER][single];
	}
	if (all) {
		var->mask = all;
		return 1;
	} else return 0;
}

bool cfg_map_auto_token(char *name, struct TextMap *map, struct MenuButton *var) 
{
	return map_auto_token(name, cfg_name, cfg_val, map, var);
}

u32 hash_id4(void *cb, void *id)
{
	// id4 is usually unique, except for customs
	return hash_string_n(id, 4);
}

u32 hash_id6(void *cb, void *id)
{
	return hash_string_n(id, 6);
}

bool title_cmp_id6(void *cb, void *key, int i)
{
	if (i < 0 || i >= num_title) return false;
	return strncmp((char*)cfg_title[i].id, (char*)key, 6) == 0;
}

bool title_cmp_id4(void *cb, void *key, int i)
{
	if (i < 0 || i >= num_title) return false;
	return strncmp((char*)cfg_title[i].id, (char*)key, 4) == 0;
}

int* title_get_hnext(void *cb, int i)
{
	if (i < 0 || i >= num_title) return NULL;
	return &cfg_title[i].hnext;
}

struct ID_Title* cfg_get_id_title(u8 *id)
{
	// check ID6 first
	int i = hash_get(&title_hash_id6, id);
	if (i < 0) {
		// if not found try ID4
		i = hash_get(&title_hash_id4, id);
	}
	if (i >= 0 && i < num_title) return &cfg_title[i];
	return NULL;
}

char *cfg_get_title(u8 *id)
{
	// titles.txt
	struct ID_Title *idt = cfg_get_id_title(id);
	if (idt) return idt->title;
	// wiitdb
	if (!CFG.db_ignore_titles) {
		gameXMLinfo *g = get_game_info_id(id);
		if (!g)	return NULL;
		if (g->title[0] != 0) return g->title;
	}
	return NULL;
}

char *get_title(struct discHdr *header)
{
	// titles.txt or wiitdb
	char *title;
	if ((memcmp("G",(char*)header->id,1)==0) && (strlen((char*)header->id)>6))
     title = header->title;
  else
	 title = cfg_get_title(header->id);
	if (title) return title;
	// disc header
	return header->title;
}

void title_set(char *id, char *title)
{
	struct ID_Title *idt = cfg_get_id_title((u8*)id);
	if (idt && strncmp(id, (char*)idt->id, 6) == 0) {
		// replace
		mbs_copy(idt->title, title, TITLE_MAX);
	} else {
		cfg_title = mem1_realloc(cfg_title, (num_title+1) * sizeof(struct ID_Title));
		if (!cfg_title) {
			// error
			num_title = 0;
			return;
		}
		// add
		memset(&cfg_title[num_title], 0, sizeof(cfg_title[num_title]));
		strcopy((char*)cfg_title[num_title].id, id, 7);
		mbs_copy(cfg_title[num_title].title, title, TITLE_MAX);
		num_title++;
		// update hash
		if (id[3] == 0 || id[4] == 0) {
			// id3 or id4
			hash_check_init(&title_hash_id4, 0, NULL, &hash_id4, &title_cmp_id4, &title_get_hnext);
			hash_add(&title_hash_id4, id, num_title - 1);
		} else {
			// id6
			hash_check_init(&title_hash_id6, 0, NULL, &hash_id6, &title_cmp_id6, &title_get_hnext);
			hash_add(&title_hash_id6, id, num_title - 1);
		}
	}
}


#define COPY_PATH(D,S) copy_path(D,S,sizeof(D))

void copy_path(char *dest, char *val, int size)
{
	if (strchr(val, ':')) {
		// absolute path with drive (sd:/images/...)
		strcopy(dest, val, size);
	} else if (val[0] == '/') {
		// absolute path without drive (/images/...)
		snprintf(dest, size, "%s%s", FAT_DRIVE, val);
	} else {
		snprintf(dest, size, "%s/%s", USBLOADER_PATH, val);
	}
}

bool copy_theme_path(char *dest, char *val, int size)
{
	// check if it's an absolute path (contains : as in sd:/...)
	if (strchr(val, ':')) {
		// absolute path with drive (sd:/images/...)
		strcopy(dest, val, size);
		return true;
	} else if (val[0] == '/') {
		// absolute path without drive (/images/...)
		snprintf(dest, size, "%s%s", FAT_DRIVE, val);
		return true;
	} else if (*theme_path) {
		struct stat st;
		snprintf(dest, size, "%s/%s", theme_path, val);
		if (stat(dest, &st) == 0) return true;
	}
	snprintf(dest, size, "%s/%s", USBLOADER_PATH, val);
	return false;
}

int find_in_hdr_list(char *id, struct discHdr *list, int num)
{
	int i;
	for (i=0; i<num; i++) {
		if (memcmp(id, list[i].id, 4) == 0) return i;
	}
	return -1;
}

int find_in_game_list(u8 *id, char list[][8], int num)
{
	int i;
	for (i=0; i<num; i++) {
		if (memcmp(id, list[i], 4) == 0) return i;
	}
	return -1;
}

void remove_from_list(int i, char list[][8], int *num)
{
	memset( list[i], 0, sizeof(list[i]) );
	if (i < *num - 1) {
		memmove( &list[i], &list[i+1], (*num - i - 1) * sizeof(list[0]) );
	}
	*num -= 1;
}

bool add_to_list(u8 *id, char list[][8], int *num, int maxn)
{
	if (*num >= maxn) return false;
	// add
	memset(list[*num], 0, sizeof(list[0]));
	memcpy(list[*num], id, 4);
	*num += 1;
	return true;
}

bool is_in_game_list(u8 *id, char list[][8], int num)
{
	int i = find_in_game_list(id, list, num);
	return (i >= 0);
}

bool is_in_hide_list(struct discHdr *game)
{
	return is_in_game_list(game->id, CFG.hide_game, CFG.num_hide_game);
}

void cfg_id_list(char *name, char list[][8], int *num_list, int max_list)
{
	int i;
	
	if (strcmp(name, cfg_name)==0) {
		char id[8], *next = cfg_val;
		while (next) {
			*id = 0;
			next = split_token(id, next, ',', sizeof(id));
			if (strcmp(id, "0")==0) {
				// reset list
				*num_list = 0;
			}
			if (strlen(id) == 4) {
				if (*num_list >= max_list) continue;
				// add id to hide list.
				
				i = find_in_game_list((u8*)id, list, *num_list);
				if (i < 0) {
					strcopy(list[*num_list], id, 8);
					(*num_list)++;
				}
			}
		}
	}
}

bool cfg_color(char *name, u32 *var)
{
	if (cfg_map(name, "black", (int*)var, 0x000000FF)) return true;
	if (cfg_map(name, "white", (int*)var, 0xFFFFFFFF)) return true;
	if (cfg_int_hex(name, (int*)var)) return true;
	return false;
}

// text color
void font_color_cfg_set(char *base_name, struct FontColor_CFG *fc)
{
	if (strncmp(cfg_name, base_name, strlen(base_name)) != 0) return;

	char name[100];
	char *old_name;
	STRCOPY(name, cfg_name);
	str_replace(name, base_name, "gui_text_", sizeof(name));
	old_name = cfg_name;
	cfg_name = name;

	cfg_color("gui_text_color", &fc->color);

	if (cfg_color("gui_text_outline", &fc->outline)) {
		if (strlen(cfg_val) == 2) fc->outline_auto = 1;
		else fc->outline_auto = 0;
	}

	if (cfg_color("gui_text_shadow", &fc->shadow)) {
		if (strlen(cfg_val) == 2) fc->shadow_auto = 1;
		else fc->shadow_auto = 0;
	}

	cfg_name = old_name;
}

bool font_color_set(char *name, struct FontColor *fc)
{
	if (strcmp(name, cfg_name)) return false;
	char color_val[16];
	char *old_val = cfg_val;
	char *next = cfg_val;
	u32 *c = &fc->color;
	int i = 0;
	memset(fc, 0, sizeof(*fc));
	while ((next = split_tokens(color_val, next, "/", sizeof(color_val)))) {
		cfg_val = color_val;
		if (!cfg_color(name, c+i)) break;
		i++;
		if (i >= 3) break;
	}
	cfg_val = old_val;
	return true;
}

int get_outline_color(int text_color, int outline_color, int outline_auto)
{
	unsigned color, alpha;
	if (!outline_color) return 0;

	if (outline_auto) {
		// only alpha specified
		int avg;
		avg = ( ((text_color>>8) & 0xFF)
		      + ((text_color>>16) & 0xFF)
		      + ((text_color>>24) & 0xFF) ) / 3;
		if (avg > 0x80) {
			color = 0x00000000;
		} else {
			color = 0xFFFFFF00;
		}
	} else {
		// full color specified
		color = outline_color;
	}
	// apply text alpha to outline alpha
	alpha = (outline_color & 0xFF) * (text_color & 0xFF) / 0xFF;
	color = (color & 0xFFFFFF00) | alpha;
	return color;
}

void expand_font_color_cfg(FontColor *fc, FontColor_CFG *fcc)
{
	fc->color = fcc->color;
	fc->outline = get_outline_color( fcc->color, fcc->outline, fcc->outline_auto); 
	fc->shadow = get_outline_color( fcc->color, fcc->shadow, fcc->shadow_auto); 
}


int cfg_pos_xy(char *name, struct PosCoords *pos)
{
	if (strcmp(name, cfg_name)) return 0;
	int i, x, y;
	pos->x = -1;
	pos->y = -1;
	i = sscanf(cfg_val, "%d,%d", &x, &y);
	if (i == 2) {
		// check for valid range
		if (x >= 0 && y >= 0 && x < 640 && y < 480) {
			pos->x = x;
			pos->y = y;
			return 1;
		}
	}
	return -1;
}

int cfg_pos_area(char *name, struct RectCoords *pos, int min_w, int min_h)
{
	if (strcmp(name, cfg_name)) return 0;
	int i, x, y, w, h;
	memset(pos, 0, sizeof(*pos));
	i = sscanf(cfg_val, "%d,%d,%d,%d", &x, &y, &w, &h);
	if (i == 4) {
		// check for valid range
		// min w * h : 480 * 320
		if (x >= 0 && y >= 0 && w >= min_w && h >= min_h && x+w <= 640 && y+h <= 480) {
			pos->x = x;
			pos->y = y;
			pos->w = w / 2 * 2;
			pos->h = h / 2 * 2;
			return 1;
		}
	}
	return -1;
}

char *custom_button_name(int i);

char *strtolower(char *s)
{
	char *ss = s;
	int c;
	while (*ss) {
		c = (unsigned char)*ss;
		*ss = tolower(c);
		ss++;
	}
	return s;
}

// n starts at 0
char* get_token_n(char *dest, int size, char *src, char *delim, int n)
{
	int i;
	char *next = src;
	*dest = 0;
	for (i=0; i<=n; i++) {
		next = split_tokens(dest, next, delim, size);
		if (!next) break;
	}
	//dbg_printf("==== tok %d : %s.\n", n, dest);
	return next;
}

bool cfg_gui_button(int i)
{
	int ret;
	char token[64];
	char *save_val = cfg_val;
	struct CfgButton *bb = &CFG.gui_button[i];
	memset(bb, 0, sizeof(*bb));
	ret = cfg_pos_area(cfg_name, &bb->pos, 4, 4);
	if (ret <= 0) return false;
	bb->enabled = 1;
	// defaults:
	CFG.gui_tc[GUI_TC_CBUTTON + i] = wgui_fc;
	bb->hover_zoom = 10;
	if (get_token_n(token, sizeof(token), cfg_val, ",", 4)) {
		cfg_val = token;
		font_color_set(cfg_name, &CFG.gui_tc[GUI_TC_CBUTTON + i]);
		cfg_val = save_val;
	}
	if (get_token_n(token, sizeof(token), cfg_val, ",", 5)) {
		STRCOPY(bb->image, token);
	}
	if (get_token_n(token, sizeof(token), cfg_val, ",", 6)) {
		cfg_val = token;
		cfg_map(cfg_name, "button", &bb->type, 0);
		cfg_map(cfg_name, "icon", &bb->type, 1);
		cfg_val = save_val;
	}
	if (get_token_n(token, sizeof(token), cfg_val, ",", 7)) {
		cfg_val = token;
		cfg_int_max(cfg_name, &bb->hover_zoom, 50);
		cfg_val = save_val;
	}
	return true;
}

bool cfg_gui_custom_buttons()
{
	int i;
	// gui_button_main
	// gui_button_quit ...
	char *opt = "gui_button_";
	int len = strlen(opt);
	char *name = cfg_name + len;
	char bname[32];
	if (strncmp(cfg_name, opt, len) == 0) {
		for (i=0; i<GUI_BUTTON_NUM; i++) {
			STRCOPY(bname, custom_button_name(i));
			strtolower(bname);
			if (strcmp(name, bname) == 0) {
				cfg_gui_button(i);
				return true;
			}
		}
	}
	return false;
}

// theme options, these can be overriden in config.txt

void theme_set_base(char *name, char *val)
{
	cfg_name = name;
	cfg_val = val;
	cfg_bool("hide_header",  &CFG.hide_header);
	cfg_bool("hide_hddinfo", &CFG.hide_hddinfo);
	cfg_bool("hide_footer",  &CFG.hide_footer);
	
	/*cfg_map("buttons", "original",   &CFG.buttons, CFG_BTN_ORIGINAL);
	cfg_map("buttons", "options",    &CFG.buttons, CFG_BTN_OPTIONS_1);
	cfg_map("buttons", "options_1",  &CFG.buttons, CFG_BTN_OPTIONS_1);
	cfg_map("buttons", "options_B",  &CFG.buttons, CFG_BTN_OPTIONS_B);*/

	if (strcmp(name, "home")==0) {
		if (strcmp(val, "exit")==0) {
			CFG.button_H = CFG_BTN_EXIT;
			CFG.home = CFG_HOME_EXIT;
		} else if (strcmp(val, "reboot")==0) {
			CFG.button_H = CFG_BTN_REBOOT;
			CFG.home = CFG_HOME_REBOOT;
		} else if (strcmp(val, "hbc")==0) {
			CFG.button_H = CFG_BTN_HBC;
			CFG.home = CFG_HOME_HBC;
		} else if (strcmp(val, "screenshot")==0) {
			CFG.button_H = CFG_BTN_SCREENSHOT;
			CFG.home = CFG_HOME_SCRSHOT;
		} else if (strcmp(val, "priiloader")==0) {
			CFG.button_H = CFG_BTN_PRIILOADER;
			CFG.home = CFG_HOME_PRIILOADER;
		} else if (strcmp(val, "channel")==0) {
			CFG.button_H = CFG_BTN_CHANNEL;
			CFG.home = CFG_HOME_CHANNEL;
		} else if (strcmp(val, "wii_menu")==0) {
			CFG.button_H = CFG_BTN_WII_MENU;
			CFG.home = CFG_HOME_WII_MENU;
		} else if (strlen(val) == 4) {
			CFG.button_H = *((int *)val);
			CFG.home = *((int *)val);
		} else if (strlen(val) == 8) {
			sscanf(val, "%X", &CFG.button_H);
			sscanf(val, "%X", &CFG.home);
		}
	}

	if (strcmp(name, "buttons")==0) {
		if (strcmp(val, "original")==0) {
			CFG.button_1 = CFG_BTN_OPTIONS;
			CFG.button_B = CFG_BTN_NOTHING;
		} else if ((strcmp(val, "options")==0) || strcmp(val, "options_1")==0) {
			CFG.button_1 = CFG_BTN_OPTIONS;
			CFG.button_B = CFG_BTN_GUI;
		} else if (strcmp(val, "options_B")==0) {
			CFG.button_B = CFG_BTN_OPTIONS;
			CFG.button_1 = CFG_BTN_GUI;
		}
	}

	/*if (strcmp(name, "button_A") && !cfg_map_auto("button_A", map_button, &CFG.button_A) && strlen(val) == 4)
		CFG.button_A = *((int *)val);*/
	if (strcmp(name, "button_B") == 0 && !cfg_map_auto("button_B", map_button, &CFG.button_B)) {
		if (strlen(val) == 4)
			CFG.button_B = *((int *)val);
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.button_B);
	}
	if (strcmp(name, "button_1") == 0 && !cfg_map_auto("button_1", map_button, &CFG.button_1)) {
		if (strlen(val) == 4)
			CFG.button_1 = *((int *)val);
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.button_1);
	}
	if (strcmp(name, "button_2") == 0 && !cfg_map_auto("button_2", map_button, &CFG.button_2)) {
		if (strlen(val) == 4)
			CFG.button_2 = *((int *)val);
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.button_2);
	}
	if (strcmp(name, "button_H") == 0 && !cfg_map_auto("button_H", map_button, &CFG.button_H)) {
		if (strlen(val) == 4)
			CFG.button_H = *((int *)val);
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.button_H);
	}
	if (strcmp(name, "button_-") == 0 && !cfg_map_auto("button_-", map_button, &CFG.button_M)) {
		if (strlen(val) == 4)
			CFG.button_M = *((int *)val);
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.button_M);
	}
	if (strcmp(name, "button_+") == 0 && !cfg_map_auto("button_+", map_button, &CFG.button_P)) {
		if (strlen(val) == 4)
			CFG.button_P = *((int *)val);
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.button_P);
	}
	if (strcmp(name, "button_Z") == 0 && !cfg_map_auto("button_Z", map_button, &CFG.button_Z)) {
		if (strlen(val) == 4)
			CFG.button_Z = *((int *)val);
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.button_Z);
	}
	if (strcmp(name, "button_C") == 0 && !cfg_map_auto("button_C", map_button, &CFG.button_C)) {
		if (strlen(val) == 4)
			CFG.button_C = *((int *)val);
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.button_C);
	}
	if (strcmp(name, "button_Y") == 0 && !cfg_map_auto("button_Y", map_button, &CFG.button_Y)) {
		if (strlen(val) == 4)
			CFG.button_Y = *((int *)val);
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.button_Y);
	}
	if (strcmp(name, "button_X") == 0 && !cfg_map_auto("button_X", map_button, &CFG.button_X)) {
		if (strlen(val) == 4)
			CFG.button_X = *((int *)val);
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.button_X);
	}
	if (strcmp(name, "button_L") == 0 && !cfg_map_auto("button_L", map_button, &CFG.button_L)) {
		if (strlen(val) == 4)
			CFG.button_L = *((int *)val);
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.button_L);
	}
	if (strcmp(name, "button_R") == 0 && !cfg_map_auto("button_R", map_button, &CFG.button_R)) {
		if (strlen(val) == 4)
			CFG.button_R = *((int *)val);
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.button_R);
	}

	//cfg_map_auto_token("button_confirm", map_button_menu, &CFG.button_confirm);
	cfg_map_auto_token("button_cancel", map_button_menu, &CFG.button_cancel);
	cfg_map_auto_token("button_exit", map_button_menu, &CFG.button_exit);
	cfg_map_auto_token("button_other", map_button_menu, &CFG.button_other);
	cfg_map_auto_token("button_save", map_button_menu, &CFG.button_save);

	// if simple is specified in theme it only affects the looks,
	// does not lock down admin modes
	int simpl;
	if (cfg_bool("simple",  &simpl)) {
		if (simpl == 1) {
			if (CFG_HIDE_HDDINFO == 0) {
				// normal version affects hddinfo
				// fat version does not change it.
				CFG.hide_hddinfo = 1;
			}
			CFG.hide_hddinfo = 1;
			CFG.hide_footer = 1;
		} else { // simple == 0
			if (CFG_HIDE_HDDINFO == 0) {
				// normal version affects hddinfo
				// fat version does not change it.
				CFG.hide_hddinfo = 0;
			}
			CFG.hide_footer = 0;
		}
	}
	if (strcmp(name, "cover_style")==0) {
		cfg_set_cover_style_str(val);
	}
	if (strcmp(name, "cursor")==0) {
		//unquote(CFG.cursor, val, 2+1);
		unquote(CFG.cursor, val, sizeof(CFG.cursor));
		mbs_trunc(CFG.cursor, 2);
		int len = mbs_len(CFG.cursor);
		memset(CFG.cursor_space, ' ', len);
		CFG.cursor_space[len] = 0;
	}
	if (strcmp(name, "menu_plus")==0) {
		unquote(CFG.menu_plus, val, sizeof(CFG.menu_plus));
		mbs_trunc(CFG.menu_plus, 4);
		int len = mbs_len(CFG.menu_plus);
		memset(CFG.menu_plus_s, ' ', len);
		CFG.menu_plus_s[len] = 0;
	}
	font_color_cfg_set("gui_text_", &CFG.gui_text_cfg);
	font_color_cfg_set("gui_text2_", &CFG.gui_text2_cfg);
	font_color_set("gui_text_color_info", &CFG.gui_tc[GUI_TC_INFO]);
	if (font_color_set("gui_text_color_menu", &CFG.gui_tc[GUI_TC_MENU])) {
		CFG.gui_tc[GUI_TC_TITLE] = CFG.gui_tc[GUI_TC_BUTTON] = CFG.gui_tc[GUI_TC_RADIO] =
			CFG.gui_tc[GUI_TC_CHECKBOX] = CFG.gui_tc[GUI_TC_MENU];
	}
	font_color_set("gui_text_color_title", &CFG.gui_tc[GUI_TC_TITLE]);
	if (font_color_set("gui_text_color_button", &CFG.gui_tc[GUI_TC_BUTTON])) {
		CFG.gui_tc[GUI_TC_RADIO] = CFG.gui_tc[GUI_TC_CHECKBOX] = CFG.gui_tc[GUI_TC_BUTTON];
	}
	font_color_set("gui_text_color_radio", &CFG.gui_tc[GUI_TC_RADIO]);
	font_color_set("gui_text_color_checkbox", &CFG.gui_tc[GUI_TC_CHECKBOX]);
	cfg_color("gui_window_color_base", &CFG.gui_window_color[GUI_COLOR_BASE]);
	cfg_color("gui_window_color_popup", &CFG.gui_window_color[GUI_COLOR_POPUP]);
	cfg_bool("gui_title_top", &CFG.gui_title_top);
	if (strcmp(name, "covers_size")==0) {
		int w,h;
		if (sscanf(val, "%d,%d", &w, &h) == 2) {
			COVER_WIDTH = w / 2 * 2;
			COVER_HEIGHT = h;
		}
	}
	if (strcmp(name, "wcovers_size")==0) {
		int w,h;
		if (sscanf(val, "%d,%d", &w, &h) == 2) {
			CFG.W_COVER_WIDTH = w / 2 * 2;
			CFG.W_COVER_HEIGHT = h;
		}
	}
}

// theme options, these can not be overriden in config.txt

void theme_set(char *name, char *val)
{
	cfg_name = name;
	cfg_val = val;

	theme_set_base(name, val);

	if (cfg_map_auto("layout", map_layout, &CFG.layout)) {
		cfg_layout();
	}

	cfg_bool("covers",  &CFG.covers);
	cfg_bool("console_transparent", &CFG.console_transparent);

	cfg_int_max("color_header",      &CFG.color_header, 15);
	cfg_int_max("color_selected_fg", &CFG.color_selected_fg, 15);
	cfg_int_max("color_selected_bg", &CFG.color_selected_bg, 15);
	cfg_int_max("color_inactive",    &CFG.color_inactive, 15);
	cfg_int_max("color_footer",      &CFG.color_footer, 15);
	cfg_int_max("color_help",        &CFG.color_help, 15);

	int colors = 0;
	cfg_map("colors", "mono",   &colors, CFG_COLORS_MONO);
	cfg_map("colors", "dark",   &colors, CFG_COLORS_DARK);
	cfg_map("colors", "light",  &colors, CFG_COLORS_BRIGHT);
	cfg_map("colors", "bright", &colors, CFG_COLORS_BRIGHT);
	if (colors) set_colors(colors);

	if (strcmp(name, "background")==0
		|| strcmp(name, "background_base")==0)
	{
		if (strcmp(val, "0")==0) {
			*CFG.background = 0;
		} else {
			copy_theme_path(CFG.background, val, sizeof(CFG.background));
		}
	}

	if (strcmp(name, "wbackground")==0
		|| strcmp(name, "wbackground_base")==0)
	{
		if (strcmp(val, "0")==0) {
			*CFG.w_background = 0;
		} else {
			copy_theme_path(CFG.w_background, val, sizeof(CFG.w_background));
		}
	}

	if (strcmp(name, "background_gui")==0)
	{
		if (strcmp(val, "0")==0) {
			*CFG.bg_gui = 0;
		} else {
			copy_theme_path(CFG.bg_gui, val, sizeof(CFG.bg_gui));
		}
	}

	if (strcmp(name, "wbackground_gui")==0)
	{
		if (strcmp(val, "0")==0) {
			*CFG.bg_gui_wide = 0;
		} else {
			copy_theme_path(CFG.bg_gui_wide, val, sizeof(CFG.bg_gui_wide));
		}
	}

	if (strcmp(name, "console_coords")==0) {
		int x,y,w,h;
		if (sscanf(val, "%d,%d,%d,%d", &x, &y, &w, &h) == 4) {
			// round x coords to even values
			CONSOLE_XCOORD = x / 2 * 2;
			CONSOLE_YCOORD = y;
			CONSOLE_WIDTH  = w / 2 * 2;
			CONSOLE_HEIGHT = h;
		}
	}
	if (strcmp(name, "wconsole_coords")==0) {
		int x,y,w,h;
		if (sscanf(val, "%d,%d,%d,%d", &x, &y, &w, &h) == 4) {
			// round x coords to even values
			CFG.W_CONSOLE_XCOORD = x / 2 * 2;
			CFG.W_CONSOLE_YCOORD = y;
			CFG.W_CONSOLE_WIDTH  = w / 2 * 2;
			CFG.W_CONSOLE_HEIGHT = h;
		}
	}
	if (strcmp(name, "console_color")==0) {
		int fg,bg;
		if (sscanf(val, "%d,%d", &fg, &bg) == 2) {
			CONSOLE_FG_COLOR = fg;
			CONSOLE_BG_COLOR = bg;
		}
	}
	if (strcmp(name, "console_entries")==0) {
		int n;
		if (sscanf(val, "%d", &n) == 1) {
			ENTRIES_PER_PAGE = n;
		}
	}
	if (strcmp(name, "covers_coords")==0) {
		int x,y;
		if (sscanf(val, "%d,%d", &x, &y) == 2) {
			COVER_XCOORD = x / 2 * 2;
			COVER_YCOORD = y;
		}
	}
	if (strcmp(name, "wcovers_coords")==0) {
		int x,y;
		if (sscanf(val, "%d,%d", &x, &y) == 2) {
			CFG.W_COVER_XCOORD = x / 2 * 2;
			CFG.W_COVER_YCOORD = y;
		}
	}
	if (strcmp(name, "preview_coords")==0) {
		int x,y,w,h;
		if (sscanf(val, "%d,%d,%d,%d", &x, &y, &w, &h) == 4) {
			CFG.theme_previewX = x;
			CFG.theme_previewY = y;
			CFG.theme_previewW = w / 2 * 2;
			CFG.theme_previewH = h;
		}
	}
	if (strcmp(name, "wpreview_coords")==0) {
		int x,y,w,h;
		if (sscanf(val, "%d,%d,%d,%d", &x, &y, &w, &h) == 4) {
			CFG.w_theme_previewX = x;
			CFG.w_theme_previewY = y;
			CFG.w_theme_previewW = w / 2 * 2;
			CFG.w_theme_previewH = h;
		}
	}

	// coverflow reflection
	if (strcmp(name, "coverflow_reflection")==0) {
		int i, top=0, bot=0;
		i = sscanf(val, "%x,%x", &top, &bot);
		if (i > 0) {
			if (i == 1) bot = top;
			for (i=0; i<CF_THEME_NUM; i++) {
				if (CFG_cf_theme[i].cover_center_reflection_used) {
					CFG_cf_theme[i].reflections_color_bottom = top;
					CFG_cf_theme[i].reflections_color_top = bot;
				}
			}
		}
	}

	cfg_pos_area("gui_cover_area", &CFG.gui_cover_area, 480, 320);
	cfg_pos_area("gui_title_area", &CFG.gui_title_area, 320, 10);
	cfg_pos_xy("gui_clock_pos", &CFG.gui_clock_pos);
	cfg_pos_xy("gui_page_pos", &CFG.gui_page_pos);
	cfg_gui_custom_buttons();
	cfg_bool("gui_bar", &CFG.gui_bar);
	cfg_map("gui_bar", "top", &CFG.gui_bar, 2);
	cfg_map("gui_bar", "bottom", &CFG.gui_bar, 3);
}

// global options

char *ios_str(int idx)
{
	return map_ios[idx].name;
}

void set_recommended_cIOS_idx(u8 ios, bool onlyD2x) {
	if (ios == cIOS_base[0]) {
		cfg_ios_set_idx(CFG_IOS_245);
	} else if (ios == cIOS_base[1]) {
		cfg_ios_set_idx(CFG_IOS_246);
	} else if (ios == cIOS_base[2]) {
		cfg_ios_set_idx(CFG_IOS_247);
	} else if (ios == cIOS_base[3]) {
		cfg_ios_set_idx(CFG_IOS_248);
	} else if (ios == cIOS_base[4]) {
		cfg_ios_set_idx(CFG_IOS_249);
	} else if (ios == cIOS_base[5]) {
		cfg_ios_set_idx(CFG_IOS_250);
	} else if (ios == cIOS_base[6]) {
		cfg_ios_set_idx(CFG_IOS_251);
	} else if (ios == cIOS_base[7] && !onlyD2x) {
		cfg_ios_set_idx(CFG_IOS_222_MLOAD);
	} else if (ios == cIOS_base[8] && !onlyD2x) {
		cfg_ios_set_idx(CFG_IOS_223_MLOAD);
	} else if (ios == cIOS_base[9] && !onlyD2x) {
		cfg_ios_set_idx(CFG_IOS_224_MLOAD);
	} else {
		int i = 0;
		for (i = 0; i < 10; i++) {
			if (cIOS_base[i] == 56) {
				cfg_ios_set_idx(i);
				return;
			}
		}
		cfg_ios_set_idx(CFG_IOS_249);
	}
}

void cfg_ios_set_idx(int ios_idx)
{
	CFG.ios = 249;
	CFG.ios_yal = 0;
	CFG.ios_mload = 0;

	switch (ios_idx) {
		case CFG_IOS_245:
			CFG.ios = 245;
			break;
		case CFG_IOS_246:
			CFG.ios = 246;
			break;
		case CFG_IOS_247:
			CFG.ios = 247;
			break;
		case CFG_IOS_248:
			CFG.ios = 248;
			break;
		case CFG_IOS_249:
			CFG.ios = 249;
			break;
		case CFG_IOS_250:
			CFG.ios = 250;
			break;
		case CFG_IOS_251:
			CFG.ios = 251;
			break;
		case CFG_IOS_222_MLOAD:
			CFG.ios = 222;
			CFG.ios_yal = 1;
			CFG.ios_mload = 1;
			break;
		case CFG_IOS_223_MLOAD:
			CFG.ios = 223;
			CFG.ios_yal = 1;
			CFG.ios_mload = 1;
			break;
		case CFG_IOS_224_MLOAD:
			CFG.ios = 224;
			CFG.ios_yal = 1;
			CFG.ios_mload = 1;
			break;
		case CFG_IOS_222_YAL:
			CFG.ios = 222;
			CFG.ios_yal = 1;
			break;
		case CFG_IOS_223_YAL:
			CFG.ios = 223;
			CFG.ios_yal = 1;
			break;
		default:
			ios_idx = CFG_IOS_AUTO;
 	}

	CFG.game.ios_idx = ios_idx;
}

bool cfg_ios_idx(char *name, char *val, int *idx)
{
	int i, id;
	i = map_auto_i("ios", name, val, map_ios, &id);
	if (i < 0) return false;
	if (i > CFG_IOS_MAX) return false;
	if (i != id) return false; // safety check for correct defines
	*idx = i;
	return true;
}

void cfg_ios(char *name, char *val)
{
	int i;
	if (!cfg_ios_idx(name, val, &i)) return;
	cfg_ios_set_idx(i);
	/*
	int ios;
	if (sscanf(val, "%d", &ios) != 1) return;
	CFG.game.ios_idx = i;
	CFG.ios = ios;
	CFG.ios_yal = 0;
	CFG.ios_mload = 0;
	if (strstr(val, "-yal")) {
		CFG.ios_yal = 1;
	}
	if (strstr(val, "-mload")) {
		CFG.ios_yal = 1;
		CFG.ios_mload = 1;
	}
	*/
}

bool is_ios_idx_mload(int ios_idx)
{
	switch (ios_idx) {
		case CFG_IOS_222_MLOAD:
		case CFG_IOS_223_MLOAD:
		case CFG_IOS_224_MLOAD:
			return true;
	}
	return false;
}

int get_ios_idx_type(int ios_idx)
{
	switch (ios_idx) {
		case CFG_IOS_245:
		case CFG_IOS_246:
		case CFG_IOS_247:
		case CFG_IOS_248:
		case CFG_IOS_249:
		case CFG_IOS_250:
		case CFG_IOS_251:
			return IOS_TYPE_WANIN;
		case CFG_IOS_222_MLOAD:
		case CFG_IOS_223_MLOAD:
		case CFG_IOS_224_MLOAD:
			return IOS_TYPE_HERMES;
		case CFG_IOS_222_YAL:
		case CFG_IOS_223_YAL:
			return IOS_TYPE_KWIIRK;
	}
	return IOS_TYPE_UNK;
}



void cfg_set_game(char *name, char *val, struct Game_CFG *game_cfg)
{
	cfg_name = name;
	cfg_val = val;

	cfg_map_auto("language", map_language, &game_cfg->language);

	cfg_map_auto("video", map_video, &game_cfg->video);
	if (strcmp("video", name) == 0 && strcmp("vidtv", val) == 0)
	{
		game_cfg->video = CFG_VIDEO_AUTO;
		game_cfg->vidtv = 1;
	}
	if (strcmp("video", name) == 0 && strcmp("patch", val) == 0)
	{
		game_cfg->video = CFG_VIDEO_AUTO;
		game_cfg->video_patch = 1;
	}

	cfg_map_auto("video_patch", map_video_patch, &game_cfg->video_patch);
	cfg_map_auto("nand_emu", map_nand_emu, &game_cfg->nand_emu);
	cfg_map_auto("channel_boot", map_boot_method, &game_cfg->channel_boot);

	cfg_bool("vidtv", &game_cfg->vidtv);
	cfg_bool("country_patch", &game_cfg->country_patch);
	cfg_bool("clear_patches", &game_cfg->clean);
	cfg_map("clear_patches", "all", &game_cfg->clean, CFG_CLEAN_ALL);
	cfg_bool("fix_002", &game_cfg->fix_002);
	cfg_bool("wide_screen", &game_cfg->wide_screen);
	cfg_bool("ntsc_j_patch", &game_cfg->ntsc_j_patch);
	cfg_bool("nodisc", &game_cfg->nodisc);
	cfg_bool("screenshot", &game_cfg->screenshot);
	cfg_ios_idx(name, val, &game_cfg->ios_idx);
	cfg_bool("block_ios_reload", &game_cfg->block_ios_reload);
	cfg_map("block_ios_reload", "auto", &game_cfg->block_ios_reload, 2);
	cfg_bool("alt_controller_cfg", &game_cfg->alt_controller_cfg);
	if (strcmp("alt_dol", name) == 0) {
		int set = 0;
		if (cfg_bool("alt_dol", &game_cfg->alt_dol)) set = 1;
		if (cfg_map ("alt_dol", "sd", &game_cfg->alt_dol, 1)) set = 1;
		if (cfg_map ("alt_dol", "disc", &game_cfg->alt_dol, 2)) set = 1;
		if (cfg_int_max("alt_dol", &game_cfg->alt_dol, 100)) set = 1;
		if (!set) {
			// name specified
			game_cfg->alt_dol = 2;
			STRCOPY(game_cfg->dol_name, val);
		}
	}
	if (strcmp("dol_name", name) == 0 && *val) {
		STRCOPY(game_cfg->dol_name, val);
	}
	cfg_bool("ocarina", &game_cfg->ocarina);
	cfg_map_auto("hooktype", map_hook, &game_cfg->hooktype);
	cfg_int_max("write_playlog", &game_cfg->write_playlog, 3);
	
}

bool cfg_set_gbl(char *name, char *val)
{
	if (cfg_map("device", "ask",  &CFG.device, CFG_DEV_ASK)) return true;
	if (cfg_map("device", "usb",  &CFG.device, CFG_DEV_USB)) return true;
	if (cfg_map("device", "sdhc", &CFG.device, CFG_DEV_SDHC)) return true;

	CFG_STR("partition", CFG.partition);

	if (cfg_map_auto("gui_style", map_gui_style, &CFG.gui_style)) return true;
	//if (cfg_int_max("dml", &CFG.dml, 6)) return true;
	cfg_int_max("devo", &CFG.default_gc_loader,2);
	cfg_bool("old_button_color", &CFG.old_button_color);

	int rows = 0;
	if (cfg_int_max("gui_rows", &rows, 4)) {
		if (rows > 0) {
			CFG.gui_rows = rows;
			return true;
		}
	}
	CFG_STR("profile", CFG.current_profile_name);

	// theme must be last, because it changes cfg_name, cfg_val
	if (strcmp(name, "theme")==0) {
		if (*val) {
			load_theme(val);
			int i;
			for (i=0; i<num_theme; i++) {
				if (stricmp(val, theme_list[i]) == 0) {
					cur_theme = i;
					break;
				}
			}
			return true;
		}
	}
	return false;
}

void cfg_set_early(char *name, char *val)
{
	cfg_name = name;
	cfg_val = val;
	cfg_int_max("debug", &CFG.debug, 255);
	if (CFG.debug & 16) {
		CFG.time_launch = 1;
		CFG.debug &= ~16;
	}
	cfg_int_max("debug_gecko", &CFG.debug_gecko, 255);
	cfg_bool("widescreen", &CFG.widescreen);
	cfg_map("widescreen", "auto", &CFG.widescreen, CFG_WIDE_AUTO);
	cfg_ios(name, val);
	CFG_STR("partition", CFG.partition);
	cfg_int_max("intro", &CFG.intro, 100);
}

void cfg_set(char *name, char *val)
{
	cfg_name = name;
	cfg_val = val;

	theme_set(name, val);

	cfg_set_early(name, val);

	cfg_bool("start_favorites", &CFG.start_favorites);
	cfg_bool("confirm_start", &CFG.confirm_start);
	cfg_bool("confirm_ocarina", &CFG.confirm_ocarina);
	
	cfg_bool("disable_format", &CFG.disable_format); 
	cfg_bool("disable_remove", &CFG.disable_remove); 
	cfg_bool("disable_install", &CFG.disable_install);
	cfg_bool("disable_options", &CFG.disable_options);
	
	cfg_bool("admin_lock", &CFG.admin_lock);
	cfg_bool("admin_unlock", &CFG.admin_lock);
	if (strcmp(name, "unlock_password")==0) {
		STRCOPY(CFG.unlock_password, val);
	}

	cfg_int_max("cursor_jump", &CFG.cursor_jump, 50);
	cfg_bool("console_mark_page", &CFG.console_mark_page);
	cfg_bool("console_mark_favorite", &CFG.console_mark_favorite);
	cfg_bool("console_mark_saved", &CFG.console_mark_saved);

	// db options - Lustar
	if (!strcmp(name, "db_url")) 
		strcpy(CFG.db_url,val);
	if (!strcmp(name, "db_language")) {
		if (strcmp(val, "auto") == 0 || strcmp(val, "AUTO") == 0) {
			STRCOPY(CFG.db_language, auto_cc());
		} else {
			strcpy(CFG.db_language,val);
		}
	}
	if (!strcmp(name, "sort"))
		strcpy(CFG.sort,val);
		
	if (!strcmp(name, "translation")) {
		if (strcmp(val, "auto") == 0 || strcmp(val, "AUTO") == 0) {
			STRCOPY(CFG.translation, getLang(CONF_GetLanguage()));
		} else {
			strcpy(CFG.translation,val);
		}
	}
	cfg_bool("load_unifont", &CFG.load_unifont);
	cfg_bool("gui_compress_covers", &CFG.gui_compress_covers);	
	cfg_int_max("gui_antialias", &CFG.gui_antialias, 4);
	if (CFG.gui_antialias==0 || !CFG.gui_compress_covers) CFG.gui_antialias = 1;
	if (CFG.gui_antialias > 1) CFG.gui_compress_covers = 1;
	cfg_bool("gui_pointer_scroll", &CFG.gui_pointer_scroll);

	if (cfg_set_gbl(name, val)) {
		return;
	}

	cfg_set_game(name, val, &CFG.game);
	if (!CFG.direct_launch) {
		*CFG.game.dol_name = 0;
	}

	// covers path
	if (strcmp(name, "covers_path")==0) {
		COPY_PATH(CFG.covers_path, val);
		cfg_set_covers_path();
	}
	if (strcmp(name, "covers_path_2d")==0) {
		COPY_PATH(CFG.covers_path_2d, val);
		CFG.covers_path_2d_set = 1;
	}
	if (strcmp(name, "covers_path_3d")==0) {
		COPY_PATH(CFG.covers_path_3d, val);
	}
	if (strcmp(name, "covers_path_disc")==0) {
		COPY_PATH(CFG.covers_path_disc, val);
	}
	if (strcmp(name, "covers_path_full")==0) {
		COPY_PATH(CFG.covers_path_full, val);
	}

	CFG_STR_LIST("gamercard_url", CFG.gamercard_url);
	CFG_STR_LIST("gamercard_key", CFG.gamercard_key);

	// urls
	CFG_STR("titles_url", CFG.titles_url);
	CFG_STR("nand_emu_path", CFG.nand_emu_path);
	CFG_STR("wbfs_fat_dir", CFG.wbfs_fat_dir);
	CFG_STR_LIST("cover_url", CFG.cover_url_2d);
	CFG_STR_LIST("cover_url_3d", CFG.cover_url_3d);
	CFG_STR_LIST("cover_url_disc", CFG.cover_url_disc);
	CFG_STR_LIST("cover_url_full", CFG.cover_url_full);
	CFG_STR_LIST("cover_url_hq", CFG.cover_url_hq);
	// download options
	cfg_bool("download_all_styles", &CFG.download_all);
	cfg_map("download_id_len", "4", &CFG.download_id_len, 4);
	cfg_map("download_id_len", "6", &CFG.download_id_len, 6);
	if (strcmp("download_cc_pal", name) == 0) {
		if (strcmp(val, "auto") == 0 || strcmp(val, "AUTO") == 0) {
			*CFG.download_cc_pal = 0;
		} else if (strlen(val) == 2) {
			strcpy(CFG.download_cc_pal, val);
		}
	}

	// gui
	if (cfg_int_max("gui", &CFG.gui, CFG_GUI_START_WGUI) ||
		cfg_map("gui", "start", &CFG.gui, CFG_GUI_START_WGUI))
	{
		if (CFG.gui == CFG_GUI_START || CFG.gui == CFG_GUI_START_WGUI) {
			CFG.gui_start = 1;
		} else {
			CFG.gui_start = 0;
		}
		if (CFG.gui > CFG_GUI_START) {
			CFG.gui_menu = 1;
		} else {
			CFG.gui_menu = 0;
		}
	}

	cfg_map("gui_transition", "scroll", &CFG.gui_transit, 0);
	cfg_map("gui_transition", "fade",   &CFG.gui_transit, 1);
	CFG_STR("gui_font", CFG.gui_font);
	cfg_bool("gui_lock", &CFG.gui_lock);

	// simple changes dependant options
	int simpl;
	if (cfg_bool("simple",  &simpl)) {
		if (simpl == 1) {
			CFG.confirm_start = 0;
			//if (CFG_HIDE_HDDINFO == 0) {
			//	// normal version affects hddinfo
			//	// fat version does not change it.
			//	CFG.hide_hddinfo = 1;
			//}
			CFG.hide_footer = 1;
			CFG.disable_remove = 1;
			CFG.disable_install = 1;
			CFG.disable_options = 1;
			CFG.disable_format = 1;
		} else { // simple == 0
			CFG.confirm_start = 1;
			//if (CFG_HIDE_HDDINFO == 0) {
			//	// normal version affects hddinfo
			//	// fat version does not change it.
			//	CFG.hide_hddinfo = 0;
			//}
			CFG.hide_footer = 0;
			CFG.disable_remove = 0;
			CFG.disable_install = 0;
			CFG.disable_options = 0;
			CFG.disable_format = 0;
		}
	}
	cfg_map("install_partitions", "only_game",
			&CFG.install_partitions, CFG_INSTALL_GAME);
	cfg_map("install_partitions", "all",
			&CFG.install_partitions, CFG_INSTALL_ALL);
	cfg_map("install_partitions", "1:1",
			&CFG.install_partitions, CFG_INSTALL_1_1);
	cfg_map("install_partitions", "iso",
			&CFG.install_partitions, CFG_INSTALL_ISO);

	cfg_int_max("fat_split_size", &CFG.fat_split_size, 4);

	//cfg_bool("write_playlog", &CFG.write_playlog);

	cfg_int_max("fat_install_dir", &CFG.fat_install_dir, 3);
	cfg_int_max("fs_install_layout", &CFG.fat_install_dir, 3);
	
	cfg_bool("ntfs_write", &CFG.ntfs_write);
	cfg_map("ntfs_write", "norecover", &CFG.ntfs_write, 2);

	cfg_bool("disable_nsmb_patch", &CFG.disable_nsmb_patch);
	cfg_bool("disable_pop_patch", &CFG.disable_pop_patch);
	cfg_bool("disable_dvd_patch", &CFG.disable_dvd_patch);
	cfg_bool("disable_wip", &CFG.disable_wip);
	cfg_bool("disable_bca", &CFG.disable_bca);

	//cfg_int_max("dml", &CFG.dml, 6);
	cfg_int_max("devo", &CFG.default_gc_loader,2);
	cfg_bool("old_button_color", &CFG.old_button_color);

	cfg_id_list("hide_game", CFG.hide_game, &CFG.num_hide_game, MAX_HIDE_GAME);
	cfg_id_list("pref_game", CFG.pref_game, &CFG.num_pref_game, MAX_PREF_GAME);
	cfg_id_list("favorite_game", CFG.favorite_game,
			&CFG.num_favorite_game, MAX_FAVORITE_GAME);
	CFG_STR("sort_ignore", CFG.sort_ignore);

	cfg_map("clock_style", "0",  &CFG.clock_style, 0);
	cfg_map("clock_style", "24", &CFG.clock_style, 24);
	cfg_map("clock_style", "12", &CFG.clock_style, 12);
	cfg_map("clock_style", "12am", &CFG.clock_style, 13);

	if (strcmp(name, "music")==0) {
		if (cfg_bool("music", &CFG.music)) {
			*CFG.music_file = 0;
		} else {
			COPY_PATH(CFG.music_file, val);
		}
	}

	cfg_bool("delay_patch", &CFG.delay_patch);
	
	if (strncmp(name, "title:", 6)==0) {
		char id[8];
		trimcopy(id, name+6, sizeof(id)); 
		title_set(id, val);
	}
	if (strncmp(name, "game:", 5)==0) {
		game_set(name, val);
	}

	if (strcmp(name, "profile_names")==0) {
		char *next;
		next = val;
		CFG.num_profiles = 0;
		while ((next = split_tokens(
				CFG.profile_names[CFG.num_profiles],
				next, ",", sizeof(CFG.profile_names[0]))
				))
		{
			if (*CFG.profile_names[CFG.num_profiles] == 0) break;
			CFG.num_profiles++;
			if (CFG.num_profiles >= MAX_PROFILES) break;
		}
		if (CFG.num_profiles == 0) {
			CFG.num_profiles = 1;
			STRCOPY(CFG.profile_names[0], "default");
		}
	}

	if (strcmp(name, "profile_start_favorites")==0) {
		int i;
		char token[25];
		for (i=0; i<MAX_PROFILES; i++) {
			if (get_token_n(token, sizeof(token), val, " ", i)) {
				cfg_val = token;
				cfg_bool("profile_start_favorites", &CFG.profile_start_favorites[i]);
			}
		}
	}

	cfg_int_max("wiird", &CFG.wiird, 2);
	if (strcmp(name, "return_to_channel")==0) {
		if (strcmp(val, "0")== 0)
			CFG.return_to = 0;
		else if (strcmp(val, "auto")== 0)
			CFG.return_to = 1;
		else if (strlen(val) == 4)
			CFG.return_to = *(int *)val;
		else if (strlen(val) == 8)
			sscanf(val, "%X", &CFG.return_to);
	}

	cfg_bool("db_show_info",  &CFG.db_show_info);
	cfg_bool("db_ignore_titles", &CFG.db_ignore_titles);
	cfg_bool("write_playstats",  &CFG.write_playstats);
	
	cfg_bool("adult_themes", &CFG.adult_themes);
	cfg_bool("theme_previews", &CFG.theme_previews);
	cfg_map("select", "previous",   &CFG.select, CFG_SELECT_PREVIOUS);
	cfg_map("select", "start",   &CFG.select, CFG_SELECT_START);
	cfg_map("select", "middle",  &CFG.select, CFG_SELECT_MIDDLE);
	cfg_map("select", "end", &CFG.select, CFG_SELECT_END);
	cfg_map("select", "most", &CFG.select, CFG_SELECT_MOST);
	cfg_map("select", "least", &CFG.select, CFG_SELECT_LEAST);
	cfg_map("select", "random", &CFG.select, CFG_SELECT_RANDOM);
}


int CFG_hide_games(struct discHdr *list, int cnt)
{
	int i;
	int kept_cnt = 0;
	for (i=0; i<cnt; i++) {
		if (!is_in_hide_list(list+i)) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	return cnt;
}

void CFG_sort_pref(struct discHdr *list, int cnt)
{
	int i, j, tmp_cnt = 0;
	struct discHdr *tmp_list;
	tmp_list = malloc(sizeof(struct discHdr)*cnt);
	if (tmp_list == NULL) return;
	for (i=0; i<CFG.num_pref_game; i++) {
		j = find_in_hdr_list(CFG.pref_game[i], list, cnt);
		if (j < 0) continue;
		// move entry to tmp
		memcpy(&tmp_list[tmp_cnt], &list[j], sizeof(struct discHdr));
		tmp_cnt++;
		// move remaining entries over
		memcpy(list+j, list+j+1, (cnt-j-1) * sizeof(struct discHdr));
		cnt--;
	}
	// append remaining list to tmp
	memcpy(&tmp_list[tmp_cnt], list, sizeof(struct discHdr)*cnt);
	// move everything back to list
	memcpy(list, tmp_list, sizeof(struct discHdr)*(cnt+tmp_cnt));
	free(tmp_list);
}

bool is_favorite(u8 *id)
{
	return is_in_game_list(id, CFG.favorite_game,
			CFG.num_favorite_game);
}

bool set_favorite(u8 *id, bool fav)
{
	int i;
	bool ret;
	i = find_in_game_list(id, CFG.favorite_game, CFG.num_favorite_game);
	if (i >= 0) {
		// alerady on the list
		if (fav) return true;
		// remove
		remove_from_list(i, CFG.favorite_game, &CFG.num_favorite_game);
		return CFG_Save_Settings(0);
	}
	// not on list
	if (!fav) return true;
	// add
	ret = add_to_list(id, CFG.favorite_game, &CFG.num_favorite_game, MAX_FAVORITE_GAME);
	if (!ret) return ret;
	return CFG_Save_Settings(0);
}

int CFG_filter_favorite(struct discHdr *list, int cnt)
{
	int i;
	int kept_cnt = 0;
	//printf("f filter %p %d\n", list, cnt); sleep(2);
	for (i=0; i<cnt; i++) {
		//printf("%d %s %d\n", i, list[i].id, is_favorite(list[i].id));
		if (is_favorite(list[i].id)) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	//printf("e filter %p %d\n", list, cnt); sleep(5);
	return cnt;
}

bool is_hide_game(u8 *id)
{
	return is_in_game_list(id, CFG.hide_game,
			CFG.num_hide_game);
}

bool set_hide_game(u8 *id, bool hide)
{
	int i;
	bool ret;
	i = find_in_game_list(id, CFG.hide_game, CFG.num_hide_game);
	if (i >= 0) {
		// already in the list
		if (hide) return true;
		// remove
		remove_from_list(i, CFG.hide_game, &CFG.num_hide_game);
		return CFG_Save_Settings(0);
	}
	// not on list
	if (!hide) return true;
	// add
	ret = add_to_list(id, CFG.hide_game, &CFG.num_hide_game, MAX_HIDE_GAME);
	if (!ret) return ret;
	return CFG_Save_Settings(0);
}

void cfg_parsearg(int argc, char **argv)
{
	int i;
	char pathname[200];
	bool is_opt;
	for (i=1; i<argc; i++) {
		//printf("arg[%d]: %s\n", i, argv[i]);
		is_opt = *argv[i] != '#' && (strchr(argv[i], '=') || strchr(argv[i], '['));
		if (is_opt) {
			cfg_parseline(argv[i], &cfg_set);
		}
		else if (argv[i][0] != '#') {
			char *p = strrchr(argv[i], '.');
			if (p && (stricmp(p, ".txt")==0 || stricmp(p,".cfg")==0)) {
				if (strchr(argv[i], ':')) {
					// absolute path
					cfg_parsefile(pathname, &cfg_set);
				} else {
					// relative path
					snprintf(pathname, sizeof(pathname), "%s/%s", USBLOADER_PATH, argv[i]);
					cfg_parsefile(pathname, &cfg_set);
				}
			}
		}
	}
}

void cfg_parsearg_early(int argc, char **argv)
{
	int i;
	char *eq;
	// setup defaults
	CFG_Default();
	// check for direct start
	cfg_direct_start(argc, argv);
	// parse the rest
	for (i=1; i<argc; i++) {
		eq = strchr(argv[i], '=');
		if (eq) {
			cfg_parseline(argv[i], &cfg_set_early);
		}
	}
}


// SETTINGS.CFG

int cfg_find_profile(char *pname)
{
	int i;
	for (i=0; i<CFG.num_profiles; i++) {
		if (strcasecmp(CFG.profile_names[i], pname) == 0) {
			return i;
		}
	}
	if (strcasecmp(pname, "default") == 0) return 0;
	return -1;
}

int profile_tag_index = 0;

void settings_set(char *name, char *val)
{
	cfg_name = name;
	cfg_val = val;
	//printf("s: %s : %s\n", name, val);
	if (strcmp(name, "[profile]") == 0) {
		int i = cfg_find_profile(val);
		CFG.current_profile = i >= 0 ? i : 0;
		profile_tag_index = i;
		//printf("%s : %s : %d\n", name, val, i);
	}
	if (profile_tag_index >= 0) {
		if (strcmp(name, "profile_theme") == 0) {
			int i;
			for (i=0; i<num_theme; i++) {
				if (stricmp(val, theme_list[i]) == 0) {
					CFG.profile_theme[profile_tag_index] = i;
					break;
				}
			}
		}
	}
	if (profile_tag_index >= 0) {
		cfg_id_list("favorite_game", CFG.favorite_game,
			&CFG.num_favorite_game, MAX_FAVORITE_GAME);
	}
	cfg_id_list("hide_game", CFG.hide_game,
			&CFG.num_hide_game, MAX_HIDE_GAME);
	game_set(name, val);
	if (cfg_set_gbl(name, val)) {
		CFG.saved_global = 1;
	}
}

// save current state
bool CFG_Save_Global_Settings()
{
	CFG.saved_global = 1;
	STRCOPY(CFG.saved_theme, CFG.theme);
	CFG.saved_device = wbfsDev;
	CFG.saved_gui_rows = grid_rows;
	CFG.saved_gui_style = gui_style;
	if (gui_style == GUI_STYLE_COVERFLOW) {
		CFG.saved_gui_style += CFG_cf_global.theme;
	}
	STRCOPY(CFG.saved_partition, CFG.partition);
	CFG.saved_profile = CFG.current_profile;
	return CFG_Save_Settings(1);
}


bool CFG_Load_Settings()
{
	char pathname[200];
	bool ret;
	int i;
	snprintf(pathname, sizeof(pathname), "%s/%s", USBLOADER_PATH, "settings.cfg");
	// reset
	CFG.num_favorite_game = 0;
	CFG.saved_global = 0;
	CFG.current_profile = 0;
	profile_tag_index = 0;
	memset(&CFG.profile_favorite, 0, sizeof(CFG.profile_favorite));
	memset(&CFG.profile_num_favorite, 0, sizeof(CFG.profile_num_favorite));
	for (i = 0; i < MAX_PROFILES; i++)
		CFG.profile_theme[i] = -1;
	// load settings
	ret = cfg_parsefile(pathname, &settings_set);
	// set current profile
	i = cfg_find_profile(CFG.current_profile_name);
	CFG.current_profile = i >= 0 ? i : 0;
	// always hide uloader's cfg entry
	i = find_in_game_list((u8*)"__CF", CFG.hide_game, CFG.num_hide_game);
	if (i < 0)
		add_to_list((u8*)"__CF", CFG.hide_game, &CFG.num_hide_game, MAX_HIDE_GAME);
	// remember saved values
	STRCOPY(CFG.saved_theme, CFG.theme);
	CFG.saved_device    = CFG.device;
	CFG.saved_gui_style = CFG.gui_style;
	CFG.saved_gui_rows  = CFG.gui_rows;
	STRCOPY(CFG.saved_partition, CFG.partition);
	CFG.saved_profile   = CFG.current_profile;
	return ret;
}

bool CFG_Save_Settings(int verbose)
{
	FILE *f;
	int i, j;
	struct stat st;
	char pathname[200];

	snprintf(D_S(pathname), "%s/", FAT_DRIVE);
	if (stat(pathname, &st) == -1) {
		printf(gt("ERROR: %s is not accessible"), pathname);
		printf("\n");
		sleep(1);
		return false;
	}
	if (stat(USBLOADER_PATH, &st) == -1) {
		//mkpath("sd:/usb-loader", 0777);
		printf(gt("ERROR: %s is not accessible"), USBLOADER_PATH);
		printf("\n");
		sleep(1);
		return false;
	}

	snprintf(D_S(pathname), "%s/%s", USBLOADER_PATH, "settings.cfg");
	f = fopen(pathname, "wb");
	if (!f) {
		printf(gt("Error saving %s"), pathname);
		printf("\n");
		sleep(1);
		return false;
	}
	fprintf(f, "# CFG USB Loader settings file %s\n", CFG_VERSION);
	fprintf(f, "# Note: This file is automatically generated\n");

	fprintf(f, "\n# Global Settings:\n");
	#define SAVE_OPT(FMT, VAL) do { \
		fprintf(f,FMT,VAL); \
		if (verbose) printf_(FMT,VAL); } while(0)
	if (CFG.saved_global) {
		if (*CFG.saved_theme) {
			SAVE_OPT("theme = %s\n", CFG.saved_theme);
		}
		char *dev_str[] = { "ask", "usb", "sdhc" };
		if (CFG.saved_device >= 0 && CFG.saved_device <= 2) {
			SAVE_OPT("device = %s\n", dev_str[CFG.saved_device]);
		}
		SAVE_OPT("partition = %s\n", CFG.saved_partition);
		char *s = map_get_name(map_gui_style, CFG.saved_gui_style);
		if (s) {
			SAVE_OPT("gui_style = %s\n", s);
		}
		if (CFG.saved_gui_rows > 0 && CFG.saved_gui_rows <= 4 ) {
			SAVE_OPT("gui_rows = %d\n", CFG.saved_gui_rows);
		}
		//SAVE_OPT("dml = %d\n", CFG.dml);
		SAVE_OPT("devo = %d\n", CFG.default_gc_loader);
		SAVE_OPT("old_button_color = %d", CFG.old_button_color);
	}

	fprintf(f, "\n# Profiles: %d\n", CFG.num_profiles);
	int save_prof = CFG.current_profile;
	for (j=0; j<CFG.num_profiles; j++) {
		CFG.current_profile = j;
		fprintf(f, "[profile=%s]\n", CFG.profile_names[j]);
		//fprintf(f, "# Profile: [%d] %s\n", j+1, CFG.profile_names[j]);
		if (CFG.profile_theme[j] != -1) {
			fprintf(f, "profile_theme = %s\n", theme_list[CFG.profile_theme[j]]);
		}
		fprintf(f, "# Favorite Games: %d\n", CFG.num_favorite_game);
		for (i=0; i<CFG.num_favorite_game; i++) {
			fprintf(f, "favorite_game = %.4s\n", CFG.favorite_game[i]);
		}
	}
	CFG.current_profile = save_prof;
	fprintf(f, "# Current profile: %d\n", CFG.saved_profile + 1);
	fprintf(f, "profile = %s\n", CFG.profile_names[CFG.saved_profile]);

	fprintf(f, "\n# Hidden Games: %d\n", CFG.num_hide_game);
	for (i=0; i<CFG.num_hide_game; i++) {
		fprintf(f, "hide_game = %.4s\n", CFG.hide_game[i]);
	}

	int saved_cfg_game = 0;
	for (i=0; i<num_cfg_game; i++) {
		if (cfg_game[i].is_saved) saved_cfg_game++;
	}
	fprintf(f, "\n# Game Options: %d\n", saved_cfg_game);
	for (i=0; i<num_cfg_game; i++) {
		if (!cfg_game[i].is_saved) continue;
		struct Game_CFG *game_cfg = &cfg_game[i].save;
		char *s;
		fprintf(f, "game:%s = ", cfg_game[i].id);
		#define SAVE_STR(N, S) \
			if (S) fprintf(f, "%s:%s; ", N, S)
		#define SAVE_NUM(N) \
			fprintf(f, "%s:%d; ", #N, game_cfg->N)
		#define SAVE_BOOL(N) SAVE_NUM(N)
		s = map_get_name(map_language, game_cfg->language);
		SAVE_STR("language", s);
		s = map_get_name(map_video, game_cfg->video);
		SAVE_STR("video", s);
		s = map_get_name(map_video_patch, game_cfg->video_patch);
		SAVE_STR("video_patch", s);
		s = map_get_name(map_nand_emu, game_cfg->nand_emu);
		SAVE_STR("nand_emu", s);
		if (strlen((char*)cfg_game[i].id) == 4)
			s = map_get_name(map_channel_boot, game_cfg->channel_boot);
		else
			s = map_get_name(map_gc_boot, game_cfg->channel_boot);
		SAVE_STR("channel_boot", s);
		SAVE_BOOL(vidtv);
		SAVE_BOOL(wide_screen);
		SAVE_BOOL(ntsc_j_patch);
		SAVE_BOOL(nodisc);
		SAVE_BOOL(screenshot);
		SAVE_BOOL(country_patch);
		SAVE_BOOL(fix_002);
		s = ios_str(game_cfg->ios_idx);
		SAVE_STR("ios", s);
		if (game_cfg->block_ios_reload == 2) {
			SAVE_STR("block_ios_reload", "auto");
		} else {
			SAVE_BOOL(block_ios_reload);
		}
		if (game_cfg->alt_dol < ALT_DOL_DISC) {
			SAVE_BOOL(alt_dol);
		} else {
			if (game_cfg->alt_dol == ALT_DOL_DISC) {
				SAVE_STR("alt_dol", "disc");
			} else {
				SAVE_NUM(alt_dol);
			}
			if (*game_cfg->dol_name) {
				SAVE_STR("dol_name", game_cfg->dol_name);
			}
		}
		SAVE_BOOL(ocarina);
		s = map_get_name(map_hook, game_cfg->hooktype);
		SAVE_STR("hooktype", s);
		SAVE_NUM(write_playlog);
		SAVE_BOOL(alt_controller_cfg);
		if (game_cfg->clean == CFG_CLEAN_OFF) {
			SAVE_STR("clear_patches", "0");
		} else if (game_cfg->clean == CFG_CLEAN_ON) {
			SAVE_STR("clear_patches", "1");
		} else {
			SAVE_STR("clear_patches", "all");
		}
		fprintf(f, "\n");
	}
	fprintf(f, "# END\n");
	fclose(f);
	return true;
}


// PER-GAME options

// find game cfg
struct Game_CFG_2* CFG_find_game(u8 *id)
{
	int i;
	for (i=0; i<num_cfg_game; i++) {
		if (strncmp((char*)id, (char*)cfg_game[i].id, 6) == 0) {
			return &cfg_game[i];
		}
	}
	return NULL;
}

// find game cfg by id
struct Game_CFG CFG_read_active_game_setting(u8 *id)
{
	struct Game_CFG_2 *game = CFG_find_game(id);
	if (game != NULL)
		return game->curr;
	else
		return CFG.game;
}

// current options to game
void cfg_init_game(struct Game_CFG_2 *game, u8 *id)
{
	memset(game, 0, sizeof(*game));
	game->curr = CFG.game;
	game->save = CFG.game;
	memcpy((char*)game->id, (char*)id, 6);
	game->id[6] = 0;
	game->is_saved = 0;
}

// return existing or new
struct Game_CFG_2* CFG_get_game(u8 *id)
{
	struct Game_CFG_2 *game = CFG_find_game(id);
	if (game) return game;
	if (num_cfg_game >= MAX_CFG_GAME) return NULL;
	game = &cfg_game[num_cfg_game];
	num_cfg_game++;
	cfg_init_game(game, id);
	return game;
}

// free game opt slot
void cfg_free_game(struct Game_CFG_2* game)
{
	int i;
	if (num_cfg_game == 0 || !game) return;
	// move entries down
	num_cfg_game--;
	for (i=game-cfg_game; i<num_cfg_game; i++) {
		cfg_game[i] = cfg_game[i+1];
	}
	memset(&cfg_game[num_cfg_game], 0, sizeof(struct Game_CFG_2));
}

void game_set(char *name, char *val)
{
	// sample line:
	// game:RTNP41 = video:game; language:english; ocarina:0;
	// game:RYWP01 = video:patch; language:console; ocarina:1;
	//printf("GAME: '%s=%s'\n", name, val);
	u8 id[8];
	struct Game_CFG_2 *game;
	if (strncmp(name, "game:", 5) != 0) return;
	split_token((char*)id, name+5, ' ', sizeof(id)); 
	if ((strlen((char*)id) != 6) && (strlen((char*)id) != 4)) return;
	game = CFG_get_game(id);
	// set id and current options as default
	cfg_init_game(game, id);
	//printf("GAME(%s) '%s'\n", id, val); sleep(1);

	// parse val
	// first split options by ;
	char opt[100], *p;
	p = val;

	while(p) {
		p = split_token(opt, p, ';', sizeof(opt));
		//printf("GAME(%s) (%s)\n", id, opt); sleep(1);
		// parse opt 'language:english'
		char opt_name[100], opt_val[100];
		if (trimsplit(opt, opt_name, opt_val, ':', sizeof(opt_name))){
			//printf("GAME(%s) (%s=%s)\n", id, opt_name, opt_val); sleep(1);
			cfg_set_game(opt_name, opt_val, &game->save);
		}
	}
	game->curr = game->save;
	game->is_saved = 1;
}

bool CFG_is_saved(u8 *id)
{
	struct Game_CFG_2 *game;
	game = CFG_find_game(id);
	if (game == NULL) return false;
	//return true;
	return game->is_saved;
}

// clean up so it's suitable for binary compare
void cfg_game_clean(struct Game_CFG *src, struct Game_CFG *dest)
{
	*dest = *src;
	memset(dest->dol_name, 0, sizeof(dest->dol_name));
	if (dest->alt_dol == ALT_DOL_DISC) {
		STRCOPY(dest->dol_name, src->dol_name);
	}
}

// return true if differ
bool cfg_game_differ(struct Game_CFG *g1, struct Game_CFG *g2)
{
	// first make clean
	struct Game_CFG gg1, gg2;
	cfg_game_clean(g1, &gg1);
	cfg_game_clean(g2, &gg2);
	return memcmp(&gg1, &gg2, sizeof(gg1));
}

bool cfg_game_changed(struct Game_CFG_2 *game)
{
	if (game->is_saved) {
		// compare curr vs saved
		return cfg_game_differ(&game->curr, &game->save);
	}
	// curr vs global
	return cfg_game_differ(&game->curr, &CFG.game);
}

bool CFG_is_changed(u8 *id)
{
	struct Game_CFG_2 *game = CFG_find_game(id);
	if (!game) return false;
	return cfg_game_changed(game);
}

bool CFG_save_game_opt(u8 *id)
{
	struct Game_CFG_2 *game = CFG_get_game(id);
	if (!game) return false;
	game->save = game->curr;
	game->is_saved = 1;
	if (CFG_Save_Settings(0)) return true;
	return false;
}

bool CFG_discard_game_opt(u8 *id)
{
	struct Game_CFG_2 *game = CFG_find_game(id);
	if (!game) return false;
	// reset options
	cfg_init_game(game, id);
	return CFG_Save_Settings(0);
}

void CFG_release_game(struct Game_CFG_2 *game)
{
	if (game->is_saved) return;
	if (cfg_game_changed(game)) return;
	// not saved and not changed - free slot
	cfg_free_game(game);
}

// theme

void get_theme_list()
{
	//DIR *dir;
	DIR_ITER *dir;
	//struct dirent *dent;
	struct stat st;
	char fname[1024] = "";
	char theme_base[200] = "";
	char theme_dir[200] = "";
	char theme_file[200] = "";
	int i;

	//dbg_time1();

	snprintf(theme_base, sizeof(theme_base), "%s/themes", USBLOADER_PATH);
	//dir = opendir(theme_base);
	dir = diropen(theme_base);
	if (!dir) return;

	//while ((dent = readdir(dir)) != NULL) {
	while (dirnext(dir, fname, &st) == 0) {
		//if (strlen(dent->d_name) >= sizeof(theme_list[0]) || dent->d_name[0] == '.') continue;
		if (strlen(fname) >= sizeof(theme_list[0]) || fname[0] == '.') continue;
		//snprintf(theme_dir, sizeof(theme_dir), "%s/%s", theme_base, dent->d_name);
		snprintf(theme_dir, sizeof(theme_dir), "%s/%s", theme_base, fname);
		//if (stat(theme_dir, &st) != 0) continue;
		if (!S_ISDIR(st.st_mode)) continue;
		snprintf(theme_file, sizeof(theme_file), "%s/theme.txt", theme_dir);
		if (stat(theme_file, &st) != 0) continue;
		if(S_ISREG(st.st_mode)) {
			// theme found
			//printf("Theme: %s\n", dent->d_name);
			if (num_theme >= MAX_THEME) break;
			//strcopy(theme_list[num_theme], dent->d_name, sizeof(theme_list[num_theme]));
			strcopy(theme_list[num_theme], fname, sizeof(theme_list[num_theme]));
			num_theme++;
		}
	}
	//closedir(dir);	
	dirclose(dir);	

	if (!num_theme) return;
	qsort(theme_list, num_theme, sizeof(*theme_list),
			(int(*)(const void*, const void*))stricmp);
	// define current
	if (!*CFG.theme) return;
	for (i=0; i<num_theme; i++) {
		if (stricmp(CFG.theme, theme_list[i]) == 0) {
			cur_theme = i;
			strcopy(CFG.theme, theme_list[i], sizeof(CFG.theme));
			break;
		}
	}
}

int find_button(int value) {
	if (CFG.button_A == value) return NUM_BUTTON_A;
	if (CFG.button_B == value) return NUM_BUTTON_B;
	if (CFG.button_1 == value) return NUM_BUTTON_1;
	if (CFG.button_2 == value) return NUM_BUTTON_2;
	if (CFG.button_M == value) return NUM_BUTTON_MINUS;
	if (CFG.button_P == value) return NUM_BUTTON_PLUS;
	if (CFG.button_H == value) return NUM_BUTTON_HOME;
	if (CFG.button_X == value) return NUM_BUTTON_X;
	if (CFG.button_Y == value) return NUM_BUTTON_Y;
	if (CFG.button_Z == value) return NUM_BUTTON_Z;
	if (CFG.button_C == value) return NUM_BUTTON_C;
	if (CFG.button_L == value) return NUM_BUTTON_L;
	if (CFG.button_R == value) return NUM_BUTTON_R;
	return 0;
}

void set_chars(void) {
	CFG.button_gui = find_button(CFG_BTN_GUI);
	CFG.button_opt = find_button(CFG_BTN_OPTIONS);
	CFG.button_fav = find_button(CFG_BTN_FAVORITES);
}

void load_theme(char *theme)
{
	char pathname[200];
	struct stat st;

	if (!*theme) return;

	snprintf(D_S(theme_path), "%s/themes/%s", USBLOADER_PATH, theme);
	snprintf(D_S(pathname), "%s/theme.txt", theme_path);
	if (stat(pathname, &st) == 0) {
		CFG_Default_Theme();
		STRCOPY(CFG.theme, theme);
		copy_theme_path(CFG.background, "background.png", sizeof(CFG.background));
		if (!copy_theme_path(CFG.w_background, "background_wide.png",
					sizeof(CFG.w_background)))
		{
			// if not found use normal
			*CFG.w_background = 0;
		}
		copy_theme_path(CFG.bg_gui, "bg_gui.png", sizeof(CFG.bg_gui));
		if (!copy_theme_path(CFG.bg_gui_wide, "bg_gui_wide.png",
					sizeof(CFG.bg_gui_wide)))
		{
			// if themed bg_gui_wide.png not found, use themed bg_gui.png
			// instead of global bg_gui_wide.png
			*CFG.bg_gui_wide = 0;
		}
		cfg_parsefile(pathname, &theme_set);
		// override some theme options with config.txt:
		snprintf(D_S(pathname), "%s/config.txt", USBLOADER_PATH);
		cfg_parsefile(pathname, &theme_set_base);
		if (strcmp(USBLOADER_PATH, APPS_DIR) != 0) {
			snprintf(D_S(pathname), "%s/config.txt", APPS_DIR);
			cfg_parsefile(pathname, &theme_set_base);
		}
		STRCOPY(CFG.theme_path, theme_path);		
		makeButtonMap();
		set_chars();
	}
	*theme_path = 0;
}

void CFG_switch_theme(int theme_i)
{
	if (num_theme <= 0) return;
	if (cur_theme == -1) theme_i = 0;
	if (theme_i < 0) theme_i = num_theme - 1;
	if (theme_i >= num_theme) theme_i = 0;
	load_theme(theme_list[theme_i]);
	cur_theme = theme_i;
	__console_flush(0);
	cfg_setup2();
	//Gui_DrawBackground();
	Gui_LoadBackground();
	Gui_InitConsole();
	cfg_setup3();
	wgui_inited = 0;
}

char *get_clock_str(time_t t)
{
	static char clock_str[16];
	char *fmt = "%H:%M";
	if (CFG.clock_style == 12) fmt = "%I:%M";
	if (CFG.clock_style == 13) fmt = "%I:%M %p";
	strftime(clock_str, sizeof(clock_str), fmt, localtime(&t));
	return clock_str;
}

//extern u8 __text_start[];
//extern u8 __Arena1Lo[], __Arena1Hi[];
//extern u8 __Arena2Lo[], __Arena2Hi[];
void test_unicode()
{
	wchar_t wc;
	char mb[32*6+1];
	int i, k, len=0, count=0;
	printf("\n");
	for (i=0; i<512; i++) {
		wc = i;
		switch (wc) {
			case '\n': strcat(mb, "n"); k=1; break;
			case '\r': strcat(mb, "r"); k=1; break;
			case '\b': strcat(mb, "b"); k=1; break;
			case '\f': strcat(mb, "f"); k=1; break;
			case '\t': strcat(mb, "t"); k=1; break;
			case 0x1b: strcat(mb, "e"); k=1; break; // esc
			default:
			k = wctomb(mb+len, wc);
			if (k < 1 || mb[len] == 0) {
				if (wc < 128) {
					mb[len] = '!'; //wc;
				} else {
					mb[len] = '?';
				}
				k = 1;
			}
		}
		len += k;
		mb[len] = 0;
		count++;
		if (count >= 32) {
			printf("%03x: %s |\n", (unsigned)i-31, mb);
			len = 0;
			count = 0;
		}
	}
	printf("\n");
	Menu_PrintWait();
	printf("\n");
}

/*
debug codes:
1 : basic
2 : unicode
3 : coverflow
4 : banner sound
8 : io benchmark
*/

void cfg_debug(int argc, char **argv)
{
	InitDebug();

	if (CFG.debug) {
		Gui_Console_Enable();
		__console_scroll = 1;
	}
	dbg_printf("debug: %d %d\n", CFG.debug, CFG.debug_gecko);
	dbg_printf("base_path: %s\n", USBLOADER_PATH);
	dbg_printf("apps_path: %s\n", APPS_DIR);
	//dbg_printf("bg_path: %s\n", *CFG.background ? CFG.background : "builtin");
	dbg_printf("covers: %s\n", CFG.covers_path);
	dbg_printf("theme: %s\n", CFG.theme_path);
	/*
	dbg_printf("theme: %s \n", CFG.theme);
	dbg_printf("covers: %d ", CFG.covers);
	dbg_printf("w: %d ", COVER_WIDTH);
	dbg_printf("h: %d ", COVER_HEIGHT);
	dbg_printf("layout: %d ", CFG.layout);
	dbg_printf("video: %d ", CFG.game.video);
	dbg_printf("home: %d ", CFG.home);
	dbg_printf("ocarina: %d ", CFG.game.ocarina);
	dbg_printf("titles: %d ", num_title);
	dbg_printf("maxc: %d ", MAX_CHARACTERS);
	dbg_printf("ent: %d ", ENTRIES_PER_PAGE);
	*/
	dbg_printf("url: %s\n", CFG.cover_url_2d);
	//dbg_printf("t[%.6s]=%s\n", cfg_title[0].id, cfg_title[0].title);
	//dbg_printf("hide_hdd: %d\n", CFG.hide_hddinfo);
	//dbg_printf("text start: %p\n", __text_start);
	//dbg_printf("M1: %p - %p\n", __Arena1Lo, __Arena1Hi);
	//dbg_printf("M2: %p - %p\n", __Arena2Lo, __Arena2Hi);
	int i;
	dbg_printf("hide: %d", CFG.num_hide_game);
	for (i=0; i<CFG.num_hide_game; i++) {
		dbg_printf(" %.4s", CFG.hide_game[i]);
	}
	dbg_printf("\n");
	dbg_printf("args[%d]:", argc);
	for (i=0; i<argc; i++) {
		dbg_printf(" [%d]=%s", i, argv[i]);
	}
	dbg_printf("\n");
	dbg_printf("music: %d ", CFG.music);
	dbg_printf("gui: %d ", CFG.gui);
	extern char *get_cc();
	dbg_printf("CC: %s ", get_cc());
	dbg_printf("\n");
	dbg_printf("device: %d ", CFG.device);
	dbg_printf("partition: %s\n", CFG.partition);

	if (CFG.debug) {
		Menu_PrintWait();
	}
	if (CFG.debug & 2) test_unicode();
	dbg_printf("\n");
}

void chdir_app(char *arg)
{
	char *pos;
	struct stat st;
	int default_try = 0;
	if (arg == NULL) goto default_dir;
	// replace fat: with sd:
	if (strncmp(arg, "fat", strlen("fat")) == 0) {
		pos = strchr(arg, ':');
		if (!pos) goto default_dir;
		STRCOPY(APPS_DIR, SDHC_MOUNT);
		strcat(APPS_DIR, pos);
	}
	// check if starts with sd: usb: or ntfs:
	else if (strncmp(arg, SDHC_MOUNT, strlen(SDHC_MOUNT)) == 0
		|| strncmp(arg, USB_MOUNT, strlen(USB_MOUNT)) == 0
		|| strncmp(arg, NTFS_MOUNT, strlen(NTFS_MOUNT)) == 0)
	{
		STRCOPY(APPS_DIR, arg);
		// trim possible drive number
		pos = strchr(APPS_DIR, ':');
		if (!pos) goto default_dir;
		pos--;
		if (ISDIGIT(*pos)) {
			memmove(pos, pos+1, strlen(pos));
		}
	} else {
		goto default_dir;
	}
	// trim /boot.dol
	pos = strrchr(APPS_DIR, '/');
	if (!pos) goto default_dir;
	*pos = 0;
	// still a valid path?
	pos = strchr(APPS_DIR, '/');
	if (!pos) goto default_dir;
	goto try_chdir;

	// no path in arg, try the default APPS_DIR
	default_dir:
	snprintf(D_S(APPS_DIR), "%s%s", FAT_DRIVE, "/apps/USBLoader");
	default_try = 1;

	try_chdir:
	if (stat(APPS_DIR, &st) == 0) {
		chdir(APPS_DIR);
	} else {
		if (!default_try) goto default_dir;
		*APPS_DIR = 0;
	}
}   

void setup_theme_preview_coords()
{
	//theme preview settings
	if (CFG.widescreen) {
		//if width and height not set then use cover width
		if ((CFG.w_theme_previewW < 1) && (CFG.w_theme_previewH < 1)) CFG.w_theme_previewW = COVER_WIDTH;
		CFG.theme_previewX = (CFG.w_theme_previewX < 0) ? COVER_XCOORD : CFG.w_theme_previewX;
		CFG.theme_previewY = (CFG.w_theme_previewY < 0) ? COVER_YCOORD : CFG.w_theme_previewY;
		CFG.theme_previewW = (CFG.w_theme_previewW > 0) ? CFG.w_theme_previewW : (CFG.w_theme_previewH * 16 / 13);
		CFG.theme_previewH = (CFG.w_theme_previewH > 0) ? CFG.w_theme_previewH : (CFG.w_theme_previewW * 13 / 16);
	} else {
		//if width and height not set then use cover width
		if ((CFG.theme_previewW < 1) && (CFG.theme_previewH < 1)) CFG.theme_previewW = COVER_WIDTH;
		CFG.theme_previewX = (CFG.theme_previewX < 0) ? COVER_XCOORD : CFG.theme_previewX;
		CFG.theme_previewY = (CFG.theme_previewY < 0) ? COVER_YCOORD : CFG.theme_previewY;
		CFG.theme_previewW = (CFG.theme_previewW > 0) ? CFG.theme_previewW : (CFG.theme_previewH * 4 / 3);
		CFG.theme_previewH = (CFG.theme_previewH > 0) ? CFG.theme_previewH : (CFG.theme_previewW * 3 / 4);		
	}	
}

// after cfg load
void cfg_setup1()
{
	// widescreen
	if (CFG.widescreen == CFG_WIDE_AUTO) {
		int widescreen = CONF_GetAspectRatio();
		if (widescreen) {
			CFG.widescreen = 1;
		} else {
			CFG.widescreen = 0;
		}
	}
	// set current profile
	int i = cfg_find_profile(CFG.current_profile_name);
	CFG.current_profile = i >= 0 ? i : 0;
}

// before video & console init
void cfg_setup2()
{
	// setup cover style: path & url
	//cfg_setup_cover_style();

	// setup cover dimensions
	CFG.N_COVER_WIDTH = COVER_WIDTH;
	CFG.N_COVER_HEIGHT = COVER_HEIGHT;
	
	// setup widescreen coords
	if (CFG.widescreen) {
		// copy over
		CONSOLE_XCOORD = CFG.W_CONSOLE_XCOORD;
		CONSOLE_YCOORD = CFG.W_CONSOLE_YCOORD;
		CONSOLE_WIDTH  = CFG.W_CONSOLE_WIDTH;
		CONSOLE_HEIGHT = CFG.W_CONSOLE_HEIGHT;
		COVER_XCOORD   = CFG.W_COVER_XCOORD;
		COVER_YCOORD   = CFG.W_COVER_YCOORD;
		if (*CFG.w_background) {
			STRCOPY(CFG.background, CFG.w_background);
		}
		if (!CFG.W_COVER_WIDTH || !CFG.W_COVER_HEIGHT) {
			// if either wide val is 0: automatic resize
			// normal (4:3): COVER_WIDTH = 160;
			// wide (16:9): COVER_WIDTH = 130;
			// ratio: *13/16
			// although true 4:3 -> 16:9 should be
			// 4:3=12:9 -> 16:9 => *12/16,
			// but it looks too compressed
			// and align to multiple of 2
			CFG.W_COVER_WIDTH = (COVER_WIDTH * 13 / 16) / 2 * 2;
			// height is same
			CFG.W_COVER_HEIGHT = COVER_HEIGHT;
		}
		COVER_WIDTH = CFG.W_COVER_WIDTH;
		COVER_HEIGHT = CFG.W_COVER_HEIGHT;
	}
	
	setup_theme_preview_coords();
	expand_font_color_cfg(&CFG.gui_text, &CFG.gui_text_cfg);
	expand_font_color_cfg(&CFG.gui_text2, &CFG.gui_text2_cfg);
}

// This has to be called AFTER video & console init
void cfg_setup3()
{
	// auto set max_chars & entries
	int cols, rows;
	CON_GetMetrics(&cols, &rows);

	//MAX_CHARACTERS = cols - 4;
	MAX_CHARACTERS = cols - 2 - strlen(CFG.cursor);
	if (CFG.console_mark_saved) MAX_CHARACTERS--;

	ENTRIES_PER_PAGE = rows - 5;
	// correct ENTRIES_PER_PAGE
	if (CFG.db_show_info) {  // lustar
		ENTRIES_PER_PAGE -= 2;
	}
	if (CFG.hide_header) ENTRIES_PER_PAGE++;
	if (CFG.hide_hddinfo) ENTRIES_PER_PAGE++;
	//if (CFG.buttons == CFG_BTN_OPTIONS_1 || CFG.buttons == CFG_BTN_OPTIONS_B) {
	if (CFG.hide_footer) ENTRIES_PER_PAGE++;
	//} else {
	//	ENTRIES_PER_PAGE--;
	//	if (CFG.hide_footer) ENTRIES_PER_PAGE+=2;
	//}
}


void cfg_direct_start(int argc, char **argv)
{
	// loadstructor
	// patched buffer in binary
	char *direct_start_arg = NULL;
	//if (strcmp(direct_start_id_buf, "#GAMEID") != 0)
	// we can't compare "#GAMEID" directly because
	// it has to be a unique string inside the binary
	char *p = direct_start_id_buf;
	if (p[0] != '#' ||
		p[1] != 'G' ||
		p[2] != 'A' ||
		p[3] != 'M' ||
		p[4] != 'E' ||
		p[5] != 'I' ||
		p[6] != 'D' )
	{
		direct_start_arg = direct_start_id_buf + 1;
	}
	// uloader forwarder arg variant
	// argv[1] = "#RHAP01" or "#RHAP01-2"
	if(argc>1 && argv && argv[1]) {
		if(argv[1][0] == '#') {
			direct_start_arg = argv[1] + 1;
		} else {
			extern bool is_gameid(char *id);
			if ((strlen(argv[1]) == 6) && is_gameid(argv[1])) {
				direct_start_arg = argv[1];
			}
		}
	}
	// is direct start?
	if (direct_start_arg) {
		char *t = direct_start_arg;
		// game id
		memcpy(CFG.launch_discid, t, 6);
		t += 6;
		if (t[0] == '-') {
			int part_num = t[1] - '0';
			//CFG.current_partition = t[1] - 48;
			//CFG.current_partition &= 3;
			if (part_num < 4) {
				sprintf(CFG.partition, "WBFS%d", part_num + 1);
			} else {
				sprintf(CFG.partition, "FAT%d", part_num - 4 + 1);
			}
		}
		// setup cfg for auto-start
		CFG.direct_launch = 1;
		CFG_Default_Theme();
		CFG.device = CFG_DEV_USB;
		*CFG.background = 0;
		*CFG.w_background = 0;
		CFG.covers = 0;
		CFG.gui = 0;
		CFG.music = 0;
		CFG.confirm_start = 0;
		//CFG.debug = 0;
	}
}

int CFG_MountUSB()
{
	static int first_time = 1;
	int ret;
	if (!first_time) return -1;
	ret = MountUSB();
	if (ret == 0) return 0; // OK
	ret = Retry_Init_Dev(WBFS_DEVICE_USB, 0);
	if (ret != 0) return -1;
	return MountUSB();
}

// This has to be called BEFORE video & console init
void CFG_Load(int argc, char **argv)
{
	char *arg0 = NULL;
	char filename[200];
	struct stat st;
	bool try_sd = true;
	bool try_usb = false;
	bool try_ntfs = false;
	bool ret;

	strcpy(FAT_DRIVE, SDHC_DRIVE);

	// are we started from usb? then check usb first.
	// also if wiiload supplied first argument is USB_DRIVE, start with usb.
	if (argc && argv && argv[0]) {

		arg0 = argv[0];
		// verify that argv[0] is an actual full pathname
		if (strstr(argv[0], ":/") == NULL) {
			// it's not, check argv[1] instead
			// can be useful when running from wiiload
			if (argc > 1 && argv[1] && strchr(argv[1], ':')
					&& !strchr(argv[1], '=')) // not an option
			{
				arg0 = argv[1];
				dbg_printf("a1:%s\n", arg0);
			}
		}

		// ignore : in case it's numbered
		if (strncmp(arg0, USB_MOUNT, strlen(USB_MOUNT)) == 0)
		{
			if (CFG_MountUSB() == 0 && mount_find(USB_DRIVE)) {
				strcpy(FAT_DRIVE, USB_DRIVE);
				try_usb = true;
				try_sd = false;
			}
		}
		// same logic applies to ntfs:
		else if (strncmp(arg0, NTFS_MOUNT, strlen(NTFS_MOUNT)) == 0)
		{
			if (CFG_MountUSB() == 0 && mount_find(NTFS_DRIVE)) {
				strcpy(FAT_DRIVE, NTFS_DRIVE);
				try_ntfs = true;
				try_sd = false;
			}
		}
	}

	retry:

	snprintf(D_S(USBLOADER_PATH), "%s%s", FAT_DRIVE, "/usb-loader");

	// Set current working directory to app path
	chdir_app(arg0);

	// setup defaults
  	//CFG_Default(); // already called by cfg_parsearg_early
  	CFG_default_path();

	// load global config
	snprintf(D_S(filename), "%s/%s", USBLOADER_PATH, "config.txt");
	ret = cfg_parsefile(filename, &cfg_set);

	// try old location if default fails
	/*
	if (!ret) {
		char pathname[200];
		snprintf(D_S(pathname), "%s%s", FAT_DRIVE, "/USBLoader");
		snprintf(D_S(filename), "%s/%s", pathname, "config.txt");
		if (stat(filename, &st) == 0) {
			// exists, use old base path
			STRCOPY(USBLOADER_PATH, pathname);
		  	CFG_default_path(); // reset defaults with old path
			ret = cfg_parsefile(filename, &cfg_set);
		}
	}
	*/

	// still no luck? try APPS_DIR
	if (!ret && *APPS_DIR) {
		snprintf(D_S(filename), "%s/%s", APPS_DIR, "config.txt");
		if (stat(filename, &st) == 0) {
			// exists, use APPS_DIR as base
			STRCOPY(USBLOADER_PATH, APPS_DIR);
		  	CFG_default_path(); // reset defaults with APPS_DIR
			ret = cfg_parsefile(filename, &cfg_set);
		}
	}

	// still no luck? try USB
	if (!ret) {
		if (!try_usb) {
			try_usb = true;
			if (CFG_MountUSB() == 0) {
				strcpy(FAT_DRIVE, USB_DRIVE);
				goto retry;
			}
		}
		if (!try_ntfs) {
			try_ntfs = true;
			if (CFG_MountUSB() == 0) {
				strcpy(FAT_DRIVE, NTFS_DRIVE);
				goto retry;
			}
		}
		if (!try_sd) {
			try_sd = true;
			strcpy(FAT_DRIVE, SDHC_DRIVE);
			goto retry;
		} else {
			// still no dir found, just reset to sd:
			if (strcmp(FAT_DRIVE, SDHC_DRIVE) != 0) {
				strcpy(FAT_DRIVE, SDHC_DRIVE);
				goto retry;
			}
		}
	}
	STRCOPY(LAST_CFG_PATH, USBLOADER_PATH);

	// load renamed titles
	snprintf(filename, sizeof(filename), "%s/%s", USBLOADER_PATH, "titles.txt");
	cfg_parsefile(filename, &title_set);

	// load per-app local config and titles
	// (/apps/USBLoader/config.txt and titles.txt if run from HBC)
	if (strcmp(USBLOADER_PATH, APPS_DIR) != 0) {
		if (cfg_parsefile("config.txt", &cfg_set)) {
			STRCOPY(LAST_CFG_PATH, APPS_DIR);
		}
		cfg_parsefile("titles.txt", &title_set);
	}

	// load custom titles
	snprintf(filename, sizeof(filename), "%s/%s", USBLOADER_PATH, "custom-titles.txt");
	cfg_parsefile(filename, &title_set);

	// get theme list
	get_theme_list();

	// load per-game settings
	CFG_Load_Settings();

	// loadstructor direct start support
	cfg_direct_start(argc, argv);

	// parse commandline arguments (wiiload)
	cfg_parsearg(argc, argv);

	// setup dependant and calculated parameters
	cfg_setup1();
	cfg_setup2();
	
	// remount NTFS if write was enabled
	RemountNTFS();
}



void CFG_Setup(int argc, char **argv)
{
	get_time(&TIME.misc1);
	cfg_setup3();

	if (CFG.debug) Gui_Console_Enable();
	
	char language_file[255];
	snprintf(language_file, sizeof(language_file), "%s/languages/%s.lang", USBLOADER_PATH, CFG.translation);
	FILE * f;
    f = fopen(language_file,"r");
    if (f) {
        fclose(f);
		gettextLoadLanguage(language_file);
	}

	button_names[NUM_BUTTON_LEFT] = gt("LEFT");
	button_names[NUM_BUTTON_RIGHT] = gt("RIGHT");
	button_names[NUM_BUTTON_UP] = gt("UP");
	button_names[NUM_BUTTON_DOWN] = gt("DOWN");

	// read playstats
	if (!playStatsRead) {
		readPlayStats();
	}
	// load unifont
	char fname[200];
	char * dbl = ((strlen(CFG.db_language) == 2) ? VerifyLangCode(CFG.db_language) : ConvertLangTextToCode(CFG.db_language));
	if ((!strncmp(CFG.translation,"JA",2))
	  ||(!strncmp(CFG.translation,"KO",2))
	  ||(!strncmp(CFG.translation,"ZH",2))
	  ||(!strncmp(dbl,"JA",2))
	  ||(!strncmp(dbl,"KO",2))
	  ||(!strncmp(dbl,"ZH",2)))
		CFG.load_unifont = true;			//all require unifont
	if (CFG.load_unifont) {
		snprintf(D_S(fname), "%s/unifont.dat", USBLOADER_PATH);
		console_load_unifont(fname);
	}
	get_time(&TIME.misc2);
	// load database
	get_time(&TIME.wiitdb1);
	ReloadXMLDatabase(USBLOADER_PATH, CFG.db_language, 0);
	get_time(&TIME.wiitdb2);
	cfg_debug(argc, argv);
}

u32 getPlayCount(u8 *id) {
	int n;
	for (n=0; n<playStatsSize; n++) {
		if (!strcmp(playStats[n].id, (char *)id))
			return playStats[n].playCount;
	}
	return 0;
}

time_t getLastPlay(u8 *id) {
	int n;
	for (n=0; n<playStatsSize; n++) {
		if (!strcmp(playStats[n].id, (char *)id))
			return playStats[n].playTime;
	}
	return 0;
}

int readPlayStats() {
	int n = 0;
	char filepath[MAXPATHLEN];
	snprintf(filepath, sizeof(filepath), "%s/playstats.txt", USBLOADER_PATH);
	time_t start;
	int count;
	char id[7];
	FILE *f;
	f = fopen(filepath, "r");
	if (!f) {
		return -1;
	}
	while (fscanf(f, "%[^:]:%d:%ld\n", id, &count, &start) == 3) {
		if (n >= playStatsSize) {
			playStatsSize++;
			playStats = (struct playStat*)realloc(playStats, (playStatsSize * sizeof(struct playStat)));
			if (!playStats)
				return -1;
		}
		strncpy(playStats[n].id, id, 7);
		playStats[n].playCount = count;
		playStats[n].playTime = start;
		n++;
	}
	fclose(f);
	playStatsRead = true;
	return 0;
}

int setPlayStat(u8 *id) {
	if (CFG.write_playstats == 0)
		return 0;
	char filepath[MAXPATHLEN];
	time_t now;
	int n = 0;
	int count = 0;
	snprintf(filepath, sizeof(filepath), "%s/playstats.txt", USBLOADER_PATH);
	FILE *f;

	if (!playStatsRead) {
		readPlayStats();
	}

	f = fopen(filepath, "w");
	if (!f) return -1;
	for (n=0; n<playStatsSize; n++) {
		if (strcmp(playStats[n].id, (char *)id)) {
			fprintf(f, "%s:%d:%ld\n", playStats[n].id, playStats[n].playCount, playStats[n].playTime);
		} else {
			count = playStats[n].playCount;
		}
	}
	now = time(NULL);
	fprintf(f, "%s:%d:%ld\n", (char *)id, count+1, now);
	fclose(f);
	return 1;
}


