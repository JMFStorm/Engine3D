#include "editor.h"
#include <glm/gtc/matrix_inverse.hpp>
#include "globals.h"
#include "j_assert.h"
#include "utils.h"
#include "j_render.h"

TransformMode get_curr_transformation_mode()
{
	if (g_inputs.as_struct.q.pressed)
		return TransformMode::Translate;

	if (g_inputs.as_struct.w.pressed && g_selected_object.type != ObjectType::Pointlight)
		return TransformMode::Rotate;

	if (g_inputs.as_struct.e.pressed 
		&& is_primitive(g_selected_object.type))
		return TransformMode::Scale;

	return g_transform_mode.mode;
}

void* get_selected_object_ptr()
{
	if (g_selected_object.type == ObjectType::Plane)
		return (void*)j_array_get(&g_scene.planes, g_selected_object.selection_index);

	if (g_selected_object.type == ObjectType::Cube)
		return (void*)j_array_get(&g_scene.meshes, g_selected_object.selection_index);

	if (g_selected_object.type == ObjectType::Pointlight)
		return (void*)j_array_get(&g_scene.pointlights, g_selected_object.selection_index);

	if (g_selected_object.type == ObjectType::Spotlight)
		return (void*)j_array_get(&g_scene.spotlights, g_selected_object.selection_index);

	return nullptr;
}

glm::vec3 get_selected_object_translation()
{
	if (g_selected_object.type == ObjectType::Plane)
	{
		Mesh* mesh = (Mesh*)j_array_get(&g_scene.planes, g_selected_object.selection_index);
		return mesh->transforms.translation;
	}

	if (g_selected_object.type == ObjectType::Cube)
	{
		Mesh* mesh = (Mesh*)j_array_get(&g_scene.meshes, g_selected_object.selection_index);
		return mesh->transforms.translation;
	}

	if (g_selected_object.type == ObjectType::Pointlight)
	{
		Pointlight* p_light = (Pointlight*)j_array_get(&g_scene.pointlights, g_selected_object.selection_index);
		return p_light->transforms.translation;
	}

	if (g_selected_object.type == ObjectType::Spotlight)
	{
		Spotlight* s_light = (Spotlight*)j_array_get(&g_scene.spotlights, g_selected_object.selection_index);
		return s_light->transforms.translation;
	}

	return glm::vec3(0);
}

