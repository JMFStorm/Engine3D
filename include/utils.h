#pragma once

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#define KILOBYTES(x) (x * 1024)
#define MEGABYTES(x) (KILOBYTES(x) * 1024)

typedef unsigned char		byte;
typedef unsigned int		u32;
typedef int					s32;
typedef unsigned long long  u64;
typedef long long			s64;
typedef float				f32;
typedef double				f64;

constexpr const s64 E_Transform_Translate = 0;
constexpr const s64 E_Transform_Rotate    = 1;
constexpr const s64 E_Transform_Scale     = 2;

constexpr const s64 E_Axis_X = 0;
constexpr const s64 E_Axis_Y = 1;
constexpr const s64 E_Axis_Z = 2;

constexpr const s64 E_Type_None  = 0;
constexpr const s64 E_Type_Mesh  = 1;
constexpr const s64 E_Type_Light = 2;

constexpr const s64 E_Primitive_Plane  = 0;
constexpr const s64 E_Primitive_Cube   = 1;
constexpr const s64 E_Primitive_Sphere = 2;

constexpr const int FILE_PATH_LEN = 256;
constexpr const int FILENAME_LEN = FILE_PATH_LEN / 4;

typedef struct Texture {
	char file_name[FILENAME_LEN];
	s64 gpu_id;
} Texture;

typedef struct Material {
	Texture* color_texture;
	Texture* specular_texture;
	f32 specular_mult;
	f32 shininess;
} Material;

typedef struct MaterialData {
	s64 material_id;
	f32 specular_mult;
	f32 shininess;
} MaterialData;

typedef struct Mesh {
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;
	Material* material;
	s64 mesh_type;
	f32 uv_multiplier;
} Mesh;

typedef struct MeshData {
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;
	s64 mesh_type;
	s64 material_id;
	f32 uv_multiplier;
} MeshData;

typedef struct Light {
	glm::vec3 position;
	glm::vec3 diffuse;
	float specular;
	float intensity;
} Light;

typedef struct SceneSelection {
	s64 selection_index;
	s64 type;
} SceneSelection;

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

typedef struct FrameData {
	float mouse_x;
	float mouse_y;
	float prev_mouse_x;
	float prev_mouse_y;
	int draw_calls;
	float deltatime;
	bool mouse_clicked;
} FrameData;

void assert_true(bool assertion, const char* assertion_title, const char* file, const char* func, int line);

#define ASSERT_TRUE(assertion, assertion_title) assert_true(assertion, assertion_title, __FILE__, __func__, __LINE__)

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

inline glm::mat4 get_rotation_matrix(Mesh* mesh)
{
	glm::quat quaternionX = glm::angleAxis(glm::radians(mesh->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat quaternionY = glm::angleAxis(glm::radians(mesh->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat quaternionZ = glm::angleAxis(glm::radians(mesh->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
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

