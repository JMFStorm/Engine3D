#include "scene.h"

#include <fstream>
#include <filesystem>

#include "j_assert.h"
#include "j_map.h"
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