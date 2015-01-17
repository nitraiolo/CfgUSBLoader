#include <gccore.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "menu.h"
#include "xml.h"
#include "cfg.h"
#include "video.h"
#include "gui.h"
#include "libwbfs/libwbfs.h"
#include "wbfs_fat.h"
#include "sort.h"
#include "wpad.h"
#include "gettext.h"
#include "gc.h"
#include "disc.h"

extern struct discHdr *all_gameList;
extern struct discHdr *gameList;
extern struct discHdr *filter_gameList;
extern struct discHdr *fav_gameList;
extern bool enable_favorite;
extern s32 filter_gameCnt;
extern s32 fav_gameCnt;

s32 filter_index = -1;
s32 filter_type = -1;
char search_str[100] = "";
int cur_search_field = 0;
int cur_search_compare_type = 0;
s32 sort_index = -1;
bool sort_desc = 0;

s32 default_sort_index = 0;
bool default_sort_desc = 0;
s32 default_filter_type = -1;

HashMap install_time;

s32 __sort_play_date(struct discHdr * hdr1, struct discHdr * hdr2, bool desc);
s32 __sort_install_date(struct discHdr * hdr1, struct discHdr * hdr2, bool desc);
s32 __sort_title(struct discHdr * hdr1, struct discHdr * hdr2, bool desc);
s32 __sort_releasedate(struct discHdr * hdr1, struct discHdr * hdr2, bool desc);
s32 __sort_players(struct discHdr * hdr1, struct discHdr * hdr2, bool desc);
s32 __sort_wifiplayers(struct discHdr * hdr1, struct discHdr * hdr2, bool desc);
s32 __sort_pub(struct discHdr * hdr1, struct discHdr * hdr2, bool desc);
s32 __sort_dev(struct discHdr * hdr1, struct discHdr * hdr2, bool desc);
s32 __sort_id(struct discHdr * hdr1, struct discHdr * hdr2, bool desc);
s32 __sort_players_asc(const void *a, const void *b);
s32 __sort_players_desc(const void *a, const void *b);
s32 __sort_wifiplayers_asc(const void *a, const void *b);
s32 __sort_wifiplayers_desc(const void *a, const void *b);
s32 __sort_pub_asc(const void *a, const void *b);
s32 __sort_pub_desc(const void *a, const void *b);
s32 __sort_dev_asc(const void *a, const void *b);
s32 __sort_dev_desc(const void *a, const void *b);
s32 __sort_releasedate_asc(const void *a, const void *b);
s32 __sort_releasedate_desc(const void *a, const void *b);
s32 __sort_title_asc(const void *a, const void *b);
s32 __sort_title_desc(const void *a, const void *b);
s32 __sort_play_count_asc(const void *a, const void *b);
s32 __sort_play_count_desc(const void *a, const void *b);
s32 __sort_install_date_asc(const void *a, const void *b);
s32 __sort_install_date_desc(const void *a, const void *b);
s32 __sort_play_date_asc(const void *a, const void *b);
s32 __sort_play_date_desc(const void *a, const void *b);
s32 __sort_id_asc(const void *a, const void *b);
s32 __sort_id_desc(const void *a, const void *b);


char *featureTypes[featureCnt][2] =
{
	{ "online",     gts("Online Content") },
	{ "download",   gts("Downloadable Content") },
	{ "score",      gts("Online Score List") },
	{ "nintendods", gts("Nintendo DS Connectivity") },
};

char *accessoryTypes[accessoryCnt][2] =
{
	{ "wiimote",           gts("Wiimote") },
	{ "nunchuk",           gts("Nunchuk") },
	{ "motionplus",        gts("Motion+") },
	{ "gamecube",          gts("Gamecube") },
	{ "nintendods",        gts("Nintendo DS") },
	{ "classiccontroller", gts("Classic Controller") },
	{ "wheel",             gts("Wheel") },
	{ "zapper",            gts("Zapper") },
	{ "balanceboard",      gts("Balance Board") },
	{ "microphone",        gts("Microphone") },
	{ "guitar",            gts("Guitar") },
	{ "drums",             gts("Drums") },
	{ "camera",            gts("Camera") },
	{ "dancepad",          gts("Dance Pad") },
	{ "infinitybase",      gts("Infinity Base") },
	{ "keyboard",          gts("Keyboard") },
	{ "portalofpower",     gts("Portal of Power") },
	{ "skateboard",        gts("Skateboard") },
	{ "totalbodytracking", gts("Total Body Tracking") },
	{ "turntable",         gts("Turntable") },
	{ "udraw",             gts("uDraw GameTablet") },
	{ "wiispeak",          gts("Wii Speak") },
	{ "vitalitysensor",    gts("Vitality Sensor") },	//must be last never used
};

char *gameTypes[gameTypeCnt][2] =
{
	{ "wii",		gts("Wii") },
	{ "gamecube",	gts("GameCube") },
	{ "channel",	gts("All Channels") },
	{ "wiiware",	gts("WiiWare") },
	{ "vc-nes",		gts("VC-NES") },
	{ "vc-snes",	gts("VC-SNES") },
	{ "vc-n64",		gts("VC-N64") },
	{ "vc-sms",		gts("VC-SMS") },
	{ "vc-md",      gts("VC-Sega Genesis") },
	{ "vc-pce",		gts("VC-TurboGrafx-16") },
	{ "vc-neogeo",	gts("VC-Neo Geo") },
	{ "vc-arcade",	gts("VC-Arcade") },
	{ "vc-c64",		gts("VC-Commodore 64") },
	{ "wiichannel",	gts("Wii Channel") },
};

char *genreTypes[genreCnt][2];
struct Sorts sortTypes[sortCnt];
char *searchFields[searchFieldCnt];
char *searchCompareTypes[searchCompareTypeCnt];

