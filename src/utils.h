#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "j_buffers.h"
#include "globals.h"
#include "structs.h"
#include "types.h"

glm::mat4 get_projection_matrix();

glm::mat4 get_view_matrix();

Framebuffer init_spotlight_shadow_map();

glm::mat4 get_spotlight_light_space_matrix(Spotlight spotlight);

inline float vw_into_screen_px(float value, float screen_width_px)
{
	return (float)screen_width_px * value * 0.01f;
}

inline float vh_into_screen_px(float value, float screen_height_px)
{
	return (float)screen_height_px * value * 0.01f;
}

inline void null_terminate_string(char* string, int str_length)
{
	string[str_length] = '\0';
}

inline float clamp_float(float value, float min, float max)
{
	if (value < min)
	{
		return min;
	}

	if (max < value)
	{
		return max;
	}

	return value;
}

inline float float_modulus_operation(float value, float divisor)
{
	return value = value - std::floor(value / divisor) * divisor;
}

inline glm::vec3 clip_vec3(glm::vec3 vec3, float clip_amount)
{
	float rounded_x = roundf(vec3.x / clip_amount) * clip_amount;
	float rounded_y = roundf(vec3.y / clip_amount) * clip_amount;
	float rounded_z = roundf(vec3.z / clip_amount) * clip_amount;
	return glm::vec3(rounded_x, rounded_y, rounded_z);
}

glm::mat4 get_model_matrix(Mesh* mesh);

glm::mat4 get_rotation_matrix(glm::vec3 rotation);

inline float get_vec3_val_by_axis(glm::vec3 vec, Axis axis)
{
	if (axis == Axis::X) return vec.x;
	if (axis == Axis::Y) return vec.y;
	if (axis == Axis::Z) return vec.z;
	return 0.0f;
}

bool str_trim_file_ext(char* str);

char* str_get_file_ext(char* str);

float normalize_screen_px_to_ndc(int value, int max);

float normalize_value(float value, float src_max, float dest_max);

void get_axis_xor(Axis axis, Axis xor_axises[]);

glm::vec3 closest_point_on_plane(const glm::vec3& point1, const glm::vec3& pointOnPlane, const glm::vec3& planeNormal);

std::array<glm::vec3, 2> get_axis_xor_normals(Axis axis);

std::array<float, 2> get_plane_axis_xor_rotations(Axis axis, Mesh* mesh);

glm::vec3 get_normal_for_axis(Axis axis);

glm::vec3 get_vec_for_smallest_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements);

glm::vec3 get_vec_for_largest_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements);

glm::vec3 get_vec_for_largest_abs_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements);

void vec3_add_for_axis(glm::vec3& for_addition, glm::vec3 to_add, Axis axis);

glm::vec3 get_plane_middle_point(Mesh mesh);

bool calculate_plane_ray_intersection(
	glm::vec3 plane_normal,
	glm::vec3 point_in_plane,
	glm::vec3 ray_origin,
	glm::vec3 ray_direction,
	glm::vec3& result);

Transforms transforms_init();

glm::vec3 get_spotlight_dir(Spotlight spotlight);

Material material_init(Texture* color_ptr, Texture* specular_ptr);

Spotlight spotlight_init();

Pointlight pointlight_init();

GameCamera scene_camera_init(float horizontal_fov);

float get_line_distance_width(glm::vec3 line_start, glm::vec3 line_end, glm::vec3 view_pos, float thickness);

SimpleShader simple_shader_init();

Framebuffer framebuffer_init();

void print_debug_texts();

void resize_windows_area_settings(s64 width_px, s64 height_px);

void init_framebuffer_resize(unsigned int* framebuffer_texture_id, unsigned int* renderbuffer_id);

void init_memory_buffers();

glm::vec3 get_camera_ray_from_scene_px(int x, int y);

bool is_primitive(ObjectType type);

PostProcessingSettings post_processings_init();
