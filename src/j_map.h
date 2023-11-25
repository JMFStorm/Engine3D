#pragma once

#include "types.h"

typedef struct JMapItem_S64_Str {
	s64 key;
	char* value;
} JMapItem_S64_Str;

typedef struct JMapItem_Str_S64 {
	char* key;
	s64 value;
} JMapItem_Str_S64;

typedef struct JMapItem_S64_Ptr {
	s64 key;
	byte* value;
} JMapItem_Str_Ptr;

typedef struct JMap {
	s64 capacity;
	s64 item_size;
	s64 items_count;
	byte* data;
} JMap;

JMap jmap_init(s64 max_size, s64 sizeof_element, byte* data_ptr);

void jmap_add(JMap* map_ptr, JMapItem_S64_Str item);
void jmap_add(JMap* map_ptr, JMapItem_Str_S64 item);
void jmap_add(JMap* map_ptr, JMapItem_S64_Ptr item);

char* jmap_get_k_s64(JMap* map_ptr, s64 key);

s64 jmap_get_k_str(JMap* map_ptr, char* key);
