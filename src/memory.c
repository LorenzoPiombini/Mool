#if defined(__linux__) || __APPLE__
	#include <unistd.h>
	#include <sys/mman.h>
#endif


#include <stdio.h>

#if defined(__linux__)
	#include <log.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "memory.h"

int e = -1; /*returning error*/
static int8_t mem_safe = 0;
static int8_t *prog_mem = NULL;
static struct Mem arena;
static pid_t proc = -1;
static int8_t *last_addr = NULL;
static int8_t *last_addr_arena = NULL;
static struct Mem shared_memory;
static struct Mem *free_memory = NULL;
static struct Mem **memory_info = NULL;
static uint32_t memory_info_size = 0;
static FILE *log = NULL;
#define _LOG_ !log ? stderr : log

/*static functions*/
static int is_free(void *mem, size_t size);
static int resize_memory_info();
static int return_mem(void *start,size_t size);


int create_shared_memory(size_t size)
{
	log = open_log_file("log_shared_mem");
	memset(&shared_memory,0,sizeof(struct Mem));
	if(!shared_memory.p){
#if defined(__linux__)  || defined(__APPLE__)
		if((shared_memory.p = mmap(NULL,size, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0)) == (void*)-1){
			fprintf(_LOG_,"cannot open shared memory\n");
			return -1;
		}
#elif defined(_WIN32)
		/*windows code*/
#endif
		shared_memory.size = (uint64_t)size;
		return 0;
	}
	return SMEM_EXIST;	
}

int read_from_shared_memory()
{
	if(!shared_memory.p) return SMEM_NOENT;

	if(*(int8_t*)shared_memory.p == 0) 
		return 0;
	else
		return -1;
}
#if 0
int write_to_shared_memory(pid_t pid, void* value)
{
	if(!shared_memory.p) return SMEM_NOENT;

	if(proc == -1){
		/*write to memory*/
		proc = pid;
		memcpy(shared_memory.p,value,sizeof(uint8_t));
		return 0;
	}else if( proc == pid){
		memcpy(shared_memory.p,value,sizeof(uint8_t));
		if(*(uint8_t*)value == 0) proc = -1;
		return 0;
	}

	return -1;
}
void close_shared_memory()
{
	if(munmap(shared_memory.p,shared_memory.size) == -1) {
		fprintf(_LOG_,"shared_memory closed failed\n");
		return;
	}
	shared_memory.p = NULL;
	shared_memory.size = 0;
}
#endif

