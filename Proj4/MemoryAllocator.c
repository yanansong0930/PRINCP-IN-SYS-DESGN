#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 80

unsigned char mem[127];

void writeDecorator(int position, int size, int status);
void readDecorator(int position, int* size, int* status);

void writemem(int index, char* string);
void printmem(int index, int length);

void _malloc(int size);
void _free(int index);

void blocklist(void);

int main(int argc,char** argv) {
	//initialize
	char buffer[BUFFER_SIZE];
	size_t size;
	char params[3][BUFFER_SIZE];
	int i;
	char* p;

	memset(mem,0,sizeof(char)*127);
	writeDecorator(0, 127, 0);
	writeDecorator(126, 127, 0);

	memset(buffer, 0, sizeof(char) * BUFFER_SIZE);
	memset(params, 0, sizeof(char) * BUFFER_SIZE * 3);
	i = 0;
	//start loop
	while (1) {
		printf(">");
		fgets(buffer, BUFFER_SIZE, stdin);
		buffer[strlen(buffer) - 1] = 0;	//rstrip
		i = 0;
		p = strtok(buffer, " ");
		while (p) {
			strcpy(params[i++], p);
			p = strtok(NULL, " ");
		}

		if (!strcmp(params[0], "quit")) {
			break;
		}

		if (!strcmp(params[0], "blocklist")) {
			blocklist();
		}

		if (!strcmp(params[0], "writemem")) {
			writemem(atoi(params[1]), params[2]);
		}

		if (!strcmp(params[0], "printmem")) {
			printmem(atoi(params[1]), atoi(params[2]));
		}

		if (!strcmp(params[0], "malloc")) {
			_malloc(atoi(params[1]));
		}

		if (!strcmp(params[0], "free")) {
			_free(atoi(params[1]));
		}
		

		memset(buffer, 0, sizeof(char) * BUFFER_SIZE);
		memset(params, 0, sizeof(char) * BUFFER_SIZE * 3);
	}

	return 0;
}

void blocklist() {
	int i;
	int size, status;
	i = 0;
	while (i < 127) {
		readDecorator(i, &size, &status);
		if (status) {
			printf("%d, %d, allocated\n", i + 1, size - 2);
		}
		else {
			printf("%d, %d, free\n", i + 1, size - 2);
		}
		i += size;
	}
}

void writemem(int index, char* string) {
	int i;
	for (i = 0; i < strlen(string); i++) {
		mem[i + index] = string[i];
	}
}

void printmem(int index, int length) {
	int i;
	for (i = 0; i < length; i++) {
		printf("%x ", mem[i + index]);
	}
	printf("\n");
}

void writeDecorator(int position, int size, int status) {
	if (!(mem[position] & 0x01)){
		mem[position] = size << 1;
		if (status) {
			mem[position] = mem[position] | 0x01;
		}
		else {
			mem[position] = mem[position] & 0xFE;
		}
	}
}

void readDecorator(int position, int* size, int* status) {
	if (mem[position] & 0x01) {
		*status = 1;
	}
	else {
		*status = 0;
	}

	*size = (int)(mem[position] >> 1) & 0x7F;
}

void _malloc(int size) {
	//first-fit strategy
	int i,space,status;
	int h0, f0, h1, f1, h2, f2;
	i = 0;
	while (i < 127) {
		readDecorator(i, &space, &status);
		if (!status && (space-1) > size) {
			h0 = i;
			f0 = h0 + space - 1;
			h1 = h0;
			f1 = h1 + size + 2 - 1;
			h2 = h1 + size + 2;
			f2 = f0;
			writeDecorator(h1, size + 2, 1);
			writeDecorator(f1, size + 2, 1);
			writeDecorator(h2, f2 - h2 + 1, 0);
			writeDecorator(f2, f2 - h2 + 1, 0);
			printf("%d\n", h1 + 1);
			return;
		}
		else {
			i += space;
		}
	}
	printf("Memory allocation failed. Full Heap.\n");
}

void coalesce_back(int index) {
	int h0, f0, h1, f1, h2, f2;
	int size, status;
	readDecorator(index, &size, &status);
	h1 = index;
	f1 = index + size - 1;
	h2 = index + size;
	readDecorator(h2, &size, &status);
	f2 = h2 + size - 1;
	h0 = h1;
	f0 = f2;
	writeDecorator(h0,f0-h0+1,0);
	writeDecorator(f0, f0 - h0 + 1, 0);
	mem[f1] = 0x00;
	mem[h2] = 0x00;
}

void coalesce_front(int index) {
	int h0, f0, h1, f1, h2, f2;
	int size, status;
	readDecorator(index, &size, &status);
	h2 = index;
	f2 = h2 + size - 1;
	f1 = h2 - 1;
	readDecorator(f1, &size, &status);
	h1 = f1 + 1 - size;
	h0 = h1;
	f0 = f2;
	writeDecorator(h0, f0 - h0 + 1, 0);
	writeDecorator(f0, f0 - h0 + 1, 0);
	mem[f1] = 0x00;
	mem[h2] = 0x00;
}

void _free(int index) {
	int size, status;
	int i;
	index--;
	readDecorator(index, &size, &status);
	mem[index] = mem[index] & 0xFE;
	mem[index + size - 1] = mem[index + size - 1] & 0xFE;
	for (i = index + 1; i < index + size - 1; i++) {
		mem[i] = 0x00;
	}
	
	//check back
	readDecorator(index + size, &size, &status);
	if (!status) {
		coalesce_back(index);
	}
	//check front
	if (index == 0) return;
	readDecorator(index - 1, &size, &status);
	if (!status) {
		coalesce_front(index);
	}
}
