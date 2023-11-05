#version 330 core

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool use_inversion;
uniform bool use_blur;
uniform float blur_amount;

out vec4 FragColor;

void main()
{
    vec4 texture_color = texture(screenTexture, TexCoords);
    vec3 final_color = texture_color.rgb;

    if (use_blur)
    {
        float offset = blur_amount / 1000.0;

        vec2 offsets[9] = vec2[](
            // Left                 // Center               // Right
            vec2(-offset,  offset), vec2(0.0f,     offset), vec2( offset,  offset), // top
            vec2(-offset,  0.0f),   vec2(0.0f,     0.0f),   vec2( offset,  0.0f),   // center
            vec2(-offset, -offset), vec2(0.0f,    -offset), vec2( offset, -offset)  // bot
        );

        float kernel[9] = float[](
            1.0 / 16, 2.0 / 16, 1.0 / 16,
            2.0 / 16, 4.0 / 16, 2.0 / 16,
            1.0 / 16, 2.0 / 16, 1.0 / 16  
        );
    
        vec3 sampleTex[9];
        vec3 col = vec3(0.0);

        for(int i = 0; i < 9; i++) sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
        for(int i = 0; i < 9; i++) col += sampleTex[i] * kernel[i];

        final_color = col;
    }

    if (use_inversion)
    {
        final_color = vec3(1.0) - final_color;
    }

    FragColor = vec4(final_color, texture_color.a);
}
