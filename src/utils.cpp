#include "utils.h"

#include <array>
#include <iostream>
#include <fstream>
#include <glm/gtc/matrix_inverse.hpp>

using namespace glm;

bool str_trim_file_ext(char* str)
{
	char* last_dot = strrchr(str, '.');
	if (last_dot == nullptr) return false;
	*last_dot = '\0';
	return true;
}

float normalize_screen_px_to_ndc(int value, int max)
{
	float this1 = static_cast<float>(value) / static_cast<float>(max);
	float this2 = 2.0f * this1;
	float res = -1.0f + this2;
	return res;
}

float normalize_value(float value, float src_max, float dest_max)
{
	// Assume 0.0 is min value
	float intermediate = value / src_max;
	float result = dest_max * intermediate;
	return result;
}

glm::vec3 closest_point_on_plane(const glm::vec3& point1, const glm::vec3& pointOnPlane, const glm::vec3& planeNormal) {
	// Calculate the vector from point1 to the point on the plane
	glm::vec3 vectorToPoint = point1 - pointOnPlane;

	// Calculate the projection of vectorToPoint onto the plane's normal
	glm::vec3 projection = glm::dot(vectorToPoint, planeNormal) / glm::dot(planeNormal, planeNormal) * planeNormal;

	// Calculate the closest point on the plane to point1
	glm::vec3 point2 = pointOnPlane + projection;

	return point2;
}

std::array<glm::vec3, 2> get_axis_xor_normals(s64 axis)
{
	std::array<glm::vec3, 2> result{};

	auto normal_vector_x = glm::vec3(1.0f, 0, 0);
	auto normal_vector_y = glm::vec3(0, 1.0f, 0);
	auto normal_vector_z = glm::vec3(0, 0, 1.0f);

	if (axis == E_Axis_X)
	{
		result[0] = normal_vector_y;
		result[1] = normal_vector_z;
	}
	else if (axis == E_Axis_Y)
	{
		result[0] = normal_vector_x;
		result[1] = normal_vector_z;
	}
	else if (axis == E_Axis_Z)
	{
		result[0] = normal_vector_x;
		result[1] = normal_vector_y;
	}

	return result;
}

std::array<float, 2> get_plane_axis_xor_rotations(s64 axis, Mesh* mesh)
{
	std::array<float, 2> result{};

	auto rotation_x = mesh->transforms.rotation.x;
	auto rotation_y = mesh->transforms.rotation.y;
	auto rotation_z = mesh->transforms.rotation.z;

	if (axis == E_Axis_X)
	{
		result[0] = rotation_y;
		result[1] = rotation_z;
	}
	else if (axis == E_Axis_Y)
	{
		result[0] = rotation_x;
		result[1] = rotation_z;
	}
	else if (axis == E_Axis_Z)
	{
		result[0] = rotation_x;
		result[1] = rotation_y;
	}

	return result;
}

glm::vec3 get_normal_for_axis(s64 axis)
{
	switch (axis)
	{
	case E_Axis_X: return glm::vec3(1.0f, 0, 0);
	case E_Axis_Y: return glm::vec3(0, 1.0f, 0);
	case E_Axis_Z: return glm::vec3(0, 0, 1.0f);
	}
}

glm::vec3 get_vec_for_smallest_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements)
{
	float smallest = std::numeric_limits<float>::max();
	auto smallest_vec = glm::vec3(0);

	for (int i = 0; i < elements; i++)
	{
		glm::vec3 current = normals[i];
		float dot = glm::dot(current, direction_compare);

		if (dot < smallest)
		{
			smallest = dot;
			smallest_vec = current;
		}
	}

	return smallest_vec;
}

glm::vec3 get_vec_for_largest_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements)
{
	float max = std::numeric_limits<float>::min();
	auto max_vec = glm::vec3(0);

	for (int i = 0; i < elements; i++)
	{
		glm::vec3 current = normals[i];
		float dot = glm::dot(current, direction_compare);

		if (max < dot)
		{
			max = dot;
			max_vec = current;
		}
	}

	return max_vec;
}

