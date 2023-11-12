#pragma once

#include "globals.h"
#include "types.h"
#include "utils.h"
#include "j_assert.h"

struct MemoryBuffer {
	char name[32];
	s64 size;
	byte* memory;
};

void memory_buffer_mallocate(MemoryBuffer* buffer, s64 size_in_bytes, char* name);
void memory_buffer_wipe(MemoryBuffer* buffer);
void memory_buffer_free(MemoryBuffer* buffer);

