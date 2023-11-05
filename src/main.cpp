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
#include "j_assert.h"
#include "utils.h"

using namespace glm;

static constexpr const char* g_materials_dir_path = "G:\\projects\\game\\Engine3D\\resources\\materials";

static bool g_inverse_color = false;
static bool g_blur_effect = false;
static float g_blur_effect_amount = 0.0f;

static bool g_use_linear_texture_filtering = false;
static bool g_generate_texture_mipmaps = false;

GLFWwindow* g_window;
glm::vec3 lastMousePos(0.0f);

constexpr int right_hand_panel_width = 400;

UserSettings g_user_settings = { 
	.window_size_px = { 1900, 1200 },
	.transform_clip = 0.25f,
	.transform_rotation_clip = 15.0f,
	.world_ambient = glm::vec3(0.1f),
};

static int g_scene_framebuffer_shader;
static unsigned int g_scene_framebuffer;
static unsigned int g_scene_framebuffer_texture;
static unsigned int g_scene_framebuffer_vao;

static unsigned int g_editor_framebuffer;
static unsigned int g_editor_framebuffer_texture;

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

GameInputsU g_inputs = {};

GameMetrics g_game_metrics = { 0 };

float debug_font_vh = 1.0f;
const char* g_debug_font_path = "G:/projects/game/Engine3D/resources/fonts/Inter-Regular.ttf";

int g_max_UI_chars = 1000;
FontData g_debug_font;

FrameData g_frame_data = {};

constexpr const s64 SCENE_MESHES_MAX_COUNT = 100;
MemoryBuffer g_temp_memory = { 0 };
MemoryBuffer g_ui_text_vertex_buffer = { 0 };

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

const char* pointlight_image_path = "G:\\projects\\game\\Engine3D\\resources\\images\\pointlight_billboard.png";
const char* spotlight_image_path = "G:\\projects\\game\\Engine3D\\resources\\images\\spotlight_billboard.png";

constexpr const s64 SCENE_TEXTURES_MAX_COUNT = 100;
MemoryBuffer g_material_names_memory = { 0 };
JStringArray g_material_names;
int g_selected_texture_item = 0;

MemoryBuffer g_texture_memory = { 0 };
JArray<Texture> g_textures;

MemoryBuffer g_materials_memory = { 0 };
JArray<Material> g_materials;

Texture pointlight_texture;
Texture spotlight_texture;

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

static std::unordered_map<char*, s64, CharPtrHash, CharPtrEqual> g_materials_index_map = {};
static std::unordered_map<char*, s64, CharPtrHash, CharPtrEqual> g_mat_data_map;
static std::unordered_map<s64, char*> g_mat_data_map_inverse;

MemoryBuffer g_scene_meshes_memory = { 0 };
JArray<Mesh> g_scene_meshes;

constexpr const s64 SCENE_POINTLIGHTS_MAX_COUNT = 20;
MemoryBuffer g_scene_pointlights_memory = { 0 };
JArray<Pointlight> g_scene_pointlights;

constexpr const s64 SCENE_SPOTLIGHTS_MAX_COUNT = 20;
MemoryBuffer g_scene_spotlights_memory = { 0 };
JArray<Spotlight> g_scene_spotlights;

SceneSelection g_selected_object = {
	.selection_index = -1,
	.type = ObjectType::None,
};

TransformationMode g_transform_mode = {};
glm::vec3 g_normal_for_ray_intersect;
glm::vec3 g_used_transform_ray;
glm::vec3 g_prev_intersection;
glm::vec3 g_new_translation;
glm::vec3 g_new_rotation;
glm::vec3 g_new_scale;
glm::vec3 g_point_on_scale_plane;
glm::vec3 g_prev_point_on_scale_plane;

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

int load_image_into_texture_id(const char* image_path)
{
	unsigned int texture;
	int x, y, n;

	stbi_set_flip_vertically_on_load(1);
	byte* data = stbi_load(image_path, &x, &y, &n, 0);
	ASSERT_TRUE(data != NULL, "Load texture");
	ASSERT_TRUE(n == 3 || n == 4, "Image format is RGB or RGBA");

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GLint filtering_mode = g_use_linear_texture_filtering ? GL_LINEAR : GL_NEAREST;
	GLint mipmap_filtering_mode = g_use_linear_texture_filtering ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;

	if (g_generate_texture_mipmaps) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmap_filtering_mode);
	else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering_mode);

	GLint use_format = n == 3 ? GL_RGB : GL_RGBA;
	glTexImage2D(GL_TEXTURE_2D, 0, use_format, x, y, 0, use_format, GL_UNSIGNED_BYTE, data);

	if (g_generate_texture_mipmaps) glGenerateMipmap(GL_TEXTURE_2D);

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
	glBindTexture(GL_TEXTURE_2D, texture.gpu_id);
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
	unsigned int camera_view_loc = glGetUniformLocation(g_mesh_shader, "view_coords");

	unsigned int ambient_loc = glGetUniformLocation(g_mesh_shader, "global_ambient_light");

	unsigned int use_texture_loc = glGetUniformLocation(g_mesh_shader, "use_texture");
	unsigned int use_gloss_texture_loc = glGetUniformLocation(g_mesh_shader, "use_specular_texture");
	unsigned int uv_loc = glGetUniformLocation(g_mesh_shader, "uv_multiplier");

	unsigned int color_texture_loc = glGetUniformLocation(g_mesh_shader, "material.color_texture");
	unsigned int specular_texture_loc = glGetUniformLocation(g_mesh_shader, "material.specular_texture");
	unsigned int specular_multiplier_loc = glGetUniformLocation(g_mesh_shader, "material.specular_mult");
	unsigned int material_shine_loc = glGetUniformLocation(g_mesh_shader, "material.shininess");

	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));

	glUniform1f(uv_loc, mesh->uv_multiplier);
	glUniform1i(color_texture_loc, 0);
	glUniform1i(use_texture_loc, true);

	glUniform3f(ambient_loc, g_user_settings.world_ambient[0], g_user_settings.world_ambient[1], g_user_settings.world_ambient[2]);
	glUniform3f(camera_view_loc, g_scene_camera.position.x, g_scene_camera.position.y, g_scene_camera.position.z);

	// Lights
	{
		char str_value[64] = { 0 };

		// Pointlights
		unsigned int pointlights_count_loc = glGetUniformLocation(g_mesh_shader, "pointlights_count");
		glUniform1i(pointlights_count_loc, g_scene_pointlights.items_count);

		for (int i = 0; i < g_scene_pointlights.items_count; i++)
		{
			auto pointlight = *j_array_get(&g_scene_pointlights, i);

			sprintf_s(str_value, "pointlights[%d].position", i);
			unsigned int light_pos_loc = glGetUniformLocation(g_mesh_shader, str_value);

			sprintf_s(str_value, "pointlights[%d].diffuse", i);
			unsigned int light_diff_loc = glGetUniformLocation(g_mesh_shader, str_value);

			sprintf_s(str_value, "pointlights[%d].specular", i);
			unsigned int light_spec_loc = glGetUniformLocation(g_mesh_shader, str_value);

			sprintf_s(str_value, "pointlights[%d].intensity", i);
			unsigned int light_intens_loc = glGetUniformLocation(g_mesh_shader, str_value);

			glUniform3f(light_pos_loc, pointlight.position.x, pointlight.position.y, pointlight.position.z);
			glUniform3f(light_diff_loc, pointlight.diffuse.x, pointlight.diffuse.y, pointlight.diffuse.z);
			glUniform1f(light_spec_loc, pointlight.specular);
			glUniform1f(light_intens_loc, pointlight.intensity);
		}

		// Spotlights
		unsigned int spotlights_count_loc = glGetUniformLocation(g_mesh_shader, "spotlights_count");
		glUniform1i(spotlights_count_loc, g_scene_spotlights.items_count);

		for (int i = 0; i < g_scene_spotlights.items_count; i++)
		{
			auto spotlight = *j_array_get(&g_scene_spotlights, i);

			sprintf_s(str_value, "spotlights[%d].diffuse", i);
			unsigned int sp_diff_loc = glGetUniformLocation(g_mesh_shader, str_value);

			sprintf_s(str_value, "spotlights[%d].position", i);
			unsigned int sp_pos_loc = glGetUniformLocation(g_mesh_shader, str_value);

			sprintf_s(str_value, "spotlights[%d].direction", i);
			unsigned int sp_dir_loc = glGetUniformLocation(g_mesh_shader, str_value);

			sprintf_s(str_value, "spotlights[%d].cutoff", i);
			unsigned int sp_cutoff_loc = glGetUniformLocation(g_mesh_shader, str_value);

			sprintf_s(str_value, "spotlights[%d].outer_cutoff", i);
			unsigned int sp_outer_cutoff_loc = glGetUniformLocation(g_mesh_shader, str_value);

			vec3 spot_dir = get_spotlight_dir(spotlight);

			glUniform3f(sp_diff_loc, spotlight.diffuse.x, spotlight.diffuse.y, spotlight.diffuse.z);
			glUniform3f(sp_pos_loc, spotlight.position.x, spotlight.position.y, spotlight.position.z);
			glUniform3f(sp_dir_loc, spot_dir.x, spot_dir.y, spot_dir.z);
			glUniform1f(sp_cutoff_loc, spotlight.cutoff);
			glUniform1f(sp_outer_cutoff_loc, spotlight.outer_cutoff);
		}
	}

	Material* material = mesh->material;
	glUniform1f(material_shine_loc, material->shininess);
	glUniform1f(specular_multiplier_loc, material->specular_mult);

	s64 draw_indicies = 0;

	if (mesh->mesh_type == E_Primitive_Plane)
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
	else if (mesh->mesh_type == E_Primitive_Cube)
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

	bool use_specular_texture = mesh->material->specular_texture != nullptr;
	glUniform1i(use_gloss_texture_loc, use_specular_texture);

	if (use_specular_texture)
	{
		glUniform1i(specular_texture_loc, 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mesh->material->specular_texture->gpu_id);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mesh->material->color_texture->gpu_id);

	glDrawArrays(GL_TRIANGLES, 0, draw_indicies);
	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void draw_mesh_wireframe(Mesh* mesh, glm::vec3 color)
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

	if (mesh->mesh_type == E_Primitive_Plane)
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
	else if (mesh->mesh_type == E_Primitive_Cube)
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

