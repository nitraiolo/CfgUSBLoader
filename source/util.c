
// by oggzee

#include <ogcsys.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <stdio.h>
#include <ctype.h>
//#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>

#include "console.h"
#include "menu.h"
#include "util.h"

// Thanks Dteyn for this nice feature =)
// Toggle wiilight (thanks Bool for wiilight source)
void wiilight(int enable)
{
	static vu32 *_wiilight_reg = (u32*)0xCD0000C0;
    u32 val = (*_wiilight_reg&~0x20);        
    if(enable) val |= 0x20;             
    *_wiilight_reg=val;            
}


int mbs_len(const char *s)
{
	int count = 0;
	int n;
	while (*s) {
		n = mblen(s, MB_LEN_MAX);
		if (n == 0) break;
		if (n < 0) {
			// invalid char, ignore
			n = 1;
		}
		s += n;
		count++;
	}
	return count;
}

int mbs_len_valid(char *s)
{
	int count = 0;
	int n;
	while (*s) {
		n = mblen(s, MB_LEN_MAX);
		if (n <= 0) {
			// invalid char, stop
			break;
		}
		s += n;
		count++;
	}
	return count;
}

char *mbs_copy(char *dest, char *src, int size)
{
	char *s;
	int n;
	strcopy(dest, src, size);
	s = dest;
	while (*s) {
		n = mblen(s, MB_LEN_MAX);
		if (n <= 0) {
			// invalid char, stop
			*s = 0;
			break;
		}
		s += n;
	}
	return dest;
}

bool mbs_trunc(char *mbs, int n)
{
	int len = mbs_len(mbs);
	if (len <= n) return false;
	int slen = strlen(mbs);
	wchar_t wbuf[n+1];
	wbuf[0] = 0;
	mbstowcs(wbuf, mbs, n);
	wbuf[n] = 0;
	wcstombs(mbs, wbuf, slen+1);
	return true;
}

char *mbs_align(const char *str, int n)
{
	static char strbuf[100];
	if (strlen(str) >= sizeof(strbuf) || n >= sizeof(strbuf)) return (char*)str;
	// fill with space
	memset(strbuf, ' ', sizeof(strbuf));
	// overwrite with str, keeping trailing space
	memcpy(strbuf, str, strlen(str));
	// terminate
	strbuf[sizeof(strbuf)-1] = 0;
	// truncate multibyte string
	mbs_trunc(strbuf, n);
	return strbuf;
}

int mbs_coll(char *a, char *b)
{
	//int lena = strlen(a);
	//int lenb = strlen(b);
	int lena = mbs_len_valid(a);
	int lenb = mbs_len_valid(b);
	wchar_t wa[lena+1];
	wchar_t wb[lenb+1];
	int wlen, i;
	int sa, sb, x;
	wlen = mbstowcs(wa, a, lena);
	wa[wlen>0?wlen:0] = 0;
	wlen = mbstowcs(wb, b, lenb);
	wb[wlen>0?wlen:0] = 0;
	for (i=0; wa[i] || wb[i]; i++) {
		sa = wa[i];
		if ((unsigned)sa < MAX_USORT_MAP) sa = usort_map[sa];
		sb = wb[i];
		if ((unsigned)sb < MAX_USORT_MAP) sb = usort_map[sb];
		x = sa - sb;
		if (x) return x;
	}
	return 0;
}

int con_len_wchar(wchar_t c)
{
	int cc;
	int len;
	if (c < 512) return 1;
	cc = map_ufont(c);
	if (cc != 0) return 1;
	if (c < 0 || c > 0xFFFF) return 1;
	if (unifont == NULL) return 1;
	len = unifont->index[c] & 0x0F;
	if (len < 1) return 1;
	if (len > 2) return 2;
	return len;
}

int con_len_mbchar(char *mb)
{
	wchar_t wc;
	int cc;
	int len;
	int n;
	n = mbtowc(&wc, mb, MB_LEN_MAX);
	if (n < 0) return 1;
	if (n == 0) return 0;
	if (wc < 512) return 1;
	cc = map_ufont(wc);
	if (cc != 0) return 1;
	if (wc < 0 || wc > 0xFFFF) return 1;
	if (unifont == NULL) return 1;
	len = unifont->index[wc] & 0x0F;
	if (len < 1) return 1;
	if (len > 2) return 2;
	return len;
}

