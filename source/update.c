
// by oggzee and Dr. Clipper
// theme previews added by usptactical

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <ctype.h>
#include <wctype.h>
#include <wchar.h>
#include <locale.h>
#include <asndlib.h>

#include "disc.h"
#include "fat.h"
#include "cache.h"
#include "gui.h"
#include "menu.h"
#include "restart.h"
#include "sys.h"
#include "util.h"
#include "utils.h"
#include "video.h"
#include "wbfs.h"
#include "wpad.h"
#include "patchcode.h"
#include "cfg.h"
#include "http.h"
#include "dns.h"
#include "wdvd.h"
#include "music.h"
#include "subsystem.h"
#include "net.h"
#include "gettext.h"
#include "xml.h"
#include "unzip/unzip.h"

////////////////////////////////////////
//
//  Updates
//
////////////////////////////////////////

#define MAX_UP_DESC 6

struct UpdateInfo
{
	char version[32];
	char url[100];
	int size;
	char date[16]; // 2009-12-30
	char desc[MAX_UP_DESC][40];
	int ndesc;
};

struct ThemeInfo
{
	int ID;
	char name[32];
	char url[100];
	char thumbpath[200];
	char creator[32];
	int version;
	int downloads;
	int votes;
	int rating;
	int adult;
	int updated;
	int type;
};

#define THEME_TYPE_NEW_THEME			0
#define THEME_TYPE_UPDATES_AVAILABLE	1
#define THEME_TYPE_UP_TO_DATE			2

#define MAX_UPDATES 20
int num_updates = 0;
int num_themes = 0;
int num_new_themes, num_theme_updates;

struct UpdateInfo updates[MAX_UPDATES];
char meta_xml[4096];

struct ThemeInfo *themes = NULL;
int themeIdx;
size_t theme_updated;

bool parsebuf_line(char *buf, int (*line_func)(char*))
{
	char line[200];
	char *p, *nl;
	int len, ret;

	nl = buf;
	for (;;) {
		p = nl;
		if (p == NULL) break;
		while (*p == '\n') p++;
		if (*p == 0) break;
		nl = strchr(p, '\n');
		if (nl == NULL) {
			len = strlen(p);
		} else {
			len = nl - p;
		}
		if (len >= sizeof(line)) len = sizeof(line) - 1;
		strcopy(line, p, len+1);
		ret = line_func(line);
		if (ret) break;
	}
	return true;
}

void themeinfo_set(char *name, char *val)
{
	//is this the "id" property in the theme info file?
	if (!strcmp(name, "id")) {
		bool found;
		int i;
		//try to find the index for this theme in the themes struct
		for (i=0, found = false; !found && i < num_themes; i++) {
			if (atoi(val) == themes[i].ID) {
				found = true;
				themeIdx = i;
			}
		}
	} else if (!strcmp(name, "updated")) {
		theme_updated = atoi(val);
	}
}

int download_theme_preview(char *path, int idx)
{
	char filename[200];
	FILE *f = NULL;
	struct block file;
	file.data = NULL;
	
	printf_("%d. %s", idx, themes[idx].name);
	printf("\n");

	snprintf(filename, sizeof(filename), "%s/%d.jpg", path, themes[idx].ID);

	file = downloadfile(themes[idx].thumbpath);
	if (file.data == NULL || file.size == 0) {
		goto err;
	}

	f = fopen(filename, "wb");
	if (!f) {
		printf_(gt("Error opening: %s"), filename);
		printf("\n");
		goto err;
	}
	fwrite(file.data, 1, file.size, f);
	fclose(f);
	SAFE_FREE(file.data);
	return 0;

err:
	SAFE_FREE(file.data);
	printf_(gt("Error downloading theme preview..."));
	printf("\n");
	return -1;
}

