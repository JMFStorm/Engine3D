#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include "structs.h"

void load_font(FontData* font_data, int font_height_px, char* font_path);

void load_font2(FontData* font_data, int font_height_px, const char* font_path);
