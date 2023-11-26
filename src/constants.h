#pragma once

#include "types.h"

constexpr const char* MATERIALS_MANIFEST_PATH = "G:\\projects\\game\\Engine3D\\resources\\materials\\materials_manifest.txt";
constexpr const char* MATERIALS_DIR_PATH = "G:\\projects\\game\\Engine3D\\resources\\materials\\";

constexpr const s64 MATERIALS_MAX_COUNT = 64;
constexpr const s64 MATERIALS_INDEXES_MAP_CAPACITY = MATERIALS_MAX_COUNT;
constexpr const s64 MATERIALS_ID_MAP_CAPACITY = MATERIALS_MAX_COUNT;

constexpr const char* g_debug_font_path = "G:\\projects\\game\\Engine3D\\resources\\fonts\\Inter-Regular.ttf";
constexpr const char* pointlight_image_path = "G:\\projects\\game\\Engine3D\\resources\\billboards\\pointlight_billboard.png";
constexpr const char* spotlight_image_path = "G:\\projects\\game\\Engine3D\\resources\\billboards\\spotlight_billboard.png";

constexpr const s64 SCENE_POINTLIGHTS_MAX_COUNT = 100;
constexpr const s64 SCENE_SPOTLIGHTS_MAX_COUNT = 100;
constexpr const s64 SCENE_TEXTURES_MAX_COUNT = 100;
constexpr const s64 SCENE_PLANES_MAX_COUNT = 100;
constexpr const s64 SCENE_MESHES_MAX_COUNT = 100;

constexpr const s64 TEXTURE_SIZE_1K = 1024;

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