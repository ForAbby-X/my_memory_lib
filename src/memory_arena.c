
#include "memory_arena.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

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

	MemoryArenaBlockFooter* block = arena->head_block;

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
	scope.top = (arena->head_block) ? arena->head_block->top : 0;
	scope.id = ++arena->scope_count;
	
	return scope;
}

void
memory_arena_scope_end(MemoryArenaScope scope)
{
	assert(scope.arena != NULL);
	assert(scope.arena->scope_count > 0);
	assert(scope.id == scope.arena->scope_count);

	MemoryArena* arena = scope.arena;

	//TODO(Alan): Add scope count to stop memory from being freed outside of related scope
	while (arena->head_block != scope.block)
	{
		__memory_arena_free_last_block(arena);
	}

	if (arena->head_block)
	{
		arena->head_block->top = scope.top;
	}

	arena->scope_count--;
}

void
memory_arena_clear(MemoryArena* arena)
{
	assert(arena != NULL);

	while (arena->head_block && arena->head_block->next)
	{
		__memory_arena_free_last_block(arena);
	}
	if (arena->head_block)
		arena->head_block->top = 0;

}

static inline
MemoryArenaBlockFooter*
__memory_arena_new_block(MemoryArena* arena, MemoryArenaBlockFooter* current_block, uintptr_t init_size, uintptr_t alignment)
{
	uintptr_t worst_case_padding = alignment - 1;
	uintptr_t size = init_size + worst_case_padding;
	uintptr_t block_size = sizeof(MemoryArenaBlockFooter) + size;
	uintptr_t memory_block_size = MAX(arena->minimum_block_capacity, block_size);

	MemoryArenaBlockFooter* new_block = malloc(memory_block_size);

	if (new_block == NULL) return NULL;

	*new_block = (MemoryArenaBlockFooter){0};
	new_block->next = current_block;
	new_block->capacity = memory_block_size - sizeof(MemoryArenaBlockFooter);

	//TODO(Alan): Clear to zero the memory in debug (or flag controled ?)

	arena->head_block = new_block;

	return new_block;
}

void*
memory_arena_push(MemoryArena* arena, uintptr_t size, uintptr_t alignment)
{
	assert(arena != NULL);
	assert(alignment > 0);
	assert((alignment & (alignment - 1)) == 0);

	MemoryArenaBlockFooter* block = arena->head_block;

	if (block == NULL)
	{
		block = __memory_arena_new_block(arena, block, size, alignment);
		if (block == NULL)
			return NULL;
	}

	uintptr_t current_addr = (uintptr_t)(block + 1) + block->top;
	uintptr_t aligned_addr = __align_forward(current_addr, alignment);
	uintptr_t padding = aligned_addr - current_addr;

	if (block->top + padding + size > block->capacity)
	{
		block = __memory_arena_new_block(arena, block, size, alignment);
		if (block == NULL)
			return NULL;

		uintptr_t current_addr = (uintptr_t)(block + 1);
		uintptr_t aligned_addr = __align_forward(current_addr, alignment);
		uintptr_t padding = aligned_addr - current_addr;
			
		block->top = padding + size;
		return (void *)aligned_addr;
	}

	block->top += padding + size;

	return (void*)aligned_addr;
}

// inline
// uintptr_t
// GetAlignmentOffset(MemoryArena* arena, uintptr_t alignment)
// {
// 	uintptr_t offset = 0;
// 	uintptr_t result = (uintptr_t)(arena->head_block + 1) + arena->head_block->top;
// 	uintptr_t mask = alignment - 1;

// 	if (result & mask)
// 	{
// 		offset = alignment - (result & mask);
// 	}

// 	return offset;
// }