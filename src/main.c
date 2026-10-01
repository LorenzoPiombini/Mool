#include <stdio.h>
#include <string.h>
#include "allocator.h"


int main(void)
{
	A_init_mainzone();

	char *s = A_Malloc(12,M_STATIC,NULL);
	strncpy(s,"Mool is cool",12);
	printf("%*s\n",12,s);


	s = A_Realloc(s,20,M_STATIC,NULL);
	if (!s) return 0;

	strncpy(&s[12],"!!!!",4);
	printf("%*s\n",16,s);
	return 0;
}
