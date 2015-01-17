// by oggzee
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "strutil.h"
#include "util.h"
#include "debug.h"

char* strcopy(char *dest, const char *src, int size)
{
	if (size > 0) {
		strncpy(dest,src,size);
		dest[size-1] = 0;
	}
	return dest;
}

char *strappend(char *dest, const char *src, int size)
{
	int len = strlen(dest);
	strcopy(dest+len, src, size-len);
	return dest;
}

bool str_replace(char *str, char *olds, char *news, int size)
{
	char *p;
	int len;
	p = strstr(str, olds);
	if (!p) return false;
	// new len
	len = strlen(str) - strlen(olds) + strlen(news);
	// check size
	if (len >= size) return false;
	// move remainder to fit (and nul)
	memmove(p+strlen(news), p+strlen(olds), strlen(p)-strlen(olds)+1);
	// copy new in place
	memcpy(p, news, strlen(news));
	// terminate
	str[len] = 0;
	return true;
}

bool str_replace_all(char *str, char *olds, char *news, int size) {
	int cnt = 0;
	bool ret = str_replace(str, olds, news, size);
	while (ret) {
		ret = str_replace(str, olds, news, size);
		cnt++;
	}
	return (cnt > 0);
}

bool str_replace_tag_val(char *str, char *tag, char *val)
{
	char *p, *end;
	p = strstr(str, tag);
	if (!p) return false;
	p += strlen(tag);
	end = strstr(p, "</");
	if (!end) return false;
	dbg_printf("%s '%.*s' -> '%s'\n", tag, end-p, p, val);
	// make space for new val
	memmove(p+strlen(val), end, strlen(end)+1); // +1 for 0 termination
	// copy over new val
	memcpy(p, val, strlen(val));
	return true;
}


// trim leading and trailing whitespace
// copy at max n or at max size-1
char* trimcopy_n(char *dest, char *src, int n, int size)
{
	int len;
	// trim leading white space
	while (ISSPACE(*src) && n>0) { src++; n--; }
	len = strlen(src);
	if (len > n) len = n;
	// trim trailing white space
	while (len > 0 && ISSPACE(src[len-1])) len--;
	// limit length
	if (len >= size) len = size-1;
	// safety
	if (len < 0) len = 0;
	strncpy(dest, src, len);
	dest[len] = 0;
	//printf("trim_copy: '%s' %d\n", dest, len); //sleep(1);
	return dest;
}

char* trimcopy(char *dest, char *src, int size)
{
	return trimcopy_n(dest, src, size, size);
}

// store token in dest and return ptr to next token
// on last token returns null
// note: omits empty tokens
char* split_token(char *dest, char *src, char delim, int size)
{
	char *next;
	start:
	next = strchr(src, delim);
	if (next) {
		trimcopy_n(dest, src, next-src, size);
		next++;
	} else {
		trimcopy(dest, src, size);
	}
	if (next && *dest == 0) {
		// omit empty tokens
		src = next;
		goto start;
	}
	return next;
}

// store token in dest and return ptr to next token
// on last token returns ptr to end of string
// when no more tokens returns null
char* split_tokens(char *dest, char *src, char *delim, int size)
{
	char *next;
	start:
	// end of string?
	if (*src == 0) return NULL;
	// strcspn:
	// returns the number of characters of str1 read before this first occurrence.
	// The search includes the terminating null-characters, so the function will
	// return the length of str1 if none of the characters of str2 are found in str1.
	next = src + strcspn(src, delim);
	if (*next) {
		trimcopy_n(dest, src, next-src, size);
		next++;
	} else {
		trimcopy(dest, src, size);
	}
	if (*dest == 0) {
		// omit empty tokens
		src = next;
		goto start;
	}
	return next;
}

// split line to part1 delimiter part2 
bool trimsplit(char *line, char *part1, char *part2, char delim, int size)
{
	char *eq = strchr(line, delim);
	if (!eq) return false;
	trimcopy_n(part1, line, eq-line, size);
	trimcopy(part2, eq+1, size);
	return true;
}

void unquote(char *dest, char *str, int size)
{
	// unquote "xx"
	if (str[0] == '"' && str[strlen(str)-1] == '"') {
		int len = strlen(str) - 2;
		if (len > size-1) len = size-1;
		if (len < 0) len = 0;
		strcopy(dest, str+1, len + 1);
	} else {
		strcopy(dest, str, size);
	}
}

// replace all whitespace (space, cr, lf, tab) with a single space
// size not needed since the resulting length will be equal or smaller
void str_flatten(char *str)
{
	char *p = str;
	char *n = str;
	// is previous char space?
	// start with true so that leading space is skipped
	bool prev_space = true;
	while (*n) {
		if (ISSPACE(*n)) {
			if (!prev_space) {
				*p = ' ';
				p++;
			}
			prev_space = true;
		} else {
			*p = *n;
			p++;
			prev_space = false;
		}
		n++;
	}
	if (prev_space && p > str) {
		// remove trailing space
		p--;
	}
	*p = 0;
}

void str_insert(char *str, char c, int n, int size)
{
	int len = strlen(str);
	if (size < 2) return;
	if (n < 1) return;
	if (n >= size) n = size - 1;
	if (len + n >= size) {
		len = size - 1 - n;
	}
	memmove(str + n, str, len);
	str[len + n] = 0;
	// fill with c
	memset(str, c, n);
}

void str_insert_at(char *str, char *pos, char c, int n, int size)
{
	str_insert(pos, c, n, size - (pos - str));
}

char* str_seek_end(char **str, int *size)
{
	int len = strlen(*str);
	*size -= len;
	*str += len;
	return *str;
}


