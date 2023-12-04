#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

void enable_cursor(bool enable);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void mouse_move_callback(GLFWwindow* window, double xposIn, double yposIn);

void set_button_state(GLFWwindow* window, ButtonState* button);
