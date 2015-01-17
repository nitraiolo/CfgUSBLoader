
// by oggzee

#ifndef _DEBUG_H
#define _DEBUG_H

struct timestats
{
	// loader startup
	long long boot1;
	long long boot2;
	long long intro1;
	long long intro2;
	long long ios11;
	long long ios12;
	long long ios21;
	long long ios22;

	long long sd_init1;
	long long sd_init2;
	long long sd_mount1;
	long long sd_mount2;
	long long usb_init1;
	long long usb_open;
	long long usb_cap;
	long long usb_init2;
	long long usb_mount1;
	long long usb_mount2;
	long long usb_retry1;
	long long usb_retry2;
	long long wpad1;
	long long wpad2;

	long long cfg1;
	long long cfg2;
	long long misc1;
	long long misc2;
	long long wiitdb1;
	long long wiitdb2;
	long long db_load1;
	long long db_load2;
	long long gamelist1;
	long long gamelist2;
	long long mp31;
	long long mp32;
	long long conbg1;
	long long conbg2;
	long long guitheme1;
	long long guitheme2;

	// run game
	long long run1;
	long long run2;
	long long rios1;
	long long rios2;
	long long gcard1;
	long long gcard2;
	long long playlog1;
	long long playlog2;
	long long playstat1;
	long long playstat2;
	long long load1;
	long long load2;

	long size;
};

extern struct timestats TIME;
#define DBG_LOG_SIZE 30720 // 10k
#define DBG_LOG_CUT  1024 // cut from end
extern char dbg_log_buf[DBG_LOG_SIZE];

void InitDebug();
int gecko_printf(const char *fmt, ... );
int dbg_printf(const char *fmt, ... );
int dbg_print(int level, const char *fmt, ...);

long long dbg_time_usec();
void dbg_time1();
unsigned dbg_time2(char *msg);
void get_time2(long long *t, char *s);
#define get_time(T) get_time2(T, #T)
void time_stats();
void time_stats2();
void time_statsf(FILE *f, char *str, int size);
#define TIME_D(X) (TIME.X##2 ? TIME.X##2 - TIME.X##1 : 0)
#define TIME_MS(X) (TIME.X##2 ? diff_msec(TIME.X##1,TIME.X##2) : 0)
void check_dol(char *name);

#endif

