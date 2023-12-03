#include "j_render.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "j_assert.h"
#include "j_files.h"
#include "globals.h"
#include "constants.h"
#include "utils.h"
#include "editor.h"

bool check_shader_compile_error(GLuint shader)
{
	GLint success;
	constexpr int length = 512;
	GLchar infoLog[length];

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(shader, length, NULL, infoLog);
		printf("ERROR::SHADER_COMPILATION_ERROR: %s", infoLog);
		return false;
	}

	return true; // Returns success
}

bool check_shader_link_error(GLuint shader)
{
	GLint success;
	constexpr int length = 512;
	GLchar infoLog[length];

	glGetProgramiv(shader, GL_LINK_STATUS, &success);

	if (!success)
	{
		glGetProgramInfoLog(shader, length, NULL, infoLog);
		printf("ERROR::PROGRAM_LINKING_ERROR: %s", infoLog);
		return false;
	}

	return true; // Returns success
}

int compile_shader(const char* vertex_shader_path, const char* fragment_shader_path, MemoryBuffer* buffer)
{
	int shader_id;
	char* vertex_shader_code = nullptr;
	char* fragment_shader_code = nullptr;

	read_file_to_memory(vertex_shader_path, buffer);
	auto vertex_code = (char*)buffer->memory;
	int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_code, NULL);
	glCompileShader(vertex_shader);

	bool vs_compile_success = check_shader_compile_error(vertex_shader);
	ASSERT_TRUE(vs_compile_success, "Vertex shader compile");

	read_file_to_memory(fragment_shader_path, buffer);
	auto fragment_code = (char*)buffer->memory;
	int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_code, NULL);
	glCompileShader(fragment_shader);

	bool fs_compile_success = check_shader_compile_error(fragment_shader);
	ASSERT_TRUE(fs_compile_success, "Fragment shader compile");

	shader_id = glCreateProgram();
	glAttachShader(shader_id, vertex_shader);
	glAttachShader(shader_id, fragment_shader);
	glLinkProgram(shader_id);

	bool link_success = check_shader_link_error(shader_id);
	ASSERT_TRUE(link_success, "Shader program link");

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return shader_id;
}

