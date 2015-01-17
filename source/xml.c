/*
Load game information from XML - Lustar
*/
#include <zlib.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "util.h"
#include "cfg.h"
#include "xml.h"
#include "wbfs.h"
#include "menu.h"
#include "mem.h"
#include "wpad.h"
#include "gettext.h"
#include "debug.h"
#include "sort.h"

#include "unzip/unzip.h"
#include "unzip/miniunz.h"

typedef struct xmlIndex
{
	char *start;
	char *attr;
	char *val;  // [-1]: '>' => '\0'
	char *vend; // '<' => '\0'
	char *end;  // '>' => '\0'
} xmlIndex;

struct xmlTag
{
	char name[16];
	int len;
	int opt;
};

enum TAG_I
{
	TAG_ID,
	//TAG_TYPE,
	TAG_REGION,
	//TAG_LANGUAGES,
	TAG_LOCALE,
	TAG_DEVELOPER,
	TAG_PUBLISHER,
	TAG_DATE,
	TAG_GENRE,
	TAG_RATING,
	TAG_WIFI,
	TAG_INPUT,
	//TAG_ROM,
	TAG_CASE,
	TAG_MAX
};

struct xmlTag tags[] =
{
	{ "id" },
	//{ "type" },
	{ "region" },
	//{ "languages" },
	{ "locale" },
	{ "developer" },
	{ "publisher" },
	{ "date" },
	{ "genre" },
	{ "rating" },
	{ "wi-fi" },
	{ "input" },
	//{ "rom" },
	{ "case" }
};

struct xmlIndex idx[TAG_MAX];

/* config */
char xmlCfgLang[3];
char xmlcfg_filename[100];
char *xmlData;
static struct gameXMLinfo gameinfo;

struct gameXMLinfo **game_info;
int xmlgameCnt;
int array_size = 0;

obj_stack obs_game_info;
HashTable hash_game_info;

struct gameXMLinfo *get_game_info(int i)
{
	return game_info[i];
}

struct gameXMLinfo *get_game_info_id(u8 *gameid)
{
	int i = getIndexFromId(gameid);
	if (i >= 0 && i < xmlgameCnt) return game_info[i];
	return NULL;
}

bool xml_loaded = false;

extern int all_gameCnt;
bool db_debug = 0;

static char langlist[11][22] =
{{"Console Default"},
{"Japanese"},
{"English"},
{"German"},
{"French"},
{"Spanish"},
{"Italian"},
{"Dutch"},
{"S. Chinese"},
{"T. Chinese"},
{"Korean"}};

static char langcodes[11][3] =
{{""},
{"JA"},
{"EN"},
{"DE"},
{"FR"},
{"ES"},
{"IT"},
{"NL"},
{"ZH"},
{"ZH"},
{"KO"}};


void xml_stat()
{
	printf("  Games in XML: %d (%d) / %d\n", xmlgameCnt, array_size, all_gameCnt);
}

char * getLang(int lang) {
	if (lang < 1 || lang > 10) return "EN";
	return langcodes[lang+1];
}

char * unescape(char *input, int size) {
	str_replace_all(input, "&gt;", ">", size);
	str_replace_all(input, "&lt;", "<", size);
	str_replace_all(input, "&quot;", "\"", size);
	str_replace_all(input, "&apos;", "'", size);
	str_replace_all(input, "&amp;", "&", size);
	return input;
}

void wordWrap(char *tmp, char *input, int width, int maxLines, int length)
{
	int maxLength = width * maxLines;
	char *whitespace = NULL;
	unescape(input, length);
	char dots[4] = {0xE2, 0x80, 0xA6, 0};
	str_replace_all(input, dots, "...", length);
	strncpy(tmp, input, maxLength);
	int lineLength = 0;
	int wordLength = 0;
	int lines = 1;
	
	char * ptr=tmp;
	while (*ptr!='\0') {
		if (*ptr == '\n' || *ptr == '\r')
			*ptr = ' ';
		ptr++;
	}
	str_replace_all(tmp, "  ", " ", sizeof(tmp));
	
	for (ptr=tmp;*ptr!=0;ptr++) {
		if (lines >= maxLines) *ptr = 0;
		if (*ptr == ' ') {
			if (lineLength + wordLength > width) {
				if (whitespace) {
					*whitespace = '\n';
					lineLength = ptr - whitespace;
					whitespace = NULL;
				} else {
					*ptr = '\n';
					lineLength = 0;
				}
				lines++;
			} else {
				whitespace = ptr;
				lineLength += wordLength+1;
			}
			wordLength = 0;
		} else if (*ptr == '\n') {
			lineLength = 0;
			wordLength = 0;
		} else {
			wordLength++;
		}
	}
	if (length > maxLength && lineLength <= (width - 3)) { 
		strncat(tmp, "...", 3);
	}
}

