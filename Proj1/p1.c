#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 

int main(){
	int d1, d2;
	int sum_hist[11];
	memset(sum_hist, 0, sizeof(int)*11);
	int i, j;
	
	srand((unsigned)time(NULL));
	for (i=0; i<100; i++){
		d1 = rand()%6 + 1;
		d2 = rand()%6 + 1;
		sum_hist[d1+d2-2]++;
	} 
	
	for (i=0; i<11; i++){
		printf("%d: %d\t", i+2, sum_hist[i]);
		for (j=0; j<sum_hist[i]; j++){
			printf("*");
		}
		printf("\n");
	}
	
	return 0;
}



