#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define VM_SIZE 64
#define MEM_SIZE 32

#define CALL printf("A Page Fault Has Occurred\n")

char buffer[80];
char params[3][80];

int disk[VM_SIZE];
int mem[MEM_SIZE];

int freq[MEM_SIZE >> 3];

int (*replace)();

typedef struct PageEntry {
	int valid;
	int dirty;
	int pageNum;
} PageEntry;

PageEntry table[VM_SIZE / 8];

void initialize(void);
void clearBuffer(void);

int FIFO();
int LRU();

void showmain(int ppn);
void showdisk(int dpn);
void showtable(void);

void bubble(int ppn);
int free_page(void);

void write(int addr, int value);
void read(int addr);

int main(int argc, char** argv) {
	int i;
	char* p;
	
	if (argc > 1 && !strcmp(argv[argc-1], "LRU")) {
		replace = LRU;
	}
	else {
		replace = FIFO;
	}

	initialize();

	while (1) {
		printf("> ");
		fgets(buffer, 80, stdin);
		buffer[strlen(buffer)-1] = 0; //rstrip
		i = 0;
		p = strtok(buffer, " ");
		while (p) {
			strcpy(params[i++], p);
			p = strtok(NULL, " ");
		}
		
		if (!strcmp(params[0], "quit")) {
			clearBuffer();
			break;
		}

		if (!strcmp(params[0], "showmain")) {
			showmain(atoi(params[1]));
		}

		if (!strcmp(params[0], "showdisk")) {
			showdisk(atoi(params[1]));
		}

		if (!strcmp(params[0], "showptable")) {
			showtable();
		}

		if (!strcmp(params[0], "read")) {
			read(atoi(params[1]));
		}

		if (!strcmp(params[0], "write")) {
			write(atoi(params[1]),atoi(params[2]));
		}


		clearBuffer();
	}

	return 1;
}

void initialize() {
	int i;

	memset(disk, -1, sizeof(int) * VM_SIZE);
	memset(mem, -1, sizeof(int) * MEM_SIZE);
	memset(freq, 0, sizeof(int) * (MEM_SIZE >> 3));
	for (i = 0; i < VM_SIZE / 8; i++) {
		table[i].valid = 0;
		table[i].dirty = 0;
		table[i].pageNum = i;
	}
	clearBuffer();
}

void clearBuffer() {
	memset(buffer, 0, sizeof(char) * 80);
	memset(params[0], 0, sizeof(char) * 80);
	memset(params[1], 0, sizeof(char) * 80);
	memset(params[2], 0, sizeof(char) * 80);
}

int FIFO() {
	int max,i,index;
	max = -1;
	index = 0;
	for (i = 0; i < MEM_SIZE >> 3; i++) {
		if (freq[i] > max) {
			max = freq[i];
			index = i;
		}
	}
	freq[index] = 0;
	return index;
}

int LRU() {
	int max, i, index;
	max = -1;
	index = 0;
	for (i = 0; i < MEM_SIZE >> 3; i++) {
		if (freq[i] > max) {
			max = freq[i];
			index = i;
		}
	}
	freq[index] = 0;
	return index;
}

void showmain(int ppn) {
	int i;
	if (ppn >= MEM_SIZE >> 3) return;
	for (i = ppn * 8; i < ppn * 8 + 8; i++) {
		printf("%d:%d\n", i, mem[i]);
	}
}

void showdisk(int dpn) {
	int i;
	if (dpn >= VM_SIZE >> 3) return;
	for (i = dpn * 8; i < dpn * 8 + 8; i++) {
		printf("%d:%d\n", i, disk[i]);
	}
}

void showtable() {
	int i;
	for (i = 0; i < VM_SIZE >> 3; i++) {
		printf("%d:%d:%d:%d\n", i, table[i].valid, table[i].dirty, table[i].pageNum);
	}
}

void bubble(int ppn) {
	int i;
	if (replace == FIFO) {
		freq[ppn]++;
		for (i = 0; i < MEM_SIZE >> 3; i++) {
			if (freq[i]) {
				freq[i]++;
			}
		}
		freq[ppn]--;
	}
	else {
		for (i = 0; i < MEM_SIZE >> 3; i++) {
			freq[i]++;
		}
		freq[ppn] = 0;
	}
	return;
}

int free_page() {
	int i;
	int vm_id, mem_id;
	int old;
	unsigned char FREE[MEM_SIZE >> 3];
	memset(FREE, 0, sizeof(unsigned char) * (MEM_SIZE >> 3));
	for (i = 0; i < VM_SIZE >> 3; i++) {
		if (table[i].valid) {
			FREE[table[i].pageNum] = 1;
		}
	}
	for (i = 0; i < MEM_SIZE >> 3; i++) {
		if (!FREE[i]) {
			return i;
		}
	}

	old = replace(); //FIFO or LRU

	for (i = 0; i < VM_SIZE >> 3; i++) {
		if (table[i].valid && table[i].pageNum == old) {
			table[i].valid = 0;
			vm_id = i * 8;
			mem_id = table[i].pageNum * 8;
			table[i].pageNum = i;
			if (table[i].dirty) {
				table[i].dirty = 0;
				for (i = 0; i < 8; i++) {
					disk[vm_id + i] = mem[mem_id + i];
					mem[mem_id + i] = -1;
				}
			}
			else {
				for (i = 0; i < 8; i++) {
					mem[mem_id + i] = -1;
				}
			}
			break;
		}
	}
	return old;
}

void write(int addr, int value) {
	int page;
	int i, j;
	int mem_page;

	if (addr >= VM_SIZE) return;

	page = addr >> 3;

	if (table[page].valid) {
		mem[table[page].pageNum * 8 + addr % 8] = value;
		table[page].dirty = 1;
		bubble(table[page].pageNum);
	}
	else {
		CALL;
		mem_page = free_page();
		table[page].valid = 1;
		table[page].dirty = 1;
		table[page].pageNum = mem_page;
		j = 0;
		for (i = page * 8; i < page * 8 + 8; i++) {
			mem[mem_page*8 + j++] = disk[i];
		}
		mem[table[page].pageNum * 8 + addr % 8] = value;
		bubble(mem_page);
	}
}


void read(int addr) {
	int page;
	int i,j;
	int mem_page;

	if (addr >= VM_SIZE) return;

	page = addr >> 3;
	
	if (table[page].valid) {
		printf("%d\n", mem[table[page].pageNum * 8 + addr % 8]);
		bubble(table[page].pageNum);
	}
	else {
		CALL;
		mem_page = free_page();
		table[page].valid = 1;
		table[page].dirty = 0;
		table[page].pageNum = mem_page;
		j = 0;
		for (i = page * 8; i < page * 8 + 8; i++) {
			mem[mem_page*8 + j++] = disk[i];
		}
		printf("%d\n", mem[table[page].pageNum * 8 + addr % 8]);
		bubble(mem_page);
	}
}
