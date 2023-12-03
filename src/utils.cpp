#include "utils.h"

#include <array>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <GL/glew.h>

#include "j_assert.h"
#include "j_buffers.h"
#include "j_render.h"
#include "j_strings.h"

glm::mat4 get_projection_matrix()
{
	glm::mat4 projection = glm::perspective(
		glm::radians(g_scene_camera.fov),
		g_scene_camera.aspect_ratio_horizontal,
		g_scene_camera.near_clip,
		g_scene_camera.far_clip);
	return projection;
}

glm::mat4 get_view_matrix()
{
	glm::mat4 view = glm::lookAt(
		g_scene_camera.position,
		g_scene_camera.position + g_scene_camera.front_vec,
		g_scene_camera.up_vec);
	return view;
}

float normalize_screen_px_to_ndc(int value, int max)
{
	float this1 = static_cast<float>(value) / static_cast<float>(max);
	float this2 = 2.0f * this1;
	float res = -1.0f + this2;
	return res;
}

float normalize_value(float value, float src_max, float dest_max)
{
	// Assume 0.0 is min value
	float intermediate = value / src_max;
	float result = dest_max * intermediate;
	return result;
}

glm::vec3 closest_point_on_plane(const glm::vec3& point1, const glm::vec3& pointOnPlane, const glm::vec3& planeNormal) {
	// Calculate the vector from point1 to the point on the plane
	glm::vec3 vectorToPoint = point1 - pointOnPlane;

	// Calculate the projection of vectorToPoint onto the plane's normal
	glm::vec3 projection = glm::dot(vectorToPoint, planeNormal) / glm::dot(planeNormal, planeNormal) * planeNormal;

	// Calculate the closest point on the plane to point1
	glm::vec3 point2 = pointOnPlane + projection;

	return point2;
}

std::array<glm::vec3, 2> get_axis_xor_normals(Axis axis)
{
	std::array<glm::vec3, 2> result{};

	auto normal_vector_x = glm::vec3(1.0f, 0, 0);
	auto normal_vector_y = glm::vec3(0, 1.0f, 0);
	auto normal_vector_z = glm::vec3(0, 0, 1.0f);

	if (axis == Axis::X)
	{
		result[0] = normal_vector_y;
		result[1] = normal_vector_z;
	}
	else if (axis == Axis::Y)
	{
		result[0] = normal_vector_x;
		result[1] = normal_vector_z;
	}
	else if (axis == Axis::Z)
	{
		result[0] = normal_vector_x;
		result[1] = normal_vector_y;
	}

	return result;
}

std::array<float, 2> get_plane_axis_xor_rotations(Axis axis, Mesh* mesh)
{
	std::array<float, 2> result{};

	auto rotation_x = mesh->transforms.rotation.x;
	auto rotation_y = mesh->transforms.rotation.y;
	auto rotation_z = mesh->transforms.rotation.z;

	if (axis == Axis::X)
	{
		result[0] = rotation_y;
		result[1] = rotation_z;
	}
	else if (axis == Axis::Y)
	{
		result[0] = rotation_x;
		result[1] = rotation_z;
	}
	else if (axis == Axis::Z)
	{
		result[0] = rotation_x;
		result[1] = rotation_y;
	}

	return result;
}

glm::vec3 get_normal_for_axis(Axis axis)
{
	switch (axis)
	{
	case Axis::X: return glm::vec3(1.0f, 0, 0);
	case Axis::Y: return glm::vec3(0, 1.0f, 0);
	case Axis::Z: return glm::vec3(0, 0, 1.0f);
	}
}

glm::vec3 get_vec_for_smallest_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements)
{
	float smallest = std::numeric_limits<float>::max();
	auto smallest_vec = glm::vec3(0);

	for (int i = 0; i < elements; i++)
	{
		glm::vec3 current = normals[i];
		float dot = glm::dot(current, direction_compare);

		if (dot < smallest)
		{
			smallest = dot;
			smallest_vec = current;
		}
	}

	return smallest_vec;
}

