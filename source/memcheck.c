/////////////////////////////////
//
// mem check
//
// by oggzee

#include <ogcsys.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>

#define XM_NO_OVERRIDE
#include "memcheck.h"
#include "util.h"
#include "wpad.h"

#ifdef DEBUG_MEM_CHECK

extern int con_inited;

unsigned int xm_signature[] =
{
	0xdeadbeef, 0xdeadc0de, 0xc001babe, 0xf001b001,
	0x12345678, 0xabcdef99, 0xa1b2c3d4, 0xe5f67890
};

#define XM_SIG_SIZE sizeof(xm_signature)
#define XM_OVERHEAD (XM_SIG_SIZE+32)
#define XM_PTR_MAX 200

struct xm_ps
{
	void *ptr;
	size_t size;
	int line;
	char fun[20];
};

struct xm_ps xm_ptr[XM_PTR_MAX];
int xm_ptr_num;
int xm_ptr_use;
int xm_ptr_size;
char xm_fun[100];
int xm_line;

void xm_call(const char *fun, int line)
{
	STRCOPY(xm_fun, fun);
	xm_line = line;
}

void xm_print_fun2(char *fun, int line)
{
	if (con_inited) {
		printf("\nXM: %s : %d\n", fun, line);
	}
}

void xm_print_fun()
{
	xm_print_fun2(xm_fun, xm_line);
}

void xm_print_err(char *fmt, ...)
{
	if (con_inited)
	{
		char str[200];
		va_list argp;
		va_start(argp, fmt);
		vsnprintf(str, sizeof(str), fmt, argp);
		va_end(argp);
		xm_print_fun();
		printf("ERROR: %s\n", str);
		//sleep(5);
		int i, j;
		for (i=0; i<10; i++) {
			for (j=0; j<60; j++) {
				VIDEO_WaitVSync();
			}
			printf(". ");
		}
		//Wpad_WaitButtonsCommon();
		//*(char*)0=0; // crash
		//Sys_Exit();
		exit(0);
	}
}

int xm_find(void *ptr)
{
	int i;
	for (i=0; i<xm_ptr_num; i++) {
		if (xm_ptr[i].ptr == ptr) return i;
	}
	return -1;
}

void xm_add(void *ptr, size_t size)
{
	int i;
	if (ptr == NULL) return;
	i = xm_find(NULL);
	if (i < 0) {
		if (xm_ptr_num < XM_PTR_MAX) {
			i = xm_ptr_num;
			xm_ptr_num++;
		} else {
			return;
		}
	}
	xm_ptr[i].ptr = ptr;
	xm_ptr[i].size = size;
	xm_ptr[i].line = xm_line;
	STRCOPY(xm_ptr[i].fun, xm_fun);
	xm_ptr_use++;
	xm_ptr_size += size;
}

void xm_del(void *ptr)
{
	int i;
	if (ptr == NULL) return;
	i = xm_find(ptr);
	if (i < 0) return;
	xm_ptr_use--;
	xm_ptr_size -= xm_ptr[i].size;
	xm_ptr[i].ptr = NULL;
	xm_ptr[i].size = 0;
	while (xm_ptr_num > 0 && xm_ptr[xm_ptr_num-1].ptr == NULL) xm_ptr_num--;
}


void xm_mark(void *ptr, size_t size)
{
	if (ptr == NULL) {
		xm_print_err("alloc %p [%d]", ptr, size);
		return;
	}
	memcpy(ptr+size, xm_signature, XM_SIG_SIZE);
	xm_add(ptr, size);
}

int xm_cmp(void *ptr, size_t size)
{
	return memcmp(ptr+size, xm_signature, XM_SIG_SIZE);
}

int xm_check(struct xm_ps *xx)
{
	if (xx->ptr == 0) return 0;
	if (xm_cmp(xx->ptr, xx->size) == 0) return 0;
	xm_print_err("ptr %p [%d]\n(%s : %d)\n",
			xx->ptr, xx->size, xx->fun, xx->line);
	return -1;
}

void xm_check_all()
{
	int i;
	for (i=0; i<xm_ptr_num; i++) {
		xm_check(&xm_ptr[i]);
	}
	mallinfo();
}

void *xm_malloc(size_t size)
{
	xm_check_all();
	void *p = malloc(size + XM_OVERHEAD);
	xm_mark(p, size);
	xm_check_all();
	return p;
}

void *xm_memalign(size_t align, size_t size)
{
	xm_check_all();
	void *p = memalign(align, size + XM_OVERHEAD);
	if (!p) return p;
	xm_mark(p, size);
	xm_check_all();
	return p;
}

void *xm_realloc(void *ptr, size_t size)
{
	xm_check_all();
	xm_del(ptr);
	void *p = realloc(ptr, size + XM_OVERHEAD);
	xm_mark(p, size);
	xm_check_all();
	return p;
}

void *xm_calloc(size_t num, size_t size)
{
	//void *p = calloc(num, size);
	size_t s = num * size;
	void *p = xm_malloc(s);
	if (p) memset(p, 0, s);
	return p;
}

void xm_free(void *ptr)
{
	int i;
	xm_check_all();
	i = xm_find(ptr);
	if (i < 0 || ptr == NULL) {
		xm_print_err("free unk %p", ptr);
	}
	free(ptr);
	xm_del(ptr);
	xm_check_all();
}

void xm_memcheck_ptr(void *base, void *ptr)
{
	xm_check_all();
	mallinfo();
	if (base == NULL || ptr == NULL || ptr < base) {
		xm_print_err("access %p %p", base, ptr);
		return;
	}
	int i;
	i = xm_find(base);
	if (i < 0) {
		xm_print_err("unk %p %p", base, ptr);
		return;
	}
	if (ptr - base > xm_ptr[i].size) {
		xm_print_err("access %p %p [%d]", base, ptr, xm_ptr[i].size);
	}
}

void memstat()
{
	int i;
	xm_check_all();
	printf("\n");
	malloc_stats();
	printf("XM: %d / %d %d\n", xm_ptr_use, xm_ptr_num, xm_ptr_size);
	// print larger than 128kb allocs
	for (i=0; i<xm_ptr_num; i++) {
		if (xm_ptr[i].size > 128*1024) {
			printf("%p %4dk %-20.20s %d\n",
					xm_ptr[i].ptr, xm_ptr[i].size/1024,
					xm_ptr[i].fun, xm_ptr[i].line);
		}
	}
}

void memcheck()
{
	xm_check_all();
	mallinfo();
}

#else

void memstat()
{
	//malloc_stats();
}

void memcheck()
{
	//mallinfo();
}

#endif