//1 = Required, 0 = Usable, -1 = Not Used
//Controller = wiimote, nunchuk, classic, guitar, etc
int getControllerTypes(char *controller, u8 * gameid)
{
	gameXMLinfo *g = get_game_info_id(gameid);
	if (!g)	return -1;
	int acc_id = get_accesory_id(controller);
	if (acc_id < 0)	return -1;
	int i;
	for (i=0; i<XML_NUM_ACCESSORY; i++) {
		int x = g->accessoryID[i];
		if (x == 0) break;
		if (x == 1 + acc_id) return 0;
		if (x == 100 + acc_id) return 1;
	}
	return -1;
}

const char *getControllerName(gameXMLinfo *g, int i, int *req)
{
	if (i >= XML_NUM_ACCESSORY) return NULL;
	int x = g->accessoryID[i];
	if (x == 0) return NULL;
	if (x >= 100) {
		x -= 100;
		*req = 1;
	} else {
		x -= 1;
		*req = 0;
	}
	return get_accesory_name(x);
}

bool hasFeature(char *feature, u8 *gameid)
{
	gameXMLinfo *g = get_game_info_id(gameid);
	if (!g)	return 0;
	int f_id = get_feature_id(feature);
	int i;
	for (i=0; i<XML_NUM_FEATURES; i++) {
		int x = g->wififeatures[i];
		if (x == 0) break;
		if (x == 1 + f_id) return 1;
	}
	return 0;
}

bool hasGenre(char *genre, u8 * gameid)
{
	gameXMLinfo *g = get_game_info_id(gameid);
	if (g) {
		if (strstr(g->genre, genre)) {
			if (!strcmp(genre,"tennis")) {		// if looking for tennis
				if ((strstr(g->genre, "table tennis,tennis")) || (!strstr(g->genre, "table tennis")))	//exclude ones that only have table tennis
					return 1;
			} else
				return 1;
		}
	}
	return 0;
}

bool xml_getCaseColor(u32 *color, u8 *gameid)
{
	gameXMLinfo *g = get_game_info_id(gameid);
	if (g)	{
		if (g->caseColor != 0) {
			*color = g->caseColor;
			return 1;
		}
	}
	return 0;
}

bool DatabaseLoaded() {
	return xml_loaded;
}

void CloseXMLDatabase()
{
	SAFE_FREE(game_info);
	xmlgameCnt = 0;
	array_size = 0;
	obs_freeall(&obs_game_info);
	hash_close(&hash_game_info);
	xml_loaded = false;
}

bool OpenXMLFile(char *filename)
{
	get_time(&TIME.db_load1);
	memset(&gameinfo, 0, sizeof(gameinfo));
	xml_loaded = false;
	char* strresult = strstr(filename,".zip");
	if (strresult == NULL) {
		FILE *filexml;
		filexml = fopen(filename, "rb");
		if (!filexml)
			return false;
		
		fseek(filexml, 0 , SEEK_END);
		u32 size = ftell(filexml);
		rewind(filexml);
		xmlData = mem_calloc(size);
		if (xmlData == NULL) {
			fclose(filexml);
			return false;
		}
		u32 ret = fread(xmlData, 1, size, filexml);
		fclose(filexml);
		if (ret != size)
			return false;
	} else {
		unzFile unzfile = unzOpen(filename);
		if (unzfile == NULL)
			return false;
			
		unzOpenCurrentFile(unzfile);
		unz_file_info zipfileinfo;
		unzGetCurrentFileInfo(unzfile, &zipfileinfo, NULL, 0, NULL, 0, NULL, 0);	
		int zipfilebuffersize = zipfileinfo.uncompressed_size;
		if (db_debug) {
			printf("uncompressed xml: %dkb\n", zipfilebuffersize/1024);
		}
		xmlData = mem_calloc(zipfilebuffersize);
		if (xmlData == NULL) {
			unzCloseCurrentFile(unzfile);
			unzClose(unzfile);
			return false;
		}
		unzReadCurrentFile(unzfile, xmlData, zipfilebuffersize);
		unzCloseCurrentFile(unzfile);
		unzClose(unzfile);
	}
	get_time(&TIME.db_load2);

	xml_loaded = (xmlData == NULL) ? false : true;
	return xml_loaded;
}

/* convert language text into ISO 639 two-letter language code */
char *ConvertLangTextToCode(char *languagetxt)
{
	if (!strcmp(languagetxt, ""))
		return "EN";
	int i;
	for (i=1;i<=10;i++)
	{
		if (!strcasecmp(languagetxt,langlist[i])) // case insensitive comparison
			return langcodes[i];
	}
	return "EN";
}

char *VerifyLangCode(char *languagetxt)
{
	if (!strcmp(languagetxt, ""))
		return "EN";
	int i;
	for (i=1;i<=10;i++)
	{
		if (!strcasecmp(languagetxt,langcodes[i])) // case insensitive comparison
			return langcodes[i];
	}
	return "EN";
}

char ConvertRatingToIndex(char *ratingtext)
{
	int type = -1;
	if (!strcmp(ratingtext,"CERO")) { type = 0; }
	if (!strcmp(ratingtext,"ESRB")) { type = 1; }
	if (!strcmp(ratingtext,"PEGI")) { type = 2; }
	return type;
}

