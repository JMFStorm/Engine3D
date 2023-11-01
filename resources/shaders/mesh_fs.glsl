#version 330 core

struct Light {
    vec3 position;
    vec3 diffuse;
    float specular;
};

struct Material {
    sampler2D color_texture;
    sampler2D specular_texture;
    float specular_mult;
    float shininess;
};

in vec3 fragPos;    // Fragment position in world space
in vec3 fragNormal; // Normal in world space
in vec2 TexCoord;

uniform bool use_texture;
uniform bool use_specular_texture;
uniform vec3 viewPos; // Camera (view) position

uniform Light light;
uniform Material material;
uniform vec3 ambientLight;

out vec4 FragColor;

void main()
{
    // ambient
    vec3 ambient = ambientLight * texture(material.color_texture, TexCoord).rgb;

    // diffuse
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.color_texture, TexCoord).rgb;

    vec3 specular = vec3(0,0,0);

    if (use_specular_texture)
    {
        vec3 viewDir = normalize(viewPos - fragPos);
        vec3 reflectDir = reflect(-lightDir, norm);  
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        specular = light.specular * spec * texture(material.specular_texture, TexCoord).rgb;
        specular = specular * material.specular_mult;
    }

    // Result
    vec3 color_result = ambient + diffuse + specular;
    FragColor = vec4(color_result, 1.0);
}
