#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <ft2build.h>
#include FT_FREETYPE_H  

#include <array>
#include <iostream>
#include <fstream>

#include "utils.h"

int g_simple_rectangle_shader;
unsigned int g_simple_rectangle_vao;
unsigned int g_simple_rectangle_vbo;

int g_ui_text_shader;
unsigned int g_ui_text_vao;
unsigned int g_ui_text_vbo;

typedef struct KeyState {
	int key;
	bool pressed;
	bool is_down;
} KeyState;

typedef struct GameMetrics {
	unsigned long frames;
	int game_width_px;
	int game_height_px;
	int game_aspect_ratio_x;
	int game_aspect_ratio_y;
	float deltatime;
	double game_time;
	double prev_frame_game_time;
} GameMetrics;

typedef struct {
	int bot_left_x;
	int bot_left_y;
	int width;
	int height;
} Rectangle2D;

typedef struct CharData {
	float UV_x0;
	float UV_y0;
	float UV_x1;
	float UV_y1;
	int width;
	int height;
	int x_offset;
	int y_offset;
	int advance;
	char character;
} CharData;

typedef struct FontData {
	std::array<CharData, 96> char_data = { 0 };
	int texture_id;
	float font_scale;
	int font_height_px;
} FontData;

GameMetrics g_game_metrics = { 0 };

FontData g_debug_font;

void read_file_to_memory(const char* file_path, MemoryArena* memory_arena)
{
	std::ifstream file_stream(file_path, std::ios::binary | std::ios::ate);
	ASSERT_TRUE(file_stream.is_open(), "Open file");

	int free_space_bytes = memory_arena->free_space();
	std::streampos file_size = file_stream.tellg();
	ASSERT_TRUE(file_size <= free_space_bytes, "File size fits buffer");

	file_stream.seekg(0, std::ios::beg);

	char* read_pointer = (char*)memory_arena->memory;
	file_stream.read(read_pointer, file_size);
	file_stream.close();

	null_terminate_string(read_pointer, file_size);
}

int compile_shader(const char* vertex_shader_path, const char* fragment_shader_path, MemoryArena* memory_arena)
{
	int shader_result = -1;
	char* vertex_shader_code = nullptr;
	char* fragment_shader_code = nullptr;

	read_file_to_memory(vertex_shader_path, memory_arena);
	auto vertex_code = (char*)memory_arena->memory;
	int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_code, NULL);
	glCompileShader(vertex_shader);

	read_file_to_memory(fragment_shader_path, memory_arena);
	auto fragment_code = (char*)memory_arena->memory;
	int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_code, NULL);
	glCompileShader(fragment_shader);

	shader_result = glCreateProgram();
	glAttachShader(shader_result, vertex_shader);
	glAttachShader(shader_result, fragment_shader);
	glLinkProgram(shader_result);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return shader_result;
}

int load_image_into_texture(const char* image_path)
{
	unsigned int texture;
	int x, y, n;

	stbi_set_flip_vertically_on_load(1);
	byte* data = stbi_load(image_path, &x, &y, &n, 0);
	ASSERT_TRUE(data != NULL, "Load texture");

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	ASSERT_TRUE(n == 3 || n == 4, "Image format is RGB or RGBA");

	GLint use_format = n == 3
		? GL_RGB
		: GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, use_format, x, y, 0, use_format, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);
	return texture;
}

