#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdint.h>

#define PAGE_SIZE 4096
#define MEM_SIZE (PAGE_SIZE*1000) /* 4 Mib */

struct Mem{
	void *p;
	uint64_t size;
};/*16 bytes*/


enum types{
	INT,
	STRING,
	LONG,
	DOUBLE,
	FLOAT
};

/* API endpoints */
int create_memory(struct Mem *memory, uint64_t size, int type);
int cancel_memory(struct Mem *memory,void *start,size_t size);
int expand_memory(struct Mem *memory, size_t size,int type);
int create_arena(size_t size);
int init_prog_memory();
void close_prog_memory();
int close_arena();
int push(struct Mem *memory,void* value,int type);
void *value_at_index(uint64_t index,struct Mem *memory,int type);

void *ask_mem(size_t size);
void *reask_mem(void *p,size_t old_size,size_t size);
/*internal function*/

#endif
