// by oggzee
#ifndef _STRUTIL_H
#define _STRUTIL_H 1

#include <ctype.h>
#include <gctypes.h>

char* strcopy(char *dest, const char *src, int size);
char *strappend(char *dest, const char *src, int size);
bool str_replace(char *str, char *olds, char *news, int size);
bool str_replace_all(char *str, char *olds, char *news, int size);
bool str_replace_tag_val(char *str, char *tag, char *val);
char* trimcopy_n(char *dest, char *src, int n, int size);
char* trimcopy(char *dest, char *src, int size);
char* split_token(char *dest, char *src, char delim, int size);
char* split_tokens(char *dest, char *src, char *delim, int size);
bool trimsplit(char *line, char *part1, char *part2, char delim, int size);
void unquote(char *dest, char *str, int size);
void str_insert(char *str, char c, int n, int size);
void str_insert_at(char *str, char *pos, char c, int n, int size);
char* str_seek_end(char **str, int *size);

#define ISALNUM(c)  (isalnum((int)(unsigned char)(c)))
#define ISALPHA(c)  (isalpha((int)(unsigned char)(c)))
#define ISDIGIT(c)  (isdigit((int)(unsigned char)(c)))
#define ISXDIGIT(c) (isxdigit((int)(unsigned char)(c)))
#define ISSPACE(c)  (isspace((int)(unsigned char)(c)))
#define ISLOWER(c)  (islower((int)(unsigned char)(c)))
#define ISCNTRL(c)  (iscntrl((int)(unsigned char)(c)))

#define STRCOPY(DEST,SRC) strcopy(DEST,SRC,sizeof(DEST)) 
#define STRAPPEND(DST,SRC) strappend(DST,SRC,sizeof(DST))


#endif
