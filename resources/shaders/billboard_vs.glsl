#version 330 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoords;

uniform mat4 model;

layout (std140) uniform ViewMatrices
{
    mat4 projection;
    mat4 view;
};

out vec2 texCoord;

void main()
{
    gl_Position = projection * view * model * vec4(inPosition, 1.0);
    texCoord = inTexCoords;
}