int ConvertRatingToAge(char *ratingvalue, char *ratingSystem)
{
	int age;
	
	age = atoi(ratingvalue);
	if ((age > 0 ) && (age <= 25)) return age;
	if (ratingvalue[0] == 'A') {
		if (!strncmp(ratingvalue,"AO",2)) return 18;
		return 0;
	}
	if (ratingvalue[0] == 'B') return 12;
	if (ratingvalue[0] == 'C') return 15;
	if (ratingvalue[0] == 'D') return 17;
	if (ratingvalue[0] == 'E') {
		if (!strncmp(ratingvalue,"E1",2)) return 10;	//E10+
		if (!strncmp(ratingvalue,"EC",2)) return 3;
		return 6;
	}
	if (ratingvalue[0] == 'G') return 0;
	if (ratingvalue[0] == 'L') return 0;
	if (ratingvalue[0] == 'T') return 13;
	if (ratingvalue[0] == 'Z') return 18;
	if (ratingvalue[0] == '0') return 0;
	if (ratingvalue[0] == 'M') {
		if (!strncmp(ratingvalue,"M18",3)) return 18;
		if (!strncmp(ratingvalue,"MA15",4)) return 15;
		if (!strncmp(ratingSystem,"ACB",3)
			|| (!ratingSystem && (CONF_GetArea() == CONF_AREA_AUS))) return 12;
		if (!strncmp(ratingSystem,"OFLC",4)
		/*	|| (!ratingSystem && (CONF_GetShopCode() == 95)) */) return 13;	//New Zeland needs updated lib for GetShopCode to work
		return 17;		//assume us ESRB system
	}
	if (ratingvalue[0] == 'P') {
		if (!strncmp(ratingvalue,"PG12",4)) return 12;
		if (!strncmp(ratingvalue,"PG 12",5)) return 12;
		if (!strncmp(ratingvalue,"PG15",4)) return 15;
		if (!strncmp(ratingvalue,"PG 15",5)) return 15;
		if (!strncmp(ratingvalue,"PG",2)) return 8;
		return 6;
	}
	if (ratingvalue[0] == 'R') {
		if (!strncmp(ratingvalue,"R13",3)) return 13;
		if (!strncmp(ratingvalue,"R16",3)) return 16;
		if (!strncmp(ratingvalue,"R18",3)) return 18;
		return 18;
	}		
	return -1;
}

void ConvertRating(char *ratingvalue, char *fromrating, char *torating, char *destvalue, int destsize)
{
	if(!strcmp(fromrating,torating)) {
		strlcpy(destvalue,ratingvalue,destsize);
		return;
	}

	strcpy(destvalue,"");
	int type = -1;
	int desttype = -1;

	type = ConvertRatingToIndex(fromrating);
	desttype = ConvertRatingToIndex(torating);
	if (type == -1 || desttype == -1)
		return;
	
	/* rating conversion table */
	/* the list is ordered to pick the most likely value first: */
	/* EC and AO are less likely to be used so they are moved down to only be picked up when converting ESRB to PEGI or CERO */
	/* the conversion can never be perfect because ratings can differ between regions for the same game */
	char ratingtable[12][3][4] =
	{
		{{"A"},{"E"},{"3"}},
		{{"A"},{"E"},{"4"}},
		{{"A"},{"E"},{"6"}},
		{{"A"},{"E"},{"7"}},
		{{"A"},{"EC"},{"3"}},
		{{"A"},{"E10+"},{"7"}},
		{{"B"},{"T"},{"12"}},
		{{"D"},{"M"},{"18"}},
		{{"D"},{"M"},{"16"}},
		{{"C"},{"T"},{"16"}},
		{{"C"},{"T"},{"15"}},
		{{"Z"},{"AO"},{"18"}},
	};
	
	int i;
	for (i=0;i<=11;i++)
	{
		if (!strcmp(ratingtable[i][type],ratingvalue)) {
			strlcpy(destvalue,ratingtable[i][desttype],destsize);
			return;
		}
	}
}

char * GetLangSettingFromGame(u8 * gameid) {
	int langcode;
	struct Game_CFG game_cfg = CFG_read_active_game_setting((u8*)gameid);
	if (game_cfg.language) {
		langcode = game_cfg.language;
	} else {
		langcode = CFG.game.language;
	}
	char *langtxt = langlist[langcode];
	return langtxt;
}

void strncpySafe(char *out, char *in, int max, int length) {
	strlcpy(out, in, ((max < length+1) ? max : length+1));
}

void prep_tags()
{
	int i;
	for (i=0; i<TAG_MAX; i++) {
		tags[i].len = strlen(tags[i].name);
	}
	tags[TAG_LOCALE].opt = 1;
}