glm::vec3 get_vec_for_largest_abs_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements)
{
	float max = std::numeric_limits<float>::min();
	auto max_vec = glm::vec3(0);

	for (int i = 0; i < elements; i++)
	{
		glm::vec3 current = normals[i];
		float dot = glm::dot(current, direction_compare);

		dot = abs(dot);

		if (max < dot)
		{
			max = dot;
			max_vec = current;
		}
	}

	return max_vec;
}

glm::vec3 get_vec_for_largest_dot_product(glm::vec3 direction_compare, glm::vec3* normals, int elements)
{
	float max = std::numeric_limits<float>::min();
	auto max_vec = glm::vec3(0);

	for (int i = 0; i < elements; i++)
	{
		glm::vec3 current = normals[i];
		float dot = glm::dot(current, direction_compare);

		if (max < dot)
		{
			max = dot;
			max_vec = current;
		}
	}

	return max_vec;
}

void vec3_add_for_axis(glm::vec3& for_addition, glm::vec3 to_add, Axis axis)
{
	if (axis == Axis::X)
	{
		for_addition.x += to_add.x;
	}
	if (axis == Axis::Y)
	{
		for_addition.y += to_add.y;
	}
	if (axis == Axis::Z)
	{
		for_addition.z += to_add.z;
	}
}

glm::vec3 get_plane_middle_point(Mesh mesh)
{
	glm::mat4 rotation = get_rotation_matrix(mesh.transforms.rotation);
	glm::vec3 scale_rotated = rotation * glm::vec4(mesh.transforms.scale.x, mesh.transforms.scale.y, mesh.transforms.scale.z, 1.0f);
	scale_rotated = scale_rotated / 2.0f;
	scale_rotated.y = 0.0f;
	glm::vec3 result = mesh.transforms.translation + scale_rotated;
	return result;
}

void get_axis_xor(Axis axis, Axis xor_axises[])
{
	if (axis == Axis::X)
	{
		xor_axises[0] = Axis::Y;
		xor_axises[1] = Axis::Z;
	}
	else if (axis == Axis::Y)
	{
		xor_axises[0] = Axis::X;
		xor_axises[1] = Axis::Z;
	}
	else if (axis == Axis::Z)
	{
		xor_axises[0] = Axis::X;
		xor_axises[1] = Axis::Y;
	}
}

bool calculate_plane_ray_intersection(
	glm::vec3 plane_normal,
	glm::vec3 point_in_plane,
	glm::vec3 ray_origin,
	glm::vec3 ray_direction,
	glm::vec3& result)
{
	// Calculate the D coefficient of the plane equation
	float D = -glm::dot(plane_normal, point_in_plane);

	// Calculate t where the ray intersects the plane
	float t = -(glm::dot(plane_normal, ray_origin) + D) / glm::dot(plane_normal, ray_direction);

	// Check if t is non-positive (ray doesn't intersect) or invalid
	if (t <= 0.0f || std::isinf(t) || std::isnan(t))
	{
		return false;
	}

	glm::vec3 intersection_point = ray_origin + t * ray_direction;
	result = intersection_point;
	return true;
}

glm::mat4 get_spotlight_light_space_matrix(Spotlight spotlight)
{
	glm::vec3 spot_dir = get_spotlight_dir(spotlight);
	spot_dir += glm::vec3(0.0001f, 0.001f, 0.0001f); // WTF, why?
	glm::vec3 light_pos = spotlight.transforms.translation;
	glm::vec3 spot_look_at = spotlight.transforms.translation + spot_dir;
	float fov_radians = glm::radians(spotlight.outer_cutoff_fov);
	glm::mat4 light_projection = glm::perspective(fov_radians, 1.0f, SHADOW_MAP_NEAR_PLANE, spotlight.range * 10);
	glm::mat4 light_view = glm::lookAt(light_pos, spot_look_at, glm::vec3(0.0, 1.0, 0.0));
	return light_projection * light_view;
}

