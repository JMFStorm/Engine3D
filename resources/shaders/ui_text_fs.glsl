#version 330 core

in vec2 texCoord;

out vec4 fragOutput;

uniform sampler2D ourTexture;

void main()
{
    fragOutput = texture(ourTexture, texCoord);
}
