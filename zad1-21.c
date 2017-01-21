#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <stdlib.h>

char *p;

// helper function to find out total size
unsigned int getsize() {
	unsigned int *heap_size = (unsigned int*)p;
	return *heap_size;
}

void *memory_alloc(unsigned int size)
{
	short *length, *next;
	void *ptr = p;
	void *current = (unsigned int *)ptr + 1;
	int diff;

	while ((diff = ((char *)ptr + sizeof(unsigned int) + getsize()) - ((char *)current + sizeof(short) + size)) >= 0) {
		// we are still in allowed range

		length = (short *)current;
		if (*length < 0) {
			// allocate new block at place of previously freed block
			if (-*length >= size) { // visual studio gives error but it s managed by previous if
				*length = -*length;
				if (*length - size > sizeof(short)) {
					// divide
					next = (short *)((char *)current + sizeof(short) + size);
					*next = -(short)(*length - size - sizeof(short));
					*length = size;
				}
				return length + 1;
			}
			else {
				// try next
				current = (char *)current + sizeof(short) + (-*length);
			}
		}
		else if (*length == 0) {
			// allocate new block after last end
			if (diff >= sizeof(short)) {
				next = (short *)((char *)current + sizeof(short) + size);
				*next = 0;
			}
			*length = size;
			return length + 1;
		}
		else {
			// try next
			current = (char *)current + sizeof(short) + *length;
		}
	}

	return NULL;

}

int memory_free(void *valid_ptr)
{
	short *before_length, *current_length, *next_length;
	void *ptr = p;
	void *start = (short *)valid_ptr - 1;
	void *before = NULL;
	void *current = (unsigned int *)ptr + 1;
	void *next;
	int diff;

	// find previous block
	while (start > current) {

		before = current;

		current_length = (short *)current;
		if (*current_length > 0) {
			current = (char *)current + sizeof(short) + *current_length;
		}
		else {
			current = (char *)current + sizeof(short) + (-*current_length);
		}
	}
	if (start != current) {
		// unexpected
		current_length = (short *)start;
		*current_length = -*current_length;
		return 0;
	}

	current_length = (short *)current;

	// join with after block
	diff = ((char *)ptr + sizeof(unsigned int) + getsize()) - ((char *)current + sizeof(short) + *current_length);
	if (diff >= sizeof(short)) {
		next = (char *)current + sizeof(short) + *current_length;
		next_length = (short *)next;
		if (*next_length < 0) {
			*current_length = *current_length + (short)sizeof(short) + (-*next_length);
		}
		else if (*next_length == 0) {
			*current_length = 0;
		}
	}
	else {
		*current_length = 0;
	}

	// join with before block
	if (before != NULL) {

		before_length = (short *)before;
		if (*before_length < 0) {
			if (*current_length == 0) {
				*before_length = 0;
			}
			else {
				*before_length = *before_length - (short)sizeof(short) - *current_length;
			}
			return 0;
		}
	}

	// freed sign is negative
	*current_length = -*current_length;
	return 0;
}

int memory_check(void *ptr)
{
	void *min = (unsigned int*)p + 1;
	void *max = (char*)min + getsize();
	short *start = (short*)ptr;

	if ((char *)min + sizeof(short) <= (char *)ptr &&
		(char *)ptr < (char *)max - sizeof(short) &&
		*(start - 1) > 0) {
		return 1;
	}
	return 0;
}

void memory_init(void *ptr, unsigned int size)
{
	// total size wil be stored as unsigned int at the very beginning
	unsigned int *heap_size = (unsigned int*)ptr;

	// head of block will be short containing size of allocated block only
	short *first = (short *)((unsigned int *)ptr + 1);
	*first = 0;

	*heap_size = size - (unsigned int)sizeof(unsigned int); // size - size of size

	p = ptr;
}


int test_one(int total, unsigned int regionSize, unsigned int minBlockSize, unsigned int maxBlockSize, int differentBlocks) {

	int i, j;
	char *region;
	char **blocks = (char **)malloc(total * sizeof(char *));
	int *blockSizes = (int *)malloc(total * sizeof(int));

	for (i = 0; i < total; i++) {
		if (differentBlocks) {
			*(blockSizes + i) = rand() % (maxBlockSize - minBlockSize) + minBlockSize;
		}
		else {
			*(blockSizes + i) = minBlockSize;
		}
	}

	region = (char*)malloc(regionSize * sizeof(char));
	memory_init(region, regionSize);

	// allocate
	for (i = 0; i < total; i++) {

		blocks[i] = (char *)memory_alloc(*(blockSizes + i));

		if (i == 1) {

			if (memory_check(blocks[i]) == 0) {
				printf("Not a valid pointer.\n");
				return 0;
			}

			for (j = 0; j < *(blockSizes + i); j++) {
				*(blocks[i] + j) = 'A';
			}
		}

		if (i == 3) {

			if (memory_check(blocks[i]) == 0) {
				printf("Not a valid pointer.\n");
				return 0;
			}

			for (j = 0; j < *(blockSizes + i); j++) {
				*(blocks[i] + j) = 'B';
			}
		}
	}

	// free
	for (i = total - 1; i >= 0; i--) {

		if (i == 1) {

			for (j = 0; j < *(blockSizes + i); j++) {
				if (*(blocks[i] + j) != 'A') {
					printf("Followed sequence was unexpectedly changed.\n");
					return 0;
				}
			}
		}

		if (i == 3) {

			for (j = 0; j < *(blockSizes + i); j++) {
				if (*(blocks[i] + j) != 'B') {
					printf("Followed sequence was unexpectedly changed.\n");
					return 0;
				}
			}
		}

		if (blocks[i] != NULL) {
			memory_free(blocks[i]);
		}
	}

	free(region);
	printf("OK total:%d region:%u min:%u max%u different:%d\n", total, regionSize, minBlockSize, maxBlockSize, differentBlocks);
	return 1;
}



