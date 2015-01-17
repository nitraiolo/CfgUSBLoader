// by oggzee
#ifndef _CFGUTIL_H
#define _CFGUTIL_H 1

#include <gctypes.h>

struct TextMap
{
	char *name;
	int id;
};

#define MAP_NUM(map) (sizeof(map) / sizeof(struct TextMap) - 1)

int map_get_num(struct TextMap *map);
int map_to_list(struct TextMap *map, int n, char **list);
int map_get_id(struct TextMap *map, char *name, int *id_val);
char* map_get_name(struct TextMap *map, int id);
int map_auto_i(char *name, char *name2, char *val, struct TextMap *map, int *var);
bool map_auto(char *name, char *name2, char *val, struct TextMap *map, int *var);
bool cfg_map_auto(char *name, struct TextMap *map, int *var);
bool cfg_map(char *name, char *val, int *var, int id);
// ignore value case:
void cfg_map_case(char *name, char *val, int *var, int id);
bool cfg_bool(char *name, int *var);
bool cfg_int_hex(char *name, int *var);
bool cfg_int_max(char *name, int *var, int max);
bool cfg_str(char *name, char *var, int size);
// if val starting with + append to a space delimited list
bool cfg_str_list(char *name, char *var, int size);
// split line to name = val and trim whitespace
void cfg_parseline(char *line, void (*set_func)(char*, char*));
bool cfg_parsebuf(char *buf, void (*set_func)(char*, char*));
bool cfg_parsefile_old(char *fname, void (*set_func)(char*, char*));
bool cfg_parsefile(char *fname, void (*set_func)(char*, char*));

#define CFG_STR(name, var) cfg_str(name, var, sizeof(var))
#define CFG_STR_LIST(name, var) cfg_str_list(name, var, sizeof(var))

extern char *cfg_name, *cfg_val;

#endif
