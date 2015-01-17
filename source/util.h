
// by oggzee

#ifndef _UTIL_H
#define _UTIL_H

#include <stdlib.h> // bool...
#include <gctypes.h> // bool...
#include <ogc/libversion.h>
#include <ogc/lwp_watchdog.h>

#include "mem.h"
#include "debug.h"
#include "strutil.h"

#define OGC_VER (_V_MAJOR_ * 100 + _V_MINOR_ * 10 + _V_PATCH_)

#if OGC_VER < 180
//extern void settime(long long);
//extern long long gettime();
extern u32 diff_msec(long long start, long long end);
extern u32 diff_usec(long long start, long long end);
#endif

#define D_S(A) A, sizeof(A)

void wiilight(int enable);

#include "memcheck.h"

#define MAX_USORT_MAP 1024
extern int usort_map[MAX_USORT_MAP];
extern int ufont_map[];
int map_ufont(int c);

int  mbs_len(const char *s);
bool mbs_trunc(char *mbs, int n);
char*mbs_align(const char *str, int n);
int  mbs_coll(char *a, char *b);
int  mbs_len_valid(char *s);
char *mbs_copy(char *dest, char *src, int size);

int con_len_wchar(wchar_t c);
int con_len_mbchar(char *mb);
int con_len(const char *s);
bool con_trunc(char *s, int n);
char*con_align(const char *str, int n);


static inline u32 _be32(const u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static inline u32 _le32(const void *d)
{
	const u8 *p = d;
	return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

static inline u32 _le16(const void *d)
{
	const u8 *p = d;
	return (p[1] << 8) | p[0];
}

void hex_dump1(void *p, int size);
void hex_dump2(void *p, int size);
void hex_dump3(void *p, int size);

typedef struct SoundInfo
{
	void *dsp_data;
	int size;
	int channels;
	int rate;
	int loop;
} SoundInfo;

void parse_banner_title(void *banner, u8 *title, s32 lang);
void parse_banner_snd(void *banner, SoundInfo *snd);
void parse_riff(void *data, SoundInfo *snd);

void printf_(const char *fmt, ...);
void printf_x(const char *fmt, ...);
void printf_h(const char *fmt, ...);

int mkpath(const char *s, int mode);

void con_wordwrap(char *str, int width, int size);
char* skip_lines(char *str, int line);
int print_lines(char *str, int line, int n);
int count_lines(char *str);
int print_page(char *str, int lines, int page, int *num);

typedef struct HashTable
{
	int size;
	int *table; // table of handles to key+value pairs
	void *cb; // callback data
	u32 (*hash_fun)(void *cb, void *key);
	bool (*compare_key)(void *cb, void *key, int handle);
	int* (*next_handle)(void *cb, int handle);
} HashTable;

int hash_init(HashTable *ht, int size, void *cb,
		u32 (*hash_fun)(void *cb, void *key),
		bool (*compare_key)(void *cb, void *key, int handle),
		int* (*next_handle)(void *cb, int handle)
		);

void hash_close(HashTable *ht);
void hash_add(HashTable *ht, void *key, int handle);
int hash_get(HashTable *ht, void *key);
void hash_remove(HashTable *ht, void *key, int a_handle);

static inline int hash_check_init(HashTable *ht, int size, void *cb,
		u32 (*hash_fun)(void *cb, void *key),
		bool (*compare_key)(void *cb, void *key, int handle),
		int* (*next_handle)(void *cb, int handle)
		)
{
	if (ht->table) return 0;
	return hash_init(ht, size, cb, hash_fun, compare_key, next_handle);
}

typedef struct HashMap
{
	HashTable ht;
	void *data;
	int num;
	int key_size;
	int val_size;
	int item_size;
} HashMap;

int hmap_init(HashMap *hmap, int key_size, int val_size);
void hmap_close(HashMap *hmap);
int hmap_add(HashMap *hmap, void *key, void *val);
void *hmap_get(HashMap *hmap, void *key);

u32 hash_string(const char *str_param);
u32 hash_string_n(const char *str_param, int n);
u32 hash_id4(void *cb, void *id);
u32 hash_id6(void *cb, void *id);

#endif

