#include "j_array.h"

#include "j_assert.h"

JArray j_array_init(s64 max_items, s64 item_size_bytes, byte* data_ptr)
{
	ASSERT_TRUE(1 < max_items, "Array has more than one slot allocated\n");

	JArray array = {
		.item_size_bytes = item_size_bytes,
		.max_items = max_items,
		.items_count = 0,
		.data = data_ptr
	};

	return array;
}

byte* j_array_add(JArray* array, byte* element_ptr)
{
	// Always reserve one slot for swaps
	ASSERT_TRUE(array->items_count + 1 < array->max_items, "Array has item capacity left\n");

	byte* array_ptr = &array->data[array->items_count * array->item_size_bytes];
	memcpy(array_ptr, element_ptr, array->item_size_bytes);
	array->items_count++;
	return array_ptr;
}

byte* j_array_get(JArray* array, s64 index)
{
	byte* item = &array->data[index * array->item_size_bytes];
	return item;
}

void j_array_pop_back(JArray* array)
{
	array->items_count--;
	if (array->items_count < 0) array->items_count = 0;
}

void j_array_replace(JArray* array, byte* new_element_ptr, u64 index)
{
	ASSERT_TRUE(index <= array->items_count - 1, "Array item index is included");
	byte* dest = &array->data[index * array->item_size_bytes];
	memcpy(dest, new_element_ptr, array->item_size_bytes);
}

void j_array_unordered_delete(JArray* jarray, u64 index)
{
	byte* last_element = j_array_get(jarray, jarray->items_count - 1);
	j_array_replace(jarray, last_element, index);
	j_array_pop_back(jarray);
}

void j_array_empty(JArray* jarray)
{
	jarray->items_count = 0;
}
