// by oggzee
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "cfgutil.h"
#include "strutil.h"
#include "debug.h"
#include "video.h"
#include "cfg.h"

char *cfg_name, *cfg_val;


int map_get_num(struct TextMap *map)
{
	int i;
	for (i=0; map[i].name != NULL; i++);
	return i;
}

int map_to_list(struct TextMap *map, int n, char **list)
{
	int i;
	for (i=0; i < n && map[i].name; i++) {
		list[i] = map[i].name;
	}
	return i;
}

int map_get_id(struct TextMap *map, char *name, int *id_val)
{
	int i;
	for (i=0; map[i].name != NULL; i++) {
		if (strcmp(name, map[i].name) == 0) {
			*id_val = map[i].id;
			return i;
		}
	}
	return -1;
}

char* map_get_name(struct TextMap *map, int id)
{
	int i;
	for (i=0; map[i].name != NULL; i++)	{
		if (id == map[i].id) return map[i].name;
	}
	return NULL;
}

int map_auto_i(char *name, char *name2, char *val, struct TextMap *map, int *var)
{
	if (strcmp(name, name2) != 0) return -1;
 	int i, id;
	i = map_get_id(map, val, &id);
	if (i >= 0) *var = id;
	//printf("MAP AUTO: %s=%s : %d [%d]\n", name, val, id, i); sleep(1);
	return i;
}

bool map_auto(char *name, char *name2, char *val, struct TextMap *map, int *var)
{
	int i = map_auto_i(name, name2, val, map, var);
	return i >= 0;
}


bool cfg_map_auto(char *name, struct TextMap *map, int *var)
{
	return map_auto(name, cfg_name, cfg_val, map, var);
}

bool cfg_map(char *name, char *val, int *var, int id)
{
	if (strcmp(name, cfg_name)==0 && strcmp(val, cfg_val)==0) {
		*var = id;
		return true;
	}
	return false;
}

// ignore value case:
void cfg_map_case(char *name, char *val, int *var, int id)
{
	if (strcmp(name, cfg_name)==0 && stricmp(val, cfg_val)==0) *var = id;
}

bool cfg_bool(char *name, int *var)
{
	if (cfg_map(name, "0", var, 0)) return true;
	if (cfg_map(name, "1", var, 1)) return true;
	return false;
}

bool cfg_int_hex(char *name, int *var)
{
	if (strcmp(name, cfg_name)==0) {
		int i;
		if (sscanf(cfg_val, "%x", &i) == 1) {
			*var = i;
			return true;
		}
	}
	return false;
}

bool cfg_int_max(char *name, int *var, int max)
{
	if (strcmp(name, cfg_name)==0) {
		int i;
		if (sscanf(cfg_val, "%d", &i) == 1) {
			if (i >= 0 && i <= max) {
				*var = i;
				return true;
			}
		}
	}
	return false;
}

bool cfg_str(char *name, char *var, int size)
{
	if (strcmp(name, cfg_name)==0) {
		if (strcmp(cfg_val,"0")==0) {
			*var = 0;
		} else {
			strcopy(var, cfg_val, size);
		}
		return true;
	}
	return false;
}

// if val starting with + append to a space delimited list
bool cfg_str_list(char *name, char *var, int size)
{
	if (strcmp(name, cfg_name)==0) {
		if (cfg_val[0] == '+') {
			if (*var) {
				// append ' '
				strappend(var, " ", size);
			}
			// skip +
			char *val = cfg_val + 1;
			// trim space
			while (*val == ' ') val++;
			// append val
			strappend(var, val, size);
		} else {
			strcopy(var, cfg_val, size);
		}
		return true;
	}
	return false;
}

