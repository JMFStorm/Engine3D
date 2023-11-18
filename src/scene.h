#pragma once

#include "structs.h"

void save_scene(char* filepath);

void load_scene(char* filename);

void save_all();

MeshData mesh_serialize(Mesh* mesh);

Mesh mesh_deserialize(MeshData data);

MaterialData material_serialize(Material material);

Material material_deserialize(MaterialData mat_data);

void save_material(Material material);

void new_scene();