void draw_billboard(glm::vec3 position, Texture texture, float scale)
{
	glUseProgram(g_billboard_shader.id);
	glBindVertexArray(g_billboard_shader.vao);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);

	auto billboard_normal = glm::vec3(0, 0, 1.0f);
	auto billboard_dir = glm::normalize(g_scene_camera.position - position);

	glm::vec3 right = glm::cross(glm::vec3(0, 1.0f, 0), billboard_dir);
	glm::vec3 up = glm::cross(billboard_dir, right);

	glm::mat4 rotation_matrix(1.0f);
	rotation_matrix[0] = glm::vec4(right, 0.0f);
	rotation_matrix[1] = glm::vec4(up, 0.0f);
	rotation_matrix[2] = glm::vec4(billboard_dir, 0.0f);

	model = model * rotation_matrix;
	model = glm::scale(model, glm::vec3(scale));

	unsigned int model_loc = glGetUniformLocation(g_billboard_shader.id, "model");
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture.gpu_id);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void draw_mesh_shadow_map(Mesh* mesh, Spotlight* spotlight)
{
	glm::mat4 model = get_model_matrix(mesh);
	unsigned int model_loc = glGetUniformLocation(g_shdow_map_shader.id, "model");

	s64 draw_indicies = 0;

	if (mesh->mesh_type == MeshType::Plane)
	{
		float vertices[] =
		{
			// Coords		
			1.0f, 0.0f, 0.0f, // top right
			0.0f, 0.0f, 0.0f, // top left
			0.0f, 0.0f, 1.0f, // bot left

			1.0f, 0.0f, 0.0f, // top right
			0.0f, 0.0f, 1.0f, // bot left 
			1.0f, 0.0f, 1.0f, // bot right
		};

		glm::mat4 rotation = get_rotation_matrix(mesh->transforms.rotation);
		glm::vec3 plane_normal = glm::vec3(rotation * glm::vec4(0, 1.0f, 0, 0));

		glm::mat4 sp_rotation = get_rotation_matrix(spotlight->transforms.rotation);
		glm::vec3 spotlight_view_dir = glm::normalize(glm::vec3(sp_rotation * glm::vec4(0, -1.0f, 0, 0)));

		float dot_result = glm::dot(glm::normalize(spotlight_view_dir), -plane_normal);
		float dot_result_mult = dot_result;
		float shadow_bias = 0.025f + (0.05f * dot_result_mult);

		glm::vec3 plane_view_dir = glm::normalize(glm::vec3(mesh->transforms.translation - spotlight->transforms.translation));
		model = glm::translate(model, plane_view_dir * shadow_bias);

		draw_indicies = 6;
		glBindBuffer(GL_ARRAY_BUFFER, g_shdow_map_shader.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}
	else if (mesh->mesh_type == MeshType::Cube)
	{
		float vertices[] =
		{
			// Coords		
			// Ceiling
			0.5f,  0.5f, -0.5f,// top right
		   -0.5f,  0.5f, -0.5f,// top left
		   -0.5f,  0.5f,  0.5f,// bot left

			0.5f,  0.5f, -0.5f,// top right
		   -0.5f,  0.5f,  0.5f,// bot left 
			0.5f,  0.5f,  0.5f,// bot right

			// Floor
		   -0.5f, -0.5f,  0.5f,// bot left 
			0.5f, -0.5f, -0.5f,// top right
			0.5f, -0.5f,  0.5f,// bot right

		   -0.5f, -0.5f,  0.5f,// bot left
		   -0.5f, -0.5f, -0.5f,// top left
			0.5f, -0.5f, -0.5f,// top right

			// North
		   -0.5f, -0.5f, -0.5f, // bot left 
			0.5f,  0.5f, -0.5f, // top right
			0.5f, -0.5f, -0.5f, // bot right

		   -0.5f, -0.5f, -0.5f, // bot left
		   -0.5f,  0.5f, -0.5f, // top left
			0.5f,  0.5f, -0.5f, // top right

			// South
			0.5f,  0.5f,  0.5f, // top right
		   -0.5f, -0.5f,  0.5f, // bot left 
			0.5f, -0.5f,  0.5f, // bot right

		   -0.5f,  0.5f,  0.5f, // top left
		   -0.5f, -0.5f,  0.5f, // bot left
			0.5f,  0.5f,  0.5f, // top right

			// West
		   -0.5f, -0.5f,  0.5f, // bot left 
		   -0.5f,  0.5f, -0.5f, // top right
		   -0.5f, -0.5f, -0.5f, // bot right

		   -0.5f, -0.5f,  0.5f, // bot left
		   -0.5f,  0.5f,  0.5f, // top left
		   -0.5f,  0.5f, -0.5f, // top right

		   // East
		   0.5f,  0.5f, -0.5f, // top right
		   0.5f, -0.5f,  0.5f, // bot left 
		   0.5f, -0.5f, -0.5f, // bot right

		   0.5f,  0.5f,  0.5f, // top left
		   0.5f, -0.5f,  0.5f, // bot left
		   0.5f,  0.5f, -0.5f, // top right
		};

		draw_indicies = 36;
		glBindBuffer(GL_ARRAY_BUFFER, g_shdow_map_shader.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}

	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDrawArrays(GL_TRIANGLES, 0, draw_indicies);
	g_frame_data.draw_calls++;
}

void draw_mesh(Mesh* mesh)
{
	glUseProgram(g_mesh_shader.id);
	glBindVertexArray(g_mesh_shader.vao);

	glm::mat4 model = get_model_matrix(mesh);

	unsigned int model_loc = glGetUniformLocation(g_mesh_shader.id, "model");
	unsigned int camera_view_loc = glGetUniformLocation(g_mesh_shader.id, "view_coords");

	unsigned int ambient_loc = glGetUniformLocation(g_mesh_shader.id, "global_ambient_light");
	unsigned int use_texture_loc = glGetUniformLocation(g_mesh_shader.id, "use_texture");
	unsigned int use_gloss_texture_loc = glGetUniformLocation(g_mesh_shader.id, "use_specular_texture");
	unsigned int uv_loc = glGetUniformLocation(g_mesh_shader.id, "uv_multiplier");

	unsigned int color_texture_loc = glGetUniformLocation(g_mesh_shader.id, "material.color_texture");
	unsigned int specular_texture_loc = glGetUniformLocation(g_mesh_shader.id, "material.specular_texture");
	unsigned int specular_multiplier_loc = glGetUniformLocation(g_mesh_shader.id, "material.specular_mult");
	unsigned int material_shine_loc = glGetUniformLocation(g_mesh_shader.id, "material.shininess");

	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

	glUniform1f(uv_loc, mesh->uv_multiplier);
	glUniform1i(color_texture_loc, 0);
	glUniform1i(use_texture_loc, true);

	glUniform3f(ambient_loc, g_user_settings.world_ambient[0], g_user_settings.world_ambient[1], g_user_settings.world_ambient[2]);
	glUniform3f(camera_view_loc, g_scene_camera.position.x, g_scene_camera.position.y, g_scene_camera.position.z);

	// Lights
	{
		char str_value[64] = { 0 };

		// Pointlights
		unsigned int pointlights_count_loc = glGetUniformLocation(g_mesh_shader.id, "pointlights_count");
		glUniform1i(pointlights_count_loc, g_scene.pointlights.items_count);

		for (int i = 0; i < g_scene.pointlights.items_count; i++)
		{
			auto pointlight = *(Pointlight*)j_array_get(&g_scene.pointlights, i);

			sprintf_s(str_value, "pointlights[%d].is_on", i);
			unsigned int light_is_on_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "pointlights[%d].position", i);
			unsigned int light_pos_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "pointlights[%d].diffuse", i);
			unsigned int light_diff_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "pointlights[%d].specular", i);
			unsigned int light_spec_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "pointlights[%d].intensity", i);
			unsigned int light_intens_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "pointlights[%d].range", i);
			unsigned int light_range_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			glUniform1i(light_is_on_loc, pointlight.is_on);
			glUniform3f(light_pos_loc, pointlight.transforms.translation.x, pointlight.transforms.translation.y, pointlight.transforms.translation.z);
			glUniform3f(light_diff_loc, pointlight.diffuse.x, pointlight.diffuse.y, pointlight.diffuse.z);
			glUniform1f(light_spec_loc, pointlight.specular);
			glUniform1f(light_intens_loc, pointlight.intensity);
			glUniform1f(light_range_loc, pointlight.range);
		}

		// Spotlights
		unsigned int spotlights_count_loc = glGetUniformLocation(g_mesh_shader.id, "spotlights_count");
		glUniform1i(spotlights_count_loc, g_scene.spotlights.items_count);

		int shadow_map_tex_id = GL_TEXTURE2;
		int shadow_loc_i = 2;

		for (int i = 0; i < g_scene.spotlights.items_count; i++)
		{
			auto spotlight = *(Spotlight*)j_array_get(&g_scene.spotlights, i);

			glm::vec3 spot_dir = get_spotlight_dir(spotlight);
			glm::mat4 light_space_matrix = get_spotlight_light_space_matrix(spotlight);

			sprintf_s(str_value, "spotlights[%d].is_on", i);
			unsigned int sp_is_on_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "spotlights[%d].diffuse", i);
			unsigned int sp_diff_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "spotlights[%d].position", i);
			unsigned int sp_pos_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "spotlights[%d].specular", i);
			unsigned int sp_spec_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "spotlights[%d].range", i);
			unsigned int sp_rng_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "spotlights[%d].direction", i);
			unsigned int sp_dir_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "spotlights[%d].cutoff", i);
			unsigned int sp_cutoff_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "spotlights[%d].outer_cutoff", i);
			unsigned int sp_outer_cutoff_loc = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "spotlights[%d].light_space_matrix", i);
			unsigned int sp_light_matrix = glGetUniformLocation(g_mesh_shader.id, str_value);

			sprintf_s(str_value, "spotlights[%d].shadow_map", i);
			unsigned int sp_shadow_map = glGetUniformLocation(g_mesh_shader.id, str_value);

			float cutoff = glm::cos(glm::radians(spotlight.fov / 2.0f));
			float cos = glm::cos(glm::radians(spotlight.outer_cutoff_fov / 2.0f));
			float outer_cutoff = cutoff - (cutoff - cos);

			glUniform1i(sp_is_on_loc, spotlight.is_on);
			glUniform3f(sp_diff_loc, spotlight.diffuse.x, spotlight.diffuse.y, spotlight.diffuse.z);
			glUniform3f(sp_pos_loc, spotlight.transforms.translation.x, spotlight.transforms.translation.y, spotlight.transforms.translation.z);
			glUniform3f(sp_dir_loc, spot_dir.x, spot_dir.y, spot_dir.z);
			glUniform1f(sp_spec_loc, spotlight.specular);
			glUniform1f(sp_rng_loc, spotlight.range);
			glUniform1f(sp_cutoff_loc, cutoff);
			glUniform1f(sp_outer_cutoff_loc, outer_cutoff);
			glUniformMatrix4fv(sp_light_matrix, 1, GL_FALSE, glm::value_ptr(light_space_matrix));
			glUniform1i(sp_shadow_map, shadow_loc_i++);

			glActiveTexture(shadow_map_tex_id++);
			glBindTexture(GL_TEXTURE_2D, spotlight.shadow_map.texture_gpu_id);
		}
	}

	Material* material = mesh->material;
	glUniform1f(material_shine_loc, material->shininess);
	glUniform1f(specular_multiplier_loc, material->specular_mult);

	s64 draw_indicies = 0;

	if (mesh->mesh_type == MeshType::Plane)
	{
		float vertices[] =
		{
			// Coords			// UV		 // Plane normal
			1.0f, 0.0f, 0.0f,	1.0f, 1.0f,	 0.0f,  1.0f, 0.0f, //  right top
			0.0f, 0.0f, 0.0f,	0.0f, 1.0f,	 0.0f,  1.0f, 0.0f, //  left  top
			0.0f, 0.0f, 1.0f,	0.0f, 0.0f,	 0.0f,  1.0f, 0.0f, //  left  bot

			1.0f, 0.0f, 0.0f,	1.0f, 1.0f,	 0.0f,  1.0f, 0.0f, //  right top
			0.0f, 0.0f, 1.0f,	0.0f, 0.0f,	 0.0f,  1.0f, 0.0f, //  left  bot
			1.0f, 0.0f, 1.0f,	1.0f, 0.0f,	 0.0f,  1.0f, 0.0f, //  right bot
		};

		draw_indicies = 6;
		glBindBuffer(GL_ARRAY_BUFFER, g_mesh_shader.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}
	else if (mesh->mesh_type == MeshType::Cube)
	{
		float vertices[] =
		{
			// Coords				 // UV			 // Plane normal
			// Ceiling
			0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	 0.0f,  1.0f, 0.0f, // top right
		   -0.5f,  0.5f, -0.5f,	 0.0f, 1.0f,	 0.0f,  1.0f, 0.0f, // top left
		   -0.5f,  0.5f,  0.5f,	 0.0f, 0.0f,	 0.0f,  1.0f, 0.0f, // bot left

			0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	 0.0f,  1.0f, 0.0f, // top right
		   -0.5f,  0.5f,  0.5f,	 0.0f, 0.0f,	 0.0f,  1.0f, 0.0f, // bot left 
			0.5f,  0.5f,  0.5f,	 1.0f, 0.0f,	 0.0f,  1.0f, 0.0f, // bot right

			// Floor
		   -0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	 0.0f, -1.0f, 0.0f, // bot left 
			0.5f, -0.5f, -0.5f,	 1.0f, 1.0f,	 0.0f, -1.0f, 0.0f, // top right
			0.5f, -0.5f,  0.5f,	 1.0f, 0.0f,	 0.0f, -1.0f, 0.0f, // bot right

		   -0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	 0.0f, -1.0f, 0.0f, // bot left
		   -0.5f, -0.5f, -0.5f,	 0.0f, 1.0f,	 0.0f, -1.0f, 0.0f, // top left
			0.5f, -0.5f, -0.5f,	 1.0f, 1.0f,	 0.0f, -1.0f, 0.0f, // top right

			// North
		   -0.5f, -0.5f, -0.5f,	 0.0f, 0.0f,	 0.0f,  0.0f, -1.0f, // bot left 
			0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	 0.0f,  0.0f, -1.0f, // top right
			0.5f, -0.5f, -0.5f,	 1.0f, 0.0f,	 0.0f,  0.0f, -1.0f, // bot right

		   -0.5f, -0.5f, -0.5f,	 0.0f, 0.0f,	 0.0f,  0.0f, -1.0f, // bot left
		   -0.5f,  0.5f, -0.5f,	 0.0f, 1.0f,	 0.0f,  0.0f, -1.0f, // top left
			0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	 0.0f,  0.0f, -1.0f, // top right

			// South
			0.5f,  0.5f,  0.5f,	 1.0f, 1.0f,	 0.0f,  0.0f,  1.0f, // top right
		   -0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	 0.0f,  0.0f,  1.0f, // bot left 
			0.5f, -0.5f,  0.5f,	 1.0f, 0.0f,	 0.0f,  0.0f,  1.0f, // bot right

		   -0.5f,  0.5f,  0.5f,	 0.0f, 1.0f,	 0.0f,  0.0f,  1.0f, // top left
		   -0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	 0.0f,  0.0f,  1.0f, // bot left
			0.5f,  0.5f,  0.5f,	 1.0f, 1.0f,	 0.0f,  0.0f,  1.0f, // top right

			// West
		   -0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	-1.0f,  0.0f,  0.0f, // bot left 
		   -0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	-1.0f,  0.0f,  0.0f, // top right
		   -0.5f, -0.5f, -0.5f,	 1.0f, 0.0f,	-1.0f,  0.0f,  0.0f, // bot right

		   -0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	-1.0f,  0.0f,  0.0f, // bot left
		   -0.5f,  0.5f,  0.5f,	 0.0f, 1.0f,	-1.0f,  0.0f,  0.0f, // top left
		   -0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	-1.0f,  0.0f,  0.0f, // top right

		   // East
		   0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	 1.0f,  0.0f,  0.0f, // top right
		   0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	 1.0f,  0.0f,  0.0f, // bot left 
		   0.5f, -0.5f, -0.5f,	 1.0f, 0.0f,	 1.0f,  0.0f,  0.0f, // bot right

		   0.5f,  0.5f,  0.5f,	 0.0f, 1.0f,	 1.0f,  0.0f,  0.0f, // top left
		   0.5f, -0.5f,  0.5f,	 0.0f, 0.0f,	 1.0f,  0.0f,  0.0f, // bot left
		   0.5f,  0.5f, -0.5f,	 1.0f, 1.0f,	 1.0f,  0.0f,  0.0f, // top right
		};

		draw_indicies = 36;
		glBindBuffer(GL_ARRAY_BUFFER, g_mesh_shader.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}

	bool use_specular_texture = mesh->material->specular_texture != nullptr;
	glUniform1i(use_gloss_texture_loc, use_specular_texture);

	if (use_specular_texture)
	{
		glUniform1i(specular_texture_loc, 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mesh->material->specular_texture->gpu_id);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mesh->material->color_texture->gpu_id);

	glDrawArrays(GL_TRIANGLES, 0, draw_indicies);
	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void draw_mesh_wireframe(Mesh* mesh, glm::vec3 color)
{
	glUseProgram(g_wireframe_shader.id);
	glBindVertexArray(g_wireframe_shader.vao);

	glm::mat4 model = get_model_matrix(mesh);

	unsigned int model_loc = glGetUniformLocation(g_wireframe_shader.id, "model");
	unsigned int color_loc = glGetUniformLocation(g_wireframe_shader.id, "color");

	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform3f(color_loc, color.r, color.g, color.b);

	s64 draw_indicies = 0;

	if (mesh->mesh_type == MeshType::Plane)
	{
		float vertices[] =
		{
			// Coords		
			1.0f, 0.0f, 0.0f, // top right
			0.0f, 0.0f, 0.0f, // top left
			0.0f, 0.0f, 1.0f, // bot left

			1.0f, 0.0f, 0.0f, // top right
			0.0f, 0.0f, 1.0f, // bot left 
			1.0f, 0.0f, 1.0f, // bot right

			0.0f, 0.0f, 0.0f, // top left
			1.0f, 0.0f, 0.0f, // top right
			0.0f, 0.0f, 1.0f, // bot left

			0.0f, 0.0f, 1.0f, // bot left 
			1.0f, 0.0f, 0.0f, // top right
			1.0f, 0.0f, 1.0f, // bot right
		};

		draw_indicies = 12;
		glBindBuffer(GL_ARRAY_BUFFER, g_wireframe_shader.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}
	else if (mesh->mesh_type == MeshType::Cube)
	{
		float vertices[] =
		{
			// Coords		
			// Ceiling
			0.5f,  0.5f, -0.5f, // top right
		   -0.5f,  0.5f, -0.5f, // top left
		   -0.5f,  0.5f,  0.5f, // bot left

			0.5f,  0.5f, -0.5f, // top right
		   -0.5f,  0.5f,  0.5f, // bot left 
			0.5f,  0.5f,  0.5f, // bot right

			// Floor		    
		   -0.5f, -0.5f,  0.5f, // bot left 
			0.5f, -0.5f, -0.5f, // top right
			0.5f, -0.5f,  0.5f, // bot right

		   -0.5f, -0.5f,  0.5f, // bot left
		   -0.5f, -0.5f, -0.5f, // top left
			0.5f, -0.5f, -0.5f, // top right

			// North
		   -0.5f, -0.5f, -0.5f, // bot left 
			0.5f,  0.5f, -0.5f, // top right
			0.5f, -0.5f, -0.5f, // bot right

		   -0.5f, -0.5f, -0.5f, // bot left
		   -0.5f,  0.5f, -0.5f, // top left
			0.5f,  0.5f, -0.5f, // top right

			// South
			0.5f,  0.5f,  0.5f, // top right
		   -0.5f, -0.5f,  0.5f, // bot left 
			0.5f, -0.5f,  0.5f, // bot right

		   -0.5f,  0.5f,  0.5f, // top left
		   -0.5f, -0.5f,  0.5f, // bot left
			0.5f,  0.5f,  0.5f, // top right

			// West
		   -0.5f, -0.5f,  0.5f, // bot left 
		   -0.5f,  0.5f, -0.5f, // top right
		   -0.5f, -0.5f, -0.5f, // bot right

		   -0.5f, -0.5f,  0.5f, // bot left
		   -0.5f,  0.5f,  0.5f, // top left
		   -0.5f,  0.5f, -0.5f, // top right

		   // East
		   0.5f,  0.5f, -0.5f, // top right
		   0.5f, -0.5f,  0.5f, // bot left 
		   0.5f, -0.5f, -0.5f, // bot right

		   0.5f,  0.5f,  0.5f, // top left
		   0.5f, -0.5f,  0.5f, // bot left
		   0.5f,  0.5f, -0.5f  // top right
		};

		draw_indicies = 40;
		glBindBuffer(GL_ARRAY_BUFFER, g_wireframe_shader.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1.5f);
	glDrawArrays(GL_TRIANGLES, 0, draw_indicies);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void append_line(glm::vec3 start, glm::vec3 end, glm::vec3 color)
{
	float vertices[LINE_VERICIES] =
	{
		// Coords				   // Color					 
		start.x, start.y, start.z, color.r, color.g, color.b,
		end.x,   end.y,   end.z,   color.r, color.g, color.b,
	};

	s64 bytes_offset = g_lines_buffered * LINE_VERICIES * sizeof(float);
	glBindBuffer(GL_ARRAY_BUFFER, g_line_shader.vbo);
	glBufferSubData(GL_ARRAY_BUFFER, bytes_offset, sizeof(vertices), vertices);
	g_lines_buffered++;
}

void draw_lines(float thickness)
{
	glUseProgram(g_line_shader.id);
	glBindVertexArray(g_line_shader.vao);

	glm::mat4 model = glm::mat4(1.0f);
	unsigned int model_loc = glGetUniformLocation(g_line_shader.id, "model");
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

	s64 indicies = g_lines_buffered * 2;
	glLineWidth(thickness);
	glDrawArrays(GL_LINES, 0, indicies);

	g_lines_buffered = 0;
	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void draw_lines_ontop(float thickness)
{
	glDisable(GL_DEPTH_TEST);
	draw_lines(thickness);
	glEnable(GL_DEPTH_TEST);
}

void init_all_shaders()
{
	// View & Projection UBO
	{
		glGenBuffers(1, &g_view_proj_ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, g_view_proj_ubo);
		glBufferData(GL_UNIFORM_BUFFER, SIZEOF_VIEW_MATRICES, NULL, GL_STATIC_DRAW);
		glBindBufferRange(GL_UNIFORM_BUFFER, 0, g_view_proj_ubo, 0, SIZEOF_VIEW_MATRICES);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	// Skybox
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/skybox_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/skybox_fs.glsl";

		g_skybox_shader = simple_shader_init();

		g_skybox_shader.id = compile_shader(vertex_shader_path, fragment_shader_path, &TEMP_MEMORY);
		{
			glGenVertexArrays(1, &g_skybox_shader.vao);
			glGenBuffers(1, &g_skybox_shader.vbo);

			glBindVertexArray(g_skybox_shader.vao);
			glBindBuffer(GL_ARRAY_BUFFER, g_skybox_shader.vbo);

			float skybox_verticies[] = {
				// Coords          
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				-1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_verticies), skybox_verticies, GL_STATIC_DRAW);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
		}
	}

	// Init billboard shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/billboard_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/billboard_fs.glsl";

		g_billboard_shader = simple_shader_init();

		g_billboard_shader.id = compile_shader(vertex_shader_path, fragment_shader_path, &TEMP_MEMORY);
		{
			glGenVertexArrays(1, &g_billboard_shader.vao);
			glGenBuffers(1, &g_billboard_shader.vbo);

			glBindVertexArray(g_billboard_shader.vao);
			glBindBuffer(GL_ARRAY_BUFFER, g_billboard_shader.vbo);

			float vertices[] =
			{
				// Coords			 // UVs
				-0.5f, -0.5f, 0.0f,	 0.0f, 0.0f, // bottom left
				 0.5f, -0.5f, 0.0f,	 1.0f, 0.0f, // bottom right
				-0.5f,  0.5f, 0.0f,	 0.0f, 1.0f, // top left

				-0.5f,  0.5f, 0.0f,	 0.0f, 1.0f, // top left 
				 0.5f, -0.5f, 0.0f,	 1.0f, 0.0f, // bottom right
				 0.5f,  0.5f, 0.0f,	 1.0f, 1.0f  // top right
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			// UV attribute
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);

			unsigned int view_matrices_loc = glGetUniformBlockIndex(g_billboard_shader.id, "ViewMatrices");
			glUniformBlockBinding(g_billboard_shader.id, view_matrices_loc, 0);
		}
	}

	// Init UI text shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/ui_text_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/ui_text_fs.glsl";

		g_ui_text_shader.id = compile_shader(vertex_shader_path, fragment_shader_path, &TEMP_MEMORY);

		glGenVertexArrays(1, &g_ui_text_shader.vao);
		glGenBuffers(1, &g_ui_text_shader.vbo);

		glBindVertexArray(g_ui_text_shader.vao);
		glBindBuffer(GL_ARRAY_BUFFER, g_ui_text_shader.vbo);

		s64 max_size_bytes = MAX_UI_CHARS_BUFFERED * sizeof(float) * UI_CHAR_VERTICIES;
		glBufferData(GL_ARRAY_BUFFER, max_size_bytes, nullptr, GL_DYNAMIC_DRAW);

		// Coord attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// UV attribute
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

	}

	// Init mesh shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/mesh_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/mesh_fs.glsl";

		g_mesh_shader.id = compile_shader(vertex_shader_path, fragment_shader_path, &TEMP_MEMORY);
		{
			glGenVertexArrays(1, &g_mesh_shader.vao);
			glBindVertexArray(g_mesh_shader.vao);

			glGenBuffers(1, &g_mesh_shader.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, g_mesh_shader.vbo);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			// UV attribute
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);

			// Normal attribute
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
			glEnableVertexAttribArray(2);

			unsigned int view_matrices_loc = glGetUniformBlockIndex(g_mesh_shader.id, "ViewMatrices");
			glUniformBlockBinding(g_mesh_shader.id, view_matrices_loc, 0);
		}
	}

	// Init wireframe shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/wireframe_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/wireframe_fs.glsl";

		g_wireframe_shader.id = compile_shader(vertex_shader_path, fragment_shader_path, &TEMP_MEMORY);

		glGenVertexArrays(1, &g_wireframe_shader.vao);
		glBindVertexArray(g_wireframe_shader.vao);

		glGenBuffers(1, &g_wireframe_shader.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, g_wireframe_shader.vbo);

		// Coord attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		unsigned int view_matrices_loc = glGetUniformBlockIndex(g_wireframe_shader.id, "ViewMatrices");
		glUniformBlockBinding(g_wireframe_shader.id, view_matrices_loc, 0);
	}

	// Init line shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/line_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/line_fs.glsl";

		g_line_shader.id = compile_shader(vertex_shader_path, fragment_shader_path, &TEMP_MEMORY);

		glGenVertexArrays(1, &g_line_shader.vao);
		glBindVertexArray(g_line_shader.vao);
		glGenBuffers(1, &g_line_shader.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, g_line_shader.vbo);

		s64 max_size_bytes = MAX_LINES_BUFFERED * sizeof(float) * LINE_VERICIES;
		glBufferData(GL_ARRAY_BUFFER, max_size_bytes, nullptr, GL_DYNAMIC_DRAW);

		// Coord attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// Color attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		unsigned int view_matrices_loc = glGetUniformBlockIndex(g_line_shader.id, "ViewMatrices");
		glUniformBlockBinding(g_line_shader.id, view_matrices_loc, 0);
	}

	// Init framebuffer shaders
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/framebuffer_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/framebuffer_fs.glsl";

		g_scene_framebuffer_shader.id = compile_shader(vertex_shader_path, fragment_shader_path, &TEMP_MEMORY);

		glGenVertexArrays(1, &g_scene_framebuffer_shader.vao);
		glGenBuffers(1, &g_scene_framebuffer_shader.vbo);
		glBindVertexArray(g_scene_framebuffer_shader.vao);

		float quadVertices[] = {
			// Coords	   // Uv
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};

		glBindBuffer(GL_ARRAY_BUFFER, g_scene_framebuffer_shader.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	}

	// Init shadow map shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/shadow_map_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/shadow_map_fs.glsl";

		g_shdow_map_shader.id = compile_shader(vertex_shader_path, fragment_shader_path, &TEMP_MEMORY);

		glGenVertexArrays(1, &g_shdow_map_shader.vao);
		glGenBuffers(1, &g_shdow_map_shader.vbo);
		glBindVertexArray(g_shdow_map_shader.vao);
		glBindBuffer(GL_ARRAY_BUFFER, g_shdow_map_shader.vbo);

		// Position attribute
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	}

	// Init shadow map debug shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/shadow_map_debug_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/shadow_map_debug_fs.glsl";

		g_shdow_map_debug_shader.id = compile_shader(vertex_shader_path, fragment_shader_path, &TEMP_MEMORY);

		unsigned int vbo;
		glGenVertexArrays(1, &g_shdow_map_debug_shader.vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(g_shdow_map_debug_shader.vao);

		float quadVertices[] = {
			// Coords	   // Uv
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	}
}

