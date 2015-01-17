
// by oggzee

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <unistd.h>
#include <sys/stat.h>
#define __LINUX_ERRNO_EXTENSIONS__ // for EBADFD
#include <sys/errno.h>

#include "disc.h"
#include "gui.h"
#include "wpad.h"
#include "cfg.h"
#include "http.h"
#include "dns.h"
#include "video.h"
#include "net.h"
#include "xml.h" /* XML - Lustar */
#include "menu.h"
#include "gettext.h"
#include "fat.h"
#include "cache.h"
#include "unzip/unzip.h"
#include "unzip/miniunz.h"
#include "fileOps.h"

extern struct discHdr *gameList;
extern s32 gameCnt, gameSelected, gameStart;
extern bool imageNotFound;

static int net_top = -1;
static int dl_abort = 0;

/*Networking - Forsaekn*/
int Net_Init(char *ip){
	
	s32 result;

	int count = 0;
	while ((result = net_init()) == -EAGAIN && count < 100)	{
		printf(".");
		fflush(stdout);
		usleep(50 * 1000); //50ms
		count++;
	}

	if (result >= 0)
	{
		char myIP[16];

		net_top = result;
		if (if_config(myIP, NULL, NULL, true) < 0)
		{
			printf("Error reading IP address.");
			return false;
		}
		else
		{
			return true;
		}
	}
	return false;
}

/* Initialize Network */
bool Init_Net()
{
	static bool firstTimeDownload = true;

	if(firstTimeDownload == true){
		char myIP[16];
		printf_x(gt("Initializing Network..."));
		printf("\n");
		if( !Net_Init(myIP) ){
			printf_(gt("Error Initializing Network."));
			printf("\n");
			return false;
		}
		printf_(gt("Network connection established."));
		printf("\n");
		firstTimeDownload = false;
	}
	__console_flush(0);
	return true;
}


