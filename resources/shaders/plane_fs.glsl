#version 330 core

struct Light {
    vec3 position;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

in vec3 fragPos;    // Fragment position in world space
in vec3 fragNormal; // Normal in world space
in vec2 TexCoord;

uniform sampler2D texture1;
uniform vec3 viewPos; // Camera (view) position

uniform Light light;
uniform Material material;
uniform vec3 ambientLight;

out vec4 FragColor;

void main()
{
    // Calculate lighting
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 viewDir = normalize(viewPos - fragPos);

    // Diffuse component
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);

    // Specular component
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);

    // Final lighting calculation
    vec3 light_result = ambientLight + diffuse + specular;

	FragColor = texture(texture1, TexCoord) * vec4(light_result, 1.0);
}
