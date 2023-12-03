#include "jfont.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define ALLOW_UNALIGNED_TRUETYPE
#include "stb_truetype.h"
#include <GL/glew.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "j_assert.h"
#include "utils.h"

void load_font(FontData* font_data, int font_height_px, char* font_path)
{
	// Temp
	FT_Library ft_lib;
	FT_Face ft_face;

	FT_Error init_ft_err = FT_Init_FreeType(&ft_lib);
	ASSERT_TRUE(init_ft_err == 0, "Init FreeType Library");

	FT_Error new_face_err = FT_New_Face(ft_lib, font_path, 0, &ft_face);
	ASSERT_TRUE(new_face_err == 0, "Load FreeType font face");

	FT_Set_Pixel_Sizes(ft_face, 0, font_height_px);
	font_data->font_height_px = font_height_px;
	// ~Temp

	stbtt_fontinfo font;
	byte* ttf_buffer = TEMP_MEMORY.memory;

	FILE* f_file = fopen(font_path, "rb");
	assert(f_file);

	fseek(f_file, 0, SEEK_END);
    s64 file_size = ftell(f_file);
    rewind(f_file);
    assert(file_size <= TEMP_MEMORY.size);
	fread(ttf_buffer, 1, file_size, f_file);

	s32 f_offset = stbtt_GetFontOffsetForIndex(ttf_buffer, 0);
	stbtt_InitFont(&font, ttf_buffer, f_offset);

	f32 f_height = (f32)font_height_px;
	f32 f_scale  = stbtt_ScaleForPixelHeight(&font, f_height);

	fclose(f_file);

	s32 bitmap_width = 0;
	s32 bitmap_height = 0;

	constexpr s32 starting_char = 33; // ('!')
	constexpr s32 last_char = 128;

	const s32 spacebar_width = font_height_px / 4;
	bitmap_width += spacebar_width; // Add spacebar

	for (int c = starting_char; c < last_char; c++)
	{
		char as_char = (char)c;
		s32 glyph_width;
		s32 glyph_height;
		s32 glyph_index = stbtt_FindGlyphIndex(&font, as_char);
    	stbtt_GetGlyphBitmap(&font, 0, f_scale, glyph_index, &glyph_width, &glyph_height, 0, 0);

		if (bitmap_height < glyph_height) bitmap_height = glyph_height;
		bitmap_width += glyph_width;
	}

	s64 bitmap_size = bitmap_width * bitmap_height;
	assert(bitmap_size < TEMP_MEMORY.size);

	byte* bitmap_memory = TEMP_MEMORY.memory;

	// Add spacebar
	{
		for (int y = 0; y < bitmap_height; y++)
		{
			s32 src_index = ((bitmap_height - 1) * spacebar_width) - y * spacebar_width;
			s32 dest_index = y * bitmap_width;
			memset(&bitmap_memory[dest_index], 0x00, spacebar_width);
		}

		CharData c_data = { 0 };
		c_data.character = static_cast<char>(' ');
		c_data.width = spacebar_width;
		c_data.advance = spacebar_width;
		c_data.height = font_height_px;
		c_data.UV_x0 = 0.0f;
		c_data.UV_y0 = 1.0f;
		c_data.UV_x1 = normalize_value(spacebar_width, bitmap_width, 1.0f);
		c_data.UV_y1 = 1.0f;
		font_data->char_data[0] = c_data;
	}

	s32 bitmap_x_offset = spacebar_width;
	s32 char_data_index = 1; // spacebar already added

	for (int c = starting_char; c < last_char; c++)
	{
		// Temp
		FT_Error load_char_err = FT_Load_Char(ft_face, c, FT_LOAD_RENDER);
		ASSERT_TRUE(load_char_err == 0, "Load FreeType Glyph");

		unsigned int glyph_width1 = ft_face->glyph->bitmap.width;
		unsigned int glyph_height1 = ft_face->glyph->bitmap.rows;

		int x_offset1 = ft_face->glyph->bitmap_left;
		int y_offset1 = ft_face->glyph->bitmap_top - ft_face->glyph->bitmap.rows;
		int advance1 = (ft_face->glyph->advance.x >> 6);
		// ~Temp

		char as_char = static_cast<char>(c);
		s32 glyph_width, glyph_height, x_offset, y_offset;

		s32 glyph_index = stbtt_FindGlyphIndex(&font, as_char);
    	byte* c_bitmap = stbtt_GetGlyphBitmap(&font, 0, f_scale, glyph_index, &glyph_width, &glyph_height, &x_offset, &y_offset);

		for (int y = 0; y < glyph_height; y++)
		{
			s32 src_index = ((glyph_height - 1) * glyph_width) - y * glyph_width;
			s32 dest_index = y * bitmap_width + bitmap_x_offset;
			memcpy(&bitmap_memory[dest_index], &c_bitmap[src_index], glyph_width);
		}

		CharData new_char_data = { 0 };
		new_char_data.character = as_char;
		new_char_data.width = glyph_width;
		new_char_data.height = glyph_height;
		new_char_data.x_offset = x_offset;
		new_char_data.y_offset = y_offset;

		new_char_data.UV_x0 = normalize_value(bitmap_x_offset, bitmap_width, 1.0f);
		new_char_data.UV_y0 = normalize_value(0.0f, bitmap_height, 1.0f);

		f32 uv_x1 = bitmap_x_offset + glyph_width;
		new_char_data.UV_x1 = normalize_value(uv_x1, bitmap_width, 1.0f);
		new_char_data.UV_y1 = normalize_value(glyph_height, bitmap_height, 1.0f);

		font_data->char_data[char_data_index++] = new_char_data;
		bitmap_x_offset += glyph_width;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLuint prev_texture = font_data->texture_id;
	glDeleteTextures(1, &prev_texture);
	GLuint new_texture;
	glGenTextures(1, &new_texture);
	font_data->texture_id = static_cast<int>(new_texture);
	glBindTexture(GL_TEXTURE_2D, font_data->texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, bitmap_width, bitmap_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap_memory);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void load_font2(FontData* font_data, int font_height_px, const char* font_path)
{
	FT_Library ft_lib;
	FT_Face ft_face;

	FT_Error init_ft_err = FT_Init_FreeType(&ft_lib);
	ASSERT_TRUE(init_ft_err == 0, "Init FreeType Library");

	FT_Error new_face_err = FT_New_Face(ft_lib, font_path, 0, &ft_face);
	ASSERT_TRUE(new_face_err == 0, "Load FreeType font face");

	FT_Set_Pixel_Sizes(ft_face, 0, font_height_px);
	font_data->font_height_px = font_height_px;

	unsigned int bitmap_width = 0;
	unsigned int bitmap_height = 0;

	constexpr int starting_char = 33; // ('!')
	constexpr int last_char = 128;

	int spacebar_width = font_height_px / 4;

	// Add spacebar
	bitmap_width += spacebar_width;

	for (int c = starting_char; c < last_char; c++)
	{
		char character = static_cast<char>(c);

		FT_Error load_char_err = FT_Load_Char(ft_face, character, FT_LOAD_RENDER);
		ASSERT_TRUE(load_char_err == 0, "Load FreeType Glyph");

		unsigned int glyph_width = ft_face->glyph->bitmap.width;
		unsigned int glyph_height = ft_face->glyph->bitmap.rows;

		if (bitmap_height < glyph_height)
		{
			bitmap_height = glyph_height;
		}

		bitmap_width += glyph_width;
	}

	int bitmap_size = bitmap_width * bitmap_height;
	ASSERT_TRUE(bitmap_size < TEMP_MEMORY.size, "Font bitmap buffer fits");

	byte* bitmap_memory = TEMP_MEMORY.memory;

	// Add spacebar
	{
		for (int y = 0; y < bitmap_height; y++)
		{
			int src_index = ((bitmap_height - 1) * spacebar_width) - y * spacebar_width;
			int dest_index = y * bitmap_width;
			memset(&bitmap_memory[dest_index], 0x00, spacebar_width);
		}

		CharData new_char_data = { 0 };
		new_char_data.character = static_cast<char>(' ');
		new_char_data.width = spacebar_width;
		new_char_data.advance = spacebar_width;
		new_char_data.height = font_height_px;
		new_char_data.UV_x0 = 0.0f;
		new_char_data.UV_y0 = 1.0f;
		new_char_data.UV_x1 = normalize_value(spacebar_width, bitmap_width, 1.0f);
		new_char_data.UV_y1 = 1.0f;
		font_data->char_data[0] = new_char_data;
	}

	int bitmap_x_offset = spacebar_width;
	int char_data_index = 1;

	for (int c = starting_char; c < last_char; c++)
	{
		int character = c;

		FT_Error load_char_err = FT_Load_Char(ft_face, character, FT_LOAD_RENDER);
		ASSERT_TRUE(load_char_err == 0, "Load FreeType Glyph");

		unsigned int glyph_width = ft_face->glyph->bitmap.width;
		unsigned int glyph_height = ft_face->glyph->bitmap.rows;

		int x_offset = ft_face->glyph->bitmap_left;
		int y_offset = ft_face->glyph->bitmap_top - ft_face->glyph->bitmap.rows;

		for (int y = 0; y < glyph_height; y++)
		{
			int src_index = ((glyph_height - 1) * glyph_width) - y * glyph_width;
			int dest_index = y * bitmap_width + bitmap_x_offset;
			memcpy(&bitmap_memory[dest_index], &ft_face->glyph->bitmap.buffer[src_index], glyph_width);
		}

		// To advance cursors for next glyph
		// bitshift by 6 to get value in pixels
		// (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
		// (note that advance is number of 1/64 pixels)
		int advance = (ft_face->glyph->advance.x >> 6);

		CharData new_char_data = { 0 };
		new_char_data.character = static_cast<char>(c);

		new_char_data.width = glyph_width;
		new_char_data.height = glyph_height;
		new_char_data.x_offset = x_offset;
		new_char_data.y_offset = y_offset;
		new_char_data.advance = advance;

		float atlas_width = static_cast<float>(bitmap_width);
		float atlas_height = static_cast<float>(bitmap_height);

		new_char_data.UV_x0 = normalize_value(bitmap_x_offset, atlas_width, 1.0f);
		new_char_data.UV_y0 = normalize_value(0.0f, atlas_height, 1.0f);

		float x1 = bitmap_x_offset + glyph_width;
		new_char_data.UV_x1 = normalize_value(x1, atlas_width, 1.0f);
		new_char_data.UV_y1 = normalize_value(glyph_height, atlas_height, 1.0f);

		font_data->char_data[char_data_index++] = new_char_data;

		bitmap_x_offset += glyph_width;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLuint prev_texture = font_data->texture_id;
	glDeleteTextures(1, &prev_texture);

	GLuint new_texture;
	glGenTextures(1, &new_texture);

	font_data->texture_id = static_cast<int>(new_texture);
	glBindTexture(GL_TEXTURE_2D, font_data->texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, bitmap_width, bitmap_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap_memory);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}