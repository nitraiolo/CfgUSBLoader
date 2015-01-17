
// by oggzee

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
#include "menu.h"
#include "gettext.h"

////////////////////////////////////////
//
//  Cheats
//
////////////////////////////////////////

bool get_line_buf(char **buf, char *line, int linesize)
{
	char *p, *nl;
	int len;

	*line = 0;

	if (buf == NULL) return false;
	if (*buf == NULL) return false;
	if (**buf == 0) return false;
	p = *buf;
	nl = strchr(p, '\n'); // find LF
	if (nl == NULL) {
		len = strlen(p);
		*buf = NULL;
	} else {
		len = nl - p;
		*buf = nl + 1;
		if (**buf == 0) *buf = NULL;
	}
	if (len >= linesize) len = linesize - 1;
	strcopy(line, p, len+1);
	// remove any CR in case it's a msdos CRLF format
	len = strlen(line);
	if (len > 0 && line[len-1] == '\r') line[len-1] = 0;
	return true;
}

#define CHEAT_MAX_TITLE 40
#define CHEAT_MAX_NOTES 3
#define CHEAT_MAX_LINES 3000
#define CODE_LINE_LEN 18
#define CHEAT_MAX 512

struct Cheat
{
	char title[CHEAT_MAX_TITLE];
	int num_codes;
	int line_idx; // index to code line
	int num_notes;
	char note[CHEAT_MAX_NOTES][CHEAT_MAX_TITLE];
	bool enabled;
	bool editable;
};

struct Cheats
{
	char id[8];
	char title[CHEAT_MAX_TITLE];
	int num_lines;
	char line[CHEAT_MAX_LINES][CODE_LINE_LEN];
	int num_cheats;
	struct Cheat cheat[CHEAT_MAX];
};

struct Cheats cheats;

int line_buf_count;
char *line_buf;
char line[200];

bool get_line()
{
	bool ret;
	line_buf_count++;
	ret = get_line_buf(&line_buf, D_S(line));
	//printf("%d %s\n", line_buf_count, line);
	return ret;
}

// sample:
//P2 No Laps (Finish right away)
//48000000 809BD730 [memorris]
//DE000000 80008180

bool is_code(char *line)
{
	int i;
	if (strlen(line) < 17) return false;
	if (strlen(line) > 17 && line[17] != ' ') return false;
	if (line[8] != ' ') return false;
	// check if address is valid
	for (i=0; i<8; i++) {
		if (!ISXDIGIT(line[i])) return false;
	}
	// check if value is hex
	for (i=9; i<17; i++) {
		if (!ISALNUM(line[i])) return false;
	}
	return true;
}

// sample:
//14000000 000000XX

bool is_editable(char *line)
{
	int i;
	if (!is_code(line)) return false;
	for (i=9; i<17; i++) {
		// is hex digit?
		if (!ISXDIGIT(line[i])) return true;
	}
	return false;
}

