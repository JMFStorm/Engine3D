#include "editor.h"
#include "globals.h"
#include "j_assert.h"

TransformMode get_curr_transformation_mode()
{
	if (g_inputs.as_struct.q.pressed)
		return TransformMode::Translate;

	if (g_inputs.as_struct.w.pressed && g_selected_object.type != ObjectType::Pointlight)
		return TransformMode::Rotate;

	if (g_inputs.as_struct.e.pressed && g_selected_object.type == ObjectType::Primitive)
		return TransformMode::Scale;

	return g_transform_mode.mode;
}

void* get_selected_object_ptr()
{
	if (g_selected_object.type == ObjectType::Primitive)
		return (void*)j_array_get(&g_scene_meshes, g_selected_object.selection_index);

	if (g_selected_object.type == ObjectType::Pointlight)
		return (void*)j_array_get(&g_scene_pointlights, g_selected_object.selection_index);

	if (g_selected_object.type == ObjectType::Spotlight)
		return (void*)j_array_get(&g_scene_spotlights, g_selected_object.selection_index);

	return nullptr;
}

glm::vec3 get_selected_object_translation()
{
	if (g_selected_object.type == ObjectType::Primitive)
	{
		Mesh* mesh = (Mesh*)j_array_get(&g_scene_meshes, g_selected_object.selection_index);
		return mesh->transforms.translation;
	}

	if (g_selected_object.type == ObjectType::Pointlight)
	{
		Pointlight* p_light = (Pointlight*)j_array_get(&g_scene_pointlights, g_selected_object.selection_index);
		return p_light->transforms.translation;
	}

	if (g_selected_object.type == ObjectType::Spotlight)
	{
		Spotlight* s_light = (Spotlight*)j_array_get(&g_scene_spotlights, g_selected_object.selection_index);
		return s_light->transforms.translation;
	}

	return glm::vec3(0);
}

Transforms* get_selected_object_transforms()
{
	switch (g_selected_object.type)
	{
		case ObjectType::Primitive:
		{
			Mesh* as_mesh = (Mesh*)get_selected_object_ptr();
			return &as_mesh->transforms;
		}
		case ObjectType::Pointlight:
		{
			Pointlight* as_pl = (Pointlight*)get_selected_object_ptr();
			return &as_pl->transforms;
		}
		case ObjectType::Spotlight:
		{
			Spotlight* as_sl = (Spotlight*)get_selected_object_ptr();
			return &as_sl->transforms;
		}
	}

	ASSERT_TRUE(false, "Selected object tranform is of known type");
}