char* scan_index(char *s, char *etag)
{
	int i;
	int first = 0;
	int elen = strlen(etag);
	memset(idx, 0, sizeof(idx));
	while (1) {
		s = strchr(s, '<');
		if (!s) break;
		s++;
		if (*s == '/') {
			s++;
			// did we reach endtag?
			if (memcmp(s, etag, elen)==0) {
				// zero terminate
				s[-2] = 0;
				s += elen;
				break;
			}
			continue;
		}
		for (i=first; i<TAG_MAX; i++) {
			if (idx[i].start) {
				if (i == first) first++;
				continue;
			}
			if (memcmp(s, tags[i].name, tags[i].len)==0) {
				idx[i].start = s-1;
				s += tags[i].len;
				idx[i].attr = s;
				s = strchr(s, '>');
				if (!s) goto out;
				if (tags[i].opt) break;
				// zero terminate
				*s = 0;
				s++;
				// is closing /> ?
				if (s[-2] != '/') {
					idx[i].val = s;
					// find closing tag
					while (1) {
						s = strstr(s, "</");
						if (!s) goto out;
						s += 2;
						if (memcmp(s, tags[i].name, tags[i].len)==0) {
							// zero terminate
							s[-2] = 0;
							idx[i].vend = s - 2;
							s = strchr(s, '>');
							if (!s) goto out;
							idx[i].end = s;
							s++;
							break;
						}
					}
				}
				break;
			}
		} // for
	} // while
out:
	/*for (i=0; i<TAG_MAX; i++) {
		if (idx[i].start) gecko_printf("%s:%p\n", tags[i].name, idx[i].attr);
	}*/
	return s;
}

static inline
int readVal(xmlIndex *x, char *val, int size)
{
	int len;
	if (!x->val || !x->vend) return 0;
	len = x->vend - x->val;

	if (len >= size) len = size - 1;
	//if (len < 0) return 0;
	memcpy(val, x->val, len);
	val[len] = 0;
	return 1;
}

static inline
char* readAttr(char *s, char *attr, char *val, int size)
{
	char *p1, *p2;
	*val = 0;
	if (!s) return NULL;
	p1 = strstr(s, attr);
	if (!p1) return NULL;
	p1 += strlen(attr);
	if (*p1 == '"') p1++;
	p2 = strchr(p1, '"');
	if (!p2) return NULL;
	strncpySafe(val, p1, size, p2-p1);
	return p2;
}

static inline
char* readAttrInt(char *s, char *attr, int *val)
{
	char *p1, *p2;
	*val = 0;
	p1 = strstr(s, attr);
	if (!p1) return NULL;
	p1 += strlen(attr);
	if (*p1 == '"') p1++;
	p2 = strchr(p1, '"');
	if (!p2) return NULL;
	*val = atoi(p1);
	return p2;
}

int intcpy(char *in, int length) {
	char tmp[10];
	strncpy(tmp, in, length);
	return atoi(tmp);
}

void readNode(char * p, char to[], char * open, char * close) {
	char * tmp = strstr(p, open);
	if (tmp == NULL) {
		strcpy(to, "");
	} else {
		char * s = tmp+strlen(open);
		strlcpy(to, s, strstr(tmp, close)-s+1);
	}
}

void readDate(xmlIndex *x, struct gameXMLinfo *g)
{
	char *p;
	p = x->attr;
	if (!p) return;
	p = readAttrInt(p, " year=", &g->year);
	if (!p) return;
	p = readAttrInt(p, " month=", &g->month);
	if (!p) return;
	p = readAttrInt(p, " day=", &g->day);
}

void readRatings(xmlIndex *x, struct gameXMLinfo *g)
{
	char *s, *p;
	s = p = x->attr;
	if (!s) return;
	readAttr(s, " type=", g->ratingtype, sizeof(g->ratingtype));
	readAttr(s, " value=", g->ratingvalue, sizeof(g->ratingvalue));
	// descriptions not needed
	/*
	if (strncmp(locEnd-1, "/>", 2)==0) return;
	locEnd = strstr(locStart, "</rating>");
	if (!locEnd) return;
	else {
		char * tmp = strndup(locStart, locEnd-locStart);
		char * reset = tmp;
		locEnd = strstr(locStart, "\" value=\"");
		if (locEnd == NULL) return;
		strncpySafe(g_info->ratingtype, locStart+14, sizeof(gameinfo.ratingtype), locEnd-(locStart+14));
		locStart = locEnd;
		locEnd = strstr(locStart, "\">");
		strncpySafe(g_info->ratingvalue, locStart+9, sizeof(gameinfo.ratingvalue), locEnd-(locStart+9));
		int z = 0;
		while (tmp != NULL) {
			if (z >= 15) break;
			tmp = strstr(tmp, "<descriptor>");
			if (tmp == NULL) {
				break;
			} else {
				char * s = tmp+strlen("<descriptor>");
				tmp = strstr(tmp+1, "</descriptor>");
				strncpySafe(g_info->ratingdescriptors[z], s, sizeof(g_info->ratingdescriptors[z]), tmp-s);
				z++;
			}
		}
		tmp = reset;
		SAFE_FREE(tmp);
	}
	*/
}

void readPlayers(xmlIndex *x, struct gameXMLinfo *g)
{
	readAttrInt(x->attr, "players=", &g->players);
}

