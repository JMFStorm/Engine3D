#include "utils.h"

#include <iostream>

void assert_true(bool assertion, const char* assertion_title, const char* file, const char* func, int line)
{
	if (!assertion)
	{
		std::cerr << "ERROR: Assertion (" << assertion_title << ") failed in: " << file << " at function: " << func << "(), line: " << line << "." << std::endl;
		exit(1);
	}
}

float normalize_screen_px_to_ndc(int value, int max)
{
	float this1 = static_cast<float>(value) / static_cast<float>(max);
	float this2 = 2.0f * this1;
	float res = -1.0f + this2;
	return res;
}

float normalize_value(float value, float src_max, float dest_max)
{
	// Assume 0.0 is min value
	float intermediate = value / src_max;
	float result = dest_max * intermediate;
	return result;
}

void memory_arena_init(MemoryArena* arena, unsigned long size_in_bytes)
{
	ASSERT_TRUE(arena->max_size == 0, "Arena is not already allocated");
	ASSERT_TRUE(arena->used		== 0, "Arena is not already allocated");

	arena->memory = (byte*)malloc(size_in_bytes);
	memset(arena->memory, 0x00, size_in_bytes);
	arena->max_size = size_in_bytes;
	arena->used = 0;
}

MemoryArena memory_arena_create_subsection(MemoryArena* arena, unsigned long size_in_bytes)
{
	unsigned long space_left = arena->free_space();
	ASSERT_TRUE(size_in_bytes <= space_left, "Arena has space left");

	MemoryArena subsection = { 0 };
	subsection.max_size = size_in_bytes; 
	subsection.used = 0;

	unsigned long arena_pointer_index = arena->used;
	subsection.memory = &arena->memory[arena_pointer_index];
	arena->used += size_in_bytes;
	return subsection;
}

void memory_arena_clear(MemoryArena* arena)
{
	memset(arena->memory, 0x00, arena->max_size);
	arena->used = 0;
}

void memory_arena_free(MemoryArena* arena)
{
	free(arena->memory);
	arena->max_size = 0;
	arena->used = 0;
	arena->memory = nullptr;
}