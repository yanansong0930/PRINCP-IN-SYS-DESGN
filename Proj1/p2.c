#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 

int main()
{
	char str[101], *s = str, *t = NULL;
	char str2[101];

	fgets(str, 100, stdin);
	
	strcpy(str2, str);

	while ((t = strtok(s, " \t\r\n\v\f")) != NULL){
		printf("%s\n", t);
		s = NULL;
	}
	fputs(str2, stdout);
   
    return 0;
}


