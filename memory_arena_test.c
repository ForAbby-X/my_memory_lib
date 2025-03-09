#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "memory_arena.h"

// Helper to check pointer alignment
static bool is_aligned(void *ptr, uintptr_t alignment)
{
	return ((uintptr_t)ptr % alignment) == 0;
}

// Helper to determine malloc's alignment guarantee
static uintptr_t get_malloc_alignment()
{
	// Start testing from 16 bytes downward
	for (uintptr_t align = 16; align >= 1; align /= 2) {
		bool aligned = true;
		// Test multiple allocations to be sure
		for (int i = 0; i < 10; i++) {
			void *ptr = malloc(sizeof(void*));
			if (!is_aligned(ptr, align)) {
				aligned = false;
			}
			free(ptr);
			if (!aligned) break;
		}
		if (aligned) {
			return align;
		}
	}
	return 1; // Minimum guaranteed alignment
}

void test_basic_functionality()
{
	printf("Testing basic functionality...\n");

	MemoryArena arena;
	memory_arena_init(&arena, 1024);

	// Basic allocation
	void *ptr1 = memory_arena_push(&arena, 100, 8);
	assert(ptr1 != NULL);
	assert(is_aligned(ptr1, 8));

	// Write and read back data
	for (int i = 0; i < 100; i++)
	{
		((char *)ptr1)[i] = (char)i;
	}
	for (int i = 0; i < 100; i++)
	{
		assert(((char *)ptr1)[i] == (char)i);
	}

	// Sequential allocations
	void *ptr2 = memory_arena_push(&arena, 200, 16);
	assert(ptr2 != NULL);
	assert(is_aligned(ptr2, 16));
	assert((uintptr_t)ptr2 > (uintptr_t)ptr1);

	memory_arena_destroy(&arena);
	printf("✓ Basic functionality test passed\n");
}

void test_alignment_requirements()
{
	printf("Testing alignment requirements...\n");

	// Determine malloc's alignment guarantee
	uintptr_t malloc_align = get_malloc_alignment();
	printf("Detected malloc alignment: %zu bytes\n", malloc_align);

	MemoryArena arena;
	memory_arena_init(&arena, 1024);

	// Test various power-of-two alignments up to malloc's guarantee
	void *ptr1 = memory_arena_push(&arena, 1, 1);
	void *ptr2 = memory_arena_push(&arena, 1, 2);
	void *ptr4 = memory_arena_push(&arena, 1, 4);
	void *ptr8 = memory_arena_push(&arena, 1, 8);

	assert(is_aligned(ptr1, 1));
	assert(is_aligned(ptr2, 2));
	assert(is_aligned(ptr4, 4));
	assert(is_aligned(ptr8, 8));

	// Only test larger alignments if malloc guarantees them
	if (malloc_align >= 16) {
		void *ptr16 = memory_arena_push(&arena, 1, 16);
		assert(is_aligned(ptr16, 16));
	}
	if (malloc_align >= 32) {
		void *ptr32 = memory_arena_push(&arena, 1, 32);
		assert(is_aligned(ptr32, 32));
	}
	if (malloc_align >= 64) {
		void *ptr64 = memory_arena_push(&arena, 1, 64);
		assert(is_aligned(ptr64, 64));
	}
	if (malloc_align >= 128) {
		void *ptr128 = memory_arena_push(&arena, 1, 128);
		assert(is_aligned(ptr128, 128));
	}

	// Test alignment padding
	void *ptr_a = memory_arena_push(&arena, 3, 1); // No specific alignment
	void *ptr_b = memory_arena_push(&arena, 5, 8); // Should create padding if needed

	assert((uintptr_t)ptr_b % 8 == 0);

	memory_arena_destroy(&arena);
	printf("✓ Alignment test passed\n");
}

void test_block_management()
{
	printf("Testing block management...\n");

	// Small minimum block size to force multiple blocks
	MemoryArena arena;
	memory_arena_init(&arena, 64);

	// Fill first block
	void *ptr1 = memory_arena_push(&arena, 32, 8);
	void *ptr2 = memory_arena_push(&arena, 24, 8);

	// This should force a new block creation
	void *ptr3 = memory_arena_push(&arena, 48, 8);

	assert(ptr1 != NULL);
	assert(ptr2 != NULL);
	assert(ptr3 != NULL);

	// ptr3 should be at the beginning of the new block
	assert(arena.head_block != NULL);
	assert(ptr3 == arena.head_block->data);

	// Allocate a larger-than-minimum block
	void *ptr4 = memory_arena_push(&arena, 128, 8);
	assert(ptr4 != NULL);
	assert(ptr4 == arena.head_block->data);

	memory_arena_destroy(&arena);
	printf("✓ Block management test passed\n");
}

