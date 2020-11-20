#include "ealloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

#define PAGE_NUM 4
#define PAGE_TABLE_SIZE PAGESIZE / MINALLOC

struct page {
	int page_table[PAGE_TABLE_SIZE];
	char *mem;
};

struct page pages[PAGE_NUM];
int cur_page_size;

int get_offset(struct page *p, int bufsiz) {
	int count = bufsiz / MINALLOC;
	int index = 0;
	for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
		if (p->page_table[i] == -1) {
			if (count == bufsiz / MINALLOC) {
				index = i;
			}
			count--;
		} else {
			count = bufsiz / MINALLOC;
		}

		if (count == 0)
			return index;
	}

	return -1;
}

void init_alloc() {
	for (int i = 0; i < PAGE_NUM; i++) {
		memset(pages[i].page_table, -1, PAGE_TABLE_SIZE);
		pages[i].mem = NULL;
	}
}

char *alloc(int bufsiz) {
	if (bufsiz % MINALLOC != 0) {
		fprintf(stderr, "%d 배수가 아님\n", MINALLOC);
		return NULL;
	}

	if (bufsiz > 4096) {
		fprintf(stderr, "4096 넘음\n");
		return NULL;
	}
	
	int page_index = -1;
	int offset = -1;
	for (int i = 0 ; i < cur_page_size; i++) {
		offset = get_offset(pages + i, bufsiz);
		if (offset != -1) {
			int size = bufsiz / MINALLOC;
			for (int j = 0; j < size; j++) {
				pages[i].page_table[offset + j] = offset + size;
			}
			return pages[i].mem + offset * MINALLOC;
		}
	}

	if (cur_page_size == PAGE_NUM)
		return NULL;

	pages[cur_page_size].mem = mmap((void *) 0, PAGESIZE, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	int size = bufsiz / MINALLOC;
	for (int j = 0; j < size; j++) {
		pages[cur_page_size].page_table[j] = size;
	}

	cur_page_size++;

	return pages[cur_page_size - 1].mem;
}

void dealloc(char *buffer) {
	int page_index = -1;
	int offset = -1;
	for (int i = 0; i < cur_page_size; i++) {
		offset = buffer - pages[i].mem;
		if (offset >= 0 && offset < PAGESIZE) {
			page_index = i;
			break;
		}
	}

	offset /= MINALLOC;

	int target = pages[page_index].page_table[offset];
	for (int i = offset; i < target; i++) {
		pages[page_index].page_table[i] = -1;
	}
}

void cleanup() {
	for (int i = 0; i < PAGE_NUM; i++) {
		memset(pages[i].page_table, 1, PAGE_TABLE_SIZE);
		pages[i].mem = NULL;
	}
}
