#ifndef _MENU_H_
#define _MENU_H_

#include <gccore.h>
#include "disc.h" // discHdr
#include "cfg.h"

#define PLUGIN_MIGHTY 0
#define PLUGIN_NEEK 1
#define PLUGIN_NINTENDONT 2

extern char *videos[CFG_VIDEO_NUM];
extern char *DML_videos[6];
extern char *languages[CFG_LANG_NUM];
extern char *playlog_name[4];
extern char *str_wiird[3];
extern char *str_dml[8];
extern char *str_nand_emu[3];
extern bool go_gui;

/* Prototypes */
void Menu_Device(void);
void Menu_Format(void);
void Menu_Install(void);
void Menu_Remove(void);
void Menu_Boot(bool disc);
int Menu_Boot_Options(struct discHdr *header, bool disc);
s32 __Menu_GetEntries(void);
void Menu_Loop(void);
void Menu_Options(void);
void Menu_Partition(bool must_select);
void Handle_Home();
void Theme_Update();
void Online_Update();
void Download_Titles();
void Menu_Cheats(struct discHdr *header);
int  Menu_PrintWait();
bool Menu_Confirm(const char *msg);
void Switch_Favorites(bool enable);
extern bool enable_favorite;
void FmtGameInfoLong(u8 *id, int cols, char *game_desc, int size);
void Menu_GameInfoStr(struct discHdr *header, char *str);
void print_debug_hdr(char *str, int size);
void Save_Debug();
void Save_IOS_Hash();
void Menu_Save_Settings();
void Menu_All_IOS_Info();
void Print_SYS_Info_str(char *str, int size, bool noMiosInfo);
int get_button_action(int buttons);
char get_unlock_buttons(int buttons);
void Admin_Unlock(bool unlock);
void Menu_Plugin(int plugin, char arguments[255][255], int argCnt);
 
void __Menu_ShowGameInfo(bool showfullinfo, u8 *id); // Lustar
char *skip_sort_ignore(char *s);

extern struct discHdr *gameList;
extern s32 gameCnt, gameSelected, gameStart;
extern s32 all_gameCnt;

void __Menu_ScrollStartList();
int Retry_Init_Dev(int device, int a_select);

extern struct discHdr *filter_gameList;

struct Menu
{
	int num_opt;
	int current;
	int line_count;
	char *active;
	int active_size;
	int window_size;
	int window_begin;
	int window_items;
	int window_pos;
};

void menu_init(struct Menu *m, int num_opt);
void menu_begin(struct Menu *m);
int  menu_mark(struct Menu *m);
void menu_move(struct Menu *m, int buttons);

void menu_init_active(struct Menu *m, char *active, int active_size);
void menu_jump_active(struct Menu *m);
void menu_move_cap(struct Menu *m);
void menu_move_wrap(struct Menu *m);
void menu_move_adir(struct Menu *m, int dir);
void menu_move_active(struct Menu *m, int buttons);
char m_active(struct Menu *m, int i);

void menu_window_begin(struct Menu *m, int size, int num_items);
bool menu_window_mark(struct Menu *m);
void menu_window_end(struct Menu *m, int cols);

#define MENU_MARK() menu_mark(&menu)

#endif

