#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;

uniform mat4 model;

layout (std140) uniform ViewMatrices
{
    mat4 projection;
    mat4 view;
};

out vec3 line_color;

void main()
{
    line_color = aColor;
    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}