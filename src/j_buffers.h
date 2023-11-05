#pragma once

#include "types.h"
#include "utils.h"
#include "j_assert.h"

typedef struct MemoryBuffer {
	char name[32];
	s64 size;
	byte* memory;
} MemoryBuffer;

void memory_buffer_mallocate(MemoryBuffer* buffer, s64 size_in_bytes, char* name);
void memory_buffer_wipe(MemoryBuffer* buffer);
void memory_buffer_free(MemoryBuffer* buffer);

template <typename T>
struct JArray {
	s64 item_size_bytes;
	s64 max_items;
	s64 items_count;
	T* data;
};

template <typename T>
JArray<T> j_array_init(s64 max_items, s64 item_size_bytes, T* data_ptr)
{
	ASSERT_TRUE(1 < max_items, "Array has more than one slot allocated\n");
	JArray array = {
		.item_size_bytes = item_size_bytes,
		.max_items = max_items,
		.items_count = 0,
		.data = (T*)data_ptr
	};
	return array;
}

template <typename T>
T* j_array_add(JArray<T>* array, T element)
{
	// Always reserve one slot for swaps
	ASSERT_TRUE(array->items_count + 1 < array->max_items, "Array has item capacity left\n");
	T* array_ptr = &array->data[array->items_count];
	memcpy(array_ptr, &element, array->item_size_bytes);
	array->items_count++;
	return array_ptr;
}

template <typename T>
T* j_array_get(JArray<T>* array, s64 index)
{
	s64 used_index = index % array->max_items;
	T* item = &array->data[used_index];
	return item;
}

template <typename T>
void j_array_pop_back(JArray<T>* array)
{
	array->items_count--;
	if (array->items_count < 0) array->items_count = 0;
}

template <typename T>
void j_array_replace(JArray<T>* array, T new_element, u64 index)
{
	ASSERT_TRUE(index <= array->items_count - 1, "Array item index is included");
	T* dest = &array->data[index];
	memcpy(dest, &new_element, array->item_size_bytes);
}

template <typename T>
void j_array_unordered_delete(JArray<T>* jarray, u64 index)
{
	T* last_element = j_array_get(jarray, jarray->items_count - 1);
	j_array_replace(jarray, *last_element, index);
	j_array_pop_back(jarray);
}

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
