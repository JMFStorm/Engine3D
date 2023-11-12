#pragma once

#include <unordered_map>

#include "types.h"
#include "j_buffers.h"

constexpr const char* g_materials_dir_path = "G:\\projects\\game\\Engine3D\\resources\\materials";

constexpr const s64 SCENE_POINTLIGHTS_MAX_COUNT = 20;
constexpr const s64 SCENE_SPOTLIGHTS_MAX_COUNT = 20;

extern bool DEBUG_SHADOWMAP;

extern GameCamera g_scene_camera;
extern bool g_camera_move_mode;

extern GameMetrics g_game_metrics;
extern GameInputsU g_inputs;
extern FrameData g_frame_data;