bool parse_cheats(char *buf)
{
	struct Cheat *cur_cheat;

	memset(&cheats, 0, sizeof(cheats));
	
	line_buf_count = 0;
	line_buf = buf;

	// first line
	if (!get_line()) goto err;

	// remove utf8 bom
	if (memcmp(line, "\xEF\xBB\xBF", 3) == 0) {
		memmove(line, line+3, strlen(line+3)+1);
	}

	// game id
	if (strlen(line) < 4 || strlen(line) > 6) goto err;
	STRCOPY(cheats.id, line);

	// game title
	if (!get_line()) goto err;
	if (strlen(line) == 0) goto err;
	STRCOPY(cheats.title, line);

	// empty line
	if (!get_line()) goto err;
	if (strlen(line) > 0) goto err;

	// loop cheats
	for (;;)
	{
		// cheat title
		if (!get_line()) break;
		if (strlen(line) == 0) return false;
		if (cheats.num_cheats >= CHEAT_MAX) {
			printf(gt("Too many cheats! (%d)"), cheats.num_cheats);
			printf("\n");
			goto err2;
		}
		cheats.num_cheats++;
		cur_cheat = &cheats.cheat[cheats.num_cheats-1];
		STRCOPY(cur_cheat->title, line);
		// loop codes
		for (;;)
		{
			if (!get_line()) break;
			if (strlen(line) == 0) break; // empty line = end
			if (is_code(line))
			{
				// it's a code.
			   	if (cheats.num_lines >= CHEAT_MAX_LINES) {
					printf(gt("Too many code lines!"));
					printf("\n");
					goto err2;
				}
			   	cheats.num_lines++;
				STRCOPY(cheats.line[cheats.num_lines-1], line);
				if (cur_cheat->num_codes == 0) {
					cur_cheat->line_idx = cheats.num_lines - 1;
				}
				cur_cheat->num_codes++;
				if (is_editable(line)) {
					cur_cheat->editable = true;
				}
			} else {
				// it's a note.
				for (;;) {
					cur_cheat->num_notes++;
					if (cur_cheat->num_notes <= CHEAT_MAX_NOTES) {
						STRCOPY(cur_cheat->note[cur_cheat->num_notes-1], line);
					} else {
						// ignore the rest of notes 
					}
					if (!get_line()) break;
					if (strlen(line) == 0) break; // empty line = end
				}
				break;
			}
		}

	}
	return true;

err:
	printf(gt("Unknown syntax!"));
	printf("\n");
	printf("%d: '%s'\n", line_buf_count, line);
err2:
	printf(gt("Press any button..."));
	printf("\n");
	Wpad_WaitButtons();
	// reset cheats on error.
	memset(&cheats, 0, sizeof(cheats));
	return false;
}

int Load_Cheats_TXT(char *id)
{
	char filepath[200];
	char *buf = NULL;
	int size = 0;
	bool ret;

	// reset all to 0
	memset(&cheats, 0, sizeof(cheats));

	snprintf(D_S(filepath), "%s/codes/%.6s.txt", USBLOADER_PATH, id);
	size = Fat_ReadFile(filepath, (void*)&buf);
	if (size <= 0 || buf == NULL) return 2; // no file
	dbg_printf("\n%s : %d\n", filepath, size);

	// null terminate
	char *newbuf;
	newbuf = realloc(buf, size+4);
	if (newbuf) {
		newbuf[size] = 0;
		buf = (void*)newbuf;
	} else {
		SAFE_FREE(buf);
		return 1;
	}
	// parse
	ret = parse_cheats(buf);
	SAFE_FREE(buf);
	if (!ret) return 1; // parse error
	if (strncmp(id, cheats.id, strlen(cheats.id)) != 0) return 1;

	return 0; // ok
}