Transforms transforms_init()
{
	Transforms t = {
		.translation = glm::vec3(0),
		.rotation = glm::vec3(0),
		.scale = glm::vec3(1)
	};
	return t;
}

glm::vec3 get_spotlight_dir(Spotlight spotlight)
{
	glm::vec3 spot_dir = glm::vec3(0, -1.0f, 0);
	glm::mat4 rotation_mat = get_rotation_matrix(spotlight.transforms.rotation);
	spot_dir = rotation_mat * glm::vec4(spot_dir, 1.0f);
	return glm::normalize(spot_dir);
}

Material material_init()
{
	Material material = {
		.color_texture = nullptr,
		.specular_texture = nullptr,
		.specular_mult = 1.0f,
		.shininess = 32.0f,
	};
	return material;
}

Framebuffer framebuffer_init()
{
	Framebuffer buffer = {
		.id = 0,
		.texture_gpu_id = 0,
		.renderbuffer = 0,
	};
	return buffer;
}

Framebuffer init_spotlight_shadow_map()
{
	Framebuffer shadow_map = framebuffer_init();

	glGenFramebuffers(1, &shadow_map.id);
	glGenTextures(1, &shadow_map.texture_gpu_id);
	glBindTexture(GL_TEXTURE_2D, shadow_map.texture_gpu_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map.id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map.texture_gpu_id, 0);
	glDrawBuffer(GL_NONE);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ASSERT_TRUE(shadow_map.id != 0 && shadow_map.texture_gpu_id != 0, "Shadow map creation");

	return shadow_map;
}

Spotlight spotlight_init()
{
	Spotlight sp = {
		.shadow_map = init_spotlight_shadow_map(),
		.transforms = transforms_init(),
		.diffuse = glm::vec3(1),
		.specular = 2.0f,
		.range = 5.0f,
		.fov = 90.0f,
		.outer_cutoff_fov = 90.0f,
		.is_on = true,
	};
	return sp;
}

Pointlight pointlight_init()
{
	Pointlight p_light = {
		.transforms = transforms_init(),
		.diffuse = glm::vec3(1.0f),
		.range = 6.0f,
		.specular = 1.0f,
		.intensity = 1.0f,
		.is_on = true,
	};
	return p_light;
}

GameCamera scene_camera_init(float horizontal_fov)
{
	GameCamera cam = {
		.position = glm::vec3(0),
		.front_vec = glm::vec3(1,0,0),
		.up_vec = glm::vec3(0,1,0),
		.yaw = 0.0f,
		.pitch = 0.0f,
		.fov = 60.0f,
		.aspect_ratio_horizontal = horizontal_fov,
		.look_sensitivity = 0.1f,
		.move_speed = 5.0f,
		.near_clip = 0.1f,
		.far_clip = 100.0f,
	};
	return cam;
}

float get_line_distance_width(glm::vec3 line_start, glm::vec3 line_end, glm::vec3 view_pos, float thickness)
{
	glm::vec3 line_midpoint = (line_start + line_end) / 2.0f;
	float line_distance = glm::length(view_pos - line_midpoint);
	float scaling = 10.0f / line_distance;

	float line_width = thickness * scaling;
	if (1.0f < line_width) line_width = 1.0f;

	return line_width;
}

SimpleShader simple_shader_init()
{
	SimpleShader shader = {
		.id = 0,
		.vao = 0,
		.vbo = 0,
	};
	return shader;
}