Transforms* get_selected_object_transforms()
{
	switch (g_selected_object.type)
	{
		case ObjectType::Plane:
		case ObjectType::Cube:
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

void delete_on_object_index(JArray* jarray_ptr, s64 jarray_index)
{
	j_array_unordered_delete(jarray_ptr, jarray_index);
	g_selected_object.selection_index = -1;
	g_selected_object.type = ObjectType::None;
}

void delete_selected_object()
{
	if (g_selected_object.type == ObjectType::Plane) delete_on_object_index(&g_scene.planes, g_selected_object.selection_index);
	else if (g_selected_object.type == ObjectType::Cube) delete_on_object_index(&g_scene.meshes, g_selected_object.selection_index);
	else if (g_selected_object.type == ObjectType::Pointlight) delete_on_object_index(&g_scene.pointlights, g_selected_object.selection_index);
	else if (g_selected_object.type == ObjectType::Spotlight) delete_on_object_index(&g_scene.spotlights, g_selected_object.selection_index);
}

s64 add_new_mesh(Mesh new_mesh)
{
	s64 new_index = -1;
	s64 material_index = g_materials_index_map[new_mesh.material->color_texture->file_name];
	g_selected_texture_item = material_index;

	if (new_mesh.mesh_type == MeshType::Cube)
	{
		j_array_add(&g_scene.meshes, (byte*)&new_mesh);
		new_index = g_scene.meshes.items_count - 1;
	}
	else if (new_mesh.mesh_type == MeshType::Plane)
	{
		j_array_add(&g_scene.planes, (byte*)&new_mesh);
		new_index = g_scene.planes.items_count - 1;
	}

	ASSERT_TRUE(new_index != -1, "Add new mesh");

	return new_index;
}

s64 add_new_pointlight(Pointlight new_light)
{
	j_array_add(&g_scene.pointlights, (byte*)&new_light);
	s64 new_index = g_scene.pointlights.items_count - 1;
	return new_index;
}

s64 add_new_spotlight(Spotlight new_light)
{
	j_array_add(&g_scene.spotlights, (byte*)&new_light);
	s64 new_index = g_scene.spotlights.items_count - 1;
	return new_index;
}

void duplicate_selected_object()
{
	if (g_selected_object.type == ObjectType::Plane)
	{
		Mesh mesh_copy = *(Mesh*)j_array_get(&g_scene.planes, g_selected_object.selection_index);
		s64 index = add_new_mesh(mesh_copy);
		g_selected_object.type = ObjectType::Plane;
		g_selected_object.selection_index = index;
	}
	else if (g_selected_object.type == ObjectType::Cube)
	{
		Mesh mesh_copy = *(Mesh*)j_array_get(&g_scene.meshes, g_selected_object.selection_index);
		s64 index = add_new_mesh(mesh_copy);
		g_selected_object.type = ObjectType::Cube;
		g_selected_object.selection_index = index;
	}
	else if (g_selected_object.type == ObjectType::Pointlight)
	{
		Pointlight light_copy = *(Pointlight*)j_array_get(&g_scene.pointlights, g_selected_object.selection_index);
		s64 index = add_new_pointlight(light_copy);
		g_selected_object.type = ObjectType::Pointlight;
		g_selected_object.selection_index = index;
	}
	else if (g_selected_object.type == ObjectType::Spotlight)
	{
		Spotlight light_copy = *(Spotlight*)j_array_get(&g_scene.spotlights, g_selected_object.selection_index);
		light_copy.shadow_map = init_spotlight_shadow_map();
		s64 index = add_new_spotlight(light_copy);
		g_selected_object.type = ObjectType::Spotlight;
		g_selected_object.selection_index = index;
	}
}

void select_object_index(ObjectType type, s64 index)
{
	g_selected_object.type = type;
	g_selected_object.selection_index = index;

	if (is_primitive(type))
	{
		Mesh* mesh_ptr = (Mesh*)get_selected_object_ptr();
		auto selected_texture_name = g_materials_index_map[mesh_ptr->material->color_texture->file_name];
		g_selected_texture_item = selected_texture_name;
	}
	else if (type == ObjectType::Pointlight) g_transform_mode.mode = TransformMode::Translate;
}

void deselect_selection()
{
	g_selected_object.selection_index = -1;
	g_selected_object.type = ObjectType::None;
}

bool clicked_scene_space(int x, int y)
{
	return x < g_game_metrics.scene_width_px && y < g_game_metrics.scene_height_px;
}

bool get_cube_selection(Mesh* cube, float* select_dist, glm::vec3 ray_o, glm::vec3 ray_dir)
{
	constexpr const s64 CUBE_PLANES_COUNT = 6;
	bool got_selected = false;
	float closest_select = std::numeric_limits<float>::max();

	glm::vec3 cube_normals[CUBE_PLANES_COUNT] = {
		glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f),
		glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(-1.0f,  0.0f,  0.0f),
		glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f,  0.0f, -1.0f)
	};

	Axis plane_axises[CUBE_PLANES_COUNT] = {
		Axis::Y, Axis::Y,
		Axis::X, Axis::X,
		Axis::Z, Axis::Z
	};

	glm::mat4 rotation_matrix = get_rotation_matrix(cube->transforms.rotation);

	for (int j = 0; j < CUBE_PLANES_COUNT; j++)
	{
		auto current_normal = cube_normals[j];
		auto current_axis = plane_axises[j];

		auto plane_normal = glm::vec3(rotation_matrix * glm::vec4(current_normal, 1.0f));
		plane_normal = glm::normalize(plane_normal);

		glm::vec3 intersection_point = glm::vec3(0);

		float plane_scale = get_vec3_val_by_axis(cube->transforms.scale, current_axis);
		glm::vec3 plane_middle_point = cube->transforms.translation + plane_normal * plane_scale * 0.5f;

		bool intersection = calculate_plane_ray_intersection(
			plane_normal, plane_middle_point, ray_o, ray_dir, intersection_point);

		if (!intersection) continue;

		glm::vec3 intersection_vec3 = intersection_point - plane_middle_point;
		glm::vec3 relative_position = intersection_vec3 - glm::vec3(rotation_matrix[3]);
		glm::mat4 inverse_rotation_matrix = glm::affineInverse(rotation_matrix);
		glm::vec3 local_intersection_point = glm::vec3(inverse_rotation_matrix * glm::vec4(relative_position, 1.0f));

		Axis xor_axises[2] = {};
		get_axis_xor(current_axis, xor_axises);

		float abs1 = std::abs(get_vec3_val_by_axis(local_intersection_point, xor_axises[0]));
		float abs2 = std::abs(get_vec3_val_by_axis(local_intersection_point, xor_axises[1]));

		bool intersect_1 = abs1 <= get_vec3_val_by_axis(cube->transforms.scale, xor_axises[0]) * 0.5f;
		bool intersect_2 = abs2 <= get_vec3_val_by_axis(cube->transforms.scale, xor_axises[1]) * 0.5f;

		if (intersect_1 && intersect_2)
		{
			float dist = glm::length(ray_o - intersection_point);

			if (dist < closest_select)
			{
				*select_dist = dist;
				closest_select = dist;
			}

			got_selected = true;
		}
	}

	return got_selected;
}

