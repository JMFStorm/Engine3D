#pragma once

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include "j_buffers.h"
#include "types.h"

using namespace glm;

bool str_trim_file_ext(char* str);

float normalize_screen_px_to_ndc(int value, int max);

float normalize_value(float value, float src_max, float dest_max);

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

inline glm::mat4 get_model_matrix(Mesh* mesh)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, mesh->translation);

	glm::quat quaternionX = glm::angleAxis(glm::radians(mesh->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat quaternionY = glm::angleAxis(glm::radians(mesh->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat quaternionZ = glm::angleAxis(glm::radians(mesh->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::quat finalRotation = quaternionY * quaternionX * quaternionZ;

	glm::mat4 rotation_matrix = glm::mat4_cast(finalRotation);
	model = model * rotation_matrix;
	model = glm::scale(model, mesh->scale);
	return model;
}

inline glm::mat4 get_rotation_matrix(glm::vec3 rotation)
{
	glm::quat quaternionX = glm::angleAxis(glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat quaternionY = glm::angleAxis(glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat quaternionZ = glm::angleAxis(glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::quat finalRotation = quaternionY * quaternionX * quaternionZ;
	glm::mat4 rotation_matrix = glm::mat4_cast(finalRotation);
	return rotation_matrix;
}

inline float get_vec3_val_by_axis(glm::vec3 vec, s64 axis)
{
	if (axis == E_Axis_X) return vec.x;
	if (axis == E_Axis_Y) return vec.y;
	if (axis == E_Axis_Z) return vec.z;
	return 0.0f;
}

void get_axis_xor(s64 axis, s64 xor_axises[]);

glm::vec3 closest_point_on_plane(const glm::vec3& point1, const glm::vec3& pointOnPlane, const glm::vec3& planeNormal);

std::array<glm::vec3, 2> get_axis_xor_normals(s64 axis);

std::array<float, 2> get_plane_axis_xor_rotations(s64 axis, Mesh* mesh);

glm::vec3 get_normal_for_axis(s64 axis);

glm::vec3 get_vec_for_smallest_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements);

glm::vec3 get_vec_for_largest_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements);

void vec3_add_for_axis(glm::vec3& for_addition, glm::vec3 to_add, s64 axis);

glm::vec3 get_plane_middle_point(Mesh mesh);

bool calculate_plane_ray_intersection(
	glm::vec3 plane_normal,
	glm::vec3 point_in_plane,
	glm::vec3 ray_origin,
	glm::vec3 ray_direction,
	glm::vec3& result);

vec3 get_spotlight_dir(Spotlight spotlight);

Material material_init(Texture* color_ptr, Texture* specular_ptr);

Spotlight spotlight_init();

Pointlight pointlight_init();

GameCamera scene_camera_init(float horizontal_fov);

float get_line_distance_width(glm::vec3 line_start, glm::vec3 line_end, glm::vec3 view_pos, float thickness);

SimpleShader simple_shader_init();
