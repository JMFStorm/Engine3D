#pragma once

#include "utils.h"

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
s64 j_array_add(JArray<T>* array, T* element_ptr)
{
	// Always reserve one slot for swaps
	ASSERT_TRUE(array->items_count + 1 < array->max_items, "Array has item capacity left\n");
	void* array_ptr = &array->data[array->items_count * array->item_size_bytes];
	memcpy(array_ptr, element_ptr, array->item_size_bytes);
	array->items_count++;
	return array->items_count - 1;
}

template <typename T>
T* j_array_get(JArray<T>* array, s64 index)
{
	s64 used_index = index % array->max_items;
	T* item = &array->data[used_index * array->item_size_bytes];
	return item;
}

template <typename T>
void j_array_pop_back(JArray<T>* array)
{
	array->items_count--;
	if (array->items_count < 0) array->items_count = 0;
}

template <typename T>
void j_array_swap(JArray<T>* array, s64 index_1, s64 index_2)
{
	s64 unused_index = array->items_count + 1;
	void* temp_ptr = &array->data[unused_index];
	memcpy(temp_ptr, &array->data[index_1], array->item_size_bytes);
	memcpy(&array->data[index_1], &array->data[index_2], array->item_size_bytes);
	memcpy(&array->data[index_2], temp_ptr, array->item_size_bytes);
}

template <typename T>
void j_array_replace(JArray<T>* array, T* item_ptr, u64 index)
{
	ASSERT_TRUE(index <= array->items_count - 1, "Array item index is included");
	void* dest = &array->data[index * array->item_size_bytes];
	memcpy(dest, item_ptr, array->item_size_bytes);
}

typedef struct JStrings {
	s64 byte_alignment;
	s64 current_chars;
	s64 strings_count;
	s64 max_chars;
	char* data;
} JStrings;

JStrings j_strings_init(s64 max_chars, char* char_ptr);
s64 j_strings_add(JStrings* strings, char* char_ptr);
