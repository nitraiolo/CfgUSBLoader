#ifndef __DEBUG_H__
#define __DEBUG_H__

void s_printf(char *format,...);

#ifdef DEBUG
#define dbg_printf s_printf
#else
#define dbg_printf(...)
#endif

#endif
