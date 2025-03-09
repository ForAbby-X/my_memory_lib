
#ifndef MEMORY_ARENA_H
# define MEMORY_ARENA_H

# include <inttypes.h>
# include <stdalign.h>

/*
| #MEMORE_ARENA
|
|| #MEMORY_BLOCK :LINKED_LIST
|| >data
|| >capacity
|| >stack_top
|| >next
|
|| #MEMORY_BLOCK :LINKED_LIST
|| >data
|| >capacity
|| >stack_top
|| >next
|
|| #MEMORY_BLOCK :LINKED_LIST
|| >data
|| >capacity
|| >stack_top
|| >next
|
| >LAST_BLOCK
| >BLOCK_COUNT
|
*/

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) > (Y)) ? (Y) : (X))

typedef struct MemoryArenaBlock
{
	struct MemoryArenaBlock* next;
	uintptr_t capacity;
	uintptr_t top;
	uint8_t* data; // TODO(Alan): Make the MemoryArenaBlock allocated in one block with the data
}
MemoryArenaBlock;

typedef struct
{
	MemoryArenaBlock* head_block;
	uintptr_t minimum_block_capacity;
}
MemoryArena;

typedef struct
{
	MemoryArena* arena;
	MemoryArenaBlock* block;
	uintptr_t top;
}
MemoryArenaScope;


void
memory_arena_init(MemoryArena* arena, uintptr_t minimum_block_capacity);

void
memory_arena_destroy(MemoryArena* arena);

MemoryArenaScope
memory_arena_scope_start(MemoryArena* arena);

void
memory_arena_scope_end(MemoryArena* arena, MemoryArenaScope scope);

void
memory_arena_clear(MemoryArena* arena);

void*
memory_arena_push(MemoryArena* arena, uintptr_t size, uintptr_t alignment);


# define memory_arena_alloc(ARENA, TYPE)	(TYPE*)memory_arena_push(ARENA, sizeof(TYPE), _Alignof(TYPE))
# define memory_arena_alloc_array(ARENA, TYPE, COUNT) (TYPE*)memory_arena_push(ARENA, sizeof(TYPE) * COUNT, _Alignof(TYPE))

// TODO(Alan): to implement later, after adding MemoryArenaScope (temp mem arena)
// # define memory_arena_scope_start(arena) memory_arena_get(arena)
// # define memory_arena_scope_end(arena, pos) memory_arena_set(arena, pos)

#endif
