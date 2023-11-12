#pragma once

#include "types.h"

typedef struct JStringArray {
	s64 byte_alignment;
	s64 current_chars;
	s64 strings_count;
	s64 max_chars;
	char* data;
} JStringArray;

JStringArray j_strings_init(s64 max_chars, char* char_ptr);

char* j_strings_add(JStringArray* strings, char* char_ptr);