void draw_simple_reactangle(Rectangle2D rect, float r, float g, float b)
{
	glUseProgram(g_simple_rectangle_shader);
	glBindVertexArray(g_simple_rectangle_vao);

	float x0 = normalize_screen_px_to_ndc(rect.bot_left_x, g_game_metrics.game_width_px);
	float y0 = normalize_screen_px_to_ndc(rect.bot_left_y, g_game_metrics.game_height_px);

	float x1 = normalize_screen_px_to_ndc(rect.bot_left_x + rect.width, g_game_metrics.game_width_px);
	float y1 = normalize_screen_px_to_ndc(rect.bot_left_y + rect.height, g_game_metrics.game_height_px);

	float vertices[] =
	{
		// Coords		// Color
		x0, y0, 0.0f,	r, g, b, // bottom left
		x1, y0, 0.0f,	r, g, b, // bottom right
		x0, y1, 0.0f,	r, g, b, // top left

		x0, y1, 0.0f,	r, g, b, // top left 
		x1, y1, 0.0f,	r, g, b, // top right
		x1, y0, 0.0f,	r, g, b  // bottom right
	};

	glBindVertexArray(g_simple_rectangle_vao);

	glBindBuffer(GL_ARRAY_BUFFER, g_simple_rectangle_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glUseProgram(0);
	glBindVertexArray(0);
}

void draw_ui_text(FontData* font_data, char* text, float pos_x_vw, float pos_y_vh, float red, float green, float blue)
{
	float* text_vertex_buffer = (float*)malloc(KILOBYTES(200));
	auto chars = font_data->char_data.data();
	char* text_string = text;

	int buffer_size = 0;
	int draw_indicies = 0;
	int length = strlen(text);

	int text_offset_x_px = 0;
	int text_offset_y_px = 0;
	int line_height_px = font_data->font_height_px;

	// Assume font start position is top left corner

	for (int i = 0; i < length; i++)
	{
		char current_char = *text_string++;

		if (current_char == '\n')
		{
			text_offset_y_px -= line_height_px;
			text_offset_x_px = 0;
			continue;
		}

		int char_index = static_cast<int>(current_char) - 32;
		CharData current = chars[char_index];

		int char_height_px = current.height;
		int char_width_px = current.width;

		int x_start = vw_into_screen_px(pos_x_vw, g_game_metrics.game_width_px) + text_offset_x_px;
		int char_y_offset = current.y_offset;
		int y_start = vh_into_screen_px(pos_y_vh, g_game_metrics.game_height_px) + text_offset_y_px - line_height_px + char_y_offset;

		float x0 = normalize_screen_px_to_ndc(x_start, g_game_metrics.game_width_px);
		float y0 = normalize_screen_px_to_ndc(y_start, g_game_metrics.game_height_px);

		float x1 = normalize_screen_px_to_ndc(x_start + char_width_px, g_game_metrics.game_width_px);
		float y1 = normalize_screen_px_to_ndc(y_start + char_height_px, g_game_metrics.game_height_px);

		float vertices[] =
		{
			// Coords			// UV
			x0, y0, 0.0f,		current.UV_x0, current.UV_y0, // bottom left
			x1, y0, 0.0f,		current.UV_x1, current.UV_y0, // bottom right
			x0, y1, 0.0f,		current.UV_x0, current.UV_y1, // top left

			x0, y1, 0.0f,		current.UV_x0, current.UV_y1, // top left 
			x1, y1, 0.0f,		current.UV_x1, current.UV_y1, // top right
			x1, y0, 0.0f,		current.UV_x1, current.UV_y0  // bottom right
		};

		memcpy(&text_vertex_buffer[draw_indicies], vertices, sizeof(vertices));

		buffer_size += sizeof(vertices);
		draw_indicies += 30;

		int advance = char_width_px + (current.x_offset == 0 ? 1 : current.x_offset);
		text_offset_x_px += advance;

		text++;
	} 

	glUseProgram(g_ui_text_shader);
	glBindVertexArray(g_ui_text_vao);

	int color_uniform = glGetUniformLocation(g_ui_text_shader, "textColor");
	glUniform3f(color_uniform, red, green, blue);

	glBindBuffer(GL_ARRAY_BUFFER, g_ui_text_vbo);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, text_vertex_buffer, GL_DYNAMIC_DRAW);

	glBindTexture(GL_TEXTURE_2D, font_data->texture_id);
	glDrawArrays(GL_TRIANGLES, 0, draw_indicies);

	glUseProgram(0);
	glBindVertexArray(0);

	free(text_vertex_buffer);
}