// Downloads the theme preview image files
void download_all_theme_previews()
{
	char pathname[200];
	char path[200];
	char old_path[200];
	char old_name[200];
	int i;
	struct stat st;
	int buttons;

	printf_("%s\n", gt("Downloading Theme previews..."));
	printf_h("%s\n\n", gt("Hold button B to cancel."));

	snprintf(D_S(old_path), "%s/themes", USBLOADER_PATH);
	snprintf(D_S(path), "%s/theme_previews", USBLOADER_PATH);
	mkpath(path, 0777);
	for (i=0; i<num_themes; i++) {
		// check cancel
		buttons = Wpad_GetButtons();
		if (buttons & CFG.button_cancel.mask) {
			printf_("\n%s\n", gt("Cancelled."));
			sleep(2);
			return;
		}
		snprintf(D_S(pathname), "%s/%d.jpg", path, themes[i].ID);
		snprintf(D_S(old_name), "%s/%d.jpg", old_path, themes[i].ID);
		//Check if file is in the old path
		if (stat(old_name, &st) == 0) {
			// move to new path
			rename(old_name, pathname);
		}
		//Check if file is there
		if (stat(pathname, &st) != 0) {
			//download missing preview image
			download_theme_preview(path, i);
		}
	}
	printf_(gt("Done."));
	printf("\n");
	__console_flush(0);
	sleep(1);
}

// Loops through each local theme in the /themes dir to determine if an update is available.
void read_themeinfo(void)
{
	char pathname[200];
	int i;
	struct stat st;
	for(i = 0; i < num_theme; i++) {
		//Check if file is there
		snprintf(D_S(pathname), "%s/themes/%s/info.txt", USBLOADER_PATH, theme_list[i]);
		if (stat(pathname, &st) == 0) {
			theme_updated = 0;
			themeIdx = -1;
			//printf("Reading %s\n", pathname);
			//sleep(1);
			
			//find this theme's index (themeIdx) in the themes struct
			cfg_parsefile(pathname, &themeinfo_set);
			
			//printf("loc = %d, updated = %d\n",themeIdx, theme_updated);
			//sleep(1);
			
			if (themeIdx >= 0) {
				if (themes[themeIdx].type != THEME_TYPE_NEW_THEME) continue;
				strcopy(themes[themeIdx].name, theme_list[i], sizeof(themes[0].name));
				if (themes[themeIdx].updated > theme_updated) {
					// An update is available
					themes[themeIdx].type = THEME_TYPE_UPDATES_AVAILABLE;
					num_theme_updates++;
					num_new_themes--;
					//printf("Type 1 - update available\n");
					//sleep(1);
				} else {
					// Theme is up to date
					themes[themeIdx].type = THEME_TYPE_UP_TO_DATE;
					num_new_themes--;
					//printf("Type 2 - up to date\n");
					//sleep(1);
				}
			}
		}
	}
}

int themenum_set(char *line)
{
	return sscanf(line, " <totalthemes>%d", &num_updates);
}

int themedata_set(char *line)
{
	char *tag = NULL;
	char *value = NULL;
	char *closetag = NULL;
	if ((tag = strchr(line,'<')) == NULL) return 0;
	tag++;
	if ((value = strchr(tag,'>')) == NULL) return 0;
	*value++ = '\0';
	if ((closetag = strchr(value,'<')) != NULL) *closetag = '\0';
	if (!strcmp(tag, "id")) {
		sscanf(value, " %d", &themes[num_themes].ID);
	} else if (!strcmp(tag, "name")) {
		strcopy(themes[num_themes].name, value, sizeof(themes[0].name));	
	} else if (!strcmp(tag, "creator")) {
		strcopy(themes[num_themes].creator, value, sizeof(themes[0].creator));	
	} else if (!strcmp(tag, "version")) {
		sscanf(value, " %d", &themes[num_themes].version);
	} else if (!strcmp(tag, "adult")) {
		sscanf(value, " %d", &themes[num_themes].adult);
	} else if (!strcmp(tag, "downloads")) {
		sscanf(value, " %d", &themes[num_themes].downloads);
	} else if (!strcmp(tag, "votes")) {
		sscanf(value, " %d", &themes[num_themes].votes);
	} else if (!strcmp(tag, "rating")) {
		sscanf(value, " %d", &themes[num_themes].rating);
	} else if (!strcmp(tag, "added") || !strcmp(tag, "lastupdated")) {
		int val;
		sscanf(value, " %d", &val);
		if (val > themes[num_themes].updated) themes[num_themes].updated = val;	
	} else if (!strcmp(tag, "downloadpath")) {
		strcopy(themes[num_themes].url, value, sizeof(themes[0].url));	
	} else if (!strcmp(tag, "thumbpath")) {
		strcopy(themes[num_themes].thumbpath, value, sizeof(themes[0].thumbpath));	
	} else if (!strcmp(tag, "/theme")) {
		num_themes++;
		if (num_themes >= num_updates) return 1;
	}
	return 0;
}

