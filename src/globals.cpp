#include "globals.h"

bool DEBUG_SHADOWMAP = false;
bool g_camera_move_mode = false;

int g_selected_texture_item = 0;

GLFWwindow* g_window = {};
GameCamera g_scene_camera = {};
GameMetrics g_game_metrics = {};
GameInputsU g_inputs = {};
FrameData g_frame_data = {};

std::unordered_map<char*, s64, CharPtrHash, CharPtrEqual> g_mat_data_map = {};
std::unordered_map<char*, s64, CharPtrHash, CharPtrEqual> g_materials_index_map = {};
std::unordered_map<s64, char*> g_mat_data_map_inverse = {};

MemoryBuffer g_temp_memory = {};
MemoryBuffer g_ui_text_vertex_buffer = {};
MemoryBuffer g_materials_memory = {};
MemoryBuffer g_scene_planes_memory = {};
MemoryBuffer g_scene_meshes_memory = {};
MemoryBuffer g_scene_pointlights_memory = {};
MemoryBuffer g_scene_spotlights_memory = {};
MemoryBuffer g_texture_memory = {};
MemoryBuffer g_material_names_memory = {};

JStringArray g_material_names = {};
TransformationMode g_transform_mode = {};

Scene g_scene = {};

JArray g_materials = {};
JArray g_textures = {};

SceneSelection g_selected_object = {
	.selection_index = -1,
	.type = ObjectType::None,
};

SimpleShader g_skybox_shader = {};
SimpleShader g_shdow_map_debug_shader = {};
SimpleShader g_shdow_map_shader = {};
SimpleShader g_mesh_shader = {};
SimpleShader g_billboard_shader = {};
SimpleShader g_ui_text_shader = {};
SimpleShader g_line_shader = {};
SimpleShader g_wireframe_shader = {};
SimpleShader g_scene_framebuffer_shader = {};

Framebuffer g_scene_framebuffer = {};

UserSettings g_user_settings = {
	.window_size_px = { 1900, 1200 },
	.transform_clip = 0.25f,
	.transform_rotation_clip = 15.0f,
	.world_ambient = glm::vec3(0.075f),
};

MemoryBuffer g_line_vertex_buffer = {};
s64 g_line_buffer_size = 0;
s64 g_line_indicies = 0;

s64 g_text_buffer_size = 0;
s64 g_text_indicies = 0;
FontData g_debug_font = {};

PostProcessingSettings g_pp_settings = {};

unsigned int g_skybox_cubemap = 0;