int con_len(const char *s)
{
	if (!s) return 0;
	int i, len = 0;
	int n = mbs_len(s);
	wchar_t wbuf[n+1];
	wbuf[0] = 0;
	mbstowcs(wbuf, s, n);
	wbuf[n] = 0;
	for (i=0; i<n; i++) {
		len += con_len_wchar(wbuf[i]);
	}
	return len;
}

bool con_trunc(char *s, int n)
{
	int slen = strlen(s);
	int i, len = 0;
	wchar_t wbuf[n+1];
	wbuf[0] = 0;
	mbstowcs(wbuf, s, n);
	wbuf[n] = 0;
	for (i=0; i<n; i++) {
		len += con_len_wchar(wbuf[i]);
		if (len > n) break;
	}
	wbuf[i] = 0; // terminate;
	wcstombs(s, wbuf, slen+1);
	return (len > n); // true if truncated
}

char *con_align(const char *str, int n)
{
	static char strbuf[100];
	if (strlen(str) >= sizeof(strbuf) || n >= sizeof(strbuf)) return (char*)str;
	// fill with space
	memset(strbuf, ' ', sizeof(strbuf));
	// overwrite with str, keeping trailing space
	memcpy(strbuf, str, strlen(str));
	// terminate
	strbuf[sizeof(strbuf)-1] = 0;
	// truncate multibyte string
	con_trunc(strbuf, n);
	while (con_len(strbuf) < n) strcat(strbuf, " ");
	return strbuf;
}


int map_ufont(int c)
{
	int i;
	if ((unsigned)c < 512) return c;
	for (i=0; ufont_map[i]; i+=2) {
		if (ufont_map[i] == c) return ufont_map[i+1];
	}
	return 0;
}

// FFx y AABB
void hex_dump1(void *p, int size)
{
	char *c = p;
	int i;
	for (i=0; i<size; i++) {
		unsigned cc = (unsigned char)c[i];
		if (cc < 32 || cc > 128) {
			printf("%02x", cc);
		} else {
			printf("%c ", cc);
		}
	}	
}

// FF 40 41 AA BB | .xy..
void hex_dump2(void *p, int size)
{
	int i = 0, j, x = 12;
	char *c = p;
	dbg_printf("\n");
	while (i<size) {
		dbg_printf("%02x ", i);
		for (j=0; j<x && i+j<size; j++) dbg_printf("%02x", (int)c[i+j]);
		dbg_printf(" |");
		for (j=0; j<x && i+j<size; j++) {
			unsigned cc = (unsigned char)c[i+j];
			if (cc < 32 || cc > 128) cc = '.';
			dbg_printf("%c", cc);
		}
		dbg_printf("|\n");
		i += x;
	}	
}

// FF4041AABB 
void hex_dump3(void *p, int size)
{
	int i = 0, j, x = 16;
	char *c = p;
	while (i<size) {
		printf_("");
		for (j=0; j<x && i+j<size; j++) printf("%02x", (int)c[i+j]);
		printf("\n");
		i += x;
	}	
}


/* Copyright 2005 Shaun Jackman
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */
//This code from dirname.c, meant to be part of libgen, modified by Clipper
char* dirname(char *path)
{
	char *p;
	if( path == NULL || *path == '\0' )
		return ".";
	p = path + strlen(path) - 1;
	while( *p == '/' ) {
		if( p == path )
			return path;
		*p-- = '\0';
	}
	while( p >= path && *p != '/' )
		p--;
	return
		p < path ? "." :
		p == path ? "/" :
		*(p-1) == ':' ? "/" :
		(*p = '\0', path);
}


// code modified from http://niallohiggins.com/2009/01/08/mkpath-mkdir-p-alike-in-c-for-unix/
int mkpath(const char *s, int mode){
        char *up, *path;
        int rv;
 
        rv = -1;

        if (strcmp(s, ".") == 0 || strcmp(s, "/") == 0)
                return (0);
 
		if ((path = strdup(s)) == NULL)
			return -1;

        if ((up = dirname(path)) == NULL)
                goto out;

        if ((mkpath(up, mode) == -1) && (errno != EEXIST))
                goto out;
 
        if ((mkdir(s, mode) == -1) && (errno != EEXIST))
                rv = -1;
        else
                rv = 0;
out:
		if (path != NULL)
			SAFE_FREE(path);
        return (rv);
}