int update_set(char *line)
{
	char name[100], val[100];
	struct UpdateInfo *u;
	bool opt = trimsplit(line, name, val, '=', sizeof(name));
	// if space in opt name, then don't consider it an option
	// this is so to allow = in comments 
	if (opt && strchr(name, ' ') != NULL) opt = false;
	if (opt)
	{
		if (stricmp("metaxml", name) == 0) {
			strncat(meta_xml, val, sizeof(meta_xml)-1);
			strncat(meta_xml, "\n", sizeof(meta_xml)-1);
			return 0;
		}
		if (stricmp("release", name) == 0) {
			if (num_updates >= MAX_UPDATES) {
				return 1;
			}
			num_updates++;
			u = &updates[num_updates-1]; 
			u->ndesc = 0;
			u->size = 0;
			*u->url = 0;
			STRCOPY(u->version, val);
			return 0;
		}

		if (num_updates <= 0) return 0;
		u = &updates[num_updates-1]; 
		if (stricmp("size", name) == 0) {
			int n;
			if (sscanf(val, "%d", &n) == 1) {
				if (n > 500000 && n < 5000000) {
					u->size = n;
				}
			}
		} else if (stricmp("date", name) == 0) {
			STRCOPY(u->date, val);
		} else if (stricmp("url", name) == 0) {
			STRCOPY(u->url, val);
		} else if (*u->url) {
			// anything unknown after url is a comment till a new release line
			goto comment;
		} else {
			// ignore unknown options
		}
	}
	else
	{
		comment:
		if (num_updates > 0) {
			u = &updates[num_updates-1]; 
			if (u->ndesc < MAX_UP_DESC) {
				STRCOPY(u->desc[u->ndesc], line);
				u->ndesc++;
			}
		}
	}
	return 0;

}

void parse_updates(char *buf)
{
	num_updates = 0;
	memset(updates, 0, sizeof(updates));
	memset(meta_xml, 0, sizeof(meta_xml));
	parsebuf_line(buf, update_set);
}

void parse_themes(char *buf)
{
	num_updates = 0;
	parsebuf_line(buf, themenum_set);
	if (!num_updates) return;
	themes = calloc(num_updates, sizeof(struct ThemeInfo));
	num_themes = 0;
	parsebuf_line(buf, themedata_set);
}

