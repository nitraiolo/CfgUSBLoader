#ifndef _MEM_H
#define _MEM_H

#include <stdio.h>
#include <ogcsys.h>

// max number of mem allocations in mem1/mem2 heaps
#define MAX_MEM_BLK 2048

typedef struct
{
	void  *ptr;
	int   size;
} mem_blk;

typedef struct
{
	int num;
	mem_blk list[MAX_MEM_BLK];
} blk_list;

typedef struct
{
	void *start;
	int size;
	blk_list used_list;
	blk_list free_list;
	mutex_t mutex;
} heap;

typedef struct
{
	int size;
	int used;
	int free;
	void *highptr; // end of last used block
} heap_stats;

// mem_blk
mem_blk* blk_find_size(blk_list *bl, int size);
mem_blk* blk_find_ptr(blk_list *bl, void *ptr);
mem_blk* blk_find_ptr_end(blk_list *bl, void *ptr);
mem_blk* blk_find_ptr_after(blk_list *bl, void *ptr);
mem_blk* blk_insert(blk_list *bl, mem_blk *b);
void     blk_remove(blk_list *bl, mem_blk *b);
mem_blk* blk_merge_add(blk_list *list, mem_blk *ab);

// heap
void* heap_alloc(heap *h, int size);
int   heap_free(heap *h, void *ptr);
int   heap_resize(heap *h, void *ptr, int size);
void* heap_realloc(heap *h, void *ptr, int size);
void  heap_init(heap *h, void *ptr, int size);
int   heap_ptr_inside(heap *h, void *ptr);
void  heap_stat(heap *h, heap_stats *s);

// mem
void  mem_init();
bool  mem_inside(int pool, void *ptr);
void* mem1_alloc(int size);
void* mem1_realloc(void *ptr, int size);
void* mem2_alloc(int size);
void* mem_alloc(int size);
void* mem_calloc(int size);
int   mem_resize(void *ptr, int size);
void* mem_realloc(void *ptr, int size);
void  mem_free(void *ptr);
void  mem_stat();
void  mem_stat_str(char *buffer, int size);
void  lib_info_str(char *str, int size);
void  lib_mem_stat_str(char *str, int size);
void  mem_statf(FILE *f);

// ogc sys
extern void* SYS_GetArena2Lo();
extern void* SYS_GetArena2Hi();
extern void* SYS_AllocArena2MemLo(u32 size,u32 align);
extern void* __SYS_GetIPCBufferLo();
extern void* __SYS_GetIPCBufferHi();

// util
void* LARGE_memalign(size_t align, size_t size);
void* LARGE_alloc(size_t size);
void* LARGE_realloc(void *ptr, size_t size);
void  LARGE_free(void *ptr);
void  memstat2();
void  util_init();
void  util_clear();

#define SAFE_FREE(P) if(P){mem_free(P);P=NULL;}

#define ALIGN_VAL 32

size_t xalign_up(int a, size_t s);
size_t xalign_down(int a, size_t s);
size_t align_up(size_t s);
size_t align_down(size_t s);

typedef struct obj_block
{
	void *ptr;
	int used;
	int size;
} obj_block;

typedef struct obj_allocator
{
	int chunk;
	void* (*m_realloc)(void *ptr, int size);
	int (*m_resize)(void *ptr, int size);
} obj_allocator;

#define OBS_MAX_BLOCKS 100

typedef struct obj_stack
{
	int num;
	obj_block block[OBS_MAX_BLOCKS];
	obj_allocator alloc;
} obj_stack;

void obb_init(obj_block *obb);
void *obb_alloc(obj_block *obb, obj_allocator *alloc, int size);
void obb_freeall(obj_block *obb, obj_allocator *alloc);
void obs_init(obj_stack *obs, int chunk,
		void* (*m_realloc)(void *ptr, int size),
		int (*m_resize)(void *ptr, int size));
void *obs_alloc(obj_stack *obs, int size);
void obs_freeall(obj_stack *obs);

#endif