// replace spaces with \n so that text fits width
// or insert \n in the middle of a word if word is longer than width/2
void con_wordwrap(char *str, int width, int size)
{
	char *s = str;
	char *e = NULL; // last white space
	int w = 0; // line len
	int wordlen = 0; // word len
	int clen; // char len (1 or 2)
	int n;
	while (*s) {
		n = mblen(s, MB_LEN_MAX);
		if (n <= 0) {
			// terminate at invalid utf8 sequence
			*s = 0;
			break;
		}
		if (*s == '\n') {
			found_nl:
			w = 0;
			e = NULL;
			s++;
			continue;
		}
		clen = con_len_mbchar(s);
		if (ISSPACE(*s)) {
			e = s;
			wordlen = 0;
		} else {
			wordlen += clen;
		}
		w += clen;
		if (w > width) {
			// wrap if word len is shorter than half width
			// otherwise break word
			if (e && wordlen < width/2) {
				// wrap line at last space
				s = e;
				*s = '\n';
			} else {
				// break word by inserting nl
				str_insert_at(str, s, '\n', 1, size);
			}
			goto found_nl;
		}
		if (s + n >= str + size) {
			*s = 0;
		}
		s += n;
	}
}

char* skip_lines(char *str, int line)
{
	char *s = str;
	while (line) {
		s = strchr(s, '\n');
		if (!s) return NULL;
		s++;
		line--;
	}
	return s;
}

// line starts with 0
// returns n - printed lines
int print_lines(char *str, int line, int n)
{
	char *s = skip_lines(str, line);
	if (!s) return n;
	char *e = s;
	while (n) {
		e = strchr(e, '\n');
		if (!e) break;
		e++;
		n--;
	}
	if (e) {
		int len = e - s;
		printf("%.*s", len, s);
	} else {
		printf("%s\n", s);
		n--;
	}
	return n;
}

// if trailing nl is missing it is assumed
// so "a\nb\n" and "a\nb" both count as 2 lines
int count_lines(char *str)
{
	char *s = str;
	int n = 1;
	while (*s) {
		if (*s == '\n') n++;
		s++;
	}
	if (s > str) {
		if (s[-1] == '\n') n--;
	}
	return n;
}

// page starts with 0
int print_page(char *str, int lines, int page, int *num)
{
	int n;
	int overlap = 1;
	lines -= overlap;
	if (lines < 1) return -1;
	if (*num == 0) {
		*num = 1 + (count_lines(str) - 1) / lines;
	}
	n = print_lines(str, page * lines, lines + overlap);
	while (n) {
		printf("\n");
		n--;
	}
	return 0;
}

int hash_init(HashTable *ht, int size, void *cb,
		u32 (*hash_fun)(void *cb, void *key),
		bool (*compare_key)(void *cb, void *key, int handle),
		int* (*next_handle)(void *cb, int handle)
		)
{
	int i;
	memset(ht, 0, sizeof(HashTable));
	if (size == 0) size = 4999; // default size
	ht->table = calloc(size, sizeof(*ht->table));
	if (!ht->table) return -1;
	for (i=0; i<size; i++) {
		ht->table[i] = -1;
	}
	ht->size = size;
	ht->cb = cb;
	ht->hash_fun = hash_fun;
	ht->compare_key = compare_key;
	ht->next_handle = next_handle;
	return 0;
}

void hash_close(HashTable *ht)
{
	SAFE_FREE(ht->table);
	memset(ht, 0, sizeof(HashTable));
}

void hash_add(HashTable *ht, void *key, int handle)
{
	//dbg_printf("hash_add(%.6s, %d)\n", key, handle);
	if (!ht->table) return;
	u32 hh = ht->hash_fun(ht->cb, key);
	int hi = hh % ht->size;
	int *next = ht->next_handle(ht->cb, handle);
	if (next) {
		*next = ht->table[hi];
	} else {
		dbg_printf("ERROR hnext\n");
	}
	ht->table[hi] = handle;
}

int hash_get(HashTable *ht, void *key)
{
	//dbg_printf("hash_get(%.6s)\n", key);
	if (!ht->table) return -1;
	u32 hh = ht->hash_fun(ht->cb, key);
	int hi = hh % ht->size;
	int handle = ht->table[hi];
	int n = 0;
	while (handle >= 0) {
		if (ht->compare_key(ht->cb, key, handle)) return handle;
		int *next = ht->next_handle(ht->cb, handle);
		if (next) {
			handle = *next;
		} else {
			dbg_printf("ERROR hnext\n");
			handle = -1;
		}
		n++;
		if (n > 10000) {
			dbg_printf("hash loop! %.6s\n", key);
			return -1;
		}
	}
	return -1;
}

