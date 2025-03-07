
#include "memory_arena.h"
#include <stdio.h>

int main()
{
	MemoryArena arena = memory_arena_create(1024 * 1024);

	double* double_ptr = memory_arena_alloc(&arena, double);

	*double_ptr = 25.67;

	printf("double's value: %f\n", *double_ptr);

	printf("(void*) size: %d\n", sizeof(void*));
	printf("(uintptr_t) size: %d\n", sizeof(uintptr_t));

	memory_arena_destroy(&arena);
	return 0;
}
