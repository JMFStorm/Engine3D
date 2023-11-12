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

typedef struct JString {
	s64 str_len;
	char* str_ptr;
} JString;

typedef struct JStringArray {
	s64 byte_alignment;
	s64 current_chars;
	s64 strings_count;
	s64 max_chars;
	char* data;
} JStringArray;

JStringArray j_strings_init(s64 max_chars, char* char_ptr);
char* j_strings_add(JStringArray* strings, char* char_ptr);
