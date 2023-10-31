#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <ft2build.h>
#include FT_FREETYPE_H  

#include <array>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <unordered_map>

#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/matrix_inverse.hpp>
#include <gtc/quaternion.hpp>
#include <gtc/type_ptr.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "j_buffers.h"
#include "utils.h"

using namespace glm;

GLFWwindow* g_window;
glm::vec3 lastMousePos(0.0f);

constexpr int right_hand_panel_width = 400;

typedef struct UserSettings {
	int window_size_px[2];
	float transform_clip;
	float transform_rotation_clip;
	glm::vec3 world_ambient;
} UserSettings;

UserSettings g_user_settings = { 
	.window_size_px = { 1900, 1200 },
	.transform_clip = 0.25f,
	.transform_rotation_clip = 15.0f,
	.world_ambient = glm::vec3(0.2f),
};

int g_wireframe_shader;
unsigned int g_wireframe_vao;
unsigned int g_wireframe_vbo;

int g_line_shader;
unsigned int g_line_vao;
unsigned int g_line_vbo;

int g_billboard_shader;
unsigned int g_billboard_vao;
unsigned int g_billboard_vbo;

int g_mesh_shader;
unsigned int g_mesh_vao;
unsigned int g_mesh_vbo;

int g_ui_text_shader;
unsigned int g_ui_text_vao;
unsigned int g_ui_text_vbo;

int g_text_buffer_size = 0;
int g_text_indicies = 0;

glm::vec3 g_debug_click_camera_pos = glm::vec3(0, 0, 0);
glm::vec3 g_debug_click_ray_normal = glm::vec3(0, 0, 0);
glm::vec3 g_debug_plane_intersection = glm::vec3(0, 0, 0);

typedef struct FrameData {
	float mouse_x;
	float mouse_y;
	float prev_mouse_x;
	float prev_mouse_y;
	int draw_calls;
	float deltatime;
	bool mouse_clicked;
} FrameData;

typedef struct ButtonState {
	int key;
	bool pressed;
	bool is_down;
} ButtonState;

typedef struct GameInputs {
	ButtonState mouse1;
	ButtonState mouse2;
	ButtonState q;
	ButtonState w;
	ButtonState e;
	ButtonState r;
	ButtonState a;
	ButtonState s;
	ButtonState d;
	ButtonState f;
	ButtonState z;
	ButtonState x;
	ButtonState c;
	ButtonState v;
	ButtonState y;
	ButtonState esc;
	ButtonState plus;
	ButtonState minus;
	ButtonState del;
	ButtonState left_ctrl;
	ButtonState space;
} GameInputs;

union GameInputsU {
	GameInputs as_struct;
	ButtonState as_array[sizeof(GameInputs)];
};

GameInputsU g_inputs = {};

typedef struct GameMetrics {
	unsigned long frames;
	int game_width_px;
	int game_height_px;
	int scene_width_px;
	int scene_height_px;
	float aspect_ratio_horizontal;
	double game_time;
	double prev_frame_game_time;
	int fps;
	int fps_frames;
	int fps_prev_second;
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

float debug_font_vh = 1.0f;
const char* g_debug_font_path = "G:/projects/game/Engine3D/resources/fonts/Inter-Regular.ttf";

int g_max_UI_chars = 1000;
FontData g_debug_font;

FrameData g_frame_data = {};

constexpr const s64 SCENE_MESHES_MAX_COUNT = 100;
MemoryBuffer g_temp_memory = { 0 };
MemoryBuffer g_ui_text_vertex_buffer = { 0 };

typedef struct GameCamera {
	glm::vec3 position;
	glm::vec3 front_vec;
	glm::vec3 up_vec;
	float yaw;
	float pitch;
	float fov;
	float aspect_ratio_horizontal;
	float look_sensitivity;
	float move_speed;
	float near_clip;
	float far_clip;
} GameCamera;

GameCamera scene_camera_init()
{
	GameCamera cam = {
		.position = vec3(0),
		.front_vec = vec3(1,0,0),
		.up_vec = vec3(0,1,0),
		.yaw = 0.0f,
		.pitch = 0.0f,
		.fov = 60.0f,
		.aspect_ratio_horizontal = 1.0f,
		.look_sensitivity = 0.1f,
		.move_speed = 5.0f,
		.near_clip = 0.1f,
		.far_clip = 100.0f,
	};
	return cam;
}

GameCamera g_scene_camera;
bool g_camera_move_mode = false;

float g_mouse_movement_x = 0;
float g_mouse_movement_y = 0;

const char* billboard_image_path = "G:/projects/game/Engine3D/resources/images/light_billboard_000.png";

char material_paths[][FILE_PATH_LEN] = {
	"G:/projects/game/Engine3D/resources/images/debug_img_01.png",
	"G:/projects/game/Engine3D/resources/materials/tilemap_floor_01.png",
	"G:/projects/game/Engine3D/resources/materials/tile_bricks_01.png"
};

constexpr const s64 SCENE_TEXTURES_MAX_COUNT = 100;
MemoryBuffer g_material_names_memory = { 0 };
JStringArray g_material_names;
int g_selected_texture_item = 0;

MemoryBuffer g_texture_memory = { 0 };
JArray<Texture> g_textures;

typedef struct Light {
	glm::vec3 position;
	glm::vec3 diffuse;
	float specular;
} Light;

typedef struct Material {
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;
} Material;

Light g_light = {
	.position = glm::vec3(0.0f, 4.0f, 0.0f),
	.diffuse = glm::vec3(1.0f),
	.specular = 1.0f
};

Material g_material = {
	.ambient = glm::vec3(1.0f),
	.diffuse = glm::vec3(1.0f),
	.specular = glm::vec3(0.1f),
	.shininess = 32.0f
};

Texture pointlight_texture;

// Custom hash function for char*
struct CharPtrHash {
	std::size_t operator()(const char* str) const {
		return std::hash<std::string_view>{}(str);
	}
};

// Custom equality operator for char*
struct CharPtrEqual {
	bool operator()(const char* a, const char* b) const {
		return std::strcmp(a, b) == 0;
	}
};
std::unordered_map<char*, s64, CharPtrHash, CharPtrEqual> g_materials_index_map = {};

Mesh* g_selected_mesh = nullptr;
int g_selected_mesh_index = -1;

MemoryBuffer g_scene_memory = { 0 };
JArray<Mesh> g_scene_meshes;

enum Transformation {
	Translate,
	Rotate,
	Scale
};

typedef struct TransformationMode {
	Axis axis;
	Transformation transformation;
	bool is_active;
} TransformationMode;

TransformationMode g_transform_mode = {};
glm::vec3 g_normal_for_ray_intersect;
glm::vec3 g_used_transform_ray;
glm::vec3 g_prev_intersection;
glm::vec3 g_new_translation;
glm::vec3 g_new_rotation;
glm::vec3 g_new_scale;
glm::vec3 g_point_on_scale_plane;
glm::vec3 g_prev_point_on_scale_plane;

inline glm::mat4 get_projection_matrix()
{
	glm::mat4 projection = glm::perspective(
		glm::radians(g_scene_camera.fov),
		g_scene_camera.aspect_ratio_horizontal,
		g_scene_camera.near_clip,
		g_scene_camera.far_clip);
	return projection;
}

inline glm::mat4 get_view_matrix()
{
	glm::mat4 view = glm::lookAt(
		g_scene_camera.position,
		g_scene_camera.position + g_scene_camera.front_vec,
		g_scene_camera.up_vec);
	return view;
}

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

bool check_shader_compile_error(GLuint shader)
{
	GLint success;
	constexpr int length = 512;
	GLchar infoLog[length];

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(shader, length, NULL, infoLog);
		std::cout
			<< "ERROR::SHADER_COMPILATION_ERROR: "
			<< infoLog << std::endl;

		return false;
	}

	return true; // Returns success
}

bool check_shader_link_error(GLuint shader)
{
	GLint success;
	constexpr int length = 512;
	GLchar infoLog[length];

	glGetProgramiv(shader, GL_LINK_STATUS, &success);

	if (!success)
	{
		glGetProgramInfoLog(shader, length, NULL, infoLog);
		std::cout
			<< "ERROR::PROGRAM_LINKING_ERROR: "
			<< infoLog << std::endl;

		return false;
	}

	return true; // Returns success
}

int compile_shader(const char* vertex_shader_path, const char* fragment_shader_path, MemoryBuffer* buffer)
{
	int shader_id;
	char* vertex_shader_code = nullptr;
	char* fragment_shader_code = nullptr;

	read_file_to_memory(vertex_shader_path, buffer);
	auto vertex_code = (char*)buffer->memory;
	int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_code, NULL);
	glCompileShader(vertex_shader);

	bool vs_compile_success = check_shader_compile_error(vertex_shader);
	ASSERT_TRUE(vs_compile_success, "Vertex shader compile");

	read_file_to_memory(fragment_shader_path, buffer);
	auto fragment_code = (char*)buffer->memory;
	int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_code, NULL);
	glCompileShader(fragment_shader);

	bool fs_compile_success = check_shader_compile_error(fragment_shader);
	ASSERT_TRUE(fs_compile_success, "Fragment shader compile");

	shader_id = glCreateProgram();
	glAttachShader(shader_id, vertex_shader);
	glAttachShader(shader_id, fragment_shader);
	glLinkProgram(shader_id);

	bool link_success = check_shader_link_error(shader_id);
	ASSERT_TRUE(link_success, "Shader program link");

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return shader_id;
}

