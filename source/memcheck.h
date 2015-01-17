
// by oggzee

#ifndef _MEMCHECK_H
#define _MEMCHECK_H

void memstat();
void memcheck();

//#define DEBUG_MEM_CHECK

#if defined(DEBUG_MEM_CHECK) && !defined(XM_NO_OVERRIDE)

#include <stdlib.h>
#include <malloc.h>

void  xm_call(const char *fun, int line);
void* xm_malloc(size_t size);
void* xm_memalign(size_t align, size_t size);
void* xm_realloc(void *ptr, size_t size);
void* xm_calloc(size_t num, size_t size);
void  xm_free(void *ptr);
void  xm_memcheck_ptr(void *base, void *ptr);

#define XM_OVERRIDE(F, ARGS...) (xm_call(__FUNCTION__, __LINE__), xm_##F(ARGS))
#define malloc(S)      XM_OVERRIDE(malloc, S)
#define memalign(A, S) XM_OVERRIDE(memalign, A, S)
#define realloc(P, S)  XM_OVERRIDE(realloc, P, S)
#define calloc(N, S)   XM_OVERRIDE(calloc, N, S)
#define free(P)        XM_OVERRIDE(free, P)
#define memcheck_ptr(B, P) XM_OVERRIDE(memcheck_ptr, B, P)

#else

#define memcheck_ptr(B,P) do{}while(0)

#endif


#endif


