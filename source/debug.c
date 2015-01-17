
// by oggzee

#include <gccore.h>
#include <ogcsys.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

#include "cfg.h"
#include "console.h"
#include "fat.h"

struct timestats TIME;

int gecko_enabled = 0;

static long long dbg_t1, dbg_t2;
static long long dbg_t;

static int debug_inited = 0;
static int debug_logging = 1;
char dbg_log_buf[DBG_LOG_SIZE];

long long dbg_time_usec()
{
	long long t = gettime();
	long long d = diff_usec(dbg_t, t);
	dbg_t = t;
	return d;
}

void dbg_time1()
{
	dbg_t1 = gettime();
}

unsigned dbg_time2(char *msg)
{
	unsigned ms;
	dbg_t2 = gettime();
	ms = diff_msec(dbg_t1, dbg_t2);
	if (msg) printf("%s %u", msg, ms);
	dbg_t1 = dbg_t2;
	return ms;
}

int gecko_prints(const char *str)
{
	if (!gecko_enabled) return 0;
	return usb_sendbuffer(EXI_CHANNEL_1, str, strlen(str));
}

int gecko_printv(const char *fmt, va_list ap)
{
	char str[1024];
	int r;
	if (!gecko_enabled) return 0;
	r = vsnprintf(str, sizeof(str), fmt, ap);
	gecko_prints(str);
	return r;
}

int gecko_printf(const char *fmt, ... )
{
	va_list ap;
	int r;
	if (!gecko_enabled) return 0;
	va_start(ap, fmt);
	r = gecko_printv(fmt, ap);
	va_end(ap);
	return r;
}

int dbg_printv(int level, const char *fmt, va_list ap)
{
	char str[1024];
	int r = 0;
	if (!CFG.debug  && !CFG.debug_gecko && !debug_logging) return 0;
	r = vsnprintf(str, sizeof(str), fmt, ap);
	if (debug_logging) {
		int len1 = strlen(dbg_log_buf);
		int len2 = strlen(str);
		if (len1 + len2 >= sizeof(dbg_log_buf)) {
			// move text down
			int need = len1 + len2 + 1 - sizeof(dbg_log_buf);
			char *cut = dbg_log_buf + sizeof(dbg_log_buf) - DBG_LOG_CUT;
			memmove(cut, cut + need, DBG_LOG_CUT - need);
			memcpy(cut, "\n\n~~~\n\n", 7);
			dbg_log_buf[sizeof(dbg_log_buf) - need] = 0;
		}
		strappend(dbg_log_buf, str, sizeof(dbg_log_buf));
		/*
		if (strlen(dbg_log_buf) >= sizeof(dbg_log_buf) - 1) {
			// buf full, disable log
			debug_logging = 0;
		}*/
	}
	if (debug_inited) {
		if (CFG.debug >= level) {
			printf("%s", str);
		}
		if (gecko_enabled && (CFG.debug_gecko & 1)) {
			gecko_prints(str);
		}
	}
	return r;
}

int dbg_printf(const char *fmt, ... )
{
	va_list ap;
	int r = 0;
	va_start(ap, fmt);
	r = dbg_printv(1, fmt, ap);
	va_end(ap);
	return r;
}

int dbg_print(int level, const char *fmt, ...)
{
	va_list ap;
	int r = 0;
	va_start(ap, fmt);
	r = dbg_printv(level, fmt, ap);
	va_end(ap);
	return r;
}

/*
debug_gecko bits:
1 : all dbg_printf
2 : all printf
4 : only gecko_printf
*/

void InitDebug()
{
	if (CFG.debug_gecko & (1|4))
	{
		if (usb_isgeckoalive(EXI_CHANNEL_1)) {
			usb_flush(EXI_CHANNEL_1);
			if (!gecko_enabled) {
				gecko_enabled = 1;
				gecko_prints("\n\n====================\n\n");
			}
		}
	}
	if (CFG.debug_gecko & 2) {
		CON_EnableGecko(EXI_CHANNEL_1, 0);
	}
	debug_inited = 1;
	if (strlen(dbg_log_buf)) {
		gecko_printf(">>>\n%s\n<<<\n", dbg_log_buf);
	}
}

/* seconds in float */
float diff_fsec(long long t1, long long t2)
{
	return (float)diff_msec(t1, t2) / 1000.0;
}

void get_time2(long long *t, char *s)
{
	if (!t) return;
	if (*t == 0) *t = gettime();
	char c;
	char *p;
	char ss[32];
	char *dir;
	int len;
	long long tt = gettime();
	p = strchr(s, '.');
	if (p) p++; else p = s;
	STRCOPY(ss, p);
	len = strlen(ss) - 1;
	c = ss[len];
	if (c == '1') { dir = "-->"; ss[len] = 0; }
	else if (c == '2') { dir = "<--"; ss[len] = 0; }
	else return; //dir = "---";
	dbg_printf("[%.3f] %s %s\n", diff_fsec(TIME.boot1, tt), dir, ss);
	if (CFG.debug) __console_flush(0);
}

long tm_sum;

#define TIME_S(X) ((float)TIME_MS(X)/1000.0)