void draw_ui_character(FontData* font_data, const char character, int x, int y)
{
	int char_as_int = static_cast<int>(character);
	int char_index = char_as_int - 32;

	CharData current = font_data->char_data.data()[char_index];

	float x0 = normalize_screen_px_to_ndc(x, g_game_metrics.game_width_px);
	float y0 = normalize_screen_px_to_ndc(y, g_game_metrics.game_height_px);

	float x1 = normalize_screen_px_to_ndc(x + current.width, g_game_metrics.game_width_px);
	float y1 = normalize_screen_px_to_ndc(y + current.height, g_game_metrics.game_height_px);

	float vertices[] =
	{
		// Coords		// UV
		x0, y0, 0.0f,	current.UV_x0, current.UV_y0, // bottom left
		x1, y0, 0.0f,	current.UV_x1, current.UV_y0, // bottom right
		x0, y1, 0.0f,	current.UV_x0, current.UV_y1, // top left

		x0, y1, 0.0f,	current.UV_x0, current.UV_y1, // top left 
		x1, y1, 0.0f,	current.UV_x1, current.UV_y1, // top right
		x1, y0, 0.0f,	current.UV_x1, current.UV_y0  // bottom right
	};

	glUseProgram(g_ui_text_shader);
	glBindVertexArray(g_ui_text_vao);

	glBindBuffer(GL_ARRAY_BUFFER, g_ui_text_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	glBindTexture(GL_TEXTURE_2D, font_data->texture_id);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glUseProgram(0);
	glBindVertexArray(0);
}

void load_font(FontData* font_data, int font_height_px)
{
	FT_Library ft_lib;
	FT_Face ft_face;

	FT_Error init_ft_err = FT_Init_FreeType(&ft_lib);
	ASSERT_TRUE(init_ft_err == 0, "Init FreeType Library");

	FT_Error new_face_err = FT_New_Face(ft_lib, "G:/projects/game/Engine3D/resources/fonts/Inter-Regular.ttf", 0, &ft_face);
	ASSERT_TRUE(new_face_err == 0, "Load FreeType font face");

	FT_Set_Pixel_Sizes(ft_face, 0, font_height_px);
	font_data->font_height_px = font_height_px;

	unsigned int bitmap_width = 0;
	unsigned int bitmap_height = 0;

	constexpr int starting_char = 32;
	constexpr int last_char = 128;

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
	byte* texture_bitmap = static_cast<byte*>(malloc(bitmap_size));
	memset(texture_bitmap, 0x00, bitmap_size);

	int bitmap_x_offset = 0;
	int char_data_index = 0;

	for (int c = starting_char; c < last_char; c++)
	{
		int character = c;

		FT_Error load_char_err = FT_Load_Char(ft_face, character, FT_LOAD_RENDER);
		ASSERT_TRUE(load_char_err == 0, "Load FreeType Glyph");

		unsigned int glyph_width = ft_face->glyph->bitmap.width;
		unsigned int glyph_height = ft_face->glyph->bitmap.rows;

		int x_offset = ft_face->glyph->bitmap_left;
		int y_offset = ft_face->glyph->bitmap_top - ft_face->glyph->bitmap.rows;
		auto x_advance = ft_face->glyph->advance.x;

		for (int y = 0; y < glyph_height; y++)
		{
			int src_index = ((glyph_height - 1) * glyph_width) - y * glyph_width;
			int dest_index = y * bitmap_width + bitmap_x_offset;
			memcpy(&texture_bitmap[dest_index], &ft_face->glyph->bitmap.buffer[src_index], glyph_width);
		}

		CharData new_char_data = { 0 };
		new_char_data.character = static_cast<char>(c);

		new_char_data.width = glyph_width;
		new_char_data.height = glyph_height;
		new_char_data.x_offset = x_offset;
		new_char_data.y_offset = y_offset;
		new_char_data.advance = x_advance;

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

	CharData new_char_data = { 0 };
	new_char_data.character = static_cast<char>(' ');
	new_char_data.width = font_height_px / 4;
	new_char_data.height = font_height_px;
	font_data->char_data[0] = new_char_data; // Add spacebar

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLuint texture;
	glGenTextures(1, &texture);

	font_data->texture_id = static_cast<int>(texture);
	glBindTexture(GL_TEXTURE_2D, font_data->texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, bitmap_width, bitmap_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texture_bitmap);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	float screen_x = (float)width;
	float screen_y = (float)height;

	float x_aspect = screen_x / (float)g_game_metrics.game_aspect_ratio_x;
	float y_aspect = screen_y / (float)g_game_metrics.game_aspect_ratio_y;

	bool is_wider_for_aspect = y_aspect < x_aspect;

	if (is_wider_for_aspect)
	{
		int new_width = (screen_y / (float)g_game_metrics.game_aspect_ratio_y) * g_game_metrics.game_aspect_ratio_x;
		g_game_metrics.game_width_px = new_width;
		g_game_metrics.game_height_px = height;

		int x_start = (screen_x / 2) - ((float)g_game_metrics.game_width_px / 2);
		glViewport(x_start, 0, new_width, height);
	}
	else
	{
		int new_height = (screen_x / (float)g_game_metrics.game_aspect_ratio_x) * g_game_metrics.game_aspect_ratio_y;
		g_game_metrics.game_width_px = width;
		g_game_metrics.game_height_px = new_height;

		int y_start = (screen_y / 2) - ((float)g_game_metrics.game_height_px / 2);
		glViewport(0, y_start, width, new_height);
	}

	std::cout << "Resize! " << height << std::endl;

	int font_height_px = normalize_value(1.5f, 100.0f, g_game_metrics.game_height_px);
	load_font(&g_debug_font, font_height_px);
}

int main(int argc, char* argv[])
{
	MemoryArena game_memory = { 0 };
	MemoryArena file_memory = { 0 };

	memory_arena_init(&game_memory, MEGABYTES(10));
	memory_arena_init(&file_memory, MEGABYTES(1));

	g_game_metrics.game_aspect_ratio_x = 4;
	g_game_metrics.game_aspect_ratio_y = 3;
	g_game_metrics.game_width_px = 1600;
	g_game_metrics.game_height_px = 1200;

	// Init window and context
	GLFWwindow* window;
	{
		int glfw_init_result = glfwInit();
		ASSERT_TRUE(glfw_init_result == GLFW_TRUE, "glfw init");

		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(g_game_metrics.game_width_px, g_game_metrics.game_height_px, "My Window", NULL, NULL);
		ASSERT_TRUE(window, "window creation");

		glfwMakeContextCurrent(window);
		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

		int glew_init_result = glewInit();
		ASSERT_TRUE(glew_init_result == GLEW_OK, "glew init");
	}

	KeyState debug_key = { 0 };
	debug_key.key = GLFW_KEY_A;

	KeyState plus_key = { 0 };
	plus_key.key = GLFW_KEY_KP_ADD;

	KeyState minus_key = { 0 };
	minus_key.key = GLFW_KEY_KP_SUBTRACT;


	// Init simple rectangle shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/simple_reactangle_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/simple_reactangle_fs.glsl";
		
		g_simple_rectangle_shader = compile_shader(vertex_shader_path, fragment_shader_path, &file_memory);
		{
			glGenVertexArrays(1, &g_simple_rectangle_vao);
			glGenBuffers(1, &g_simple_rectangle_vbo);
			glBindVertexArray(g_simple_rectangle_vao);

			int buffer_size = (6 * 6) * sizeof(float); // vec3 + vec3 * 6 indicies
			glBindBuffer(GL_ARRAY_BUFFER, g_simple_rectangle_vbo);
			glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_DYNAMIC_DRAW);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			// Color attribute
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
		}
	}

	// Init UI text shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/ui_text_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/ui_text_fs.glsl";

		g_ui_text_shader = compile_shader(vertex_shader_path, fragment_shader_path, &file_memory);

		{
			glGenVertexArrays(1, &g_ui_text_vao);
			glGenBuffers(1, &g_ui_text_vbo);
			glBindVertexArray(g_ui_text_vao);

			int buffer_size = (5 * 6) * sizeof(float); // vec3 + vec2 * 6 indicies
			glBindBuffer(GL_ARRAY_BUFFER, g_ui_text_vbo);
			glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_DYNAMIC_DRAW);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			// UV attribute
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
		}
	}

	const char* image_path = "G:/projects/game/Engine3D/resources/images/debug_img_01.png";
	unsigned int debug_texture = load_image_into_texture(image_path);

	int font_height_px = normalize_value(1.5f, 100.0f, g_game_metrics.game_height_px);
	load_font(&g_debug_font, font_height_px);

	memory_arena_wipe(&file_memory);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);

	bool show_debug = false;

	while (!glfwWindowShouldClose(window))
	{
		// Input

		glfwPollEvents();

		{
			int key_state;

			key_state = glfwGetKey(window, debug_key.key);
			debug_key.pressed = !debug_key.is_down && key_state == GLFW_PRESS;
			debug_key.is_down = key_state == GLFW_PRESS;

			key_state = glfwGetKey(window, minus_key.key);
			minus_key.pressed = !minus_key.is_down && key_state == GLFW_PRESS;
			minus_key.is_down = key_state == GLFW_PRESS;

			key_state = glfwGetKey(window, plus_key.key);
			plus_key.pressed = !plus_key.is_down && key_state == GLFW_PRESS;
			plus_key.is_down = key_state == GLFW_PRESS;
		}

		// Logic

		g_game_metrics.prev_frame_game_time = g_game_metrics.game_time;
		g_game_metrics.game_time = glfwGetTime();
		g_game_metrics.deltatime = (g_game_metrics.game_time - g_game_metrics.prev_frame_game_time) * 1000;

		if (plus_key.pressed)
		{
			glfwSetWindowSize(window, g_game_metrics.game_width_px + 100, g_game_metrics.game_height_px + 100);
		}
		else if (minus_key.pressed)
		{
			glfwSetWindowSize(window, g_game_metrics.game_width_px - 100, g_game_metrics.game_height_px - 100);
		}

		// Draw
		
		glClear(GL_COLOR_BUFFER_BIT);

		Rectangle2D rect1 = { 0 };
		rect1.bot_left_x = 0;
		rect1.bot_left_y = 0;
		rect1.height = g_game_metrics.game_height_px;
		rect1.width = g_game_metrics.game_width_px;

		draw_simple_reactangle(rect1, 0.9f, 0.9f, 0.9f);

		// Print debug info
		{
			char debug_str[1024];
			const char* debug_str_format = "Delta: %.2fms Frames: %lu";

			sprintf_s(debug_str, debug_str_format,
				g_game_metrics.deltatime,
				g_game_metrics.frames);

			draw_ui_text(&g_debug_font, debug_str, 1.0f, 100.0f, 0.1f, 0.1f, 0.1f);
		}

		glfwSwapBuffers(window);
		g_game_metrics.frames++;
	}

	glfwTerminate(); // @Performance: Unnecessary slowdown

	return 0;
}
