#include "j_files.h"

#include <fstream>
#include "j_assert.h"
#include "utils.h"

void* stb_malloc_impl(int size)
{
	MemoryBuffer buffer = memory_buffer_suballocate(&g_temp_memory, size);
	return buffer.memory;
}

void* stb_realloc_impl(void* ptr, int size)
{
	if (ptr == 0)
	{
		MemoryBuffer buffer = memory_buffer_suballocate(&g_temp_memory, size);
		return buffer.memory;
	}

	MemoryBuffer buffer = memory_buffer_suballocate(&g_temp_memory, size);
	return buffer.memory;
}

void stb_free_impl(void* ptr)
{
}

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_MALLOC(sz)           stb_malloc_impl(sz)
#define STBI_REALLOC(p,newsz)     stb_realloc_impl(p,newsz)
#define STBI_FREE(p)              stb_free_impl(p)
#include "stb_image.h"

void read_file_to_memory(const char* file_path, MemoryBuffer* buffer)
{
	std::ifstream file_stream(file_path, std::ios::binary | std::ios::ate);
	ASSERT_TRUE(file_stream.is_open(), "Open file");

	std::streampos file_size = file_stream.tellg();
	ASSERT_TRUE(file_size <= buffer->size, "File size fits buffer");

	file_stream.seekg(0, std::ios::beg);

	char* read_pointer = (char*)buffer->memory;
	file_stream.read(read_pointer, file_size);
	file_stream.close();

	null_terminate_string(read_pointer, file_size);
}

void flip_vertical_image_load(bool flip)
{
	stbi_set_flip_vertically_on_load(flip);
}

ImageData load_image_data(char* image_path)
{
	ImageData data = {};
	data.image_data = stbi_load(image_path, &data.width_px, &data.height_px, &data.channels, 0);
	ASSERT_TRUE(data.image_data != NULL, "STB load image");
	return data;
}

void free_loaded_image(ImageData data)
{
	stbi_image_free(data.image_data);
	memory_buffer_wipe(&g_temp_memory);
}