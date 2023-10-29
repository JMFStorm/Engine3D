#include "j_buffers.h"

void memory_buffer_mallocate(MemoryBuffer* buffer, s64 size_in_bytes, char* name)
{
	ASSERT_TRUE(buffer->size == 0, "Arena is not already allocated");
	buffer->memory = (byte*)malloc(size_in_bytes);
	memset(buffer->memory, 0x00, size_in_bytes);
	buffer->size = size_in_bytes;
	strcpy_s(buffer->name, name);
	printf("memory_buffer_mallocate(): %.3f KB allocated for: '%s'.\n", (float)size_in_bytes / (float)1024, name);
}

void memory_buffer_wipe(MemoryBuffer* buffer)
{
	memset(buffer->memory, 0x00, buffer->size);
	printf("Buffer '%s' wiped to zero.\n", buffer->name);
}

void memory_buffer_free(MemoryBuffer* buffer)
{
	free(buffer->memory);
	buffer->size = 0;
	buffer->memory = nullptr;
	printf("Buffer '%s' freed.\n", buffer->name);
}

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

JString j_strings_add(JStringArray* strings, char* char_ptr)
{
	s64 str_len = strlen(char_ptr) + 1;
	ASSERT_TRUE(str_len <= strings->max_chars, "Strings has capacity left");

	s64 index = strings->current_chars;
	char* str_ptr = &strings->data[index];
	memcpy(str_ptr, char_ptr, str_len);
	strings->current_chars += str_len;

	JString j_string = {
		.str_len = str_len - 1,
		.str_ptr = str_ptr,
	};
	return j_string;
}