s64 get_pointlight_selection_index(JArray* lights, f32* select_dist, glm::vec3 ray_origin, glm::vec3 ray_direction)
{
	s64 index = -1;
	Mesh as_cube;
	Pointlight* as_light;
	*select_dist = std::numeric_limits<float>::max();

	for (int i = 0; i < lights->items_count; i++)
	{
		as_light = (Pointlight*)j_array_get(lights, i);
		as_cube = {};
		as_cube.mesh_type = MeshType::Cube;
		as_cube.transforms.translation = as_light->transforms.translation;
		as_cube.transforms.scale = glm::vec3(0.35f);

		f32 new_select_dist;
		bool selected_light = get_cube_selection(&as_cube, &new_select_dist, ray_origin, ray_direction);

		if (selected_light && new_select_dist < *select_dist)
		{
			index = i;
			*select_dist = new_select_dist;
		}
	}

	return index;
}

s64 get_spotlight_selection_index(JArray* lights, f32* select_dist, glm::vec3 ray_origin, glm::vec3 ray_direction)
{
	s64 index = -1;
	Mesh as_cube;
	Spotlight* as_light;
	*select_dist = std::numeric_limits<float>::max();

	for (int i = 0; i < lights->items_count; i++)
	{
		as_light = (Spotlight*)j_array_get(lights, i);
		as_cube = {};
		as_cube.mesh_type = MeshType::Cube;
		as_cube.transforms.translation = as_light->transforms.translation;
		as_cube.transforms.scale = glm::vec3(0.35f);

		f32 new_select_dist;
		bool selected_light = get_cube_selection(&as_cube, &new_select_dist, ray_origin, ray_direction);

		if (selected_light && new_select_dist < *select_dist)
		{
			index = i;
			*select_dist = new_select_dist;
		}
	}

	return index;
}

