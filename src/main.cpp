#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <array>
#include <iostream>
#include <fstream>
#include <thread>
#include <filesystem>
#include <mutex>

#define KILOBYTES(x) (x * 1024)

typedef unsigned char byte;

std::mutex global_log_mutex;
std::ofstream global_log_file_output;
std::filesystem::path global_log_file_path = {};
constexpr const char* GLOBAL_LOG_FILE_NAME = "Engine3D_mylog.txt";

constexpr size_t GLOBAL_STRING_POOL_SIZE = 2048;
std::array<char, GLOBAL_STRING_POOL_SIZE> g_string_pool = {0}; 

int g_simple_rectangle_shader;
unsigned int g_simple_rectangle_vao;
unsigned int g_simple_rectangle_vbo;

int g_ui_text_shader;
unsigned int g_ui_text_vao;
unsigned int g_ui_text_vbo;

int g_game_width_px = 1600;
int g_game_height_px = 1200;

int g_game_aspect_ratio_x = 4;
int g_game_aspect_ratio_y = 3;

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

FontData g_debug_font = { 0 };

void assert_true(bool assertion, const char* assertion_title, const char* file, const char* func, int line)
{
	if (!assertion)
	{
		std::cerr << "ERROR: Assertion (" << assertion_title << ") failed in: " << file << " at function: " << func << "(), line: " << line << "." << std::endl;
		exit(1);
	}
}

#define ASSERT_TRUE(assertion, assertion_title) assert_true(assertion, assertion_title, __FILE__, __func__, __LINE__)


bool read_file_to_global_string_pool(const char* file_path, char** str_pointer)
{
	std::ifstream file_stream(file_path);

	if (!file_stream.is_open())
	{
		std::cerr << "Failed to open file: " << file_path << std::endl;
		return false;
	}

	g_string_pool = { 0 }; // Reset global string pool to 0

	file_stream.read(g_string_pool.data(), GLOBAL_STRING_POOL_SIZE);
	file_stream.close();

	*str_pointer = g_string_pool.data();

	return true;
}

int compile_shader(const char* vertex_shader_path, const char* fragment_shader_path)
{
	int shader_result = -1;
	char* vertex_shader_code = nullptr;
	char* fragment_shader_code = nullptr;

	bool read_vertex_success = read_file_to_global_string_pool(vertex_shader_path, &vertex_shader_code);
	ASSERT_TRUE(read_vertex_success, "Read vertex shader");

	int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_code, NULL);
	glCompileShader(vertex_shader);

	bool read_fragment_success = read_file_to_global_string_pool(fragment_shader_path, &fragment_shader_code);
	ASSERT_TRUE(read_fragment_success, "Read fragment shader");

	int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_code, NULL);
	glCompileShader(fragment_shader);

	shader_result = glCreateProgram();
	glAttachShader(shader_result, vertex_shader);
	glAttachShader(shader_result, fragment_shader);
	glLinkProgram(shader_result);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return shader_result;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	float screen_x = (float)width;
	float screen_y = (float)height;

	float x_aspect = screen_x / (float)g_game_aspect_ratio_x;
	float y_aspect = screen_y / (float)g_game_aspect_ratio_y;

	bool is_wider_for_aspect = y_aspect < x_aspect;

	if (is_wider_for_aspect)
	{
		int new_width = (screen_y / (float)g_game_aspect_ratio_y) * g_game_aspect_ratio_x;
		g_game_width_px = new_width;
		g_game_height_px = height;

		int x_start = (screen_x / 2) - ((float)g_game_width_px / 2);
		glViewport(x_start, 0, new_width, height);
	}
	else
	{
		int new_height = (screen_x / (float)g_game_aspect_ratio_x) * g_game_aspect_ratio_y;
		g_game_width_px = width;
		g_game_height_px = new_height;

		int y_start = (screen_y / 2) - ((float)g_game_height_px / 2);
		glViewport(0, y_start, width, new_height);
	}
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

float normalize_screen_px_to_ndc(int value, int max)
{
	float this1 = static_cast<float>(value) / static_cast<float>(max);
	float this2 = 2.0f * this1;
	float res = -1.0f + this2;
	return res;
}