void print_debug_texts()
{
	char debug_str[256];

	sprintf_s(debug_str, "FPS: %d", g_game_metrics.fps);
	append_ui_text(&g_debug_font, debug_str, 0.5f, 100.0f);

	float display_deltatime = g_frame_data.deltatime * 1000;
	sprintf_s(debug_str, "Delta: %.2fms", display_deltatime);
	append_ui_text(&g_debug_font, debug_str, 4.5f, 100.0f);

	sprintf_s(debug_str, "Frames: %lu", g_game_metrics.frames);
	append_ui_text(&g_debug_font, debug_str, 10.5f, 100.0f);

	sprintf_s(debug_str, "Draw calls: %lld", ++g_frame_data.draw_calls);
	append_ui_text(&g_debug_font, debug_str, 17.0f, 100.0f);

	sprintf_s(debug_str, "Camera X=%.2f Y=%.2f Z=%.2f", g_scene_camera.position.x, g_scene_camera.position.y, g_scene_camera.position.z);
	append_ui_text(&g_debug_font, debug_str, 0.5f, 99.0f);

	sprintf_s(debug_str, "Meshes %lld / %lld", g_scene.meshes.items_count, g_scene.meshes.max_items);
	append_ui_text(&g_debug_font, debug_str, 0.5f, 98.0f);

	sprintf_s(debug_str, "Pointlights %lld / %lld", g_scene.pointlights.items_count, g_scene.pointlights.max_items);
	append_ui_text(&g_debug_font, debug_str, 0.5f, 97.0f);

	sprintf_s(debug_str, "Spotlights %lld / %lld", g_scene.spotlights.items_count, g_scene.spotlights.max_items);
	append_ui_text(&g_debug_font, debug_str, 0.5f, 96.0f);

	char* t_mode = nullptr;
	const char* tt = "Translate";
	const char* tr = "Rotate";
	const char* ts = "Scale";
	const char* transform_mode_debug_str_format = "Transform mode: %s";

	if (g_transform_mode.mode == TransformMode::Translate) t_mode = const_cast<char*>(tt);
	if (g_transform_mode.mode == TransformMode::Rotate)	t_mode = const_cast<char*>(tr);
	if (g_transform_mode.mode == TransformMode::Scale)		t_mode = const_cast<char*>(ts);

	sprintf_s(debug_str, transform_mode_debug_str_format, t_mode);
	append_ui_text(&g_debug_font, debug_str, 0.5f, 2.0f);
	draw_ui_text(&g_debug_font, 0.9f, 0.9f, 0.9f);
}

void resize_windows_area_settings(s64 width_px, s64 height_px)
{
	g_game_metrics.game_width_px = width_px;
	g_game_metrics.game_height_px = height_px;

	g_game_metrics.scene_width_px = g_game_metrics.game_width_px - PROPERTIES_PANEL_WIDTH;
	g_game_metrics.scene_height_px = g_game_metrics.game_height_px;

	g_scene_camera.aspect_ratio_horizontal =
		(float)g_game_metrics.scene_width_px / (float)g_game_metrics.scene_height_px;
}

void init_framebuffer_resize(unsigned int* framebuffer_texture_id, unsigned int* renderbuffer_id)
{
	glDeleteTextures(1, framebuffer_texture_id);
	glDeleteRenderbuffers(1, renderbuffer_id);

	glGenTextures(1, framebuffer_texture_id);
	glBindTexture(GL_TEXTURE_2D, *framebuffer_texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_game_metrics.scene_width_px, g_game_metrics.scene_height_px, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *framebuffer_texture_id, 0);

	glGenRenderbuffers(1, renderbuffer_id);
	glBindRenderbuffer(GL_RENDERBUFFER, *renderbuffer_id);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, g_game_metrics.scene_width_px, g_game_metrics.scene_height_px);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *renderbuffer_id);

	ASSERT_TRUE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer successfull");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void allocate_temp_memory(s64 bytes)
{
	memory_buffer_mallocate(&TEMP_MEMORY, MEGABYTES(25), const_cast<char*>("Temp memory"));
}

void deallocate_temp_memory()
{
	memory_buffer_free(&TEMP_MEMORY);
}