#ifdef DEBUG_NET
#define debug_printf(fmt, args...) \
	fprintf(stderr, "%s:%d:" fmt, __FUNCTION__, __LINE__, ##args)
#else
#define debug_printf(fmt, args...)
#endif // DEBUG_NET

#define STACK_ALIGN(type, name, cnt, alignment)         u8 _al__##name[((sizeof(type)*(cnt)) + (alignment) + (((sizeof(type)*(cnt))%(alignment)) > 0 ? ((alignment) - ((sizeof(type)*(cnt))%(alignment))) : 0))]; \
	type *name = (type*)(((u32)(_al__##name)) + ((alignment) - (((u32)(_al__##name))&((alignment)-1))))

#define IOCTL_NWC24_CLEANUP 0x07

#define NET_UNKNOWN_ERROR_OFFSET	-10000

static char __kd_fs[] ATTRIBUTE_ALIGN(32) = "/dev/net/kd/request";

// 0 means we don't know what this error code means
// I sense a pattern here...
static u8 _net_error_code_map[] = {
	0, // 0
	0, 
	0, 
	0,
	0, 
	0, // 5
	EAGAIN, 
	EALREADY,
	EBADFD,
	0,
	0, // 10
	0,
	0,
	0,
	0,
	0, // 15
	0,
	0,
	0,
	0,
	0, // 20
	0,
	0,
	0,
	0,
	0, // 25
	EINPROGRESS,
	0,
	0,
	0,
	EISCONN, // 30
	0,
	0,
	0,
	0,
	0, // 35
	0,
	0,
	0,
	ENETDOWN, //?
	0, // 40
	0,
	0,
	0,
	0,
	0, // 45
	0,
	0,
	0,
	0,
	0, // 50
	0,
	0,
	0,
	0,
	0, // 55
	0,
	0,
	0,
	0,
	0, // 60
};

static s32 _net_convert_error(s32 ios_retval)
{
//	return ios_retval;
	if (ios_retval >= 0) return ios_retval;
	if (ios_retval < -sizeof(_net_error_code_map)
		|| !_net_error_code_map[-ios_retval])
			return NET_UNKNOWN_ERROR_OFFSET + ios_retval;
	return -_net_error_code_map[-ios_retval];
}

// by oggzee
static s32 NWC24iCleanupSocket(void)
{
	s32 kd_fd, ret;
	STACK_ALIGN(u8, kd_buf, 0x20, 32);
	
	kd_fd = _net_convert_error(IOS_Open(__kd_fs, 0));
	if (kd_fd < 0) {
		debug_printf(gt("IOS_Open(%s) failed with code %d"), __kd_fs, kd_fd);
		printf("\n");
		return kd_fd;
	}
  
	ret = _net_convert_error(IOS_Ioctl(kd_fd, IOCTL_NWC24_CLEANUP, NULL, 0, kd_buf, 0x20));
	if (ret < 0) debug_printf("IOS_Ioctl(7)=%d\n", ret);
  	IOS_Close(kd_fd);
  	return ret;
}

void Net_Close(int close_wc24)
{
	if (net_top >= 0) {
		if (close_wc24) {
			NWC24iCleanupSocket();
			IOS_Close(net_top);
		}
		net_top = -1;
	}
}

/*

Channel regions:

A 	41 	All regions. System channels like the Mii channel use it.
D 	44 	German-speaking regions. Only if separate versions exist, e.g. Zelda: A Link to the Past
E 	45 	USA and other NTSC regions except Japan
F 	46 	French-speaking regions. Only if separate versions exist, e.g. Zelda: A Link to the Past.
J 	4A 	Japan
K 	4B 	Korea
L 	4C 	Japanese Import to Europe, Australia and other PAL regions
M 	4D 	American Import to Europe, Australia and other PAL regions
N 	4E 	Japanese Import to USA and other NTSC regions
P 	50 	Europe, Australia and other PAL regions
Q 	51 	Korea with Japanese language.
T 	54 	Korea with English language.
X 	58 	Not a real region code. Used by DVDX and Homebrew Channel. 

Game regions:

H	Netherlands (quite rare)
I	Italy
S	Spain
X	PAL (not sure if it is for a specific country,
	but it was confirmed to be used on disc that only had French)
Y	PAL, probably the same as for X

*/   

char *gameid_to_cc(char *gameid)
{
	// language codes:
	//char *EN = "EN";
	char *US = "US";
	char *FR = "FR";
	char *DE = "DE";
	char *JA = "JA";
	char *KO = "KO";
	char *NL = "NL";
	char *IT = "IT";
	char *ES = "ES";
	char *ZH = "ZH";
	char *PAL = "PAL";
	char *OTHER = "other";
	// gameid regions:
	switch (gameid[3]) {
		// channel regions:
		case 'A': return PAL;// All regions. System channels like the Mii channel use it.
		case 'C': return US;
		case 'D': return DE; // German-speaking regions.
		case 'E': return US; // USA and other NTSC regions except Japan
		case 'F': return FR; // French-speaking regions.
		case 'J': return JA; // Japan
		case 'K': return KO; // Korea
		case 'L': return PAL;// Japanese Import to Europe, Australia and other PAL regions
		case 'M': return PAL;// American Import to Europe, Australia and other PAL regions
		case 'N': return US; // Japanese Import to USA and other NTSC regions
		case 'P': return PAL;// Europe, Australia and other PAL regions
		case 'Q': return KO; // Korea with Japanese language.
		case 'T': return KO; // Korea with English language.
		// game regions:
		case 'H': return NL; // Netherlands
		case 'I': return IT; // Italy
		case 'S': return ES; // Spain
		case 'X': return PAL;
		case 'Y': return PAL;
		case 'W': return ZH; // Taiwan
	}
	return OTHER;
}

char *lang_to_cc()
{
	//printf("lang: %d\n", CONF_GetLanguage());
	switch(CONF_GetLanguage()){
		case CONF_LANG_JAPANESE: return "JA";
		case CONF_LANG_ENGLISH:  return "EN";
		case CONF_LANG_GERMAN:   return "DE";
		case CONF_LANG_FRENCH:   return "FR";
		case CONF_LANG_SPANISH:  return "ES";
		case CONF_LANG_ITALIAN:  return "IT";
		case CONF_LANG_DUTCH:    return "NL";
		case CONF_LANG_KOREAN:   return "KO";
		case CONF_LANG_SIMP_CHINESE: return "ZHCN";
		case CONF_LANG_TRAD_CHINESE: return "ZHTW";
	}
	return "EN";
}

char *auto_cc()
{
	//printf("area: %d\n", CONF_GetArea());
	switch(CONF_GetArea()){
		case CONF_AREA_AUS: return "AU";
		case CONF_AREA_BRA: return "PT";
	}
	return lang_to_cc();
}

char *get_cc()
{
	if (*CFG.download_cc_pal) {
		return CFG.download_cc_pal;
	} else {
		return auto_cc();
	}
}

static bool fmt_cc_pal = false;

void format_URL(char *game_id, char *url, int size)
{
	// widescreen
	int width, height;
	width = CFG.N_COVER_WIDTH;
	height = CFG.N_COVER_HEIGHT;
	// region
	char region[10];
	switch(game_id[3]){
	case 'E':
		sprintf(region,"ntsc");
		break;
	case 'J':
		sprintf(region,"ntscj");
		break;
	default:
	case 'P':
		sprintf(region,"pal");
		break;
	}
	// country code
	fmt_cc_pal = false;
	char *cc = gameid_to_cc(game_id);
	if (strcmp(cc, "PAL") == 0) {
		cc = get_cc();
		if (strcmp(cc, "EN") != 0) {
			fmt_cc_pal = true;
		}
	}
	// set URL
	char id[8];
	char tmps[15];
	STRCOPY(id, game_id);
	str_replace(url, "{REGION}", region, size);
	if (str_replace(url, "{CC}", cc, size) == false) fmt_cc_pal = false;
	id[6] = 0;
	str_replace(url, "{PUB}", id+4, size);
	id[6] = 0;
	str_replace(url, "{ID6}", id, size);
	id[4] = 0;
	str_replace(url, "{ID4}", id, size);
	id[3] = 0;
	str_replace(url, "{ID3}", id, size);
	sprintf(tmps, "%d", width);
	str_replace(url, "{WIDTH}", tmps, size);
	sprintf(tmps, "%d", height);
	str_replace(url, "{HEIGHT}", tmps, size);

	dbg_printf("URL: %s\n", url);
}

struct block Download_URL(char *id, char *url_list, int style)
{
	struct block file;
	char *next_url;
	char url_fmt[200];
	char url[200];
	int retry_cnt = 0;
	int id_cnt;
	int pal_cnt;

	next_url = url_list;

	retry_url:

	next_url = split_token(url_fmt, next_url, ' ', sizeof(url_fmt));

	pal_cnt = 0;

	retry_cc_pal:

	id_cnt = 3;

	retry_id:

	if (retry_cnt > 0) {
		printf_(gt("Trying (url#%d) ..."), retry_cnt+1);
		printf("\n");
	}

	STRCOPY(url, url_fmt);

	if (pal_cnt) {
		str_replace(url, "{CC}", "EN", sizeof(url));
	}

	if (strstr(url, "{ID}")) {
		// {ID} auto retries with 6,4,3
		switch (id_cnt) {
		case 3:
			str_replace(url, "{ID}", "{ID6}", sizeof(url));
			break;
		case 2:
			str_replace(url, "{ID}", "{ID4}", sizeof(url));
			break;
		case 1:
			str_replace(url, "{ID}", "{ID3}", sizeof(url));
			break;
		}
		id_cnt--;
	} else {
		id_cnt = 0;
	}

	format_URL(id, url, sizeof(url));

	if (url[0] == 0) {
		printf_(gt("Error: no URL."));
		printf("\n");
		goto dl_err;
	}

	printf_("%.35s\n", url);
	if (strlen(url) > 35) {
		printf_("%.35s\n", url+35);
	}
	printf_("[");
	__console_flush(0);
	int chunk = 16;
	if (style == CFG_COVER_STYLE_FULL) chunk = 32;
	if (style == CFG_COVER_STYLE_HQ) chunk = 96;
	file = downloadfile_progress(url, chunk);
	printf("]");
	__console_flush(0);
	
	if (file.data == NULL || file.size == 0) {
		//printf("%s\n", url);
		printf("\n");
		printf_(gt("Error: no data."));
		printf("\n");
		goto dl_err;
	}
	//printf(gt("Size: %d "), file.size);
	printf(" %d", file.size);

	// verify if valid png
	s32 ret;
	u32 w, h;
	ret = __Gui_GetPngDimensions(file.data, &w, &h);
	if (ret) {
		//printf("\n%s\n", url);
		printf("\n");
		printf_(gt("Error: Invalid PNG image!"));
		printf("\n");
		SAFE_FREE(file.data);
		goto dl_err;
	}

	// success
	goto out;

	// error
	dl_err:
	retry_cnt++;
	if (id_cnt > 0) goto retry_id;
	if (fmt_cc_pal && pal_cnt == 0) {
		pal_cnt++;
		goto retry_cc_pal;
	}
	if (next_url) goto retry_url;
	file.data = NULL;
	file.size = 0;
	out:
	if (retry_cnt > 0) {
		//sleep(2);
		if (CFG.debug) { printf("Press any button.\n"); Wpad_WaitButtons(); }
	}
	__console_flush(0);
		
	return file;
}

const char *get_style_name(int style)
{
	switch (style) {
		case CFG_COVER_STYLE_2D:   return gt("FLAT cover");
		case CFG_COVER_STYLE_3D:   return gt("3D cover");
		case CFG_COVER_STYLE_DISC: return gt("DISC cover");
		case CFG_COVER_STYLE_FULL: return gt("FULL cover");
		case CFG_COVER_STYLE_HQ:   return gt("HQ cover");
	}
	return "?";
}

char* get_cover_url(int style)
{
	switch (style) {
		default:
		case CFG_COVER_STYLE_2D:
			return CFG.cover_url_2d;

		case CFG_COVER_STYLE_3D:
			return CFG.cover_url_3d;

		case CFG_COVER_STYLE_DISC:
			return CFG.cover_url_disc;

		case CFG_COVER_STYLE_FULL:
			return CFG.cover_url_full;

		case CFG_COVER_STYLE_HQ:
			return CFG.cover_url_hq;
	}
	return "";
}

bool Download_Cover_Style(char *id, int style)
{
	char imgName[100];
	char imgPath[100];
	char *path = "";
	char *url = "";
	char *wide = "";
	bool success = false;
	struct stat st;

	url = get_cover_url(style);
	if (strlen(url) < 10) return false;

	path = cfg_get_covers_path(style);
	if (style == CFG_COVER_STYLE_2D && path == CFG.covers_path) {
		if (stat(CFG.covers_path_2d, &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				path = CFG.covers_path_2d;
			}
		}
	}

	if (CFG.download_id_len == 6) {
		// save as 6 character ID
		snprintf(imgName, sizeof(imgName), "%.6s%s.png", id, wide);
	} else {
		// save as 4 character ID
		snprintf(imgName, sizeof(imgName), "%.4s%s.png", id, wide);
	}
	printf("[%.6s] : %s", id, get_style_name(style));
	printf("\n");

	if (strncmp(path, NTFS_DRIVE, strlen(NTFS_DRIVE)) == 0) {
		if (CFG.ntfs_write == 0) {
			void print_ntfs_write_error();
			print_ntfs_write_error();
			dl_abort = 1;
			goto dl_err;
		}
	}
	
	snprintf(imgPath, sizeof(imgPath), "%s/%s", path, imgName);

	// try to download image
	struct block file;
	file = Download_URL(id, url, style);
	if (file.data == NULL) {
		goto dl_err;
	}

	// check path access
	char drive_root[8];
	snprintf(drive_root, sizeof(drive_root), "%s/", FAT_DRIVE);
	if (stat(drive_root, &st)) {
		printf_(gt("ERROR: %s is not accessible"), drive_root);
		printf("\n");
		goto dl_err;
	}
	if (stat(path, &st)) {
		if (mkpath(path, 0777)) {
			printf_(gt("Cannot create dir: %s"), path);
			printf("\n");
			goto dl_err;
		}
	}
		
	// save png to sd card for future use
	FILE *f;
	f = fopen(imgPath, "wb");
	if (!f) {
		printf("\n");
		printf_(gt("ERROR: creating: %s"), imgPath);
		printf("\n");
		goto dl_err;
	}
	int ret = fwrite(file.data,file.size,1,f);
	fclose (f);
	SAFE_FREE(file.data);
	Cache_Invalidate_Cover((u8*)id, style);
	if (ret != 1) {
		printf(" ");
		printf(gt("ERROR: writing %s (%d)."), imgPath, ret);
		printf("\n");
		remove(imgPath);
	} else {
		printf(" ");
		//printf(gt("Download complete."));
		printf(gt("OK"));
		printf("\n\n");
		success = true;
	}
	__console_flush(0);

	//refresh = true;				
	//sleep(3);
	return success;

	dl_err:
	__console_flush(0);
	sleep(2);
	if (CFG.debug) { printf(gt("Press any button...")); printf("\n"); Wpad_WaitButtons(); }
	return false;
}

bool Download_Cover_Missing(char *id, int style, bool missing_only, bool verbose)
{
	int ret;
	void *data = NULL;
	char path[200];

	if (!id) return false;

	if (missing_only) {
		// check if missing
		ret = Gui_LoadCover_style((u8*)id, &data, false, style, path);
		if (ret > 0 && data) {
			// found. verify if valid png
			u32 width, height;
			ret = __Gui_GetPngDimensions(data, &width, &height);
			if (ret == 0 && width > 0 && height > 0) {
				GRRLIB_texImg tx_tmp = Gui_LoadTexture_RGBA8(data, width, height, NULL, path);
				bool found = false;
				if (tx_tmp.data) found = true;
				SAFE_FREE(data);
				SAFE_FREE(tx_tmp.data);
				if (style == CFG_COVER_STYLE_HQ) {
					if (width <= 512) found = false;
				}
				if (found) {
					if (verbose) {
						printf_(gt("Found %s"), get_style_name(style));
						printf("\n");
					}
					return true;
				}
			}
		}
	}
	return Download_Cover_Style(id, style);
}

void Download_Cover(char *id, bool missing_only, bool verbose)
{
	if (!id)
		return;

	if (strlen(id) < 4) {
		printf(gt("ERROR: Invalid Game ID"));
		printf("\n");
		goto dl_err;
	}

	if (verbose) {
		if (missing_only) {
			printf_x(gt("Downloading MISSING covers for %.6s"), id);
		} else {
			printf_x(gt("Downloading ALL covers for %.6s"), id);
		}
		printf("\n");
	}

	//the first time no image is found, attempt to init network
	if(!Init_Net()) goto dl_err;
	if (verbose) printf("\n");
	dl_abort = 0;

	if (CFG.download_all) {
		Download_Cover_Missing(id, CFG_COVER_STYLE_2D, missing_only, verbose);
		Download_Cover_Missing(id, CFG_COVER_STYLE_3D, missing_only, verbose);
		Download_Cover_Missing(id, CFG_COVER_STYLE_DISC, missing_only, verbose);
		if (!Download_Cover_Missing(id, CFG_COVER_STYLE_HQ, missing_only, verbose)) {
			Download_Cover_Missing(id, CFG_COVER_STYLE_FULL, missing_only, verbose);
		}
	} else {
		if (gui_style == GUI_STYLE_COVERFLOW) {
			// if in a coverflow GUI mode, download full covers.
			// If full cover not found, download 2D.
			if (!Download_Cover_Missing(id, CFG_COVER_STYLE_HQ, missing_only, verbose)) {
				if (!Download_Cover_Missing(id, CFG_COVER_STYLE_FULL, missing_only, verbose)) {
					Download_Cover_Missing(id, CFG_COVER_STYLE_2D, missing_only, verbose);
				}
			}
		} else {
			Download_Cover_Missing(id, CFG.cover_style, missing_only, verbose);
		}
	}
	if (dl_abort) goto dl_err;

	return;
	dl_err:
	sleep(3);
}

void Download_All_Covers(bool missing_only)
{
	int i;
	struct discHdr *header;
	u32 buttons;
	printf("\n");
	if (missing_only) {
		printf_x(gt("Downloading ALL MISSING covers"));
	} else {
		printf_x(gt("Downloading ALL covers"));
	}
	printf("\n\n");
	printf_(gt("Hold button B to cancel."));
	printf("\n\n");
	//dbg_time1();
	for (i=0; i<gameCnt; i++) {
		buttons = Wpad_GetButtons();
		if ((buttons & CFG.button_cancel.mask) || dl_abort) {
			printf("\n");
			printf_(gt("Cancelled."));
			printf("\n");
			sleep(1);
			return;
		}
		header = &gameList[i];
		printf("%d / %d %.4s %.25s\n", i+1, gameCnt, header->id, get_title(header));
		__console_flush(0);
		Gui_DrawCover(header->id);
		Download_Cover((char*)header->id, missing_only, false);
		Gui_DrawCover(header->id);
	}
	printf("\n");
	printf_(gt("Done."));
	printf("\n");
	//unsigned ms = dbg_time2(NULL);
	//printf_("Time: %.2f seconds\n", (float)ms/1000); Wpad_WaitButtons();
	sleep(1);
}


/* download zipped XML - Lustar */
/* based on Download_Cover by Forsaeken, modified by oggzee */
void Download_XML()
{

	printf("\n");
	if(!Init_Net()) goto dl_err;

	struct stat st;
	char drive_root[8];
	snprintf(drive_root, sizeof(drive_root), "%s/", FAT_DRIVE);
	if (stat(drive_root, &st)) {
		printf_(gt("ERROR: %s is not accessible"), drive_root);
		printf("\n");
		goto dl_err;
	}

	char zippath[200];
	snprintf(zippath, sizeof(zippath), "%s/%s", USBLOADER_PATH, "wiitdb_new.zip");
	char zipurl[100];
	extern char *get_cc();
	char *cc = get_cc();
	char * dbl = ((strlen(CFG.db_language) == 2) ? VerifyLangCode(CFG.db_language) : ConvertLangTextToCode(CFG.db_language));
	strcopy(zipurl, CFG.db_url, sizeof(zipurl));
	str_replace(zipurl, "{CC}", cc, sizeof(zipurl));
	str_replace(zipurl, "{DBL}", dbl, sizeof(zipurl));
	
	if (zipurl[0] == 0) {
		printf_(gt("Error: no URL."));
		printf("\n");
		goto dl_err;	
	}
	printf_x(gt("Downloading database."));
	printf("\n");
	printf_("%s\n", zipurl);
//goto skip_download;
	printf_("[.");
	struct block file = downloadfile_progress(zipurl, 64);
	printf("]\n");
	
	if (file.data == NULL) {
		printf_(gt("Error: no data."));
		printf("\n");
		goto dl_err;
	}
	printf_(gt("Size: %d bytes"), file.size);
	printf("\n");
	
	FILE *f;
	f = fopen(zippath, "wb");
	if (!f) {
		printf("\n");
		printf_(gt("Error opening: %s"), zippath);
		printf("\n");
		goto dl_err;
	}
	fwrite(file.data,1,file.size,f);
	fclose (f);
	SAFE_FREE(file.data);
	printf_(gt("Download complete."));
	printf("\n");

	/* try to open newly downloaded zipped XML */
	printf_x(gt("Updating database."));
	printf("\n");

	//FreeXMLMemory();
	char currentzippath[200];
	snprintf(currentzippath, sizeof(currentzippath), "%s/wiitdb.zip", USBLOADER_PATH);
	char tmppath[200];
	snprintf(tmppath, sizeof(tmppath), "%s/wiitdb_old.zip", USBLOADER_PATH);
	remove(tmppath);
	rename(currentzippath, tmppath);
	rename(zippath, currentzippath);
//skip_download:
	if (ReloadXMLDatabase(USBLOADER_PATH, CFG.db_language, 0)) {
		printf_(gt("Database update successful."));
		printf("\n");
	} else {
		// revert to the previous file
// TODO File is to big to load on the fly need to free up 1 meg in mem2
// their is enough memory to load it on startup
// and it already displays a message telling user to restart to use the new file
//		remove(currentzippath);
//		rename(tmppath, currentzippath);
//		ReloadXMLDatabase(USBLOADER_PATH, CFG.db_language, 0);
		printf_(gt("Error opening database, update did not complete."));
		printf("\n");
		goto dl_err;
	}
	
	__console_flush(0);
	sleep(2);
	return;
	
	dl_err:
	sleep(4);
} /* end download zipped xml */

/* download zipped Devolution - Lustar */
/* based on Download_Cover by Forsaeken, modified by oggzee */
void Download_Plugins()
{

	printf("\n");
	if(!Init_Net()) goto dl_err;

	struct stat st;
	char drive_root[8];
	snprintf(drive_root, sizeof(drive_root), "%s/", FAT_DRIVE);
	if (stat(drive_root, &st)) {
		printf_(gt("ERROR: %s is not accessible"), drive_root);
		printf("\n");
		goto dl_err;
	}

	char zippath[200];
	snprintf(zippath, sizeof(zippath), "%s/%s", USBLOADER_PATH, "devolution.zip");
	remove(zippath);
	char zipurl[100];
	strcopy(zipurl, "http://www.tueidj.net/gc_devo_src.zip", sizeof(zipurl));
	if (zipurl[0] == 0) {
		printf_(gt("Error: no URL."));
		printf("\n");
		goto dl_err;	
	}
	
	printf_x(gt("Downloading devolution."));
	printf("\n");
	printf_("%s\n", zipurl);

	printf_("[.");
	struct block file = downloadfile_progress(zipurl, 64);
	printf("]\n");
	
	if (file.data == NULL) {
		printf_(gt("Error: no data."));
		printf("\n");
		goto dl_err;
	}
	printf_(gt("Size: %d bytes"), file.size);
	printf("\n");
	
	FILE *f;
	f = fopen(zippath, "wb");
	if (!f) {
		printf("\n");
		printf_(gt("Error opening: %s"), zippath);
		printf("\n");
		goto dl_err;
	}
	fwrite(file.data,1,file.size,f);
	fclose (f);
	SAFE_FREE(file.data);
	printf_(gt("Download complete."));
	printf("\n");

	/* try to open newly downloaded zipped XML */
	printf_x(gt("Updating devolution"));
	printf("\n");

	////////////////////////
	unzFile unzfile = unzOpen(zippath);
	if (unzfile == NULL) {
		printf_(gt("Error opening: %s"), zippath);
		printf("\n");
		goto dl_err;
	}
	
	u8 *buffer = NULL;
	//unzOpenCurrentFile(unzfile);
	unz_file_info zipfileinfo;
	unzLocateFile(unzfile, "gc_devo/data/loader.bin", 0);
	unzOpenCurrentFile(unzfile);
	unzGetCurrentFileInfo(unzfile, &zipfileinfo, NULL, 0, NULL, 0, NULL, 0);	
	int zipfilebuffersize = zipfileinfo.uncompressed_size;
	printf_(gt("loader.bin size: %d"), zipfilebuffersize);
	printf("\n");

	buffer = mem_calloc(zipfilebuffersize);
	if (buffer == NULL) {
		unzCloseCurrentFile(unzfile);
		unzClose(unzfile);
		printf_(gt("Error allocating buffer: %d"), zipfilebuffersize);
		printf("\n");
		goto dl_err;
	}
	
	unzReadCurrentFile(unzfile, buffer, zipfilebuffersize);
	unzCloseCurrentFile(unzfile);
	unzClose(unzfile);
	
	char devopath[200];
	snprintf(devopath, sizeof(zippath), "%s/%s", USBLOADER_PATH, "loader.bin");
	remove(devopath);
	
	f = fopen(devopath, "wb");
	if (!f) {
		printf("\n");
		printf_(gt("Error opening: %s"), devopath);
		printf("\n");
		goto dl_err;
	}
	printf_((char*)buffer+4);
	fwrite(buffer,1,zipfilebuffersize,f);
	fclose(f);
	memset(buffer, 0, zipfilebuffersize);
	SAFE_FREE(buffer);
	///////////////////////
	
	/* Download Mighty Plugin */
	char mightyPath[200];
	snprintf(mightyPath, sizeof(mightyPath), "%s/%s", USBLOADER_PATH, "/plugins");
	if (!fsop_DirExist(mightyPath)) mkpath(mightyPath, 0777);
	strcat(mightyPath, "/mighty.dol");
	remove(mightyPath);
	char url[255];
	strcopy(url, "http://cfg-loader-mod.googlecode.com/files/mighty.dol", sizeof(url));
	if (url[0] == 0) {
		printf_(gt("Error: no URL."));
		printf("\n");
		goto dl_err;	
	}
	
	printf_x(gt("Downloading mighty plugin."));
	printf("\n");
	printf_("%s\n", url);

	printf_("[.");
	file = downloadfile_progress(url, 64);
	printf("]\n");
	
	f = fopen(mightyPath, "wb");
	if (!f) {
		printf("\n");
		printf_(gt("Error opening: %s"), mightyPath);
		printf("\n");
		goto dl_err;
	}
	fwrite(file.data,1,file.size,f);
	fclose(f);
	SAFE_FREE(file.data);
	printf_(gt("Download complete."));
	printf("\n");
	
	
	///////////////////////
	
	/* Download Neek2o Plugin */
	char neekPath[200];
	snprintf(neekPath, sizeof(neekPath), "%s/%s", USBLOADER_PATH, "/plugins");
	if (!fsop_DirExist(neekPath)) mkpath(neekPath, 0777);
	strcat(neekPath, "/neek2o.dol");
	remove(neekPath);
	strcopy(url, "http://cfg-loader-mod.googlecode.com/files/neek2o.dol", sizeof(url));
	if (url[0] == 0) {
		printf_(gt("Error: no URL."));
		printf("\n");
		goto dl_err;	
	}
	
	printf_x(gt("Downloading neek2o plugin."));
	printf("\n");
	printf_("%s\n", url);

	printf_("[.");
	file = downloadfile_progress(url, 64);
	printf("]\n");
	
	f = fopen(neekPath, "wb");
	if (!f) {
		printf("\n");
		printf_(gt("Error opening: %s"), neekPath);
		printf("\n");
		goto dl_err;
	}
	fwrite(file.data,1,file.size,f);
	fclose(f);
	SAFE_FREE(file.data);
	printf_(gt("Download complete."));
	printf("\n");
	
	
	///////////////////////
	__console_flush(0);
	printf_x(gt("Press any button.\n")); 
	Wpad_WaitButtons();
	return;
	
	dl_err:
	sleep(4);
} /* end download zipped devolution */

void Download_Translation()
{
	char destPath[200];
	char url[255];
	struct block file;
	FILE *f = NULL;
	file.data = NULL;

	printf("\n");
	if(!Init_Net()) goto dl_err;

	///////////////////////
	
	/* Download unifont */
	snprintf(destPath, sizeof(destPath), "%s", USBLOADER_PATH);
	if (!fsop_DirExist(destPath)) mkpath(destPath, 0777);
	strcat(destPath, "/unifont.dat");
	if ((!CFG.load_unifont) || fsop_FileExist(destPath))	//if unifont not used or already exists
		goto skip_unifont;									//dont need to download it
	strcopy(url, "http://cfg-loader-mod.googlecode.com/svn/trunk/tools/unifont.dat", sizeof(url));
	if (url[0] == 0) {
		printf_(gt("Error: no URL."));
		printf("\n");
		goto dl_err;	
	}
	
	printf_x(gt("Downloading unifont."));
	printf("\n");
	printf_("%s\n", url);

	printf_("[.");
	file = downloadfile_progress(url, 64);
	printf("]\n");
	
	if (file.data == NULL || file.size < 1300000) {
		goto dl_err;
	}

	remove(destPath);
	
	f = fopen(destPath, "wb");
	if (!f) {
		printf("\n");
		printf_(gt("Error opening: %s"), destPath);
		printf("\n");
		goto dl_err;
	}
	fwrite(file.data,1,file.size,f);
	fclose(f);
	SAFE_FREE(file.data);
	printf_(gt("Download complete."));
	printf("\n");
	
	skip_unifont:
	///////////////////////
	
	/* Download language file */
	if (strcmp(CFG.translation, "EN") == 0)		// if english
		goto skip_language;						// no file to download
	snprintf(destPath, sizeof(destPath), "%s/%s", USBLOADER_PATH, "/languages");
	if (!fsop_DirExist(destPath)) mkpath(destPath, 0777);
	snprintf(destPath, sizeof(destPath), "%s/%s.lang", destPath, CFG.translation);
	snprintf(url, sizeof(url), "http://cfg-loader-mod.googlecode.com/svn/trunk/Languages/%s.lang", CFG.translation);
	if (url[0] == 0) {
		printf_(gt("Error: no URL."));
		printf("\n");
		goto dl_err;	
	}
	
	printf_x(gt("Downloading translation file."));
	printf("\n");
	printf_("%s\n", url);

	printf_("[.");
	file = downloadfile_progress(url, 64);
	printf("]\n");
	
	size_t old_file_size;
	fsop_GetFileSizeBytes (destPath, &old_file_size);
	if (file.data == NULL || file.size < 30000 || old_file_size > file.size) {
		goto dl_err;
	}

	// backup old language file
	char bak_name[200];
	strcpy(bak_name, destPath);
	strcat(bak_name, ".bak");
	// remove old backup
	remove(bak_name);
	// rename current to backup
	rename(destPath, bak_name);
	
	f = fopen(destPath, "wb");
	if (!f) {
		printf("\n");
		printf_(gt("Error opening: %s"), destPath);
		printf("\n");
		goto dl_err;
	}
	fwrite(file.data,1,file.size,f);
	fclose(f);
	SAFE_FREE(file.data);
	printf_(gt("Download complete."));
	printf("\n");
	
	skip_language:
	///////////////////////
	
//	__console_flush(0);
//	printf_x(gt("Press any button.\n")); 
//	Wpad_WaitButtons();
	return;
	
	dl_err:
	SAFE_FREE(file.data);
	sleep(4);
}

int gamercard_enabled = 1;

int gamercard_update(char *ID)
{
	if (!gamercard_enabled) return 0;
	char *next_key, *next_url;
	bool net_initted = false;
	int gcard_cnt = 0;
	int result = 0;
	next_key = CFG.gamercard_key;
	next_url = CFG.gamercard_url;
	if (*next_key == 0 || *next_url == 0)
		return 0;
	while(next_key && next_url) {
		char key[80];
		char url[200];
		next_key = split_token(key, next_key, ' ', sizeof(key));
		next_url = split_token(url, next_url, ' ', sizeof(url));
		gcard_cnt++;
		if (strcmp(key, "0") == 0) {
			continue;
		}
		
		if(!net_initted && !Init_Net()) {
			printf_x(gt("Network error. Can't update gamercards."));
			printf("\n");
			return -1;
		} else 
			net_initted = true;

		str_replace(url, "{KEY}", key, sizeof(url));
		str_replace(url, "{ID6}", ID, sizeof(url));
	
		struct block file;
		file = downloadfile(url);
		if(file.data == NULL || file.size == 0) {
			printf_x(gt("Download error on gamercard #%d."), gcard_cnt);
			printf("\n");
			result = -2;
		} else {
			printf_x(gt("Gamercard #%d reported: %.*s"), gcard_cnt, file.size,file.data);
			printf("\n");
		}
	}
	return result;
}
