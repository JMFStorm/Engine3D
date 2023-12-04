#pragma once

#include "glm/glm.hpp"
#include "types.h"
#include "globals.h"
#include "structs.h"
#include "j_array.h"

TransformMode get_curr_transformation_mode();

void* get_selected_object_ptr();

glm::vec3 get_selected_object_translation();

Transforms* get_selected_object_transforms();

void delete_on_object_index(JArray* jarray_ptr, s64 jarray_index);

void delete_selected_object();

s64 add_new_mesh(Mesh new_mesh);

s64 add_new_pointlight(Pointlight new_light);

s64 add_new_spotlight(Spotlight new_light);

void duplicate_selected_object();

void select_object_index(ObjectType type, s64 index);

void deselect_selection();

bool mouse_in_scene_space(s32 x, s32 y);

bool get_cube_selection(Mesh* cube, float* select_dist, glm::vec3 ray_o, glm::vec3 ray_dir);

s64 get_pointlight_selection_index(JArray* lights, f32* select_dist, glm::vec3 ray_origin, glm::vec3 ray_direction);

s64 get_spotlight_selection_index(JArray* lights, f32* select_dist, glm::vec3 ray_origin, glm::vec3 ray_direction);

s64 get_mesh_selection_index(JArray* meshes, f32* select_dist, glm::vec3 ray_origin, glm::vec3 ray_direction);

void draw_selected_shadow_map();

bool try_init_transform_mode();

void draw_selection_arrows(glm::vec3 position);

inline bool has_object_selection()
{
	return g_selected_object.type != ObjectType::None;
}

void save_materials();

void handle_tranformation_mode();
