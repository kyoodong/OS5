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


/**
  * @param p 검색할 페이지 (4KB)
  * @param bufsiz 요청한 메모리의 크기
  */
int get_offset(struct page *p, int bufsiz) {
	int count = bufsiz / MINALLOC;
	int index = 0;

	// 최초적합
	for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
		if (p->page_table[i] == -1) {
			if (count == bufsiz / MINALLOC) {
				index = i;
			}
			count--;
		} else {
			count = bufsiz / MINALLOC;
		}

		// 최초 적합
		if (count == 0)
			return index;
	}

	return -1;
}

void init_alloc() {
	// 페이지 테이블 초기화
	for (int i = 0; i < PAGE_NUM; i++) {
		memset(pages[i].page_table, -1, PAGE_TABLE_SIZE);
		pages[i].mem = NULL;
	}
}

char *alloc(int bufsiz) {
	// 256 의 배수가 아닌 메모리를 요청한 경우
	if (bufsiz % MINALLOC != 0) {
		fprintf(stderr, "%d 배수가 아님\n", MINALLOC);
		return NULL;
	}

	// PAGESIZE 보다 큰 메모리를 요청한 경우
	if (bufsiz > PAGESIZE) {
		fprintf(stderr, "4096 넘음\n");
		return NULL;
	}
	
	int page_index = -1;
	int offset = -1;

	// 현재까지 할당된 모든 페이지 내에서 요청한 메모리를 할당할 수 있는지 검색
	for (int i = 0 ; i < cur_page_size; i++) {
		// 요쳥한 페이지 할당 가능한지 검색
		offset = get_offset(pages + i, bufsiz);

		// 할당 가능한 경우
		if (offset != -1) {
			// 페이지 테이블에 사용 여부(사용)을 기록한 뒤 리턴
			int size = bufsiz / MINALLOC;
			for (int j = 0; j < size; j++) {
				pages[i].page_table[offset + j] = offset + size;
			}
			return pages[i].mem + offset * MINALLOC;
		}
	}

	///////////////// 현재까지 할당된 모든 페이지에서 요쳥한 메모리를 할당할 수 없음


	// 이미 4개의 페이지를 모두 할당하여 더 이상 할당할 수 없는 상태
	if (cur_page_size == PAGE_NUM)
		return NULL;

	///////////////// 아직 추가 페이지를 할당할 수 있는 상태


	// 추가 페이지 할당 작업
	pages[cur_page_size].mem = mmap((void *) 0, PAGESIZE, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	// 추가 페이지를 할당했을 때는 0번 주소부터 바로 할당처리하여 리턴
	int size = bufsiz / MINALLOC;
	for (int j = 0; j < size; j++) {
		pages[cur_page_size].page_table[j] = size;
	}

	// 현재 페이지 할당 수 + 1
	cur_page_size++;

	return pages[cur_page_size - 1].mem;
}

void dealloc(char *buffer) {
	int page_index = -1;
	int offset = -1;

	// 몇 번 페이지에서 할당된 메모리인지 검색
	for (int i = 0; i < cur_page_size; i++) {
		offset = buffer - pages[i].mem;

		// 메모리 기준 주소와의 차가 4096 미만이면 해당 페이지임을 의미
		if (offset >= 0 && offset < PAGESIZE) {
			page_index = i;
			break;
		}
	}

	offset /= MINALLOC;

	// 페이지 테이블에 사용 여부(미사용)를 기록
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
