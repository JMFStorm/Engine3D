#version 330 core

in vec3 my_color;
in vec2 my_UV;

uniform sampler2D my_texture;

out vec4 fragOutput;

void main()
{
    vec3 texture_color = vec3(texture(my_texture, my_UV)) * my_color;
    fragOutput = vec4(texture_color, 1.0);
}
