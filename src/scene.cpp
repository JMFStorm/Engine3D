#include "scene.h"

#include <fstream>
#include <filesystem>

#include "j_assert.h"
#include "structs.h"
#include "globals.h"
#include "utils.h"

MeshData mesh_serialize(Mesh* mesh)
{
	s64 material_id = g_mat_data_map[mesh->material->color_texture->file_name];

	MeshData data = {
		.transforms = mesh->transforms,
		.mesh_type = mesh->mesh_type,
		.material_id = material_id,
		.uv_multiplier = mesh->uv_multiplier,
	};
	return data;
}

Mesh mesh_deserialize(MeshData data)
{
	char* material_name = g_mat_data_map_inverse[data.material_id];
	s64 material_index = g_materials_index_map[material_name];
	Material* material_ptr = (Material*)j_array_get(&g_materials, material_index);

	Mesh mesh = {
		.transforms = data.transforms,
		.material = material_ptr,
		.mesh_type = data.mesh_type,
		.uv_multiplier = data.uv_multiplier,
	};
	return mesh;
}

MaterialData material_serialize(Material material)
{
	s64 material_id = g_mat_data_map[material.color_texture->file_name];

	MaterialData m_data = {
		.material_id = material_id,
		.specular_mult = material.specular_mult,
		.shininess = material.shininess
	};
	return m_data;
}

void save_material(Material material)
{
	MaterialData m_data = material_serialize(material);

	char filename[FILENAME_LEN] = { 0 };
	strcpy_s(filename, material.color_texture->file_name);
	str_trim_file_ext(filename);

	char filepath[FILE_PATH_LEN] = { 0 };
	sprintf_s(filepath, "%s\\%s.jmat", g_materials_dir_path, filename);

	std::ofstream output_mat_file(filepath, std::ios::binary | std::ios::trunc);
	ASSERT_TRUE(output_mat_file.is_open(), ".jmat file opened");

	output_mat_file.write(".jmat", 5);
	output_mat_file.write(reinterpret_cast<char*>(&m_data), sizeof(m_data));
	output_mat_file.close();
}

Material material_deserialize(MaterialData mat_data)
{
	Material mat = {
		.color_texture = nullptr,
		.specular_texture = nullptr,
		.specular_mult = mat_data.specular_mult,
		.shininess = mat_data.shininess,
	};
	return mat;
}

void save_scene()
{
	std::ofstream output_file("scene_01.jmap", std::ios::binary);
	ASSERT_TRUE(output_file.is_open(), ".jmap file opened for save");

	GameCamera data = g_scene_camera;

	// Header
	output_file.write(".jmap", 5);

	// Scene camera
	output_file.write(reinterpret_cast<char*>(&data), sizeof(data));

	// Mesh count
	output_file.write(reinterpret_cast<char*>(&g_scene_meshes.items_count), sizeof(s64));

	// Mesh data
	for (int i = 0; i < g_scene_meshes.items_count; i++)
	{
		Mesh* mesh_ptr = (Mesh*)j_array_get(&g_scene_meshes, i);
		MeshData m_data = mesh_serialize(mesh_ptr);
		output_file.write(reinterpret_cast<char*>(&m_data), sizeof(m_data));
	}

	// Pointlight count
	output_file.write(reinterpret_cast<char*>(&g_scene_pointlights.items_count), sizeof(s64));

	// Pointlight data
	for (int i = 0; i < g_scene_pointlights.items_count; i++)
	{
		Pointlight light = *(Pointlight*)j_array_get(&g_scene_pointlights, i);
		output_file.write(reinterpret_cast<char*>(&light), sizeof(light));
	}

	// Spotlight count
	output_file.write(reinterpret_cast<char*>(&g_scene_spotlights.items_count), sizeof(s64));

	// Spotlight data
	for (int i = 0; i < g_scene_spotlights.items_count; i++)
	{
		Spotlight light = *(Spotlight*)j_array_get(&g_scene_spotlights, i);
		output_file.write(reinterpret_cast<char*>(&light), sizeof(light));
	}

	output_file.close();
}

void load_scene()
{
	const char* filename = "scene_01.jmap";
	float h_aspect = (float)g_game_metrics.scene_width_px / (float)g_game_metrics.scene_height_px;

	if (!std::filesystem::exists(filename))
	{
		printf(".jmap file not found.\n");
		g_scene_camera = scene_camera_init(h_aspect);
		return;
	}

	std::ifstream input_file(filename, std::ios::binary);
	ASSERT_TRUE(input_file.is_open(), ".jmap file opened for load");

	// Header
	char header[6] = { 0 };
	input_file.read(header, 5);
	ASSERT_TRUE(strcmp(header, ".jmap") == 0, ".jmap file has valid header");

	// Scene camera
	GameCamera cam_data = {};
	input_file.read(reinterpret_cast<char*>(&cam_data), sizeof(cam_data));
	g_scene_camera = cam_data;
	g_scene_camera.aspect_ratio_horizontal = h_aspect;

	// Mesh count
	s64 mesh_count = 0;
	input_file.read(reinterpret_cast<char*>(&mesh_count), sizeof(s64));

	// Mesh data
	for (int i = 0; i < mesh_count; i++)
	{
		MeshData m_data;
		input_file.read(reinterpret_cast<char*>(&m_data), sizeof(m_data));
		Mesh mesh = mesh_deserialize(m_data);
		j_array_add(&g_scene_meshes, (byte*)&mesh);
	}

	// Pointlight count
	s64 pointlight_count = 0;
	input_file.read(reinterpret_cast<char*>(&pointlight_count), sizeof(s64));

	// Pointlight data
	for (int i = 0; i < pointlight_count; i++)
	{
		Pointlight light;
		input_file.read(reinterpret_cast<char*>(&light), sizeof(light));
		j_array_add(&g_scene_pointlights, (byte*)&light);
	}

	// Spotlight count
	s64 spotlight_count = 0;
	input_file.read(reinterpret_cast<char*>(&spotlight_count), sizeof(s64));


	// Spotlight data
	for (int i = 0; i < spotlight_count; i++)
	{
		Spotlight light;
		input_file.read(reinterpret_cast<char*>(&light), sizeof(light));
		j_array_add(&g_scene_spotlights, (byte*)&light);
	}

	input_file.close();
}

void save_all()
{
	for (int i = 0; i < g_materials.items_count; i++)
	{
		Material to_save = *(Material*)j_array_get(&g_materials, i);
		save_material(to_save);
		printf("Mat saved.\n");
	}

	save_scene();
}