void Download_Update(int n, char *app_path, int update_meta)
{
	struct UpdateInfo *u;
	struct block file;
	FILE *f = NULL;
	file.data = NULL;
	u = &updates[n]; 

	printf_(gt("Downloading: %s"), u->version);
	printf("\n");
	printf_("[.");

	file = downloadfile_progress(u->url, 1);
	printf("]\n");
	
	if (file.data == NULL || file.size < 500000) {
		goto err;
	}

	if (u->size && u->size != file.size) {
		printf_(gt("Wrong Size: %d (%d)"), file.size, u->size);
		printf("\n");
		goto err;
	}

	printf_(gt("Complete. Size: %d"), file.size);
	printf("\n");

	printf_(gt("Saving: %s"), app_path);
	printf("\n");
	// backup
	char bak_name[200];
	strcpy(bak_name, app_path);
	strcat(bak_name, ".bak");
	// remove old backup
	remove(bak_name);
	// rename current to backup
	rename(app_path, bak_name);
	// save new
	f = fopen(app_path, "wb");
	if (!f) {
		printf_(gt("Error opening: %s"), app_path);
		printf("\n");
		goto err;
	}
	fwrite(file.data, 1, file.size, f);
	fclose(f);

	if (update_meta) {
		// reformat date
		char date_long[32]; // YYYYmmddHHMMSS
		char *p;
		strcpy(date_long, u->date); // YYYY-mm-dd
		p = strchr(date_long, '-');
		if (p) memmove(p, p+1, strlen(p));
		p = strchr(date_long, '-');
		if (p) memmove(p, p+1, strlen(p));
		strcat(date_long, "000000");

		strcpy(strrchr(app_path, '/')+1, "meta.xml");
		f = fopen(app_path, "rb");
		if (f) {
			char tmp_meta[sizeof(meta_xml)];
			int size = fread(tmp_meta, 1, sizeof(tmp_meta)-32, f);
			fclose(f);
			if (size <= 0) {
				goto new_meta;
			}
			tmp_meta[size] = 0; // 0 terminate
			//dbg_printf("existing:\n%s\n", tmp_meta);
			// verify it's a valid <app*...</app> meta.xml
			if (!strstr(tmp_meta, "<app") || !strstr(tmp_meta, "</app>")) {
				goto new_meta;
			}
			if (!str_replace_tag_val(tmp_meta, "<version>", u->version)) {
				goto new_meta;
			}
			if (!str_replace_tag_val(tmp_meta, "<release_date>", date_long)) {
				goto new_meta;
			}
			strcpy(meta_xml, tmp_meta);
		} else {
			new_meta:
			if (*meta_xml) {
				str_replace(meta_xml, "{VERSION}", u->version, sizeof(meta_xml));
				str_replace(meta_xml, "{DATE}", date_long, sizeof(meta_xml));
				printf_(gt("Saving: %s"), app_path);
				printf("\n");
				f = fopen(app_path, "wb");
				if (!f) {
					printf_(gt("Error opening: %s"), app_path);
					printf("\n");
				} else {
					fwrite(meta_xml, 1, strlen(meta_xml), f);
					fclose(f);
				}
			}
		}
		if (*meta_xml) {
			printf_(gt("Saving: %s"), app_path);
			printf("\n");
			//dbg_printf("saving:\n%s\n", meta_xml);
			f = fopen(app_path, "wb");
			if (!f) {
				printf_(gt("Error opening: %s"), app_path);
				printf("\n");
			} else {
				fwrite(meta_xml, 1, strlen(meta_xml), f);
				fclose(f);
			}
		}
	}

	Download_Translation();		//download new translation file

	printf_(gt("Done."));
	printf("\n\n");
	printf_(gt("NOTE: loader restart is required\nfor the update to take effect."));
	printf("\n");
	printf_(gt("If Cfg does not start properly\nnext time, copy boot.dol.bak over\nboot.dol and try again."));
	printf("\n\n");
	printf_(gt("Press any button..."));
	Wpad_WaitButtonsCommon();

	SAFE_FREE(file.data);
	return;

err:
	SAFE_FREE(file.data);
	printf_(gt("Error downloading update..."));
	printf("\n");
	sleep(3);
}



