#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool use_inversion;

void main()
{
    if (use_inversion)
    {
        vec4 texture_color = texture(screenTexture, TexCoords);
        vec3 inversed_color = vec3(1.0) - texture_color.rgb;
        FragColor = vec4(inversed_color, texture_color.a);
    }
    else
    {
        FragColor = texture(screenTexture, TexCoords);
    }
}
