#pragma once

#include <array>
#include <glm.hpp>

#define KILOBYTES(x) (x * 1024)
#define MEGABYTES(x) (KILOBYTES(x) * 1024)

typedef unsigned char		byte;
typedef unsigned int		u32;
typedef int					s32;
typedef unsigned long long  u64;
typedef long long			s64;
typedef float				f32;
typedef double				f64;

constexpr const s64 E_Axis_X = 0;
constexpr const s64 E_Axis_Y = 1;
constexpr const s64 E_Axis_Z = 2;

enum class TransformMode {
	Translate,
	Rotate,
	Scale
};

enum class ObjectType {
	None,
	Primitive,
	Pointlight,
	Spotlight
};

constexpr const s64 E_Primitive_Plane = 0;
constexpr const s64 E_Primitive_Cube = 1;
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

typedef struct Pointlight {
	glm::vec3 position;
	glm::vec3 diffuse;
	float specular;
	float intensity;
} Pointlight;

typedef struct Spotlight {
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 diffuse;
	float specular;
	float intensity;
	float cutoff;
	float outer_cutoff;
} Spotlight;

typedef struct SceneSelection {
	s64 selection_index;
	ObjectType type;
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

typedef struct TransformationMode {
	s64 axis;
	TransformMode mode;
	bool is_active;
} TransformationMode;

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

typedef struct UserSettings {
	int window_size_px[2];
	float transform_clip;
	float transform_rotation_clip;
	glm::vec3 world_ambient;
} UserSettings;

typedef struct MaterialIdData {
	char material_name[FILENAME_LEN];
	s64 material_id;
} MaterialIdData;