void Menu_Updates()
{
	struct Menu menu;
	struct UpdateInfo *u;
	char app_dir[100];
	char app_path[100];
	char app_bak[100];
	struct stat st;
	int cols, rows;
	int max_desc;
	int win_size;
	int i;

	CON_GetMetrics(&cols, &rows);
	//calc_rows: // debug
	win_size = rows - 16; // rows >= 22
	if (rows < 22 && rows > 16) win_size = 5;
	if (rows <= 16) win_size = rows - 12;
	if (win_size < 1) win_size = 1;
	if (win_size > num_updates) win_size = num_updates;
	menu_init(&menu, num_updates);

	STRCOPY(app_dir, APPS_DIR);
	snprintf(D_S(app_path),"%s/%s", app_dir, "boot.dol");
	snprintf(D_S(app_bak),"%s/%s", app_dir, "boot.dol.bak");
	// check if app_dir/boot.dol exists
	if (stat(app_path, &st) != 0 && stat(app_bak, &st) != 0) {
		// not found, use default path
		STRCOPY(app_dir, "sd:/apps/usbloader");
		snprintf(D_S(app_path),"%s/%s", app_dir, "boot.dol");
	}

	for (;;) {
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Available Updates"));
		printf(":\n");
		DefaultColor();
		max_desc = MAX_UP_DESC;
		menu_begin(&menu);
		menu_window_begin(&menu, win_size, num_updates);
		for (i=0; i<num_updates; i++) {
			if (menu_window_mark(&menu)) {
				printf("%s\n", updates[i].version);
			}
			if (updates[i].ndesc > max_desc) {
				max_desc = updates[i].ndesc;
			}
		}
		DefaultColor();
		menu_window_end(&menu, cols);

		FgColor(CFG.color_header);
		printf_x(gt("Release Notes: (short)"));
		printf("\n");
		DefaultColor();
		u = &updates[menu.current]; 
		for (i=0; i<max_desc; i++) {
			if (i < u->ndesc) {
				printf_("%.*s\n",
						cols - strlen(CFG.menu_plus_s) - 1,
						u->desc[i]);
			} else {
				printf("\n");
			}
		}

		if (rows > 18) {
			printf_h(gt("Press %s to download and update"), (button_names[CFG.button_confirm.num]));
			printf("\n");
			if (rows > 20) {
				printf_h(gt("Press %s to update without meta.xml"), (button_names[CFG.button_other.num]));
				printf("\n");
			}
			printf_h(gt("Press %s to return"), (button_names[CFG.button_cancel.num]));
			printf("\n");
		} else {
			printf_h(gt("Press %s to download, %s to return"), (button_names[CFG.button_confirm.num]), (button_names[CFG.button_cancel.num]));
			printf("\n");
		}
		if (rows > 19) printf("\n");
		FgColor(CFG.color_inactive);
		printf_(gt("Current Version: %s"), CFG_VERSION);
		printf("\n");
		printf_(gt("App. Path: %s"), app_dir);
		DefaultColor();
		//Con_SetPosition(0, rows-1); printf(" -- %d %d --", rows, win_size);
		__console_flush(0);

		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move(&menu, buttons);

		if (buttons & CFG.button_exit.mask) {
			Handle_Home(0);
		}
		if (buttons & CFG.button_cancel.mask) break;
		if (buttons & CFG.button_confirm.mask) {
			printf("\n\n");
			Download_Update(menu.current, app_path, 1);
			break;
		}
		if (buttons & CFG.button_other.mask) {
			printf("\n\n");
			Download_Update(menu.current, app_path, 0);
			break;
		}
		//if (buttons & WPAD_BUTTON_MINUS) { rows--; goto calc_rows; }
		//if (buttons & WPAD_BUTTON_PLUS) { rows++; goto calc_rows; }
	}
	printf("\n");
}

