#pragma once

#include "types.h"

constexpr const char* g_materials_dir_path = "G:\\projects\\game\\Engine3D\\resources\\materials";
constexpr const char* g_debug_font_path = "G:\\projects\\game\\Engine3D\\resources\\fonts\\Inter-Regular.ttf";
constexpr const char* pointlight_image_path = "G:\\projects\\game\\Engine3D\\resources\\images\\pointlight_billboard.png";
constexpr const char* spotlight_image_path = "G:\\projects\\game\\Engine3D\\resources\\images\\spotlight_billboard.png";

constexpr const s64 SCENE_POINTLIGHTS_MAX_COUNT = 100;
constexpr const s64 SCENE_SPOTLIGHTS_MAX_COUNT = 100;
constexpr const s64 SCENE_TEXTURES_MAX_COUNT = 100;
constexpr const s64 SCENE_PLANES_MAX_COUNT = 100;
constexpr const s64 SCENE_MESHES_MAX_COUNT = 100;

constexpr const s64 MAX_LINES_BUFFERED = 200;
constexpr const s64 LINE_VERICIES = 12;

constexpr const s64 MAX_UI_CHARS_BUFFERED = 500;
constexpr const s64 UI_CHAR_VERTICIES = 30;

constexpr const s64 PROPERTIES_PANEL_WIDTH = 400;
constexpr const s64 SIZEOF_VIEW_MATRICES = 2 * sizeof(glm::mat4);

constexpr const s64 FILE_PATH_LEN = 256;
constexpr const s64 FILENAME_LEN = FILE_PATH_LEN / 4;

constexpr const u64 SHADOW_MAP_WIDTH = 1024;
constexpr const u64 SHADOW_MAP_HEIGHT = 1024;

constexpr const f32 SHADOW_MAP_NEAR_PLANE = 0.25f;

constexpr const float debug_font_vh = 1.0f;
