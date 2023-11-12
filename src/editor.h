#pragma once
#include "glm/glm.hpp"
#include "types.h"
#include "structs.h"

TransformMode get_curr_transformation_mode();

void* get_selected_object_ptr();

glm::vec3 get_selected_object_translation();

Transforms* get_selected_object_transforms();