// vrati 0 ak testy prebehli OK, inak vrati 1
int memory_test()
{
	int total;
	for (total = 10; total <= 50; total += 10) {
		
		// prideľovanie rovnakých blokov malej veľkosti (veľkosti 8 až 24 bytov)
		if (!test_one(total, 50, 8, 8, 0)) {
			return 1;
		}
		if (!test_one(total, 100, 8, 8, 0)) {
			return 1;
		}
		if (!test_one(total, 100, 16, 16, 0)) {
			return 1;
		}
		if (!test_one(total, 200, 8, 8, 0)) {
			return 1;
		}
		if (!test_one(total, 200, 16, 16, 0)) {
			return 1;
		}
		if (!test_one(total, 200, 24, 24, 0)) {
			return 1;
		}

		// prideľovanie nerovnakých blokov malej veľkosti (náhodné veľkosti 8 až 24 bytov)
		if (!test_one(total, 100, 8, 16, 1)) {
			return 1;
		}
		if (!test_one(total, 200, 8, 16, 1)) {
			return 1;
		}
		if (!test_one(total, 200, 8, 24, 1)) {
			return 1;
		}
		if (!test_one(total, 200, 16, 24, 1)) {
			return 1;
		}

		// prideľovanie nerovnakých blokov väčšej veľkosti (veľkosti 500 až 5000 bytov)
		if (!test_one(total, 5000, 128, 256, 1)) {
			return 1;
		}
		if (!test_one(total, 5000, 128, 512, 1)) {
			return 1;
		}
		if (!test_one(total, 5000, 128, 1024, 1)) {
			return 1;
		}
		if (!test_one(total, 5000, 256, 512, 1)) {
			return 1;
		}
		if (!test_one(total, 5000, 256, 1024, 1)) {
			return 1;
		}
		if (!test_one(total, 5000, 512, 1024, 1)) {
			return 1;
		}
		if (!test_one(total, 10000, 128, 256, 1)) {
			return 1;
		}
		if (!test_one(total, 10000, 128, 512, 1)) {
			return 1;
		}
		if (!test_one(total, 10000, 128, 1024, 1)) {
			return 1;
		}
		if (!test_one(total, 10000, 256, 512, 1)) {
			return 1;
		}
		if (!test_one(total, 10000, 256, 1024, 1)) {
			return 1;
		}
		if (!test_one(total, 10000, 512, 1024, 1)) {
			return 1;
		}

		// prideľovanie nerovnakých blokov malých a veľkých veľkostí (veľkosti od 8 bytov do 50 000)
		if (!test_one(total, 25000, 8, 4096, 1)) {
			return 1;
		}
		if (!test_one(total, 25000, 64, 4096, 1)) {
			return 1;
		}
		if (!test_one(total, 25000, 256, 4096, 1)) {
			return 1;
		}
		if (!test_one(total, 25000, 512, 4096, 1)) {
			return 1;
		}
		if (!test_one(total, 25000, 1024, 4096, 1)) {
			return 1;
		}
		if (!test_one(total, 50000, 8, 4096, 1)) {
			return 1;
		}
		if (!test_one(total, 50000, 64, 4096, 1)) {
			return 1;
		}
		if (!test_one(total, 50000, 256, 4096, 1)) {
			return 1;
		}
		if (!test_one(total, 50000, 512, 4096, 1)) {
			return 1;
		}
		if (!test_one(total, 50000, 1024, 4096, 1)) {
			return 1;
		}
		if (!test_one(total, 50000, 8, 8192, 1)) {
			return 1;
		}
		if (!test_one(total, 50000, 64, 8192, 1)) {
			return 1;
		}
		if (!test_one(total, 50000, 256, 8192, 1)) {
			return 1;
		}
		if (!test_one(total, 50000, 512, 8192, 1)) {
			return 1;
		}
		if (!test_one(total, 50000, 1024, 8192, 1)) {
			return 1;
		}
		if (!test_one(total, 50000, 2048, 8192, 1)) {
			return 1;
		}
		if (!test_one(total, 50000, 4096, 8192, 1)) {
			return 1;
		}

	}


	return 0;
}

int main()
{
	int d;

	if (memory_test())
		printf("Implementacia je chybna\n");
	else
		printf("Implementacia je OK\n");

	scanf("%d", &d);

	return 0;
}

