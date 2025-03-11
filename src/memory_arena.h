
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

typedef struct MemoryArenaBlockFooter
{
	struct MemoryArenaBlockFooter* next;
	uintptr_t capacity;
	uintptr_t top;
	//NOTE(Alan): The raw data is right behind this struct in one allocated space
	//	[[FOOTER]-PADDING-[RAW_DATA]]
}
MemoryArenaBlockFooter;

typedef struct
{
	MemoryArenaBlockFooter* head_block;
	uintptr_t minimum_block_capacity;
	uintptr_t scope_count;
}
MemoryArena;

typedef struct
{
	MemoryArena* arena;
	MemoryArenaBlockFooter* block;
	uintptr_t top;
	uintptr_t id;
}
MemoryArenaScope;


void
memory_arena_init(MemoryArena* arena, uintptr_t minimum_block_capacity);

void
memory_arena_destroy(MemoryArena* arena);

MemoryArenaScope
memory_arena_scope_start(MemoryArena* arena);

void
memory_arena_scope_end(MemoryArenaScope scope);

void
memory_arena_clear(MemoryArena* arena);

void*
memory_arena_push(MemoryArena* arena, uintptr_t size, uintptr_t alignment);

# define memory_arena_alloc(ARENA, TYPE) (TYPE*)memory_arena_push(ARENA, sizeof(TYPE), _Alignof(TYPE))
# define memory_arena_alloc_array(ARENA, TYPE, COUNT) (TYPE*)memory_arena_push(ARENA, sizeof(TYPE) * COUNT, _Alignof(TYPE))

#endif
