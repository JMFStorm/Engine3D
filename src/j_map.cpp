#include "j_map.h"
#include "j_assert.h"

JMap jmap_init(s64 max_size, s64 sizeof_element, byte* data_ptr)
{
	JMap hm = {
		.capacity = max_size,
		.item_size = sizeof_element,
		.items_count = 0,
		.data = data_ptr
	};
	return hm;
}

u64 hash_s64(JMap* map_ptr, s64 key)
{
	u64 index = (key % map_ptr->capacity);
	u64 as_offset = index * map_ptr->item_size;
	return as_offset;
}

u64 hash_str(JMap* map_ptr, char* key)
{
	u64 hash = 5381;
	int c;

	while ((c = *key++))
	{
		hash = ((hash << 5) + hash) + c; // DJB2 hash
	}

	u64 index = hash % map_ptr->capacity;
	u64 as_offset = index * map_ptr->item_size;
	return as_offset;
}

void jmap_add(JMap* map_ptr, JMapItem_S64_Ptr item)
{
	ASSERT_TRUE(map_ptr->items_count + 1 <= map_ptr->capacity, "Map has space");
	u64 as_offset = hash_s64(map_ptr, item.key);

	for (;;)
	{
		JMapItem_S64_Str* new_location = (JMapItem_S64_Str*)&map_ptr->data[as_offset];
		if (new_location->key == 0)
		{
			memcpy_s(new_location, map_ptr->item_size, &item, map_ptr->item_size);
			map_ptr->items_count++;
			break;
		}

		as_offset = as_offset + map_ptr->item_size;
	}
}

void jmap_add(JMap* map_ptr, JMapItem_S64_Str item)
{
	JMapItem_S64_Ptr as_cast = {
		.key = item.key,
		.value = (byte*)item.value
	};
	return jmap_add(map_ptr, as_cast);
}

void jmap_add(JMap* map_ptr, JMapItem_Str_S64 item)
{
	ASSERT_TRUE(map_ptr->items_count + 1 <= map_ptr->capacity, "Map has space");
	u64 as_offset = hash_str(map_ptr, item.key);

	for (;;)
	{
		JMapItem_Str_S64* new_location = (JMapItem_Str_S64*)&map_ptr->data[as_offset];
		bool match = new_location->key == 0;
		if (match)
		{
			memcpy_s(new_location, map_ptr->item_size, &item, map_ptr->item_size);
			map_ptr->items_count++;
			break;
		}

		as_offset = as_offset + map_ptr->item_size;
	}
}

char* jmap_get_k_s64(JMap* map_ptr, s64 key)
{
	u64 as_offset = hash_s64(map_ptr, key);

	for (;;)
	{
		JMapItem_S64_Str* item = (JMapItem_S64_Str*)&map_ptr->data[as_offset];
		bool match = item->key == key;
		if (match) return item->value;
		as_offset = as_offset + map_ptr->item_size;
	}

	return nullptr;
}

s64 jmap_get_k_str(JMap* map_ptr, char* key)
{
	u64 as_offset = hash_str(map_ptr, key);

	for (;;)
	{
		JMapItem_Str_S64* item = (JMapItem_Str_S64*)&map_ptr->data[as_offset];
		bool match = strcmp(item->key, key) == 0;
		if (match) return item->value;
		as_offset = as_offset + map_ptr->item_size;
	}

	return 0;
}