#include "utils.h"

#include <array>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <GL/glew.h>

#include "j_assert.h"

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

std::array<glm::vec3, 2> get_axis_xor_normals(Axis axis)
{
	std::array<glm::vec3, 2> result{};

	auto normal_vector_x = glm::vec3(1.0f, 0, 0);
	auto normal_vector_y = glm::vec3(0, 1.0f, 0);
	auto normal_vector_z = glm::vec3(0, 0, 1.0f);

	if (axis == Axis::X)
	{
		result[0] = normal_vector_y;
		result[1] = normal_vector_z;
	}
	else if (axis == Axis::Y)
	{
		result[0] = normal_vector_x;
		result[1] = normal_vector_z;
	}
	else if (axis == Axis::Z)
	{
		result[0] = normal_vector_x;
		result[1] = normal_vector_y;
	}

	return result;
}

std::array<float, 2> get_plane_axis_xor_rotations(Axis axis, Mesh* mesh)
{
	std::array<float, 2> result{};

	auto rotation_x = mesh->transforms.rotation.x;
	auto rotation_y = mesh->transforms.rotation.y;
	auto rotation_z = mesh->transforms.rotation.z;

	if (axis == Axis::X)
	{
		result[0] = rotation_y;
		result[1] = rotation_z;
	}
	else if (axis == Axis::Y)
	{
		result[0] = rotation_x;
		result[1] = rotation_z;
	}
	else if (axis == Axis::Z)
	{
		result[0] = rotation_x;
		result[1] = rotation_y;
	}

	return result;
}

glm::vec3 get_normal_for_axis(Axis axis)
{
	switch (axis)
	{
	case Axis::X: return glm::vec3(1.0f, 0, 0);
	case Axis::Y: return glm::vec3(0, 1.0f, 0);
	case Axis::Z: return glm::vec3(0, 0, 1.0f);
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

glm::vec3 get_vec_for_largest_abs_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements)
{
	float max = std::numeric_limits<float>::min();
	auto max_vec = glm::vec3(0);

	for (int i = 0; i < elements; i++)
	{
		glm::vec3 current = normals[i];
		float dot = glm::dot(current, direction_compare);

		dot = abs(dot);

		if (max < dot)
		{
			max = dot;
			max_vec = current;
		}
	}

	return max_vec;
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

void vec3_add_for_axis(glm::vec3& for_addition, glm::vec3 to_add, Axis axis)
{
	if (axis == Axis::X)
	{
		for_addition.x += to_add.x;
	}
	if (axis == Axis::Y)
	{
		for_addition.y += to_add.y;
	}
	if (axis == Axis::Z)
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

void get_axis_xor(Axis axis, Axis xor_axises[])
{
	if (axis == Axis::X)
	{
		xor_axises[0] = Axis::Y;
		xor_axises[1] = Axis::Z;
	}
	else if (axis == Axis::Y)
	{
		xor_axises[0] = Axis::X;
		xor_axises[1] = Axis::Z;
	}
	else if (axis == Axis::Z)
	{
		xor_axises[0] = Axis::X;
		xor_axises[1] = Axis::Y;
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
		.translation = glm::vec3(0),
		.rotation = glm::vec3(0),
		.scale = glm::vec3(1)
	};
	return t;
}

glm::vec3 get_spotlight_dir(Spotlight spotlight)
{
	glm::vec3 spot_dir = glm::vec3(0, -1.0f, 0);
	glm::mat4 rotation_mat = get_rotation_matrix(spotlight.transforms.rotation);
	spot_dir = rotation_mat * glm::vec4(spot_dir, 1.0f);
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

Framebuffer init_spotlight_shadow_map()
{
	Framebuffer shadow_map = framebuffer_init();

	glGenFramebuffers(1, &shadow_map.id);
	glGenTextures(1, &shadow_map.texture_gpu_id);
	glBindTexture(GL_TEXTURE_2D, shadow_map.texture_gpu_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map.id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map.texture_gpu_id, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ASSERT_TRUE(shadow_map.id != 0 && shadow_map.texture_gpu_id != 0, "Shadow map creation");

	return shadow_map;
}

Spotlight spotlight_init()
{
	Spotlight sp = {
		.shadow_map = init_spotlight_shadow_map(),
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
		.position = glm::vec3(0),
		.front_vec = glm::vec3(1,0,0),
		.up_vec = glm::vec3(0,1,0),
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
