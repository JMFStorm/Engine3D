#pragma once

#include "structs.h"
#include "types.h"

void load_font(FontData* font_data, int font_height_px, const char* font_path);

void create_font_atlas_texture(FontData* font_data, s32 bitmap_width, s32 bitmap_height, byte* bitmap_memory);
