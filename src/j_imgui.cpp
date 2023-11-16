#include "j_imgui.h"

#include <GL/glew.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include "globals.h"
#include "utils.h"
#include "editor.h"

void imgui_new_frame()
{
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();
}

void init_imgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui_ImplGlfw_InitForOpenGL(g_window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");
}

void right_hand_editor_panel()
{
	ImGui::SetNextWindowPos(ImVec2(static_cast<float>(g_game_metrics.game_width_px - PROPERTIES_PANEL_WIDTH), 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(PROPERTIES_PANEL_WIDTH, g_game_metrics.game_height_px), ImGuiCond_Always);

	ImGui::Begin("Properties", nullptr, 0);

	// Add new objects
	{
		if (ImGui::Button("Add plane"))
		{
			Mesh new_plane = {
				.transforms = transforms_init(),
				.material = (Material*)j_array_get(&g_materials, 0),
				.mesh_type = MeshType::Plane,
				.uv_multiplier = 1.0f,
			};
			s64 new_mesh_index = add_new_mesh(new_plane);
			select_object_index(ObjectType::Plane, new_mesh_index);
		}
		else if (ImGui::Button("Add Cube"))
		{
			Mesh new_cube = {
				.transforms = transforms_init(),
				.material = (Material*)j_array_get(&g_materials, 0),
				.mesh_type = MeshType::Cube,
				.uv_multiplier = 1.0f,
			};
			s64 new_mesh_index = add_new_mesh(new_cube);
			select_object_index(ObjectType::Cube, new_mesh_index);
		}
		else if (ImGui::Button("Add pointlight"))
		{
			Pointlight new_pointlight = pointlight_init();
			s64 new_light_index = add_new_pointlight(new_pointlight);
			select_object_index(ObjectType::Pointlight, new_light_index);
		}
		else if (ImGui::Button("Add spotlight"))
		{
			Spotlight new_spotlight = spotlight_init();
			s64 new_light_index = add_new_spotlight(new_spotlight);
			select_object_index(ObjectType::Spotlight, new_light_index);
		}
	}

	// Selection properties
	{
		if (is_primitive(g_selected_object.type))
		{
			char selected_mesh_str[24];
			sprintf_s(selected_mesh_str, "Mesh index: %lld", g_selected_object.selection_index);
			ImGui::Text(selected_mesh_str);

			Mesh* selected_mesh_ptr = (Mesh*)get_selected_object_ptr();

			ImGui::Text("Mesh properties");
			ImGui::InputFloat3("Translation", &selected_mesh_ptr->transforms.translation[0], "%.2f");
			ImGui::InputFloat3("Rotation", &selected_mesh_ptr->transforms.rotation[0], "%.2f");
			ImGui::InputFloat3("Scale", &selected_mesh_ptr->transforms.scale[0], "%.2f");

			ImGui::Text("Select material");
			ImGui::Image((ImTextureID)selected_mesh_ptr->material->color_texture->gpu_id, ImVec2(128, 128));

			char* texture_names_ptr = (char*)g_material_names.data;

			if (ImGui::Combo("Material", &g_selected_texture_item, texture_names_ptr, g_material_names.strings_count))
			{
				selected_mesh_ptr->material = (Material*)j_array_get(&g_materials, g_selected_texture_item);
			}

			ImGui::InputFloat("UV mult", &selected_mesh_ptr->uv_multiplier, 0, 0, "%.2f");

			ImGui::Text("Material properties");
			ImGui::InputFloat("Specular mult", &selected_mesh_ptr->material->specular_mult, 0, 0, "%.3f");
			ImGui::InputFloat("Material shine", &selected_mesh_ptr->material->shininess, 0, 0, "%.3f");
		}
		else if (g_selected_object.type == ObjectType::Pointlight)
		{
			char selected_light_str[24];
			sprintf_s(selected_light_str, "Light index: %lld", g_selected_object.selection_index);
			ImGui::Text(selected_light_str);

			Pointlight* selected_light_ptr = (Pointlight*)get_selected_object_ptr();

			ImGui::Text("Pointight properties");
			ImGui::Checkbox("On/off", &selected_light_ptr->is_on);
			ImGui::InputFloat3("Position", &selected_light_ptr->transforms.translation[0], "%.3f");
			ImGui::ColorEdit3("Color", &selected_light_ptr->diffuse[0], 0);
			ImGui::InputFloat("Range", &selected_light_ptr->range, 0, 0, "%.2f");
			ImGui::InputFloat("Specular", &selected_light_ptr->specular, 0, 0, "%.2f");
			ImGui::InputFloat("Intensity", &selected_light_ptr->intensity, 0, 0, "%.2f");
		}
		else if (g_selected_object.type == ObjectType::Spotlight)
		{
			Spotlight* selected_spotlight_ptr = (Spotlight*)get_selected_object_ptr();

			ImGui::Text("Spotlight properties");
			ImGui::Checkbox("On/off", &selected_spotlight_ptr->is_on);
			ImGui::ColorEdit3("Color", &selected_spotlight_ptr->diffuse[0], 0);
			ImGui::InputFloat3("Position", &selected_spotlight_ptr->transforms.translation[0], "%.2f");
			ImGui::InputFloat3("Rotation", &selected_spotlight_ptr->transforms.rotation[0], "%.2f");
			ImGui::InputFloat("Specular", &selected_spotlight_ptr->specular, 0, 0, "%.2f");
			ImGui::InputFloat("Range", &selected_spotlight_ptr->range, 0, 0, "%.2f");
			ImGui::InputFloat("FOV", &selected_spotlight_ptr->fov, 0, 0, "%.1f");
			ImGui::InputFloat("Outer cutoff", &selected_spotlight_ptr->outer_cutoff_fov, 0, 0, "%.1f");
			ImGui::Checkbox("DEBUG_SHADOWMAP", &DEBUG_SHADOWMAP);
		}
	}

	ImGui::Text("Editor settings");
	ImGui::InputFloat("Transform clip", &g_user_settings.transform_clip, 0, 0, "%.2f");
	ImGui::InputFloat("Rotation clip", &g_user_settings.transform_rotation_clip, 0, 0, "%.2f");

	ImGui::Text("Scene settings");
	ImGui::ColorEdit3("Global ambient", &g_user_settings.world_ambient[0], 0);

	ImGui::Text("Game window");
	ImGui::InputInt2("Screen width px", &g_user_settings.window_size_px[0]);

	if (ImGui::Button("Apply changes")) glfwSetWindowSize(g_window, g_user_settings.window_size_px[0], g_user_settings.window_size_px[1]);

	ImGui::Text("Post processing");
	ImGui::Checkbox("Inverse", &g_pp_settings.inverse_color);
	ImGui::Checkbox("Blur", &g_pp_settings.blur_effect);
	ImGui::InputFloat("Blur amount", &g_pp_settings.blur_effect_amount, 0, 0, "%.1f");
	ImGui::InputFloat("Gamma", &g_pp_settings.gamma_amount, 0, 0, "%.1f");

	ImGui::End();
}

void imgui_end_frame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
