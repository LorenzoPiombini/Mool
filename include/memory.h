#ifndef _MEMORY_H
#define _MEMORY_H

#include <types.h>

#define PAGE_SIZE 4096
#define MEM_SIZE (PAGE_SIZE*1000) /* 4 Mib */
#define ONE_Mib (1024 * 1000)
#define FIVE_H_Kib (1024 * 500)
#define EIGTH_Kib (1024 * 8)
#define SMEM_EXIST 2
#define SMEM_NOENT 3

struct Mem{
	void *p;
	size_t size;
};/*16 bytes*/

struct arena{
	void *p;
	uint64_t size;
	uint64_t bwritten;
};

enum types{
	INT,
	STRING,
	LONG,
	DOUBLE,
	FLOAT
};

/* API endpoints */
void *get_arena(size_t *size);
int is_inside_arena(size_t size,struct arena a);
void clear_memory();
int create_shared_memory(size_t size);
int read_from_shared_memory();
//int write_to_shared_memory(pid_t pid, void* value);
void close_shared_memory();
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
