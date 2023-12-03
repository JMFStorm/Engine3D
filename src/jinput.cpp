#include "jinput.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

GameInputs init_game_inputs()
{
	GameInputs inputs = {};
	inputs.mouse1 = ButtonState{ .key = GLFW_MOUSE_BUTTON_1 };
	inputs.mouse2 = ButtonState{ .key = GLFW_MOUSE_BUTTON_2 };
	inputs.q = ButtonState{ .key = GLFW_KEY_Q };
	inputs.w = ButtonState{ .key = GLFW_KEY_W };
	inputs.e = ButtonState{ .key = GLFW_KEY_E };
	inputs.r = ButtonState{ .key = GLFW_KEY_R };
	inputs.a = ButtonState{ .key = GLFW_KEY_A };
	inputs.s = ButtonState{ .key = GLFW_KEY_S };
	inputs.d = ButtonState{ .key = GLFW_KEY_D };
	inputs.f = ButtonState{ .key = GLFW_KEY_F };
	inputs.z = ButtonState{ .key = GLFW_KEY_Z };
	inputs.x = ButtonState{ .key = GLFW_KEY_X };
	inputs.c = ButtonState{ .key = GLFW_KEY_C };
	inputs.v = ButtonState{ .key = GLFW_KEY_V };
	inputs.y = ButtonState{ .key = GLFW_KEY_Y };
	inputs.esc = ButtonState{ .key = GLFW_KEY_ESCAPE };
	inputs.plus = ButtonState{ .key = GLFW_KEY_KP_ADD };
	inputs.minus = ButtonState{ .key = GLFW_KEY_KP_SUBTRACT };
	inputs.del = ButtonState{ .key = GLFW_KEY_DELETE };
	inputs.left_ctrl = ButtonState{ .key = GLFW_KEY_LEFT_CONTROL };
	inputs.space = ButtonState{ .key = GLFW_KEY_SPACE };
	return inputs;
}