void draw_shadow_map_debug_screen(s64 spotlight_index)
{
	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, 500, 500);
	glUseProgram(g_shdow_map_debug_shader.id);
	glBindVertexArray(g_shdow_map_debug_shader.vao);

	float near_plane = 0.25f, far_plane = 15.0f;
	unsigned int near_loc = glGetUniformLocation(g_shdow_map_debug_shader.id, "near_plane");
	glUniform1f(near_loc, near_plane);
	unsigned int far_loc = glGetUniformLocation(g_shdow_map_debug_shader.id, "far_plane");
	glUniform1f(far_loc, far_plane);

	Spotlight* sp = (Spotlight*)j_array_get(&g_scene.spotlights, spotlight_index);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sp->shadow_map.texture_gpu_id);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	g_frame_data.draw_calls++;

	glViewport(0, 0, g_game_metrics.scene_width_px, g_game_metrics.scene_height_px);
	glUseProgram(0);
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void append_ui_text(FontData* font_data, char* text, float pos_x_vw, float pos_y_vh)
{
	auto chars = font_data->char_data.data();
	char* text_string = text;
	int length = strlen(text);

	int text_offset_x_px = 0;
	int text_offset_y_px = 0;
	int line_height_px = font_data->font_height_px;

	glBindBuffer(GL_ARRAY_BUFFER, g_ui_text_shader.vbo);

	for (int i = 0; i < length; i++)
	{
		char current_char = *text_string++;

		if (current_char == '\n')
		{
			text_offset_y_px -= line_height_px;
			text_offset_x_px = 0;
			continue;
		}

		int char_index = static_cast<int>(current_char) - 32;
		CharData current = chars[char_index];

		// Assume font start position is top left corner
		int char_height_px = current.height;
		int char_width_px = current.width;

		int x_start = vw_into_screen_px(pos_x_vw, g_game_metrics.scene_width_px) + current.x_offset + text_offset_x_px;
		int char_y_offset = current.y_offset;
		int y_start = vh_into_screen_px(pos_y_vh, g_game_metrics.scene_height_px) + text_offset_y_px - line_height_px + char_y_offset;

		float x0 = normalize_screen_px_to_ndc(x_start, g_game_metrics.scene_width_px);
		float y0 = normalize_screen_px_to_ndc(y_start, g_game_metrics.scene_height_px);

		float x1 = normalize_screen_px_to_ndc(x_start + char_width_px, g_game_metrics.scene_width_px);
		float y1 = normalize_screen_px_to_ndc(y_start + char_height_px, g_game_metrics.scene_height_px);

		float vertices[UI_CHAR_VERTICIES] =
		{
			// Coords			// UV
			x0, y1, 0.0f,		current.UV_x0, current.UV_y1, // top left
			x0, y0, 0.0f,		current.UV_x0, current.UV_y0, // bottom left
			x1, y0, 0.0f,		current.UV_x1, current.UV_y0, // bottom right

			x1, y1, 0.0f,		current.UV_x1, current.UV_y1, // top right
			x0, y1, 0.0f,		current.UV_x0, current.UV_y1, // top left 
			x1, y0, 0.0f,		current.UV_x1, current.UV_y0  // bottom right
		};

		s64 bytes_offset = g_ui_chars_buffered * UI_CHAR_VERTICIES * sizeof(float);
		glBufferSubData(GL_ARRAY_BUFFER, bytes_offset, sizeof(vertices), vertices);

		g_ui_chars_buffered++;
		text_offset_x_px += current.advance;
		text++;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw_ui_text(FontData* font_data, float red, float green, float blue)
{
	glUseProgram(g_ui_text_shader.id);
	glBindVertexArray(g_ui_text_shader.vao);

	int color_uniform = glGetUniformLocation(g_ui_text_shader.id, "textColor");
	glUniform3f(color_uniform, red, green, blue);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, font_data->texture_id);

	s64 indicies = g_ui_chars_buffered * 6;
	glDrawArrays(GL_TRIANGLES, 0, indicies);

	g_ui_chars_buffered = 0;
	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

unsigned int load_cubemap()
{
	unsigned int texture_id;
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

	const s64 cube_faces = 6;
	const char* cubemaps[cube_faces] = {
		"G:\\projects\\game\\Engine3D\\resources\\skybox\\right.jpg",
		"G:\\projects\\game\\Engine3D\\resources\\skybox\\left.jpg",
		"G:\\projects\\game\\Engine3D\\resources\\skybox\\top.jpg",
		"G:\\projects\\game\\Engine3D\\resources\\skybox\\bottom.jpg",
		"G:\\projects\\game\\Engine3D\\resources\\skybox\\front.jpg",
		"G:\\projects\\game\\Engine3D\\resources\\skybox\\back.jpg",
	};

	flip_vertical_image_load(false);

	for (unsigned int i = 0; i < cube_faces; i++)
	{
		ImageData data = load_image_data(const_cast<char*>(cubemaps[i]));
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, data.width_px, data.height_px, 0, GL_RGB, GL_UNSIGNED_BYTE, data.image_data);
		free_loaded_image(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return texture_id;
}

void draw_skybox()
{
	glDepthMask(GL_FALSE);
	glUseProgram(g_skybox_shader.id);

	glm::mat4 projection = get_projection_matrix();
	glm::mat4 view = get_view_matrix();
	glm::mat4 view_matrix = glm::mat3(view);

	unsigned int view_loc = glGetUniformLocation(g_skybox_shader.id, "view");
	unsigned int projection_loc = glGetUniformLocation(g_skybox_shader.id, "projection");

	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));

	glBindVertexArray(g_skybox_shader.vao);

	glBindTexture(GL_TEXTURE_CUBE_MAP, g_skybox_cubemap);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);
}

