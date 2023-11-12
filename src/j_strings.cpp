#include "j_strings.h"

#include "j_assert.h"

JStringArray j_strings_init(s64 max_chars, char* char_ptr)
{
	JStringArray strings = {
		.byte_alignment = 0,
		.current_chars = 0,
		.max_chars = max_chars,
		.data = char_ptr,
	};
	return strings;
}

char* j_strings_add(JStringArray* strings, char* char_ptr)
{
	s64 str_len = strlen(char_ptr) + 1;
	ASSERT_TRUE(str_len <= strings->max_chars, "Strings has capacity left");

	s64 index = strings->current_chars;
	char* str_ptr = &strings->data[index];
	memcpy(str_ptr, char_ptr, str_len);
	strings->current_chars += str_len;

	return str_ptr;
}