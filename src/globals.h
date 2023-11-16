#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <unordered_map>

#include "j_array.h"
#include "types.h"
#include "j_buffers.h"
#include "j_strings.h"
#include "structs.h"

extern bool DEBUG_SHADOWMAP;

extern GLFWwindow* g_window;
extern GameCamera g_scene_camera;
extern bool g_camera_move_mode;

extern GameMetrics g_game_metrics;
extern GameInputsU g_inputs;
extern FrameData g_frame_data;

extern std::unordered_map<char*, s64, CharPtrHash, CharPtrEqual> g_mat_data_map;
extern std::unordered_map<char*, s64, CharPtrHash, CharPtrEqual> g_materials_index_map;
extern std::unordered_map<s64, char*> g_mat_data_map_inverse;

extern MemoryBuffer g_temp_memory;
extern MemoryBuffer g_ui_text_vertex_buffer;
extern MemoryBuffer g_materials_memory;
extern MemoryBuffer g_scene_planes_memory;
extern MemoryBuffer g_scene_meshes_memory;
extern MemoryBuffer g_scene_pointlights_memory;
extern MemoryBuffer g_scene_spotlights_memory;
extern MemoryBuffer g_texture_memory;
extern MemoryBuffer g_material_names_memory;

extern JStringArray g_material_names;
extern TransformationMode g_transform_mode;

extern JArray g_materials;
extern JArray g_scene_spotlights;
extern JArray g_scene_pointlights;
extern JArray g_scene_planes;
extern JArray g_scene_meshes;
extern JArray g_textures;

extern int g_selected_texture_item;

extern SceneSelection g_selected_object;

extern SimpleShader g_shdow_map_debug_shader;
extern SimpleShader g_shdow_map_shader;
extern SimpleShader g_mesh_shader;
extern SimpleShader g_billboard_shader;
extern SimpleShader g_ui_text_shader;
extern SimpleShader g_line_shader;
extern SimpleShader g_wireframe_shader;
extern SimpleShader g_scene_framebuffer_shader;
extern Framebuffer g_scene_framebuffer;

extern UserSettings g_user_settings;

extern MemoryBuffer g_line_vertex_buffer;
extern s64 g_line_buffer_size;
extern s64 g_line_indicies;

extern s64 g_text_buffer_size;
extern s64 g_text_indicies;
extern FontData g_debug_font;

extern PostProcessingSettings g_pp_settings;
