
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

	MemoryArena arena = {0};
	arena->minimum_block_capacity = minimum_block_capacity;
}

static inline
__memory_arena_free_last_block(MemoryArena* arena)
{
	assert(arena != NULL);

	MemoryArenaBlock* block = arena->head_block;

	free(block->data);
	free(block);
	arena->head_block = block->next;
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

	scope.arena = arena;
	scope.block = arena->head_block;
	scope.top = arena->head_block->top;

	return scope;
}

void
memory_arena_scope_end(MemoryArena* arena, MemoryArenaScope scope)
{
	assert(arena != NULL);

	// scope represent the past, so no need for a pointer to the previous block.
	
	while (arena->head_block != scope.block)
		__memory_arena_free_last_block(arena);
	
	// now arena->head_block is the scope's block.

	arena->head_block->top = scope.top;
	
}

void
memory_arena_clear(MemoryArena* arena)
{
	assert(arena != NULL);

	arena->top = 0;
}

void*
memory_arena_push(MemoryArena* arena, uintptr_t size, uintptr_t alignment)
{
	assert(arena != NULL);
	assert(alignment > 0);
	assert((alignment & (alignment - 1)) == 0);

	uintptr_t current_addr = (uintptr_t)arena->data + arena->top;
	uintptr_t aligned_addr = __align_forward(current_addr, alignment);
	uintptr_t padding = aligned_addr - current_addr;

	if (arena->top + padding + size > arena->capacity) return NULL;

	arena->top += padding + size;

	return (void*)aligned_addr;
}

void
memory_arena_pop(MemoryArena* arena, uintptr_t size)
{
	assert(arena != NULL);

	if (size > arena->top) return;

	arena->top -= size;
}
