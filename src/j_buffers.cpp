#include "j_buffers.h"

#include "j_assert.h"

void memory_buffer_mallocate(MemoryBuffer* buffer, s64 size_in_bytes, char* name)
{
	ASSERT_TRUE(buffer->size == 0, "Arena is not already allocated");
	buffer->memory = (byte*)malloc(size_in_bytes);
	memset(buffer->memory, 0x00, size_in_bytes);
	buffer->size = size_in_bytes;
	buffer->used_sub_allocation_capacity = 0;
	strcpy_s(buffer->name, name);
	printf("memory_buffer_mallocate(): %.3f KB allocated for buffer: %s.\n", (float)size_in_bytes / (float)1024, name);
}

MemoryBuffer memory_buffer_suballocate(MemoryBuffer* buffer, s64 size_in_bytes)
{
	ASSERT_TRUE(
		buffer->size + size_in_bytes <= buffer->size - buffer->used_sub_allocation_capacity,
		"Arena has size of suballocation");

	s64 memory_index = sizeof(byte) * buffer->used_sub_allocation_capacity;
	byte* sub_alloc_mem_start = &buffer->memory[memory_index];
	buffer->used_sub_allocation_capacity += size_in_bytes;

	MemoryBuffer sub_allocation = {
		.name = "",
		.size = size_in_bytes,
		.used_sub_allocation_capacity = 0,
		.memory = sub_alloc_mem_start
	};
	return sub_allocation;
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
