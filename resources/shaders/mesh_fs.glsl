#version 330 core

struct Pointlight {
    vec3 position;
    vec3 diffuse;
    float specular;
    float intensity;
};

struct Spotlight {
    vec3 position;
    vec3 direction;
    vec3 diffuse;
    float cutoff;
    float outer_cutoff;
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
uniform Pointlight pointlights[20];
uniform Spotlight spotlight1;
uniform Material material;
uniform vec3 global_ambient_light;

out vec4 FragColor;

vec3 point_lights_color(Pointlight light, vec3 frag_normal, vec3 frag_pos, vec3 view_dir)
{
    vec3 specular = vec3(0, 0, 0);
    vec3 light_dir = normalize(light.position - frag_pos);
    float light_radius = light.intensity;

    float distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (1.0 + (distance / light_radius) * (distance / light_radius));

    float diff = max(dot(frag_normal, light_dir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.color_texture, TexCoord));

    if (use_specular_texture)
    {
        vec3 reflectDir = reflect(-light_dir, frag_normal);
        float spec = pow(max(dot(view_dir, reflectDir), 0.0), material.shininess);
        specular = light.specular * spec * vec3(texture(material.specular_texture, TexCoord)) * material.specular_mult;
        specular *= diffuse;
    }

    diffuse *= attenuation;
    specular *= attenuation;

    return vec3(diffuse + specular);
}

vec3 spotlight_color(Spotlight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (1.0 + light.linear * distance + light.quadratic * (distance * distance));

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutoff - light.outer_cutoff;
    float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);

    vec3 diffuse = light.diffuse * diff * vec3(texture(material.color_texture, TexCoord));
    diffuse *= attenuation * intensity;
    return diffuse;
}

void main()
{
    vec3 color_result = vec3(0);
    vec3 norm = normalize(fragNormal);
    vec3 view_dir = normalize(view_coords - fragPos);

    vec3 ambient = global_ambient_light * texture(material.color_texture, TexCoord).rgb;
    color_result += ambient;

    for (int i = 0; i < pointlights_count; i++)
    {
        color_result += point_lights_color(pointlights[i], norm, fragPos, view_dir);
    }

    Spotlight use_spotlight;
    use_spotlight.position = spotlight1.position;
    use_spotlight.direction = spotlight1.direction;
    use_spotlight.cutoff = spotlight1.cutoff;
    use_spotlight.outer_cutoff = spotlight1.outer_cutoff;
    use_spotlight.linear = 0.09;
    use_spotlight.quadratic = 0.032;
    use_spotlight.diffuse = vec3(0.5, 0.5, 0.5);

    color_result += spotlight_color(use_spotlight, norm, fragPos, view_dir);
    FragColor = vec4(color_result, 1.0);
}
