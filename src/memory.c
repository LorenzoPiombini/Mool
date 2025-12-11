#if defined(__linux__)
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <sys/mman.h>
#endif

#if defined(__APPLE__)
	#include <unistd.h>
	#include <sys/mman.h>
#endif


#include <stdio.h>

#if defined(__linux__)
	#include <log.h>
#endif

#include <string.h>
#include <assert.h>
#include "memory.h"

#define MAX_ARENAS 10
int e = -1; /*returning error*/
static i8 *prog_mem = 0x0;
static struct Arena arenas[MAX_ARENAS] = {0};

static i8 *last_addr = 0x0;
static i8 *last_addr_arena[MAX_ARENAS] = {0};
static struct Mem *free_memory = 0x0;
static struct Mem **memory_info = 0x0;
static ui32 memory_info_size = 0;
static FILE *log = 0x0;
#define _LOG_ !log ? stderr : log

/*static functions*/
static int is_free(void *mem, size_t size);
static int resize_memory_info();
static int return_mem(void *start,size_t size);



int create_arena(size_t size){
#if defined(__linux__)
	log = open_log_file("log_memory_areana");
#endif

#if defined(__linux__) || defined(__APPLE__)
	if(!prog_mem){
		size_t original_size = size;
		if(size % PAGE_SIZE != 0){
			if(size < (size_t)PAGE_SIZE) size = (size_t)PAGE_SIZE;
			if(size > (size_t)PAGE_SIZE) size = ((size_t)PAGE_SIZE *((size / (size_t)PAGE_SIZE) + 1));
		}

		i32 i;
		for(i = 0; i < MAX_ARENAS; i++){
			if(arenas[i].mem.p){
				if(arenas[i].mem.size + original_size >= arenas[i].capacity)continue;

				last_addr_arena[i] = ((i8*)arenas[i].mem.p + original_size) - 1;
				arenas[i].mem.size += original_size;

				return 0;
			}

			arenas[i].mem.p = mmap(0x0,size, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
			if(arenas[i].mem.p == MAP_FAILED) return -1;
			arenas[i].mem.size = original_size;
			arenas[i].capacity = size;
			break;
		}
		
		if(i == MAX_ARENAS){
			/*nothing was allocated!*/
			return -1;
		}
	}else{
		if(last_addr){
			if(((ui64)((last_addr + size) - prog_mem)) < MEM_SIZE -1 ){
				i8 *p = last_addr + 1;
				while(is_free((void*)p,size) == -1){
					if(((p + size) - prog_mem) >= (MEM_SIZE -1)) return -1; 
					p = p + size;
				}

				i32 i;
				for(i = 0;i < MAX_ARENAS;i++){
					if(arenas[i].mem.p) continue;
					arenas[i].mem.p = (void*)p;
					arenas[i].mem.size = size;
					arenas[i].capacity = size;

					last_addr = (p + size) - 1;
				}
				return 0;
			}
			return -1;
		}

		i32 i;
		for(i = 0;i < MAX_ARENAS;i++){
			if(arenas[i].mem.p)continue;

			memset(&arenas[i],0,sizeof(struct Arena));
			arenas[i].mem.p = prog_mem;
			arenas[i].mem.size = size;
			arenas[i].capacity = size;
		}
	}

#elif defined(_WIN32)
	/*windows code*/
#endif
	return 0;
}

int close_arena(){
#if defined(__linux__) || __APPLE__
	i32 i;
	if(prog_mem){
		for(i = 0;i < MAX_ARENAS; i++){
			if(!arenas[i].mem.p) continue;
			
			last_addr = ((i8 *)arenas[i].mem.p - 1);
		}
		memset(&arenas,0,MAX_ARENAS * sizeof(struct Arena));
		return 0;
	}

	for(i = 0;i < MAX_ARENAS; i++){
		if(!arenas[i].mem.p) continue;
		
		if(munmap(arenas[i].mem.p,arenas[i].capacity) == -1){
			fprintf(_LOG_,"failed to unmap memory, %s:%d.\n",__FILE__,__LINE__-1);
			if(log) fclose(log);
			return -1;
		}
		last_addr_arena[i] = 0x0;
	}

	memset(&arenas,0,MAX_ARENAS * sizeof(struct Arena));
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
	prog_mem = (i8 *)mmap(0x0,sizeof (i8) * (MEM_SIZE + PAGE_SIZE), 
			PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,
			-1,0);

	if(prog_mem == MAP_FAILED) return -1;

	memset(prog_mem,0,MEM_SIZE * sizeof(i8));

	/*we insert a page after the memory so we get a SIGSEGV if we overflow*/
	if(mprotect(prog_mem + MEM_SIZE,PAGE_SIZE,PROT_NONE) == -1){
		fprintf(_LOG_,"memory protection failed.\n");
	}

	free_memory = (struct Mem*) mmap(0x0,PAGE_SIZE,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);

	if(free_memory == MAP_FAILED){
		fprintf(_LOG_,"cannot allocate memory for free_memory data, %s:%d.\n"
				,__FILE__,__LINE__-1);
		return -1;
	}
#elif
	/* TODO WINDOWS CODE */
#endif

	fprintf(_LOG_,"memory pool created.\n");
	return 0;
}


void *get_arena(size_t *size)
{
	if(prog_mem && size){/*get a piece of memory from the big chunk*/
		if(last_addr){
			if(((last_addr + *size ) - prog_mem) < (MEM_SIZE -1)){
				i8 *p = last_addr + 1;
				while(is_free(p,*size) != 0){
					if(((p + *size) - prog_mem) >= (MEM_SIZE -1)) return 0x0; 
					p += *size;
				}
				last_addr = (p + *size) - 1;
				return (void *)p;
			}
		}
		last_addr = (prog_mem + *size) - 1;
		return (void *)prog_mem;
	}

	i32 i;
	for(i = 0; i < MAX_ARENAS; i++){
		if(!arenas[i].mem.p) continue;
		if((ui64)((i8*)arenas[i].mem.p + arenas[i].mem.size - &((i8*)(arenas[i].mem.p))[0]) > arenas[i].capacity) continue;
		void *p = arenas[i].mem.p + arenas[i].mem.size;
		return p;
	}

	return 0x0;
}


void close_prog_memory()
{
#if defined(__linux__) || __APPLE__
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
			ui32 i;
			for( i = 0; i < memory_info_size; i++){
				if(memory_info[i]){
					if(memory_info[i]->p)
						munmap(memory_info[i]->p,memory_info[i]->size);
					munmap(memory_info[i],sizeof **memory_info);
				}
			}
			if(memory_info_size > (PAGE_SIZE / sizeof(struct Mem*))){
				munmap(memory_info,(1 +(memory_info_size / (PAGE_SIZE /sizeof(struct Mem*)))) * PAGE_SIZE);
			}else{
				munmap(memory_info,PAGE_SIZE);
			}
		}
#if defined(__linux__)
		fprintf(_LOG_,"memory pool closed.\n");
		if(log) fclose(log);
#endif /*__linux__*/
		return;
	}
	return;
