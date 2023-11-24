#include "j_map.h"
#include "j_assert.h"

Map64 hash_map_init(s64 max_size, s64 sizeof_element, byte* data_ptr)
{
	Map64 hm = {
		.capacity = max_size,
		.item_size = sizeof_element,
		.items_count = 0,
		.data = data_ptr
	};
	return hm;
}

void hash_map_add(Map64* map_ptr, Map64Item item)
{
	ASSERT_TRUE(map_ptr->items_count + 1 <= map_ptr->capacity, "Map has space");

	s64 index = (item.key % map_ptr->capacity);
	s64 as_offset = index * map_ptr->item_size;
	Map64Item* new_location = (Map64Item*)&map_ptr->data[as_offset];

	for (;;)
	{
		if (new_location->key == 0)
		{
			memcpy_s(new_location, map_ptr->item_size, &item, map_ptr->item_size);
			map_ptr->items_count++;
			break;
		}

		as_offset = as_offset + map_ptr->item_size;
	}
}

byte* hash_map_get(Map64* map_ptr, s64 key)
{
	s64 index = (key % map_ptr->capacity);
	s64 as_offset = index * map_ptr->item_size;
	Map64Item* item = (Map64Item*)&map_ptr->data[as_offset];

	for (;;)
	{
		if (item->key == key)
		{
			return item->value;
		}

		as_offset = as_offset + map_ptr->item_size;
	}

	return nullptr;
}