void readCaseColor(xmlIndex *x, struct gameXMLinfo *g)
{
	char cc[16];
	int len, num;
	u32 col;
	g->caseColor = 0x0;
	if (!x->attr) return;
	char *p = readAttr(x->attr, "color=", cc, sizeof(cc));
	if (!p) return;
	num = sscanf(cc, "%x%n", &col, &len);
	if (num == 1 && len == 6) {
		// force alpha to opaque and mark color found
		// (so black can be specified too)
		g->caseColor = (col << 8) | 0xFF;
		//gecko_printf("case: %s -> %08x\n", cc, g->caseColor);
	}
}

void readWifi(xmlIndex *x, struct gameXMLinfo *g)
{
	char *p;
	if (!x->attr) return;
	p = readAttrInt(x->attr, " players=", &g->wifiplayers);
	if (!p) return;
	char *tmp = x->val;
	char *s;
	int z = 0;
	while (tmp != NULL) {
		if (z >= XML_NUM_FEATURES) break;
		tmp = strstr(tmp, "<feature>");
		if (!tmp) break;
		s = tmp+strlen("<feature>");
		tmp = strstr(s, "</feature>");
		if (!tmp) break;
		*tmp = 0;
		tmp++;
		int f_id = get_feature_id(s);
		if (f_id >= 0) {
			g->wififeatures[z] = 1 + f_id;
			z++;
		}
	}
}


void readControls(char * start, struct gameXMLinfo *g)
{
	int z = 0;
	char *p = start;
	char *str = "<control type=\"";
	int len = strlen(str);
	while (p != NULL) {
		if (z >= XML_NUM_ACCESSORY) break;
		p = strstr(p, str);
		if (p == NULL) break;
		char *s = p + len;
		p = strstr(p+1, "\" required=\"");
		if (p == NULL) break;
		bool required = (p[12] == 't' || p[12] == 'T'); // "True"
		*p = 0;
		p++;
		int acc_id = get_accesory_id(s);
		if (acc_id >= 0) {
			g->accessoryID[z] = acc_id + (required ? 100 : 1);
			z++;
		}
	}
	// The followint games use controllers not supported by the gametdb database
	if (strncmp(g->id, "RX5", 3) == 0)		//Tony Hawk Ride
		g->accessoryID[z] = get_accesory_id("skateboard") + 100;
	if (strncmp(g->id, "STY", 3) == 0)		//Tony Hawk Shred
		g->accessoryID[z] = get_accesory_id("skateboard") + 100;
	if (strncmp(g->id, "SQI", 3) == 0)		//Disney Infinity
		g->accessoryID[z] = get_accesory_id("infinitybase") + 100;
	if (strncmp(g->id, "SSP", 3) == 0)		//Skylanders Spyros Adventure
		g->accessoryID[z] = get_accesory_id("portalofpower") + 100;
	if (strncmp(g->id, "SKY", 3) == 0)		//Skylanders Giants
		g->accessoryID[z] = get_accesory_id("portalofpower") + 100;
	if (strncmp(g->id, "SVX", 3) == 0)		//Skylanders Swamp Force
		g->accessoryID[z] = get_accesory_id("portalofpower") + 100;
	if (strncmp(g->id, "SWA", 3) == 0)		//DJ Hero
		g->accessoryID[z] = get_accesory_id("turntable") + 100;
	if (strncmp(g->id, "SWB", 3) == 0)		//DJ Hero 2
		g->accessoryID[z] = get_accesory_id("turntable") + 100;
	if (strncmp(g->id, "RYR", 3) == 0)		//Your Shape
		g->accessoryID[z] = get_accesory_id("camera") + 100;
	if (strncmp(g->id, "SRQ", 3) == 0)		//Racquet Sports
		g->accessoryID[z] = get_accesory_id("camera") + 1;
	if (strncmp(g->id, "SF5", 3) == 0)		//Fit in Six
		g->accessoryID[z] = get_accesory_id("camera") + 1;
	if (strncmp(g->id, "SE2", 3) == 0)		//Active 2
		g->accessoryID[z] = get_accesory_id("totalbodytracking") + 100;
	if (strncmp(g->id, "SNF", 3) == 0)		//NFL Training Camp
		g->accessoryID[z] = get_accesory_id("totalbodytracking") + 100;
	
}


