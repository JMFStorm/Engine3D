#include <AL/al.h>
#include <AL/alc.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "j_imgui.h"
#include "globals.h"
#include "editor.h"
#include "main.h"
#include "structs.h"
#include "scene.h"
#include "utils.h"
#include "jfont.h"
#include "j_array.h"
#include "j_assert.h"
#include "j_buffers.h"
#include "jfiles.h"
#include "jinput.h"
#include "j_map.h"
#include "j_render.h"
#include "j_strings.h"

void init_openal()
{
	ALCdevice* openal_device = alcOpenDevice(nullptr);
	assert(openal_device);

    ALCcontext* openal_context = alcCreateContext(openal_device, nullptr);
    assert(openal_context);

    alcMakeContextCurrent(openal_context);

    int channels, sample_rate, num_of_samples;

    s16* ogg_sound_data = load_ogg_file(
    	const_cast<char*>("G:\\projects\\game\\Engine3D\\resources\\sounds\\No.ogg"),
    	&channels, &sample_rate, &num_of_samples);

    ALuint buffer;
    alGenBuffers(1, &buffer);

	ALsizei frequency = sample_rate;
    ALsizei size = num_of_samples * channels * sizeof(s16);
    ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

    alBufferData(buffer, format, ogg_sound_data, size, frequency);
    free(ogg_sound_data);

    ALuint source;
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcePlay(source);
}

void enable_cursor(bool enable)
{
	int use = enable ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
	glfwSetInputMode(g_window, GLFW_CURSOR, use);
}

void update_mouse_loc()
{
	double xpos, ypos;
	glfwGetCursorPos(g_window, &xpos, &ypos);
	g_frame_data.mouse_x = (f32)xpos;
	g_frame_data.mouse_y = (f32)ypos;
}

void init_window_and_context()
{
	int glfw_init_result = glfwInit();
	assert(glfw_init_result == GLFW_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	g_window = glfwCreateWindow(640, 400, "JMF Engine3D", NULL, NULL);
	assert(g_window);
	glfwMakeContextCurrent(g_window);
	glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);
	glfwSetCursorPosCallback(g_window, mouse_move_callback);
	int glad_init_success = gladLoadGL();
	assert(glad_init_success);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	resize_windows_area_settings(width, height);
	glViewport(0, 0, g_game_metrics.scene_width_px, g_game_metrics.scene_height_px);

	glGenFramebuffers(1, &g_scene_framebuffer.id);
	glBindFramebuffer(GL_FRAMEBUFFER, g_scene_framebuffer.id);
	init_framebuffer_resize(&g_scene_framebuffer.texture_gpu_id, &g_scene_framebuffer.renderbuffer);

	glGenFramebuffers(1, &editor_framebuffer.id);
	glBindFramebuffer(GL_FRAMEBUFFER, editor_framebuffer.id);
	init_framebuffer_resize(&editor_framebuffer.texture_gpu_id, &editor_framebuffer.renderbuffer);

	if (get_allocated_temp_memory() <= 0) allocate_temp_memory(MEGABYTES(1));
	int font_height_px = normalize_value(debug_font_vh, 100.0f, (float)height);
	load_font(&g_debug_font, font_height_px, g_debug_font_path);
	deallocate_temp_memory();
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

	init_openal();
	init_window_and_context();

	init_imgui();
	init_all_shaders();

	g_pp_settings = post_processings_init();
	g_inputs.as_struct = init_game_inputs();

	Material materials[MATERIALS_MAX_COUNT] = {};
	s64 materials_count = get_materials_from_manifest(materials, MATERIALS_MAX_COUNT);
	load_material_textures(materials, materials_count);
	load_materials_into_memory(materials, materials_count);

	load_core_textures();
	init_framebuffers();

	glfwSetWindowSize(g_window, g_user_settings.window_size_px[0], g_user_settings.window_size_px[1]);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);

	deallocate_temp_memory();
	new_scene();

	while (!glfwWindowShouldClose(g_window))
	{
		// -------------
		// Inputs

		glfwPollEvents();
		imgui_new_frame();
		right_hand_editor_panel();

		update_mouse_loc();
		register_frame_inputs();
		update_frame_data();

		// -------------
		// Logics

		if (g_inputs.as_struct.left_ctrl.is_down && g_inputs.as_struct.s.pressed) save_all();

		if (!g_camera_move_mode) g_transform_mode.mode = get_curr_transformation_mode();

		if (g_inputs.as_struct.mouse1.pressed)
		{
			g_frame_data.mouse_clicked = true;
			bool scene_click = mouse_in_scene_space(g_frame_data.mouse_x, g_frame_data.mouse_y);
			if (scene_click) try_get_mouse_selection(g_frame_data.mouse_x, g_frame_data.mouse_y);
		}

		bool camera_mode_start = g_inputs.as_struct.mouse2.pressed
			&& mouse_in_scene_space((int)g_frame_data.mouse_x, (int)g_frame_data.mouse_y);

		if (camera_mode_start) g_camera_move_mode = true;
		else if (!g_inputs.as_struct.mouse2.is_down) g_camera_move_mode = false;

		handle_camera_move_mode();

		if (has_object_selection())
		{
			if (g_inputs.as_struct.del.pressed) delete_selected_object();
			else if (g_inputs.as_struct.left_ctrl.is_down && g_inputs.as_struct.d.pressed) duplicate_selected_object();
		}

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
		else if (g_transform_mode.is_active) handle_tranformation_mode();

		// -------------
		// Draw OpenGL

		update_ubos();

		draw_shadow_map_framebuffers();
		draw_scene_framebuffer();
		draw_editor_framebuffer();
		draw_main_framebuffer();

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
