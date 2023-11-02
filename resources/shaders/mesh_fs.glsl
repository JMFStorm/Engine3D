#version 330 core

struct Light {
    vec3 position;
    vec3 diffuse;
    float specular;
    float linear;
    float quadratic;
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
uniform vec3 view_coords; // Camera (view) position

uniform int pointlights_count;
uniform Light pointlights[20];
uniform Material material;
uniform vec3 global_ambient_light;

out vec4 FragColor;

vec3 point_lights_color(Light light, vec3 frag_normal, vec3 frag_pos, vec3 view_dir)
{
    vec3 lightDir = normalize(light.position - frag_pos);

    float distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (1.0 + light.linear * distance + light.quadratic * (distance * distance));

    float diff = max(dot(frag_normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.color_texture, TexCoord));

    vec3 specular = vec3(0, 0, 0);

    if (use_specular_texture)
    {
        vec3 reflectDir = reflect(-lightDir, frag_normal);
        float spec = pow(max(dot(view_dir, reflectDir), 0.0), material.shininess);
        specular = light.specular * spec * vec3(texture(material.specular_texture, TexCoord)) * material.specular_mult;
    }

    diffuse *= attenuation;
    specular *= attenuation;

    return vec3(diffuse + specular);
}

void main()
{
    vec3 ambient = global_ambient_light * texture(material.color_texture, TexCoord).rgb;

    vec3 norm = normalize(fragNormal);
    vec3 view_dir = normalize(view_coords - fragPos);

    vec3 color_result = vec3(0);

    for (int i = 0; i < pointlights_count; i++)
    {
        color_result += point_lights_color(pointlights[i], norm, fragPos, view_dir);
    }

    color_result += ambient;

    FragColor = vec4(color_result, 1.0);
}
