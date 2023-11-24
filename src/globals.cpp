#include "globals.h"

bool DEBUG_SHADOWMAP = false;
bool g_camera_move_mode = false;

int g_selected_texture_item = 0;

GLFWwindow* g_window = {};
GameCamera g_scene_camera = {};
GameMetrics g_game_metrics = {};
GameInputsU g_inputs = {};
FrameData g_frame_data = {};

MemoryBuffer materials_id_map_memory = {};
Map64 materials_id_map = {};

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

SimpleShader g_simple_rect_shader = {};
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

s64 g_rects_buffered = 0;
s64 g_lines_buffered = 0;
s64 g_ui_chars_buffered = 0;

FontData g_debug_font = {};

PostProcessingSettings g_pp_settings = {};

unsigned int g_skybox_cubemap = 0;
unsigned int g_view_proj_ubo = 0;

unsigned int g_simple_rect_offset_vbo = 0;
unsigned int g_simple_rect_tex_index_vbo = 0;

GLuint g_texture_arr_01 = 0;

bool g_use_linear_texture_filtering = false;
bool g_generate_texture_mipmaps = false;
bool g_load_texture_sRGB = false;