#define printx(...) do{ \
	if (f) fprintf(f,__VA_ARGS__); \
	if (str) { snprintf(str,size,__VA_ARGS__); str_seek_end(&str, &size); } \
   	gecko_printf(__VA_ARGS__); \
    }while(0)

#define print_t3(S,X,Y) do{\
	tm_sum += TIME_MS(X);\
	printx("%s: %.3f%s", S, TIME_S(X),Y);\
	}while(0)

#define print_t2(S,X) print_t3(S,X,"\n")
#define print_t_(X) print_t3(#X,X," ")
#define print_tn(X) print_t3(#X,X,"\n")


void time_statsf(FILE *f, char *str, int size)
{
	long sum;
	printx("times in seconds:\n");
	tm_sum = 0;
	print_t_(intro);
	print_tn(wpad);
	print_t_(ios1);
	print_tn(ios2);
	print_t_(sd_init);
	print_tn(sd_mount);
	print_t_(usb_init);
	print_t3("mount", usb_mount, " ");
	print_t3("retry", usb_retry, "\n");
	printx("open: %.3f ini: %.3f cap: %.3f\n",
		diff_fsec(TIME.usb_init1, TIME.usb_open),
		diff_fsec(TIME.usb_open, TIME.usb_cap),
		diff_fsec(TIME.usb_cap, TIME.usb_init2));
	print_t3("cfg", cfg, " (config,settings,titles,theme)\n");
	print_t3("misc", misc, " (lang,playstat,unifont)\n");
	print_t_(wiitdb);
	printx("load: %.3f parse: %.3f\n", TIME_S(db_load), TIME_S(wiitdb)-TIME_S(db_load));
	print_t_(gamelist);
	print_tn(mp3);
	print_t_(conbg);
	print_tn(guitheme);
	sum = tm_sum;
	printx("sum: %.3f ", (float)sum/1000.0);
	printx("uncounted: %.3f\n", (float)((long)TIME_MS(boot)-sum)/1000.0);
	print_t2("total startup", boot);
}

void time_stats()
{
	time_statsf(stdout, NULL, 0);
}

void time_stats2()
{
	FILE *f = stdout;
	char *str = NULL;
	int size = 0;
	long sum;
	if (!CFG.time_launch) {
		if (CFG.debug) goto out;
		return;
	}
	printx("times in seconds:\n");
	tm_sum = 0;
	print_tn(rios);
	print_tn(gcard);
	print_tn(playlog);
	print_tn(playstat);
	print_tn(load);
	sum = tm_sum;
	printx("sum: %.3f ", (float)sum/1000.0);
	printx("uncounted: %.3f\n", (float)((long)TIME_MS(run)-sum)/1000.0);
	print_t2("total launch", run);
	printx("size: %.2f MB\n", (float)TIME.size / 1024.0 / 1024.0);
	printx("speed: %.2f MB/s\n",
			(float)TIME.size / (float)TIME_MS(load) * 1000.0 / 1024.0 / 1024.0);
	out:;
	extern void Menu_PrintWait();
	Menu_PrintWait();
}

#if 0

// 7: text 11: data
typedef struct dolheader1
{
	u32 pos[18];
	u32 start[18];
	u32 size[18];
	u32 bss_start;
	u32 bss_size;
	u32 entry_point;
} dolheader1;

void compare_buf(int i, char *src, char *dest, int size)
{
	if (!size) return;
	int x;
	int e = 0; // error counter
	int esum = 0;
	char *estart = NULL;
	char *efirst = NULL;
	char *elast = NULL;
	int ecnt = 0;
	dbg_printf("[%d] %p %p %d\n", i, src, dest, size);
	for (x=0; x<size; x++) {
		if (src[x] != dest[x]) {
			if (!e) estart = dest + x;
			if (!efirst) efirst = dest + x;
			elast = dest + x;
			esum++;
			e++;
		} else {
			if (e && i < 7 && ecnt < 10) {
				dbg_printf("E: %p %d\n", estart, e);
				ecnt++;
			}
			e = 0;
		}
	}
	if (e && i < 7) dbg_printf("E: %p %d\n", estart, e);
	if (esum) dbg_printf("E tot: %d %p - %p\n", esum, efirst, elast);
}

void check_dol_buf(char *buf, int bufsize)
{
	dolheader1 *hdr = (dolheader1*)buf;
	int i;
	char *src, *dest;
	int size;
	for (i=0; i<18; i++) {
		size = hdr->size[i];
		src = buf + hdr->pos[i];
		dest = (char*)hdr->start[i];
		compare_buf(i, src, dest, size);
	}
}

void check_dol(char *name)
{
	void *buf = NULL;
	int size = -1;
	if (name) {
		dbg_printf("loading: %s\n", name);
		size = Fat_ReadFile(name, &buf);
		dbg_printf("size: %d\n", size);
	}
	if (size<0) {
		name = "sd:/apps/USBLoader/boot.dol";
		dbg_printf("loading: %s\n", name);
		size = Fat_ReadFile(name, &buf);
		dbg_printf("size: %d\n", size);
	}
	if (size < 0) return;
	check_dol_buf(buf, size);
	SAFE_FREE(buf);
}

#endif

