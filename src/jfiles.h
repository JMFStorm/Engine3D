#pragma once

#include "j_buffers.h"
#include "structs.h"

void read_file_to_memory(const char* file_path, MemoryBuffer* buffer);

void flip_vertical_image_load(bool flip);

ImageData load_image_data(char* image_path);

void free_loaded_image(ImageData data);

s16* load_ogg_file(char* filename, int* get_channels, int* get_sample_rate, int* num_of_samples);
