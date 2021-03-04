#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct Node {
	char name[80];
	int start;
	int end;
	struct Node* prev;
	struct Node* next;
};

int main(){
	char str[100];
	char *params[4];
	struct Node* head = NULL;
	struct Node* e, *prev, *next;
	int i, start, end;
	char *t;
	
	for (i=0; i<4; i++){
		t = (char*)malloc(sizeof(char)*80);
		params[i] = t;
	}
	
	while (1) {
		for(i=0;i<4;i++){
            free(params[i]);
            params[i] = (char*)malloc(sizeof(char)*80);
        }
		
		printf("$ ");
		fgets(str, 100, stdin);
		t = strtok(str, " \n");
		for (i=0; i<4; i++){
			if (!t) {break;}
			strcpy(params[i],t);
			t = strtok(NULL, " \n");
		}
		
		
		if (!strcmp(params[0], "quit")){
			break;
		}
		
		if (!strcmp(params[0], "add")) {
			start = strtol(params[2], NULL, 10);
			end = strtol(params[3], NULL, 10);
			if (start >= end && start != 0 && end != 0){
				printf("Event overlap error\n");
				continue;
			}
			if (!head){
				head = (struct Node*)malloc(sizeof(struct Node));
				strcpy(head->name, params[1]);
				head->start = start;
				head->end = end;
				head->prev = NULL;
				head->next = NULL;
			}
			else {
				e = head;
				if ((head->start==0 && head->end==0) || (start==0 && end==0)){
					printf("Event overlap error\n");
					continue;
				}
				while (e) {
					if (end == 0){
						if (!e->next){
							if (e->end > start || e->end == 0){
								printf("Event overlap error\n");
								break;
							}
							prev = e;
							e = (struct Node*)malloc(sizeof(struct Node));
							strcpy(e->name, params[1]);
							e->start = start;
							e->end = end;
							e->prev = prev;
							e->next = NULL;
							prev->next = e;
							break;
						}
						e = e->next;
						continue;
					}
					
					if (e->start >= end){
						if (!e->prev){
							head = (struct Node*)malloc(sizeof(struct Node));
							strcpy(head->name, params[1]);
							head->start = start;
							head->end = end;
							head->prev = NULL;
							head->next = e;
							e->prev = head;
							break;
						}
						
						else {
							if (e->prev->end > start) {
								printf("Event overlap error\n");
								break;
							}
							prev = e->prev;
							next = e;
							e = (struct Node*)malloc(sizeof(struct Node));
							strcpy(e->name, params[1]);
							e->start = start;
							e->end = end;
							e->prev = prev;
							e->next = next;
							prev->next = e;
							next->prev = e;
							break;
							
						}
					}
					if (!e->next) {
						if (e->end > start || (e->end==0 && end > e->start)){
							printf("Event overlap error\n");
							break;
						}
						prev = e;
						e = (struct Node*)malloc(sizeof(struct Node));
						strcpy(e->name, params[1]);
						e->start = start;
						e->end = end;
						e->prev = prev;
						e->next = NULL;
						prev->next = e;
						break;
					}
					
					e = e->next;
					
				}
			}
			
		}
		
		if (!strcmp(params[0], "printcalendar")) {
			e = head;
			while (e) {
				printf("%s %d %d\n", e->name, e->start, e->end);
				e = e->next;
			}
		}
		
		if (!strcmp(params[0], "delete")) {
			e = head;
			while (e) {
				if (!strcmp(e->name, params[1])){
					if (!e->prev && !e->next) {
						free(e);
						head = NULL;
						break;
					}
					
					prev = e->prev;
					next = e->next;
					if (!prev) {
                    	head = e->next;
                    	head->prev = NULL;
                    	free(e);
                    	break;
					}
					else{
						prev->next = next;
					}
					
					if (next) {
						next->prev = prev;
					}
					free(e);
					break;
				}
				else {
					e = e->next;
				}
			}
		}	
		
	}
	
	for (i=0; i<4; i++){
		free(params[i]);
	}

	return 0;
} 




   
