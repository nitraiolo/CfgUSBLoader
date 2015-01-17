#ifndef _MENU_H_
#define _MENU_H_

//#define DEBUG_MODE			1
#define MENU_SELECT_GAME	0
#define MENU_CONFIG_GAME	1
#define MENU_SORT_GAMES		2
#define MENU_SHOW_GAME		3
#define MENU_START_GAME		4
#define MENU_HOME			5
#define MENU_MANAGE			6
#define MENU_DOWNLOAD		7
#define MENU_EXIT			8

#define MIGHTY_VERSION		11
#define HOTSPOT_LEFT		MAXHOTSPOTS-2
#define HOTSPOT_RIGHT		MAXHOTSPOTS-1

// Prototypes
typedef struct{
	u64 idInt;
	char id[5];
	char* title;

	u8 videoMode;
	u8 videoPatch;
	u8 language;
	u8 hooktype;
	u8 ocarina;
	u8 debugger;
	u8 bootMethod;
} title; //32 bytes!

void __Start_Game(title game, int ios, int mode);

#endif