int Save_Cheats_GCT(char *id)
{
	char filepath[200];
	char gct_head[] = { 0x00, 0xD0, 0xC0, 0xDE, 0x00, 0xD0, 0xC0, 0xDE };
	char gct_tail[] = { 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int i, j, k, count;
	struct Cheat *cheat = NULL;
	FILE *f;

	count = 0;
	for (i=0; i<cheats.num_cheats; i++) {
		if (cheats.cheat[i].enabled) count++;
	}
	if (count == 0) return -1;

	snprintf(D_S(filepath), "%s/codes/%.6s.gct", USBLOADER_PATH, id);
	printf("\n");
	printf_(gt("Saving: %s"), filepath);
	printf("\n");

	f = fopen(filepath, "wb");
	if (!f) {
		printf_(gt("Error opening: %s"), filepath);
		printf("\n");
		sleep(2);
		return -1;	
	}
	// gct header
	fwrite(gct_head, 1, sizeof(gct_head), f);
	// codes
	for (i=0; i<cheats.num_cheats; i++) {
		cheat = &cheats.cheat[i];
		if (cheat->enabled) {
			for (j=0; j<cheat->num_codes; j++) {
				char *cp;
				unsigned int ci = 0;
				unsigned char c = 0;
				cp = cheats.line[cheat->line_idx + j];
				//printf("[%d %d] %s\n      ", i, j, cp);
				for (k=0; k<8; k++) {
					if (k == 4) cp++; // skip space
					sscanf(cp, "%2x", &ci);
					c = ci;
					//printf("%02x ", c);
					fwrite(&c, 1, 1, f);
					cp += 2;
				}
				//printf("\n");
			}
		}
	}
	// gct tail
	fwrite(gct_tail, 1, sizeof(gct_tail), f);
	fclose(f);
	printf_(gt("Done."));
	printf("\n");
	sleep(2);
	//Wpad_WaitButtons();

	return 0; // ok
}

bool Download_Cheats_TXT(char *id)
{
	char filepath[200];
	//char *url_base = "http://www.usbgecko.com/codes/codes";
	char *url_base = "http://www.geckocodes.org/codes";
	char url[200];
	struct block file;
	FILE *f;

	file.data = NULL;

	DefaultColor();
	printf("\n");
	printf_(gt("Downloading cheats..."));
	printf("\n");

	extern bool Init_Net();
	if (!Init_Net()) goto err;

	// 6 letters ID
	//snprintf(D_S(url), "%s/%c/%.6s.txt", url_base, id[0], id);
	snprintf(D_S(url), "%s/R/%.6s.txt", url_base, id);
	dbg_printf("url: %s\n", url);
	file = downloadfile(url);
	if (file.data == NULL || file.size == 0) {
	
		snprintf(D_S(url), "%s/G/%.6s.txt", url_base, id);
		dbg_printf("url: %s\n", url);
		file = downloadfile(url);
		if (file.data == NULL || file.size == 0) {
			// 4 letters ID
			snprintf(D_S(url), "%s/%c/%.4s.txt", url_base, id[0], id);
			dbg_printf("url: %s\n", url);
			file = downloadfile(url);
			if (file.data == NULL || file.size == 0) {
				printf_(gt("Error downloading."));
				printf("\n");
				goto err;
			}
		}
	}

	printf_(gt("Saving cheats..."));
	printf("\n");
	snprintf(D_S(filepath), "%s/codes", USBLOADER_PATH);
	mkpath(filepath, 0777);
	snprintf(D_S(filepath), "%s/codes/%.6s.txt", USBLOADER_PATH, id);
	printf_("%s\n", filepath);
	f = fopen(filepath, "wb");
	if (!f) {
		printf("\n");
		printf_(gt("Error opening: %s"), filepath);
		printf("\n");
		goto err;
	}
	fwrite(file.data, 1, file.size, f);
	fclose(f);
	printf_(gt("OK"));
	printf("\n");
	SAFE_FREE(file.data);
	sleep(2);

	return true;

err:
	printf("\n");
	printf(gt("Press any button..."));
	printf("\n");
	Wpad_WaitButtons();
	SAFE_FREE(file.data);
	return false;
}

void print_game_info(struct discHdr *header, int cols)
{
	int len;
	len = cols - strlen(CFG.menu_plus_s) - 10;
	printf_("");
	printf("(%.6s) ", header->id);
	printf("%.*s\n", len, get_title(header));
}


/*
	Ocarina Cheat Manager
	RHAP Wii Play
	Cheats: 20 found / no file / parse error

	<download txt>
	<save gct>
	<select all>
    +
	[ ] cheat 1
	[x] cheat 2
	[x] cheat 3
	[x] cheat 4
    +    
	Notes...
	Notes...
	Notes...
   	(more notes) / Press button B to go back
*/

void Menu_Cheats(struct discHdr *header)
{
	struct Menu menu;
	char active[CHEAT_MAX + 5];
	int cheat_state = 3;
	// 3: loading
	// 2: no file
	// 1: parse err
	// 0: ok
	int cols, rows;
	int cols_opt;
	int cols_note;
	int rows_note;
	int window_size;
	int current_cheat;
	struct Cheat *cheat = NULL;
	int i;

	menu_init(&menu, 0);
	memset(&cheats, 0, sizeof(cheats));

	CON_GetMetrics(&cols, &rows);
	rows_note = rows > 15 ? 3 : 0;
	cols_note = cols - strlen(CFG.menu_plus_s) -1;
	cols_opt = cols - strlen(CFG.cursor) - 2 -1;
	window_size = rows - 8 - rows_note - 2 - 1;

	for (;;) {
		menu.num_opt = 3 + cheats.num_cheats;
		// update active state
		menu_init_active(&menu, active, sizeof(active));
		for (i=0; i<cheats.num_cheats; i++) {
			cheat = &cheats.cheat[i];
			if (cheat->num_codes == 0) {
				active[i+3] = 0;
			} else if (cheat->editable) {
				active[i+3] = 0;
			} else {
				active[i+3] = 1;
			}
		}

		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Ocarina Cheat Manager"));
		printf("\n");
		DefaultColor();
		print_game_info(header, cols);
		printf_x(gt("Cheats: "));
		switch (cheat_state) {
			case 3:
				printf(gt("Loading ..."));
				printf("\n");
				__console_flush(0);
				cheat_state = Load_Cheats_TXT((char*)header->id);
				usleep(100000);
				continue;
			case 2:
				printf(gt("no file"));
				printf("\n");
				break;
			case 1:
				printf(gt("parse error"));
				printf("\n");
				break;
			case 0:
				printf(gt("%d available"), cheats.num_cheats);
				printf("\n");
				break;
		}
		printf("\n");

		menu.line_count = 0;
		// allow moving on all lines, so no active check & jump
		//menu_jump_active(&menu);

		MENU_MARK();
		printf("<%s>\n", gt("Download .txt"));
		MENU_MARK();
		printf("<%s>\n", gt("Save .gct"));
		MENU_MARK();
		printf("<%s>\n", gt("Select all"));

		// cheats
		DefaultColor();
		menu_window_begin(&menu, window_size, cheats.num_cheats);
		for (i=0; i<cheats.num_cheats; i++) {
			if (!menu_window_mark(&menu)) continue;
			cheat = &cheats.cheat[i];
			if (cheat->num_codes == 0) {
				printf("    ");
			} else if (cheat->editable) {
				printf("(E) ");
			} else if (cheat->enabled) {
				printf("[x] ");
			} else {
				printf("[ ] ");
			}
			printf("%.*s\n", cols_opt-4, cheat->title);
		}
		DefaultColor();
		menu_window_end(&menu, cols);
		// notes
		FgColor(CFG.color_inactive);
		current_cheat = menu.current - 3;
		int printed_notes = 0;
		if (current_cheat < 0) {
			cheat = NULL;
		} else {
			cheat = &cheats.cheat[current_cheat];
			int n = MIN(cheat->num_notes, CHEAT_MAX_NOTES);
			n = MIN(n, rows_note);
			for (i=0; i<n; i++) {
				printf_("");
				printf("%.*s\n", cols_note, cheat->note[i]);
			}
			printed_notes = n;
			if (cheat->num_notes > n) {
				printf_("");
				printf("(");
				printf(gt("%d more notes"), cheat->num_notes - n);
				printf(")\n");
				printed_notes++;
			}
		}
		// help
		for (i=0; i < 3 - printed_notes; i++) {
			printf("\n");
		}
		if (printed_notes <= 3) {
			printf_h(gt("Press %s to return"), (button_names[CFG.button_cancel.num]));
			printf("\n");
		}
		__console_flush(0);

		// move
		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move(&menu, buttons);

		// change
		int change = 0;
		if (buttons & CFG.button_confirm.mask) change = 1;
		if (buttons & WPAD_BUTTON_LEFT) change = -1;
		if (buttons & WPAD_BUTTON_RIGHT) change = 1;

		if (change)
		{
			switch (menu.current) {
			case 0:
				Download_Cheats_TXT((char*)header->id);
				cheat_state = 3;
				continue;
			case 1:
				Save_Cheats_GCT((char*)header->id);
				break;
			case 2:
				for (i=0; i<cheats.num_cheats; i++) {
					cheat = &cheats.cheat[i];
					if (cheat->num_codes > 0 && !cheat->editable) {
						cheat->enabled = change > 0;
					}
				}
				break;
			}
			if (menu.current >= 3 && cheat) {
				if (cheat->num_codes > 0 && !cheat->editable) {
					if (buttons & CFG.button_confirm.mask) {
						cheat->enabled = !cheat->enabled;
					} else if (change < 0) {
						cheat->enabled = false;
					} else {
						cheat->enabled = true;
					}
				}
			}
		}

		if (buttons & CFG.button_exit.mask) {
			Handle_Home(0);
		}
		if (buttons & CFG.button_cancel.mask) break;
	}
	printf("\n");
}
