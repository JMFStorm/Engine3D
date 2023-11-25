#pragma once

#include <glm/glm.hpp>
#include "types.h"
#include "constants.h"
#include "j_array.h"

typedef struct Transforms {
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;
} Transforms;

typedef struct Texture {
	s64 gpu_id;
} Texture;

typedef struct Material {
	char* name;
	Texture* color_texture;
	Texture* specular_texture;
	s64 id;
	f32 specular_mult;
	f32 shininess;
} Material;

typedef struct MaterialData {
	s64 id;
	f32 specular_mult;
	f32 shininess;
} MaterialData;

typedef struct Mesh {
	Transforms transforms;
	Material* material;
	MeshType mesh_type;
	f32 uv_multiplier;
} Mesh;

typedef struct MeshData {
	Transforms transforms;
	MeshType mesh_type;
	s64 material_id;
	f32 uv_multiplier;
} MeshData;

typedef struct Framebuffer {
	u32 id;
	u32 texture_gpu_id;
	u32 renderbuffer;
} Framebuffer;

typedef struct Pointlight {
	Transforms transforms;
	glm::vec3 diffuse;
	f32 range;
	f32 specular;
	f32 intensity;
	bool is_on;
} Pointlight;

typedef struct Spotlight {
	Framebuffer shadow_map;
	Transforms transforms;
	glm::vec3 diffuse;
	f32 specular;
	f32 range;
	f32 fov;
	f32 outer_cutoff_fov;
	bool is_on;
} Spotlight;

typedef struct SpotlightSerialized {
	Transforms transforms;
	glm::vec3 diffuse;
	f32 specular;
	f32 range;
	f32 fov;
	f32 outer_cutoff_fov;
	bool is_on;
} SpotlightSerialized;

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
	s64 draw_calls;
	f32 mouse_x;
	f32 mouse_y;
	f32 mouse_move_x;
	f32 mouse_move_y;
	f32 prev_mouse_x;
	f32 prev_mouse_y;
	f32 deltatime;
	bool mouse_clicked;
} FrameData;

typedef struct TransformationMode {
	glm::vec3 new_tranformation;
	glm::vec3 transform_plane_normal;
	glm::vec3 transform_ray;
	glm::vec3 new_intersection_point;
	glm::vec3 prev_intersection_point;
	Axis axis;
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

typedef struct SimpleShader {
	u32 id;
	u32 vao;
	u32 vbo;
} SimpleShader;

typedef struct PostProcessingSettings {
	bool inverse_color;
	bool blur_effect;
	f32 blur_effect_amount;
	f32 gamma_amount;
} PostProcessingSettings;

typedef struct Scene {
	char filepath[FILE_PATH_LEN];
	JArray planes;
	JArray meshes;
	JArray pointlights;
	JArray spotlights;
} Scene;

typedef struct ImageData {
	s32 width_px;
	s32 height_px;
	s32 channels;
	byte* image_data;
} ImageData;
