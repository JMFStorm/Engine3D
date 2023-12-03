#version 330 core

in vec2 my_UV;

uniform sampler2D my_texture;

out vec4 fragOutput;

void main()
{
    fragOutput = texture(my_texture, my_UV);
}
