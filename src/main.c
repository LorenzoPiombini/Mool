#include <stdio.h>
#include <string.h>
#include "allocator.h"


int main(void)
{
	A_init_mainzone();

	char *s = A_Malloc(12,M_STATIC,NULL);
	strncpy(s,"Mool is cool",12);
	printf("%*s\n",12,s);


	A_free(s);

	s = A_Malloc(12,M_STATIC,NULL);
	strncpy(s,"Mool is Fool",12);
	printf("%*s\n",12,s);
	

	return 0;
}