s64 get_mesh_selection_index(JArray* meshes, f32* select_dist, glm::vec3 ray_origin, glm::vec3 ray_direction)
{
	s64 index = -1;
	*select_dist = std::numeric_limits<float>::max();

	for (int i = 0; i < meshes->items_count; i++)
	{
		Mesh* mesh = (Mesh*)j_array_get(meshes, i);

		if (mesh->mesh_type == MeshType::Plane)
		{
			auto plane_up = glm::vec3(0.0f, 1.0f, 0.0f);
			glm::mat4 rotationMatrix = get_rotation_matrix(mesh->transforms.rotation);

			auto planeNormal = glm::vec3(rotationMatrix * glm::vec4(plane_up, 1.0f));
			planeNormal = glm::normalize(planeNormal);

			glm::vec3 intersection_point = glm::vec3(0);

			bool intersection = calculate_plane_ray_intersection(
				planeNormal, mesh->transforms.translation, ray_origin, ray_direction, intersection_point);

			if (!intersection) continue;

			glm::vec3 intersection_vec3 = intersection_point - mesh->transforms.translation;

			// Calculate the relative position of the intersection point with respect to the plane's origin
			glm::vec3 relative_position = intersection_vec3 - glm::vec3(rotationMatrix[3]);

			// Calculate the inverse of the plane's rotation matrix
			glm::mat4 inverse_rotation_matrix = glm::affineInverse(rotationMatrix);

			// Apply the inverse rotation matrix to convert the point from world space to local space
			glm::vec3 local_intersection_point = glm::vec3(inverse_rotation_matrix * glm::vec4(relative_position, 1.0f));

			bool intersect_x = 0.0f <= local_intersection_point.x && local_intersection_point.x <= mesh->transforms.scale.x;
			bool intersect_z = 0.0f <= local_intersection_point.z && local_intersection_point.z <= mesh->transforms.scale.z;

			if (intersect_x && intersect_z)
			{
				float dist = glm::length(ray_origin - intersection_point);

				if (dist < *select_dist)
				{
					*select_dist = dist;
					index = i;
				}
			}
		}
		else if (mesh->mesh_type == MeshType::Cube)
		{
			f32 new_select_dist;
			bool selected_cube = get_cube_selection(mesh, &new_select_dist, ray_origin, ray_direction);

			if (selected_cube && new_select_dist < *select_dist)
			{
				index = i;
				*select_dist = new_select_dist;
			}
		}
	}

	return index;
}

void draw_selected_shadow_map()
{
	s64 index = g_selected_object.selection_index;
	draw_shadow_map_debug_screen(index);
}