// split line to name = val and trim whitespace
void cfg_parseline(char *line, void (*set_func)(char*, char*))
{
	// split name = value
	char name[400], val[400];
	// handle [group=param] as name=[group] val=param
	// '=param' is optional
	if (*line == '[') {
		trimcopy(name, line, sizeof(name));
		if (name[strlen(name)-1] != ']') return;
		// check for optional =param
		*val = 0;
		char *eq = strchr(name, '=');
		if (eq) {
			trimcopy(val, eq+1, sizeof(val));
			if (val[strlen(val)-1] == ']') {
				val[strlen(val)-1] = 0;
			}
			strcpy(eq, "]");
		}
	} else {
		if (!trimsplit(line, name, val, '=', sizeof(name))) return;
	}
	//printf("CFG: '%s=%s'\n", name, val); //sleep(1);
	set_func(name, val);
}

bool cfg_parsebuf(char *buf, void (*set_func)(char*, char*))
{
	char line[500];
	char *p, *nl;
	int len;
	char bom[] = {0xEF, 0xBB, 0xBF, 0};
	int i = 0;
	nl = buf;
	// skip BOM UTF-8 (ef bb bf)
	if (strncmp(nl, bom, 3) == 0) nl += 3;
	for (;;) {
		p = nl;
		if (p == NULL) break;
		while (*p == '\n' || *p == '\r') p++;
		if (*p == 0) break;
		nl = strchr(p, '\n');
		if (nl == NULL) {
			len = strlen(p);
		} else {
			len = nl - p;
		}
		if (!len) continue;
		// lines starting with # are comments
		if (*p == '#') continue;
		if (len >= sizeof(line)) len = sizeof(line) - 1;
		strcopy(line, p, len+1);
		if (line[len-1] == '\r') {
			line[len-1] = 0;
			len--;
		}

		if (CFG.debug > 1) {
			dbg_print(2, "\r[%d] <%.30s>%*s ", i, line, (len<30?30-len:0),"");
			if (i<20) {
				if (i<1) dbg_print(2, "\n");
				VIDEO_WaitVSync();
			}
		}
		i++;

		cfg_parseline(line, set_func);
	}
	dbg_print(2, "\n");
	return true;
}

bool cfg_parsefile_old(char *fname, void (*set_func)(char*, char*))
{
	FILE *f;
	char line[500];
	char bom[] = {0xEF, 0xBB, 0xBF};
	int first_line = 1;

	//printf("opening(%s)\n", fname); sleep(3);
	f = fopen(fname, "rb");
	if (!f) {
		//printf("error opening(%s)\n", fname); sleep(3);
		return false;
	}

	while (fgets(line, sizeof(line), f)) {
		// skip BOM UTF-8 (ef bb bf)
		if (first_line) {
			if (memcmp(line, bom, sizeof(bom)) == 0) {
				memmove(line, line+sizeof(bom), strlen(line)-sizeof(bom)+1);
				/*printf("BOM found in %s\n", fname);
				printf("line: '%s'\n", line);
				sleep(3);*/
			}
			first_line = 0;
		}
		// lines starting with # are comments
		if (line[0] == '#') continue;
		// parse
		cfg_parseline(line, set_func);
	}
	fclose(f);
	return true;
}

bool cfg_parsefile(char *fname, void (*set_func)(char*, char*))
{
	int fd;
	struct stat st;
	char *buf = NULL;
	int size;
	bool ret;
	int r;

	dbg_print(2, "parse(%s)", fname);
	r = stat(fname, &st);
	if (r != 0) {
		dbg_print(2, " -\n");
		return false;
	}
	if (st.st_size == 0) {
		dbg_print(2, " 0\n");
		return true;
	}
	fd = open(fname, O_RDONLY);
	dbg_print(2, " = %d\n", fd);
	if (fd < 0) return false;
	buf = malloc(st.st_size + 32);
	if (!buf) return false;
	dbg_print(2, "read(%d)", (int)st.st_size);
	size = read(fd, buf, st.st_size);
	dbg_print(2, " = ");
	close(fd);
	dbg_print(2, "%d\n", size);
	if (size != st.st_size) {
		SAFE_FREE(buf);
		return false;
	}
	buf[size] = 0; // zero terminate
	ret = cfg_parsebuf(buf, set_func);
	SAFE_FREE(buf);
	dbg_print(2, "EOF(%s)\n", fname);
	return ret;
}

