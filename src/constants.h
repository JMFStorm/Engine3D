#pragma once

#include "types.h"

constexpr const char* g_materials_dir_path = "G:\\projects\\game\\Engine3D\\resources\\materials";

constexpr const s64 SCENE_POINTLIGHTS_MAX_COUNT = 100;
constexpr const s64 SCENE_SPOTLIGHTS_MAX_COUNT = 100;
constexpr const s64 MAX_LINES_BUFFER = 200;

constexpr s64 PROPERTIES_PANEL_WIDTH = 400;

constexpr const s64 FILE_PATH_LEN = 256;
constexpr const s64 FILENAME_LEN = FILE_PATH_LEN / 4;

constexpr const u64 SHADOW_MAP_WIDTH = 1024 * 2;
constexpr const u64 SHADOW_MAP_HEIGHT = 1024 * 2;

constexpr f32 SHADOW_MAP_NEAR_PLANE = 0.25f;

constexpr float debug_font_vh = 1.0f;
constexpr const char* g_debug_font_path = "G:/projects/game/Engine3D/resources/fonts/Inter-Regular.ttf";
constexpr const s64 g_max_UI_chars = 1000;