void init_memory_buffers()
{
	allocate_temp_memory(MEGABYTES(25));

	// Scene objects
	{
		memory_buffer_mallocate(&g_scene_planes_memory, sizeof(Mesh) * SCENE_MESHES_MAX_COUNT, const_cast<char*>("Scene plane meshes"));
		g_scene.planes = j_array_init(SCENE_PLANES_MAX_COUNT, sizeof(Mesh), g_scene_planes_memory.memory);

		memory_buffer_mallocate(&g_scene_meshes_memory, sizeof(Mesh) * SCENE_MESHES_MAX_COUNT, const_cast<char*>("Scene 3D meshes"));
		g_scene.meshes = j_array_init(SCENE_MESHES_MAX_COUNT, sizeof(Mesh), g_scene_meshes_memory.memory);

		memory_buffer_mallocate(&g_scene_pointlights_memory, sizeof(Pointlight) * SCENE_POINTLIGHTS_MAX_COUNT, const_cast<char*>("Scene pointlights"));
		g_scene.pointlights = j_array_init(SCENE_POINTLIGHTS_MAX_COUNT, sizeof(Pointlight), g_scene_pointlights_memory.memory);

		memory_buffer_mallocate(&g_scene_spotlights_memory, sizeof(Spotlight) * SCENE_SPOTLIGHTS_MAX_COUNT, const_cast<char*>("Scene spotlights"));
		g_scene.spotlights = j_array_init(SCENE_SPOTLIGHTS_MAX_COUNT, sizeof(Spotlight), g_scene_spotlights_memory.memory);

		memory_buffer_mallocate(&g_texture_memory, sizeof(Texture) * SCENE_TEXTURES_MAX_COUNT, const_cast<char*>("Textures"));
		g_textures = j_array_init(SCENE_TEXTURES_MAX_COUNT, sizeof(Texture), g_texture_memory.memory);

		memory_buffer_mallocate(&g_materials_memory, sizeof(Material) * SCENE_TEXTURES_MAX_COUNT, const_cast<char*>("Materials"));
		g_materials = j_array_init(SCENE_TEXTURES_MAX_COUNT, sizeof(Material), g_materials_memory.memory);
	}

	// Material names string list
	constexpr const s64 material_names_arr_size = FILENAME_LEN * SCENE_TEXTURES_MAX_COUNT;
	memory_buffer_mallocate(&g_material_names_memory, material_names_arr_size, const_cast<char*>("Material strings"));
	g_material_names = j_strings_init(material_names_arr_size, (char*)g_material_names_memory.memory);

	// material_id - material_ptr map
	s64 sizeof_material_id_elem = sizeof(char*) + sizeof(s64);
	s64 sizeof_materials_id_map = (sizeof_material_id_elem) * MATERIALS_ID_MAP_CAPACITY;
	memory_buffer_mallocate(&materials_id_map_memory, sizeof_materials_id_map, const_cast<char*>("Material ids map"));
	materials_id_map = jmap_init(MATERIALS_ID_MAP_CAPACITY, sizeof_material_id_elem, materials_id_map_memory.memory);

	// material_name - material_index map
	s64 sizeof_material_index_elem = sizeof(char*) + sizeof(s64);
	s64 sizeof_material_indexes_map = sizeof_material_index_elem * MATERIALS_INDEXES_MAP_CAPACITY;
	memory_buffer_mallocate(&material_indexes_map_memory, sizeof_materials_id_map, const_cast<char*>("Material indexes map"));
	material_indexes_map = jmap_init(MATERIALS_INDEXES_MAP_CAPACITY, sizeof_material_id_elem, material_indexes_map_memory.memory);
}

glm::vec3 get_camera_ray_from_scene_px(int x, int y)
{
	float x_NDC = (2.0f * x) / g_game_metrics.scene_width_px - 1.0f;
	float y_NDC = 1.0f - (2.0f * y) / g_game_metrics.scene_height_px;

	glm::vec4 ray_clip(x_NDC, y_NDC, -1.0f, 1.0f);

	glm::mat4 projection = get_projection_matrix();
	glm::mat4 view = get_view_matrix();

	glm::mat4 inverse_projection = glm::inverse(projection);
	glm::mat4 inverse_view = glm::inverse(view);

	glm::vec4 ray_eye = inverse_projection * ray_clip;
	ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

	glm::vec4 ray_world = inverse_view * ray_eye;
	ray_world = glm::normalize(ray_world);

	return glm::vec3(ray_world);
}

