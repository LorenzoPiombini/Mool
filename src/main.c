#include <stdio.h>
#include <string.h>
#include "memory.h"

struct Mytype{
	short number;
	char str[5];
};


int main(void)
{
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
	if(cancel_memory(&string) == -1){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}

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
		printf("integer at index %ld:%d\n",i,*(int*)value_at_index(i,&number_array,INT));

	printf("trying to print out of range, int at index %d:%d.\n",10,*(int*)value_at_index(10,&number_array,INT));
	if(cancel_memory(&number_array) == -1){
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

	if(cancel_memory(&big_array) == -1){
		printf("create memory failed");	
		close_prog_memory();
		return 0;
	}

	if(cancel_memory(&big_array2) == -1){
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

	if(cancel_memory(&string_array[0]) == -1){
		printf("cancel memory failed");	
		close_prog_memory();
		return 0;
	}

	if(cancel_memory(&string) == -1){
		printf("cancel memory failed");	
		close_prog_memory();
		return 0;
	}

	close_prog_memory();
	printf("===== FALL BACK SYSTEM WORKED ====\n");
	printf("program succed\n");
	return 0;
}