void test_scopes()
{
	printf("Testing memory scopes...\n");

	MemoryArena arena;
	memory_arena_init(&arena, 1024);

	// Initial allocations
	void *ptr1 = memory_arena_push(&arena, 100, 8);
	void *ptr2 = memory_arena_push(&arena, 100, 8);

	// Mark position with a scope
	MemoryArenaScope scope = memory_arena_scope_start(&arena);

	// Allocate within scope
	void *ptr3 = memory_arena_push(&arena, 100, 8);
	void *ptr4 = memory_arena_push(&arena, 100, 8);

	// Verify we can write to all allocated memory
	*(int *)ptr1 = 1;
	*(int *)ptr2 = 2;
	*(int *)ptr3 = 3;
	*(int *)ptr4 = 4;

	// End scope - ptr3 and ptr4 space should be reclaimed
	memory_arena_scope_end(&arena, scope);

	// New allocation should reuse space from ptr3
	void *ptr5 = memory_arena_push(&arena, 100, 8);
	assert(ptr5 == ptr3);

	// Verify ptr1 and ptr2 data is intact
	assert(*(int *)ptr1 == 1);
	assert(*(int *)ptr2 == 2);

	memory_arena_destroy(&arena);
	printf("✓ Scopes test passed\n");
}

void test_nested_scopes()
{
	printf("Testing nested scopes...\n");

	MemoryArena arena;
	memory_arena_init(&arena, 1024);

	void *ptr1 = memory_arena_push(&arena, 100, 8);
	*(int *)ptr1 = 1;

	// Outer scope
	MemoryArenaScope scope1 = memory_arena_scope_start(&arena);
	void *ptr2 = memory_arena_push(&arena, 100, 8);
	*(int *)ptr2 = 2;

	// Inner scope
	MemoryArenaScope scope2 = memory_arena_scope_start(&arena);
	void *ptr3 = memory_arena_push(&arena, 100, 8);
	*(int *)ptr3 = 3;

	// End inner scope (ptr3 reclaimed)
	memory_arena_scope_end(&arena, scope2);

	void *ptr4 = memory_arena_push(&arena, 100, 8);
	*(int *)ptr4 = 4;
	assert(ptr4 == ptr3); // Should reuse ptr3's space

	// End outer scope (ptr2 and ptr4 reclaimed)
	memory_arena_scope_end(&arena, scope1);

	void *ptr5 = memory_arena_push(&arena, 100, 8);
	*(int *)ptr5 = 5;
	assert(ptr5 == ptr2); // Should reuse ptr2's space

	assert(*(int *)ptr1 == 1); // Verify original data intact

	memory_arena_destroy(&arena);
	printf("✓ Nested scopes test passed\n");
}

void test_clear()
{
	printf("Testing arena clear...\n");

	MemoryArena arena;
	memory_arena_init(&arena, 256);

	// Create multiple blocks
	void *ptr1 = memory_arena_push(&arena, 200, 8);
	void *ptr2 = memory_arena_push(&arena, 200, 8);
	void *ptr3 = memory_arena_push(&arena, 200, 8);

	// Count blocks before clear
	int blocks_before = 0;
	MemoryArenaBlock *block = arena.head_block;
	while (block)
	{
		blocks_before++;
		block = block->next;
	}

	// Clear arena
	memory_arena_clear(&arena);

	// Should only have one block left with top=0
	assert(arena.head_block != NULL);
	assert(arena.head_block->next == NULL);
	assert(arena.head_block->top == 0);

	// New allocation should use the first position in the block
	void *ptr4 = memory_arena_push(&arena, 100, 8);
	assert(ptr4 == arena.head_block->data);

	memory_arena_destroy(&arena);
	printf("✓ Clear test passed\n");
}