#elif defined(_WIN32)
	/*WINDOWS CODE*/
#endif
}


int create_memory(struct Mem *memory, ui64 size, int type)
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
	memset(&arenas,0,MAX_ARENAS * sizeof(struct Arena));
	if(!prog_mem) return;	

	memset(prog_mem,0,MEM_SIZE);
	if(free_memory)
		memset(free_memory,0,sizeof(struct Mem) * (PAGE_SIZE / sizeof (struct Mem)));

	last_addr = 0x0;
	if(memory_info){
		if(memory_info_size > (PAGE_SIZE / sizeof(struct Mem*))){
			munmap(memory_info,(1 +(memory_info_size / (PAGE_SIZE /sizeof(struct Mem*)))) * PAGE_SIZE);
		}else{
			munmap(memory_info,PAGE_SIZE);
		}
		memory_info = 0x0;
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
			ui32 i;
			for(i = 0; i < memory_info_size; i++){
				if(memory_info[i]){
					if(memory_info[i]->p == start){
						munmap(memory_info[i]->p,memory_info[i]->size);	
						memory_info[i]->p = 0x0;
						munmap(memory_info[i],sizeof **memory_info);
						memory_info[i] = 0x0;
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
			ui32 i;
			for(i = 0; i < memory_info_size; i++){
				if(memory_info[i]){
					if(memory_info[i]->p == memory->p){
						munmap(memory->p,memory->size);	
						munmap(memory_info[i],sizeof **memory_info);
						memory_info[i] = 0x0;
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
	 * this needs a careful investigation, because it should not 
	 * be neccessery
	 * */

	if(is_free(prog_mem,MEM_SIZE-1) == 0){
		memset(free_memory,0,sizeof(struct Mem) * (PAGE_SIZE / sizeof(struct Mem)));	
		memset(prog_mem,0,MEM_SIZE);
		last_addr = 0x0;
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
					ui64 i;
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
					ui64 i;
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
					ui64 i;
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
					ui64 i;
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
					ui64 i;
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

void *value_at_index(ui64 index,struct Mem *memory,int type)
{
	switch(type){
		case INT:
			if((int)((memory->size / sizeof(int)) - (int)index ) <= 0 ) return 0x0;
			return (void*)((int*)memory->p + index);
		case STRING:
			if((int)((memory->size / sizeof(char)) - index ) <= 0 ) return 0x0;
			return (void*)((char*)memory->p + index);
		case LONG:
			if((int)((memory->size / sizeof(long)) - index ) <= 0 ) return 0x0;
			return (void*)((long*)memory->p + index);
		case FLOAT:
			if((int)((memory->size / sizeof(float)) - index ) <= 0 ) return 0x0;
			return (void*)((float*)memory->p + index);
		case DOUBLE:
			if((int)((memory->size / sizeof(double)) - index ) <= 0 ) return 0x0;
			return (void*)((double*)memory->p + index);
		default:
			return 0x0;
	}
}

void *ask_mem(size_t size){
	if(size <= 0) return 0x0;

	i32 i;
	ui8 null = 1;
	for(i = 0;i < MAX_ARENAS;i++){
		if(arenas[i].mem.p) continue;			

		null = 0;
		break;
	}

	if(!prog_mem && null) return 0x0;

	if(prog_mem)
		if(size > (MEM_SIZE - 1)) goto fall_back;

	for(i = 0;i < MAX_ARENAS; i++){ 
		if(arenas[i].mem.p){
			if((arenas[i].capacity-1) < size) continue; 

			if(!last_addr_arena[i]){
				fprintf(_LOG_,"first allocation of %ld bytes\n",size);
				/*this is the first allocation*/
				last_addr_arena[i] = (arenas[i].mem.p + size) - 1;
				return (void*) arenas[i].mem.p;
			}else{
				/*look for space in the memory block*/
				i8 *p = 0x0;
				if(last_addr_arena[i]){
					if((ui64)(last_addr_arena[i] - (i8*)arenas[i].mem.p) >= (arenas[i].capacity - 1)) return 0x0;
					if((ui64)((last_addr_arena[i] + size) - (i8*)arenas[i].mem.p) >= (arenas[i].capacity - 1)) return 0x0;

					p = last_addr_arena[i] + 1;
				}else{
					p = arenas[i].mem.p;
				}

				while(is_free((void*)p,size) == -1) {
					if((ui64)((p + size) - (i8*)arenas[i].mem.p) > (arenas[i].mem.size -1)) {
						return 0x0;
					}
					p += size;
				}

				/*here we know we have a big enough block*/
				fprintf(_LOG_,"memory allocated at position %ld, for %ld bytes, in the arena.\n",(long)(p - (i8*)arenas[i].mem.p),size);
				last_addr_arena[i] = (p + size) - 1;
				return (void*) p;
			}
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
			size_t i;
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
					free_memory[i].p = (i8*)free_memory[i].p + size;
					free_memory[i].size -= size;
					memset(found,0,sizeof(i8)*size);/*should not be neccessery*/
					return found;
				}
			}

			/*look for space in the memory block*/
			i8 *p = 0x0;
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


fall_back:
#if defined(__linux__)
	fprintf(_LOG_,"using fall back system.\n");
#endif
#if defined(__linux__) || defined(__APPLE__)
	/*make sure the size is conform to PAGE_SIZE !!*/
	if(size < PAGE_SIZE) size = PAGE_SIZE;
	if(size > PAGE_SIZE){ 
		double mul = ((double)((double)size / PAGE_SIZE)) 
		size = mul > 1.0 ? PAGE_SIZE * (int)(mul+1.0) : PAGE_SIZE;
	}

	void *p = mmap(0x0,size,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);

	if(p == MAP_FAILED){
		fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
		return 0x0;
	}

	if(memory_info_size == 0){
		memory_info = (struct Mem**) mmap(0x0,PAGE_SIZE,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);

		if(memory_info == MAP_FAILED){
			fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
			return 0x0;
		}

		memory_info[0] = (struct Mem*) mmap(0x0,sizeof **memory_info,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);

		if(memory_info[0] == MAP_FAILED){
			fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
			return 0x0;
		}

		memory_info[0]->p = p;
		memory_info[0]->size = size;
		memory_info_size++;
	}else if (memory_info_size >= (PAGE_SIZE/sizeof(struct Mem*))){
		ui32 new_size = memory_info_size + 1;
		struct Mem **n_mi = 0x0;

		if((double)memory_info_size / (PAGE_SIZE / sizeof(struct Mem*)) > 1.00){
			long old_size = (memory_info_size / (PAGE_SIZE /sizeof(struct Mem*)) + 1) * PAGE_SIZE;
			n_mi = (struct Mem**) mremap(memory_info,old_size,old_size + PAGE_SIZE, MREMAP_MAYMOVE);
		}else if((double)memory_info_size / (PAGE_SIZE / sizeof(struct Mem*)) == 1.00) {
			n_mi = (struct Mem**) mremap(memory_info,PAGE_SIZE,PAGE_SIZE * 2, MREMAP_MAYMOVE);
		}

		if(n_mi == MAP_FAILED){
			fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
			return 0x0;
		}
		memory_info_size = new_size;
		memory_info = n_mi;
		memory_info[memory_info_size - 1] = mmap(0x0,sizeof **memory_info,
				PROT_READ | PROT_WRITE,
				MAP_SHARED | MAP_ANONYMOUS,-1,0);

		if(memory_info[memory_info_size - 1] == MAP_FAILED){
			fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
			return 0x0;
		}

		memory_info[memory_info_size - 1]->size = size;
		memory_info[memory_info_size - 1]->p = p;
	}else{

		ui32 i = 0;
		while(i < memory_info_size && memory_info[i]) i++;

		memory_info[i] = (struct Mem*) mmap(0x0,sizeof **memory_info,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);

		if(memory_info[i] == MAP_FAILED){
			fprintf(_LOG_,"memory allocation failed, %s:%d.\n",__FILE__,__LINE__-2);
			return 0x0;
		}

		memory_info[i]->p = p;
		memory_info[i]->size = size;
		memory_info_size++;
	}
	return p;
#elif defined(_WIN32)
	/*WINDOWS code*/
#endif
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
		/*check where is the memory from */
		ui32 i;
		for(i = 0; i < memory_info_size; i++){
			if(memory_info[i]){

				if(memory_info[i]->p == p){

					void *n_mem = mremap(memory_info[i]->p,memory_info[i]->size,size,MREMAP_MAYMOVE);
					if(n_mem == MAP_FAILED) return 0x0;

					memory_info[i]->size = size;
					memory_info[i]->p = n_mem;

					fprintf(_LOG_,"reask_mem() performed a remap on the fall_back system,"\
							"pointer remapped ocated %p, new size = %ld\n",
							(void*)memory_info[i]->p,size);

					return memory_info[i]->p;
				}
			}
		}
	}

	if(size < old_size){

		/*free the memory that is not needed anymore*/
		void* p_to_left_mem = (i8*)p + size;
		if(return_mem(p_to_left_mem,old_size - size) == -1){
			fprintf(_LOG_,"return_mem() failed. %s:%d.\n",__FILE__,__LINE__-1);
			return 0x0;
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

	if((((i8*)p - prog_mem) + (remain == 1 ? remaining_size : size)) > MEM_SIZE){
		/*look for a new block*/
		if(size > old_size){
			void *new_block = ask_mem(size);
			if(!new_block) return 0x0;

			/*cpy memory from oldblock to new and free the old*/
			memcpy(new_block,p,old_size);
			if(return_mem(p,old_size) == -1){
				return_mem(new_block,old_size+size);
				return 0x0;
			}

			return new_block;
		}
		/*TODO add case size < old_size*/
	}

	/* check if we can expand the memory in place, meaning
	 * the requested new memory size is avaiabale adjason to the old memory location
	 * if not we look for another block */
	if(is_free(&((i8*)p)[old_size],(remain == 1 ? remaining_size : size)) == -1 || 
			((last_addr -  prog_mem) > (&((i8*)p)[old_size] - prog_mem))){
		/*look for a new block*/
		if(size > old_size){
			void *new_block = ask_mem(size);
			if(!new_block) return 0x0;

			/*cpy memory from oldblock to new and free the old*/
			memcpy(new_block,p,old_size);
			if(return_mem(p,old_size) == -1){
				return_mem(new_block,old_size+size);
				return 0x0;
			}

			return new_block;
		}
	}

	if((last_addr - prog_mem) > (((i8*)p + old_size + size) - prog_mem)){
		fprintf(_LOG_,"reask_mem expand memory in place from %ld, to %ld",old_size, size);	
		return p;
	}

	last_addr = &prog_mem[(((i8*)p + old_size + size)- prog_mem) -1];
	return p;
}

static int return_mem(void *start, size_t size){
	if(!start) return -1;
	if(((i8 *)start - prog_mem) < 0) return -1;
	if(((i8 *)start - prog_mem) > (MEM_SIZE -1)) return -1;

	ui64 s = (ui64)((i8 *)start - prog_mem);
	ui64 i;
	for(i = 0; i < (PAGE_SIZE / sizeof(struct Mem)); i++){
		if(free_memory[i].p){
			continue;
		}else{
			free_memory[i].p = (void*)&prog_mem[s];
			free_memory[i].size = size;
			memset(&prog_mem[s],0,sizeof(i8)*size);
			return 0;
		}
		/*
		 * in this case we do not have space to register the free block
		 * but we 'free' the memory anyway.
		 * */

		memset(&prog_mem[s],0,sizeof(i8)*size);
		return 0;
	}

	return 0;
}


static int is_free(void *mem, size_t size){
	if(size == MEM_SIZE-1){
		ui32 i;
		for(i = 0;i < MEM_SIZE;i++)
			if(((i8*)prog_mem)[i] != '\000') return -1;

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
		if(strstr(cbuf,(char *)&i) != 0x0) return -1;
	}

	return 0;
}

static int resize_memory_info()
{
	if(memory_info_size == 1){
		memory_info_size = 0;
		if(memory_info[0]){
			if(memory_info[0]->p)
				munmap(memory_info[0]->p,memory_info[0]->size);
			munmap(memory_info[0],sizeof **memory_info);
		}

		if(memory_info_size > (PAGE_SIZE / sizeof(struct Mem*))){
			munmap(memory_info,(1 +(memory_info_size / (PAGE_SIZE /sizeof(struct Mem*)))) * PAGE_SIZE);
		}else{
			munmap(memory_info,PAGE_SIZE);
		}
		memory_info = 0x0;
		return 0;
	}	

	/*move the 0x0 pointers at the end*/
	ui32 i,j;
	for(i = 0,j = 1; i < memory_info_size;i++,j++){
		if(memory_info_size - i > 1){
			if(memory_info[i] == 0x0 && memory_info[j] != 0x0){
				memory_info[i] = memory_info[j];
				memory_info[j] = 0x0;
				continue;
			}
		}
		break;
	}

	ui32 new_size = memory_info_size - 1;
	struct Mem **n_mi = 0x0;

	if(memory_info_size / (PAGE_SIZE / sizeof(struct Mem*)) == 
			new_size / (PAGE_SIZE / sizeof(struct Mem*))) return 0;


	if((double)new_size / (PAGE_SIZE / sizeof(struct Mem*)) > 1.00){
		long old_size = (memory_info_size / (PAGE_SIZE /sizeof(struct Mem*)) + 1) * PAGE_SIZE;
		n_mi = (struct Mem**) mremap(memory_info,old_size,old_size - PAGE_SIZE, MREMAP_MAYMOVE);
	}

	if(n_mi == MAP_FAILED){
		fprintf(_LOG_,"resizing memory_info array failed, %s:%d.\n",__FILE__,__LINE__-2);
		return -1;
	}

	memory_info = n_mi;
	memory_info_size = new_size;
	return 0;
}
