#pragma once

#include "structs.h"

void save_scene();

void load_scene();

void save_all();

Material material_deserialize(MaterialData mat_data);
