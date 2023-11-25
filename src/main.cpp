#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H  

#include <array>
#include <iostream>
#include <filesystem>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "globals.h"
#include "editor.h"
#include "structs.h"
#include "scene.h"
#include "utils.h"
#include "j_array.h"
#include "j_assert.h"
#include "j_buffers.h"
#include "j_files.h"
#include "j_imgui.h"
#include "j_map.h"
#include "j_render.h"
#include "j_strings.h"

Framebuffer g_editor_framebuffer;
Texture pointlight_texture;
Texture spotlight_texture;

void load_font(FontData* font_data, int font_height_px, const char* font_path)
{
	FT_Library ft_lib;
	FT_Face ft_face;

	FT_Error init_ft_err = FT_Init_FreeType(&ft_lib);
	ASSERT_TRUE(init_ft_err == 0, "Init FreeType Library");

	FT_Error new_face_err = FT_New_Face(ft_lib, font_path, 0, &ft_face);
	ASSERT_TRUE(new_face_err == 0, "Load FreeType font face");

	FT_Set_Pixel_Sizes(ft_face, 0, font_height_px);
	font_data->font_height_px = font_height_px;

	unsigned int bitmap_width = 0;
	unsigned int bitmap_height = 0;

	constexpr int starting_char = 33; // ('!')
	constexpr int last_char = 128;

	int spacebar_width = font_height_px / 4;

	// Add spacebar
	bitmap_width += spacebar_width;

	for (int c = starting_char; c < last_char; c++)
	{
		char character = static_cast<char>(c);

		FT_Error load_char_err = FT_Load_Char(ft_face, character, FT_LOAD_RENDER);
		ASSERT_TRUE(load_char_err == 0, "Load FreeType Glyph");

		unsigned int glyph_width = ft_face->glyph->bitmap.width;
		unsigned int glyph_height = ft_face->glyph->bitmap.rows;

		if (bitmap_height < glyph_height)
		{
			bitmap_height = glyph_height;
		}

		bitmap_width += glyph_width;
	}

	int bitmap_size = bitmap_width * bitmap_height;
	ASSERT_TRUE(bitmap_size < g_temp_memory.size, "Font bitmap buffer fits");

	byte* bitmap_memory = g_temp_memory.memory;

	// Add spacebar
	{
		for (int y = 0; y < bitmap_height; y++)
		{
			int src_index = ((bitmap_height - 1) * spacebar_width) - y * spacebar_width;
			int dest_index = y * bitmap_width;
			memset(&bitmap_memory[dest_index], 0x00, spacebar_width);
		}

		CharData new_char_data = { 0 };
		new_char_data.character = static_cast<char>(' ');
		new_char_data.width = spacebar_width;
		new_char_data.advance = spacebar_width;
		new_char_data.height = font_height_px;
		new_char_data.UV_x0 = 0.0f;
		new_char_data.UV_y0 = 1.0f;
		new_char_data.UV_x1 = normalize_value(spacebar_width, bitmap_width, 1.0f);
		new_char_data.UV_y1 = 1.0f;
		font_data->char_data[0] = new_char_data;
	}

	int bitmap_x_offset = spacebar_width;
	int char_data_index = 1;

	for (int c = starting_char; c < last_char; c++)
	{
		int character = c;

		FT_Error load_char_err = FT_Load_Char(ft_face, character, FT_LOAD_RENDER);
		ASSERT_TRUE(load_char_err == 0, "Load FreeType Glyph");

		unsigned int glyph_width = ft_face->glyph->bitmap.width;
		unsigned int glyph_height = ft_face->glyph->bitmap.rows;

		int x_offset = ft_face->glyph->bitmap_left;
		int y_offset = ft_face->glyph->bitmap_top - ft_face->glyph->bitmap.rows;

		for (int y = 0; y < glyph_height; y++)
		{
			int src_index = ((glyph_height - 1) * glyph_width) - y * glyph_width;
			int dest_index = y * bitmap_width + bitmap_x_offset;
			memcpy(&bitmap_memory[dest_index], &ft_face->glyph->bitmap.buffer[src_index], glyph_width);
		}

		// To advance cursors for next glyph
		// bitshift by 6 to get value in pixels
		// (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
		// (note that advance is number of 1/64 pixels)
		int advance = (ft_face->glyph->advance.x >> 6);

		CharData new_char_data = { 0 };
		new_char_data.character = static_cast<char>(c);

		new_char_data.width = glyph_width;
		new_char_data.height = glyph_height;
		new_char_data.x_offset = x_offset;
		new_char_data.y_offset = y_offset;
		new_char_data.advance = advance;

		float atlas_width = static_cast<float>(bitmap_width);
		float atlas_height = static_cast<float>(bitmap_height);

		new_char_data.UV_x0 = normalize_value(bitmap_x_offset, atlas_width, 1.0f);
		new_char_data.UV_y0 = normalize_value(0.0f, atlas_height, 1.0f);

		float x1 = bitmap_x_offset + glyph_width;
		new_char_data.UV_x1 = normalize_value(x1, atlas_width, 1.0f);
		new_char_data.UV_y1 = normalize_value(glyph_height, atlas_height, 1.0f);

		font_data->char_data[char_data_index++] = new_char_data;

		bitmap_x_offset += glyph_width;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLuint prev_texture = font_data->texture_id;
	glDeleteTextures(1, &prev_texture);

	GLuint new_texture;
	glGenTextures(1, &new_texture);

	font_data->texture_id = static_cast<int>(new_texture);
	glBindTexture(GL_TEXTURE_2D, font_data->texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, bitmap_width, bitmap_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap_memory);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	resize_windows_area_settings(width, height);
	glViewport(0, 0, g_game_metrics.scene_width_px, g_game_metrics.scene_height_px);

	glGenFramebuffers(1, &g_scene_framebuffer.id);
	glBindFramebuffer(GL_FRAMEBUFFER, g_scene_framebuffer.id);
	init_framebuffer_resize(&g_scene_framebuffer.texture_gpu_id, &g_scene_framebuffer.renderbuffer);

	glGenFramebuffers(1, &g_editor_framebuffer.id);
	glBindFramebuffer(GL_FRAMEBUFFER, g_editor_framebuffer.id);
	init_framebuffer_resize(&g_editor_framebuffer.texture_gpu_id, &g_editor_framebuffer.renderbuffer);

	int font_height_px = normalize_value(debug_font_vh, 100.0f, (float)height);
	load_font(&g_debug_font, font_height_px, g_debug_font_path);
}

void mouse_move_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	g_frame_data.mouse_move_x = xpos - g_frame_data.prev_mouse_x;
	g_frame_data.mouse_move_y = g_frame_data.prev_mouse_y - ypos;

	g_frame_data.prev_mouse_x = xpos;
	g_frame_data.prev_mouse_y = ypos;
}