void draw_simple_reactangle(Rectangle2D rect, float r, float g, float b)
{
	glUseProgram(g_simple_rectangle_shader);
	glBindVertexArray(g_simple_rectangle_vao);

	float x0 = normalize_screen_px_to_ndc(rect.bot_left_x, g_game_width_px);
	float y0 = normalize_screen_px_to_ndc(rect.bot_left_y, g_game_height_px);

	float x1 = normalize_screen_px_to_ndc(rect.bot_left_x + rect.width, g_game_width_px);
	float y1 = normalize_screen_px_to_ndc(rect.bot_left_y + rect.height, g_game_height_px);

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

inline float vw_into_screen_px(float value)
{
	return (float)(g_game_width_px) * value * 0.01f;
}

inline float vh_into_screen_px(float value)
{
	return (float)(g_game_height_px) * value * 0.01f;
}

void draw_ui_text(FontData* font_data, char* text, float font_height_vh, float pos_x_vw, float pos_y_vh)
{
	float* text_vertex_buffer = (float*)malloc(KILOBYTES(200));
	auto chars = font_data->char_data.data();
	char* text_string = text;

	int buffer_size = 0;
	int draw_indicies = 0;
	int length = strlen(text);

	float font_scale = vh_into_screen_px(font_height_vh) / (float)font_data->font_height_px;

	int text_offset_x_px = 0;
	int text_offset_y_px = 0;
	int line_height_px = vh_into_screen_px(font_height_vh);

	// Assume font start position is top left corner

	for(int i = 0; i < length; i++)
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

		int char_height_px = current.height * font_scale;
		int char_width_px = current.width * font_scale;

		int x_start = vw_into_screen_px(pos_x_vw) + text_offset_x_px;
		int char_y_offset = current.y_offset * font_scale;
		int y_start = vh_into_screen_px(pos_y_vh) + text_offset_y_px - line_height_px + char_y_offset;

		float x0 = normalize_screen_px_to_ndc(x_start, g_game_width_px);
		float y0 = normalize_screen_px_to_ndc(y_start, g_game_height_px);

		float x1 = normalize_screen_px_to_ndc(x_start + char_width_px, g_game_width_px);
		float y1 = normalize_screen_px_to_ndc(y_start + char_height_px, g_game_height_px);

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
		text_offset_x_px = text_offset_x_px + char_width_px + (current.x_offset * font_scale);

		text++;
	} 

	glUseProgram(g_ui_text_shader);
	glBindVertexArray(g_ui_text_vao);

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

	float x0 = normalize_screen_px_to_ndc(x, g_game_width_px);
	float y0 = normalize_screen_px_to_ndc(y, g_game_height_px);

	float x1 = normalize_screen_px_to_ndc(x + current.width, g_game_width_px);
	float y1 = normalize_screen_px_to_ndc(y + current.height, g_game_height_px);

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

float normalize_value(float value, float src_max, float dest_max)
{
	// Assume 0.0 is min value
	float intermediate = value / src_max;
	float result = dest_max * intermediate;
	return result;
}

int main(int argc, char* argv[])
{
	// Init window and context
	GLFWwindow* window;
	{
		int glfw_init_result = glfwInit();
		ASSERT_TRUE(glfw_init_result == GLFW_TRUE, "glfw init");

		window = glfwCreateWindow(g_game_width_px, g_game_height_px, "My Window", NULL, NULL);
		ASSERT_TRUE(window, "window creation");

		glfwMakeContextCurrent(window);
		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

		int glew_init_result = glewInit();
		ASSERT_TRUE(glew_init_result == GLEW_OK, "glew init");
	}

	// Load debug font
	{
		constexpr int ttf_buffer_size = KILOBYTES(1024);
		unsigned char* ttf_buffer = (unsigned char*)malloc(ttf_buffer_size);

		FILE* file;
		fopen_s(&file, "G:/projects/game/Engine3D/resources/fonts/Inter-Regular.ttf", "rb");
		fread(ttf_buffer, 1, ttf_buffer_size, file);

		stbtt_fontinfo font;
		int font_offset = stbtt_GetFontOffsetForIndex(ttf_buffer, 0);
		stbtt_InitFont(&font, ttf_buffer, font_offset);

		float font_height_px = 64;
		float font_scale = stbtt_ScaleForPixelHeight(&font, font_height_px);
		
		g_debug_font.font_height_px = font_height_px;
		g_debug_font.font_scale = font_scale;

		// Iterate fontinfo and get max height / widths for bitmap atlas

		int bitmap_width = 0;
		int bitmap_height = 0;

		constexpr int starting_char = 32;
		constexpr int last_char = 128;

		for (int c = starting_char; c < last_char; c++)
		{
			int character = c;
			int glyph_index = stbtt_FindGlyphIndex(&font, character);

			int x0, y0, x1, y1;
			stbtt_GetGlyphBitmapBox(&font, glyph_index, font_scale, font_scale, &x0, &y0, &x1, &y1);

			int glyph_width = x1 - x0;
			int glyph_height = y1 - y0;

			if (bitmap_height < glyph_height)
			{
				bitmap_height = glyph_height;
			}

			bitmap_width += glyph_width;
		}

		int bitmap_size = bitmap_width * bitmap_height;
		byte* texture_bitmap = static_cast<byte*>(malloc(bitmap_size));
		memset(texture_bitmap, 0x00, bitmap_size);

		// Create bitmap buffer

		int bitmap_x_offset = 0;
		int char_data_index = 0;

		for (int c = starting_char; c < last_char; c++)
		{
			int character = c;
			int glyph_index = stbtt_FindGlyphIndex(&font, character);

			int font_width, font_height, x_offset, y_offset;
			byte* bitmap = stbtt_GetGlyphBitmap(&font, 0, font_scale, glyph_index, &font_width, &font_height, &x_offset, &y_offset);

			int advance, lsb;
			stbtt_GetCodepointHMetrics(&font, glyph_index, &advance, &lsb);

			int used_y_offset = (font_height + y_offset) * (-1.0f);

			for (int y = 0; y < font_height; y++)
			{
				int src_index = ((font_height - 1) * font_width) - y * font_width;
				int dest_index = y * bitmap_width + bitmap_x_offset;
				memcpy(&texture_bitmap[dest_index], &bitmap[src_index], font_width);

				// texture_bitmap[dest_index] = 255;
				// texture_bitmap[dest_index + (font_width - 1)] = 255;
			}

			CharData new_char_data = { 0 };
			new_char_data.character = static_cast<char>(c);

			new_char_data.width = font_width;
			new_char_data.height = font_height;
			new_char_data.x_offset = x_offset;
			new_char_data.y_offset = used_y_offset;
			new_char_data.advance = (float)lsb * font_scale;

			float atlas_width = static_cast<float>(bitmap_width);
			float atlas_height = static_cast<float>(bitmap_height);

			new_char_data.UV_x0 = normalize_value(bitmap_x_offset, atlas_width, 1.0f);
			new_char_data.UV_y0 = normalize_value(0.0f, atlas_height, 1.0f);

			float x1 = bitmap_x_offset + font_width;
			new_char_data.UV_x1 = normalize_value(x1, atlas_width, 1.0f);
			new_char_data.UV_y1 = normalize_value(font_height, atlas_height, 1.0f);

			g_debug_font.char_data[char_data_index++] = new_char_data;

			bitmap_x_offset += font_width;
		}

		CharData new_char_data = { 0 };
		new_char_data.character = static_cast<char>(' ');
		new_char_data.width = font_height_px / 4;
		new_char_data.height = font_height_px;
		g_debug_font.char_data[0] = new_char_data; // Add spacebar

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		GLuint texture;
		glGenTextures(1, &texture);

		g_debug_font.texture_id = static_cast<int>(texture);
		glBindTexture(GL_TEXTURE_2D, g_debug_font.texture_id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, bitmap_width, bitmap_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texture_bitmap);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}

	// Init simple rectangle shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/simple_reactangle_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/simple_reactangle_fs.glsl";

		g_simple_rectangle_shader = compile_shader(vertex_shader_path, fragment_shader_path);
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

		g_ui_text_shader = compile_shader(vertex_shader_path, fragment_shader_path);

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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	while (!glfwWindowShouldClose(window))
	{
		// Input

		glfwPollEvents();

		// Draw
		
		glClear(GL_COLOR_BUFFER_BIT);

		Rectangle2D rect1 = { 0 };
		rect1.bot_left_x = 0;
		rect1.bot_left_y = 0;
		rect1.height = g_game_height_px;
		rect1.width = g_game_width_px;

		draw_simple_reactangle(rect1, 0.9f, 0.9f, 0.9f);

		const char* my_text =
			"Lorem ipsum is simply dummy text\n";

		draw_ui_text(&g_debug_font, const_cast<char*>(my_text), 5.0f, 1.0f, 100.0f);

		glfwSwapBuffers(window);
	}

	glfwTerminate(); // @Performance: Unnecessary slowdown

	return 0;
}
