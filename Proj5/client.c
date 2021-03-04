#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#define raise printf("Invalid syntax\n")
#define log printf("Here\n")

int sock;
int valread;
struct sockaddr_in server_addr;
struct hostent *hptr;

char message[256];

void update_message(char* buffer);

int main(int argc, char** argv){
    char buffer[256];
    int i;
    char params[3][20];
    char* p;

    int y,m,d;

    memset(params,0,sizeof(params));

    hptr = gethostbyname(argv[1]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = *((unsigned long*)hptr->h_addr_list[0]);
    
    while(1){
        printf("> ");
        fgets(buffer,256,stdin);

        buffer[strlen(buffer)-1] = 0;

        update_message(buffer);

        if(!strlen(buffer)){
            raise;
            continue;
        }

        i = 0;
        p = strtok(buffer," ");
        while(p){
            // if(i > 2){
            //     break;
            // }
            strcpy(params[i++],p);
            p = strtok(NULL," ");
        }

        if(!strcmp(params[0],"quit")){
            break;
        }

        if(!(!strcmp(params[0],"Prices") || !strcmp(params[0],"MaxProfit"))){
            raise;
            continue;
        }

        if(!strcmp(params[0],"Prices")){
            if(i != 3){
                raise;
                continue;
            }

            if(!(!strcmp(params[1],"AAPL") || !strcmp(params[1],"TWTR"))){
                raise;
                continue;
            }

            if(sscanf(params[2],"%d-%d-%d",&y,&m,&d) != 3){
                raise;
                continue;
            }
            
            if (strlen(params[2]) != 10){
            	raise;
            	continue;
			}

            if(m > 12 || d > 31){
                raise;
                continue;
            }

        }

        if(!strcmp(params[0],"MaxProfit")){
            if(i != 2){
                raise;
                continue;
            }

            if(!(!strcmp(params[1],"AAPL") || !strcmp(params[1],"TWTR"))){
                raise;
                continue;
            }
        }
        sock = socket(AF_INET,SOCK_STREAM,0);
        connect(sock,(struct sockaddr*)&server_addr,sizeof(server_addr));

        send(sock,message,strlen(message),0);
        valread = read(sock,message,256);

        valread = message[0];
        for(i=0;i<valread;i++){
            printf("%c",message[i+1]);
        }
        printf("\n");
    }
    
    close(sock);

    return 0;
}

void update_message(char* buffer){
    memset(message,0,sizeof(message));
    strcpy(message+1,buffer);
    message[0] = strlen(buffer);
}
