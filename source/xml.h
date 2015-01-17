#ifndef _XML_H_
#define _XML_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define XML_NUM_ACCESSORY 12
// all: 16 actual max: 7 controllers for SC7E52

#define XML_NUM_FEATURES 4

#define XML_MAX_SYNOPSIS 3000

typedef struct gameXMLinfo
{	
	char id[8];
	char region[8];
	char title[100];
	char *synopsis;
	char developer[40];
	char publisher[40];
	int year;
	int month;
	int day;
	char genre[64];
	char ratingtype[6];
	char ratingvalue[6];
	//char ratingdescriptors[16][40];
	int wifiplayers;
	char wififeatures[XML_NUM_FEATURES];
	int players;
	// accessoryID: 0=unused 1+id=supported 100+id=required
	char accessoryID[XML_NUM_ACCESSORY];
	u32 caseColor;
	int hnext;
} gameXMLinfo;

struct gameXMLinfo *get_game_info(int i);
struct gameXMLinfo *get_game_info_id(u8 *gameid);

char* getLang(int lang);
bool ReloadXMLDatabase(char* xmlfilepath, char* argdblang, bool argJPtoEN);
void CloseXMLDatabase();
bool LoadGameInfoFromXML(u8 * gameid);
char *ConvertLangTextToCode(char *langtext);
char *VerifyLangCode(char *langtext);
int ConvertRatingToAge(char *ratingvalue, char *ratingSystem);
void ConvertRating(char *ratingvalue, char *fromrating, char *torating, char *destvalue, int destsize);
void FmtGameInfo(char *linebuf, int cols, int size);
void PrintGameInfo();
void PrintGameSynopsis();
int getIndexFromId(u8 * gameid);

bool DatabaseLoaded(void);
int getControllerTypes(char *controller, u8 * gameid);
const char *getControllerName(gameXMLinfo *g, int i, int *req);
bool hasGenre(char *genre, u8 * gameid);
bool hasFeature(char *feature, u8 *gameid);
bool xml_getCaseColor(u32 *color, u8 *gameid);

#ifdef __cplusplus
}
#endif

#endif
