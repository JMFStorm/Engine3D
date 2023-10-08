#version 330 core

in vec2 texCoord;

out vec4 fragOutput;

uniform sampler2D ourTexture;

void main()
{
    fragOutput = vec4(0.7, 0.5, 0.5, texture(ourTexture, texCoord).a);
}
