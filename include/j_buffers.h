#pragma once

#define KILOBYTES(x) (x * 1024)
#define MEGABYTES(x) (KILOBYTES(x) * 1024)

typedef unsigned char		byte;
typedef unsigned long long  u64;
typedef long long			s64;

typedef struct MemoryBuffer {
	char name[32];
	s64 size;
	byte* memory;
} MemoryBuffer;

void memory_buffer_mallocate(MemoryBuffer* buffer, s64 size_in_bytes, char* name);
void memory_buffer_wipe(MemoryBuffer* buffer);
void memory_buffer_free(MemoryBuffer* buffer);

typedef struct JArray {
	s64 item_size_bytes;
	s64 max_items;
	s64 items_count;
	byte* data;
} JArray;

JArray j_array_init(s64 max_items, s64 item_size, byte* data_ptr);
s64    j_array_add(JArray* array, byte* element_ptr);
void   j_array_pop_back(JArray* array);
void   j_array_swap(JArray* array, s64 index_1, s64 index_2);
void   j_array_replace(JArray* array, byte* item_ptr, u64 index);
byte*  j_array_get(JArray* array, s64 index);
