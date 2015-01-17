#ifndef _SORT_H_
#define _SORT_H_


#ifdef __cplusplus
extern "C"
{
#endif

#include "disc.h"

struct Sorts 
{
	char cfg_val[20];
	char name[100];
	int (*sortAsc) (const void *, const void *);
	int (*sortDsc) (const void *, const void *);
};

#define featureCnt 4
#define accessoryCnt 23
#define genreCnt 76
#define sortCnt 10
#define gameTypeCnt 14
#define searchFieldCnt 12
#define searchCompareTypeCnt 6

#define FILTER_ALL			-1
#define FILTER_ONLINE		0
#define FILTER_UNPLAYED		1
#define FILTER_GENRE		2
#define FILTER_FEATURES		3
#define FILTER_CONTROLLER	4
#define FILTER_GAMECUBE		5
#define FILTER_WII			6
#define FILTER_CHANNEL		7
#define FILTER_DUPLICATE_ID3 8
#define FILTER_GAME_TYPE	9
#define FILTER_SEARCH		10

#define GAME_TYPE_Wii			0
#define GAME_TYPE_GameCube		1
#define GAME_TYPE_All_Channels	2
#define GAME_TYPE_Wiiware		3
#define GAME_TYPE_VC_NES		4
#define GAME_TYPE_VC_SNES		5
#define GAME_TYPE_VC_N64		6
#define GAME_TYPE_VC_SMS		7
#define GAME_TYPE_VC_MD			8
#define GAME_TYPE_VC_PCE		9
#define GAME_TYPE_VC_NeoGeo		10
#define GAME_TYPE_VC_Arcade		11
#define GAME_TYPE_VC_C64		12
#define GAME_TYPE_Wii_Channel	13

extern s32 filter_type;
extern s32 filter_index;
extern char search_str[100];
extern int cur_search_field;
extern int cur_search_compare_type;
extern s32 sort_index;
extern bool sort_desc;
extern struct Sorts sortTypes[sortCnt];

extern char *genreTypes[genreCnt][2];
extern char *accessoryTypes[accessoryCnt][2];
extern char *featureTypes[featureCnt][2];
extern char *gameTypes[gameTypeCnt][2];
extern char *searchFields[searchFieldCnt];
extern char *searchCompareTypes[searchCompareTypeCnt];

int (*default_sort_function) (const void *a, const void *b);

int filter_features(struct discHdr *list, int cnt, char *feature, bool requiredOnly);
int filter_online(struct discHdr *list, int cnt, char *ignore, bool notused);
int filter_play_count(struct discHdr *list, int cnt, char *ignore, bool notused);
int filter_controller(struct discHdr *list, int cnt, char *controller, bool requiredOnly);
int filter_genre(struct discHdr *list, int cnt, char *genre, bool notused);
int filter_gamecube(struct discHdr *list, int cnt, char *ignore, bool notused);
int filter_wii(struct discHdr *list, int cnt, char *ignore, bool notused);
int filter_channel(struct discHdr *list, int cnt, char *ignore, bool notused);
int filter_duplicate_id3(struct discHdr *list, int cnt, char *ignore, bool notused);
int filter_game_type(struct discHdr *list, int cnt, char *typeWanted, bool notused);
int filter_game_search(struct discHdr *list, int cnt, char *strWanted, bool notused);
int filter_games(int (*filter) (struct discHdr *, int, char *, bool), char * name, bool num);
int filter_games_set(int type, int index);
void showAllGames(void);

void sortList(int (*sortFunc) (const void *, const void *));
void __set_default_sort(void);
void reset_sort_default();
void sortList_default();
void sortList_set(int index, bool desc);
int Menu_Filter(void);
int Menu_Filter2(void);
int Menu_Filter3(void);
int Menu_Filter4(void);
int Menu_Sort(void);

int get_accesory_id(char *accessory);
const char* get_accesory_name(int i);
int get_feature_id(char *feature);

#ifdef __cplusplus
}
#endif

#endif