int Download_Theme(int n)
{
	struct ThemeInfo *u;
	struct block file;
	int rename = 0;
	char theme_dir[200] = "";
	char filename[200] = "";
	char theme_name[32];


	FILE *f = NULL;
	file.data = NULL;
	u = &themes[n]; 

	snprintf(theme_dir, sizeof(theme_dir), "%s/themes/%s", USBLOADER_PATH, u->name);

	if (themes[n].type == THEME_TYPE_NEW_THEME) {
		while (mkpath(theme_dir, 0777) != 0) {
			if (errno != EEXIST) goto err2;
			rename++;
			snprintf(theme_name, sizeof(theme_name), "%.29s_%d", u->name, rename + 1);
			snprintf(theme_dir, sizeof(theme_dir), "%s/themes/%s", USBLOADER_PATH, theme_name);
		}
	}

	printf("\n");
	if (rename)
		printf_(gt("Downloading: %s as %s"), u->name, theme_name);
	else
		printf_(gt("Downloading: %s"), u->name);
	printf("\n");

	snprintf(filename, sizeof(filename), "%s/%s", theme_dir, "theme.zip");

	printf_("[.");
	file = downloadfile_progress_fname(u->url, 1, filename);
	printf("]\n");
	
	if (file.filesize == 0) {
		goto err;
	}

	/*if (u->size && u->size != file.size) {
		printf_(gt("Wrong Size: %d (%d)"), file.size, u->size);
		printf("\n");
		goto err;
	}*/

	printf_(gt("Complete. Size: %d"), file.filesize);
	printf("\n");

	/*f = fopen(filename, "wb");
	if (!f) {
		printf_(gt("Error opening: %s"), filename);
		printf("\n");
		goto err;
	}
	fwrite(file.data, 1, file.size, f);
	fclose(f);*/
	SAFE_FREE(file.data);

	//unzip theme
	unzFile zf;
	zf = unzOpen(filename);
	unzGoToFirstFile(zf);
	int ret = UNZ_OK;
	while (ret == UNZ_OK) {
		unz_file_info file_info;
		char zippedname[200];
		char savename[200];
		char *p, *zn;
		void *buf;
		FILE *f;
		ret = unzGetCurrentFileInfo(zf,&file_info,zippedname,sizeof(zippedname),NULL,0,NULL,0);
		for (p = zn = zippedname; *p != '\0'; p++) {
			if (*p == '/' || *p == '\\')
				zn = p + 1;
		}
		if (zn[0] == '\0') {
			ret = unzGoToNextFile(zf);
			continue;
		}
		snprintf(savename, sizeof(savename), "%s/%s", theme_dir, zn);
		if ((f = fopen(savename, "wb")) == NULL) {
			unzClose(zf);
			goto err3;
		}
		if ((ret = unzOpenCurrentFile(zf)) != UNZ_OK) {
			unzClose(zf);
			goto err3;
		}
		printf(gt("Extracting: %s"), zn);
		printf("\n");
		buf = malloc(file_info.uncompressed_size);
		unzReadCurrentFile(zf, buf, file_info.uncompressed_size);
		unzCloseCurrentFile(zf);
		fwrite(buf,1,file_info.uncompressed_size,f);
		fclose(f);
		free(buf);
		ret = unzGoToNextFile(zf);
	}
	unzClose(zf);

	//delete zip
	remove(filename);

	//create info.txt
	snprintf(filename, sizeof(filename), "%s/%s", theme_dir, "info.txt");
	f = fopen(filename, "w");
	if (f == NULL) goto err;
	fprintf(f, "id = %d\n", u->ID);
	fprintf(f, "version = %d\n", u->version);
	fprintf(f, "updated = %d\n", u->updated);
	fprintf(f, "creator = %s\n", u->creator);
	fprintf(f, "adult = %d\n", u->adult);
	fclose(f);

	//Rename theme
	if (rename)
		strcopy(themes[n].name, theme_name, sizeof(themes[0].name));

	printf_(gt("Done."));
	printf("\n\n");
	printf_(gt("Press any button..."));
	printf("\n");

	Wpad_WaitButtonsCommon();

	return 0;

err:
	SAFE_FREE(file.data);
	printf_(gt("Error downloading theme..."));
	printf("\n");
	printf_(gt("Theme may be too large.\nReboot of Cfg suggested.\n"));
	printf_(gt("Press any button..."));
	Wpad_WaitButtonsCommon();
	return -1;

err2:
	SAFE_FREE(file.data);
	printf_(gt("Error creating directory..."));
	printf("\n");
	printf_(gt("Press any button..."));
	Wpad_WaitButtonsCommon();
	return -2;

err3:
	SAFE_FREE(file.data);
	printf_(gt("Error extracting theme..."));
	printf("\n");
	printf_(gt("Press any button..."));
	Wpad_WaitButtonsCommon();
	return -3;
}