void readTitles(xmlIndex *x, struct gameXMLinfo *g)
{
	char *locStart;
	char *locEnd;
	char *tmpLang;
	int found = 0;
	int f_title = 0;
	int f_synopsis = 0;
	char *locTmp;
	char *titStart;
	char *titEnd;
	char temp_synopsis[XML_MAX_SYNOPSIS *2];

	if (!x->start) return;
	locEnd = x->start;

	while ((locStart = strstr(locEnd, "<locale lang=\"")) != NULL) {
		locEnd = strstr(locStart, "</locale>");
		if (locEnd == NULL) {
			locEnd = strstr(locStart, "\"/>");
			if (locEnd == NULL) break;
			continue;
		}
		*locEnd = 0; // zero terminate
		locEnd++; // move to next entry
		tmpLang = locStart+14;
		if (memcmp(tmpLang, xmlCfgLang, 2) == 0) {
			found = 3;
		} else if (memcmp(tmpLang, "EN", 2) == 0) {
			found = 2;
		} else {
			found = 1;
		}
		// 3. get the configured language text
		// 2. if not found, get english
		// 1. else get whatever is found
		locTmp = locStart;
		if (f_title < found) {
			titStart = strstr(locTmp, "<title>");
			if (titStart != NULL) {
				titEnd = strstr(titStart, "</title>");
				strncpySafe(g->title, titStart+7,
						sizeof(g->title), titEnd-(titStart+7));
				unescape(g->title, sizeof(g->title));
				f_title = found;
			}
		}
		if (f_synopsis < found) {
			titStart = strstr(locTmp, "<synopsis>");
			if (titStart != NULL) {
				titEnd = strstr(titStart, "</synopsis>");
				int len = titEnd-(titStart+10);
				if (len >= sizeof(temp_synopsis)) len = sizeof(temp_synopsis) - 1;
				strcopy(temp_synopsis, titStart+10, len+1);
				unescape(temp_synopsis,sizeof(temp_synopsis));
				len = strlen(temp_synopsis);
				if (len >= XML_MAX_SYNOPSIS) len = XML_MAX_SYNOPSIS - 1;
				g->synopsis = obs_alloc(&obs_game_info, len+1);
				if (!g->synopsis) break;
				strcopy(g->synopsis, temp_synopsis, len+1);
				f_synopsis = found;
			}
		}
		if (f_title == 3 && f_synopsis == 3) break;
	}
}

bool xml_compare_key(void *cb, void *key, int handle)
{
	char *id = (char*)get_game_info(handle)->id;
	// Some entries in wiitdb have ID4 instead of ID6 (wiiware)
	// but the image has ID6 (ID4+"00")
	// so we cut the key to ID4 if it is ID4 in wiitdb
//	int len = (id[4] == 0) ? 4 : 6;
	// always need to compare whole id6.
	// game id4 s are null terminated when they are loaded.
	// only comparing id4 was causing incorrect matching for titles like wii fit plus and mario kart wii,
	// when their is a channel id4 and the first 4 of the id6 match.
	int len = 6;
	return (strncmp(id, (char *)key, len) == 0);
}

int* xml_next_handle(void *cb, int handle)
{
	return &get_game_info(handle)->hnext;
}

void LoadTitlesFromXML(char *langtxt, bool forcejptoen)
{
	char * pos = xmlData;
	bool forcelang = false;
	struct gameXMLinfo *g;
	
	xmlgameCnt = 0;
	obs_init(&obs_game_info, 10240, mem1_realloc, mem_resize);
	hash_init(&hash_game_info, 0, NULL, &hash_id4, &xml_compare_key, &xml_next_handle);

	if (strcmp(langtxt,""))
		forcelang = true;
	if (forcelang) {
		// convert language text into ISO 639 two-letter language code
		strcpy(xmlCfgLang, (strlen(langtxt) == 2) ? VerifyLangCode(langtxt) : ConvertLangTextToCode(langtxt));
	}
	if (forcejptoen && (!strcmp(xmlCfgLang,"JA")))
		strcpy(xmlCfgLang,"EN");
	
	prep_tags();

	while (1) {
		if (xmlgameCnt >= array_size) {
			const int alloc_chunk = 100;
			array_size += alloc_chunk;
			game_info = realloc(game_info, array_size * sizeof(*game_info));
		}
		g = game_info[xmlgameCnt] = obs_alloc(&obs_game_info, sizeof(struct gameXMLinfo));
		if (g == NULL) break;
		//memset(g, 0, sizeof(*g)); //already done by mem1_realloc
		pos = strstr(pos, "<game");
		if (pos == NULL) {
			break;
		}
		pos = scan_index(pos, "game");
		if (pos == NULL) {
			break;
		}
		#define READ_VAL(T,V) readVal(idx+T, V, sizeof(V))
		READ_VAL(TAG_ID, g->id);
		if (g->id[0] == '\0') { //WTF? ERROR
			printf(" ID NULL\n");
			break;
		}
		hash_add(&hash_game_info, g->id, xmlgameCnt);
		readTitles(idx+TAG_LOCALE, g);
		readDate(idx+TAG_DATE, g);
		readRatings(idx+TAG_RATING, g);
		readWifi(idx+TAG_WIFI, g);
		readControls(idx[TAG_INPUT].val, g);
		READ_VAL(TAG_REGION, g->region);
		READ_VAL(TAG_DEVELOPER, g->developer);
		READ_VAL(TAG_PUBLISHER, g->publisher);
		READ_VAL(TAG_GENRE, g->genre);
		readCaseColor(idx+TAG_CASE, g);
		readPlayers(idx+TAG_INPUT, g);
		//ConvertRating(get_game_info(n)->ratingvalue, get_game_info(n)->ratingtype, "ESRB");
		xmlgameCnt++;
	}
	SAFE_FREE(xmlData);
	return;
}

