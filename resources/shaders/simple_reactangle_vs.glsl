#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aOffset;
layout(location = 2) in vec3 aColor;

out vec3 my_color;

void main()
{
   vec2 pos = vec2(aPosition.x + aOffset.x, aPosition.y + aOffset.y);
   gl_Position = vec4(pos, 0, 1.0);
   my_color = aColor;
}