void Menu_Themes()
{
	struct Menu menu;
	int rows, cols, size, n, count, menu_rows;
	
	CON_GetMetrics(&cols, &rows);
	if ((size = rows-10) < 3) size = 3;

	menu_rows = 0;
	if (num_theme_updates) menu_rows += num_theme_updates + 1;
	if (num_new_themes) menu_rows += num_new_themes + 1;
	char active[menu_rows];
	int lookup[menu_rows];
	
	//redraw the background without the current cover
	Video_DrawBg();
	
	menu_init(&menu, menu_rows);
	menu.current = 1;
	for (;;) {
		n = 0;
		count = 0;
		menu_init_active(&menu, active, sizeof(active));
		active[0] = 0;
		if (num_theme_updates && num_new_themes) active[num_theme_updates+1] = 0;
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Themes provided by wii.spiffy360.com"));
		printf("\n%s:\n", gt("Themes to download"));
		DefaultColor();
		menu_begin(&menu);
		menu_window_begin(&menu, size, menu_rows);
		if (num_theme_updates) {
			lookup[count++] = -1;
			if (menu_window_mark(&menu))
				printf("%s:\n", gt("Themes with updates"));
			for (n = 0; n < num_themes; n++) {
				if (themes[n].type == THEME_TYPE_UPDATES_AVAILABLE) {
					lookup[count++] = n;
					if (menu_window_mark(&menu))
						printf("%s\n", themes[n].name);
				}
			}
		}
		if (num_new_themes) {
			lookup[count++] = -1;
			if (menu_window_mark(&menu))
				printf("%s:\n", gt("New Themes"));
			for (n = 0; n < num_themes; n++) {
				if (themes[n].type == THEME_TYPE_NEW_THEME) {
					lookup[count++] = n;
					if (menu_window_mark(&menu))
						printf("%s\n", themes[n].name);
				}
			}
		}
		DefaultColor();
		menu_window_end(&menu, cols);
		
		//printf("\n");
		FgColor(CFG.color_header);
		printf_x("%s:\n", gt("Theme Info"));
		DefaultColor();
		if (m_active(&menu, menu.current)) {
			printf_("  %s: %s\n", gt("Creator"), themes[lookup[menu.current]].creator);
			printf_("  %s: %d", gt("Version"), themes[lookup[menu.current]].version);
			printf("  %s: %s\n", gt("Adult"), (themes[lookup[menu.current]].adult) ? gt("Yes") : gt("No"));
			printf_("  %s: %d", gt("Downloads"), themes[lookup[menu.current]].downloads);
			printf("  %s: ", gt("Rating"));
			if (themes[lookup[menu.current]].votes)
				printf("%.1f\n", (float)themes[lookup[menu.current]].rating/themes[lookup[menu.current]].votes);
			else
				printf("--\n");
			printf_h(gt("Press %s to download this theme"), (button_names[CFG.button_confirm.num]));
			printf("\n");
			printf_h(gt("Press %s for fullscreen preview"), (button_names[CFG.button_other.num]));

			//draw the theme preview image
			if (CFG.theme_previews) {
				Gui_DrawThemePreview(themes[lookup[menu.current]].name, themes[lookup[menu.current]].ID);
			}
		}
		__console_flush(0);
		
		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move(&menu, buttons);
		
		if ((buttons & CFG.button_confirm.mask) && lookup[menu.current] >= 0) {
			int ret = Download_Theme(lookup[menu.current]);
			if (ret == 0) {
				if (num_theme < MAX_THEME) {
					strcopy(theme_list[num_theme], themes[lookup[menu.current]].name, sizeof(theme_list[num_theme]));
					num_theme++;
				}
			}
			if (themes[lookup[menu.current]].type == THEME_TYPE_NEW_THEME)
				num_new_themes--;
			else if (themes[lookup[menu.current]].type == THEME_TYPE_UPDATES_AVAILABLE)
				num_theme_updates--;
			themes[lookup[menu.current]].type = THEME_TYPE_UP_TO_DATE;
			menu_rows = 0;
			if (num_theme_updates) menu_rows += num_theme_updates + 1;
			if (num_new_themes) menu_rows += num_new_themes + 1;
			menu.num_opt = menu_rows;
			if (menu.current == (menu_rows)) menu.current = menu_rows-1;
		}

		// HOME button
		if (buttons & CFG.button_exit.mask) {
			Handle_Home(0);
		}
		if (buttons & CFG.button_cancel.mask) break;
		// button 1
		if (buttons & CFG.button_other.mask) {
			//draw the theme preview image
			if (CFG.theme_previews) {
				Gui_DrawThemePreviewLarge(
						themes[lookup[menu.current]].name,
						themes[lookup[menu.current]].ID);
			}
		}
	}
	//redraw the background and current cover image
	Video_DrawBg();
	extern void __Menu_ShowCover();
	__Menu_ShowCover();
}