/* load renamed titles from proper names and game info XML, needs to be after cfg_load_games */
bool OpenXMLDatabase(char* xmlfilepath, char* argdblang, bool argJPtoEN)
{
	if (xml_loaded) {
		return true;
	}
	long long start = 0;
	if (db_debug) {
		printf("Database Debug Info:\n");
		mem_stat();
		start = gettime();
	}
	bool opensuccess = true;
	sprintf(xmlcfg_filename, "wiitdb.zip");

	char pathname[200];
	snprintf(pathname, sizeof(pathname), "%s", xmlfilepath);
	if (xmlfilepath[strlen(xmlfilepath) - 1] != '/') snprintf(pathname, sizeof(pathname), "%s/",pathname);
	snprintf(pathname, sizeof(pathname), "%s%s", pathname, xmlcfg_filename);
	opensuccess = OpenXMLFile(pathname);
	if (!opensuccess) {
		CloseXMLDatabase();
		sprintf(xmlcfg_filename, "wiitdb_%s.zip", CFG.partition);

		snprintf(pathname, sizeof(pathname), "%s", xmlfilepath);
		if (xmlfilepath[strlen(xmlfilepath) - 1] != '/') snprintf(pathname, sizeof(pathname), "%s/",pathname);
		snprintf(pathname, sizeof(pathname), "%s%s", pathname, xmlcfg_filename);
		opensuccess = OpenXMLFile(pathname);
		if (!opensuccess) {
			CloseXMLDatabase();
			return false;
		}
	}
	LoadTitlesFromXML(argdblang, argJPtoEN);
	if (db_debug) {
		printf_("Load Time: %u ms\n", diff_msec(start, gettime()));
		mem_stat();
		Wpad_WaitButtons();
	}
	return true;
}

bool ReloadXMLDatabase(char* xmlfilepath, char* argdblang, bool argJPtoEN)
{
	if (xml_loaded) {
		if (db_debug) xml_stat();
		CloseXMLDatabase();
	}
	return OpenXMLDatabase(xmlfilepath, argdblang, argJPtoEN);
}

int getIndexFromId(u8 * gameid)
{
	if (gameid == NULL) return -1;
	return hash_get(&hash_game_info, gameid);
}

/* gameid: full game id */
bool LoadGameInfoFromXML(u8 * gameid)
{
	memset(&gameinfo, 0, sizeof(gameinfo));
	gameXMLinfo *g = get_game_info_id(gameid);
	if (!g) return false;
	gameinfo = *g;
	return true;
}

void RemoveChar(char *string, char toremove)
{
	char *ptr;
	ptr = string;
	while (*ptr!='\0')
	{
		if (*ptr==toremove){
			while (*ptr!='\0'){
				*ptr=*(ptr+1);
				if (*ptr!='\0') ptr++;
			}
			ptr=string;
		}
		ptr++;
	}
}

char *utf8toconsole(char *string)
{
	char *ptr; 
	ptr = string;
	RemoveChar(string,0xC3);
	ptr = string;
	while (*ptr != '\0') {
		switch (*ptr){
			case 0x87: // Ç
				*ptr = 0x80; 
				break;
			case 0xA7: // E
				*ptr = 0x87; 
				break;
			case 0xA0: // E
				*ptr = 0x85; 
				break;
			case 0xA2: // E
				*ptr = 0x83; 
				break;
			case 0x80: // À
				*ptr = 0x41; 
				break;
			case 0x82: // Â
				*ptr = 0x41; 
				break;
			case 0xAA: // E
				*ptr = 0x88; 
				break;
			case 0xA8: // E
				*ptr = 0x8A; 
				break;
			case 0xA9: // E
				*ptr = 0x82;  
				break;	
			case 0x89: // É
				*ptr = 0x90; 
				break;
			case 0x88: // È
				*ptr = 0x45; 
				break;
			case 0xC5: // Okami
				*ptr = 0x4F; 
				break;
			case 0xB1: // E
				*ptr = 0xA4; 
				break;
			case 0x9F: // ß
				*ptr = 0xE1; 
				break;	
				
		}
		ptr++;
	}
	return string;
}

void FmtGameInfo(char *linebuf, int cols, int size)
{
	*linebuf = 0;
	if (gameinfo.year != 0)
		snprintf(linebuf, size, "%d ", gameinfo.year);
	if (strcmp(gameinfo.publisher,"") != 0)
		snprintf(linebuf, size, "%s%s", linebuf, unescape(gameinfo.publisher, sizeof(gameinfo.publisher)));
	if (strcmp(gameinfo.developer,"") != 0 && strcmp(gameinfo.developer,gameinfo.publisher) != 0)
		snprintf(linebuf, size, "%s / %s", linebuf, unescape(gameinfo.developer, sizeof(gameinfo.developer)));
	if (strlen(linebuf) >= cols) {
		linebuf[cols - 3] = 0;
		strcat(linebuf, "...");
	}
	strappend(linebuf, "\n", size);
	int len = strlen(linebuf);
	linebuf += len;
	size -= len;
		
	if (strcmp(gameinfo.ratingvalue,"") != 0) {
		char rating[8];
		STRCOPY(rating, gameinfo.ratingvalue);
		if (!strcmp(gameinfo.ratingtype,"PEGI")) strcat(rating, "+");
		snprintf(linebuf, size, gt("Rated %s"), rating);
		strcat(linebuf, " ");
	}
	if (gameinfo.players != 0) {
		char players[32];
		if (gameinfo.players > 1) {
			snprintf(D_S(players), gt("for %d players"), gameinfo.players);
		} else {
			strcpy(players, gt("for 1 player"));
		}
		strcat(linebuf, players);
		if (gameinfo.wifiplayers > 0) {
			snprintf(D_S(players), gt("(%d online)"), gameinfo.wifiplayers);
			strcat(linebuf, " ");
			strcat(linebuf, players);
		}
	}
}

