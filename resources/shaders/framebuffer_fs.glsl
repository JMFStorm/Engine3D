#version 330 core

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool use_inversion;
uniform bool use_effect;
uniform float effect_amount;

out vec4 FragColor;

const float k_offset = 1.0 / 600.0;  

void main()
{
    vec4 texture_color = texture(screenTexture, TexCoords);
    vec3 final_color = texture_color.rgb;

    if (use_inversion)
    {
        final_color = vec3(1.0) - final_color;
    }

    if (use_effect)
    {
        vec2 offsets[9] = vec2[](
            vec2(-k_offset,  k_offset), // top-left
            vec2( 0.0f,      k_offset), // top-center
            vec2( k_offset,  k_offset), // top-right
            vec2(-k_offset,  0.0f),     // center-left
            vec2( 0.0f,      0.0f),     // center-center
            vec2( k_offset,  0.0f),     // center-right
            vec2(-k_offset, -k_offset), // bottom-left
            vec2( 0.0f,     -k_offset), // bottom-center
            vec2( k_offset, -k_offset)  // bottom-right    
        );

        float kernel[9] = float[](
            -1, -1, -1,
            -1,  9, -1,
            -1, -1, -1
        );
        
        vec3 sampleTex[9];

        for(int i = 0; i < 9; i++) sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));

        vec3 col = vec3(0.0);

        for(int i = 0; i < 9; i++) col += sampleTex[i] * kernel[i];

        final_color += col;
    }

    FragColor = vec4(final_color, texture_color.a);
}
