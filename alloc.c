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
	// PRIVATE, ANONYMOUS 로 메모리 맵핑
	map_addr = (char*) mmap((void*) 0, PAGESIZE, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (map_addr == (void *) -1) {
		fprintf(stderr, "mmap 오류\n");
		return -2;
	}

	// 페이지 가용 여부를 초기화
	// -1 이면 사용 가능
	// -1 이 아닌 정수는 페이지의 범위를 의미.
	// 예를 들어 page_table[10] = 20 이라면 10 ~ 20까지가 하나의 chunk 로 사용되는 중임을 의미
	for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
		page_table[i] = -1;
	}

	return 0;
}

int cleanup() {
	// 맵핑 해제 전 sync
	if (msync(map_addr, PAGESIZE, MS_SYNC) < 0)
		return -1;

	// 맵핑 해제
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

	// 할당받은 4KB 이상의 페이지를 요구한 경우
	if (bufsiz > PAGESIZE) {
		fprintf(stderr, "4KB 넘음\n");
		return NULL;
	}

	// 논리적 페이지 단위로 변환
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

		// 최초 적합
		if (count == 0) {
			break;
		}
	}

	// 적절한 공간을 찾지못함. 즉 여유공간이 부족한 경우
	if (count > 0) {
		fprintf(stderr, "여유공간부족\n");
		return NULL;
	}

	// 페이지 테이블 상에 사용 여부(사용)를 기록
	count = bufsiz / MINALLOC;
	for (int i = 0; i < count; i++) {
		page_table[page_offset + i] = page_offset + count;
	}

	return map_addr + page_offset * MINALLOC;
}

void dealloc(char *buffer) {
	int offset = (buffer - map_addr) / MINALLOC;
	int size = page_table[offset];

	// 페이지 테이블 상에 사용 여부(미사용)를 기록
	for (int i = offset; i < size; i++) {
		page_table[i] = -1;
	}
}