glm::mat4 get_model_matrix(Mesh* mesh)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, mesh->transforms.translation);

	glm::quat quaternionX = glm::angleAxis(glm::radians(mesh->transforms.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat quaternionY = glm::angleAxis(glm::radians(mesh->transforms.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat quaternionZ = glm::angleAxis(glm::radians(mesh->transforms.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::quat finalRotation = quaternionY * quaternionX * quaternionZ;

	glm::mat4 rotation_matrix = glm::mat4_cast(finalRotation);
	model = model * rotation_matrix;
	model = glm::scale(model, mesh->transforms.scale);
	return model;
}

glm::mat4 get_rotation_matrix(glm::vec3 rotation)
{
	glm::quat quaternionX = glm::angleAxis(glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat quaternionY = glm::angleAxis(glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat quaternionZ = glm::angleAxis(glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::quat finalRotation = quaternionY * quaternionX * quaternionZ;

	glm::mat4 rotation_matrix = glm::mat4_cast(finalRotation);
	return rotation_matrix;
}

bool is_primitive(ObjectType type)
{
	return type == ObjectType::Plane || type == ObjectType::Cube;
}

PostProcessingSettings post_processings_init()
{
	PostProcessingSettings res = {
		.inverse_color = false,
		.blur_effect = false,
		.blur_effect_amount = 8.0f,
		.gamma_amount = 1.6f
	};
	return res;
}

Texture texture_load_from_filepath(char* path)
{
	int texture_id = load_image_into_texture_id(path);
	Texture texture = {
		.gpu_id = texture_id,
	};
	return texture;
}

MeshData mesh_serialize(Mesh* mesh)
{
	MeshData data = {
		.transforms = mesh->transforms,
		.mesh_type = mesh->mesh_type,
		.material_id = mesh->material->id,
		.uv_multiplier = mesh->uv_multiplier,
	};
	return data;
}

Mesh mesh_deserialize(MeshData data)
{
	Material* material_ptr = (Material*)jmap_get_k_s64(&materials_id_map, data.material_id);
	Mesh mesh = {
		.transforms = data.transforms,
		.material = material_ptr,
		.mesh_type = data.mesh_type,
		.uv_multiplier = data.uv_multiplier,
	};
	return mesh;
}

Spotlight spotlight_deserialize(SpotlightSerialized serialized)
{
	Spotlight spotlight = {
		.shadow_map = init_spotlight_shadow_map(),
		.transforms = serialized.transforms,
		.diffuse = serialized.diffuse,
		.specular = serialized.specular,
		.range = serialized.range,
		.fov = serialized.fov,
		.outer_cutoff_fov = serialized.outer_cutoff_fov,
		.is_on = serialized.is_on,
	};
	return spotlight;
}

SpotlightSerialized spotlight_serialize(Spotlight spotlight)
{
	SpotlightSerialized serialized = {
		.transforms = spotlight.transforms,
		.diffuse = spotlight.diffuse,
		.specular = spotlight.specular,
		.range = spotlight.range,
		.fov = spotlight.fov,
		.outer_cutoff_fov = spotlight.outer_cutoff_fov,
		.is_on = spotlight.is_on,
	};
	return serialized;
}

MaterialData material_serialize(Material material)
{
	MaterialData m_data = {
		.id = material.id,
		.specular_mult = material.specular_mult,
		.shininess = material.shininess
	};
	return m_data;
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

Material get_material_from_asset_line(char* line)
{
	char* endptr;
	char* next_token; char* token;

	// Material id
	token = strtok_s(line, " ", &next_token);
	bool is_id = (strncmp(token, "id=", 3) == 0);
	ASSERT_TRUE(is_id, "Material id");
	long material_id = strtol(&token[3], &endptr, 10);

	// Material shine
	token = strtok_s(NULL, " ", &next_token);
	bool is_shine = (strncmp(token, "shine=", 6) == 0);
	ASSERT_TRUE(is_shine, "Material shine");

	float material_shine = 0.0f;
	bool got_shine = sscanf_s(&token[6], "%f", &material_shine) == 1;
	ASSERT_TRUE(got_shine, "Material shine as float");

	// Material specular multiplier
	token = strtok_s(NULL, " ", &next_token);
	bool is_spec = (strncmp(token, "specular_mult=", 14) == 0);
	ASSERT_TRUE(is_spec, "Material specular mult");

	float specular_mult = 0.0f;
	bool got_spec = sscanf_s(&token[14], "%f", &specular_mult) == 1;
	ASSERT_TRUE(got_spec, "Specular mult as float");

	// Material name
	token = strtok_s(NULL, " ", &next_token);
	bool is_name = (strncmp(token, "name=", 5) == 0);
	ASSERT_TRUE(is_name, "Material name");

	char* material_name = &token[5];
	str_trim_from_char(material_name, '\n');
	char* str_ptr = j_strings_add(&g_material_names, material_name);

	Material material = {};
	material.name = str_ptr;
	material.id = material_id;
	material.shininess = material_shine;
	material.specular_mult = specular_mult;
	return material;
}

s64 get_materials_from_manifest(Material materials[], s64 max_items)
{
	FILE* file;
	s64 materials_count = 0;
	char line_buffer[256];

	int success = fopen_s(&file, MATERIALS_MANIFEST_PATH, "r");
	ASSERT_TRUE(success == 0, "File opened");

	char* new_line = fgets(line_buffer, sizeof(line_buffer), file);
	bool is_valid = new_line != NULL && strcmp(line_buffer, "materials/\n") == 0;
	ASSERT_TRUE(is_valid, "Valid materials header");

	while (materials_count < max_items)
	{
		new_line = fgets(line_buffer, sizeof(line_buffer), file);
		if (str_is_empty_newline(line_buffer)) break;

		Material mat = get_material_from_asset_line(line_buffer);
		materials[materials_count] = mat;
		materials_count++;
	}

	fclose(file);
	return materials_count;
}

void load_material_textures(Material materials[], s64 materials_count)
{
	for (int i = 0; i < materials_count; i++)
	{
		char filepath[FILE_PATH_LEN] = {};
		Material* material = &materials[i];

		sprintf_s(filepath, "%s%s%s", MATERIALS_DIR_PATH, material->name, const_cast<char*>(".png"));
		g_load_texture_sRGB = true;
		Texture color_texture = texture_load_from_filepath(filepath);

		sprintf_s(filepath, "%s%s%s", MATERIALS_DIR_PATH, material->name, const_cast<char*>("_specular.png"));
		g_load_texture_sRGB = false;
		Texture specular_texture = texture_load_from_filepath(filepath);

		Texture* color_texture_prt = (Texture*)j_array_add(&g_textures, (byte*)&color_texture);
		Texture* specular_texture_ptr = (Texture*)j_array_add(&g_textures, (byte*)&specular_texture);

		material->color_texture = color_texture_prt;
		material->specular_texture = specular_texture_ptr;
	}
}

void load_materials_into_memory(Material materials[], s64 materials_count)
{
	for (int i = 0; i < materials_count; i++)
	{
		Material material = materials[i];
		Material* added_mat_ptr = (Material*)j_array_add(&g_materials, (byte*)&material);

		JMapItem_S64_Ptr mat_id_item = {
			.key = added_mat_ptr->id,
			.value = (byte*)added_mat_ptr
		};

		JMapItem_Str_S64 mat_name_index_item = {
			.key = added_mat_ptr->name,
			.value = i
		};

		jmap_add(&materials_id_map, mat_id_item);
		jmap_add(&material_indexes_map, mat_name_index_item);
	}
}
