#pragma once

#include "structs.h"

void save_scene(char* filepath);

void load_scene(char* filename);

void save_all();

void new_scene();

void try_get_mouse_selection(s32 xpos, s32 ypos);
