#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TICKETS 600

#define MAX_STOCK 10

#define MAX_LINE 100

//websock related
int server_fd, new_socket, valread, addrlen;
struct sockaddr_in address;
int opt;

void initialize(int PORT);

//stock related
char Paths[MAX_STOCK][15];
char Names[MAX_STOCK][15];
int Count[MAX_STOCK];

typedef struct data{
    int year;
    int month;
    int day;

    double price;
}ticket;

ticket Stock[MAX_STOCK][MAX_TICKETS];

void load(char* path, int index);
int indexOf(char* name);

//Transaction related
char message[256];
void update_message(char* buffer);

//Database query related
void prices(int index, int year, int month, int day);
void maxProfit(int index);

int main(int argc, char** argv){
    int i;
    size_t message_size;
    char buffer[256];
    char params[3][20];
    char* p;

    int year,month,day;

    i = 0;

    memset(Stock,0,sizeof(Stock));
    memset(Paths,0,sizeof(Paths));
    memset(Names,0,sizeof(Names));
    memset(Count,0,sizeof(Count));

    memset(message,0,sizeof(message));
    memset(buffer,0,sizeof(buffer));
    memset(params,0,sizeof(params));

    for(i=0;i<argc-2;i++){
        strcpy(Paths[i],argv[i+1]);
        load(Paths[i], i);
    }

    initialize(atoi(argv[argc-1]));

    printf("server started\n");

    while(1){
        new_socket = accept(server_fd,(struct sockaddr*)&address,(socklen_t*)&addrlen);
        if(new_socket < 0){
            perror("accept");
            exit(EXIT_FAILURE);
        }
        valread = read(new_socket,message,100);
        message_size = (int)message[0];

        for(i=0;i<message_size;i++){
            buffer[i] = message[i+1];
        }

        buffer[i+1] = 0;
        i = 0;

        p = strtok(buffer," ");
        while(p){
            strcpy(params[i++],p);
            p = strtok(NULL, " ");
        }

        if(!strcmp(params[0], "Prices")){
        	if (!strcmp(params[1],"AAPL")){
        		i = 0;
			}
			else if (!strcmp(params[1], "TWTR")){
				i = 1;
			}
			else {
				i = indexOf(params[1]);
			}
            
            sscanf(params[2],"%d-%d-%d",&year,&month,&day);
            prices(i,year,month,day);
        }

        if(!strcmp(params[0], "MaxProfit")){
            if (!strcmp(params[1],"AAPL")){
        		i = 0;
			}
			else if (!strcmp(params[1], "TWTR")){
				i = 1;
			}
			else {
				i = indexOf(params[1]);
			}
            maxProfit(i);
        }

        memset(buffer,0,sizeof(buffer));
        memset(message,0,sizeof(message));
        memset(params,0,sizeof(params));
    }

    return 0;
}

void initialize(int PORT){
    opt = 1;

    server_fd = socket(AF_INET,SOCK_STREAM,0);
    if(server_fd == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    addrlen = sizeof(address);

    if(bind(server_fd,(struct sockaddr*)&address,sizeof(address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd,3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

void update_message(char* buffer){
    memset(message,0,sizeof(message));
    strcpy(message+1,buffer);
    message[0] = strlen(buffer);
}

void load(char* path, int index){
    FILE* file;
    char line[MAX_LINE];
    char* buffer;
    int i;
    char date[12];
    ticket* TGT;

    memset(line,0,sizeof(line));
    memset(date,0,sizeof(date));
    file = fopen(path,"r");
    if(!file) perror("file opening");
    
    TGT = Stock[index];
    for(i=0;i<strlen(path)-4;i++){
        Names[index][i] = path[i];
    }
    Names[index][i] = 0;

    i = -1;
    while(fgets(line,100,file)){
        if(i == -1){
            i++;
            continue;
        }
        buffer = strtok(line,",");
        strcpy(date,buffer);
        sscanf(date,"%d-%d-%d",&TGT[i].year,&TGT[i].month,&TGT[i].day);
        buffer = strtok(NULL,",");
        buffer = strtok(NULL,",");
        buffer = strtok(NULL,",");
        buffer = strtok(NULL,",");
        TGT[i].price = atof(buffer);
        i++;
    }

    Count[index] = i;

    fclose(file);
    file = NULL;
    return;
}

int indexOf(char* name){
    int i;
    for(i=0;i<MAX_STOCK;i++){
        if(!strcmp(name,Names[i]))
            return i;
    }
    return i;
}

void prices(int index, int year, int month, int day){
    int count;
    ticket* TGT;
    int i;
    char buffer[20];

    memset(buffer,0,sizeof(buffer));

    count = Count[index];
    TGT = Stock[index];
    
    if (index == 0){
    	printf("Prices AAPL %d-%02d-%02d\n",year,month,day);
	} 
	else if (index == 1){
		printf("Prices TWTR %d-%02d-%02d\n",year,month,day);
	}
	else {
		printf("Prices %s %d-%02d-%02d\n",Names[index],year,month,day);
	} 
   

    for(i=0;i<count;i++){
        if(TGT[i].year == year &&
        TGT[i].month == month &&
        TGT[i].day == day){
            sprintf(buffer,"%6.2f",TGT[i].price);
            update_message(buffer);
            send(new_socket,message,strlen(message),0);

            return;
        }
    }

    sprintf(buffer,"Unknown");
    update_message(buffer);
    send(new_socket,message,strlen(message),0);
    return;
}

void maxProfit(int index){
    double min;
    double profit;

    int count;
    ticket* TGT;
    int i;

    char buffer[100];
    char name[20];
    if (index == 0){
    	strcpy(name, "AAPL");
	}
	else if (index == 1){
		strcpy(name, "TWTR");
	}
	else {
		strcpy(name, Names[index]);
	}
 
    memset(buffer,0,sizeof(buffer));

    count = Count[index];
    TGT = Stock[index];
    printf("MaxProfit %s\n",name);

    profit = 0.0;
    min = TGT[0].price;

    for(i=1;i<count;i++){
        if(TGT[i].price < min){
            min = TGT[i].price;
        }
        if(TGT[i].price - min > profit){
            profit = TGT[i].price - min;
        }
    }

    sprintf(buffer,"Maximum Profit for %s: %.2f",name,profit);
    update_message(buffer);
    send(new_socket,message,strlen(message),0);
    return;
}