void PrintGameInfo()
{
	char linebuf[1000] = "";
	FmtGameInfo(linebuf, 39, sizeof(linebuf));
	printf("%s",linebuf);
}

void PrintGameSynopsis()
{
	char linebuf[1000] = "";
	/*
	printf("   ID: %s\n", gameinfo.id);
	printf("   REGION: %s\n", gameinfo.region);
	printf("   TITLE: %s\n", gameinfo.title);
	printf("   DEV: %s\n", gameinfo.developer);
	printf("   PUB: %s\n", gameinfo.publisher);
	printf("   DATE: %d / %d / %d\n", gameinfo.year, gameinfo.month, gameinfo.day);
	printf("   GEN: %s\n", gameinfo.genre);
	printf("   COLOR: %s\n", gameinfo.caseColor);
	printf("   RAT: %s\n", gameinfo.ratingtype);
	printf("   VAL: %s\n", gameinfo.ratingvalue);
	printf("   RAT DESC: %s\n", gameinfo.ratingdescriptors[0]);
	printf("   WIFI: %d\n", gameinfo.wifiplayers);
	printf("   WIFI DESC: %s\n", gameinfo.wififeatures[0]);
	printf("   PLAY: %d\n", gameinfo.players);
	printf("   ACC: %s\n", gameinfo.accessories[0]);
	printf("   REQ ACC: %s\n", gameinfo.accessoriesReq[0]);
	*/
	//INSERT STUFF HERE
	if (db_debug) {
		mem_stat();
		xml_stat();
	}
	int cols, rows;
	CON_GetMetrics(&cols, &rows);
	cols -= 1;
	rows -= 14;
	if (rows < 2) rows = 2;
	char *syn = gameinfo.synopsis;
	if (!syn) syn = "";
	wordWrap(linebuf, syn, cols, rows, strlen(syn));
	printf("\n%s\n",linebuf);
}

// chartowidechar for UTF-8
// adapted from FreeTypeGX.cpp + UTF-8 fix by Rudolph
wchar_t* charToWideChar(char* strChar) {
	wchar_t *strWChar[strlen(strChar) + 1];
	int	bt;
	bt = mbstowcs(*strWChar, strChar, strlen(strChar));
	if(bt > 0) {
		strWChar[bt] = (wchar_t)'\0';
		return *strWChar;
	}
	
	char *tempSrc = strChar;
	wchar_t *tempDest = *strWChar;
	while((*tempDest++ = *tempSrc++));
	
	return *strWChar;
}

/*
	<game name="Wii Play (USA) (EN,FR,ES)">
		<id>RHAE01</id>
		<type/>
		<region>NTSC-U</region>
		<languages>EN,FR,ES</languages>
		<locale lang="EN">
			<title>Wii Play</title>
			<synopsis>Bundled with a Wii Remote, Wii Play offers a little something for everyone who enjoyed the pick-up-and-play gaming of Wii Sports.

Wii Play is made up of nine games that will test the physical and mental reflexes of players of all ages.

Whether you pick up the Wii Remote to play the shooting gallery that harkens back to the days of Duck Hunt or use it to find matching Miis, a world of fun is in your hands!

In addition to the shooting gallery and Mii-matching game, Wii play offers billiards, air hockey, tank battles, table tennis rally, Mii poses and a cow-riding race.</synopsis>
		</locale>
		<locale lang="ZHTW">
			<title>Wiiç¬¬ä¸€æ¬¡æŽ¥è§¸(ç¾Ž)</title>
			<synopsis/>
		</locale>
		<locale lang="ZHCN">
			<title>Wiiç¬¬ä¸€æ¬¡æŽ¥è§¦(ç¾Ž)</title>
			<synopsis/>
		</locale>
		<developer>Nintendo EAD</developer>
		<publisher>Nintendo</publisher>
		<date year="2007" month="2" day="12"/>
		<genre>miscellaneous</genre>
		<rating type="ESRB" value="E">
			<descriptor>mild cartoon violence</descriptor>
		</rating>
		<wi-fi players="0"/>
		<input players="2">
			<control type="wiimote" required="true"/>
			<control type="nunchuk" required="true"/>
		</input>
		<rom version="" name="Wii Play (USA) (EN,FR,ES).iso" size="4699979776" crc="ffbe5d7f" md5="ac33937e2e14673f7607b6889473089e" sha1="cf734f2c97292bc4e451bf263b217aa35599062e"/>
	</game>
*/

