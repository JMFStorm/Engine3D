#version 330 core

in vec2 texCoord;

out vec4 fragOutput;

uniform sampler2D ourTexture;
uniform vec3 textColor;

void main()
{
    fragOutput = vec4(textColor.r, textColor.g, textColor.b, texture(ourTexture, texCoord).a);
}
