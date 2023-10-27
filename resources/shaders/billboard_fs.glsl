#version 330 core

in vec2 texCoord;

uniform sampler2D billboard_texture;

out vec4 fragColor;

void main()
{
    fragColor = texture(billboard_texture, texCoord);
}