void vec3_add_for_axis(glm::vec3& for_addition, glm::vec3 to_add, s64 axis)
{
	if (axis == E_Axis_X)
	{
		for_addition.x += to_add.x;
	}
	if (axis == E_Axis_Y)
	{
		for_addition.y += to_add.y;
	}
	if (axis == E_Axis_Z)
	{
		for_addition.z += to_add.z;
	}
}

glm::vec3 get_plane_middle_point(Mesh mesh)
{
	glm::mat4 rotation = get_rotation_matrix(mesh.transforms.rotation);
	glm::vec3 scale_rotated = rotation * glm::vec4(mesh.transforms.scale.x, mesh.transforms.scale.y, mesh.transforms.scale.z, 1.0f);
	scale_rotated = scale_rotated / 2.0f;
	scale_rotated.y = 0.0f;
	glm::vec3 result = mesh.transforms.translation + scale_rotated;
	return result;
}

void get_axis_xor(s64 axis, s64 xor_axises[])
{
	if (axis == E_Axis_X)
	{
		xor_axises[0] = E_Axis_Y;
		xor_axises[1] = E_Axis_Z;
	}
	else if (axis == E_Axis_Y)
	{
		xor_axises[0] = E_Axis_X;
		xor_axises[1] = E_Axis_Z;
	}
	else if (axis == E_Axis_Z)
	{
		xor_axises[0] = E_Axis_X;
		xor_axises[1] = E_Axis_Y;
	}
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

Transforms transforms_init()
{
	Transforms t = {
		.translation = vec3(0),
		.rotation = vec3(0),
		.scale = vec3(1)
	};
	return t;
}

vec3 get_spotlight_dir(Spotlight spotlight)
{
	vec3 spot_dir = vec3(0, -1.0f, 0);
	glm::mat4 rotation_mat = get_rotation_matrix(spotlight.transforms.rotation);
	spot_dir = rotation_mat * vec4(spot_dir, 1.0f);
	return glm::normalize(spot_dir);
}

Material material_init(Texture* color_ptr, Texture* specular_ptr)
{
	Material material = {
		.color_texture = color_ptr,
		.specular_texture = specular_ptr,
		.specular_mult = 1.0f,
		.shininess = 32.0f,
	};
	return material;
}

Framebuffer framebuffer_init()
{
	Framebuffer buffer = {
		.id = 0,
		.texture_gpu_id = 0,
		.renderbuffer = 0,
	};
	return buffer;
}

Spotlight spotlight_init()
{
	Spotlight sp = {
		.shadow_map = framebuffer_init(),
		.transforms = transforms_init(),
		.diffuse = glm::vec3(1),
		.specular = 2.0f,
		.range = 5.0f,
		.cutoff = 0.90f,
		.outer_cutoff = 0.85,
	};
	return sp;
}

Pointlight pointlight_init()
{
	Pointlight p_light = {
		.transforms = transforms_init(),
		.diffuse = glm::vec3(1.0f),
		.range = 6.0f,
		.specular = 1.0f,
		.intensity = 1.0f
	};
	return p_light;
}

GameCamera scene_camera_init(float horizontal_fov)
{
	GameCamera cam = {
		.position = vec3(0),
		.front_vec = vec3(1,0,0),
		.up_vec = vec3(0,1,0),
		.yaw = 0.0f,
		.pitch = 0.0f,
		.fov = 60.0f,
		.aspect_ratio_horizontal = horizontal_fov,
		.look_sensitivity = 0.1f,
		.move_speed = 5.0f,
		.near_clip = 0.1f,
		.far_clip = 100.0f,
	};
	return cam;
}

float get_line_distance_width(glm::vec3 line_start, glm::vec3 line_end, glm::vec3 view_pos, float thickness)
{
	glm::vec3 line_midpoint = (line_start + line_end) / 2.0f;
	float line_distance = glm::length(view_pos - line_midpoint);
	float scaling = 10.0f / line_distance;

	float line_width = thickness * scaling;
	if (1.0f < line_width) line_width = 1.0f;

	return line_width;
}

SimpleShader simple_shader_init()
{
	SimpleShader shader = {
		.id = 0,
		.vao = 0,
		.vbo = 0,
	};
	return shader;
}