// Main method for Themes
//   Downloads the theme xml file from spiffy360.com
//   Parses the file and stores theme info in a ThemeInfo struct
//   Displays the Themes menu (console mode)
void Theme_Update()
{
	char *themes_url;
	if (CFG.adult_themes)
		themes_url = "http://wii.spiffy360.com/themes.php?xml=1&adult=1&category=5";
	else
		themes_url = "http://wii.spiffy360.com/themes.php?xml=1&category=5";
	struct block file;
	file.data = NULL;

	DefaultColor();
	printf("\n");
	printf_(gt("Checking for themes..."));
	printf("\n");

	//download the latest theme xml
	if (Init_Net()) {
		file = downloadfile(themes_url);
	}
	if (file.data == NULL || file.size == 0) {
		printf_(gt("Error downloading themes..."));
		printf("\n");
		sleep(3);
		return;
	}

	// null terminate
	char *newbuf;
	newbuf = mem_realloc(file.data, file.size+4);
	if (!newbuf) goto err;
	newbuf[file.size] = 0;
	file.data = (void*)newbuf;

	//check if any updates are available
	parse_themes((char*)file.data);
	SAFE_FREE(file.data);

	if (num_updates <= 0) {
		printf_(gt("No themes found."));
		printf("\n");
		goto out;
	}

	//printf("\n");
	//printf_("Available: %d updates.\n", num_updates);
	//sleep(1);
	num_new_themes = num_themes;
	num_theme_updates = 0;
	read_themeinfo();
	
	//update all the theme preview images
	if (CFG.theme_previews)
		download_all_theme_previews();

	//if all themes have been downloaded get out
	if (num_new_themes == 0) {
		printf_(gt("All themes up to date."));
		printf("\n");
		goto out;
	}
	
	Menu_Themes();
	SAFE_FREE(themes);
	return;

err:
	printf_(gt("Error downloading themes..."));
	printf("\n");
out:
	sleep(3);
	SAFE_FREE(file.data);
	return;
}

void Online_Update()
{
	char *updates_url = "http://cfg-loader-mod.googlecode.com/svn/trunk/updates.txt";
	struct block file;
	file.data = NULL;

	DefaultColor();
	printf("\n");
	printf_(gt("Checking for updates..."));
	printf("\n");

	if (Init_Net()) {
		file = downloadfile(updates_url);
	}
	if (file.data == NULL || file.size == 0) {
		printf_(gt("Error downloading updates..."));
		printf("\n");
		sleep(3);
		return;
	}

	// null terminate
	char *newbuf;
	newbuf = mem_realloc(file.data, file.size+4);
	if (newbuf) {
		newbuf[file.size] = 0;
		file.data = (void*)newbuf;
	} else {
		goto err;
	}
	// parse
	parse_updates((char*)file.data);
	SAFE_FREE(file.data);

	if (num_updates <= 0) {
		printf_(gt("No updates found."));
		printf("\n");
		goto out;
	}

	//printf("\n");
	//printf_("Available: %d updates.\n", num_updates);
	//sleep(1);

	Menu_Updates();
	return;

err:
	printf_(gt("Error downloading updates..."));
	printf("\n");
out:
	sleep(3);
	SAFE_FREE(file.data);
	return;
}

void Download_Titles()
{
	struct block file;
	file.data = NULL;
	char fname[100];
	char url[100];
	FILE *f;

	DefaultColor();
	printf("\n");
	
	if (!Init_Net()) goto out;

	printf_(gt("Downloading titles.txt ..."));
	printf("\n");
	extern char *get_cc();
	char *cc = get_cc();
	char * dbl = ((strlen(CFG.db_language) == 2) ? VerifyLangCode(CFG.db_language) : ConvertLangTextToCode(CFG.db_language));
	//if (strcmp(cc, "JA") ==0 || strcmp(cc, "KO") ==0) cc = "EN";
	strcpy(url, CFG.titles_url);
	str_replace(url, "{CC}", cc, sizeof(url));
	str_replace(url, "{DBL}", dbl, sizeof(url));
	printf_("%s\n", url);

	file = downloadfile(url);
	if (file.data == NULL || file.size == 0) {
		printf_(gt("Error downloading updates..."));
		printf("\n");
		sleep(3);
		return;
	}

	snprintf(D_S(fname),"%s/%s", USBLOADER_PATH, "titles.txt");
	printf_(gt("Saving: %s"), fname);
	printf("\n");
	f = fopen(fname, "wb");
	if (!f) {
		printf_(gt("Error opening: %s"), fname);
		printf("\n");
	} else {
		fwrite(file.data, 1, file.size, f);
		fclose(f);
		printf_(gt("Done."));
		printf("\n\n");
		printf_(gt("NOTE: loader restart is required\nfor the update to take effect."));
		printf("\n\n");
	}
out:
	printf_(gt("Press any button..."));
	Wpad_WaitButtonsCommon();
	SAFE_FREE(file.data);
	return;
}

