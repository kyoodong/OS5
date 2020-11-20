#include "alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define PAGE_TABLE_SIZE PAGESIZE / MINALLOC

char *map_addr;
int page_table[PAGE_TABLE_SIZE];

int init_alloc() {
	map_addr = (char*) mmap((void*) 0, PAGESIZE, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (map_addr == (void *) -1) {
		fprintf(stderr, "mmap 오류\n");
		return -2;
	}

	for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
		page_table[i] = -1;
	}

	return 0;
}

int cleanup() {
	if (msync(map_addr, PAGESIZE, MS_SYNC) < 0)
		return -1;

	if (munmap(map_addr, PAGESIZE) < 0)
		return -2;
	return 0;
}

char *alloc(int bufsiz) {
	// 8의 배수가 아닌 버퍼 사이즈를 요구
	if (bufsiz % MINALLOC != 0) {
		fprintf(stderr, "8의 배수 아님\n");
		return NULL;
	}

	if (bufsiz > PAGESIZE) {
		fprintf(stderr, "4KB 넘음\n");
		return NULL;
	}

	int count = bufsiz / MINALLOC;
	int page_offset;

	// 최초 적합
	for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
		if (page_table[i] == -1) {
			if (count == bufsiz / MINALLOC) {
				page_offset = i;
			}
			count--;
		} else {
			count = bufsiz / MINALLOC;
		}

		if (count == 0) {
			break;
		}
	}

	// 여유공간이 부족한 경우
	if (count > 0) {
		fprintf(stderr, "여유공간부족\n");
		return NULL;
	}

	count = bufsiz / MINALLOC;
	for (int i = 0; i < count; i++) {
		page_table[page_offset + i] = page_offset + count;
	}

	return map_addr + page_offset * MINALLOC;
}

void dealloc(char *buffer) {
	int offset = (buffer - map_addr) / MINALLOC;
	int size = page_table[offset];

	for (int i = offset; i < size; i++) {
		page_table[i] = -1;
	}
}