int hash_getx(HashTable *ht, void *key)
{
	if (!ht->table) return -1;
	u32 hh = ht->hash_fun(ht->cb, key);
	int hi = hh % ht->size;
	int *prev = NULL;
	int handle = ht->table[hi];
	while (handle >= 0) {
		if (ht->compare_key(ht->cb, key, handle)) {
			if (prev) {
				// push result to front so that next time it's faster
				int *next = ht->next_handle(ht->cb, handle);
				if (next) {
					*prev = *next;
					*next = ht->table[hi];
					ht->table[hi] = handle;
				} else {
					dbg_printf("ERROR hnext\n");
				}
			}
			return handle;
		}
		prev = ht->next_handle(ht->cb, handle);
		handle = *prev;
	}
	return -1;
}

void hash_remove(HashTable *ht, void *key, int a_handle)
{
	//dbg_printf("hash_remove(%.6s, %d)\n", key, handle);
	if (!ht->table) return;
	u32 hh = ht->hash_fun(ht->cb, key);
	int hi = hh % ht->size;
	int *prev = &ht->table[hi];
	int handle = *prev;
	int n = 0;
	while (handle >= 0) {
		if (handle == a_handle) {
			// remove handle from linked list
			*prev = *ht->next_handle(ht->cb, handle);
			return;
		}
		prev = ht->next_handle(ht->cb, handle);
		if (!prev) {
			// error: handle not found
			return;
		}
		handle = *prev;
		n++;
		if (n > 10000) {
			dbg_printf("hash loop! %.6s\n", key);
			return;
		}
	}
	return;
}

#define HMAP_ITEM(HM,N) (HM->data + HM->item_size * N)
#define HMAP_NEXT(HM,N) HMAP_ITEM(HM,N)
#define HMAP_KEY(HM,N) (HMAP_ITEM(HM,N) + sizeof(int))
#define HMAP_VAL(HM,N) (HMAP_ITEM(HM,N) + sizeof(int) + HM->key_size)

u32 hmap_hash(void *cb, void *key)
{
	HashMap *hmap = cb;
	return hash_string_n(key, hmap->key_size);
}

bool hmap_cmp(void *cb, void *key, int handle)
{
	HashMap *hmap = cb;
	return memcmp(key, HMAP_KEY(hmap, handle), hmap->key_size) == 0;
}

int* hmap_next(void *cb, int handle)
{
	int *p = HMAP_NEXT(((HashMap*)cb), handle);
	//dbg_printf("next(%d) = %p\n", handle, p);
	return p;
}

int hmap_init(HashMap *hmap, int key_size, int val_size)
{
	int ret;
	memset(hmap, 0, sizeof(HashMap));
	ret = hash_init(&hmap->ht, 0, hmap, &hmap_hash, &hmap_cmp, &hmap_next);
	if (ret) return ret;
	hmap->key_size = key_size;
	hmap->val_size = val_size;
	int item_size = sizeof(int) + hmap->key_size + hmap->val_size;
	hmap->item_size = xalign_up(sizeof(int), item_size);
	return 0;
}

void hmap_close(HashMap *hmap)
{
	SAFE_FREE(hmap->data);
	hash_close(&hmap->ht);
}

int hmap_add(HashMap *hmap, void *key, void *val)
{
	//dbg_printf("hmap_add(%.6s, %d)\n", key, *(int*)val);
	void *ptr;
	int n = hmap->num;
	int size = hmap->item_size * (n + 1);
	ptr = realloc(hmap->data, size);
	if (!ptr) return -1;
	hmap->data = ptr;
	hmap->num++;
	hash_add(&hmap->ht, key, n);
	//dbg_printf("%d data: %p key: %p val: %p\n", n, hmap->data, HMAP_KEY(hmap,n), HMAP_VAL(hmap,n));
	memcpy(HMAP_KEY(hmap,n), key, hmap->key_size);
	memcpy(HMAP_VAL(hmap,n), val, hmap->val_size);
	return 0;
}

void *hmap_get(HashMap *hmap, void *key)
{
	//dbg_printf("hmap_get(%.6s)\n", key);
	int n = hash_get(&hmap->ht, key);
	if (n == -1) return NULL;
	return HMAP_VAL(hmap,n);
}