int create_arena(size_t size){
#if defined(__linux__)
	log = open_log_file("log_memory_areana");
#endif

#if defined(__linux__) || defined(__APPLE__)
	if(!prog_mem){
		if(size % PAGE_SIZE != 0){
			if(size < (size_t)PAGE_SIZE) size = (size_t)PAGE_SIZE;
			if(size > (size_t)PAGE_SIZE) size = ((size_t)PAGE_SIZE *((size / (size_t)PAGE_SIZE) + 1));
		}
		if(!arena.p)
			arena.p = mmap(NULL,size, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
		else
			return -1;

		if(arena.p == MAP_FAILED) return -1;
		arena.size = size;
	}else{
		if(last_addr){
			if(((uint64_t)((last_addr + size) - prog_mem)) < MEM_SIZE -1 ){
				int8_t *p = last_addr + 1;
				while(is_free((void*)p,size) == -1){
					if(((p + size) - prog_mem) >= (MEM_SIZE -1)) return -1; 
					p = p + size;
				}
				arena.p = (void*)(last_addr + 1);
				arena.size = size;
				last_addr = (last_addr + size) - 1;
				return 0;
			}
			return -1;
		}
		
		memset(&arena,0,sizeof(struct Mem));
		arena.p = prog_mem;
		arena.size = size;
	}

#elif defined(_WIN32)
	/*windows code*/
#endif
	return 0;
}

int close_arena(){
#if defined(__linux__) || __APPLE__
	if(prog_mem){
		last_addr = ((int8_t*)arena.p - 1);
		memset(arena.p,0,arena.size);
		memset(&arena,0,sizeof(struct Mem));
		return 0;
	}

	if(munmap(arena.p,arena.size) == -1){
		fprintf(_LOG_,"failed to unmap memory, %s:%d.\n",__FILE__,__LINE__-1);
		if(log) fclose(log);
		return -1;
	}
	memset(&arena,0,sizeof(struct Mem));
	last_addr_arena = NULL;
#elif defined(_WIN32)
	/*windows code*/
#endif

#if defined(__linux__)
		fprintf(_LOG_,"memory arena closed.\n");
		if(log) fclose(log);
#endif /*__linux__*/
	return 0;
}

int init_prog_memory()
{

#if defined(__linux__) 
	log = open_log_file("log_mem");
#endif

#if defined(__linux__) || __APPLE__
	prog_mem = (int8_t *)mmap(NULL,sizeof (int8_t) * (MEM_SIZE + PAGE_SIZE), 
					PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,
					-1,0);

	if(prog_mem == MAP_FAILED) return -1;

	memset(prog_mem,0,MEM_SIZE * sizeof(int8_t));

	/*we insert a page after the memory so we get a SIGSEGV if we overflow*/
	if(mprotect(prog_mem + MEM_SIZE,PAGE_SIZE,PROT_NONE) == -1){
		fprintf(_LOG_,"memory protection failed.\n");
	}

	free_memory = mmap(NULL,sizeof(struct Mem) * (PAGE_SIZE / sizeof(struct Mem)),
					PROT_READ | PROT_WRITE,
					MAP_SHARED | MAP_ANONYMOUS,
					-1,0);
	if(free_memory == MAP_FAILED){
		fprintf(_LOG_,"cannot allocate memory for free_memory data, %s:%d.\n"
				,__FILE__,__LINE__-1);
		return -1;
	}
	memset(free_memory,0,sizeof(struct Mem) * (PAGE_SIZE / sizeof(struct Mem)));
#elif
	/* TODO WINDOWS CODE */
	prog_mem = malloc(MEM_SIZE*sizeof(int8_t));
	if(!prog_mem){ 
		fprintf(_LOG_,"initial fall back system using malloc failed, %s:%d.\n"
				,__FILE__,__LINE__-2);
		return -1;
	}

	memset(prog_mem,0,MEM_SIZE * sizeof(int8_t));

	free_memory = malloc(sizeof(struct Mem) * (PAGE_SIZE / sizeof (struct Mem)));
	if(!free_memory){
		fprintf(_LOG_,"cannot allocate memory for free_memory data, %s:%d.\n"
				,__FILE__,__LINE__-1);
		return -1;
	}

	memset(free_memory,0,sizeof(struct Mem) * (PAGE_SIZE / sizeof(struct Mem)));
#endif

	fprintf(_LOG_,"memory pool created.\n");
	return 0;
}


void *get_arena(size_t *size)
{
	if(prog_mem && size){
		if(last_addr){
			if(((last_addr + *size ) - prog_mem) < (MEM_SIZE -1)) return (void*) last_addr + 1;
		}
		return arena.p;
	}

	if(arena.p) return arena.p;
	
	return NULL;
}

int is_inside_arena(size_t size,struct arena a)
{
	return a.bwritten + size < a.size;
}

void close_prog_memory()
{
#if defined(__linux__) || __APPLE__
	if(mem_safe){
		if(prog_mem){
			memset(free_memory,0,sizeof (struct Mem) * (PAGE_SIZE / sizeof(struct Mem)));
			if(munmap(prog_mem,MEM_SIZE+(PAGE_SIZE)) == -1){
				fprintf(_LOG_,"failed to unmap memory, %s:%d.\n",__FILE__,__LINE__-1);
				if(log) fclose(log);
				return;
			}

		    if(munmap(free_memory,sizeof(struct Mem) * (PAGE_SIZE / sizeof(struct Mem))) == -1){
				fprintf(_LOG_,"failed to unmap memory, %s:%d.\n",__FILE__,__LINE__-1);
				if(log) fclose(log);
				return;
			}

			if(memory_info){
				uint32_t i;
				for( i = 0; i < memory_info_size; i++){
					if(memory_info[i]){
						if(memory_info[i]->p)
							free(memory_info[i]->p);
						free(memory_info[i]);
					}
				}
				free(memory_info);
			}
#if defined(__linux__)
			fprintf(_LOG_,"memory pool closed.\n");
			if(log) fclose(log);

#endif /*__linux__*/
		return;
		}
	}else{
		return;
	}
#endif /*__linux__ || __APPLE__*/
	memset(free_memory,0,sizeof (struct Mem) * (PAGE_SIZE / sizeof(struct Mem)));
	free(prog_mem);
	free(free_memory);
	if(memory_info){
		uint32_t i;
		for( i = 0; i < memory_info_size; i++){
			if(memory_info[i]){
				if(memory_info[i]->p)
					free(memory_info[i]->p);
				free(memory_info[i]);
			}

		}
		free(memory_info);
	}
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

void clear_memory(){
	if(!prog_mem && !arena.p) return;	

	if(arena.p){
		memset(arena.p,0,arena.size);	
	}else{
		memset(prog_mem,0,MEM_SIZE);
		if(free_memory)
			memset(free_memory,0,sizeof(struct Mem) * (PAGE_SIZE / sizeof (struct Mem)));

		last_addr = NULL;
		if(memory_info){
			uint32_t i;
			for(i = 0; i < memory_info_size; i++){
				if(memory_info[i]){
					free(memory_info[i]->p);	
					memory_info[i]->p = NULL;
					free(memory_info[i]);
					memory_info[i] = NULL;
					if(resize_memory_info() == -1) return;
				}
			}
		}
	}
}
int cancel_memory(struct Mem *memory,void *start,size_t size){
	if(memory && start) return -1;
	if(!memory && !start) return -1;
	if(memory)
		if(!memory->p && !start) return -1;

	if(size <= 0 && start) return -1;

	if(start){
		if(memory_info){
			uint32_t i;
			for(i = 0; i < memory_info_size; i++){
				if(memory_info[i]){
					if(memory_info[i]->p == start){
						free(memory_info[i]->p);	
						memory_info[i]->p = NULL;
						free(memory_info[i]);
						memory_info[i] = NULL;
						if(resize_memory_info() == -1) return -1;

						fprintf(_LOG_,"freed %ld bytes from fall back system\n",size);
						return 0;
					}	
				}
			}
		}	


		if(return_mem(start,size) == -1) return -1;
		fprintf(_LOG_,"'freed' %ld bytes of memory in the pool\n",size);

	}else{
		if(memory_info){
			uint32_t i;
			for(i = 0; i < memory_info_size; i++){
				if(memory_info[i]){
					if(memory_info[i]->p == memory->p){
						free(memory->p);	
						free(memory_info[i]);
						memory_info[i] = NULL;
						if(resize_memory_info() == -1) return -1;

						fprintf(_LOG_,"freed %ld bytes from fall back system\n",memory->size);
						return 0;
					}	
				}
			}
		}	


		if(memory->size == 0 || memory->size >= MEM_SIZE) return -1;

		if(return_mem(memory->p,memory->size) == -1) return -1;

		fprintf(_LOG_,"freed %ld bytes from fall back system\n",memory->size);
	}

	/* 
	 * check if the all block is free
	 * if its free, we zeroed out the free_memory data,
	 * and the block iself,
	 * this needs a carlfulw investigation, because it should not 
	 * be neccessery
	 * */

	if(is_free(prog_mem,MEM_SIZE-1) == 0){
		memset(free_memory,0,sizeof(struct Mem) * (PAGE_SIZE / sizeof(struct Mem)));	
		memset(prog_mem,0,MEM_SIZE);
		last_addr = NULL;
		fprintf(_LOG_,"freed all memory\n");
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
	if(!prog_mem && !arena.p) return NULL;

	if(prog_mem)
		if(size > (MEM_SIZE - 1)) goto fall_back;

	if(arena.p){
		if((arena.size-1) < size) { 
			/*here you may want to expand the memory*/
			return NULL;
		}
	}

	/*we have enough space from the start of the memory block*/
	if(prog_mem){
		if(!last_addr){
			fprintf(_LOG_,"first allocation of %ld bytes\n",size);
			/*this is the first allocation*/
			last_addr = (prog_mem + size) - 1;
			return (void*) prog_mem;
		}else{
			/*first check if we have a big enough freed block*/
			uint64_t i;
			for(i = 0;i < (PAGE_SIZE / sizeof (struct Mem));i++){
				if(!free_memory[i].p) continue;

				if(free_memory[i].size == size){
					fprintf(_LOG_,"using a free block at positions %ld, of size %ld\n",i,size);
					void *found = free_memory[i].p;
					memset(&free_memory[i],0,sizeof(struct Mem));
					return found;
				}

				if(free_memory[i].size > size){
					fprintf(_LOG_,"using a pice of a block at positions %ld, of size %ld\n",i,size);
					void *found = free_memory[i].p;
					free_memory[i].p = (int8_t*)free_memory[i].p + size;
					free_memory[i].size -= size;
					memset(found,0,sizeof(int8_t)*size);/*should not be neccessery*/
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
				if(((p + size) - prog_mem) >= (MEM_SIZE -1)) {
					goto fall_back;
				}
				p += size;
			}

			/*here we know we have a big enough block*/
			fprintf(_LOG_,"memory allocated at position %ld, for %ld bytes\n",(long)(p - prog_mem),size);
			last_addr = (p + size) - 1;
			return (void*) p;
		}
	}

	if(arena.p){
		if(!last_addr_arena){
			fprintf(_LOG_,"first allocation of %ld bytes\n",size);
			/*this is the first allocation*/
			last_addr_arena = (arena.p + size) - 1;
			return (void*) arena.p;
		}else{
			/*look for space in the memory block*/
			int8_t *p = NULL;
			if(last_addr_arena){
				if((uint64_t)(last_addr_arena - (int8_t*)arena.p) >= (arena.size - 1)) return NULL;
				if((uint64_t)((last_addr_arena + size) - (int8_t*)arena.p) >= (arena.size - 1)) return NULL;

				p = last_addr_arena + 1;
			}else{
				p = arena.p;
			}

			while(is_free((void*)p,size) == -1) {
				if((uint64_t)((p + size) - (int8_t*)arena.p) > (arena.size -1)) {
					return NULL;
				}
				p += size;
			}

			/*here we know we have a big enough block*/
			fprintf(_LOG_,"memory allocated at position %ld, for %ld bytes, in the arena.\n",(long)(p - (int8_t*)arena.p),size);
			last_addr_arena = (p + size) - 1;
			return (void*) p;
		}
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
		memory_info = malloc(sizeof(struct Mem*));
		if(!memory_info){
			fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
			return NULL;
		}

		memset(memory_info,0,sizeof *memory_info);
		memory_info[0] = malloc(sizeof **memory_info);
		if(!memory_info[0]){
			fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
			return NULL;
		}

		memset(memory_info[0],0,sizeof **memory_info);
		memory_info[0]->p = p;
		memory_info[0]->size = size;
		memory_info_size++;
	}else{
		uint32_t new_size = memory_info_size + 1;
		struct Mem **n_mi = realloc(memory_info,new_size * sizeof *memory_info);
		if(!n_mi){
			fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
			return NULL;
		}
		memory_info_size = new_size;
		memory_info = n_mi;
		memory_info[memory_info_size - 1] = malloc(sizeof **memory_info);
		if(!memory_info[memory_info_size - 1]){
			fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
			return NULL;
		}

		memset(memory_info[memory_info_size -1],0,sizeof **memory_info);
		memory_info[memory_info_size - 1]->size = size;
		memory_info[memory_info_size - 1]->p = p;
	}
	return p;
}

int expand_memory(struct Mem *memory, size_t size,int type){
	void *expanded_m = reask_mem(memory->p,memory->size,size);
	if(!expanded_m) return -1;

	switch(type){
		case INT:
			memory->p = (int*)expanded_m;
			memory->size += size;
			break;
		case LONG:
			memory->p = (long*)expanded_m;
			memory->size += size;
			break;
		case STRING:
			memory->p = (char*)expanded_m;
			memory->size += size;
			break;
		case DOUBLE:
			memory->p = (double*)expanded_m;
			memory->size += size;
			break;
		case FLOAT:
			memory->p = (float*)expanded_m;
			memory->size += size;
			break;
		default:
			memory->p = expanded_m;
			memory->size += size;
			break;
	}

	return 0;
}

void *reask_mem(void *p,size_t old_size,size_t size)
{
	if(memory_info){
		/*you have to check where is the memory from */
		uint32_t i;
		for(i = 0; i < memory_info_size; i++){
			if(memory_info[i]){

				if(memory_info[i]->p == p){
					void *n_mem = realloc(memory_info[i]->p,size);
					if(!n_mem) return NULL;

					memory_info[i]->size = size;
					memory_info[i]->p = n_mem;

					fprintf(_LOG_,"reask_mem() performed a realloc on the fall_back system,"\
							"pointer reallocated %p, new size = %ld\n",
							(void*)memory_info[i]->p,size);

					return memory_info[i]->p;
				}
			}
		}
	}

	if(size < old_size){

		/*free the memory that is not needed anymore*/
		void* p_to_left_mem = (int8_t*)p + size;
		if(return_mem(p_to_left_mem,old_size - size) == -1){
			fprintf(_LOG_,"return_mem() failed. %s:%d.\n",__FILE__,__LINE__-1);
			return NULL;
		}
		fprintf(_LOG_,"memory resized to %ld, from %ld",old_size,size);
		return p;
	}

	size_t remaining_size = 0;
	int remain = 0;
	if(size > old_size){
		remaining_size = size - old_size;
		remain = 1;
	}

	if((((int8_t*)p - prog_mem) + (remain == 1 ? remaining_size : size)) > MEM_SIZE){
		/*look for a new block*/
		if(size > old_size){
			void *new_block = ask_mem(size);
			if(!new_block) return NULL;

			/*cpy memory from oldblock to new and free the old*/
			memcpy(new_block,p,old_size);
			if(return_mem(p,old_size) == -1){
				return_mem(new_block,old_size+size);
				return NULL;
			}

			return new_block;
		}
		/*TODO add case size < old_size*/
	}

	/* check if we can expand the memory in place, meaning
	 * the requested new memory size is avaiabale adjason to the old memory location
	 * if not we look for another block */
	if(is_free(&((int8_t*)p)[old_size],(remain == 1 ? remaining_size : size)) == -1 || 
			((last_addr -  prog_mem) > (&((int8_t*)p)[old_size] - prog_mem))){
		/*look for a new block*/
		if(size > old_size){
			void *new_block = ask_mem(size);
			if(!new_block) return NULL;

			/*cpy memory from oldblock to new and free the old*/
			memcpy(new_block,p,old_size);
			if(return_mem(p,old_size) == -1){
				return_mem(new_block,old_size+size);
				return NULL;
			}

			return new_block;
		}
	}

	if((last_addr - prog_mem) > (((int8_t*)p + old_size + size) - prog_mem)){
		fprintf(_LOG_,"reask_mem expand memory in place from %ld, to %ld",old_size, size);	
		return p;
	}

	last_addr = &prog_mem[(((int8_t*)p + old_size + size)- prog_mem) -1];
	return p;
}

static int return_mem(void *start, size_t size){
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
	if(size == MEM_SIZE-1){
		uint32_t i;
		for(i = 0;i < MEM_SIZE;i++)
			if(((int8_t*)prog_mem)[i] != '\000') return -1;

		return 0;
	}

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
		if(memory_info[0]){
			if(memory_info[0]->p)
				free(memory_info[0]->p);
			free(memory_info[0]);
		}
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

	uint32_t new_size = memory_info_size - 1;
	struct Mem **n_mi = realloc(memory_info,new_size * sizeof *memory_info);
	if(!n_mi){
		fprintf(_LOG_,"resizing memory_info array failed, %s:%d.\n",__FILE__,__LINE__-2);
		return -1;
	}

	memory_info = n_mi;
	memory_info_size = new_size;
	return 0;
}