int load_image_into_texture_id(char* image_path)
{
	unsigned int texture;
	flip_vertical_image_load(true);
	ImageData im_data = load_image_data(image_path);

	ASSERT_TRUE(im_data.channels == 3 || im_data.channels == 4, "Image format is RGB or RGBA");

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GLint filtering_mode = g_use_linear_texture_filtering ? GL_LINEAR : GL_NEAREST;

	GLint mipmap_filtering_mode = g_use_linear_texture_filtering
		? GL_LINEAR_MIPMAP_LINEAR
		: GL_NEAREST_MIPMAP_NEAREST;

	if (g_generate_texture_mipmaps) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmap_filtering_mode);
	else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering_mode);

	GLint use_format = im_data.channels == 3 ? GL_RGB : GL_RGBA;
	GLint internal_format;

	if (g_load_texture_sRGB) internal_format = im_data.channels == 3 ? GL_SRGB : GL_SRGB_ALPHA;
	else internal_format = im_data.channels == 3 ? GL_RGB : GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, im_data.width_px, im_data.height_px, 0, use_format, GL_UNSIGNED_BYTE, im_data.image_data);

	if (g_generate_texture_mipmaps) glGenerateMipmap(GL_TEXTURE_2D);

	free_loaded_image(im_data);
	return texture;
}
