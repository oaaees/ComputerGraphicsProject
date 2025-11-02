#version 410

in vec2 TexCoord;
in vec3 FragPos;
in mat3 TBN;

out vec4 FragColor;

uniform sampler2D texture_sampler;
uniform sampler2D normal_sampler;
uniform vec3 viewPosition;

// A simple material structure
struct Material {
    float shininess;
};
uniform Material material;

// A simple directional light structure
struct DirLight {
    vec3 direction;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;

// A simple point light structure
struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
	
    vec3 diffuse;
    vec3 specular;
};

#define NR_POINT_LIGHTS 2
uniform PointLight pointLights[NR_POINT_LIGHTS];

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    // Obtain normal from normal map. It's in tangent space, so transform to world space.
    // The range [0,1] is mapped to [-1,1].
    vec3 norm = texture(normal_sampler, TexCoord).rgb;
    norm = normalize(norm * 2.0 - 1.0);
    norm = normalize(TBN * norm);

    vec3 viewDir = normalize(viewPosition - FragPos);

    // Phase 1: Directional lighting
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    // Phase 2: Point lights
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);    

    // Global ambient light
    vec3 ambient = vec3(0.1) * texture(texture_sampler, TexCoord).rgb;
    result += ambient;

    FragColor = vec4(result, 1.0);
}

// calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results
    vec3 diffuse  = light.diffuse  * diff * texture(texture_sampler, TexCoord).rgb;
    vec3 specular = light.specular * spec * vec3(1.0); // White highlight
    return (diffuse + specular);
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 diffuse = light.diffuse * diff * texture(texture_sampler, TexCoord).rgb;
    vec3 specular = light.specular * spec * vec3(1.0);
    diffuse *= attenuation;
    specular *= attenuation;
    return (diffuse + specular);
}