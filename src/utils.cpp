#include "utils.h"

#include <iostream>

void assert_true(bool assertion, const char* assertion_title, const char* file, const char* func, int line)
{
	if (!assertion)
	{
		std::cerr 
			<< "ERROR: Assertion (" << assertion_title 
			<< ") failed in: " << file 
			<< " at function: " << func 
			<< "(), line: " << line 
			<< "." << std::endl;

		exit(1);
	}
}

float normalize_screen_px_to_ndc(int value, int max)
{
	float this1 = static_cast<float>(value) / static_cast<float>(max);
	float this2 = 2.0f * this1;
	float res = -1.0f + this2;
	return res;
}

float normalize_value(float value, float src_max, float dest_max)
{
	// Assume 0.0 is min value
	float intermediate = value / src_max;
	float result = dest_max * intermediate;
	return result;
}

void memory_buffer_mallocate(MemoryBuffer* buffer, unsigned long size_in_bytes, char* name)
{
	ASSERT_TRUE(buffer->size == 0, "Arena is not already allocated");

	buffer->memory = (byte*)malloc(size_in_bytes);
	memset(buffer->memory, 0x00, size_in_bytes);
	buffer->size = size_in_bytes;
	strcpy_s(buffer->name, name);

	std::cout 
		<< "Buffer '" << name << "' mallocated with " 
		<< (size_in_bytes / 1024) << " kilobytes." << std::endl;
}

void memory_buffer_wipe(MemoryBuffer* buffer)
{
	memset(buffer->memory, 0x00, buffer->size);
	std::cout << "Buffer " << buffer->name << " wiped to zero." << std::endl;
}

void memory_buffer_free(MemoryBuffer* buffer)
{
	free(buffer->memory);
	buffer->size = 0;
	buffer->memory = nullptr;

	std::cout << "Buffer " << buffer->name << " freed." << std::endl;
}