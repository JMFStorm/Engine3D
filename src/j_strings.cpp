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
	strings->strings_count++;

	return str_ptr;
}

bool str_trim_from_char(char* str, char c)
{
	char* last_dot = strchr(str, c);
	if (last_dot == nullptr) return false;
	*last_dot = '\0';
	return true;
}

char* str_get_file_ext(char* str)
{
	char* last_dot = strrchr(str, '.');
	if (last_dot == nullptr) return nullptr;
	return last_dot;
}