void set_button_state(GLFWwindow* window, ButtonState* button)
{
	int key_state = glfwGetKey(window, button->key);
	button->pressed = !button->is_down && key_state == GLFW_PRESS;
	button->is_down = key_state == GLFW_PRESS;
}

int main(int argc, char* argv[])
{
	init_memory_buffers();

	// Init window and context
	{
		int glfw_init_result = glfwInit();
		ASSERT_TRUE(glfw_init_result == GLFW_TRUE, "glfw init");

		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		g_window = glfwCreateWindow(640, 400, "JMF Engine3D", NULL, NULL);
		ASSERT_TRUE(g_window, "Window creation");

		glfwMakeContextCurrent(g_window);
		glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);
		glfwSetCursorPosCallback(g_window, mouse_move_callback);

		int glew_init_result = glewInit();
		ASSERT_TRUE(glew_init_result == GLEW_OK, "glew init");
	}

	init_imgui();

	// Init game inputs
	{
		g_inputs.as_struct.mouse1 = ButtonState{ .key = GLFW_MOUSE_BUTTON_1 };
		g_inputs.as_struct.mouse2 = ButtonState{ .key = GLFW_MOUSE_BUTTON_2 };

		g_inputs.as_struct.q = ButtonState{ .key = GLFW_KEY_Q };
		g_inputs.as_struct.w = ButtonState{ .key = GLFW_KEY_W };
		g_inputs.as_struct.e = ButtonState{ .key = GLFW_KEY_E };
		g_inputs.as_struct.r = ButtonState{ .key = GLFW_KEY_R };
		g_inputs.as_struct.a = ButtonState{ .key = GLFW_KEY_A };
		g_inputs.as_struct.s = ButtonState{ .key = GLFW_KEY_S };
		g_inputs.as_struct.d = ButtonState{ .key = GLFW_KEY_D };
		g_inputs.as_struct.f = ButtonState{ .key = GLFW_KEY_F };
		g_inputs.as_struct.z = ButtonState{ .key = GLFW_KEY_Z };
		g_inputs.as_struct.x = ButtonState{ .key = GLFW_KEY_X };
		g_inputs.as_struct.c = ButtonState{ .key = GLFW_KEY_C };
		g_inputs.as_struct.v = ButtonState{ .key = GLFW_KEY_V };
		g_inputs.as_struct.y = ButtonState{ .key = GLFW_KEY_Y };
		g_inputs.as_struct.esc = ButtonState{ .key = GLFW_KEY_ESCAPE };
		g_inputs.as_struct.plus = ButtonState{ .key = GLFW_KEY_KP_ADD };
		g_inputs.as_struct.minus = ButtonState{ .key = GLFW_KEY_KP_SUBTRACT };
		g_inputs.as_struct.del = ButtonState{ .key = GLFW_KEY_DELETE };
		g_inputs.as_struct.left_ctrl = ButtonState{ .key = GLFW_KEY_LEFT_CONTROL };
		g_inputs.as_struct.space = ButtonState{ .key = GLFW_KEY_SPACE };
	}

	init_all_shaders();

	g_pp_settings = post_processings_init();

	// Load assets
	{
		FILE* file;
		int success = fopen_s(&file, ASSETS_MANIFEST_PATH, "r");
		ASSERT_TRUE(success == 0, "File opened");

		s64 materials_count = 0;
		s64 material_ids[32] = {};
		char* material_str_names[32] = {};
		char line_buffer[256];

		for (;;)
		{
			int got_line = fgets(line_buffer, sizeof(line_buffer), file) != NULL;
			if (got_line == NULL) break;

			if (strcmp(line_buffer, "materials/\n") == 0)
			{
				g_material_names = j_strings_init(2048, (char*)g_material_names_memory.memory);

				for (;;)
				{
					got_line = fgets(line_buffer, sizeof(line_buffer), file) != NULL;
					if (got_line == NULL) break;

					bool starts_new_material = line_buffer[0] == '-' && line_buffer[1] == ' ';
					if (!starts_new_material) break;

					char* endptr;
					long material_id = strtol(&line_buffer[2], &endptr, 10);

					char* material_name = ++endptr;
					str_trim_from_char(material_name, '\n');
					char* str_ptr = j_strings_add(&g_material_names, material_name);

					material_str_names[materials_count] = str_ptr;
					material_ids[materials_count] = material_id;
					materials_count++;
				}
			}
		}

		fclose(file);

		for (int i = 0; i < materials_count; i++)
		{
			char filepath[FILE_PATH_LEN] = {};

			sprintf_s(filepath, "%s%s%s", MATERIALS_DIR_PATH, material_str_names[i], const_cast<char*>(".png"));
			g_load_texture_sRGB = true;
			Texture color_texture = texture_load_from_filepath(filepath);

			sprintf_s(filepath, "%s%s%s", MATERIALS_DIR_PATH, material_str_names[i], const_cast<char*>("_specular.png"));
			g_load_texture_sRGB = false;
			Texture specular_texture = texture_load_from_filepath(filepath);

			s64 id = material_ids[i];
			Texture* color_texture_prt    = (Texture*)j_array_add(&g_textures, (byte*)&color_texture);
			Texture* specular_texture_ptr = (Texture*)j_array_add(&g_textures, (byte*)&specular_texture);

			Material new_material = material_init();
			new_material.color_texture = color_texture_prt;
			new_material.specular_texture = specular_texture_ptr;
			new_material.id = id;

			Material* new_material_ptr = (Material*)j_array_add(&g_materials, (byte*)&new_material);
			JMapItem_S64_Ptr map_item = {
				.key = id,
				.value = (byte*)new_material_ptr
			};
			jmap_add(&materials_id_map, map_item);
		}
	}

	// Texture array (test)
	{
		glGenTextures(1, &g_texture_arr_01);
		glBindTexture(GL_TEXTURE_2D_ARRAY, g_texture_arr_01);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_SRGB8, TEXTURE_SIZE_1K, TEXTURE_SIZE_1K, 24);

		flip_vertical_image_load(true);

		ImageData im_data =  load_image_data(const_cast<char*>("G:\\projects\\game\\Engine3D\\resources\\materials\\zellige_squares.png"));
		ASSERT_TRUE(im_data.channels == 3 || im_data.channels == 4, "Image format is RGB or RGBA");

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, TEXTURE_SIZE_1K, TEXTURE_SIZE_1K, 1, GL_RGB, GL_UNSIGNED_BYTE, im_data.image_data);

		free_loaded_image(im_data);

		im_data = load_image_data(const_cast<char*>("G:\\projects\\game\\Engine3D\\resources\\materials\\wood_floor_ash_white.png"));
		ASSERT_TRUE(im_data.channels == 3 || im_data.channels == 4, "Image format is RGB or RGBA");

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, TEXTURE_SIZE_1K, TEXTURE_SIZE_1K, 1, GL_RGB, GL_UNSIGNED_BYTE, im_data.image_data);
		free_loaded_image(im_data);

		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	// Load core textures
	{
		g_use_linear_texture_filtering = true;
		g_generate_texture_mipmaps = true;
		g_load_texture_sRGB = false;

		char path_str[FILE_PATH_LEN] = { 0 };
		strcpy_s(path_str, pointlight_image_path);
		pointlight_texture = texture_load_from_filepath(path_str);

		strcpy_s(path_str, spotlight_image_path);
		spotlight_texture = texture_load_from_filepath(path_str);

		g_skybox_cubemap = load_cubemap();
	}

	// Load fonts
	{
		g_use_linear_texture_filtering = false;
		g_generate_texture_mipmaps = false;
		g_load_texture_sRGB = false;
		int font_height_px = normalize_value(debug_font_vh, 100.0f, g_game_metrics.game_height_px);
		load_font(&g_debug_font, font_height_px, g_debug_font_path);
	}

	glfwSetWindowSize(g_window, g_user_settings.window_size_px[0], g_user_settings.window_size_px[1]);

	// Init framebuffers
	{
		glGenFramebuffers(1, &g_scene_framebuffer.id);
		glBindFramebuffer(GL_FRAMEBUFFER, g_scene_framebuffer.id);
		init_framebuffer_resize(&g_scene_framebuffer.texture_gpu_id, &g_scene_framebuffer.renderbuffer);

		glGenFramebuffers(1, &g_editor_framebuffer.id);
		glBindFramebuffer(GL_FRAMEBUFFER, g_editor_framebuffer.id);
		init_framebuffer_resize(&g_editor_framebuffer.texture_gpu_id, &g_editor_framebuffer.renderbuffer);
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);

	new_scene();

	while (!glfwWindowShouldClose(g_window))
	{
		glfwPollEvents();

		imgui_new_frame();

		right_hand_editor_panel();

		// Register inputs
		{
			int key_state;
			int buttons_count = sizeof(g_inputs) / sizeof(ButtonState);
			g_frame_data.mouse_clicked = false;

			if (glfwGetKey(g_window, g_inputs.as_struct.esc.key) == GLFW_PRESS) glfwSetWindowShouldClose(g_window, true);

			key_state = glfwGetMouseButton(g_window, g_inputs.as_struct.mouse1.key);
			g_inputs.as_struct.mouse1.pressed = !g_inputs.as_struct.mouse1.is_down && key_state == GLFW_PRESS;
			g_inputs.as_struct.mouse1.is_down = key_state == GLFW_PRESS;

			key_state = glfwGetMouseButton(g_window, g_inputs.as_struct.mouse2.key);
			g_inputs.as_struct.mouse2.pressed = !g_inputs.as_struct.mouse2.is_down && key_state == GLFW_PRESS;
			g_inputs.as_struct.mouse2.is_down = key_state == GLFW_PRESS;

			for (int i = 2; i < buttons_count - 1; i++) // Skip first two mouse buttons
			{
				ButtonState* button = &g_inputs.as_array[i];
				set_button_state(g_window, button);
			}
		}

		// Logic
		g_game_metrics.prev_frame_game_time = g_game_metrics.game_time;
		g_game_metrics.game_time = glfwGetTime();
		g_frame_data.deltatime = (g_game_metrics.game_time - g_game_metrics.prev_frame_game_time);

		double xpos, ypos;
		glfwGetCursorPos(g_window, &xpos, &ypos);

		g_frame_data.mouse_x = xpos;
		g_frame_data.mouse_y = ypos;

		s64 current_game_second = (s64)g_game_metrics.game_time;

		if (0 < current_game_second - g_game_metrics.fps_prev_second)
		{
			g_game_metrics.fps = g_game_metrics.fps_frames;
			g_game_metrics.fps_frames = 0;
			g_game_metrics.fps_prev_second = current_game_second;
		}

		// Save scene
		if (g_inputs.as_struct.left_ctrl.is_down && g_inputs.as_struct.s.pressed) save_all();

		// Get tranformation mode
		if (!g_camera_move_mode) g_transform_mode.mode = get_curr_transformation_mode();

		// Mouse 1 => select object
		if (g_inputs.as_struct.mouse1.pressed)
		{
			g_frame_data.mouse_clicked = true;

			if (clicked_scene_space((int)g_frame_data.mouse_x, (int)g_frame_data.mouse_y))
			{
				glm::vec3 ray_origin = g_scene_camera.position;
				glm::vec3 ray_direction = get_camera_ray_from_scene_px((int)xpos, (int)ypos);

				s64 object_types_count = 4;
				ObjectType select_types[] = { ObjectType::Plane, ObjectType::Cube, ObjectType::Pointlight, ObjectType::Spotlight };
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
		}

		bool camera_mode_start = g_inputs.as_struct.mouse2.pressed
			&& clicked_scene_space((int)g_frame_data.mouse_x, (int)g_frame_data.mouse_y);

		// Check camero move mode
		if (camera_mode_start) g_camera_move_mode = true;
		else if (!g_inputs.as_struct.mouse2.is_down) g_camera_move_mode = false;

		// Handle camera mode mode
		if (g_camera_move_mode)
		{
			glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			g_frame_data.mouse_move_x *= g_scene_camera.look_sensitivity;
			g_frame_data.mouse_move_y *= g_scene_camera.look_sensitivity;

			g_scene_camera.yaw += g_frame_data.mouse_move_x;
			g_scene_camera.pitch += g_frame_data.mouse_move_y;

			if (g_scene_camera.pitch > 89.0f)
			{
				g_scene_camera.pitch = 89.0f;
			}
			else if (g_scene_camera.pitch < -89.0f)
			{
				g_scene_camera.pitch = -89.0f;
			}

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
		else glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		// Duplicate / Delete object
		if (has_object_selection())
		{
			if (g_inputs.as_struct.del.pressed) delete_selected_object();
			else if (g_inputs.as_struct.left_ctrl.is_down && g_inputs.as_struct.d.pressed) duplicate_selected_object();
		}

		// Check transformation mode start
		if (has_object_selection()
			&& (g_inputs.as_struct.x.pressed || g_inputs.as_struct.z.pressed || g_inputs.as_struct.c.pressed))
		{
			g_transform_mode.is_active = try_init_transform_mode();
		}
		else if (g_transform_mode.is_active
			&& !g_inputs.as_struct.z.is_down && !g_inputs.as_struct.x.is_down && !g_inputs.as_struct.c.is_down)
		{
			g_transform_mode.is_active = false;
		}
		// Handle transformation mode
		else if (g_transform_mode.is_active)
		{
			glm::vec3 intersection_point;
			g_transform_mode.transform_ray = get_camera_ray_from_scene_px((int)xpos, (int)ypos);

			Transforms* selected_t_ptr = get_selected_object_transforms();

			bool intersection = calculate_plane_ray_intersection(
				g_transform_mode.transform_plane_normal,
				selected_t_ptr->translation,
				g_scene_camera.position,
				g_transform_mode.transform_ray,
				intersection_point);

			g_transform_mode.new_intersection_point = intersection_point;

			if (intersection && g_transform_mode.mode == TransformMode::Translate)
			{
				glm::vec3 travel_dist = intersection_point - g_transform_mode.prev_intersection_point;
				vec3_add_for_axis(g_transform_mode.new_tranformation, travel_dist, g_transform_mode.axis);
				g_transform_mode.prev_intersection_point = intersection_point;

				if (0.0f < g_user_settings.transform_clip)
					selected_t_ptr->translation = clip_vec3(g_transform_mode.new_tranformation, g_user_settings.transform_clip);
				else selected_t_ptr->translation = g_transform_mode.new_tranformation;
			}
			else if (intersection && g_transform_mode.mode == TransformMode::Scale)
			{
				glm::mat4 rotation_mat4 = get_rotation_matrix(selected_t_ptr->rotation);
				glm::vec3 used_normal = get_normal_for_axis(g_transform_mode.axis);

				used_normal = rotation_mat4 * glm::vec4(used_normal, 1.0f);
				glm::vec3 point_on_scale_plane = closest_point_on_plane(g_transform_mode.new_intersection_point, selected_t_ptr->translation, used_normal);
				g_transform_mode.new_intersection_point = point_on_scale_plane;

				// Reverse the rotation by applying the inverse rotation matrix to the vector
				glm::vec3 reversedVector  = glm::inverse(rotation_mat4) * glm::vec4(point_on_scale_plane, 1.0f);
				glm::vec3 reversedVector2 = glm::inverse(rotation_mat4) * glm::vec4(g_transform_mode.prev_intersection_point, 1.0f);

				glm::vec3 travel_dist = reversedVector - reversedVector2;
				g_transform_mode.prev_intersection_point = point_on_scale_plane;

				vec3_add_for_axis(g_transform_mode.new_tranformation, travel_dist, g_transform_mode.axis);

				if (0.0f < g_user_settings.transform_clip)
					selected_t_ptr->scale = clip_vec3(g_transform_mode.new_tranformation, g_user_settings.transform_clip);
				else selected_t_ptr->scale = g_transform_mode.new_tranformation;

				if (selected_t_ptr->scale.x < 0.01f) selected_t_ptr->scale.x = 0.01f;
				if (selected_t_ptr->scale.y < 0.01f) selected_t_ptr->scale.y = 0.01f;
				if (selected_t_ptr->scale.z < 0.01f) selected_t_ptr->scale.z = 0.01f;
			}
			else if (intersection && g_transform_mode.mode == TransformMode::Rotate)
			{
				auto new_line = g_transform_mode.new_intersection_point - selected_t_ptr->translation;

				if (0.15f < glm::length(new_line))
				{
					glm::vec3 prev_rotation_dir = glm::normalize(g_transform_mode.prev_intersection_point - selected_t_ptr->translation);
					glm::vec3 new_rotation_dir = glm::normalize(new_line);

					g_transform_mode.prev_intersection_point = g_transform_mode.new_intersection_point;

					bool equal = glm::all(glm::equal(prev_rotation_dir, new_rotation_dir));

					if (!equal)
					{
						// Calculate the rotation axis using the cross product of the unit vectors
						glm::vec3 rotation_axis = glm::normalize(glm::cross(prev_rotation_dir, new_rotation_dir));

						// Calculate the rotation angle in radians
						float angle = glm::acos(glm::dot(glm::normalize(prev_rotation_dir), glm::normalize(new_rotation_dir)));

						glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1.0f), angle, rotation_axis);
						glm::quat rotation_quaternion = rotation_matrix;
						glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(rotation_quaternion));

						if (!(glm::isnan(eulerAngles.x) || glm::isnan(eulerAngles.y) || glm::isnan(eulerAngles.z)))
						{
							g_transform_mode.new_tranformation += eulerAngles;

							if (0.0f < g_user_settings.transform_rotation_clip)
								selected_t_ptr->rotation = clip_vec3(g_transform_mode.new_tranformation, g_user_settings.transform_rotation_clip);
							else selected_t_ptr->rotation = g_transform_mode.new_tranformation;

							selected_t_ptr->rotation.x = float_modulus_operation(selected_t_ptr->rotation.x, 360.0f);
							selected_t_ptr->rotation.y = float_modulus_operation(selected_t_ptr->rotation.y, 360.0f);
							selected_t_ptr->rotation.z = float_modulus_operation(selected_t_ptr->rotation.z, 360.0f);
						}
					}
				}
			}
		}

		// -------------
		// Draw OpenGL

		// Update UBOs
		{
			glBindBuffer(GL_UNIFORM_BUFFER, g_view_proj_ubo);

			auto projection = get_projection_matrix();
			auto view = get_view_matrix();

			glBufferSubData(GL_UNIFORM_BUFFER, 0,				  sizeof(glm::mat4), glm::value_ptr(projection));
			glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));

			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		// Shadow map framebuffers
		{
			glUseProgram(g_shdow_map_shader.id);
			glViewport(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

			for (int i = 0; i < g_scene.spotlights.items_count; i++)
			{
				Spotlight* spotlight = (Spotlight*)j_array_get(&g_scene.spotlights, i);
				glm::mat4 light_space_matrix = get_spotlight_light_space_matrix(*spotlight);

				unsigned int light_matrix_loc = glGetUniformLocation(g_shdow_map_shader.id, "lightSpaceMatrix");
				glUniformMatrix4fv(light_matrix_loc, 1, GL_FALSE, glm::value_ptr(light_space_matrix));

				glBindFramebuffer(GL_FRAMEBUFFER, spotlight->shadow_map.id);
				glClear(GL_DEPTH_BUFFER_BIT);

				glUseProgram(g_shdow_map_shader.id);
				glBindVertexArray(g_shdow_map_shader.vao);
				glCullFace(GL_BACK);

				// Disable for planes
				for (int i = 0; i < g_scene.planes.items_count; i++)
				{
					Mesh plane = *(Mesh*)j_array_get(&g_scene.planes, i);
					draw_mesh_shadow_map(&plane, spotlight);
				}

				glCullFace(GL_FRONT);

				for (int i = 0; i < g_scene.meshes.items_count; i++)
				{
					Mesh mesh = *(Mesh*)j_array_get(&g_scene.meshes, i);
					draw_mesh_shadow_map(&mesh, spotlight);
				}
			}

			glUseProgram(0);
			glBindVertexArray(0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, g_game_metrics.scene_width_px, g_game_metrics.scene_height_px);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glCullFace(GL_BACK);
		}

		// Scene framebuffer
		{
			glBindFramebuffer(GL_FRAMEBUFFER, g_scene_framebuffer.id);
			glEnable(GL_DEPTH_TEST);
			glClearColor(0.34f, 0.44f, 0.42f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			draw_skybox();

			// Coordinate lines
			append_line(glm::vec3(-1000.0f, 0.0f, 0.0f), glm::vec3(1000.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			append_line(glm::vec3(0.0f, -1000.0f, 0.0f), glm::vec3(0.0f, 1000.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			append_line(glm::vec3(0.0f, 0.0f, -1000.0f), glm::vec3(0.0f, 0.0f, 1000.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			draw_lines(1.0f);

			for (int i = 0; i < g_scene.planes.items_count; i++)
			{
				Mesh plane = *(Mesh*)j_array_get(&g_scene.planes, i);
				draw_mesh(&plane);
			}

			for (int i = 0; i < g_scene.meshes.items_count; i++)
			{
				Mesh mesh = *(Mesh*)j_array_get(&g_scene.meshes, i);
				draw_mesh(&mesh);
			}

			// Pointlights
			for (int i = 0; i < g_scene.pointlights.items_count; i++)
			{
				auto light = *(Pointlight*)j_array_get(&g_scene.pointlights, i);
				draw_billboard(light.transforms.translation, pointlight_texture, 0.5f);
			}

			// Spotlights
			for (int i = 0; i < g_scene.spotlights.items_count; i++)
			{
				auto spotlight = *(Spotlight*)j_array_get(&g_scene.spotlights, i);
				draw_billboard(spotlight.transforms.translation, spotlight_texture, 0.5f);
				glm::vec3 sp_dir = get_spotlight_dir(spotlight);
				append_line(spotlight.transforms.translation, spotlight.transforms.translation + sp_dir, spotlight.diffuse);
			}

			draw_lines(2.0f);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// Editor framebuffer
		{
			glBindFramebuffer(GL_FRAMEBUFFER, g_editor_framebuffer.id);
			glEnable(GL_DEPTH_TEST);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Transformation mode debug lines
			if (has_object_selection() && g_transform_mode.is_active)
			{
				glm::vec3 line_color;

				if (g_transform_mode.axis == Axis::X) line_color = glm::vec3(1.0f, 0.2f, 0.2f);
				if (g_transform_mode.axis == Axis::Y) line_color = glm::vec3(0.2f, 1.0f, 0.2f);
				if (g_transform_mode.axis == Axis::Z) line_color = glm::vec3(0.2f, 0.2f, 1.0f);

				if (g_transform_mode.mode == TransformMode::Translate)
				{
					append_line(g_transform_mode.new_intersection_point, g_transform_mode.new_tranformation, line_color);
					draw_lines(1.0f);
				}
				else if (g_transform_mode.mode == TransformMode::Rotate)
				{
					auto transforms = get_selected_object_transforms();
					append_line(g_transform_mode.new_intersection_point, transforms->translation, line_color);
					draw_lines(1.0f);
				}
				else if (g_transform_mode.mode == TransformMode::Scale)
				{
					auto transforms = get_selected_object_transforms();
					append_line(g_transform_mode.new_intersection_point, transforms->translation, line_color);
					draw_lines(1.0f);
				}
			}

			// Draw selection
			if (has_object_selection())
			{
				if (is_primitive(g_selected_object.type))
				{
					Mesh* selected_mesh = (Mesh*)get_selected_object_ptr();
					draw_mesh_wireframe(selected_mesh, glm::vec3(1.0f));
					draw_selection_arrows(selected_mesh->transforms.translation);
				}
				else if (g_selected_object.type == ObjectType::Pointlight)
				{
					Pointlight* selected_light = (Pointlight*)get_selected_object_ptr();
					Mesh as_cube = {};
					as_cube.mesh_type = MeshType::Cube;
					as_cube.transforms.scale = glm::vec3(0.35f);
					as_cube.transforms.translation = selected_light->transforms.translation;
					draw_mesh_wireframe(&as_cube, selected_light->diffuse);
					draw_selection_arrows(selected_light->transforms.translation);
				}
				else if (g_selected_object.type == ObjectType::Spotlight)
				{
					Spotlight* selected_light = (Spotlight*)get_selected_object_ptr();
					Mesh as_cube = {};
					as_cube.mesh_type = MeshType::Cube;
					as_cube.transforms.scale = glm::vec3(0.35f);
					as_cube.transforms.translation = selected_light->transforms.translation;
					draw_mesh_wireframe(&as_cube, selected_light->diffuse);
					draw_selection_arrows(selected_light->transforms.translation);
				}
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// -------------------
		// Draw framebuffers
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDisable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(g_scene_framebuffer_shader.id);
			glBindVertexArray(g_scene_framebuffer_shader.vao);

			unsigned int inversion_loc = glGetUniformLocation(g_scene_framebuffer_shader.id, "use_inversion");
			unsigned int blur_loc = glGetUniformLocation(g_scene_framebuffer_shader.id, "use_blur");
			unsigned int blur_amount_loc = glGetUniformLocation(g_scene_framebuffer_shader.id, "blur_amount");
			unsigned int gamma_amount_loc = glGetUniformLocation(g_scene_framebuffer_shader.id, "gamma_amount");

			glUniform1i(inversion_loc, g_pp_settings.inverse_color);
			glUniform1i(blur_loc, g_pp_settings.blur_effect);
			glUniform1f(blur_amount_loc, g_pp_settings.blur_effect_amount);
			glUniform1f(gamma_amount_loc, g_pp_settings.gamma_amount);

			glBindTexture(GL_TEXTURE_2D, g_scene_framebuffer.texture_gpu_id);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glUniform1i(inversion_loc, false);
			glUniform1i(blur_loc, false);
			glBindTexture(GL_TEXTURE_2D, g_editor_framebuffer.texture_gpu_id);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glEnable(GL_DEPTH_TEST);
		}

		if (DEBUG_SHADOWMAP && g_selected_object.type == ObjectType::Spotlight) draw_selected_shadow_map();

		print_debug_texts();

		imgui_end_frame();

		glfwSwapBuffers(g_window);
		g_game_metrics.frames++;
		g_game_metrics.fps_frames++;
		g_frame_data.draw_calls = 0;
	}

	glfwTerminate();
	return 0;
}
