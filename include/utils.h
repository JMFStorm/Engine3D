#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#define KILOBYTES(x) (x * 1024)
#define MEGABYTES(x) (KILOBYTES(x) * 1024)

typedef unsigned char byte;

typedef struct MemoryBuffer {
	unsigned long size;
	byte* memory;
	char name[32];
} MemoryBuffer;

enum Axis {
	X,
	Y,
	Z
};

constexpr const int TEXTURE_PATH_LEN = 128;
constexpr const int TEXTURE_FILENAME_LEN = TEXTURE_PATH_LEN / 2;

typedef struct Texture {
	char file_name[TEXTURE_FILENAME_LEN] = { 0 };
	int texture_id;
} Texture;

typedef struct Plane {
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;
	Texture* texture;
	float uv_multiplier = 1.0f;
} Plane;

void assert_true(bool assertion, const char* assertion_title, const char* file, const char* func, int line);

#define ASSERT_TRUE(assertion, assertion_title) assert_true(assertion, assertion_title, __FILE__, __func__, __LINE__)

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

inline glm::mat4 get_model_matrix(Plane* plane)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, plane->translation);

	glm::quat quaternionX = glm::angleAxis(glm::radians(plane->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat quaternionY = glm::angleAxis(glm::radians(plane->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat quaternionZ = glm::angleAxis(glm::radians(plane->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::quat finalRotation = quaternionY * quaternionX * quaternionZ;
	glm::mat4 rotation_matrix = glm::mat4_cast(finalRotation);

	model = model * rotation_matrix;

	// model = glm::rotate(model, glm::radians(plane->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	// model = glm::rotate(model, glm::radians(plane->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	// model = glm::rotate(model, glm::radians(plane->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	model = glm::scale(model, plane->scale);
	return model;
}

inline glm::mat4 get_rotation_matrix(Plane* plane)
{
	glm::quat quaternionX = glm::angleAxis(glm::radians(plane->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat quaternionY = glm::angleAxis(glm::radians(plane->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat quaternionZ = glm::angleAxis(glm::radians(plane->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::quat finalRotation = quaternionY * quaternionX * quaternionZ;
	glm::mat4 rotation_matrix = glm::mat4_cast(finalRotation);

	return rotation_matrix;

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(plane->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(plane->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(plane->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	return model;
}

void memory_buffer_mallocate(MemoryBuffer* buffer, unsigned long size_in_bytes, char* name);

void memory_buffer_wipe(MemoryBuffer* buffer);

void memory_buffer_free(MemoryBuffer* buffer);

glm::vec3 closest_point_on_plane(const glm::vec3& point1, const glm::vec3& pointOnPlane, const glm::vec3& planeNormal);

std::array<glm::vec3, 2> get_axis_xor_normals(Axis axis);

std::array<float, 2> get_plane_axis_xor_rotations(Axis axis, Plane* plane);

glm::vec3 get_normal_for_axis(Axis axis);

glm::vec3 get_vec_for_smallest_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements);

glm::vec3 get_vec_for_largest_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements);

void vec3_add_for_axis(glm::vec3& for_addition, glm::vec3 to_add, Axis axis);

glm::vec3 get_plane_middle_point(Plane plane);
