#include <stdio.h>
#include <string.h>
#include "memory.h"

struct Mytype{
	short number;
	char str[5];
};


int main(void)
{
	/*using arena*/
	
	if(create_arena(500) == -1) return -1 ;
	struct arena ar;
	ar.p = get_arena();
	ar.size = 500;
	ar.bwritten = 0;
	
	if(!ar.p) return -1;
	
	char *help = (char*) &(((char*)ar.p)[ar.bwritten]);

	strncpy(help,"when i was younger so much younger than today",strlen("when i wad younger so much younger than today")+1);

	ar.bwritten = strlen("when i was younger so much younger than today");
	
	printf("%s\n",help);
	close_arena();

	init_prog_memory();
	
	struct Mem my_type;
	memset(&my_type,0,sizeof (struct Mem));

	if(create_memory(&my_type,sizeof (struct Mytype),-1) == -1){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}
	

	((struct Mytype*)my_type.p)->number = 45;
	strncpy(((struct Mytype*)my_type.p)->str,"ciao",strlen("ciao")+1);

	printf("your type first member is %d.\n",((struct Mytype*) my_type.p)->number);
	printf("your type second member is %s.\n",((struct Mytype*) my_type.p)->str);


	struct Mem string;
	memset(&string,0,sizeof (struct Mem));
	if(create_memory(&string,strlen("hey what is up??") + 1,STRING) == -1){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}

	strncpy(string.p,"hey what is up",strlen("hey what is up") + 1);

	printf("%s\n",(char*)string.p);	

	struct Mem number_array;
	memset(&number_array,0,sizeof (struct Mem));

	if(create_memory(&number_array,sizeof(int)*2,INT) == -1){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}

	int a = 234, b = 235;
	push(&number_array,(void *)&a,INT);
	push(&number_array,(void *)&b,INT);

	uint64_t i;
	for(i = 0; i < (number_array.size / (sizeof(int))); i++)
#if __APPLE__
		printf("integer at index %llu:%d\n",i,*(int*)value_at_index(i,&number_array,INT));
#else
		printf("integer at index %ld:%d\n",i,*(int*)value_at_index(i,&number_array,INT));
#endif /*__APPLE__*/

	int *r = value_at_index(10,&number_array,INT);

	printf("trying to print out of range, int at index %d:%d.\n",10,r == NULL ? -1 : *r);

	/*create an array of 2 doubles*/
	struct Mem double_array;
	memset(&double_array,0,sizeof(struct Mem));
	if(create_memory(&double_array,sizeof(double)*2,DOUBLE) == -1){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}

	/*add values to the array*/
	double d1 = 23.45, d2 = 12.34;
	push(&double_array,(void *)&d1,DOUBLE);
	push(&double_array,(void *)&d2,DOUBLE);
	for(i = 0; i < (double_array.size / (sizeof(double))); i++)
#if __APPLE__
		printf("double at index %llu:%.2f\n",i,*(double*)value_at_index(i,&double_array,DOUBLE));
#else
		printf("double at index %ld:%.2f\n",i,*(double*)value_at_index(i,&double_array,DOUBLE));
#endif /*__APPLE__*/



	/*allocating memory right after the double array*/
	int *C = (int*)ask_mem(sizeof(int));
	
	

	

	/*expand the array of 2 elements*/
	if(!(double_array.p = (double*)reask_mem(double_array.p,double_array.size,2*sizeof(double)))){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}

	double_array.size += 2*sizeof(double);

	d1 = 11.89;
	d2 = 13.80;
	push(&double_array,(void *)&d1,DOUBLE);
	push(&double_array,(void *)&d2,DOUBLE);
	

	for(i = 0; i < (double_array.size / (sizeof(double))); i++)
#if __APPLE__
		printf("double at index %llu:%.2f\n",i,*(double*)value_at_index(i,&double_array,DOUBLE));
#else
		printf("double at index %ld:%.2f\n",i,*(double*)value_at_index(i,&double_array,DOUBLE));
#endif /*__APPLE__*/



	double *res = (double*)value_at_index(10,&double_array,DOUBLE);
	printf("trying to print out of range, double at index %d:%.2f.\n",10,res == NULL ? -1.00 : *res);

	/* this two calls to cancel_memory are here 
	 * only because i want to test the behavoiur of  
	 * reask_mem()
	 * */
	if(cancel_memory(&number_array,NULL,0) == -1){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}
	if(cancel_memory(&string,NULL,0) == -1){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}


	printf("\n\n\n====== TESTING FALL BACK SYSTEM ============\n\n");		

	struct Mem big_array;
	struct Mem big_array2;
	memset(&big_array,0,sizeof(struct Mem));
	memset(&big_array2,0,sizeof(struct Mem));



	
	
	if(create_memory(&big_array,MEM_SIZE + 4,INT) == -1){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}
	
	if(create_memory(&big_array2,MEM_SIZE + 4,INT) == -1){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}

	if(cancel_memory(&big_array,NULL,0) == -1){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}

	if(cancel_memory(&big_array2,NULL,0) == -1){
		printf("cancel memory failed");	
		close_prog_memory();
		return 0;
	}


	struct Mem string_array[20];
	memset(string_array,0,sizeof(struct Mem)*20);

	for(i = 0; i < 20;i++)
		if(create_memory(&string_array[i],204800,STRING) == -1){
			printf("create memory failed");	
			close_prog_memory();
			return 0;
		}


	/* 
	 * here we should not have anythig free
	 * and this call should fall back to malloc
	 * */


	if(create_memory(&string,100,STRING) == -1){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}
	
	strncpy(string.p,"ciao!!",strlen("ciao!!")+1);

	if(cancel_memory(&string_array[0],NULL,0) == -1){
		printf("cancel memory failed");	
		close_prog_memory();
		return 0;
	}

	if(cancel_memory(&string,NULL,0) == -1){
		printf("cancel memory failed");	
		close_prog_memory();
		return 0;
	}

	close_prog_memory();
	printf("===== FALL BACK SYSTEM WORKED ====\n");
	printf("program succed\n");
	return 0;
}
