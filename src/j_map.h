#pragma once

#include "types.h"

typedef struct Map64Item {
	s64 key;
	byte* value;
} Map64Item;

typedef struct Map64 {
	s64 capacity;
	s64 item_size;
	s64 items_count;
	byte* data;
} Map64;

Map64 hash_map_init(s64 max_size, s64 sizeof_element, byte* data_ptr);
void hash_map_add(Map64* map_ptr, Map64Item item);
byte* hash_map_get(Map64* map_ptr, s64 key);
