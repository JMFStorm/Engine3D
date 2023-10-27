#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 normal_vec3;
uniform float uv_multiplier;

out vec2 TexCoord;
out vec3 fragPos;	  // Fragment position in world space
out vec3 fragNormal;  // Normal in world space

void main()
{
    float scale_u = length(model[0].xyz); // Scale along the X-axis
    float scale_v = length(model[2].xyz); // Scale along the Z-axis

	float used_uv_mult_u = scale_u / uv_multiplier;
	float used_uv_mult_v = scale_v / uv_multiplier;

	gl_Position = projection * view * model * vec4(aPos, 1.0);
	TexCoord = vec2(aTexCoord.x * used_uv_mult_u, aTexCoord.y * used_uv_mult_v);

	// Calculate the position and normal in world space
    fragPos = vec3(model * vec4(aPos, 1.0));
    fragNormal = mat3(transpose(inverse(model))) * normal_vec3;
}
