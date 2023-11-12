#include "globals.h"

bool DEBUG_SHADOWMAP = false;

GameCamera g_scene_camera = {};
bool g_camera_move_mode = false;

GameMetrics g_game_metrics = {};

GameInputsU g_inputs = {};

FrameData g_frame_data = {};

std::unordered_map<char*, s64, CharPtrHash, CharPtrEqual> g_mat_data_map = {};
std::unordered_map<char*, s64, CharPtrHash, CharPtrEqual> g_materials_index_map = {};
std::unordered_map<s64, char*> g_mat_data_map_inverse = {};

TransformationMode g_transform_mode = {};

JArray g_materials = {};
JArray g_scene_spotlights = {};
JArray g_scene_pointlights = {};
JArray g_scene_meshes = {};
JArray g_textures = {};

SceneSelection g_selected_object = {
	.selection_index = -1,
	.type = ObjectType::None,
};

SimpleShader g_shdow_map_debug_shader = {};
SimpleShader g_shdow_map_shader = {};
SimpleShader g_mesh_shader = {};
SimpleShader g_billboard_shader = {};
SimpleShader g_ui_text_shader = {};
SimpleShader g_line_shader = {};
SimpleShader g_wireframe_shader = {};

UserSettings g_user_settings = {
	.window_size_px = { 1900, 1200 },
	.transform_clip = 0.25f,
	.transform_rotation_clip = 15.0f,
	.world_ambient = glm::vec3(0.075f),
};

MemoryBuffer g_line_vertex_buffer = {};
s64 g_line_buffer_size = 0;
s64 g_line_indicies = 0;
