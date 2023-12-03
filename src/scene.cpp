#include "scene.h"

#include <fstream>
#include <filesystem>

#include "j_assert.h"
#include "j_map.h"
#include "main.h"
#include "structs.h"
#include "globals.h"
#include "utils.h"
#include "j_platform.h"
#include "editor.h"

void save_scene(char* filepath)
{
	std::ofstream output_file(filepath, std::ios::binary);
	ASSERT_TRUE(output_file.is_open(), ".jmap file opened for save");

	GameCamera data = g_scene_camera;

	// Header
	output_file.write(".jmap", 5);

	// Scene camera
	output_file.write(reinterpret_cast<char*>(&data), sizeof(data));

	// Plane count
	output_file.write(reinterpret_cast<char*>(&g_scene.planes.items_count), sizeof(s64));

	// Plane data
	for (int i = 0; i < g_scene.planes.items_count; i++)
	{
		Mesh* plane_ptr = (Mesh*)j_array_get(&g_scene.planes, i);
		MeshData plane_data = mesh_serialize(plane_ptr);
		output_file.write(reinterpret_cast<char*>(&plane_data), sizeof(plane_data));
	}

	// Mesh count
	output_file.write(reinterpret_cast<char*>(&g_scene.meshes.items_count), sizeof(s64));

	// Mesh data
	for (int i = 0; i < g_scene.meshes.items_count; i++)
	{
		Mesh* mesh_ptr = (Mesh*)j_array_get(&g_scene.meshes, i);
		MeshData m_data = mesh_serialize(mesh_ptr);
		output_file.write(reinterpret_cast<char*>(&m_data), sizeof(m_data));
	}

	// Pointlight count
	output_file.write(reinterpret_cast<char*>(&g_scene.pointlights.items_count), sizeof(s64));

	// Pointlight data
	for (int i = 0; i < g_scene.pointlights.items_count; i++)
	{
		Pointlight light = *(Pointlight*)j_array_get(&g_scene.pointlights, i);
		output_file.write(reinterpret_cast<char*>(&light), sizeof(light));
	}

	// Spotlight count
	output_file.write(reinterpret_cast<char*>(&g_scene.spotlights.items_count), sizeof(s64));

	// Spotlight data
	for (int i = 0; i < g_scene.spotlights.items_count; i++)
	{
		Spotlight light = *(Spotlight*)j_array_get(&g_scene.spotlights, i);
		SpotlightSerialized sp_data = spotlight_serialize(light);
		output_file.write(reinterpret_cast<char*>(&sp_data), sizeof(sp_data));
	}

	output_file.close();
	printf("Saved scene: %s\n", g_scene.filepath);
}

void load_scene(char* filepath)
{
	float h_aspect = (float)g_game_metrics.scene_width_px / (float)g_game_metrics.scene_height_px;

	ASSERT_TRUE(std::filesystem::exists(filepath), "Scene filepath exists");

	std::ifstream input_file(filepath, std::ios::binary);
	ASSERT_TRUE(input_file.is_open(), ".jmap file opened for load");

	strcpy_s(g_scene.filepath, FILE_PATH_LEN, filepath);

	// Header
	char header[6] = { 0 };
	input_file.read(header, 5);
	ASSERT_TRUE(strcmp(header, ".jmap") == 0, ".jmap file has valid header");

	// Scene camera
	GameCamera cam_data = {};
	input_file.read(reinterpret_cast<char*>(&cam_data), sizeof(cam_data));
	g_scene_camera = cam_data;
	g_scene_camera.aspect_ratio_horizontal = h_aspect;

	// Plane count
	s64 plane_count = 0;
	input_file.read(reinterpret_cast<char*>(&plane_count), sizeof(s64));

	// Plane data
	for (int i = 0; i < plane_count; i++)
	{
		MeshData plane_data;
		input_file.read(reinterpret_cast<char*>(&plane_data), sizeof(plane_data));
		Mesh plane = mesh_deserialize(plane_data);
		j_array_add(&g_scene.planes, (byte*)&plane);
	}

	// Mesh count
	s64 mesh_count = 0;
	input_file.read(reinterpret_cast<char*>(&mesh_count), sizeof(s64));

	// Mesh data
	for (int i = 0; i < mesh_count; i++)
	{
		MeshData m_data;
		input_file.read(reinterpret_cast<char*>(&m_data), sizeof(m_data));
		Mesh mesh = mesh_deserialize(m_data);
		j_array_add(&g_scene.meshes, (byte*)&mesh);
	}

	// Pointlight count
	s64 pointlight_count = 0;
	input_file.read(reinterpret_cast<char*>(&pointlight_count), sizeof(s64));

	// Pointlight data
	for (int i = 0; i < pointlight_count; i++)
	{
		Pointlight light;
		input_file.read(reinterpret_cast<char*>(&light), sizeof(light));
		j_array_add(&g_scene.pointlights, (byte*)&light);
	}

	// Spotlight count
	s64 spotlight_count = 0;
	input_file.read(reinterpret_cast<char*>(&spotlight_count), sizeof(s64));


	// Spotlight data
	for (int i = 0; i < spotlight_count; i++)
	{
		SpotlightSerialized sp_data;
		input_file.read(reinterpret_cast<char*>(&sp_data), sizeof(sp_data));
		Spotlight sp = spotlight_deserialize(sp_data);
		j_array_add(&g_scene.spotlights, (byte*)&sp);
	}

	input_file.close();
	printf("Loaded scene: %s\n", g_scene.filepath);
}

