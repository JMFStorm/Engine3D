#include "utils.h"

#include <array>
#include <iostream>

void assert_true(bool assertion, const char* assertion_title, const char* file, const char* func, int line)
{
	if (!assertion)
	{
		std::cerr 
			<< "ERROR: Assertion (" << assertion_title 
			<< ") failed in: " << file 
			<< " at function: " << func 
			<< "(), line: " << line 
			<< "." << std::endl;

		exit(1);
	}
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

	auto rotation_x = mesh->rotation.x;
	auto rotation_y = mesh->rotation.y;
	auto rotation_z = mesh->rotation.z;

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
	glm::mat4 rotation = get_rotation_matrix(&mesh);
	glm::vec3 scale_rotated = rotation * glm::vec4(mesh.scale, 1.0f);
	scale_rotated = scale_rotated / 2.0f;
	scale_rotated.y = 0.0f;
	glm::vec3 result = mesh.translation + scale_rotated;
	return result;
}