void build_arrays() {
	genreTypes[0][0] = "action";
	genreTypes[0][1] = (char *)gt("Action");
	genreTypes[1][0] = "adventure";
	genreTypes[1][1] = (char *)gt("Adventure");
	genreTypes[2][0] = "fighting";
	genreTypes[2][1] = (char *)gt("Fighting");
	genreTypes[3][0] = "music";
	genreTypes[3][1] = (char *)gt("Music");
	genreTypes[4][0] = "platformer";
	genreTypes[4][1] = (char *)gt("Platformer");
	genreTypes[5][0] = "puzzle";
	genreTypes[5][1] = (char *)gt("Puzzle");
	genreTypes[6][0] = "racing";
	genreTypes[6][1] = (char *)gt("Racing");
	genreTypes[7][0] = "role-playing";
	genreTypes[7][1] = (char *)gt("RPG");
	genreTypes[8][0] = "shooter";
	genreTypes[8][1] = (char *)gt("Shooter");
	genreTypes[9][0] = "simulation";
	genreTypes[9][1] = (char *)gt("Simulation");
	genreTypes[10][0] = "sports";
	genreTypes[10][1] = (char *)gt("Sports");
	genreTypes[11][0] = "strategy";
	genreTypes[11][1] = (char *)gt("Strategy");
	genreTypes[12][0] = "arcade";
	genreTypes[12][1] = (char *)gt("Arcade");
	genreTypes[13][0] = "baseball";
	genreTypes[13][1] = (char *)gt("Baseball");
	genreTypes[14][0] = "basketball";
	genreTypes[14][1] = (char *)gt("Basketball");
	genreTypes[15][0] = "bike racing";
	genreTypes[15][1] = (char *)gt("Bike Racing");
	genreTypes[16][0] = "billiards";
	genreTypes[16][1] = (char *)gt("Billiards");
	genreTypes[17][0] = "board game";
	genreTypes[17][1] = (char *)gt("Board Game");
	genreTypes[18][0] = "bowling";
	genreTypes[18][1] = (char *)gt("Bowling");
	genreTypes[19][0] = "boxing";
	genreTypes[19][1] = (char *)gt("Boxing");
	genreTypes[20][0] = "business simulation";
	genreTypes[20][1] = (char *)gt("Business Sim");
	genreTypes[21][0] = "cards";
	genreTypes[21][1] = (char *)gt("Cards");
	genreTypes[22][0] = "chess";
	genreTypes[22][1] = (char *)gt("Chess");
	genreTypes[23][0] = "coaching";
	genreTypes[23][1] = (char *)gt("Coaching");
	genreTypes[24][0] = "compilation";
	genreTypes[24][1] = (char *)gt("Compilation");
	genreTypes[25][0] = "construction simulation";
	genreTypes[25][1] = (char *)gt("Construction Sim");
	genreTypes[26][0] = "cooking";
	genreTypes[26][1] = (char *)gt("Cooking");
	genreTypes[27][0] = "cricket";
	genreTypes[27][1] = (char *)gt("Cricket");
	genreTypes[28][0] = "dance";
	genreTypes[28][1] = (char *)gt("Dance");
	genreTypes[29][0] = "darts";
	genreTypes[29][1] = (char *)gt("Darts");
	genreTypes[30][0] = "drawing";
	genreTypes[30][1] = (char *)gt("Drawing");
	genreTypes[31][0] = "educational";
	genreTypes[31][1] = (char *)gt("Educational");
	genreTypes[32][0] = "exercise";
	genreTypes[32][1] = (char *)gt("Exercise");
	genreTypes[33][0] = "first-person shooter";
	genreTypes[33][1] = (char *)gt("1st-Person Shooter");
	genreTypes[34][0] = "fishing";
	genreTypes[34][1] = (char *)gt("Fishing");
	genreTypes[35][0] = "fitness";
	genreTypes[35][1] = (char *)gt("Fitness");
	genreTypes[36][0] = "flight simulation";
	genreTypes[36][1] = (char *)gt("Flight Sim");
	genreTypes[37][0] = "football";
	genreTypes[37][1] = (char *)gt("Football");
	genreTypes[38][0] = "futuristic racing";
	genreTypes[38][1] = (char *)gt("Futuristic Racing");
	genreTypes[39][0] = "golf";
	genreTypes[39][1] = (char *)gt("Golf");
	genreTypes[40][0] = "health";
	genreTypes[40][1] = (char *)gt("Health");
	genreTypes[41][0] = "hidden object";
	genreTypes[41][1] = (char *)gt("Hidden Object");
	genreTypes[42][0] = "hockey";
	genreTypes[42][1] = (char *)gt("Hockey");
	genreTypes[43][0] = "hunting";
	genreTypes[43][1] = (char *)gt("Hunting");
	genreTypes[44][0] = "karaoke";
	genreTypes[44][1] = (char *)gt("Karaoke");
	genreTypes[45][0] = "kart racing";
	genreTypes[45][1] = (char *)gt("Kart Racing");
	genreTypes[46][0] = "life simulation";
	genreTypes[46][1] = (char *)gt("Life Simulation");
	genreTypes[47][0] = "management simulation";
	genreTypes[47][1] = (char *)gt("Management Sim");
	genreTypes[48][0] = "martial arts";
	genreTypes[48][1] = (char *)gt("Martial Arts");
	genreTypes[49][0] = "motorcycle racing";
	genreTypes[49][1] = (char *)gt("Motorcycle Racing");
	genreTypes[50][0] = "off-road racing";
	genreTypes[50][1] = (char *)gt("Off-Road Racing");
	genreTypes[51][0] = "party";
	genreTypes[51][1] = (char *)gt("Party");
	genreTypes[52][0] = "pétanque";
	genreTypes[52][1] = (char *)gt("Petanque");
	genreTypes[53][0] = "pinball";
	genreTypes[53][1] = (char *)gt("Pinball");
	genreTypes[54][0] = "poker";
	genreTypes[54][1] = (char *)gt("Poker");
	genreTypes[55][0] = "rail shooter";
	genreTypes[55][1] = (char *)gt("Rail Shooter");
	genreTypes[56][0] = "rhythm";
	genreTypes[56][1] = (char *)gt("Rhythm");
	genreTypes[57][0] = "rugby";
	genreTypes[57][1] = (char *)gt("Rugby");
	genreTypes[58][0] = "sim racing";
	genreTypes[58][1] = (char *)gt("Sim Racing");
	genreTypes[59][0] = "skateboarding";
	genreTypes[59][1] = (char *)gt("Skateboarding");
	genreTypes[60][0] = "ski";
	genreTypes[60][1] = (char *)gt("Skiing");
	genreTypes[61][0] = "snowboarding";
	genreTypes[61][1] = (char *)gt("Snowboarding");
	genreTypes[62][0] = "soccer";
	genreTypes[62][1] = (char *)gt("Soccer");
	genreTypes[63][0] = "stealth action";
	genreTypes[63][1] = (char *)gt("Stealth Action");
	genreTypes[64][0] = "surfing";
	genreTypes[64][1] = (char *)gt("Surfing");
	genreTypes[65][0] = "survival horror";
	genreTypes[65][1] = (char *)gt("Survival Horror");
	genreTypes[66][0] = "table tennis";
	genreTypes[66][1] = (char *)gt("Table Tennis");
	genreTypes[67][0] = "tennis";
	genreTypes[67][1] = (char *)gt("Tennis");
	genreTypes[68][0] = "third-person shooter";
	genreTypes[68][1] = (char *)gt("3rd-person Shooter");
	genreTypes[69][0] = "train simulation";
	genreTypes[69][1] = (char *)gt("Train Simulation");
	genreTypes[70][0] = "trivia";
	genreTypes[70][1] = (char *)gt("Trivia");
	genreTypes[71][0] = "truck racing";
	genreTypes[71][1] = (char *)gt("Truck Racing");
	genreTypes[72][0] = "virtual pet";
	genreTypes[72][1] = (char *)gt("Virtual Pet");
	genreTypes[73][0] = "volleyball";
	genreTypes[73][1] = (char *)gt("Volleyball");
	genreTypes[74][0] = "watercraft racing";
	genreTypes[74][1] = (char *)gt("Watercraft Racing");
	genreTypes[75][0] = "wrestling";
	genreTypes[75][1] = (char *)gt("Wrestling");



	strcpy(sortTypes[0].cfg_val, "title");
	strcpy(sortTypes[0].name, gt("Title"));
	sortTypes[0].sortAsc = __sort_title_asc;
	sortTypes[0].sortDsc = __sort_title_desc;
	strcpy(sortTypes[1].cfg_val, "players");
	strcpy(sortTypes[1].name, gt("Number of Players"));
	sortTypes[1].sortAsc = __sort_players_asc;
	sortTypes[1].sortDsc = __sort_players_desc;
	strcpy(sortTypes[2].cfg_val, "online_players");
	strcpy(sortTypes[2].name, gt("Number of Online Players"));
	sortTypes[2].sortAsc = __sort_wifiplayers_asc;
	sortTypes[2].sortDsc = __sort_wifiplayers_desc;
	strcpy(sortTypes[3].cfg_val, "publisher");
	strcpy(sortTypes[3].name, gt("Publisher"));
	sortTypes[3].sortAsc = __sort_pub_asc;
	sortTypes[3].sortDsc = __sort_pub_desc;
	strcpy(sortTypes[4].cfg_val, "developer");
	strcpy(sortTypes[4].name, gt("Developer"));
	sortTypes[4].sortAsc = __sort_dev_asc;
	sortTypes[4].sortDsc = __sort_dev_desc;
	strcpy(sortTypes[5].cfg_val, "release");
	strcpy(sortTypes[5].name, gt("Release Date"));
	sortTypes[5].sortAsc = __sort_releasedate_asc;
	sortTypes[5].sortDsc = __sort_releasedate_desc;
	strcpy(sortTypes[6].cfg_val, "play_count");
	strcpy(sortTypes[6].name, gt("Play Count"));
	sortTypes[6].sortAsc = __sort_play_count_asc;
	sortTypes[6].sortDsc = __sort_play_count_desc;
	strcpy(sortTypes[7].cfg_val, "play_date");
	strcpy(sortTypes[7].name, gt("Last Play Date"));
	sortTypes[7].sortAsc = __sort_play_date_asc;
	sortTypes[7].sortDsc = __sort_play_date_desc;
	strcpy(sortTypes[8].cfg_val, "install");
	strcpy(sortTypes[8].name, gt("Install Date"));
	sortTypes[8].sortAsc = __sort_install_date_asc;
	sortTypes[8].sortDsc = __sort_install_date_desc;
	strcpy(sortTypes[9].cfg_val, "game_id");
	strcpy(sortTypes[9].name, gt("Game ID"));
	sortTypes[9].sortAsc = __sort_id_asc;
	sortTypes[9].sortDsc = __sort_id_desc;

	searchFields[0]  = (char *)gt("Title");
	searchFields[1]  = (char *)gt("Synopsis");
	searchFields[2]  = (char *)gt("Developer");
	searchFields[3]  = (char *)gt("Publisher");
	searchFields[4]  = (char *)gt("Game ID");
	searchFields[5]  = (char *)gt("Region");
	searchFields[6]  = (char *)gt("Rating");
	searchFields[7]  = (char *)gt("Players");
	searchFields[8]  = (char *)gt("Online Players");
	searchFields[9]  = (char *)gt("Play Count");
	searchFields[10] = (char *)gt("Synopsis Length");
	searchFields[11] = (char *)gt("Covers Available");

	searchCompareTypes[0] = (char *)gt("Contains");
	searchCompareTypes[1] = (char *)"<";
	searchCompareTypes[2] = (char *)"<=";
	searchCompareTypes[3] = (char *)"=";
	searchCompareTypes[4] = (char *)">=";
	searchCompareTypes[5] = (char *)">";

}

