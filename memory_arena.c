
#include "memory_arena.h"
#include <stdlib.h>
#include <assert.h>

static inline
uintptr_t
__align_forward(uintptr_t addr, uintptr_t align)
{
	return (addr + (align - 1)) & ~(align - 1);
}

void
memory_arena_init(MemoryArena* arena, uintptr_t minimum_block_capacity)
{
	assert(minimum_block_capacity > 0);

	*arena = (MemoryArena){0};
	arena->minimum_block_capacity = minimum_block_capacity;
}

static inline
void
__memory_arena_free_last_block(MemoryArena* arena)
{
	assert(arena != NULL);

	MemoryArenaBlock* block = arena->head_block;

	free(block->data);
	arena->head_block = block->next;
	free(block);
}

void
memory_arena_destroy(MemoryArena* arena)
{
	assert(arena != NULL);

	while (arena->head_block)
		__memory_arena_free_last_block(arena);
}

MemoryArenaScope
memory_arena_scope_start(MemoryArena* arena)
{
	assert(arena != NULL);

	MemoryArenaScope scope = {0};

	if (arena->head_block == NULL)
		return (MemoryArenaScope){0};

	scope.arena = arena;
	scope.block = arena->head_block;
	scope.top = arena->head_block->top;

	return scope;
}

// TODO(Alan): Could be better to not free memory for speed
void
memory_arena_scope_end(MemoryArena* arena, MemoryArenaScope scope)
{
	assert(arena != NULL);

	// scope represent the past, so no need for a pointer to the previous block.
	while (arena->head_block != scope.block)
		__memory_arena_free_last_block(arena);
	
	// now arena->head_block is the scope's block.
	if (arena->head_block)
		arena->head_block->top = scope.top;
}

// TODO(Alan): Could be better to not free memory for speed
void
memory_arena_clear(MemoryArena* arena)
{
	assert(arena != NULL);

	while (arena->head_block && arena->head_block->next)
		__memory_arena_free_last_block(arena);
	if (arena->head_block)
		arena->head_block->top = 0;
}

static inline
MemoryArenaBlock*
__memory_arena_new_block(MemoryArena* arena, MemoryArenaBlock* current_block, uintptr_t size, uintptr_t alignment)
{
	size_t alloc_size = MAX(arena->minimum_block_capacity, size);
	alloc_size = __align_forward(alloc_size, alignment);

	MemoryArenaBlock* new_block = malloc(sizeof(MemoryArenaBlock));

	if (new_block == NULL) return NULL;

	// We define the new block of memory
	*new_block = (MemoryArenaBlock){0};
	new_block->data = malloc(alloc_size);
	new_block->capacity = alloc_size;
	new_block->top = __align_forward(size, alignment);
	new_block->next = current_block;

	if (new_block->data == NULL)
	{
		free(new_block);
		return NULL;
	}

	arena->head_block = new_block;

	return new_block;
}

void*
memory_arena_push(MemoryArena* arena, uintptr_t size, uintptr_t alignment)
{
	assert(arena != NULL);
	assert(alignment > 0);
	assert((alignment & (alignment - 1)) == 0);

	MemoryArenaBlock* block = arena->head_block;

	if (block == NULL)
	{
		block = __memory_arena_new_block(arena, block, size, alignment);
		if (block == NULL)
			return NULL;
	}

	uintptr_t current_addr = (uintptr_t)block->data + block->top;
	uintptr_t aligned_addr = __align_forward(current_addr, alignment);
	uintptr_t padding = aligned_addr - current_addr;

	if (block->top + padding + size > block->capacity)
	{
		if (__memory_arena_new_block(arena, block, size, alignment) == NULL)
			return NULL;

		// INFO(Alan): malloc should already align the memory
		return arena->head_block->data;
	}

	block->top += padding + size;

	return (void*)aligned_addr;
}