void save_all()
{
	save_materials();

	char used_filepath[FILE_PATH_LEN] = {};

	if (strcmp(g_scene.filepath, "") == 0)
	{
		bool success = file_dialog_make_filepath(used_filepath);
		if (!success) return;
	}
	else
	{
		strcpy_s(used_filepath, FILE_PATH_LEN, g_scene.filepath);
	}

	save_scene(used_filepath);
}

void new_scene()
{
	g_scene_camera = scene_camera_init(g_scene_camera.aspect_ratio_horizontal);
	deselect_selection();

	j_array_empty(&g_scene.planes);
	j_array_empty(&g_scene.meshes);
	j_array_empty(&g_scene.pointlights);
	j_array_empty(&g_scene.spotlights);
	memset(g_scene.filepath, 0, 256);
}

void try_get_mouse_selection(s32 xpos, s32 ypos)
{
	glm::vec3 ray_origin = g_scene_camera.position;
	glm::vec3 ray_direction = get_camera_ray_from_scene_px(xpos, ypos);

	s64 object_types_count = 4;
	s64 object_index[4] = { -1, -1, -1, -1 };
	f32 closest_dist[4] = {};
	object_index[0] = get_mesh_selection_index(&g_scene.planes, &closest_dist[0], ray_origin, ray_direction);
	object_index[1] = get_mesh_selection_index(&g_scene.meshes, &closest_dist[1], ray_origin, ray_direction);
	object_index[2] = get_pointlight_selection_index(&g_scene.pointlights, &closest_dist[2], ray_origin, ray_direction);
	object_index[3] = get_spotlight_selection_index(&g_scene.spotlights, &closest_dist[3], ray_origin, ray_direction);
	ObjectType selected_type = ObjectType::None;
	s64 closest_obj_index = -1;
	f32 closest_dist_result = std::numeric_limits<float>::max();

	bool got_selection = false;
	ObjectType select_types[] = { ObjectType::Plane, ObjectType::Cube, ObjectType::Pointlight, ObjectType::Spotlight };

	for (int i = 0; i < object_types_count; i++)
	{
		s64 current_index = object_index[i];
		f32 current_dist = closest_dist[i];

		if (current_index != -1 && current_dist < closest_dist_result)
		{
			closest_dist_result = current_dist;
			closest_obj_index = current_index;
			selected_type = select_types[i];
			got_selection = true;
		}
	}

	if (got_selection) select_object_index(selected_type, closest_obj_index);
	else deselect_selection();
}

void handle_camera_move_mode()
{
	if (g_camera_move_mode)
	{
		enable_cursor(false);
		g_frame_data.mouse_move_x *= g_scene_camera.look_sensitivity;
		g_frame_data.mouse_move_y *= g_scene_camera.look_sensitivity;
		g_scene_camera.yaw += g_frame_data.mouse_move_x;
		g_scene_camera.pitch += g_frame_data.mouse_move_y;

		if 		(g_scene_camera.pitch > 89.0f) g_scene_camera.pitch = 89.0f;
		else if (g_scene_camera.pitch < -89.0f) g_scene_camera.pitch = -89.0f;

		glm::vec3 new_camera_front;
		new_camera_front.x = cos(glm::radians(g_scene_camera.yaw)) * cos(glm::radians(g_scene_camera.pitch));
		new_camera_front.y = sin(glm::radians(g_scene_camera.pitch));
		new_camera_front.z = sin(glm::radians(g_scene_camera.yaw)) * cos(glm::radians(g_scene_camera.pitch));

		g_scene_camera.front_vec = glm::normalize(new_camera_front);

		float speed_mult = g_scene_camera.move_speed * g_frame_data.deltatime;

		if (g_inputs.as_struct.w.is_down)
		{
			g_scene_camera.position += speed_mult * g_scene_camera.front_vec;
		}
		if (g_inputs.as_struct.s.is_down)
		{
			g_scene_camera.position -= speed_mult * g_scene_camera.front_vec;
		}
		if (g_inputs.as_struct.a.is_down)
		{
			g_scene_camera.position -= glm::normalize(glm::cross(g_scene_camera.front_vec, g_scene_camera.up_vec)) * speed_mult;
		}
		if (g_inputs.as_struct.d.is_down)
		{
			g_scene_camera.position += glm::normalize(glm::cross(g_scene_camera.front_vec, g_scene_camera.up_vec)) * speed_mult;
		}
	}
	else enable_cursor(true);
}