#pragma once

#include "types.h"

struct JArray {
	s64 item_size_bytes;
	s64 max_items;
	s64 items_count;
	byte* data;
};

JArray j_array_init(s64 max_items, s64 item_size_bytes, byte* data_ptr);

byte* j_array_add(JArray* array, byte* element_ptr);

byte* j_array_get(JArray* array, s64 index);

void j_array_pop_back(JArray* array);

void j_array_replace(JArray* array, byte* new_element_ptr, u64 index);

void j_array_unordered_delete(JArray* jarray, u64 index);

void j_array_empty(JArray* jarray);
