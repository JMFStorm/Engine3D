#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "j_array.h"
#include "j_buffers.h"
#include "j_map.h"
#include "j_strings.h"
#include "structs.h"
#include "types.h"

extern bool DEBUG_SHADOWMAP;

extern GLFWwindow* g_window;
extern GameCamera g_scene_camera;
extern bool g_camera_move_mode;

extern GameMetrics g_game_metrics;
extern GameInputsU g_inputs;
extern FrameData g_frame_data;

extern MemoryBuffer g_temp_memory;
extern MemoryBuffer g_materials_memory;
extern MemoryBuffer g_scene_planes_memory;
extern MemoryBuffer g_scene_meshes_memory;
extern MemoryBuffer g_scene_pointlights_memory;
extern MemoryBuffer g_scene_spotlights_memory;
extern MemoryBuffer g_texture_memory;
extern MemoryBuffer g_material_names_memory;

extern MemoryBuffer materials_id_map_memory;
extern JMap materials_id_map;

extern MemoryBuffer material_indexes_map_memory;
extern JMap material_indexes_map;

extern JStringArray g_material_names;
extern TransformationMode g_transform_mode;

extern Scene g_scene;

extern JArray g_materials;
extern JArray g_textures;

extern int g_selected_texture_item;

extern SceneSelection g_selected_object;

extern SimpleShader g_simple_rect_shader;
extern SimpleShader g_skybox_shader;
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

extern s64 g_rects_buffered;
extern s64 g_lines_buffered;
extern s64 g_ui_chars_buffered;

extern FontData g_debug_font;

extern PostProcessingSettings g_pp_settings;

extern unsigned int g_skybox_cubemap;
extern unsigned int g_view_proj_ubo;

extern unsigned int g_simple_rect_offset_vbo;
extern unsigned int g_simple_rect_tex_index_vbo;

extern GLuint g_texture_arr_01;

extern bool g_use_linear_texture_filtering;
extern bool g_generate_texture_mipmaps;
extern bool g_load_texture_sRGB;