int load_image_into_texture_id(const char* image_path, bool use_nearest)
{
	unsigned int texture;
	int x, y, n;

	stbi_set_flip_vertically_on_load(1);
	byte* data = stbi_load(image_path, &x, &y, &n, 0);
	ASSERT_TRUE(data != NULL, "Load texture");

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	GLint texture_mode = use_nearest ? GL_NEAREST : GL_LINEAR;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_mode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture_mode);

	ASSERT_TRUE(n == 3 || n == 4, "Image format is RGB or RGBA");

	GLint use_format = n == 3
		? GL_RGB
		: GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, use_format, x, y, 0, use_format, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);
	return texture;
}

void draw_billboard(glm::vec3 position, Texture texture, float scale)
{
	glUseProgram(g_billboard_shader);
	glBindVertexArray(g_billboard_vao);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);

	auto billboard_normal = glm::vec3(0, 0, 1.0f);
	auto billboard_dir = glm::normalize(g_scene_camera.position - position);

	glm::vec3 right = glm::cross(glm::vec3(0, 1.0f, 0), billboard_dir);
	glm::vec3 up = glm::cross(billboard_dir, right);

	glm::mat4 rotation_matrix(1.0f);
	rotation_matrix[0] = glm::vec4(right, 0.0f);
	rotation_matrix[1] = glm::vec4(up, 0.0f);
	rotation_matrix[2] = glm::vec4(billboard_dir, 0.0f);

	model = model * rotation_matrix;
	model = glm::scale(model, glm::vec3(scale));

	glm::mat4 projection = get_projection_matrix();
	glm::mat4 view = get_view_matrix();

	unsigned int model_loc = glGetUniformLocation(g_billboard_shader, "model");
	unsigned int view_loc = glGetUniformLocation(g_billboard_shader, "view");
	unsigned int projection_loc = glGetUniformLocation(g_billboard_shader, "projection");

	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture.texture_id);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void draw_mesh(Mesh* mesh)
{
	glUseProgram(g_mesh_shader);
	glBindVertexArray(g_mesh_vao);

	glm::mat4 model = get_model_matrix(mesh);
	glm::mat4 projection = get_projection_matrix();
	glm::mat4 view = get_view_matrix();

	unsigned int model_loc = glGetUniformLocation(g_mesh_shader, "model");
	unsigned int view_loc = glGetUniformLocation(g_mesh_shader, "view");
	unsigned int projection_loc = glGetUniformLocation(g_mesh_shader, "projection");

	unsigned int use_texture_loc = glGetUniformLocation(g_mesh_shader, "use_texture");
	unsigned int uv_loc = glGetUniformLocation(g_mesh_shader, "uv_multiplier");
	unsigned int ambient_loc = glGetUniformLocation(g_mesh_shader, "ambientLight");
	unsigned int camera_view_loc = glGetUniformLocation(g_mesh_shader, "viewPos");

	unsigned int light_pos_loc = glGetUniformLocation(g_mesh_shader, "light.position");
	unsigned int light_diff_loc = glGetUniformLocation(g_mesh_shader, "light.diffuse");
	unsigned int light_spec_loc = glGetUniformLocation(g_mesh_shader, "light.specular");

	unsigned int material_amb_loc = glGetUniformLocation(g_mesh_shader, "material.ambient");
	unsigned int material_diff_loc = glGetUniformLocation(g_mesh_shader, "material.diffuse");
	unsigned int material_spec_loc = glGetUniformLocation(g_mesh_shader, "material.specular");
	unsigned int material_shine_loc = glGetUniformLocation(g_mesh_shader, "material.shininess");

	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));
	glUniform1f(uv_loc, mesh->uv_multiplier);
	glUniform1i(use_texture_loc, true);

	glUniform3f(ambient_loc, g_user_settings.world_ambient[0], g_user_settings.world_ambient[1], g_user_settings.world_ambient[2]);
	glUniform3f(camera_view_loc, g_scene_camera.position.x, g_scene_camera.position.y, g_scene_camera.position.z);

	glUniform3f(light_pos_loc, g_light.position.x, g_light.position.y, g_light.position.z);
	glUniform3f(light_diff_loc, g_light.diffuse.x, g_light.diffuse.y, g_light.diffuse.z);
	glUniform1f(light_spec_loc, g_light.specular);

	glUniform3f(material_amb_loc, g_material.ambient.x, g_material.ambient.y, g_material.ambient.z);
	glUniform3f(material_diff_loc, g_material.diffuse.x, g_material.diffuse.y, g_material.diffuse.z);
	glUniform3f(material_spec_loc, g_material.specular.x, g_material.specular.y, g_material.specular.z);
	glUniform1f(material_shine_loc, g_material.shininess);

	s64 draw_indicies = 0;

	if (mesh->mesh_type == PrimitiveType::Plane)
	{
		float vertices[] =
		{
			// Coords			// UV		 // Plane normal
			1.0f, 0.0f, 0.0f,	1.0f, 1.0f,	 0.0f, 1.0f, 0.0f, // top right
			0.0f, 0.0f, 0.0f,	0.0f, 1.0f,	 0.0f, 1.0f, 0.0f, // top left
			0.0f, 0.0f, 1.0f,	0.0f, 0.0f,	 0.0f, 1.0f, 0.0f, // bot left

			1.0f, 0.0f, 0.0f,	1.0f, 1.0f,	 0.0f, 1.0f, 0.0f, // top right
			0.0f, 0.0f, 1.0f,	0.0f, 0.0f,	 0.0f, 1.0f, 0.0f, // bot left 
			1.0f, 0.0f, 1.0f,	1.0f, 0.0f,	 0.0f, 1.0f, 0.0f  // bot right
		};

		draw_indicies = 6;
		glBindBuffer(GL_ARRAY_BUFFER, g_mesh_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}
	else if (mesh->mesh_type == PrimitiveType::Cube)
	{
		float vertices[] =
		{
			 // Coords				 // UV			 // Plane normal
			 // Ceiling
			 0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	 0.0f,  1.0f, 0.0f, // top right
			-0.5f,  0.5f, -0.5f,	 0.0f, 1.0f,	 0.0f,  1.0f, 0.0f, // top left
			-0.5f,  0.5f,  0.5f,	 0.0f, 0.0f,	 0.0f,  1.0f, 0.0f, // bot left
								 				    
			 0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	 0.0f,  1.0f, 0.0f, // top right
			-0.5f,  0.5f,  0.5f,	 0.0f, 0.0f,	 0.0f,  1.0f, 0.0f, // bot left 
			 0.5f,  0.5f,  0.5f,	 1.0f, 0.0f,	 0.0f,  1.0f, 0.0f, // bot right
								 
			 // Floor
			-0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	 0.0f, -1.0f, 0.0f, // bot left 
			 0.5f, -0.5f, -0.5f,	 1.0f, 1.0f,	 0.0f, -1.0f, 0.0f, // top right
			 0.5f, -0.5f,  0.5f,	 1.0f, 0.0f,	 0.0f, -1.0f, 0.0f, // bot right

			-0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	 0.0f, -1.0f, 0.0f, // bot left
			-0.5f, -0.5f, -0.5f,	 0.0f, 1.0f,	 0.0f, -1.0f, 0.0f, // top left
			 0.5f, -0.5f, -0.5f,	 1.0f, 1.0f,	 0.0f, -1.0f, 0.0f, // top right

			 // North
			-0.5f, -0.5f, -0.5f,	 0.0f, 0.0f,	 0.0f,  0.0f, -1.0f, // bot left 
			 0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	 0.0f,  0.0f, -1.0f, // top right
			 0.5f, -0.5f, -0.5f,	 1.0f, 0.0f,	 0.0f,  0.0f, -1.0f, // bot right

			-0.5f, -0.5f, -0.5f,	 0.0f, 0.0f,	 0.0f,  0.0f, -1.0f, // bot left
			-0.5f,  0.5f, -0.5f,	 0.0f, 1.0f,	 0.0f,  0.0f, -1.0f, // top left
			 0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	 0.0f,  0.0f, -1.0f, // top right

			 // South
			 0.5f,  0.5f,  0.5f,	 1.0f, 1.0f,	 0.0f,  0.0f,  1.0f, // top right
			-0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	 0.0f,  0.0f,  1.0f, // bot left 
			 0.5f, -0.5f,  0.5f,	 1.0f, 0.0f,	 0.0f,  0.0f,  1.0f, // bot right
			
			-0.5f,  0.5f,  0.5f,	 0.0f, 1.0f,	 0.0f,  0.0f,  1.0f, // top left
			-0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	 0.0f,  0.0f,  1.0f, // bot left
			 0.5f,  0.5f,  0.5f,	 1.0f, 1.0f,	 0.0f,  0.0f,  1.0f, // top right

			 // West
			-0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	-1.0f,  0.0f,  0.0f, // bot left 
			-0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	-1.0f,  0.0f,  0.0f, // top right
			-0.5f, -0.5f, -0.5f,	 1.0f, 0.0f,	-1.0f,  0.0f,  0.0f, // bot right
			
			-0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	-1.0f,  0.0f,  0.0f, // bot left
			-0.5f,  0.5f,  0.5f,	 0.0f, 1.0f,	-1.0f,  0.0f,  0.0f, // top left
			-0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	-1.0f,  0.0f,  0.0f, // top right

			 // East
			 0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	 1.0f,  0.0f,  0.0f, // top right
			 0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	 1.0f,  0.0f,  0.0f, // bot left 
			 0.5f, -0.5f, -0.5f,	 1.0f, 0.0f,	 1.0f,  0.0f,  0.0f, // bot right
			
			 0.5f,  0.5f,  0.5f,	 0.0f, 1.0f,	 1.0f,  0.0f,  0.0f, // top left
			 0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	 1.0f,  0.0f,  0.0f, // bot left
			 0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	 1.0f,  0.0f,  0.0f, // top right
		};

		draw_indicies = 40;
		glBindBuffer(GL_ARRAY_BUFFER, g_mesh_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mesh->texture->texture_id);
	glDrawArrays(GL_TRIANGLES, 0, draw_indicies);
	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void draw_mesh_wireframe(Mesh *mesh, glm::vec3 color)
{
	glUseProgram(g_wireframe_shader);
	glBindVertexArray(g_wireframe_vao);

	glm::mat4 model = get_model_matrix(mesh);
	glm::mat4 projection = get_projection_matrix();
	glm::mat4 view = get_view_matrix();

	unsigned int model_loc = glGetUniformLocation(g_wireframe_shader, "model");
	unsigned int view_loc = glGetUniformLocation(g_wireframe_shader, "view");
	unsigned int projection_loc = glGetUniformLocation(g_wireframe_shader, "projection");
	unsigned int color_loc = glGetUniformLocation(g_wireframe_shader, "color");

	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3f(color_loc, color.r, color.g, color.b);

	s64 draw_indicies = 0;

	if (mesh->mesh_type == PrimitiveType::Plane)
	{
		float vertices[] =
		{
			// Coords		
			1.0f, 0.0f, 0.0f, // top right
			0.0f, 0.0f, 0.0f, // top left
			0.0f, 0.0f, 1.0f, // bot left
							
			1.0f, 0.0f, 0.0f, // top right
			0.0f, 0.0f, 1.0f, // bot left 
			1.0f, 0.0f, 1.0f, // bot right

			0.0f, 0.0f, 0.0f, // top left
			1.0f, 0.0f, 0.0f, // top right
			0.0f, 0.0f, 1.0f, // bot left

			0.0f, 0.0f, 1.0f, // bot left 
			1.0f, 0.0f, 0.0f, // top right
			1.0f, 0.0f, 1.0f  // bot right
		};

		draw_indicies = 12;
		glBindBuffer(GL_ARRAY_BUFFER, g_wireframe_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}
	else if (mesh->mesh_type == PrimitiveType::Cube)
	{
		float vertices[] =
		{
			// Coords		
			// Ceiling
			0.5f,  0.5f, -0.5f, // top right
		   -0.5f,  0.5f, -0.5f, // top left
		   -0.5f,  0.5f,  0.5f, // bot left
							    
			0.5f,  0.5f, -0.5f, // top right
		   -0.5f,  0.5f,  0.5f, // bot left 
			0.5f,  0.5f,  0.5f, // bot right
							    
			// Floor		    
		   -0.5f, -0.5f,  0.5f, // bot left 
			0.5f, -0.5f, -0.5f, // top right
			0.5f, -0.5f,  0.5f, // bot right
							    
		   -0.5f, -0.5f,  0.5f, // bot left
		   -0.5f, -0.5f, -0.5f, // top left
			0.5f, -0.5f, -0.5f, // top right

			// North
		   -0.5f, -0.5f, -0.5f, // bot left 
			0.5f,  0.5f, -0.5f, // top right
			0.5f, -0.5f, -0.5f, // bot right

		   -0.5f, -0.5f, -0.5f, // bot left
		   -0.5f,  0.5f, -0.5f, // top left
			0.5f,  0.5f, -0.5f, // top right

			// South
			0.5f,  0.5f,  0.5f, // top right
		   -0.5f, -0.5f,  0.5f, // bot left 
			0.5f, -0.5f,  0.5f, // bot right

		   -0.5f,  0.5f,  0.5f, // top left
		   -0.5f, -0.5f,  0.5f, // bot left
			0.5f,  0.5f,  0.5f, // top right

			// West
		   -0.5f, -0.5f,  0.5f, // bot left 
		   -0.5f,  0.5f, -0.5f, // top right
		   -0.5f, -0.5f, -0.5f, // bot right

		   -0.5f, -0.5f,  0.5f, // bot left
		   -0.5f,  0.5f,  0.5f, // top left
		   -0.5f,  0.5f, -0.5f, // top right

		   // East
		   0.5f,  0.5f, -0.5f, // top right
		   0.5f, -0.5f,  0.5f, // bot left 
		   0.5f, -0.5f, -0.5f, // bot right

		   0.5f,  0.5f,  0.5f, // top left
		   0.5f, -0.5f,  0.5f, // bot left
		   0.5f,  0.5f, -0.5f  // top right
		};

		draw_indicies = 40;
		glBindBuffer(GL_ARRAY_BUFFER, g_wireframe_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1.5f);
	glDrawArrays(GL_TRIANGLES, 0, draw_indicies);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void draw_line(glm::vec3 start, glm::vec3 end, glm::vec3 color, float thickness_min, float thickness_max)
{
	glUseProgram(g_line_shader);
	glBindVertexArray(g_line_vao);

	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 projection = glm::perspective(glm::radians(g_scene_camera.fov), g_scene_camera.aspect_ratio_horizontal, 0.1f, 100.0f);
	auto new_mat_4 = g_scene_camera.position + g_scene_camera.front_vec;
	glm::mat4 view = glm::lookAt(g_scene_camera.position, new_mat_4, g_scene_camera.up_vec);

	unsigned int model_loc = glGetUniformLocation(g_line_shader, "model");
	unsigned int view_loc = glGetUniformLocation(g_line_shader, "view");
	unsigned int projection_loc = glGetUniformLocation(g_line_shader, "projection");

	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));

	unsigned int color_loc = glGetUniformLocation(g_line_shader, "lineColor");
	glUniform3f(color_loc, color.r, color.g, color.b);

	float vertices[] =
	{
		// Coords
		start.x, start.y, start.z,
		end.x,   end.y,   end.z
	};

	glBindBuffer(GL_ARRAY_BUFFER, g_line_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	glm::vec3 line_midpoint = (start + end) / 2.0f;
	float line_distance = glm::length(g_scene_camera.position - line_midpoint);
	float scaling = 8.0f / (line_distance);
	float average_thickness = (thickness_max + thickness_min) / 2.0f;
	float line_width = clamp_float(average_thickness * scaling, thickness_min, thickness_max);

	glLineWidth(line_width);
	glDrawArrays(GL_LINES, 0, 2);
	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void draw_line_ontop(glm::vec3 start, glm::vec3 end, glm::vec3 color, float thickness)
{
	glUseProgram(g_line_shader);
	glBindVertexArray(g_line_vao);

	glDisable(GL_DEPTH_TEST);

	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 projection = glm::perspective(glm::radians(g_scene_camera.fov), g_scene_camera.aspect_ratio_horizontal, 0.1f, 100.0f);
	auto new_mat_4 = g_scene_camera.position + g_scene_camera.front_vec;
	glm::mat4 view = glm::lookAt(g_scene_camera.position, new_mat_4, g_scene_camera.up_vec);

	unsigned int model_loc = glGetUniformLocation(g_line_shader, "model");
	unsigned int view_loc = glGetUniformLocation(g_line_shader, "view");
	unsigned int projection_loc = glGetUniformLocation(g_line_shader, "projection");

	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));

	unsigned int color_loc = glGetUniformLocation(g_line_shader, "lineColor");
	glUniform3f(color_loc, color.r, color.g, color.b);

	float vertices[] =
	{
		// Coords
		start.x, start.y, start.z,
		end.x,   end.y,   end.z
	};

	glBindBuffer(GL_ARRAY_BUFFER, g_line_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	glLineWidth(thickness);
	glDrawArrays(GL_LINES, 0, 2);
	g_frame_data.draw_calls++;

	glEnable(GL_DEPTH_TEST);
	glUseProgram(0);
	glBindVertexArray(0);
}

void draw_selection_arrows(glm::vec3 position)
{
	auto vec_x = glm::vec3(1.0f, 0, 0);
	auto vec_y = glm::vec3(0, 1.0f, 0);
	auto vec_z = glm::vec3(0, 0, 1.0f);

	constexpr float line_width = 14.0f;

	if (g_transform_mode.transformation == Transformation::Rotate)
	{
		auto start_x = position - vec_x * 0.5f;
		auto start_y = position - vec_y * 0.5f;
		auto start_z = position - vec_z * 0.5f;

		auto end_x = position + vec_x * 0.5f;
		auto end_y = position + vec_y * 0.5f;
		auto end_z = position + vec_z * 0.5f;

		draw_line_ontop(start_y, end_y, glm::vec3(0.0f, 1.0f, 0.0f), line_width / 2);
		draw_line_ontop(start_x, end_x, glm::vec3(1.0f, 0.0f, 0.0f), line_width / 4);
		draw_line_ontop(start_z, end_z, glm::vec3(0.0f, 0.0f, 1.0f), line_width / 4);
	}
	else
	{
		if (g_transform_mode.transformation == Transformation::Scale)
		{
			glm::mat4 rotation_mat = get_rotation_matrix(g_selected_mesh);
			vec_x = rotation_mat * glm::vec4(vec_x, 1.0f);
			vec_y = rotation_mat * glm::vec4(vec_y, 1.0f);
			vec_z = rotation_mat * glm::vec4(vec_z, 1.0f);
		}

		auto end_x = position + vec_x;
		auto end_y = position + vec_y;
		auto end_z = position + vec_z;

		draw_line_ontop(position, end_x, glm::vec3(1.0f, 0.0f, 0.0f), line_width);
		draw_line_ontop(position, end_y, glm::vec3(0.0f, 1.0f, 0.0f), line_width);
		draw_line_ontop(position, end_z, glm::vec3(0.0f, 0.0f, 1.0f), line_width);
	}
}

void append_ui_text(FontData* font_data, char* text, float pos_x_vw, float pos_y_vh)
{
	auto chars = font_data->char_data.data();
	char* text_string = text;
	int length = strlen(text);

	int text_offset_x_px = 0;
	int text_offset_y_px = 0;
	int line_height_px = font_data->font_height_px;

	float* memory_location = (float*)g_ui_text_vertex_buffer.memory;

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

		int x_start = vw_into_screen_px(pos_x_vw, g_game_metrics.scene_width_px) + current.x_offset + text_offset_x_px;
		int char_y_offset = current.y_offset;
		int y_start = vh_into_screen_px(pos_y_vh, g_game_metrics.scene_height_px) + text_offset_y_px - line_height_px + char_y_offset;

		float x0 = normalize_screen_px_to_ndc(x_start, g_game_metrics.scene_width_px);
		float y0 = normalize_screen_px_to_ndc(y_start, g_game_metrics.scene_height_px);

		float x1 = normalize_screen_px_to_ndc(x_start + char_width_px, g_game_metrics.scene_width_px);
		float y1 = normalize_screen_px_to_ndc(y_start + char_height_px, g_game_metrics.scene_height_px);

		float vertices[] =
		{
			// Coords			// UV
			x0, y1, 0.0f,		current.UV_x0, current.UV_y1, // top left
			x0, y0, 0.0f,		current.UV_x0, current.UV_y0, // bottom left
			x1, y0, 0.0f,		current.UV_x1, current.UV_y0, // bottom right

			x1, y1, 0.0f,		current.UV_x1, current.UV_y1, // top right
			x0, y1, 0.0f,		current.UV_x0, current.UV_y1, // top left 
			x1, y0, 0.0f,		current.UV_x1, current.UV_y0  // bottom right
		};

		memcpy(&memory_location[g_text_indicies], vertices, sizeof(vertices));

		g_text_buffer_size += sizeof(vertices);
		g_text_indicies += 30;

		text_offset_x_px += current.advance;
		text++;
	}
}

void draw_ui_text(FontData* font_data, float red, float green, float blue)
{
	glUseProgram(g_ui_text_shader);
	glBindVertexArray(g_ui_text_vao);

	int color_uniform = glGetUniformLocation(g_ui_text_shader, "textColor");
	glUniform3f(color_uniform, red, green, blue);

	glBindBuffer(GL_ARRAY_BUFFER, g_ui_text_vbo);
	glBufferData(GL_ARRAY_BUFFER, g_text_buffer_size, g_ui_text_vertex_buffer.memory, GL_DYNAMIC_DRAW);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, font_data->texture_id);
	glDrawArrays(GL_TRIANGLES, 0, g_text_buffer_size);

	glUseProgram(0);
	glBindVertexArray(0);

	g_text_buffer_size = 0;
	g_text_indicies = 0;
	g_frame_data.draw_calls++;
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

	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void load_font(FontData* font_data, int font_height_px, const char* font_path)
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
	ASSERT_TRUE(bitmap_size < g_temp_memory.size, "Font bitmap buffer fits");

	byte* bitmap_memory = g_temp_memory.memory;

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

inline void ResizeWindowAreaData(s64 width_px, s64 height_px)
{
	g_game_metrics.game_width_px = width_px;
	g_game_metrics.game_height_px = height_px;

	g_game_metrics.scene_width_px = g_game_metrics.game_width_px - right_hand_panel_width;
	g_game_metrics.scene_height_px = g_game_metrics.game_height_px;

	g_scene_camera.aspect_ratio_horizontal =
		(float)g_game_metrics.scene_width_px / (float)g_game_metrics.scene_height_px;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	ResizeWindowAreaData(width, height);
	glViewport(0, 0, g_game_metrics.scene_width_px, g_game_metrics.scene_height_px);
	int font_height_px = normalize_value(debug_font_vh, 100.0f, (float)height);
	load_font(&g_debug_font, font_height_px, g_debug_font_path);
}

void mouse_move_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	g_mouse_movement_x = xpos - g_frame_data.prev_mouse_x;
	g_mouse_movement_y = g_frame_data.prev_mouse_y - ypos;

	g_frame_data.prev_mouse_x = xpos;
	g_frame_data.prev_mouse_y = ypos;
}

bool clicked_scene_space(int x, int y)
{
	return x < g_game_metrics.scene_width_px && y < g_game_metrics.scene_height_px;
}

void delete_mesh(s64 plane_index)
{
	Mesh* last_plane = j_array_get(&g_scene_meshes, g_scene_meshes.items_count - 1);
	j_array_replace(&g_scene_meshes, *last_plane, plane_index);
	j_array_pop_back(&g_scene_meshes);
	g_selected_mesh = nullptr;
	g_selected_mesh_index = -1;
}

s64 add_new_mesh(Mesh new_mesh)
{
	s64 texture_index = 0;
	Texture* texture_ptr = j_array_get(&g_textures, texture_index);
	g_selected_texture_item = texture_index;
	s64 new_index = j_array_add(&g_scene_meshes, new_mesh);
	return new_index;
}

void duplicate_mesh(s64 plane_index)
{
	Mesh mesh_copy = *j_array_get(&g_scene_meshes, plane_index);
	s64 index = add_new_mesh(mesh_copy);
	g_selected_mesh = j_array_get(&g_scene_meshes, index);
	g_selected_mesh_index = index;
}

void select_mesh_index(s64 index)
{
	g_selected_mesh = j_array_get(&g_scene_meshes, index);
	g_selected_mesh_index = index;
	auto selected_texture_name = g_materials_index_map[g_selected_mesh->texture->file_name];
	g_selected_texture_item = selected_texture_name;
}

void deselect_current_mesh()
{
	g_selected_mesh = nullptr;
	g_selected_mesh_index = -1;
}

bool calculate_plane_ray_intersection(
	glm::vec3 plane_normal,
	glm::vec3 point_in_plane,
	glm::vec3 ray_origin,
	glm::vec3 ray_direction,
	glm::vec3& result)
{
	// Calculate the D coefficient of the plane equation
	float D = -glm::dot(plane_normal, point_in_plane);

	// Calculate t where the ray intersects the plane
	float t = -(glm::dot(plane_normal, ray_origin) + D) / glm::dot(plane_normal, ray_direction);

	// Check if t is non-positive (ray doesn't intersect) or invalid
	if (t <= 0.0f || std::isinf(t) || std::isnan(t))
	{
		return false;
	}

	glm::vec3 intersection_point = ray_origin + t * ray_direction;
	result = intersection_point;
	return true;
}

bool get_cube_selection(Mesh* cube, float* select_dist, vec3 ray_o, vec3 ray_dir)
{
	constexpr const s64 CUBE_PLANES_COUNT = 6;
	bool got_selected = false;
	float closest_select = std::numeric_limits<float>::max();

	glm::vec3 cube_normals[CUBE_PLANES_COUNT] = {
		glm::vec3(0.0f,  1.0f,  0.0f),
		glm::vec3(0.0f, -1.0f,  0.0f),
		glm::vec3(1.0f,  0.0f,  0.0f),
		glm::vec3(-1.0f,  0.0f,  0.0f),
		glm::vec3(0.0f,  0.0f,  1.0f),
		glm::vec3(0.0f,  0.0f, -1.0f)
	};

	Axis plane_axises[CUBE_PLANES_COUNT] = {
		Axis::Y,
		Axis::Y,
		Axis::X,
		Axis::X,
		Axis::Z,
		Axis::Z
	};

	glm::mat4 rotation_matrix = get_rotation_matrix(cube);

	for (int j = 0; j < CUBE_PLANES_COUNT; j++)
	{
		auto current_normal = cube_normals[j];
		auto current_axis = plane_axises[j];

		auto plane_normal = glm::vec3(rotation_matrix * glm::vec4(current_normal, 1.0f));
		plane_normal = glm::normalize(plane_normal);

		glm::vec3 intersection_point = glm::vec3(0);

		float plane_scale = get_vec3_val_by_axis(cube->scale, current_axis);
		vec3 plane_middle_point = cube->translation + plane_normal * plane_scale * 0.5f;

		bool intersection = calculate_plane_ray_intersection(
			plane_normal, plane_middle_point, ray_o, ray_dir, intersection_point);

		if (!intersection) continue;

		glm::vec3 intersection_vec3 = intersection_point - plane_middle_point;
		glm::vec3 relative_position = intersection_vec3 - glm::vec3(rotation_matrix[3]);
		glm::mat4 inverse_rotation_matrix = glm::affineInverse(rotation_matrix);
		glm::vec3 local_intersection_point = glm::vec3(inverse_rotation_matrix * glm::vec4(relative_position, 1.0f));

		Axis xor_axises[2] = {};
		get_axis_xor(current_axis, xor_axises);

		float abs1 = std::abs(get_vec3_val_by_axis(local_intersection_point, xor_axises[0]));
		float abs2 = std::abs(get_vec3_val_by_axis(local_intersection_point, xor_axises[1]));

		bool intersect_1 = abs1 <= get_vec3_val_by_axis(cube->scale, xor_axises[0]) * 0.5f;
		bool intersect_2 = abs2 <= get_vec3_val_by_axis(cube->scale, xor_axises[1]) * 0.5f;

		if (intersect_1 && intersect_2)
		{
			float dist = glm::length(ray_o - intersection_point);

			if (dist < closest_select)
			{
				*select_dist = dist;
				closest_select = dist;
			}

			got_selected = true;
		}
	}

	return got_selected;
}

int get_mesh_selection_index(JArray<Mesh> meshes, glm::vec3 ray_origin, glm::vec3 ray_direction)
{
	s64 index = -1;
	float closest_selection = std::numeric_limits<float>::max();

	for (int i = 0; i < meshes.items_count; i++)
	{
		Mesh* mesh = j_array_get(&meshes, i);

		if (mesh->mesh_type == PrimitiveType::Plane)
		{
			auto plane_up = glm::vec3(0.0f, 1.0f, 0.0f);
			glm::mat4 rotationMatrix = get_rotation_matrix(mesh);

			auto planeNormal = glm::vec3(rotationMatrix * glm::vec4(plane_up, 1.0f));
			planeNormal = glm::normalize(planeNormal);

			glm::vec3 intersection_point = glm::vec3(0);

			bool intersection = calculate_plane_ray_intersection(
				planeNormal, mesh->translation, ray_origin, ray_direction, intersection_point);

			if (!intersection) continue;

			glm::vec3 intersection_vec3 = intersection_point - mesh->translation;

			// Calculate the relative position of the intersection point with respect to the plane's origin
			glm::vec3 relative_position = intersection_vec3 - glm::vec3(rotationMatrix[3]);

			// Calculate the inverse of the plane's rotation matrix
			glm::mat4 inverse_rotation_matrix = glm::affineInverse(rotationMatrix);

			// Apply the inverse rotation matrix to convert the point from world space to local space
			glm::vec3 local_intersection_point = glm::vec3(inverse_rotation_matrix * glm::vec4(relative_position, 1.0f));

			bool intersect_x = 0.0f <= local_intersection_point.x && local_intersection_point.x <= mesh->scale.x;
			bool intersect_z = 0.0f <= local_intersection_point.z && local_intersection_point.z <= mesh->scale.z;

			if (intersect_x && intersect_z)
			{
				float dist = glm::length(ray_origin - intersection_point);

				if (dist < closest_selection)
				{
					closest_selection = dist;
					index = i;
				}
			}
		}
		else if (mesh->mesh_type == PrimitiveType::Cube)
		{
			float select_dist;
			bool selected_cube = get_cube_selection(mesh, &select_dist, ray_origin, ray_direction);

			if (selected_cube && select_dist < closest_selection)
			{
				index = i;
				closest_selection = select_dist;
			}
		}
	}

	return index;
}

inline glm::vec3 get_camera_ray_from_scene_px(int x, int y)
{
	float x_NDC = (2.0f * x) / g_game_metrics.scene_width_px - 1.0f;
	float y_NDC = 1.0f - (2.0f * y) / g_game_metrics.scene_height_px;

	glm::vec4 ray_clip(x_NDC, y_NDC, -1.0f, 1.0f);

	glm::mat4 projection = get_projection_matrix();
	glm::mat4 view = get_view_matrix();

	glm::mat4 inverse_projection = glm::inverse(projection);
	glm::mat4 inverse_view = glm::inverse(view);

	glm::vec4 ray_eye = inverse_projection * ray_clip;
	ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

	glm::vec4 ray_world = inverse_view * ray_eye;
	ray_world = glm::normalize(ray_world);

	return glm::vec3(ray_world);
}

inline void set_button_state(GLFWwindow* window, ButtonState* button)
{
	int key_state = glfwGetKey(window, button->key);
	button->pressed = !button->is_down && key_state == GLFW_PRESS;
	button->is_down = key_state == GLFW_PRESS;
}

inline MeshData mesh_serialize(Mesh* mesh)
{
	MeshData data = {
		.translation = mesh->translation,
		.rotation = mesh->rotation,
		.scale = mesh->scale,
		.texture_file_name = "",
		.mesh_type = mesh->mesh_type,
		.uv_multiplier = mesh->uv_multiplier,
	};
	strcpy_s(data.texture_file_name, mesh->texture->file_name);
	return data;
}

inline Mesh mesh_deserailize(MeshData data)
{
	s64 texture_index = g_materials_index_map[data.texture_file_name];
	Texture* m_texture = j_array_get(&g_textures, texture_index);
	Mesh mesh = {
		.translation = data.translation,
		.rotation = data.rotation,
		.scale = data.scale,
		.texture = m_texture,
		.mesh_type = data.mesh_type,
		.uv_multiplier = data.uv_multiplier,
	};
	return mesh;
}

void save_scene()
{
	std::ofstream output_file("scene_01.jmap", std::ios::binary);
	ASSERT_TRUE(output_file.is_open(), ".jmap file opened for save");

	GameCamera data = g_scene_camera;

	// Header
	output_file.write(".jmap", 5);

	// Scene camera
	output_file.write(reinterpret_cast<char*>(&data), sizeof(data));

	// Mesh count
	output_file.write(reinterpret_cast<char*>(&g_scene_meshes.items_count), sizeof(s64));

	// Mesh data
	for (int i = 0; i < g_scene_meshes.items_count; i++)
	{
		Mesh* mesh_ptr = j_array_get(&g_scene_meshes, i);
		MeshData m_data = mesh_serialize(mesh_ptr);
		output_file.write(reinterpret_cast<char*>(&m_data), sizeof(m_data));
	}

	output_file.close();

	printf("Scene saved.\n");
}

void load_scene()
{
	const char* filename = "scene_01.jmap";

	if (!std::filesystem::exists(filename))
	{
		printf(".jmap file not found.\n");
		g_scene_camera = scene_camera_init();
		return;
	}

	std::ifstream input_file(filename, std::ios::binary);
	ASSERT_TRUE(input_file.is_open(), ".jmap file opened for load");

	// Header
	char header[6] = { 0 };
	input_file.read(header, 5);
	int xx = strcmp(header, ".jmap");
	ASSERT_TRUE(strcmp(header, ".jmap") == 0, ".jmap file has valid header");

	// Scene camera
	GameCamera cam_data = {};
	input_file.read(reinterpret_cast<char*>(&cam_data), sizeof(cam_data));
	g_scene_camera = cam_data;

	// Mesh count
	s64 mesh_count = 0;
	input_file.read(reinterpret_cast<char*>(&mesh_count), sizeof(s64));

	// Mesh data
	for (int i = 0; i < mesh_count; i++)
	{
		MeshData m_data;
		input_file.read(reinterpret_cast<char*>(&m_data), sizeof(m_data));
		Mesh mesh = mesh_deserailize(m_data);
		j_array_add(&g_scene_meshes, mesh);
	}

	input_file.close();

	printf("Scene loaded.\n");
}

Texture texture_load_from_filepath(char* path)
{
	int texture_id = load_image_into_texture_id(path, true);
	char* file_name = strrchr(const_cast<char*>(path), '/');
	file_name++;
	ASSERT_TRUE(file_name, "Filename from file path");
	s64 name_len = strlen(file_name);
	Texture texture = {
		.file_name = "",
		.texture_id = texture_id,
	};
	memcpy(texture.file_name, file_name, name_len);
	return texture;
}

int main(int argc, char* argv[])
{
	// Init buffers
	{
		memory_buffer_mallocate(&g_temp_memory, MEGABYTES(5), const_cast<char*>("Temp memory"));

		memory_buffer_mallocate(&g_scene_memory, sizeof(Mesh) * SCENE_MESHES_MAX_COUNT, const_cast<char*>("Scene meshes"));
		g_scene_meshes = j_array_init(SCENE_MESHES_MAX_COUNT, sizeof(Mesh), (Mesh*)g_scene_memory.memory);

		memory_buffer_mallocate(&g_texture_memory, sizeof(Texture) * SCENE_TEXTURES_MAX_COUNT, const_cast<char*>("Textures"));
		g_textures = j_array_init(SCENE_TEXTURES_MAX_COUNT, sizeof(Texture),(Texture*)g_texture_memory.memory);

		constexpr const s64 material_names_arr_size = TEXTURE_FILENAME_LEN * SCENE_TEXTURES_MAX_COUNT;
		memory_buffer_mallocate(&g_material_names_memory, material_names_arr_size, const_cast<char*>("Material items"));
		g_material_names = j_strings_init(material_names_arr_size, (char*)g_material_names_memory.memory);
	}

	// Init window and context
	{
		int glfw_init_result = glfwInit();
		ASSERT_TRUE(glfw_init_result == GLFW_TRUE, "glfw init");

		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		g_window = glfwCreateWindow(640, 400, "JMF Engine3D", NULL, NULL);
		ASSERT_TRUE(g_window, "Window creation");

		glfwMakeContextCurrent(g_window);
		glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);
		glfwSetCursorPosCallback(g_window, mouse_move_callback);

		int glew_init_result = glewInit();
		ASSERT_TRUE(glew_init_result == GLEW_OK, "glew init");
	}

	// Init ImGui
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui_ImplGlfw_InitForOpenGL(g_window, true);
		ImGui_ImplOpenGL3_Init("#version 330 core");
	}

	// Init game inputs
	{
		g_inputs.as_struct.mouse1 = ButtonState{ .key = GLFW_MOUSE_BUTTON_1 };
		g_inputs.as_struct.mouse2 = ButtonState{ .key = GLFW_MOUSE_BUTTON_2 };

		g_inputs.as_struct.q = ButtonState{ .key = GLFW_KEY_Q };
		g_inputs.as_struct.w = ButtonState{ .key = GLFW_KEY_W };
		g_inputs.as_struct.e = ButtonState{ .key = GLFW_KEY_E };
		g_inputs.as_struct.r = ButtonState{ .key = GLFW_KEY_R };
		g_inputs.as_struct.a = ButtonState{ .key = GLFW_KEY_A };
		g_inputs.as_struct.s = ButtonState{ .key = GLFW_KEY_S };
		g_inputs.as_struct.d = ButtonState{ .key = GLFW_KEY_D };
		g_inputs.as_struct.f = ButtonState{ .key = GLFW_KEY_F };
		g_inputs.as_struct.z = ButtonState{ .key = GLFW_KEY_Z };
		g_inputs.as_struct.x = ButtonState{ .key = GLFW_KEY_X };
		g_inputs.as_struct.c = ButtonState{ .key = GLFW_KEY_C };
		g_inputs.as_struct.v = ButtonState{ .key = GLFW_KEY_V };
		g_inputs.as_struct.y = ButtonState{ .key = GLFW_KEY_Y };
		g_inputs.as_struct.esc = ButtonState{ .key = GLFW_KEY_ESCAPE };
		g_inputs.as_struct.plus = ButtonState{ .key = GLFW_KEY_KP_ADD };
		g_inputs.as_struct.minus = ButtonState{ .key = GLFW_KEY_KP_SUBTRACT };
		g_inputs.as_struct.del = ButtonState{ .key = GLFW_KEY_DELETE };
		g_inputs.as_struct.left_ctrl = ButtonState{ .key = GLFW_KEY_LEFT_CONTROL };
		g_inputs.as_struct.space = ButtonState{ .key = GLFW_KEY_SPACE };
	}

	// Init billboard shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/billboard_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/billboard_fs.glsl";

		g_billboard_shader = compile_shader(vertex_shader_path, fragment_shader_path, &g_temp_memory);
		{
			glGenVertexArrays(1, &g_billboard_vao);
			glGenBuffers(1, &g_billboard_vbo);

			glBindVertexArray(g_billboard_vao);
			glBindBuffer(GL_ARRAY_BUFFER, g_billboard_vbo);

			float vertices[] =
			{
				// Coords			 // UVs
				-0.5f, -0.5f, 0.0f,	 0.0f, 0.0f, // bottom left
				 0.5f, -0.5f, 0.0f,	 1.0f, 0.0f, // bottom right
				-0.5f,  0.5f, 0.0f,	 0.0f, 1.0f, // top left
										
				-0.5f,  0.5f, 0.0f,	 0.0f, 1.0f, // top left 
				 0.5f, -0.5f, 0.0f,	 1.0f, 0.0f, // bottom right
				 0.5f,  0.5f, 0.0f,	 1.0f, 1.0f  // top right
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			// UV attribute
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
		}
	}

	// Init UI text shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/ui_text_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/ui_text_fs.glsl";

		g_ui_text_shader = compile_shader(vertex_shader_path, fragment_shader_path, &g_temp_memory);
		{
			glGenVertexArrays(1, &g_ui_text_vao);
			glGenBuffers(1, &g_ui_text_vbo);

			glBindVertexArray(g_ui_text_vao);
			glBindBuffer(GL_ARRAY_BUFFER, g_ui_text_vbo);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			// UV attribute
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);

			// Reserve vertex array
			int vertex_data_for_char = 120;
			int text_buffer_size = vertex_data_for_char * g_max_UI_chars;
			memory_buffer_mallocate(&g_ui_text_vertex_buffer, text_buffer_size, const_cast<char*>("UI text verticies"));
		}
	}

	// Init mesh shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/mesh_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/mesh_fs.glsl";

		g_mesh_shader = compile_shader(vertex_shader_path, fragment_shader_path, &g_temp_memory);
		{
			glGenVertexArrays(1, &g_mesh_vao);
			glBindVertexArray(g_mesh_vao);

			glGenBuffers(1, &g_mesh_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, g_mesh_vbo);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			// UV attribute
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);

			// Normal attribute
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
			glEnableVertexAttribArray(2);
		}
	}

	// Init wireframe shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/wireframe_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/wireframe_fs.glsl";

		g_wireframe_shader = compile_shader(vertex_shader_path, fragment_shader_path, &g_temp_memory);
		{
			glGenVertexArrays(1, &g_wireframe_vao);
			glBindVertexArray(g_wireframe_vao);

			glGenBuffers(1, &g_wireframe_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, g_wireframe_vbo);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
		}
	}

	// Init line shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/line_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/line_fs.glsl";

		g_line_shader = compile_shader(vertex_shader_path, fragment_shader_path, &g_temp_memory);
		{
			glGenVertexArrays(1, &g_line_vao);
			glBindVertexArray(g_line_vao);

			glGenBuffers(1, &g_line_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, g_line_vbo);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
		}
	}

	// Init textures
	{
		pointlight_texture = texture_load_from_filepath(const_cast<char*>(billboard_image_path));

		s64 materials_count = sizeof(material_paths) / FILE_PATH_LEN;

		for (int i = 0; i < materials_count; i++)
		{
			char* path = material_paths[i];
			Texture new_texture = texture_load_from_filepath(const_cast<char*>(path));
			j_array_add(&g_textures, new_texture);

			char* new_str = j_strings_add(&g_material_names, new_texture.file_name);
			g_materials_index_map[new_str] = i;
			printf("Whaat %s - %d\n", new_texture.file_name, i);
		}
	}

	int font_height_px = normalize_value(debug_font_vh, 100.0f, g_game_metrics.game_height_px);
	load_font(&g_debug_font, font_height_px, g_debug_font_path);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glClearColor(0.25f, 0.35f, 0.35f, 1.0f);

	load_scene();
	glfwSetWindowSize(g_window, g_user_settings.window_size_px[0], g_user_settings.window_size_px[1]);

	while (!glfwWindowShouldClose(g_window))
	{
		glfwPollEvents();

		// -------------
		// ImGui logic
		{
			ImGui_ImplGlfw_NewFrame();
			ImGui_ImplOpenGL3_NewFrame();
			ImGui::NewFrame();

			// Right hand panel
			{
				ImGui::SetNextWindowPos(ImVec2(static_cast<float>(g_game_metrics.game_width_px - right_hand_panel_width), 0), ImGuiCond_Always);
				ImGui::SetNextWindowSize(ImVec2(right_hand_panel_width, g_game_metrics.game_height_px), ImGuiCond_Always);

				ImGui::Begin("Properties", nullptr, 0);

				if (ImGui::Button("Add plane"))
				{
					Mesh new_plane = {
						.translation = vec3(0),
						.rotation = vec3(0),
						.scale = vec3(1.0f),
						.texture = j_array_get(&g_textures, 0),
						.mesh_type = PrimitiveType::Plane,
						.uv_multiplier = 1.0f,
					};
					s64 new_mesh_index = add_new_mesh(new_plane);
					select_mesh_index(new_mesh_index);
				}

				if (ImGui::Button("Add Cube"))
				{
					Mesh new_cube = {
						.translation = vec3(0),
						.rotation = vec3(0),
						.scale = vec3(1.0f),
						.texture = j_array_get(&g_textures, 0),
						.mesh_type = PrimitiveType::Cube,
						.uv_multiplier = 1.0f,
					};
					s64 new_mesh_index = add_new_mesh(new_cube);
					select_mesh_index(new_mesh_index);
				}

				char selected_mesh_str[24];
				sprintf_s(selected_mesh_str, "Plane index: %d", g_selected_mesh_index);
				ImGui::Text(selected_mesh_str);

				if (g_selected_mesh != nullptr)
				{
					ImGui::Text("Mesh properties");
					ImGui::InputFloat3("Translation", &g_selected_mesh->translation[0], "%.2f");
					ImGui::InputFloat3("Rotation", &g_selected_mesh->rotation[0], "%.2f");
					ImGui::InputFloat3("Scale", &g_selected_mesh->scale[0], "%.2f");

					ImGui::Text("UV properties");
					ImGui::Image((ImTextureID)g_selected_mesh->texture->texture_id, ImVec2(128, 128));

					char* texture_names_ptr = (char*)g_material_names.data;

					if (ImGui::Combo("Material", &g_selected_texture_item, texture_names_ptr, g_material_names.strings_count))
					{
						g_selected_mesh->texture = (Texture*)j_array_get(&g_textures, g_selected_texture_item);
					}

					ImGui::InputFloat("UV mult", &g_selected_mesh->uv_multiplier, 0, 0, "%.2f");
				}

				ImGui::Text("Editor settings");
				ImGui::InputFloat("Transform clip", &g_user_settings.transform_clip, 0, 0, "%.2f");
				ImGui::InputFloat("Rotation clip", &g_user_settings.transform_rotation_clip, 0, 0, "%.2f");

				ImGui::Text("Scene settings");
				ImGui::Text("Light");
				ImGui::InputFloat3("Global ambient", &g_user_settings.world_ambient[0], "%.3f");
				ImGui::InputFloat3("Light pos", &g_light.position[0], "%.3f");
				ImGui::InputFloat("Light specular", &g_light.specular, 0, 0, "%.3f");
				ImGui::InputFloat3("Light diffuse", &g_light.diffuse[0], "%.3f");

				ImGui::Text("Material");
				ImGui::InputFloat3("Material ambient", &g_material.ambient[0], "%.3f");
				ImGui::InputFloat3("Material specular", &g_material.specular[0], "%.3f");
				ImGui::InputFloat3("Material diffuse", &g_material.diffuse[0], "%.3f");
				ImGui::InputFloat("Material shine", &g_material.shininess, 0, 0, "%.3f");

				ImGui::Text("Game window");
				ImGui::InputInt2("Screen width px", &g_user_settings.window_size_px[0]);

				if (ImGui::Button("Apply changes"))
				{
					glfwSetWindowSize(g_window, g_user_settings.window_size_px[0], g_user_settings.window_size_px[1]);
				}

				ImGui::End();
			}
		}

		// -----------------
		// Register inputs
		{
			g_frame_data.mouse_clicked = false;

			if (glfwGetKey(g_window, g_inputs.as_struct.esc.key) == GLFW_PRESS)
			{
				glfwSetWindowShouldClose(g_window, true);
			}

			int key_state;

			key_state = glfwGetMouseButton(g_window, g_inputs.as_struct.mouse1.key);
			g_inputs.as_struct.mouse1.pressed = !g_inputs.as_struct.mouse1.is_down && key_state == GLFW_PRESS;
			g_inputs.as_struct.mouse1.is_down = key_state == GLFW_PRESS;

			key_state = glfwGetMouseButton(g_window, g_inputs.as_struct.mouse2.key);
			g_inputs.as_struct.mouse2.pressed = !g_inputs.as_struct.mouse2.is_down && key_state == GLFW_PRESS;
			g_inputs.as_struct.mouse2.is_down = key_state == GLFW_PRESS;

			int buttons_count = sizeof(g_inputs) / sizeof(ButtonState);

			for (int i = 2; i < buttons_count - 1; i++) // Skip 2 mouse buttons
			{
				ButtonState* button = &g_inputs.as_array[i];
				set_button_state(g_window, button);
			}
		}

		// -------
		// Logic

		g_game_metrics.prev_frame_game_time = g_game_metrics.game_time;
		g_game_metrics.game_time = glfwGetTime();
		g_frame_data.deltatime = (g_game_metrics.game_time - g_game_metrics.prev_frame_game_time);

		double xpos, ypos;
		glfwGetCursorPos(g_window, &xpos, &ypos);

		g_frame_data.mouse_x = xpos;
		g_frame_data.mouse_y = ypos;

		int current_game_second = (int)g_game_metrics.game_time;

		if (current_game_second - g_game_metrics.fps_prev_second > 0)
		{
			g_game_metrics.fps = g_game_metrics.fps_frames;
			g_game_metrics.fps_frames = 0;
			g_game_metrics.fps_prev_second = current_game_second;
		}

		if (g_inputs.as_struct.left_ctrl.is_down && g_inputs.as_struct.s.pressed)
		{
			save_scene();
		}

		if (g_inputs.as_struct.mouse1.pressed)
		{
			g_frame_data.mouse_clicked = true;

			if (clicked_scene_space((int)g_frame_data.mouse_x, (int)g_frame_data.mouse_y))
			{
				glm::vec3 ray_origin = g_scene_camera.position;
				glm::vec3 ray_direction = get_camera_ray_from_scene_px((int)xpos, (int)ypos);

				// Debug line
				if (g_inputs.as_struct.left_ctrl.is_down)
				{
					g_debug_click_ray_normal = ray_direction;
					g_debug_click_camera_pos = ray_origin;
				}
				else
				{
					g_debug_click_ray_normal = glm::vec3(0.0f);
					g_debug_click_camera_pos = glm::vec3(0.0f);
				}

				int select_index = get_mesh_selection_index(g_scene_meshes, ray_origin, ray_direction);

				if (select_index != -1)
				{
					select_mesh_index(select_index);
				}
				else
				{
					deselect_current_mesh();
				}
			}
		}

		if (g_inputs.as_struct.mouse2.pressed && clicked_scene_space((int)g_frame_data.mouse_x, (int)g_frame_data.mouse_y))
		{
			g_camera_move_mode = true;
		}
		else if (!g_inputs.as_struct.mouse2.is_down)
		{
			g_camera_move_mode = false;

			if		(g_inputs.as_struct.q.pressed) g_transform_mode.transformation = Transformation::Translate;
			else if (g_inputs.as_struct.w.pressed) g_transform_mode.transformation = Transformation::Rotate;
			else if (g_inputs.as_struct.e.pressed) g_transform_mode.transformation = Transformation::Scale;
		}

		if (g_camera_move_mode)
		{
			glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			g_mouse_movement_x *= g_scene_camera.look_sensitivity;
			g_mouse_movement_y *= g_scene_camera.look_sensitivity;

			g_scene_camera.yaw += g_mouse_movement_x;
			g_scene_camera.pitch += g_mouse_movement_y;

			if (g_scene_camera.pitch > 89.0f)
			{
				g_scene_camera.pitch = 89.0f;
			}
			else if (g_scene_camera.pitch < -89.0f)
			{
				g_scene_camera.pitch = -89.0f;
			}

			glm::vec3 new_camera_front;
			new_camera_front.x = cos(glm::radians(g_scene_camera.yaw)) * cos(glm::radians(g_scene_camera.pitch));
			new_camera_front.y = sin(glm::radians(g_scene_camera.pitch));
			new_camera_front.z = sin(glm::radians(g_scene_camera.yaw)) * cos(glm::radians(g_scene_camera.pitch));

			g_scene_camera.front_vec = glm::normalize(new_camera_front);

			float speed_mult = g_scene_camera.move_speed * g_frame_data.deltatime;

			if (g_inputs.as_struct.w.is_down)
			{
				g_scene_camera.position += speed_mult * g_scene_camera.front_vec;
			}
			if (g_inputs.as_struct.s.is_down)
			{
				g_scene_camera.position -= speed_mult * g_scene_camera.front_vec;
			}
			if (g_inputs.as_struct.a.is_down)
			{
				g_scene_camera.position -= glm::normalize(glm::cross(g_scene_camera.front_vec, g_scene_camera.up_vec)) * speed_mult;
			}
			if (g_inputs.as_struct.d.is_down)
			{
				g_scene_camera.position += glm::normalize(glm::cross(g_scene_camera.front_vec, g_scene_camera.up_vec)) * speed_mult;
			}
		}
		else
		{
			glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		if (g_selected_mesh != nullptr)
		{
			if (g_inputs.as_struct.del.pressed)
			{
				delete_mesh(g_selected_mesh_index);
			}
			else if (g_inputs.as_struct.left_ctrl.is_down && g_inputs.as_struct.d.pressed)
			{
				duplicate_mesh(g_selected_mesh_index);
			}
		}

		// Transformation mode start
		if (g_selected_mesh != nullptr
			&& (g_inputs.as_struct.z.pressed || g_inputs.as_struct.x.pressed || g_inputs.as_struct.c.pressed))
		{
			g_transform_mode.is_active = true;

			if		(g_inputs.as_struct.x.is_down) g_transform_mode.axis = Axis::X;
			else if (g_inputs.as_struct.c.is_down) g_transform_mode.axis = Axis::Y;
			else if (g_inputs.as_struct.z.is_down) g_transform_mode.axis = Axis::Z;

			glm::vec3 intersection_point;
			std::array<glm::vec3, 2> use_normals = get_axis_xor_normals(g_transform_mode.axis);
			g_used_transform_ray = get_camera_ray_from_scene_px((int)xpos, (int)ypos);

			if (g_transform_mode.transformation == Transformation::Translate)
			{
				g_normal_for_ray_intersect = get_vec_for_smallest_dot_product(g_used_transform_ray, use_normals.data(), use_normals.size());

				bool intersection = calculate_plane_ray_intersection(
					g_normal_for_ray_intersect,
					g_selected_mesh->translation,
					g_scene_camera.position,
					g_used_transform_ray,
					intersection_point);

				if (intersection)
				{
					g_prev_intersection = intersection_point;
					g_new_translation = g_selected_mesh->translation;
				}
				else
				{
					g_transform_mode.is_active = false;
				}
			}
			else if (g_transform_mode.transformation == Transformation::Scale)
			{
				glm::mat4 model = get_rotation_matrix(g_selected_mesh);

				use_normals[0] = model * glm::vec4(use_normals[0], 1.0f);
				use_normals[1] = model * glm::vec4(use_normals[1], 1.0f);

				g_normal_for_ray_intersect = get_vec_for_smallest_dot_product(g_used_transform_ray, use_normals.data(), use_normals.size());

				bool intersection = calculate_plane_ray_intersection(
					g_normal_for_ray_intersect,
					g_selected_mesh->translation,
					g_scene_camera.position,
					g_used_transform_ray,
					intersection_point);

				if (intersection)
				{
					glm::vec3 used_normal = get_normal_for_axis(g_transform_mode.axis);
					used_normal = model * glm::vec4(used_normal, 1.0f);

					glm::vec3 point_on_scale_plane = closest_point_on_plane(intersection_point, g_selected_mesh->translation, used_normal);
					g_prev_point_on_scale_plane = point_on_scale_plane;
					g_new_scale = g_selected_mesh->scale;
				}
				else
				{
					g_transform_mode.is_active = false;
				}
			}
			else if (g_transform_mode.transformation == Transformation::Rotate)
			{
				g_normal_for_ray_intersect = get_normal_for_axis(g_transform_mode.axis);

				bool intersection = calculate_plane_ray_intersection(
					g_normal_for_ray_intersect,
					g_selected_mesh->translation,
					g_scene_camera.position,
					g_used_transform_ray,
					intersection_point);

				if (intersection)
				{
					g_prev_intersection = intersection_point;
					g_new_rotation = g_selected_mesh->rotation;
				}
				else
				{
					g_transform_mode.is_active = false;
				}
			}
		}
		// Check transformation mode end
		else if (g_transform_mode.is_active
			&& !g_inputs.as_struct.z.is_down && !g_inputs.as_struct.x.is_down && !g_inputs.as_struct.c.is_down)
		{
			g_transform_mode.is_active = false;
		}
		// Handle transformation mode
		else if (g_transform_mode.is_active)
		{
			glm::vec3 intersection_point;
			g_used_transform_ray = get_camera_ray_from_scene_px((int)xpos, (int)ypos);

			bool intersection = calculate_plane_ray_intersection(
				g_normal_for_ray_intersect,
				g_selected_mesh->translation,
				g_scene_camera.position,
				g_used_transform_ray,
				intersection_point);

			g_debug_plane_intersection = intersection_point;

			if (intersection && g_transform_mode.transformation == Transformation::Translate)
			{
				glm::vec3 travel_dist = intersection_point - g_prev_intersection;
				vec3_add_for_axis(g_new_translation, travel_dist, g_transform_mode.axis);
				g_prev_intersection = intersection_point;

				if (0.0f < g_user_settings.transform_clip)
				{
					g_selected_mesh->translation = clip_vec3(g_new_translation, g_user_settings.transform_clip);
				}
				else
				{
					g_selected_mesh->translation = g_new_translation;
				}
			}
			else if (intersection && g_transform_mode.transformation == Transformation::Scale)
			{
				glm::mat4 rotation_mat4 = get_rotation_matrix(g_selected_mesh);
				glm::vec3 used_normal = get_normal_for_axis(g_transform_mode.axis);

				used_normal = rotation_mat4 * glm::vec4(used_normal, 1.0f);
				glm::vec3 point_on_scale_plane = closest_point_on_plane(g_debug_plane_intersection, g_selected_mesh->translation, used_normal);
				g_point_on_scale_plane = point_on_scale_plane;

				// Reverse the rotation by applying the inverse rotation matrix to the vector
				glm::vec3 reversedVector =  glm::inverse(rotation_mat4) * glm::vec4(point_on_scale_plane, 1.0f);
				glm::vec3 reversedVector2 = glm::inverse(rotation_mat4) * glm::vec4(g_prev_point_on_scale_plane, 1.0f);

				glm::vec3 travel_dist = reversedVector - reversedVector2;
				g_prev_point_on_scale_plane = point_on_scale_plane;

				vec3_add_for_axis(g_new_scale, travel_dist, g_transform_mode.axis);

				if (0.0f < g_user_settings.transform_clip)
				{
					g_selected_mesh->scale = clip_vec3(g_new_scale, g_user_settings.transform_clip);
				}
				else
				{
					g_selected_mesh->scale = g_new_scale;
				}
			}
			else if (intersection && g_transform_mode.transformation == Transformation::Rotate)
			{
				auto new_line = g_debug_plane_intersection - g_selected_mesh->translation;

				if (0.15f < glm::length(new_line))
				{
					glm::vec3 prev_vec = glm::normalize(g_prev_intersection - g_selected_mesh->translation);
					glm::vec3 current_vec = glm::normalize(new_line);

					g_prev_intersection = g_debug_plane_intersection;

					bool equal = glm::all(glm::equal(prev_vec, current_vec));

					if (!equal)
					{
						// Calculate the rotation axis using the cross product of the unit vectors
						glm::vec3 rotation_axis_cross = glm::normalize(glm::cross(prev_vec, current_vec));

						// Calculate the rotation angle in radians
						float angle = glm::acos(glm::dot(glm::normalize(prev_vec), glm::normalize(current_vec)));

						// Create the rotation matrix
						glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1.0f), angle, rotation_axis_cross);

						// Extract the rotation as a quaternion
						glm::quat rotation_quaternion = rotation_matrix;

						// Convert the quaternion to Euler angles
						glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(rotation_quaternion));

						if (!(glm::isnan(eulerAngles.x) || glm::isnan(eulerAngles.y) || glm::isnan(eulerAngles.z)))
						{
							g_new_rotation += eulerAngles;

							if (0.0f < g_user_settings.transform_rotation_clip)
							{
								g_selected_mesh->rotation = clip_vec3(g_new_rotation, g_user_settings.transform_rotation_clip);
							}
							else
							{
								g_selected_mesh->rotation = g_new_rotation;
							}

							g_selected_mesh->rotation.x = float_modulus_operation(g_selected_mesh->rotation.x, 360.0f);
							g_selected_mesh->rotation.y = float_modulus_operation(g_selected_mesh->rotation.y, 360.0f);
							g_selected_mesh->rotation.z = float_modulus_operation(g_selected_mesh->rotation.z, 360.0f);
						}
					}
				}
			}
		}

		// -------------
		// Draw OpenGL
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (int i = 0; i < g_scene_meshes.items_count; i++)
		{
			Mesh plane = *j_array_get(&g_scene_meshes, i);
			draw_mesh(&plane);
		}

		// Draw lights
		draw_billboard(g_light.position, pointlight_texture, 0.35f);

		// Transformation mode debug lines
		if (g_selected_mesh != nullptr && g_transform_mode.is_active)
		{
			// Scale mode
			if (g_transform_mode.transformation == Transformation::Scale)
			{

			}

			// Translate mode
			if (g_transform_mode.transformation == Transformation::Translate)
			{

			}

			// Rotate mode
			if (g_transform_mode.transformation == Transformation::Rotate)
			{
				draw_line(g_debug_plane_intersection, g_selected_mesh->translation, glm::vec3(1.0f, 1.0f, 0.0f), 2.0f, 2.0f);
			}
		}

		// Coordinate lines
		{
			draw_line(glm::vec3(-1000.0f, 0.0f, 0.0f), glm::vec3(1000.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 1.0f);
			draw_line(glm::vec3(0.0f, -1000.0f, 0.0f), glm::vec3(0.0f, 1000.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, 1.0f);
			draw_line(glm::vec3(0.0f, 0.0f, -1000.0f), glm::vec3(0.0f, 0.0f, 1000.0f), glm::vec3(0.0f, 0.0f, 1.0f), 1.0f, 1.0f);
		}

		// Draw selection
		if (-1 < g_selected_mesh_index)
		{
			Mesh selected_mesh = *j_array_get(&g_scene_meshes, g_selected_mesh_index);
			auto model = get_model_matrix(&selected_mesh);

			if (selected_mesh.mesh_type == PrimitiveType::Plane)
			{
				draw_mesh_wireframe(&selected_mesh, glm::vec3(1.0f));
			}
			else if (selected_mesh.mesh_type == PrimitiveType::Cube)
			{
				draw_mesh_wireframe(&selected_mesh, glm::vec3(1.0f));
			}

			draw_selection_arrows(selected_mesh.translation);
		}

		// Click ray
		auto debug_click_end = g_debug_click_camera_pos + g_debug_click_ray_normal * 20.0f;
		draw_line(g_debug_click_camera_pos, debug_click_end, glm::vec3(1.0f, 0.2f, 1.0f), 1.0f, 2.0f);

		// Print debug info
		{
			char debug_str[512];

			const char* fps_debug_str_format = "FPS: %d";
			sprintf_s(debug_str, fps_debug_str_format, g_game_metrics.fps);
			append_ui_text(&g_debug_font, debug_str, 0.5f, 100.0f);

			const char* delta_debug_str_format = "Delta: %.2fms";
			float display_deltatime = g_frame_data.deltatime * 1000;
			sprintf_s(debug_str, delta_debug_str_format, display_deltatime);
			append_ui_text(&g_debug_font, debug_str, 4.5f, 100.0f);

			const char* frames_debug_str_format = "Frames: %lu";
			sprintf_s(debug_str, frames_debug_str_format, g_game_metrics.frames);
			append_ui_text(&g_debug_font, debug_str, 10.5f, 100.0f);

			const char* draw_calls_debug_str_format = "Draw calls: %d";
			sprintf_s(debug_str, draw_calls_debug_str_format, ++g_frame_data.draw_calls);
			append_ui_text(&g_debug_font, debug_str, 17.0f, 100.0f);

			const char* camera_pos_debug_str_format = "Camera X=%.2f Y=%.2f Z=%.2f";
			sprintf_s(debug_str, camera_pos_debug_str_format, g_scene_camera.position.x, g_scene_camera.position.y, g_scene_camera.position.z);
			append_ui_text(&g_debug_font, debug_str, 0.5f, 99.0f);

			char* t_mode = nullptr;
			const char* tt = "Translate";
			const char* tr = "Rotate";
			const char* ts = "Scale";
			const char* transform_mode_debug_str_format = "Transform mode: %s";

			if (g_transform_mode.transformation == Transformation::Translate) t_mode = const_cast<char*>(tt);
			if (g_transform_mode.transformation == Transformation::Rotate) t_mode = const_cast<char*>(tr);
			if (g_transform_mode.transformation == Transformation::Scale) t_mode = const_cast<char*>(ts);

			sprintf_s(debug_str, transform_mode_debug_str_format, t_mode);
			append_ui_text(&g_debug_font, debug_str, 0.5f, 2.0f);
			draw_ui_text(&g_debug_font, 0.9f, 0.9f, 0.9f);
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(g_window);
		g_game_metrics.frames++;
		g_game_metrics.fps_frames++;
		g_frame_data.draw_calls = 0;
	}

	glfwTerminate();

	return 0;
}