bool try_init_transform_mode()
{
	bool has_valid_mode = g_transform_mode.mode == TransformMode::Translate
		|| (g_transform_mode.mode == TransformMode::Rotate && g_selected_object.type != ObjectType::Pointlight)
		|| (g_transform_mode.mode == TransformMode::Scale && is_primitive(g_selected_object.type));

	if (!has_valid_mode) return false;

	if (g_inputs.as_struct.x.is_down) g_transform_mode.axis = Axis::X;
	else if (g_inputs.as_struct.c.is_down) g_transform_mode.axis = Axis::Y;
	else if (g_inputs.as_struct.z.is_down) g_transform_mode.axis = Axis::Z;

	double xpos, ypos;
	glfwGetCursorPos(g_window, &xpos, &ypos);

	glm::vec3 intersection_point;
	std::array<glm::vec3, 2> use_normals = get_axis_xor_normals(g_transform_mode.axis);
	g_transform_mode.transform_ray = get_camera_ray_from_scene_px((int)xpos, (int)ypos);

	Transforms selected_obj_t = *get_selected_object_transforms();

	if (g_transform_mode.mode == TransformMode::Translate)
	{
		g_transform_mode.transform_plane_normal = get_vec_for_largest_abs_dot_product(g_transform_mode.transform_ray, use_normals.data(), use_normals.size());

		bool intersection = calculate_plane_ray_intersection(
			g_transform_mode.transform_plane_normal,
			selected_obj_t.translation,
			g_scene_camera.position,
			g_transform_mode.transform_ray,
			intersection_point);

		if (!intersection) return false;

		g_transform_mode.prev_intersection_point = intersection_point;
		g_transform_mode.new_tranformation = selected_obj_t.translation;
	}
	else if (g_transform_mode.mode == TransformMode::Scale)
	{
		glm::mat4 model = get_rotation_matrix(selected_obj_t.rotation);

		use_normals[0] = model * glm::vec4(use_normals[0], 1.0f);
		use_normals[1] = model * glm::vec4(use_normals[1], 1.0f);

		g_transform_mode.transform_plane_normal = get_vec_for_largest_abs_dot_product(g_transform_mode.transform_ray, use_normals.data(), use_normals.size());

		bool intersection = calculate_plane_ray_intersection(
			g_transform_mode.transform_plane_normal,
			selected_obj_t.translation,
			g_scene_camera.position,
			g_transform_mode.transform_ray,
			intersection_point);

		if (!intersection) return false;

		glm::vec3 used_normal = get_normal_for_axis(g_transform_mode.axis);
		used_normal = model * glm::vec4(used_normal, 1.0f);

		glm::vec3 point_on_scale_plane = closest_point_on_plane(intersection_point, selected_obj_t.translation, used_normal);
		g_transform_mode.prev_intersection_point = point_on_scale_plane;
		g_transform_mode.new_tranformation = selected_obj_t.scale;
	}
	else if (g_transform_mode.mode == TransformMode::Rotate)
	{
		g_transform_mode.transform_plane_normal = get_normal_for_axis(g_transform_mode.axis);

		bool intersection = calculate_plane_ray_intersection(
			g_transform_mode.transform_plane_normal,
			selected_obj_t.translation,
			g_scene_camera.position,
			g_transform_mode.transform_ray,
			intersection_point);

		if (!intersection) return false;

		g_transform_mode.prev_intersection_point = intersection_point;
		g_transform_mode.new_tranformation = selected_obj_t.rotation;
	}

	return true;
}

void draw_selection_arrows(glm::vec3 position)
{
	auto vec_x = glm::vec3(1.0f, 0, 0);
	auto vec_y = glm::vec3(0, 1.0f, 0);
	auto vec_z = glm::vec3(0, 0, 1.0f);

	if (g_transform_mode.mode == TransformMode::Rotate)
	{
		auto transforms = get_selected_object_transforms();
		auto rotation_m = glm::mat3(get_rotation_matrix(transforms->rotation));

		vec_x = rotation_m * vec_x;
		vec_z = rotation_m * vec_z;

		auto start_x = position - vec_x * 0.5f;
		auto start_y = position - vec_y * 0.5f;
		auto start_z = position - vec_z * 0.5f;

		auto end_x = position + vec_x * 0.5f;
		auto end_y = position + vec_y * 0.5f;
		auto end_z = position + vec_z * 0.5f;

		append_line(start_y, end_y, glm::vec3(0.0f, 1.0f, 0.0f));
		append_line(start_x, end_x, glm::vec3(1.0f, 0.0f, 0.0f));
		append_line(start_z, end_z, glm::vec3(0.0f, 0.0f, 1.0f));
		draw_lines_ontop(7.0f);
	}
	else
	{
		if (g_transform_mode.mode == TransformMode::Scale)
		{
			Mesh* mesh_ptr = (Mesh*)get_selected_object_ptr();
			glm::mat4 rotation_mat = get_rotation_matrix(mesh_ptr->transforms.rotation);
			vec_x = rotation_mat * glm::vec4(vec_x, 1.0f);
			vec_y = rotation_mat * glm::vec4(vec_y, 1.0f);
			vec_z = rotation_mat * glm::vec4(vec_z, 1.0f);
		}

		auto end_x = position + vec_x;
		auto end_y = position + vec_y;
		auto end_z = position + vec_z;

		append_line(position, end_x, glm::vec3(1.0f, 0.0f, 0.0f));
		append_line(position, end_y, glm::vec3(0.0f, 1.0f, 0.0f));
		append_line(position, end_z, glm::vec3(0.0f, 0.0f, 1.0f));
		draw_lines_ontop(14.0f);
	}
}