void* get_selected_object_ptr()
{
	if (g_selected_object.type == ObjectType::Primitive)
	{
		return (void*)j_array_get(&g_scene_meshes, g_selected_object.selection_index);
	}

	if (g_selected_object.type == ObjectType::Pointlight)
	{
		return (void*)j_array_get(&g_scene_pointlights, g_selected_object.selection_index);
	}

	if (g_selected_object.type == ObjectType::Spotlight)
	{
		return (void*)j_array_get(&g_scene_spotlights, g_selected_object.selection_index);
	}

	return nullptr;
}

void draw_selection_arrows(glm::vec3 position)
{
	auto vec_x = glm::vec3(1.0f, 0, 0);
	auto vec_y = glm::vec3(0, 1.0f, 0);
	auto vec_z = glm::vec3(0, 0, 1.0f);

	constexpr float line_width = 14.0f;

	if (g_transform_mode.mode == TransformMode::Rotate)
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
		if (g_transform_mode.mode == TransformMode::Scale)
		{
			Mesh* mesh_ptr = (Mesh*)get_selected_object_ptr();
			glm::mat4 rotation_mat = get_rotation_matrix(mesh_ptr->rotation);
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

template <typename T>
void delete_on_object_index(JArray<T>* jarray_ptr, s64 jarray_index)
{
	j_array_unordered_delete(jarray_ptr, jarray_index);
	g_selected_object.selection_index = -1;
	g_selected_object.type = ObjectType::None;
}

void delete_selected_object()
{
	if (g_selected_object.type == ObjectType::Primitive) delete_on_object_index(&g_scene_meshes, g_selected_object.selection_index);
	else if (g_selected_object.type == ObjectType::Pointlight) delete_on_object_index(&g_scene_pointlights, g_selected_object.selection_index);
	else if (g_selected_object.type == ObjectType::Spotlight) delete_on_object_index(&g_scene_spotlights, g_selected_object.selection_index);
}

s64 add_new_mesh(Mesh new_mesh)
{
	s64 material_index = g_materials_index_map[new_mesh.material->color_texture->file_name];
	g_selected_texture_item = material_index;
	j_array_add(&g_scene_meshes, new_mesh);
	s64 new_index = g_scene_meshes.items_count - 1;
	return new_index;
}

s64 add_new_pointlight(Pointlight new_light)
{
	j_array_add(&g_scene_pointlights, new_light);
	s64 new_index = g_scene_pointlights.items_count - 1;
	return new_index;
}

s64 add_new_spotlight(Spotlight new_light)
{
	j_array_add(&g_scene_spotlights, new_light);
	s64 new_index = g_scene_spotlights.items_count - 1;
	return new_index;
}

void duplicate_selected_object()
{
	if (g_selected_object.type == ObjectType::Primitive)
	{
		Mesh mesh_copy = *j_array_get(&g_scene_meshes, g_selected_object.selection_index);
		s64 index = add_new_mesh(mesh_copy);
		g_selected_object.type = ObjectType::Primitive;
		g_selected_object.selection_index = index;
	}
	else if (g_selected_object.type == ObjectType::Pointlight)
	{
		Pointlight light_copy = *j_array_get(&g_scene_pointlights, g_selected_object.selection_index);
		s64 index = add_new_pointlight(light_copy);
		g_selected_object.type = ObjectType::Pointlight;
		g_selected_object.selection_index = index;
	}
	else if (g_selected_object.type == ObjectType::Spotlight)
	{
		Spotlight light_copy = *j_array_get(&g_scene_spotlights, g_selected_object.selection_index);
		s64 index = add_new_spotlight(light_copy);
		g_selected_object.type = ObjectType::Spotlight;
		g_selected_object.selection_index = index;
	}
}

void select_object_index(ObjectType type, s64 index)
{
	g_selected_object.type = type;
	g_selected_object.selection_index = index;

	if (type == ObjectType::Primitive)
	{
		Mesh* mesh_ptr = (Mesh*)get_selected_object_ptr();
		auto selected_texture_name = g_materials_index_map[mesh_ptr->material->color_texture->file_name];
		g_selected_texture_item = selected_texture_name;
	}
	else if (type == ObjectType::Pointlight) g_transform_mode.mode = TransformMode::Translate;
}

void deselect_selection()
{
	g_selected_object.selection_index = -1;
	g_selected_object.type = ObjectType::None;
}

bool get_cube_selection(Mesh* cube, float* select_dist, vec3 ray_o, vec3 ray_dir)
{
	constexpr const s64 CUBE_PLANES_COUNT = 6;
	bool got_selected = false;
	float closest_select = std::numeric_limits<float>::max();

	glm::vec3 cube_normals[CUBE_PLANES_COUNT] = {
		glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f),
		glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(-1.0f,  0.0f,  0.0f),
		glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f,  0.0f, -1.0f)
	};

	s64 plane_axises[CUBE_PLANES_COUNT] = {
		E_Axis_Y, E_Axis_Y,
		E_Axis_X, E_Axis_X,
		E_Axis_Z, E_Axis_Z
	};

	glm::mat4 rotation_matrix = get_rotation_matrix(cube->rotation);

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

		s64 xor_axises[2] = {};
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

s64 get_pointlight_selection_index(JArray<Pointlight>& lights, f32* select_dist, glm::vec3 ray_origin, glm::vec3 ray_direction)
{
	s64 index = -1;
	Mesh as_cube;
	Pointlight* as_light;
	*select_dist = std::numeric_limits<float>::max();

	for (int i = 0; i < lights.items_count; i++)
	{
		as_light = j_array_get(&lights, i);
		as_cube = {};
		as_cube.mesh_type = E_Primitive_Cube;
		as_cube.translation = as_light->position;
		as_cube.scale = vec3(0.35f);

		f32 new_select_dist;
		bool selected_light = get_cube_selection(&as_cube, &new_select_dist, ray_origin, ray_direction);

		if (selected_light && new_select_dist < *select_dist)
		{
			index = i;
			*select_dist = new_select_dist;
		}
	}

	return index;
}

s64 get_spotlight_selection_index(JArray<Spotlight>& lights, f32* select_dist, glm::vec3 ray_origin, glm::vec3 ray_direction)
{
	s64 index = -1;
	Mesh as_cube;
	Spotlight* as_light;
	*select_dist = std::numeric_limits<float>::max();

	for (int i = 0; i < lights.items_count; i++)
	{
		as_light = j_array_get(&lights, i);
		as_cube = {};
		as_cube.mesh_type = E_Primitive_Cube;
		as_cube.translation = as_light->position;
		as_cube.scale = vec3(0.35f);

		f32 new_select_dist;
		bool selected_light = get_cube_selection(&as_cube, &new_select_dist, ray_origin, ray_direction);

		if (selected_light && new_select_dist < *select_dist)
		{
			index = i;
			*select_dist = new_select_dist;
		}
	}

	return index;
}

s64 get_mesh_selection_index(JArray<Mesh>& meshes, f32* select_dist, glm::vec3 ray_origin, glm::vec3 ray_direction)
{
	s64 index = -1;
	*select_dist = std::numeric_limits<float>::max();

	for (int i = 0; i < meshes.items_count; i++)
	{
		Mesh* mesh = j_array_get(&meshes, i);

		if (mesh->mesh_type == E_Primitive_Plane)
		{
			auto plane_up = glm::vec3(0.0f, 1.0f, 0.0f);
			glm::mat4 rotationMatrix = get_rotation_matrix(mesh->rotation);

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

				if (dist < *select_dist)
				{
					*select_dist = dist;
					index = i;
				}
			}
		}
		else if (mesh->mesh_type == E_Primitive_Cube)
		{
			f32 new_select_dist;
			bool selected_cube = get_cube_selection(mesh, &new_select_dist, ray_origin, ray_direction);

			if (selected_cube && new_select_dist < *select_dist)
			{
				index = i;
				*select_dist = new_select_dist;
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

bool try_init_transform_mode()
{
	bool has_valid_mode = g_transform_mode.mode == TransformMode::Translate
		|| (g_transform_mode.mode == TransformMode::Rotate && g_selected_object.type != ObjectType::Pointlight)
		|| (g_transform_mode.mode == TransformMode::Scale && g_selected_object.type == ObjectType::Primitive);

	if (!has_valid_mode) return false;

	if (g_inputs.as_struct.x.is_down) g_transform_mode.axis = E_Axis_X;
	else if (g_inputs.as_struct.c.is_down) g_transform_mode.axis = E_Axis_Y;
	else if (g_inputs.as_struct.z.is_down) g_transform_mode.axis = E_Axis_Z;

	double xpos, ypos;
	glfwGetCursorPos(g_window, &xpos, &ypos);

	glm::vec3 intersection_point;
	std::array<glm::vec3, 2> use_normals = get_axis_xor_normals(g_transform_mode.axis);
	g_used_transform_ray = get_camera_ray_from_scene_px((int)xpos, (int)ypos);

	Mesh* selected_mesh_ptr = (Mesh*)get_selected_object_ptr();

	if (g_transform_mode.mode == TransformMode::Translate)
	{
		g_normal_for_ray_intersect = get_vec_for_smallest_dot_product(g_used_transform_ray, use_normals.data(), use_normals.size());

		bool intersection = calculate_plane_ray_intersection(
			g_normal_for_ray_intersect,
			selected_mesh_ptr->translation,
			g_scene_camera.position,
			g_used_transform_ray,
			intersection_point);

		if (!intersection) return false;

		g_prev_intersection = intersection_point;
		g_new_translation = selected_mesh_ptr->translation;
	}
	else if (g_transform_mode.mode == TransformMode::Scale)
	{
		glm::mat4 model = get_rotation_matrix(selected_mesh_ptr->rotation);

		use_normals[0] = model * glm::vec4(use_normals[0], 1.0f);
		use_normals[1] = model * glm::vec4(use_normals[1], 1.0f);

		g_normal_for_ray_intersect = get_vec_for_smallest_dot_product(g_used_transform_ray, use_normals.data(), use_normals.size());

		bool intersection = calculate_plane_ray_intersection(
			g_normal_for_ray_intersect,
			selected_mesh_ptr->translation,
			g_scene_camera.position,
			g_used_transform_ray,
			intersection_point);

		if (!intersection) return false;

		glm::vec3 used_normal = get_normal_for_axis(g_transform_mode.axis);
		used_normal = model * glm::vec4(used_normal, 1.0f);

		glm::vec3 point_on_scale_plane = closest_point_on_plane(intersection_point, selected_mesh_ptr->translation, used_normal);
		g_prev_point_on_scale_plane = point_on_scale_plane;
		g_new_scale = selected_mesh_ptr->scale;
	}
	else if (g_transform_mode.mode == TransformMode::Rotate)
	{
		g_normal_for_ray_intersect = get_normal_for_axis(g_transform_mode.axis);

		bool intersection = calculate_plane_ray_intersection(
			g_normal_for_ray_intersect,
			selected_mesh_ptr->translation,
			g_scene_camera.position,
			g_used_transform_ray,
			intersection_point);

		if (!intersection) return false;

		g_prev_intersection = intersection_point;
		g_new_rotation = selected_mesh_ptr->rotation;
	}

	return true;
}

inline MeshData mesh_serialize(Mesh* mesh)
{
	s64 material_id = g_mat_data_map[mesh->material->color_texture->file_name];

	MeshData data = {
		.translation = mesh->translation,
		.rotation = mesh->rotation,
		.scale = mesh->scale,
		.mesh_type = mesh->mesh_type,
		.material_id = material_id,
		.uv_multiplier = mesh->uv_multiplier,
	};
	return data;
}

inline Mesh mesh_deserialize(MeshData data)
{
	char* material_name = g_mat_data_map_inverse[data.material_id];
	s64 material_index = g_materials_index_map[material_name];
	Material* material_ptr = j_array_get(&g_materials, material_index);

	Mesh mesh = {
		.translation = data.translation,
		.rotation = data.rotation,
		.scale = data.scale,
		.material = material_ptr,
		.mesh_type = data.mesh_type,
		.uv_multiplier = data.uv_multiplier,
	};
	return mesh;
}

inline void save_scene()
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

	// Pointlight count
	output_file.write(reinterpret_cast<char*>(&g_scene_pointlights.items_count), sizeof(s64));

	// Pointlight data
	for (int i = 0; i < g_scene_pointlights.items_count; i++)
	{
		Pointlight light = *j_array_get(&g_scene_pointlights, i);
		output_file.write(reinterpret_cast<char*>(&light), sizeof(light));
	}

	// Spotlight count
	output_file.write(reinterpret_cast<char*>(&g_scene_spotlights.items_count), sizeof(s64));

	// Spotlight data
	for (int i = 0; i < g_scene_spotlights.items_count; i++)
	{
		Spotlight light = *j_array_get(&g_scene_spotlights, i);
		output_file.write(reinterpret_cast<char*>(&light), sizeof(light));
	}

	output_file.close();

	printf("Scene saved.\n");
}

inline void load_scene()
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
		Mesh mesh = mesh_deserialize(m_data);
		j_array_add(&g_scene_meshes, mesh);
	}

	// Pointlight count
	s64 pointlight_count = 0;
	input_file.read(reinterpret_cast<char*>(&pointlight_count), sizeof(s64));

	// Pointlight data
	for (int i = 0; i < pointlight_count; i++)
	{
		Pointlight light;
		input_file.read(reinterpret_cast<char*>(&light), sizeof(light));
		j_array_add(&g_scene_pointlights, light);
	}

	// Spotlight count
	s64 spotlight_count = 0;
	input_file.read(reinterpret_cast<char*>(&spotlight_count), sizeof(s64));


	// Spotlight data
	for (int i = 0; i < spotlight_count; i++)
	{
		Spotlight light;
		input_file.read(reinterpret_cast<char*>(&light), sizeof(light));
		j_array_add(&g_scene_spotlights, light);
	}

	input_file.close();

	printf("Scene loaded.\n");
}

inline bool has_object_selection()
{
	return g_selected_object.type != ObjectType::None;
}

Texture texture_load_from_filepath(char* path)
{
	int texture_id = load_image_into_texture_id(path);
	char* file_name = strrchr(path, '\\');
	file_name++;
	str_trim_file_ext(file_name);
	ASSERT_TRUE(file_name, "Filename from file path");
	s64 name_len = strlen(file_name);
	Texture texture = {
		.file_name = "",
		.gpu_id = texture_id,
	};
	memcpy(texture.file_name, file_name, name_len);
	return texture;
}

TransformMode get_curr_transformation_mode()
{
	if (g_inputs.as_struct.q.pressed) return TransformMode::Translate;
	if (g_inputs.as_struct.w.pressed && g_selected_object.type != ObjectType::Pointlight) return TransformMode::Rotate;
	if (g_inputs.as_struct.e.pressed && g_selected_object.type == ObjectType::Primitive) return TransformMode::Scale;
	return g_transform_mode.mode;
}

inline MaterialData material_serialize(Material material)
{
	s64 material_id = g_mat_data_map[material.color_texture->file_name];

	MaterialData m_data = {
		.material_id = material_id,
		.specular_mult = material.specular_mult,
		.shininess = material.shininess
	};
	return m_data;
}

inline Material material_deserialize(MaterialData mat_data)
{
	Material mat = {
		.color_texture = nullptr,
		.specular_texture = nullptr,
		.specular_mult = mat_data.specular_mult,
		.shininess = mat_data.shininess,
	};
	return mat;
}

void save_material(Material material)
{
	MaterialData m_data = material_serialize(material);

	char filename[FILENAME_LEN] = { 0 };
	strcpy_s(filename, material.color_texture->file_name);
	str_trim_file_ext(filename);

	char filepath[FILE_PATH_LEN] = { 0 };
	sprintf_s(filepath, "%s\\%s.jmat", g_materials_dir_path, filename);

	std::ofstream output_mat_file(filepath, std::ios::binary | std::ios::trunc);
	ASSERT_TRUE(output_mat_file.is_open(), ".jmat file opened");

	output_mat_file.write(".jmat", 5);
	output_mat_file.write(reinterpret_cast<char*>(&m_data), sizeof(m_data));
	output_mat_file.close();
}

void save_all()
{
	for (int i = 0; i < g_materials.items_count; i++)
	{
		Material to_save = *j_array_get(&g_materials, i);
		save_material(to_save);
		printf("Mat saved.\n");
	}
	save_scene();
}

int main(int argc, char* argv[])
{
	// Init buffers
	{
		memory_buffer_mallocate(&g_temp_memory, MEGABYTES(5), const_cast<char*>("Temp memory"));

		memory_buffer_mallocate(&g_scene_meshes_memory, sizeof(Mesh) * SCENE_MESHES_MAX_COUNT, const_cast<char*>("Scene meshes"));
		g_scene_meshes = j_array_init(SCENE_MESHES_MAX_COUNT, sizeof(Mesh), (Mesh*)g_scene_meshes_memory.memory);

		memory_buffer_mallocate(&g_scene_pointlights_memory, sizeof(Pointlight) * SCENE_POINTLIGHTS_MAX_COUNT, const_cast<char*>("Scene pointlights"));
		g_scene_pointlights = j_array_init(SCENE_POINTLIGHTS_MAX_COUNT, sizeof(Pointlight), (Pointlight*)g_scene_pointlights_memory.memory);

		memory_buffer_mallocate(&g_scene_spotlights_memory, sizeof(Spotlight) * SCENE_SPOTLIGHTS_MAX_COUNT, const_cast<char*>("Scene spotlights"));
		g_scene_spotlights = j_array_init(SCENE_SPOTLIGHTS_MAX_COUNT, sizeof(Spotlight), (Spotlight*)g_scene_spotlights_memory.memory);

		memory_buffer_mallocate(&g_texture_memory, sizeof(Texture) * SCENE_TEXTURES_MAX_COUNT, const_cast<char*>("Textures"));
		g_textures = j_array_init(SCENE_TEXTURES_MAX_COUNT, sizeof(Texture), (Texture*)g_texture_memory.memory);

		memory_buffer_mallocate(&g_materials_memory, sizeof(Material) * SCENE_TEXTURES_MAX_COUNT, const_cast<char*>("Materials"));
		g_materials = j_array_init(SCENE_TEXTURES_MAX_COUNT, sizeof(Material), (Material*)g_materials_memory.memory);

		constexpr const s64 material_names_arr_size = FILENAME_LEN * SCENE_TEXTURES_MAX_COUNT;
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

		glGenVertexArrays(1, &g_wireframe_vao);
		glBindVertexArray(g_wireframe_vao);

		glGenBuffers(1, &g_wireframe_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, g_wireframe_vbo);

		// Coord attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
	}

	// Init line shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/line_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/line_fs.glsl";

		g_line_shader = compile_shader(vertex_shader_path, fragment_shader_path, &g_temp_memory);
		
		glGenVertexArrays(1, &g_line_vao);
		glBindVertexArray(g_line_vao);
		glGenBuffers(1, &g_line_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, g_line_vbo);

		// Coord attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
	}

	// Init framebuffer shaders
	{
		{
			const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/framebuffer_vs.glsl";
			const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/framebuffer_fs.glsl";

			g_scene_framebuffer_shader = compile_shader(vertex_shader_path, fragment_shader_path, &g_temp_memory);

			unsigned int quadVBO;
			glGenVertexArrays(1, &g_scene_framebuffer_vao);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(g_scene_framebuffer_vao);

			float quadVertices[] = {
				// Coords	   // Uv
				-1.0f,  1.0f,  0.0f, 1.0f,
				-1.0f, -1.0f,  0.0f, 0.0f,
				 1.0f, -1.0f,  1.0f, 0.0f,

				-1.0f,  1.0f,  0.0f, 1.0f,
				 1.0f, -1.0f,  1.0f, 0.0f,
				 1.0f,  1.0f,  1.0f, 1.0f
			};

			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		}
	}

	// Init textures/materials
	{
		g_use_linear_texture_filtering = true;
		g_generate_texture_mipmaps = true;

		char path_str[FILE_PATH_LEN] = { 0 };
		strcpy_s(path_str, pointlight_image_path);
		pointlight_texture = texture_load_from_filepath(path_str);

		strcpy_s(path_str, spotlight_image_path);
		spotlight_texture = texture_load_from_filepath(path_str);

		char header[6] = { 0 };
		char filepath[FILE_PATH_LEN] = { 0 };
		const char* mat_data_filename = "materials.data";

		s64 materials_count = 0;

		// Read materials data file
		{
			sprintf_s(filepath, "%s\\%s", g_materials_dir_path, mat_data_filename);

			std::ifstream mat_data_file(filepath, std::ios::in | std::ios::out | std::ios::app);
			mat_data_file.read(header, 5);
			ASSERT_TRUE(strcmp(header, ".data") == 0, "valid .data header");

			s64 materials_data_count = 0;
			mat_data_file.read(reinterpret_cast<char*>(&materials_data_count), sizeof(materials_data_count));

			// Add existing materials from data file
			for (int i = 0; i < materials_data_count; i++)
			{
				MaterialIdData existing_data = {};
				mat_data_file.read(reinterpret_cast<char*>(&existing_data), sizeof(existing_data));

				char* new_name_ptr = j_strings_add(&g_material_names, existing_data.material_name);
				g_mat_data_map.insert(std::make_pair(new_name_ptr, existing_data.material_id));
				g_mat_data_map_inverse.insert(std::make_pair(existing_data.material_id, new_name_ptr));
				g_materials_index_map[new_name_ptr] = materials_count;
				materials_count++;
			}
		}

		// Read .jmat files
		ASSERT_TRUE(std::filesystem::is_directory(g_materials_dir_path), "Valid materials directory");
		const char* material_ext = ".jmat";

		for (const auto& entry : std::filesystem::directory_iterator(g_materials_dir_path))
		{
			bool valid_file = entry.is_regular_file() && entry.path().extension() == material_ext;
			if (!valid_file) continue;

			// Material file
			Material new_material;
			auto filename = entry.path().stem().filename().string();
			sprintf_s(filepath, "%s\\%s.jmat", g_materials_dir_path, filename.c_str());

			std::ifstream input_mat_file(filepath, std::ios::binary);
			ASSERT_TRUE(input_mat_file.is_open(), ".jmat file opened");

			input_mat_file.read(header, 5);
			bool valid_header = strcmp(header, ".jmat") == 0;

			if (valid_header)
			{
				MaterialData mat_data = {};
				input_mat_file.read(reinterpret_cast<char*>(&mat_data), sizeof(mat_data));
				new_material = material_deserialize(mat_data);
			}

			// Texture images
			filename = entry.path().stem().filename().string();
			sprintf_s(filepath, "%s\\%s.png", g_materials_dir_path, filename.c_str());

			Texture color_texture = texture_load_from_filepath(filepath);
			Texture* color_texture_prt = j_array_add(&g_textures, color_texture);
			new_material.color_texture = color_texture_prt;

			Texture* specular_texture_ptr = nullptr;
			sprintf_s(filepath, "%s\\%s_specular.png", g_materials_dir_path, filename.c_str());
			std::ifstream spec_file(filepath);

			if (spec_file.good())
			{
				Texture specular_texture = texture_load_from_filepath(filepath);
				specular_texture_ptr = j_array_add(&g_textures, specular_texture);
				new_material.specular_texture = specular_texture_ptr;
			}

			if (!valid_header) new_material = material_init(color_texture_prt, specular_texture_ptr);
			Material* new_material_prt = j_array_add(&g_materials, new_material);

			// Add new material as data
			bool not_found = g_mat_data_map.find(color_texture.file_name) == g_mat_data_map.end();

			if (not_found)
			{
				char* new_str = j_strings_add(&g_material_names, color_texture.file_name);
				g_mat_data_map.insert(std::make_pair(new_str, materials_count));
				g_mat_data_map_inverse.insert(std::make_pair(materials_count, new_str));
				g_materials_index_map[new_str] = materials_count;
				++materials_count;
			}
		}

		// Add material ids to .data
		sprintf_s(filepath, "%s\\%s", g_materials_dir_path, mat_data_filename);
		std::ofstream mat_data_file(filepath, std::ios::binary | std::ios::trunc);
		mat_data_file.write(".data", 5);
		mat_data_file.write(reinterpret_cast<char*>(&materials_count), sizeof(materials_count));

		for (const auto& mat_data : g_mat_data_map)
		{
			MaterialIdData new_data = {
				.material_name = "",
				.material_id = mat_data.second,
			};
			strcpy_s(new_data.material_name, mat_data.first);
			mat_data_file.write(reinterpret_cast<char*>(&new_data), sizeof(new_data));
		}
	}

	glfwSetWindowSize(g_window, g_user_settings.window_size_px[0], g_user_settings.window_size_px[1]);

	// Init framebuffer
	{
		{
			glGenFramebuffers(1, &g_scene_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, g_scene_framebuffer);

			glGenTextures(1, &g_scene_framebuffer_texture);
			glBindTexture(GL_TEXTURE_2D, g_scene_framebuffer_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_game_metrics.scene_width_px, g_game_metrics.scene_height_px, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_scene_framebuffer_texture, 0);

			unsigned int rbo;
			glGenRenderbuffers(1, &rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, g_game_metrics.scene_width_px, g_game_metrics.scene_height_px);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

			ASSERT_TRUE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer successfull");
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		{
			glGenFramebuffers(1, &g_editor_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, g_editor_framebuffer);

			glGenTextures(1, &g_editor_framebuffer_texture);
			glBindTexture(GL_TEXTURE_2D, g_editor_framebuffer_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_game_metrics.scene_width_px, g_game_metrics.scene_height_px, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_editor_framebuffer_texture, 0);

			unsigned int rbo;
			glGenRenderbuffers(1, &rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, g_game_metrics.scene_width_px, g_game_metrics.scene_height_px);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

			ASSERT_TRUE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer successfull");
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	g_use_linear_texture_filtering = false;
	g_generate_texture_mipmaps = false;
	int font_height_px = normalize_value(debug_font_vh, 100.0f, g_game_metrics.game_height_px);
	load_font(&g_debug_font, font_height_px, g_debug_font_path);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);

	load_scene();

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
						.material = j_array_get(&g_materials, 0),
						.mesh_type = E_Primitive_Plane,
						.uv_multiplier = 1.0f,
					};
					s64 new_mesh_index = add_new_mesh(new_plane);
					select_object_index(ObjectType::Primitive, new_mesh_index);
				}

				if (ImGui::Button("Add Cube"))
				{
					Mesh new_cube = {
						.translation = vec3(0),
						.rotation = vec3(0),
						.scale = vec3(1.0f),
						.material = j_array_get(&g_materials, 0),
						.mesh_type = E_Primitive_Cube,
						.uv_multiplier = 1.0f,
					};
					s64 new_mesh_index = add_new_mesh(new_cube);
					select_object_index(ObjectType::Primitive, new_mesh_index);
				}

				if (ImGui::Button("Add pointlight"))
				{
					Pointlight new_pointlight = pointlight_init();
					s64 new_light_index = add_new_pointlight(new_pointlight);
					select_object_index(ObjectType::Pointlight, new_light_index);
				}

				if (ImGui::Button("Add spotlight"))
				{
					Spotlight new_spotlight = spotlight_init();
					s64 new_light_index = add_new_spotlight(new_spotlight);
					select_object_index(ObjectType::Spotlight, new_light_index);
				}

				if (g_selected_object.type == ObjectType::Primitive)
				{
					char selected_mesh_str[24];
					sprintf_s(selected_mesh_str, "Mesh index: %lld", g_selected_object.selection_index);
					ImGui::Text(selected_mesh_str);

					Mesh* selected_mesh_ptr = (Mesh*)get_selected_object_ptr();

					ImGui::Text("Mesh properties");
					ImGui::InputFloat3("Translation", &selected_mesh_ptr->translation[0], "%.2f");
					ImGui::InputFloat3("Rotation", &selected_mesh_ptr->rotation[0], "%.2f");
					ImGui::InputFloat3("Scale", &selected_mesh_ptr->scale[0], "%.2f");

					ImGui::Text("Select material");
					ImGui::Image((ImTextureID)selected_mesh_ptr->material->color_texture->gpu_id, ImVec2(128, 128));

					char* texture_names_ptr = (char*)g_material_names.data;

					if (ImGui::Combo("Material", &g_selected_texture_item, texture_names_ptr, g_material_names.strings_count))
					{
						selected_mesh_ptr->material = j_array_get(&g_materials, g_selected_texture_item);
					}

					ImGui::InputFloat("UV mult", &selected_mesh_ptr->uv_multiplier, 0, 0, "%.2f");

					ImGui::Text("Material properties");
					ImGui::InputFloat("Specular mult", &selected_mesh_ptr->material->specular_mult, 0, 0, "%.3f");
					ImGui::InputFloat("Material shine", &selected_mesh_ptr->material->shininess, 0, 0, "%.3f");
				}
				else if (g_selected_object.type == ObjectType::Pointlight)
				{
					char selected_light_str[24];
					sprintf_s(selected_light_str, "Light index: %lld", g_selected_object.selection_index);
					ImGui::Text(selected_light_str);

					Pointlight* selected_light_ptr = (Pointlight*)get_selected_object_ptr();

					ImGui::Text("Pointight properties");
					ImGui::InputFloat3("Light pos", &selected_light_ptr->position[0], "%.3f");
					ImGui::ColorEdit3("Color Picker", &selected_light_ptr->diffuse[0], 0);
					ImGui::InputFloat("Light specular", &selected_light_ptr->specular, 0, 0, "%.3f");
					ImGui::InputFloat("Light intensity", &selected_light_ptr->intensity, 0, 0, "%.3f");
				}
				else if (g_selected_object.type == ObjectType::Spotlight)
				{
					Spotlight* selected_spotlight_ptr = (Spotlight*)get_selected_object_ptr();

					ImGui::Text("Spotlight properties");
					ImGui::ColorEdit3("Color", &selected_spotlight_ptr->diffuse[0], 0);
					ImGui::InputFloat3("Position", &selected_spotlight_ptr->position[0], "%.2f");
					ImGui::InputFloat3("Direction", &selected_spotlight_ptr->rotation[0], "%.2f");
					ImGui::InputFloat("Cutoff", &selected_spotlight_ptr->cutoff, 0, 0, "%.2f");
					ImGui::InputFloat("Outer cutoff", &selected_spotlight_ptr->outer_cutoff, 0, 0, "%.2f");
				}

				ImGui::Text("Editor settings");
				ImGui::InputFloat("Transform clip", &g_user_settings.transform_clip, 0, 0, "%.2f");
				ImGui::InputFloat("Rotation clip", &g_user_settings.transform_rotation_clip, 0, 0, "%.2f");

				ImGui::Text("Scene settings");
				ImGui::ColorEdit3("Global ambient", &g_user_settings.world_ambient[0], 0);

				ImGui::Text("Game window");
				ImGui::InputInt2("Screen width px", &g_user_settings.window_size_px[0]);

				if (ImGui::Button("Apply changes"))
				{
					glfwSetWindowSize(g_window, g_user_settings.window_size_px[0], g_user_settings.window_size_px[1]);
				}

				ImGui::Text("Post processing");
				ImGui::Checkbox("Inverse", &g_inverse_color);
				ImGui::Checkbox("Blur", &g_blur_effect);
				ImGui::InputFloat("Blur amount", &g_blur_effect_amount, 0, 0, "%.2f");

				ImGui::End();
			}
		}

		// Register inputs
		{
			int key_state;
			int buttons_count = sizeof(g_inputs) / sizeof(ButtonState);
			g_frame_data.mouse_clicked = false;

			if (glfwGetKey(g_window, g_inputs.as_struct.esc.key) == GLFW_PRESS) glfwSetWindowShouldClose(g_window, true);

			key_state = glfwGetMouseButton(g_window, g_inputs.as_struct.mouse1.key);
			g_inputs.as_struct.mouse1.pressed = !g_inputs.as_struct.mouse1.is_down && key_state == GLFW_PRESS;
			g_inputs.as_struct.mouse1.is_down = key_state == GLFW_PRESS;

			key_state = glfwGetMouseButton(g_window, g_inputs.as_struct.mouse2.key);
			g_inputs.as_struct.mouse2.pressed = !g_inputs.as_struct.mouse2.is_down && key_state == GLFW_PRESS;
			g_inputs.as_struct.mouse2.is_down = key_state == GLFW_PRESS;

			for (int i = 2; i < buttons_count - 1; i++) // Skip first two mouse buttons
			{
				ButtonState* button = &g_inputs.as_array[i];
				set_button_state(g_window, button);
			}
		}

		// Logic
		g_game_metrics.prev_frame_game_time = g_game_metrics.game_time;
		g_game_metrics.game_time = glfwGetTime();
		g_frame_data.deltatime = (g_game_metrics.game_time - g_game_metrics.prev_frame_game_time);

		double xpos, ypos;
		glfwGetCursorPos(g_window, &xpos, &ypos);

		g_frame_data.mouse_x = xpos;
		g_frame_data.mouse_y = ypos;

		s64 current_game_second = (s64)g_game_metrics.game_time;

		if (0 < current_game_second - g_game_metrics.fps_prev_second)
		{
			g_game_metrics.fps = g_game_metrics.fps_frames;
			g_game_metrics.fps_frames = 0;
			g_game_metrics.fps_prev_second = current_game_second;
		}

		// Save scene
		if (g_inputs.as_struct.left_ctrl.is_down && g_inputs.as_struct.s.pressed) save_all();

		// Get tranformation mode
		if (!g_camera_move_mode) g_transform_mode.mode = get_curr_transformation_mode();

		// Mouse 1 => select object
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

				s64 object_types_count = 3;
				ObjectType select_types[] = { ObjectType::Primitive, ObjectType::Pointlight, ObjectType::Spotlight };
				s64 object_index[3] = {-1, -1, -1};
				f32 closest_dist[3] = {};

				object_index[0] = get_mesh_selection_index(g_scene_meshes, &closest_dist[0], ray_origin, ray_direction);
				object_index[1] = get_pointlight_selection_index(g_scene_pointlights, &closest_dist[1], ray_origin, ray_direction);
				object_index[2] = get_spotlight_selection_index(g_scene_spotlights, &closest_dist[2], ray_origin, ray_direction);

				ObjectType selected_type = ObjectType::None;
				s64 closest_obj_index = -1;
				f32 closest_dist_result = std::numeric_limits<float>::max();
				bool got_selection = false;

				for (int i = 0; i < object_types_count; i++)
				{
					s64 current_index = object_index[i];
					f32 current_dist = closest_dist[i];

					if (current_index != -1 && current_dist < closest_dist_result)
					{
						closest_dist_result = current_dist;
						closest_obj_index = current_index;
						selected_type = select_types[i];
						got_selection = true;
					}
				}

				if (got_selection) select_object_index(selected_type, closest_obj_index);
				else deselect_selection();
			}
		}

		bool camera_mode_start = g_inputs.as_struct.mouse2.pressed
			&& clicked_scene_space((int)g_frame_data.mouse_x, (int)g_frame_data.mouse_y);

		// Check camero move mode
		if (camera_mode_start) g_camera_move_mode = true;
		else if (!g_inputs.as_struct.mouse2.is_down) g_camera_move_mode = false;

		// Handle camera mode mode
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
		else glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		// Duplicate / Delete object
		if (has_object_selection())
		{
			if (g_inputs.as_struct.del.pressed) delete_selected_object();
			else if (g_inputs.as_struct.left_ctrl.is_down && g_inputs.as_struct.d.pressed) duplicate_selected_object();
		}

		// Check transformation mode start
		if (has_object_selection()
			&& (g_inputs.as_struct.x.pressed || g_inputs.as_struct.z.pressed || g_inputs.as_struct.c.pressed))
		{
			g_transform_mode.is_active = try_init_transform_mode();
		}
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

			Mesh* selected_mesh_ptr = (Mesh*)get_selected_object_ptr();

			bool intersection = calculate_plane_ray_intersection(
				g_normal_for_ray_intersect,
				selected_mesh_ptr->translation,
				g_scene_camera.position,
				g_used_transform_ray,
				intersection_point);

			g_debug_plane_intersection = intersection_point;

			if (intersection && g_transform_mode.mode == TransformMode::Translate)
			{
				glm::vec3 travel_dist = intersection_point - g_prev_intersection;
				vec3_add_for_axis(g_new_translation, travel_dist, g_transform_mode.axis);
				g_prev_intersection = intersection_point;

				if (0.0f < g_user_settings.transform_clip) selected_mesh_ptr->translation = clip_vec3(g_new_translation, g_user_settings.transform_clip);
				else selected_mesh_ptr->translation = g_new_translation;
			}
			else if (intersection && g_transform_mode.mode == TransformMode::Scale)
			{
				glm::mat4 rotation_mat4 = get_rotation_matrix(selected_mesh_ptr->rotation);
				glm::vec3 used_normal = get_normal_for_axis(g_transform_mode.axis);

				used_normal = rotation_mat4 * glm::vec4(used_normal, 1.0f);
				glm::vec3 point_on_scale_plane = closest_point_on_plane(g_debug_plane_intersection, selected_mesh_ptr->translation, used_normal);
				g_point_on_scale_plane = point_on_scale_plane;

				// Reverse the rotation by applying the inverse rotation matrix to the vector
				glm::vec3 reversedVector = glm::inverse(rotation_mat4) * glm::vec4(point_on_scale_plane, 1.0f);
				glm::vec3 reversedVector2 = glm::inverse(rotation_mat4) * glm::vec4(g_prev_point_on_scale_plane, 1.0f);

				glm::vec3 travel_dist = reversedVector - reversedVector2;
				g_prev_point_on_scale_plane = point_on_scale_plane;

				vec3_add_for_axis(g_new_scale, travel_dist, g_transform_mode.axis);

				if (0.0f < g_user_settings.transform_clip) selected_mesh_ptr->scale = clip_vec3(g_new_scale, g_user_settings.transform_clip);
				else selected_mesh_ptr->scale = g_new_scale;

				if (selected_mesh_ptr->scale.x < 0.01f) selected_mesh_ptr->scale.x = 0.01f;
				if (selected_mesh_ptr->scale.y < 0.01f) selected_mesh_ptr->scale.y = 0.01f;
				if (selected_mesh_ptr->scale.z < 0.01f) selected_mesh_ptr->scale.z = 0.01f;
			}
			else if (intersection && g_transform_mode.mode == TransformMode::Rotate)
			{
				auto new_line = g_debug_plane_intersection - selected_mesh_ptr->translation;

				if (0.15f < glm::length(new_line))
				{
					glm::vec3 prev_vec = glm::normalize(g_prev_intersection - selected_mesh_ptr->translation);
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
								selected_mesh_ptr->rotation = clip_vec3(g_new_rotation, g_user_settings.transform_rotation_clip);
							}
							else
							{
								selected_mesh_ptr->rotation = g_new_rotation;
							}

							selected_mesh_ptr->rotation.x = float_modulus_operation(selected_mesh_ptr->rotation.x, 360.0f);
							selected_mesh_ptr->rotation.y = float_modulus_operation(selected_mesh_ptr->rotation.y, 360.0f);
							selected_mesh_ptr->rotation.z = float_modulus_operation(selected_mesh_ptr->rotation.z, 360.0f);
						}
					}
				}
			}
		}

		// -------------
		// Draw OpenGL

		// Scene framebuffer
		{
			glBindFramebuffer(GL_FRAMEBUFFER, g_scene_framebuffer);
			glEnable(GL_DEPTH_TEST);
			glClearColor(0.34f, 0.44f, 0.42f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			for (int i = 0; i < g_scene_meshes.items_count; i++)
			{
				Mesh plane = *j_array_get(&g_scene_meshes, i);
				draw_mesh(&plane);
			}
		}

		// Editor framebuffer
		{
			glBindFramebuffer(GL_FRAMEBUFFER, g_editor_framebuffer);
			glEnable(GL_DEPTH_TEST);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Transformation mode debug lines
			if (has_object_selection() && g_transform_mode.is_active)
			{
				// Scale mode
				if (g_transform_mode.mode == TransformMode::Scale)
				{

				}

				// Translate mode
				if (g_transform_mode.mode == TransformMode::Translate)
				{

				}

				// Rotate mode
				if (g_transform_mode.mode == TransformMode::Rotate)
				{
					Mesh* selected_mesh_ptr = (Mesh*)get_selected_object_ptr();
					draw_line(g_debug_plane_intersection, selected_mesh_ptr->translation, glm::vec3(1.0f, 1.0f, 0.0f), 2.0f, 2.0f);
				}
			}

			// Coordinate lines
			draw_line(glm::vec3(-1000.0f, 0.0f, 0.0f), glm::vec3(1000.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 1.0f);
			draw_line(glm::vec3(0.0f, -1000.0f, 0.0f), glm::vec3(0.0f, 1000.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, 1.0f);
			draw_line(glm::vec3(0.0f, 0.0f, -1000.0f), glm::vec3(0.0f, 0.0f, 1000.0f), glm::vec3(0.0f, 0.0f, 1.0f), 1.0f, 1.0f);

			// Draw selection
			if (has_object_selection())
			{
				if (g_selected_object.type == ObjectType::Primitive)
				{
					Mesh* selected_mesh = (Mesh*)get_selected_object_ptr();
					draw_mesh_wireframe(selected_mesh, glm::vec3(1.0f));
					draw_selection_arrows(selected_mesh->translation);
				}
				else if (g_selected_object.type == ObjectType::Pointlight)
				{
					Pointlight* selected_light = (Pointlight*)get_selected_object_ptr();
					Mesh as_cube = {};
					as_cube.mesh_type = E_Primitive_Cube;
					as_cube.scale = vec3(0.35f);
					as_cube.translation = selected_light->position;
					draw_mesh_wireframe(&as_cube, selected_light->diffuse);
					draw_selection_arrows(selected_light->position);
				}
				else if (g_selected_object.type == ObjectType::Spotlight)
				{
					Spotlight* selected_light = (Spotlight*)get_selected_object_ptr();
					Mesh as_cube = {};
					as_cube.mesh_type = E_Primitive_Cube;
					as_cube.scale = vec3(0.35f);
					as_cube.translation = selected_light->position;
					draw_mesh_wireframe(&as_cube, selected_light->diffuse);
					draw_selection_arrows(selected_light->position);
				}
			}

			// Draw billboards
			{
				// Pointlights
				for (int i = 0; i < g_scene_pointlights.items_count; i++)
				{
					auto light = *j_array_get(&g_scene_pointlights, i);
					draw_billboard(light.position, pointlight_texture, 0.5f);
				}

				// Spotlights
				for (int i = 0; i < g_scene_spotlights.items_count; i++)
				{
					auto spotlight = *j_array_get(&g_scene_spotlights, i);
					draw_billboard(spotlight.position, spotlight_texture, 0.5f);
					vec3 sp_dir = get_spotlight_dir(spotlight);
					draw_line(spotlight.position, spotlight.position + sp_dir, spotlight.diffuse, 2.0f, 2.0f);
				}
			}

			// Click ray
			auto debug_click_end = g_debug_click_camera_pos + g_debug_click_ray_normal * 20.0f;
			draw_line(g_debug_click_camera_pos, debug_click_end, glm::vec3(1.0f, 0.2f, 1.0f), 1.0f, 2.0f);

		}

		// -------------------
		// Draw framebuffers

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(g_scene_framebuffer_shader);
		glBindVertexArray(g_scene_framebuffer_vao);

		unsigned int inversion_loc = glGetUniformLocation(g_scene_framebuffer_shader, "use_inversion");
		unsigned int blur_loc = glGetUniformLocation(g_scene_framebuffer_shader, "use_blur");
		unsigned int blur_amount_loc = glGetUniformLocation(g_scene_framebuffer_shader, "blur_amount");

		glUniform1i(inversion_loc, g_inverse_color);
		glUniform1i(blur_loc, g_blur_effect);
		glUniform1f(blur_amount_loc, g_blur_effect_amount);
		glBindTexture(GL_TEXTURE_2D, g_scene_framebuffer_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glUniform1i(inversion_loc, false);
		glUniform1i(blur_loc, false);
		glBindTexture(GL_TEXTURE_2D, g_editor_framebuffer_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Print debug info
		{
			char debug_str[256];

			sprintf_s(debug_str, "FPS: %d", g_game_metrics.fps);
			append_ui_text(&g_debug_font, debug_str, 0.5f, 100.0f);

			float display_deltatime = g_frame_data.deltatime * 1000;
			sprintf_s(debug_str, "Delta: %.2fms", display_deltatime);
			append_ui_text(&g_debug_font, debug_str, 4.5f, 100.0f);

			sprintf_s(debug_str, "Frames: %lu", g_game_metrics.frames);
			append_ui_text(&g_debug_font, debug_str, 10.5f, 100.0f);

			sprintf_s(debug_str, "Draw calls: %d", ++g_frame_data.draw_calls);
			append_ui_text(&g_debug_font, debug_str, 17.0f, 100.0f);

			sprintf_s(debug_str, "Camera X=%.2f Y=%.2f Z=%.2f", g_scene_camera.position.x, g_scene_camera.position.y, g_scene_camera.position.z);
			append_ui_text(&g_debug_font, debug_str, 0.5f, 99.0f);

			sprintf_s(debug_str, "Meshes %lld / %lld", g_scene_meshes.items_count, g_scene_meshes.max_items);
			append_ui_text(&g_debug_font, debug_str, 0.5f, 98.0f);

			sprintf_s(debug_str, "Pointlights %lld / %lld", g_scene_pointlights.items_count, g_scene_pointlights.max_items);
			append_ui_text(&g_debug_font, debug_str, 0.5f, 97.0f);

			sprintf_s(debug_str, "Spotlights %lld / %lld", g_scene_spotlights.items_count, g_scene_spotlights.max_items);
			append_ui_text(&g_debug_font, debug_str, 0.5f, 96.0f);

			char* t_mode = nullptr;
			const char* tt = "Translate";
			const char* tr = "Rotate";
			const char* ts = "Scale";
			const char* transform_mode_debug_str_format = "Transform mode: %s";

			if (g_transform_mode.mode == TransformMode::Translate) t_mode = const_cast<char*>(tt);
			if (g_transform_mode.mode == TransformMode::Rotate)	t_mode = const_cast<char*>(tr);
			if (g_transform_mode.mode == TransformMode::Scale)		t_mode = const_cast<char*>(ts);

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