int get_accesory_id(char *accessory)
{
	int i;
	for (i=0; i<accessoryCnt; i++) {
		if (strcmp(accessory, accessoryTypes[i][0]) == 0) return i;
	}
	return -1;
}

const char* get_accesory_name(int i)
{
	if (i<accessoryCnt) {
		return gt(accessoryTypes[i][1]);
	}
	return NULL;
}

int get_feature_id(char *feature)
{
	int i;
	for (i=0; i<featureCnt; i++) {
		if (strcmp(feature, featureTypes[i][0]) == 0) return i;
	}
	return -1;
}

void reset_sort_default()
{
	sort_index = default_sort_index;
	sort_desc = default_sort_desc;
	filter_type = default_filter_type;
}

void __set_default_sort()
{

	build_arrays();
	char tmp[20];
	bool dsc = 0;
	int n = 0;
	strncpy(tmp, CFG.sort, sizeof(tmp));
	char * ptr = strchr(tmp, '-');
	if (ptr) {
		*ptr = '\0';
		dsc = (*(ptr+1) == 'd' || *(ptr+1) == 'D');
	}
	for (; n<sortCnt; n++) {
		if (!strncmp(sortTypes[n].cfg_val, tmp, strlen(tmp))) {
			sort_index = default_sort_index = n;
			if (!dsc) {
				default_sort_function = (*sortTypes[sort_index].sortAsc);
				sort_desc = default_sort_desc = 0;
			} else {
				default_sort_function = (*sortTypes[sort_index].sortDsc);
				sort_desc = default_sort_desc = 1;
			}
			return;
		}
	}
	sort_index = default_sort_index = 0;
	default_sort_function =  (*sortTypes[sort_index].sortAsc);
}

