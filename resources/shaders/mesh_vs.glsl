#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform float uv_multiplier;

out VS_OUT {
    vec3 fragPos;
    vec3 fragNormal;
    vec2 TexCoord;
    vec4 FragPosLightSpace;
} vs_out;

void main()
{
    float scale_u = length(model[0].xyz); // Scale along the X-axis
    float scale_v = length(model[2].xyz); // Scale along the Z-axis

	float used_uv_mult_u = scale_u / uv_multiplier;
	float used_uv_mult_v = scale_v / uv_multiplier;

	gl_Position = projection * view * model * vec4(aPos, 1.0);
	vs_out.TexCoord = vec2(aTexCoord.x * used_uv_mult_u, aTexCoord.y * used_uv_mult_v);

	// Calculate the position and normal in world space
    vs_out.fragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.fragNormal = mat3(transpose(inverse(model))) * aNormal;

    vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.fragPos, 1.0);
}