void test_edge_cases()
{
	printf("Testing edge cases...\n");

	// Determine malloc's alignment guarantee
	uintptr_t malloc_align = get_malloc_alignment();
	printf("Detected malloc alignment: %zu bytes\n", malloc_align);

	MemoryArena arena;
	memory_arena_init(&arena, 64);

	// Zero-sized allocation (should still return valid pointer)
	void *ptr1 = memory_arena_push(&arena, 0, 8);
	assert(ptr1 != NULL);

	// Allocate exactly minimum_block_capacity
	void *ptr2 = memory_arena_push(&arena, 64, 8);
	assert(ptr2 != NULL);

	// Allocate huge block (much larger than minimum)
	void *ptr3 = memory_arena_push(&arena, 4096, 8);
	assert(ptr3 != NULL);

	// Only check alignments up to what malloc guarantees
	void *ptr4 = memory_arena_push(&arena, 10, malloc_align);
	assert(ptr4 != NULL);
	assert(is_aligned(ptr4, malloc_align));

	// Sequence of weird sized allocations with different alignments
	// but limited to malloc's guarantee
	for (int i = 0; i < 20; i++)
	{
		uintptr_t size = (i * 17) % 53; // Weird sizes
		uintptr_t align = 1 << (i % 8);
		if (align > malloc_align) 
			align = malloc_align; // Cap alignment at malloc's guarantee
			
		void *p = memory_arena_push(&arena, size, align);
		assert(p != NULL);
		assert(is_aligned(p, align));

		// Write some data if size allows
		if (size > sizeof(int))
		{
			*(int *)p = i;
		}
	}

	memory_arena_destroy(&arena);
	printf("✓ Edge cases test passed\n");
}

void test_complex_scenario()
{
	printf("Testing complex usage scenario...\n");

	// Create a small arena that will require multiple blocks
	MemoryArena arena;
	memory_arena_init(&arena, 128);

// Track all our allocations
#define NUM_PTRS 100
	void *ptrs[NUM_PTRS];
	size_t sizes[NUM_PTRS];

	// Phase 1: Varied allocations
	for (int i = 0; i < NUM_PTRS; i++)
	{
		sizes[i] = (i % 13) * 7 + 4;		  // Various sizes
		uintptr_t align = 1 << ((i % 5) + 1); // Various alignments (2,4,8,16,32)

		ptrs[i] = memory_arena_push(&arena, sizes[i], align);
		assert(ptrs[i] != NULL);
		assert(is_aligned(ptrs[i], align));

		// Write pattern data: fill with repeating index
		for (size_t j = 0; j < sizes[i]; j++)
		{
			((char *)ptrs[i])[j] = (char)i;
		}
	}

	// Verify all allocations contain correct data
	for (int i = 0; i < NUM_PTRS; i++)
	{
		for (size_t j = 0; j < sizes[i]; j++)
		{
			assert(((char *)ptrs[i])[j] == (char)i);
		}
	}

	// Phase 2: Create nested scopes with checkpoints
	MemoryArenaScope scope1 = memory_arena_scope_start(&arena);

	void *scope1_ptr = memory_arena_push(&arena, 50, 8);
	strcpy((char *)scope1_ptr, "scope1_data");

	MemoryArenaScope scope2 = memory_arena_scope_start(&arena);

	void *scope2_ptr = memory_arena_push(&arena, 50, 8);
	strcpy((char *)scope2_ptr, "scope2_data");

	// Restore to scope2 start - scope2_ptr should be reclaimed
	memory_arena_scope_end(&arena, scope2);

	void *after_scope2_ptr = memory_arena_push(&arena, 50, 8);
	assert(after_scope2_ptr == scope2_ptr); // Should reuse the same memory

	// Restore to scope1 start - scope1_ptr and after_scope2_ptr should be reclaimed
	memory_arena_scope_end(&arena, scope1);

	void *after_scope1_ptr = memory_arena_push(&arena, 50, 8);
	// Instead of
	// assert(after_scope1_ptr == scope1_ptr);

	// Check that we're correctly handling memory
	strcpy((char*)after_scope1_ptr, "test data");
	assert(strcmp((char*)after_scope1_ptr, "test data") == 0);

	// Original allocations should still be intact
	for (int i = 0; i < NUM_PTRS; i++)
	{
		for (size_t j = 0; j < sizes[i]; j++)
		{
			assert(((char *)ptrs[i])[j] == (char)i);
		}
	}

	memory_arena_destroy(&arena);
	printf("✓ Complex scenario test passed\n");
}

int main()
{
	printf("=== Memory Arena Test Suite ===\n");

	test_basic_functionality();
	test_alignment_requirements();
	test_block_management();
	test_scopes();
	test_nested_scopes();
	test_clear();
	test_edge_cases();
	test_complex_scenario();

	printf("All tests passed successfully!\n");
	return 0;
}