int filter_features(struct discHdr *list, int cnt, char *feature, bool requiredOnly)
{
	int i;
	int kept_cnt = 0;
	for (i=0; i<cnt; i++) {
		if (hasFeature(feature, list[i].id)) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	return cnt;
}

int filter_online(struct discHdr *list, int cnt, char * name, bool notused)
{
	int i;
	int kept_cnt = 0;
	for (i=0; i<cnt; i++) {
		gameXMLinfo *g = get_game_info_id(list[i].id);
		if (g && g->wifiplayers >= 1) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	return cnt;
}

int filter_controller(struct discHdr *list, int cnt, char *controller, bool requiredOnly)
{
	int i;
	int kept_cnt = 0;
	for (i=0; i<cnt; i++) {
		if (getControllerTypes(controller, list[i].id) >= (requiredOnly ? 1 : 0)) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	return cnt;
}

int filter_genre(struct discHdr *list, int cnt, char *genre, bool notused)
{
	int i;
	int kept_cnt = 0;
	for (i=0; i<cnt; i++) {
		if (hasGenre(genre, list[i].id)) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	return cnt;
}

int filter_gamecube(struct discHdr *list, int cnt, char *ignore, bool notused)
{
	int i;
	int kept_cnt = 0;
	for (i=0; i<cnt; i++) 
	{
		if (list[i].magic == GC_GAME_ON_DRIVE) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	return cnt;
}

int filter_wii(struct discHdr *list, int cnt, char *ignore, bool notused)
{
	int i;
	int kept_cnt = 0;
	for (i=0; i<cnt; i++) 
	{
		if (list[i].magic == WII_MAGIC) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	return cnt;
}

int filter_channel(struct discHdr *list, int cnt, char *ignore, bool notused)
{
	int i;
	int kept_cnt = 0;
	for (i=0; i<cnt; i++) 
	{
		if (list[i].magic == CHANNEL_MAGIC) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	return cnt;
}

int filter_game_type(struct discHdr *list, int cnt, char *typeWanted, bool notused)
{
	int i;
	int kept_cnt = 0;
	bool isGameType;
	
	for (i=0; i<cnt; i++) {
		switch ((int) typeWanted) {
			case GAME_TYPE_Wii:
				isGameType = (list[i].magic == WII_MAGIC);
				break;
			case GAME_TYPE_GameCube:
				isGameType = (list[i].magic == GC_GAME_ON_DRIVE);
				break;
			case GAME_TYPE_All_Channels:
				isGameType = (list[i].magic == CHANNEL_MAGIC);
				break;
			case GAME_TYPE_Wiiware:
				isGameType = (list[i].magic == CHANNEL_MAGIC) && (memcmp("W",list[i].id,1) == 0);
				break;
			case GAME_TYPE_VC_NES:
				isGameType = (list[i].magic == CHANNEL_MAGIC) && (memcmp("F",list[i].id,1) == 0);
				break;
			case GAME_TYPE_VC_SNES:
				isGameType = (list[i].magic == CHANNEL_MAGIC) && (memcmp("J",list[i].id,1) == 0);
				break;
			case GAME_TYPE_VC_N64:
				isGameType = (list[i].magic == CHANNEL_MAGIC) && (memcmp("N",list[i].id,1) == 0)
							&& (memcmp("NEOG",list[i].id,4) != 0);	//backup disk channel
				break;
			case GAME_TYPE_VC_SMS:
				isGameType = (list[i].magic == CHANNEL_MAGIC) && (memcmp("L",list[i].id,1) == 0);
				break;
			case GAME_TYPE_VC_MD:
				isGameType = (list[i].magic == CHANNEL_MAGIC) && (memcmp("M",list[i].id,1) == 0)
							&& (memcmp("MAUI",list[i].id,4) != 0);		//backup homebrew channel
				break;
			case GAME_TYPE_VC_PCE:
				isGameType = (list[i].magic == CHANNEL_MAGIC) && ((memcmp("P",list[i].id,1) == 0) || (memcmp("Q",list[i].id,1) == 0));
				break;
			case GAME_TYPE_VC_NeoGeo:
				isGameType = (list[i].magic == CHANNEL_MAGIC) && (memcmp("E",list[i].id,1) == 0) && (memcmp("EA",list[i].id,2) <= 0);
				break;
			case GAME_TYPE_VC_Arcade:
				isGameType = (list[i].magic == CHANNEL_MAGIC) && (memcmp("E",list[i].id,1) == 0) && (memcmp("EA",list[i].id,2) > 0);
				break;
			case GAME_TYPE_VC_C64:
				isGameType = (list[i].magic == CHANNEL_MAGIC) && (memcmp("C",list[i].id,1) == 0);
				break;
			case GAME_TYPE_Wii_Channel:
				isGameType = (list[i].magic == CHANNEL_MAGIC) && ((memcmp("H",list[i].id,1) == 0)
							|| (memcmp("UCXF",list[i].id,4) == 0)		//cfg loader
							|| (memcmp("MAUI",list[i].id,4) == 0)		//backup homebrew channel
							|| (memcmp("NEOG",list[i].id,4) == 0));	//backup disk channel
				break;
			default:
				isGameType = false;
	}
		
		if (isGameType) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	return cnt;
}

int filter_play_count(struct discHdr *list, int cnt, char *ignore, bool notused)
{
	int i;
	int kept_cnt = 0;
	for (i=0; i<cnt; i++) {
		if (getPlayCount(list[i].id) <= 0) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	return cnt;
}

int filter_duplicate_id3(struct discHdr *list, int cnt, char *ignore, bool notused)
{
	int i, j;
	int kept_cnt = 0;
	bool duplicate;
	for (i=0; i<cnt; i++) {
		duplicate = false;
		for (j=0; j<cnt; ) {
			if (i != j) 
				if (strncmp((char*)list[i].id, (char*)list[j].id, 3) == 0) {
					duplicate = true;
					break;
					}
			j++;
			}
		if (duplicate) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	return cnt;
}


int filter_search(struct discHdr *list, int cnt, char *search_field, bool notused)
{
	int i;
	char simple_title[XML_MAX_SYNOPSIS + 1];
	char *full_title;
	int full_pos;
	int simple_pos;
	int kept_cnt = 0;
	struct gameXMLinfo *g;
	char temp_str[200];
	int	search_int;
	int rec_int;
	bool keep_record;
	
	if (cur_search_compare_type > 0)	//numeric compare
		search_int = atoi(search_str);
	if ((int)search_field == 6)			//rating
		search_int = ConvertRatingToAge(search_str, "");
	
	for (i=0; i<cnt; i++) {
		switch ((int)search_field) {
			case 0:		//title
				full_title = get_title(&list[i]);
				break;
			case 1:		//synopsis
				g = get_game_info_id(list[i].id);
				if (g)
					full_title = g->synopsis;
				if (!g || !full_title) {
					continue;
				}
				break;
			case 2:		//developer
				g = get_game_info_id(list[i].id);
				if (g)
					full_title = g->developer;
				else {
					continue;
				}
				break;
			case 3:		//publisher
				g = get_game_info_id(list[i].id);
				if (g)
					full_title = g->publisher;
				else {
					continue;
				}
				break;
			case 4:		//ID6
				sprintf(temp_str, "%s", list[i].id);
				full_title = temp_str;
				break;
			case 5:		//region
				temp_str[0] = list[i].id[3];
				temp_str[1] = 0;
				full_title = temp_str;
				break;
			case 6:		//rating
				g = get_game_info_id(list[i].id);
				if (g)
					rec_int = ConvertRatingToAge(g->ratingvalue, g->ratingtype);
				else {
					continue;
				}
				goto numeric_compare;
				break;
			case 7:		//Number of Players
				g = get_game_info_id(list[i].id);
				if (g)
					rec_int = g->players;
				else {
					rec_int = 0;
				}
				goto numeric_compare;
				break;
			case 8:		//Number of Online Players
				g = get_game_info_id(list[i].id);
				if (g)
					rec_int = g->wifiplayers;
				else {
					rec_int = 0;
				}
				goto numeric_compare;
				break;
			case 9:		//Play Count
				rec_int = getPlayCount(list[i].id);
				goto numeric_compare;
				break;
			case 10:		//test synopsis len
				g = get_game_info_id(list[i].id);
				if (g)
					full_title = g->synopsis;
				if (!g || !full_title) {
					rec_int = 0;
				}
				else
					rec_int = strlen(g->synopsis);
				goto numeric_compare;
				break;
			case 11:		//Covers available
				if (find_cover_path(list[i].id, CFG_COVER_STYLE_2D, temp_str, sizeof(temp_str), NULL) == 0) {
					rec_int = 1;
					if ((cur_search_compare_type == 3) && (search_int == 0))	//most common search for missing covers 4x faster
						goto numeric_compare;
					if (find_cover_path(list[i].id, CFG_COVER_STYLE_3D, temp_str, sizeof(temp_str), NULL) == 0) {
						rec_int = 2;
						if (find_cover_path(list[i].id, CFG_COVER_STYLE_FULL, temp_str, sizeof(temp_str), NULL) == 0) {
							rec_int = 3;
							if (find_cover_path(list[i].id, CFG_COVER_STYLE_DISC, temp_str, sizeof(temp_str), NULL) == 0) {
								rec_int = 4;
							}
						}
					}
				}
				else
					rec_int = 0;
				goto numeric_compare;
				break;
			default:		//should never happen
				continue;
		}
		simple_pos = 0;
		for (full_pos = 0; full_pos < XML_MAX_SYNOPSIS - 1; full_pos++){
			if (((full_title[full_pos] >= '0') && (full_title[full_pos] <= '9'))
			 || ((full_title[full_pos] >= 'A') && (full_title[full_pos] <= 'Z'))
			 || ((full_title[full_pos] >= 'a') && (full_title[full_pos] <= 'z'))) {
				simple_title[simple_pos] = full_title[full_pos];
				simple_pos++;
				continue;
			}
			if ((full_title[full_pos] == ' ')		//space
			 || (full_title[full_pos] == 0x0D)		//carrage return
			 || (full_title[full_pos] == 0x0A)		//line feed
			 || (full_title[full_pos] == 0x09)) {	//tab
				if (simple_title[simple_pos - 1] != ' ') {
					simple_title[simple_pos] = full_title[full_pos];
					simple_pos++;
				}
				continue;
			}
			if (full_title[full_pos] == 0x00) {		// null
				break;
			}
			if (full_title[full_pos] == '&') {		//possable xml esc seaquences
				if ((full_title[full_pos+1] == 'g')
				 && (full_title[full_pos+2] == 't')
				 && (full_title[full_pos+3] == ';')) {	//&gt;
					full_pos = full_pos + 3;
					continue;
				}
				if ((full_title[full_pos+1] == 'l')
				 && (full_title[full_pos+2] == 't')
				 && (full_title[full_pos+3] == ';')) {	//&lt;
					full_pos = full_pos + 3;
					continue;
				}
				if ((full_title[full_pos+1] == 'q')
				 && (full_title[full_pos+2] == 'u')
				 && (full_title[full_pos+3] == 'o')
				 && (full_title[full_pos+4] == 't')
				 && (full_title[full_pos+5] == ';')) {	//&quot;
					full_pos = full_pos + 5;
					continue;
				}
				if ((full_title[full_pos+1] == 'a')
				 && (full_title[full_pos+2] == 'p')
				 && (full_title[full_pos+3] == 'o')
				 && (full_title[full_pos+4] == 's')
				 && (full_title[full_pos+5] == ';')) {	//&apos;
					full_pos = full_pos + 5;
					continue;
				}
				if ((full_title[full_pos+1] == 'a')
				 && (full_title[full_pos+2] == 'm')
				 && (full_title[full_pos+3] == 'p')
				 && (full_title[full_pos+4] == ';')) {	//&amp;
					full_pos = full_pos + 4;
					continue;
				}
			}
			if (full_title[full_pos] == 0xC5) {
				if (full_title[full_pos+1] == 0x8C) {	//Okami
					simple_title[simple_pos] = 'O';
					simple_pos++;
					full_pos++;
					continue;
				}
				if ((full_title[full_pos+1] == 0x8D)
				  ||(full_title[full_pos+1] == 0x91)) {
					simple_title[simple_pos] = 'o';
					simple_pos++;
					full_pos++;
					continue;
				}
				if (full_title[full_pos+1] == 0x92) {
					simple_title[simple_pos] = 'O';
					simple_pos++;
					simple_title[simple_pos] = 'E';
					simple_pos++;
					full_pos++;
					continue;
				}
				if (full_title[full_pos+1] == 0x93) {
					simple_title[simple_pos] = 'o';
					simple_pos++;
					simple_title[simple_pos] = 'e';
					simple_pos++;
					full_pos++;
					continue;
				}
			}
			if (full_title[full_pos] == 0xC3) {
				if ((full_title[full_pos+1] >= 0x80) && (full_title[full_pos+1] <= 0x85)) {
					simple_title[simple_pos] = 'A';
					simple_pos++;
					full_pos++;
					continue;
				}
				if (full_title[full_pos+1] == 0x86) {
					simple_title[simple_pos] = 'A';
					simple_pos++;
					simple_title[simple_pos] = 'E';
					simple_pos++;
					full_pos++;
					continue;
				}
				if (full_title[full_pos+1] == 0x87) {
					simple_title[simple_pos] = 'C';
					simple_pos++;
					full_pos++;
					continue;
				}
				if ((full_title[full_pos+1] >= 0x88) && (full_title[full_pos+1] <= 0x8B)) {
					simple_title[simple_pos] = 'E';
					simple_pos++;
					full_pos++;
					continue;
				}
				if ((full_title[full_pos+1] >= 0x8C) && (full_title[full_pos+1] <= 0x8F)) {
					simple_title[simple_pos] = 'I';
					simple_pos++;
					full_pos++;
					continue;
				}
				if (full_title[full_pos+1] == 0x91) {
					simple_title[simple_pos] = 'N';
					simple_pos++;
					full_pos++;
					continue;
				}
				if (((full_title[full_pos+1] >= 0x92) && (full_title[full_pos+1] <= 0x96))
				  || (full_title[full_pos+1] == 0x98)) {
					simple_title[simple_pos] = 'O';
					simple_pos++;
					full_pos++;
					continue;
				}
				if ((full_title[full_pos+1] >= 0x99) && (full_title[full_pos+1] <= 0x9C)) {
					simple_title[simple_pos] = 'U';
					simple_pos++;
					full_pos++;
					continue;
				}
				if (full_title[full_pos+1] == 0x9D) {
					simple_title[simple_pos] = 'Y';
					simple_pos++;
					full_pos++;
					continue;
				}
				if ((full_title[full_pos+1] >= 0xA0) && (full_title[full_pos+1] <= 0xA5)) {
					simple_title[simple_pos] = 'a';
					simple_pos++;
					full_pos++;
					continue;
				}
				if (full_title[full_pos+1] == 0xA6) {
					simple_title[simple_pos] = 'a';
					simple_pos++;
					simple_title[simple_pos] = 'e';
					simple_pos++;
					full_pos++;
					continue;
				}
				if (full_title[full_pos+1] == 0xA7) {
					simple_title[simple_pos] = 'c';
					simple_pos++;
					full_pos++;
					continue;
				}
				if ((full_title[full_pos+1] >= 0xA8) && (full_title[full_pos+1] <= 0xAB)) {	//pokemon
					simple_title[simple_pos] = 'e';
					simple_pos++;
					full_pos++;
					continue;
				}
				if ((full_title[full_pos+1] >= 0xAC) && (full_title[full_pos+1] <= 0xAF)) {
					simple_title[simple_pos] = 'i';
					simple_pos++;
					full_pos++;
					continue;
				}
				if (full_title[full_pos+1] == 0xB1) {
					simple_title[simple_pos] = 'n';
					simple_pos++;
					full_pos++;
					continue;
				}
				if (((full_title[full_pos+1] >= 0xB2) && (full_title[full_pos+1] <= 0xB6))
				  || (full_title[full_pos+1] == 0xB8)) {
					simple_title[simple_pos] = 'o';
					simple_pos++;
					full_pos++;
					continue;
				}
				if ((full_title[full_pos+1] >= 0xB9) && (full_title[full_pos+1] <= 0xBC)) {
					simple_title[simple_pos] = 'u';
					simple_pos++;
					full_pos++;
					continue;
				}
				if ((full_title[full_pos+1] >= 0xBD) || (full_title[full_pos+1] <= 0xBF)) {
					simple_title[simple_pos] = 'y';
					simple_pos++;
					full_pos++;
					continue;
				}
			}
		}
		simple_title[simple_pos] = 0;	//null terminate it

//		if (!strcasestr(get_title(&list[i]), search_str)) {
		if (strcasestr(simple_title, search_str)) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
		continue;

numeric_compare:
		switch (cur_search_compare_type) {
			case 0:		//contains
				keep_record = strcasestr(simple_title, search_str);
				break;
			case 1:		//<
				keep_record = (rec_int < search_int);
				break;
			case 2:		//<=
				keep_record = (rec_int <= search_int);
				break;
			case 3:		//=
				keep_record = (rec_int == search_int);
				break;
			case 4:		//>=
				keep_record = (rec_int >= search_int);
				break;
			case 5:		//>
				keep_record = (rec_int > search_int);
				break;
			default:
				keep_record = false;
		}
		
		if (keep_record) {
			if (kept_cnt != i)
				list[kept_cnt] = list[i];
			kept_cnt++;
		}
	}
	cnt = kept_cnt;
	return cnt;
}

int filter_games(int (*filter) (struct discHdr *, int, char *, bool), char * name, bool num)
{
	int i, len;
	int ret = 0;
	u8 *id = NULL;

	// remember the selected game before we change the list
	if (gameSelected >= 0 && gameSelected < gameCnt) {
		id = gameList[gameSelected].id;
	}
	// filter
	if (filter_gameList) {
		len = sizeof(struct discHdr) * all_gameCnt;
		if (enable_favorite){
			memcpy(filter_gameList, fav_gameList, len);
			filter_gameCnt = filter(filter_gameList, fav_gameCnt, name, num);
		}
		else {
			memcpy(filter_gameList, all_gameList, len);
			filter_gameCnt = filter(filter_gameList, all_gameCnt, name, num);
		}
	}
	//if (filter_gameCnt > 0) {
		gameList = filter_gameList;
 		gameCnt = filter_gameCnt;
		ret = 1;
	/*} else {
		// this won't work because gameList might
		// already point to filter_gameCnt
		Con_Clear();
		FgColor(CFG.color_header);
		printf("\n");
		printf(" %s ", CFG.cursor);
		printf("%s", gt("No games found!"));
		printf("\n");
		printf(gt("Loading previous game list..."));
		printf("\n");
		DefaultColor();
		__console_flush(0);
		sleep(1);
		ret = -1;
	}
	*/
	gameStart = 0;
	gameSelected = 0;
	if (id) for (i=0; i<gameCnt; i++) {
		if (strncmp((char*)gameList[i].id, (char*)id, 6) == 0) {
			gameSelected = i;
			break;
		}
	}
	// scroll start list
	__Menu_ScrollStartList();
	return ret;
}

void showAllGames(void)
{
	int i;
	u8 *id = NULL;
	if (gameSelected >= 0 && gameSelected < gameCnt) {
		id = gameList[gameSelected].id;
	}
	if (enable_favorite){
		gameList = fav_gameList;
		gameCnt = fav_gameCnt;
	}
	else {
		gameList = all_gameList;
		gameCnt = all_gameCnt;
	}
	// find game selected
	gameStart = 0;
	gameSelected = 0;
	if (id) for (i=0; i<gameCnt; i++) {
		if (strncmp((char*)gameList[i].id, (char*)id, 6) == 0) {
			gameSelected = i;
			break;
		}
	}
	// scroll start list
	__Menu_ScrollStartList();
}

int filter_games_set(int type, int index)
{
	int ret = -1;
	switch (type) {
		case FILTER_ALL:
			showAllGames();
			ret = 0;
			index = -1;
			break;
		case FILTER_ONLINE:
			ret = filter_games(filter_online, "", 0);
			index = -1;
			break;
		case FILTER_UNPLAYED:
			ret = filter_games(filter_play_count, "", 0);
			index = -1;
			break;
		case FILTER_GENRE:
			ret = filter_games(filter_genre, genreTypes[index][0], 0);
			break;
		case FILTER_FEATURES:
			ret = filter_games(filter_features, featureTypes[index][0], 0);
			break;
		case FILTER_CONTROLLER:
			ret = filter_games(filter_controller, accessoryTypes[index][0], 0);
			break;
		case FILTER_GAMECUBE:
			ret = filter_games(filter_gamecube, "", 0);
			break;
		case FILTER_WII:
			ret = filter_games(filter_wii, "", 0);
			break;
		case FILTER_CHANNEL:
			ret = filter_games(filter_channel, "", 0);
			break;
		case FILTER_DUPLICATE_ID3:
			ret = filter_games(filter_duplicate_id3, "", 0);
			break;
		case FILTER_GAME_TYPE:
			ret = filter_games(filter_game_type, (char*)index, 0);
			break;
		case FILTER_SEARCH:
			ret = filter_games(filter_search, (char*)index, 0);
			break;
	}
	if (ret > -1) {
		filter_type = type;
		filter_index = index;
	}
	return ret;
}

bool is_filter(int type, int index)
{
	if (type != filter_type) return false;
	switch (type) {
		case FILTER_GENRE:
		case FILTER_CONTROLLER:
		case FILTER_FEATURES:
		case FILTER_GAME_TYPE:
		case FILTER_SEARCH:
			return index == filter_index;
	}
	return true;
}

char* mark_filter(int type, int index)
{
	return is_filter(type, index) ? "*" : " ";
}

s32 __sort_play_date(struct discHdr * hdr1, struct discHdr * hdr2, bool desc)
{
	int ret;
	if (desc)
		ret = (getLastPlay(hdr2->id) - getLastPlay(hdr1->id));
	else
		ret = (getLastPlay(hdr1->id) - getLastPlay(hdr2->id));
	if (ret == 0) ret = __sort_title(hdr1, hdr2, desc);
	return ret;
}

s32 __sort_install_date(struct discHdr * hdr1, struct discHdr * hdr2, bool desc)
{
	int ret = 0;
	void *p1, *p2;
	time_t t1 = 0;
	time_t t2 = 0;
	p1 = hmap_get(&install_time, hdr1->id);
	p2 = hmap_get(&install_time, hdr2->id);
	if (p1) memcpy(&t1, p1, sizeof(time_t));
	if (p2) memcpy(&t2, p2, sizeof(time_t));
	ret = t1 - t2;
	if (desc) ret = -ret;
	if (ret == 0) ret = __sort_title(hdr1, hdr2, desc);
	return ret;
}

s32 __sort_title(struct discHdr * hdr1, struct discHdr * hdr2, bool desc)
{
	char *title1 = get_title(hdr1);
	char *title2 = get_title(hdr2);
	title1 = skip_sort_ignore(title1);
	title2 = skip_sort_ignore(title2);
	if (desc)
		return mbs_coll(title2, title1);
	else
		return mbs_coll(title1, title2);
}

s32 __sort_releasedate(struct discHdr * hdr1, struct discHdr * hdr2, bool desc)
{
	int ret = 0;
	gameXMLinfo *g1 = get_game_info_id(hdr1->id);
	gameXMLinfo *g2 = get_game_info_id(hdr2->id);
	if (g1 && g2) {
		int date1 = (g1->year * 10000) + (g1->month * 100) + (g1->day);
		int date2 = (g2->year * 10000) + (g2->month * 100) + (g2->day);
		ret = (date1 - date2);
		if (desc) ret = -ret;
	}
	if (ret == 0) ret = __sort_title(hdr1, hdr2, desc);
	return ret;
}

s32 __sort_players(struct discHdr * hdr1, struct discHdr * hdr2, bool desc)
{
	int ret = 0;
	gameXMLinfo *g1 = get_game_info_id(hdr1->id);
	gameXMLinfo *g2 = get_game_info_id(hdr2->id);
	if (g1 && g2) {
		ret = (g1->players - g2->players);
		if (desc) ret = -ret;
	}
	if (ret == 0) ret = __sort_title(hdr1, hdr2, desc);
	return ret;
}

s32 __sort_wifiplayers(struct discHdr * hdr1, struct discHdr * hdr2, bool desc)
{
	int ret = 0;
	gameXMLinfo *g1 = get_game_info_id(hdr1->id);
	gameXMLinfo *g2 = get_game_info_id(hdr2->id);
	if (g1 && g2) {
		ret = (g1->wifiplayers - g2->wifiplayers);
		if (desc) ret = -ret;
	}
	if (ret == 0) ret = __sort_title(hdr1, hdr2, desc);
	return ret;
}

s32 __sort_pub(struct discHdr * hdr1, struct discHdr * hdr2, bool desc)
{
	int ret = 0;
	gameXMLinfo *g1 = get_game_info_id(hdr1->id);
	gameXMLinfo *g2 = get_game_info_id(hdr2->id);
	if (g1 && g2) {
		ret = mbs_coll(g1->publisher, g2->publisher);
		if (desc) ret = -ret;
	}
	if (ret == 0) ret = __sort_title(hdr1, hdr2, desc);
	return ret;
}

s32 __sort_dev(struct discHdr * hdr1, struct discHdr * hdr2, bool desc)
{
	int ret = 0;
	gameXMLinfo *g1 = get_game_info_id(hdr1->id);
	gameXMLinfo *g2 = get_game_info_id(hdr2->id);
	if (g1 && g2) {
		ret = mbs_coll(g1->developer, g2->developer);
		if (desc) ret = -ret;
	}
	if (ret == 0) ret = __sort_title(hdr1, hdr2, desc);
	return ret;
}

s32 __sort_play_count(struct discHdr * hdr1, struct discHdr * hdr2, bool desc)
{
	int ret;
	if (desc)
		ret = (getPlayCount(hdr2->id) - getPlayCount(hdr1->id));
	else
		ret = (getPlayCount(hdr1->id) - getPlayCount(hdr2->id));
	if (ret == 0) ret = __sort_title(hdr1, hdr2, desc);
	return ret;
}

s32 __sort_id(struct discHdr * hdr1, struct discHdr * hdr2, bool desc)
{
	if (desc)
		return strncmp((char*)hdr2->id, (char*)hdr1->id, 6);
	else
		return strncmp((char*)hdr1->id, (char*)hdr2->id, 6);
}

s32 __sort_players_asc(const void *a, const void *b)
{
	return __sort_players((struct discHdr *)a, (struct discHdr *)b, 0);
}

s32 __sort_players_desc(const void *a, const void *b)
{
	return __sort_players((struct discHdr *)a, (struct discHdr *)b, 1);
}

s32 __sort_wifiplayers_asc(const void *a, const void *b)
{
	return __sort_wifiplayers((struct discHdr *)a, (struct discHdr *)b, 0);
}

s32 __sort_wifiplayers_desc(const void *a, const void *b)
{
	return __sort_wifiplayers((struct discHdr *)a, (struct discHdr *)b, 1);
}

s32 __sort_pub_asc(const void *a, const void *b)
{
	return __sort_pub((struct discHdr *)a, (struct discHdr *)b, 0);
}

s32 __sort_pub_desc(const void *a, const void *b)
{
	return __sort_pub((struct discHdr *)a, (struct discHdr *)b, 1);
}

s32 __sort_dev_asc(const void *a, const void *b)
{
	return __sort_dev((struct discHdr *)a, (struct discHdr *)b, 0);
}

s32 __sort_dev_desc(const void *a, const void *b)
{
	return __sort_dev((struct discHdr *)a, (struct discHdr *)b, 1);
}

s32 __sort_releasedate_asc(const void *a, const void *b)
{
	return __sort_releasedate((struct discHdr *)a, (struct discHdr *)b, 0);
}

s32 __sort_releasedate_desc(const void *a, const void *b)
{
	return __sort_releasedate((struct discHdr *)a, (struct discHdr *)b, 1);
}

s32 __sort_play_count_asc(const void *a, const void *b)
{
	return __sort_play_count((struct discHdr *)a, (struct discHdr *)b, 0);
}

s32 __sort_play_count_desc(const void *a, const void *b)
{
	return __sort_play_count((struct discHdr *)a, (struct discHdr *)b, 1);
}

s32 __sort_title_asc(const void *a, const void *b)
{
	return __sort_title((struct discHdr *)a, (struct discHdr *)b, 0);
}

s32 __sort_title_desc(const void *a, const void *b)
{
	return __sort_title((struct discHdr *)a, (struct discHdr *)b, 1);
}

s32 __sort_install_date_asc(const void *a, const void *b)
{
	return __sort_install_date((struct discHdr *)a, (struct discHdr *)b, 0);
}

s32 __sort_install_date_desc(const void *a, const void *b)
{
	return __sort_install_date((struct discHdr *)a, (struct discHdr *)b, 1);
}

s32 __sort_play_date_asc(const void *a, const void *b)
{
	return __sort_play_date((struct discHdr *)a, (struct discHdr *)b, 0);
}

s32 __sort_play_date_desc(const void *a, const void *b)
{
	return __sort_play_date((struct discHdr *)a, (struct discHdr *)b, 1);
}

s32 __sort_id_asc(const void *a, const void *b)
{
	return __sort_id((struct discHdr *)a, (struct discHdr *)b, 0);
}

s32 __sort_id_desc(const void *a, const void *b)
{
	return __sort_id((struct discHdr *)a, (struct discHdr *)b, 1);
}

void sortList(int (*sortFunc) (const void *, const void *))
{
	int inst_time = 0;
	if (!all_gameList) {	//During init all-gamesList dosent exist yet fake it
		all_gameList = gameList;
		all_gameCnt  = gameCnt;
	}
	if (sortFunc == __sort_install_date_asc	|| sortFunc == __sort_install_date_desc) {
		inst_time = 1;
		// prepare install times
		char fname[1024];
		struct stat st;
		int i;
		int ret;
		time_t time;
		//dbg_time1();
		printf("\n\n");
		hmap_init(&install_time, 6, sizeof(time_t));
		for (i=0; i<all_gameCnt; i++) {
			printf_("... %3d%%\r", 100*(i+1)/all_gameCnt);
			__console_flush(1);
			*fname = 0;
			time = 0;
			if (all_gameList[i].magic == GC_GAME_ON_DRIVE) {
				sprintf(fname, "%s", all_gameList[i].path);	
				ret = 1;
			} else if (all_gameList[i].magic == CHANNEL_MAGIC) {
				sprintf(fname, "%s/title/00010001/%02x%02x%02x%02x/content/title.tmd", CFG.nand_emu_path, all_gameList[i].id[0], all_gameList[i].id[1], all_gameList[i].id[2], all_gameList[i].id[3]);
				ret = 1;
			} else {
				ret = WBFS_FAT_find_fname(all_gameList[i].id, fname, sizeof(fname));
			}
			if (ret > 0 && stat(fname, &st) == 0) {
				time = st.st_mtime;
			}
			hmap_add(&install_time, all_gameList[i].id, &time);
		}
		//dbg_time2("stat"); Menu_PrintWait();
	}

	qsort(all_gameList, all_gameCnt, sizeof(struct discHdr), sortFunc);	//sort the whole list or it gets lost when changing filters
	if (fav_gameList)	// if the list exists
		qsort(fav_gameList, fav_gameCnt, sizeof(struct discHdr), sortFunc);	//fav list needs to be sorted also
	if (inst_time) {
		hmap_close(&install_time);
	}

	if (filter_gameList)	// if the list exists
		filter_games_set(filter_type, filter_index);	//refilter the list with the new sort
	
	// scroll start list
	__Menu_ScrollStartList();
}

void sortList_default()
{
	sortList(default_sort_function);
}

void sortList_set(int index, bool desc)
{
	if (index < 0 || index > sortCnt) index = 0;
	sort_index = index;
	sort_desc = desc;
	if (sort_desc) {
		sortList(sortTypes[sort_index].sortDsc);
	} else {
		sortList(sortTypes[sort_index].sortAsc);
	}
}

int Menu_Filter()			//Filter by Genre
{
	struct discHdr *header = NULL;
	int redraw_cover = 0;
	struct Menu menu;
	int rows, cols, size;
	CON_GetMetrics(&cols, &rows);
	if ((size = rows-10) < 3) size = 3;		//scroll area -5 fixed and 5 overhead
	menu_init(&menu, genreCnt+5);			//Add 5 fixed menu items
	for (;;) {

		menu.line_count = 0;

		if (gameCnt) {
			header = &gameList[gameSelected];
		} else {
			header = NULL;
		}
		int n;
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Filter by Genre"));
		printf(":\n\n");
		MENU_MARK();
		printf("<%s>\n", gt("Filter by Controller"));
		MENU_MARK();
		printf("<%s>\n", gt("Filter by Online Features"));
		MENU_MARK();
		printf("<%s>\n", gt("Filter by Game Type"));
		MENU_MARK();
		printf("%s %s\n", mark_filter(FILTER_ALL,-1), gt("All Games"));
		MENU_MARK();
		printf("%s %s\n", mark_filter(FILTER_UNPLAYED,-1), gt("Unplayed Games"));
		menu_window_begin(&menu, size, genreCnt);
		for (n=0;n<genreCnt;n++) {
			if (menu_window_mark(&menu))
				printf("%s %s\n", mark_filter(FILTER_GENRE,n), genreTypes[n][1]);
		}
		DefaultColor();
		menu_window_end(&menu, cols);
		
		//printf("\n");
		printf_h(gt("Press %s to select filter type"), (button_names[CFG.button_confirm.num]));
		__console_flush(0);

		if (redraw_cover) {
			if (header) Gui_DrawCover(header->id);
			redraw_cover = 0;
		}
		
		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move(&menu, buttons);

		int change = 0;
		if (buttons & WPAD_BUTTON_LEFT) change = -1;
		if (buttons & WPAD_BUTTON_RIGHT || buttons & CFG.button_confirm.mask || buttons & CFG.button_save.mask) change = +1;

		if (change) {
			if (0 == menu.current) {
				Menu_Filter2();
				redraw_cover = 1;
				break;
			} else if (1 == menu.current) {
				Menu_Filter3();
				redraw_cover = 1;
				break;
			} else if (2 == menu.current) {
				Menu_Filter4();
				redraw_cover = 1;
				break;
			} else if (3 == menu.current) {
				filter_games_set(FILTER_ALL, -1);
				redraw_cover = 1;
			} else if (4 == menu.current) {
				redraw_cover = 1;
				filter_games_set(FILTER_UNPLAYED, -1);
			}
			n = menu.current - 5;
			if (n >= 0 && n < genreCnt) {
				redraw_cover = 1;
				filter_games_set(FILTER_GENRE, n);
			}
		}
		
		// HOME button
		if (buttons & CFG.button_exit.mask) {
			Handle_Home(0);
		}
		if (buttons & CFG.button_cancel.mask) break;
	}
	return 0;
}

int Menu_Filter2()				//Filter by Controler
{
	struct discHdr *header = NULL;
	int redraw_cover = 0;
	struct Menu menu;
	int rows, cols, size;
	CON_GetMetrics(&cols, &rows);
	if ((size = rows-9) < 3) size = 3;		//scroll area -4 fixed and 5 overhead
	menu_init(&menu, accessoryCnt+4);		//Add 4 fixed menu items
	for (;;) {

		menu.line_count = 0;

		if (gameCnt) {
			header = &gameList[gameSelected];
		} else {
			header = NULL;
		}
		int n;
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Filter by Controller"));
		printf(":\n\n");
		MENU_MARK();
		printf("<%s>\n", gt("Filter by Genre"));
		MENU_MARK();
		printf("<%s>\n", gt("Filter by Online Features"));
		MENU_MARK();
		printf("<%s>\n", gt("Filter by Game Type"));
		MENU_MARK();
		printf("%s %s\n", mark_filter(FILTER_ALL,-1), gt("All Games"));
		menu_window_begin(&menu, size, accessoryCnt);
		for (n=0; n<accessoryCnt; n++) {
			if (menu_window_mark(&menu)) {
				printf("%s ", mark_filter(FILTER_CONTROLLER,n));
				printf("%s\n", gt(accessoryTypes[n][1]));
			}
		}
		DefaultColor();
		menu_window_end(&menu, cols);
	
		printf_h(gt("Press %s to select filter type"), (button_names[CFG.button_confirm.num]));
		__console_flush(0);

		if (redraw_cover) {
			if (header) Gui_DrawCover(header->id);
			redraw_cover = 0;
		}
		
		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move(&menu, buttons);

		int change = 0;
		if (buttons & WPAD_BUTTON_LEFT) change = -1;
		if (buttons & WPAD_BUTTON_RIGHT || buttons & CFG.button_confirm.mask || buttons & CFG.button_save.mask) change = +1;

		if (change) {
			if (0 == menu.current) {
				Menu_Filter();
				redraw_cover = 1;
				goto end;
			} else if (1 == menu.current) {
				Menu_Filter3();
				redraw_cover = 1;
				goto end;
			} else if (2 == menu.current) {
				Menu_Filter4();
				redraw_cover = 1;
				goto end;
			} else if (3 == menu.current) {
				filter_games_set(FILTER_ALL, -1);
				redraw_cover = 1;
			}
			n = menu.current - 4;
			if (n >= 0 && n < accessoryCnt) {
				redraw_cover = 1;
				filter_games_set(FILTER_CONTROLLER, n);
			}
		}
		
		// HOME button
		if (buttons & CFG.button_exit.mask) {
			Handle_Home(0);
		}
		if (buttons & CFG.button_cancel.mask) break;
	}
	end:
	return 0;
}

int Menu_Filter3()			//Filter by Online Features
{
	struct discHdr *header = NULL;
	int redraw_cover = 0;
	struct Menu menu;
	int rows, cols, size;
	CON_GetMetrics(&cols, &rows);
	if ((size = rows-10) < 3) size = 3;		//scroll area -5 fixed and 5 overhead
	menu_init(&menu, featureCnt+5);			//Add 5 fixed menu items
	for (;;) {

		menu.line_count = 0;

		if (gameCnt) {
			header = &gameList[gameSelected];
		} else {
			header = NULL;
		}
		int n;
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Filter by Online Features"));
		printf(":\n\n");
		MENU_MARK();
		printf("<%s>\n", gt("Filter by Genre"));
		MENU_MARK();
		printf("<%s>\n", gt("Filter by Controller"));		
		MENU_MARK();
		printf("<%s>\n", gt("Filter by Game Type"));		
		MENU_MARK();
		printf("%s %s\n", mark_filter(FILTER_ALL,-1), gt("All Games"));
		MENU_MARK();
		printf("%s %s\n", mark_filter(FILTER_ONLINE,-1), gt("Online Play"));
		menu_window_begin(&menu, size, featureCnt);
		for (n=0;n<featureCnt;n++) {
			if (menu_window_mark(&menu))
			printf("%s ", mark_filter(FILTER_FEATURES,n));
			printf("%s\n", gt(featureTypes[n][1]));
		}
		DefaultColor();
		menu_window_end(&menu, cols);
		
		printf_h(gt("Press %s to select filter type"), (button_names[CFG.button_confirm.num]));
		__console_flush(0);

		if (redraw_cover) {
			if (header) Gui_DrawCover(header->id);
			redraw_cover = 0;
		}
		
		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move(&menu, buttons);

		int change = 0;
		if (buttons & WPAD_BUTTON_LEFT) change = -1;
		if (buttons & WPAD_BUTTON_RIGHT || buttons & CFG.button_confirm.mask || buttons & CFG.button_save.mask) change = +1;

		if (change) {
			if (0 == menu.current) {
				Menu_Filter();
				redraw_cover = 1;
				goto end;
			} else if (1 == menu.current) {
				Menu_Filter2();
				redraw_cover = 1;
				goto end;
			} else if (2 == menu.current) {
				Menu_Filter4();
				redraw_cover = 1;
				goto end;
			} else if (3 == menu.current) {
				filter_games_set(FILTER_ALL, -1);
				redraw_cover = 1;
			}
			else if (4 == menu.current) {
				redraw_cover = 1;
				filter_games_set(FILTER_ONLINE, -1);
			}
			n = menu.current - 5;
			if (n >= 0 && n < featureCnt) {
				redraw_cover = 1;
				filter_games_set(FILTER_FEATURES, n);
			}
		}
		
		// HOME button
		if (buttons & CFG.button_exit.mask) {
			Handle_Home(0);
		}
		if (buttons & CFG.button_cancel.mask) break;
	}
	end:
	return 0;
}

int Menu_Filter4()			//Filter by Game Type
{
	struct discHdr *header = NULL;
	int redraw_cover = 0;
	struct Menu menu;
	int rows, cols, size;
	CON_GetMetrics(&cols, &rows);
	if ((size = rows-10) < 3) size = 3;		//scroll area -5 fixed and 5 overhead
	menu_init(&menu, gameTypeCnt+5);			//Add 5 fixed menu items
	for (;;) {

		menu.line_count = 0;

		if (gameCnt) {
			header = &gameList[gameSelected];
		} else {
			header = NULL;
		}
		int n;
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Filter by Game Type"));
		printf(":\n\n");
		MENU_MARK();
		printf("<%s>\n", gt("Filter by Genre"));
		MENU_MARK();
		printf("<%s>\n", gt("Filter by Controller"));
		MENU_MARK();
		printf("<%s>\n", gt("Filter by Online Features"));
		MENU_MARK();
		printf("%s %s\n", mark_filter(FILTER_ALL,-1), gt("All Games"));
		MENU_MARK();
		printf("%s %s\n", mark_filter(FILTER_UNPLAYED,-1), gt("Unplayed Games"));
		menu_window_begin(&menu, size, gameTypeCnt);
		for (n=0;n<gameTypeCnt;n++) {
			if (menu_window_mark(&menu))
				printf("%s %s\n", mark_filter(FILTER_GAME_TYPE,n), gt(gameTypes[n][1]));
		}
		DefaultColor();
		menu_window_end(&menu, cols);
		
		//printf("\n");
		printf_h(gt("Press %s to select filter type"), (button_names[CFG.button_confirm.num]));
		__console_flush(0);

		if (redraw_cover) {
			if (header) Gui_DrawCover(header->id);
			redraw_cover = 0;
		}
		
		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move(&menu, buttons);

		int change = 0;
		if (buttons & WPAD_BUTTON_LEFT) change = -1;
		if (buttons & WPAD_BUTTON_RIGHT || buttons & CFG.button_confirm.mask || buttons & CFG.button_save.mask) change = +1;

		if (change) {
			if (0 == menu.current) {
				Menu_Filter();
				redraw_cover = 1;
				break;
			} else if (1 == menu.current) {
				Menu_Filter2();
				redraw_cover = 1;
				break;
			} else if (2 == menu.current) {
				Menu_Filter3();
				redraw_cover = 1;
				break;
			} else if (3 == menu.current) {
				filter_games_set(FILTER_ALL, -1);
				redraw_cover = 1;
			} else if (4 == menu.current) {
				redraw_cover = 1;
				filter_games_set(FILTER_UNPLAYED, -1);
			}
			n = menu.current - 5;
			if (n >= 0 && n < gameTypeCnt) {
				redraw_cover = 1;
				filter_games_set(FILTER_GAME_TYPE, n);
			}
		}
		
		// HOME button
		if (buttons & CFG.button_exit.mask) {
			Handle_Home(0);
		}
		if (buttons & CFG.button_cancel.mask) break;
	}
	return 0;
}

int Menu_Sort()
{
	struct discHdr *header = NULL;
	int redraw_cover = 0;
	int n = 0;
	bool first_run = 1;
	bool descend[sortCnt];
	for (;n<sortCnt;n++)
		descend[n] = 0;
	struct Menu menu;
	menu_init(&menu, sortCnt +1);
	for (;;) {

		menu.line_count = 0;

		if (gameCnt) {
			header = &gameList[gameSelected];
		} else {
			header = NULL;
		}
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Choose a sorting method"));
		printf(":\n\n");
		for (n=0; n<sortCnt; n++) {
			if (sort_index == n && sort_desc && first_run) {
				first_run = 0;
				descend[n] = 1;
			}
			MENU_MARK();
			printf("%s ", (sort_index == n ? "*": " "));
			printf("%s", con_align(sortTypes[n].name,25));
			printf("%s\n", !descend[n] ? gt("< ASC  >") : gt("< DESC >"));
		}
		MENU_MARK();
		printf("%s ", (sort_index == n ? "*": " "));
		printf("%s", con_align(gt("Duplicate ID3"),25));
		DefaultColor();

		printf("\n");
		printf_h(gt("Press %s to select sorting method"), (button_names[CFG.button_confirm.num]));
		__console_flush(0);

		if (redraw_cover) {
			if (header) Gui_DrawCover(header->id);
			redraw_cover = 0;
		}
		
		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move(&menu, buttons);

		int change = 0;
		if (buttons & WPAD_BUTTON_LEFT || buttons & WPAD_BUTTON_RIGHT) change = -1;
		if (buttons & CFG.button_confirm.mask || buttons & CFG.button_save.mask) change = +1;

		if (change) {
			n = menu.current;
			if (n >= 0 && n < sortCnt) {
				if (change == -1) descend[n] = !descend[n];
				else {
					if (descend[n]) {
						sort_desc = 1;
						sortList(sortTypes[n].sortDsc);
					} else {
						sort_desc = 0;
						sortList(sortTypes[n].sortAsc);
					}
					redraw_cover = 1;
					sort_index = n;
				}
			}
			if (n == sortCnt){
				sortList(__sort_id_asc);
				filter_games_set(FILTER_DUPLICATE_ID3, -1);
				redraw_cover = 1;
				sort_index = n;

			}
			
		}
		
		// HOME button
		if (buttons & CFG.button_exit.mask) {
			Handle_Home(0);
		}
		if (buttons & CFG.button_cancel.mask) break;
	}
	return 0;
}
