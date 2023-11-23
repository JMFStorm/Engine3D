#version 330 core

in vec2 my_UV;
flat in int texture_index;

uniform sampler2DArray my_texture;

out vec4 fragOutput;

void main()
{
    fragOutput = texture(my_texture, vec3(my_UV, texture_index));
}
