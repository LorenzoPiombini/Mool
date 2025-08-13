#if defined(__linux__)
	#include <sys/mman.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <log.h>
#include "memory.h"

#define _LOG_ !log ? stderr : log
int e = -1; /*returning error*/
static int8_t mem_safe = 0;
static int8_t *prog_mem = NULL;
static int8_t *last_addr = NULL;
static struct Mem *free_memory = NULL;
static void **memory_info = NULL;
static uint32_t memory_info_size = 0;
static FILE *log = NULL;

/*static functions*/
static int is_free(void *mem, size_t size);
static int resize_memory_info();



void init_prog_memory()
{

	log = open_log_file();
#if defined(__linux__)
	prog_mem = (int8_t *)mmap(NULL,sizeof (int8_t) * (MEM_SIZE + PAGE_SIZE), PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
	if(prog_mem == MAP_FAILED){
		fprintf(log,"mmap, failed, fallback malloc used.\n");
		prog_mem = NULL;
		prog_mem = malloc(MEM_SIZE*sizeof(int8_t));
		if(!prog_mem) 
			fprintf(_LOG_,"fall back system using malloc failed, %s:%d.\n"
					,__FILE__,__LINE__-2);

		assert(prog_mem != NULL);
		memset(prog_mem,0,MEM_SIZE * sizeof(int8_t));

		free_memory = malloc(sizeof(struct Mem) * (PAGE_SIZE / sizeof (struct Mem)));
		if(!free_memory) 
			fprintf(_LOG_,"cannot allocate memory for free_memory data, %s:%d.\n"
					,__FILE__,__LINE__-1);

		assert(free_memory != NULL);
		memset(free_memory,0,sizeof(struct Mem) * (PAGE_SIZE / sizeof(struct Mem)));
		return;
	}
	/*we insert a page after the memory so we get a SIGSEGV if we overflow*/
	if(mprotect(prog_mem + MEM_SIZE,PAGE_SIZE,PROT_NONE) == -1){
		fprintf(_LOG_,"memory protection failed.\n");
	}

	free_memory = malloc(sizeof(struct Mem) * (PAGE_SIZE / sizeof(struct Mem)));
	if(!free_memory) 
		fprintf(_LOG_,"cannot allocate memory for free_memory data, %s:%d.\n"
				,__FILE__,__LINE__-1);

	assert(free_memory != NULL);
	memset(free_memory,0,sizeof(struct Mem) * (PAGE_SIZE / sizeof(struct Mem)));
	mem_safe = 1;
#else
	prog_mem = malloc(MEM_SIZE*sizeof(int8_t));
	if(!prog_mem) 
		fprintf(_LOG_,"fall back system using malloc failed, %s:%d.\n"
				,__FILE__,__LINE__-2);

	assert(prog_mem != NULL);
	memset(prog_mem,0,MEM_SIZE * sizeof(int8_t));

	free_memory = malloc(sizeof(struct Mem) * (PAGE_SIZE / sizeof (struct Mem)));
	if(!free_memory) 
		fprintf(_LOG_,"cannot allocate memory for free_memory data, %s:%d.\n"
				,__FILE__,__LINE__-1);

	assert(free_memory != NULL);
	memset(free_memory,0,sizeof(struct Mem) * (PAGE_SIZE / sizeof(struct Mem)));
#endif

	fprintf(_LOG_,"memory pool created.\n");
}

void close_prog_memory()
{
	if(mem_safe){
		memset(free_memory,0,sizeof (struct Mem) * (PAGE_SIZE / sizeof(struct Mem)));
		if(munmap(prog_mem,MEM_SIZE+(PAGE_SIZE)) == -1){
			fprintf(_LOG_,"failed to unmap memory, %s:%d.\n",__FILE__,__LINE__-1);
			if(log) fclose(log);
			return;
		}

		free(free_memory);
		if(memory_info){
			uint32_t i;
			for( i = 0; i < memory_info_size; i++){
				free(memory_info[i]);
			}
			free(memory_info);
		}
	}else{
		memset(free_memory,0,sizeof (struct Mem) * (PAGE_SIZE / sizeof(struct Mem)));
		free(prog_mem);
		free(free_memory);
		if(memory_info){
			uint32_t i;
			for( i = 0; i < memory_info_size; i++){
				free(memory_info[i]);
			}
			free(memory_info);
		}
	}
	
	fprintf(_LOG_,"memory pool closed.\n");
	if(log) fclose(log);
}


int create_memory(struct Mem *memory, uint64_t size, int type)
{
	if(!memory) return -1;

	switch(type){
	case INT:
		if((size % sizeof(int)) != 0) return -1;
		memory->p = (int*)ask_mem(size);
		if(!memory->p) return -1;
		break;
	case STRING:
		memory->p = (char*)ask_mem(size);
		if(!memory->p) return -1;
		memset(memory->p,06,size);
		break;
	case LONG:
		if((size % sizeof(long)) != 0) return -1;
		memory->p = (long*)ask_mem(size);
		if(!memory->p) return -1;
		break;
	case DOUBLE:
		if((size % sizeof(double)) != 0) return -1;
		memory->p = (double*)ask_mem(size);
		if(!memory->p) return -1;
		break;
	case FLOAT:
		if((size % sizeof(float)) != 0) return -1;
		memory->p = (float*)ask_mem(size);
		if(!memory->p) return -1;
		break;
	default:
		memory->p = ask_mem(size);
		if(!memory->p) return -1;
		break;
	}
	memory->size = size;

	return 0;
}

int cancel_memory(struct Mem *memory){
	if(!memory) return -1;
	if(!memory->p) return -1;
	if(memory_info){
		uint32_t i;
		for(i = 0; i < memory_info_size; i++){
			if(memory_info[i]){
				if(memory_info[i] == memory->p){
					free(memory->p);	
					memory_info[i] = NULL;
					if(resize_memory_info() == -1) return -1;

					return 0;
				}	
			}
		}
	}	

	if(memory->size == 0 || memory->size >= MEM_SIZE) return -1;

	if(return_mem(memory->p,memory->size) == -1) return -1;

	/* check if the all block is free
	 * if its free, we zeroed out the free_memory data
	 * */
	if(is_free(prog_mem,MEM_SIZE-1) == 0){
		memset(free_memory,0,(PAGE_SIZE / sizeof(struct Mem)));	
		last_addr = NULL;
	}
	return 0;
}

int push(struct Mem *memory,void* value,int type){
	
	switch(type){
	case INT:
		if(is_free((void*)memory->p,memory->size) == -1){
			if((memory->size / sizeof(int)) > 1){
				/*find the first empty memory slot*/
				uint64_t i;
				for(i = 0; i < (memory->size / sizeof(int));i++){
					if(*(int*)memory->p == 0) break;
						
					memory->p = (int*)memory->p + 1;
				}	
				memcpy(memory->p,value,sizeof(int));
				memory->p= (int*)memory->p - i;	
				break;
			}
		}
		memcpy(memory->p,value,sizeof(int));
		break;
	case STRING:
		if(is_free((void*)memory->p,memory->size) == -1){
			if((memory->size / sizeof(char)) > 1){
				/*find the first empty memory slot*/
				uint64_t i;
				for(i = 0; i < (memory->size / sizeof(char));i++){
					if(*(char*)memory->p == '\0') break;
						
					memory->p = (char*)memory->p + 1;
				}	
				memcpy(memory->p,value,sizeof(char));
				memory->p= (char*)memory->p - i;	
				break;
			}
		}
		memcpy(memory->p,value,sizeof(char));
		break;
	case LONG:
		if(is_free((void*)memory->p,memory->size) == -1){
			if((memory->size / sizeof(long)) > 1){
				/*find the first empty memory slot*/
				uint64_t i;
				for(i = 0; i < (memory->size / sizeof(char));i++){
					if(*(long*)memory->p == 0) break;
						
					memory->p = (long*)memory->p + 1;
				}	
				memcpy(memory->p,value,sizeof(long));
				memory->p= (long*)memory->p - i;	
				break;
			}
		}
		memcpy(memory->p,value,sizeof(long));
		break;
	case DOUBLE:
		if(is_free((void*)memory->p,memory->size) == -1){
			if((memory->size / sizeof(double)) > 1){
				/*find the first empty memory slot*/
				uint64_t i;
				for(i = 0; i < (memory->size / sizeof(char));i++){
					if(*(double*)memory->p == 0.00) break;
						
					memory->p = (double*)memory->p + 1;
				}	
				memcpy(memory->p,value,sizeof(double));
				memory->p= (double*)memory->p - i;	
				break;
			}
		}
		memcpy(memory->p,value,sizeof(double));
		break;
	case FLOAT:
		if(is_free((void*)memory->p,memory->size) == -1){
			if((memory->size / sizeof(double)) > 1){
				/*find the first empty memory slot*/
				uint64_t i;
				for(i = 0; i < (memory->size / sizeof(char));i++){
					if(*(double*)memory->p == 0.00) break;
						
					memory->p = (double*)memory->p + 1;
				}	
				memcpy(memory->p,value,sizeof(double));
				memory->p= (double*)memory->p - i;	
				break;
			}
		}
		memcpy(memory->p,value,sizeof(double));
		break;
	default:
		return -1;
	}

	return 0;
}
	
void *value_at_index(uint64_t index,struct Mem *memory,int type)
{
	switch(type){
	case INT:
		if((int)((memory->size / sizeof(int)) - (int)index ) <= 0 ) return NULL;
		return (void*)((int*)memory->p + index);
	case STRING:
		if((int)((memory->size / sizeof(char)) - index ) <= 0 ) return NULL;
		return (void*)((char*)memory->p + index);
	case LONG:
		if((int)((memory->size / sizeof(long)) - index ) <= 0 ) return NULL;
		return (void*)((long*)memory->p + index);
	case FLOAT:
		if((int)((memory->size / sizeof(float)) - index ) <= 0 ) return NULL;
		return (void*)((float*)memory->p + index);
	case DOUBLE:
		if((int)((memory->size / sizeof(double)) - index ) <= 0 ) return NULL;
		return (void*)((double*)memory->p + index);
	default:
		return NULL;
	}
}

void *ask_mem(size_t size){
	if(size <= 0) return NULL;
	if(!prog_mem) return NULL;
	if(size > (MEM_SIZE - 1)) goto fall_back;

	/*we have enough space from the start of the memory block*/
	if(!last_addr){
		/*this is the first allocation*/
		last_addr = (prog_mem + size) - 1;
		return (void*) prog_mem;
	}else{
		/*first check if we have a big enough freed block*/
		uint64_t i;
		for(i = 0;i < (PAGE_SIZE / sizeof (struct Mem));i++){
			if(!free_memory[i].p) continue;

			if(free_memory[i].size == size){
				/*last_addr = free_memory[i].p + size -1;*/
				void *found = free_memory[i].p;
				memset(&free_memory[i],0,sizeof(struct Mem));
				return found;
			}

			if(free_memory[i].size > size){
				/*last_addr = free_memory[i].p + size -1;*/
				void *found = free_memory[i].p;
				free_memory[i].p = (int8_t*)free_memory[i].p + size;
				free_memory[i].size -= size;
				return found;
			}
		}

		/*look for space in the memory block*/
		int8_t *p = NULL;
		if(last_addr){
			if((last_addr - prog_mem) >= (MEM_SIZE-1)) goto fall_back;
			if(((last_addr + size) - prog_mem) >= (MEM_SIZE-1)) goto fall_back;

			p = last_addr + 1;

		}else{
			p = prog_mem;
		}

		while(is_free((void*)p,size) == -1) {
			if(((p + size) - prog_mem) > (MEM_SIZE -1)) {
				goto fall_back;
			}
			p += size;
		}

		/*here we know we have a big enough block*/
		last_addr = (p + size) - 1;
		return (void*) p;
	}

	
fall_back:
	fprintf(_LOG_,"using fall back system.\n");
	void *p = malloc(size);
	if(!p){
		fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
		return NULL;
	}
	memset(p,0,size);

	if(memory_info_size == 0){
		memory_info = malloc(sizeof(void*));
		if(!memory_info){
			fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
			return NULL;
		}

		memset(memory_info,0,sizeof(void*));
		memory_info[0] = p;
		memory_info_size++;
	}else{
		uint32_t new_size = memory_info_size + 1;
		void **n_mi = realloc(memory_info,new_size * sizeof *memory_info);
		if(!n_mi){
			fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
			return NULL;
		}
		memory_info_size++;
		memory_info = n_mi;
		memory_info[memory_info_size - 1] = p;
	}
	return p;
}

void *reask_mem(void *p,size_t old_size,size_t size)
{
	if(is_free(&((int8_t*)p)[old_size],size) == -1 || ((last_addr -  prog_mem) > (&((int8_t*)p)[old_size] - prog_mem))){
		/*look for a new block*/
		void *new_block = ask_mem(old_size+size);
		if(!new_block) return NULL;

		/*cpy memory from oldblock to new*/
		memcpy(new_block,p,old_size);
		if(return_mem(p,old_size) == -1){
			return_mem(new_block,old_size+size);
			return NULL;
		}

		return new_block;
	}

	if((last_addr - prog_mem) > (((int8_t*)p + old_size + size) - prog_mem)) return p;
	
	last_addr = &prog_mem[(((int8_t*)p + old_size + size)- prog_mem) -1];
	return p;
}
int return_mem(void *start, size_t size){
	if(!start) return -1;
	if(((int8_t *)start - prog_mem) < 0) return -1;
	if(((int8_t *)start - prog_mem) > (MEM_SIZE -1)) return -1;

	uint64_t s = (uint64_t)((int8_t *)start - prog_mem);
	uint64_t i;
	for(i = 0; i < (PAGE_SIZE / sizeof(struct Mem)); i++){
		if(free_memory[i].p){
			continue;
		}else{
			free_memory[i].p = (void*)&prog_mem[s];
			free_memory[i].size = size;
			memset(&prog_mem[s],0,sizeof(int8_t)*size);
			return 0;
		}
		/*
		 * in this case we do not have space to register the free block
		 * but we 'free' the memory anyway.
		 * */

		memset(&prog_mem[s],0,sizeof(int8_t)*size);
		return 0;
	}

	return 0;
}


static int is_free(void *mem, size_t size){
	char cbuf[size+1];
	memset(cbuf,0,size+1);
	memcpy(cbuf,(char*)mem,size);

	/*
	 * if the memory is 'free' each byte in the block is set to 0
	 * the loop try to find any ascii charater (besides '\0') in the block,
	 * if any char is found, the block is not free
	 * */
	if(cbuf[0] !=  '\0') return -1;
	int i;
	for(i = 1; i < 128;i++){
		if(strstr(cbuf,(char *)&i) != NULL) return -1;
	}

	return 0;
}

static int resize_memory_info()
{
	if(memory_info_size == 1){
		memory_info_size = 0;
		free(*memory_info);
		free(memory_info);
		memory_info = NULL;
		return 0;
	}	
	
	/*move the NULL pointers at the end*/
	uint32_t i,j;
	for(i = 0,j = 1; i < memory_info_size;i++,j++){
		if(memory_info_size - i > 1){
			if(memory_info[i] == NULL && memory_info[j] != NULL){
				memory_info[i] = memory_info[j];
				memory_info[j] = NULL;
				continue;
			}
		}
		break;
	}

	void **n_mi = realloc(memory_info,(--memory_info_size)*sizeof(void*));
	if(!n_mi){
		fprintf(_LOG_,"resizing memory_info array failed, %s:%d.\n",__FILE__,__LINE__-2);
		return -1;
	}

	memory_info = n_mi;
	return 0;
}
