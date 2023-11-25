#pragma once

#include "types.h"

typedef struct MemoryBuffer {
	char name[32];
	s64 size;
	s64 used_sub_allocation_capacity;
	byte* memory;
} MemoryBuffer;

void memory_buffer_mallocate(MemoryBuffer* buffer, s64 size_in_bytes, char* name);
MemoryBuffer memory_buffer_suballocate(MemoryBuffer* buffer, s64 size_in_bytes);
void memory_buffer_wipe(MemoryBuffer* buffer);
void memory_buffer_free(MemoryBuffer* buffer);
