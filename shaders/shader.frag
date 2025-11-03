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
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NR_POINT_LIGHTS 1
uniform PointLight pointLights[NR_POINT_LIGHTS];

// Shadow cubemap for the (single) point light
uniform samplerCube shadowMap;
uniform float far_plane;
uniform float shadowRadius; // world-space sampling radius for PCF
uniform bool enableShadows;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float CalcPointShadow(PointLight light, vec3 normal, vec3 fragPos);

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
    
    vec3 ambient = light.ambient * texture(texture_sampler, TexCoord).rgb;
    vec3 diffuse = light.diffuse * diff * texture(texture_sampler, TexCoord).rgb;
    vec3 specular = light.specular * spec * vec3(1.0);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    // Shadow calculation for point light (cubemap)
    float shadow = CalcPointShadow(light, normal, fragPos);
    // Reduce diffuse and specular when in shadow
    diffuse *= (1.0 - shadow);
    specular *= (1.0 - shadow);
    return (ambient + diffuse + specular);
}

float CalcPointShadow(PointLight light, vec3 normal, vec3 fragPos)
{
    vec3 fragToLight = fragPos - light.position;
    float currentDepth = length(fragToLight);
    // Bias to avoid shadow acne (normal/slope based)
    // slope-scale + constant bias (better across grazing angles)
    float normalDot = dot(normal, normalize(light.position - fragPos));
    float bias = max(0.02 * (1.0 - normalDot), 0.001) + 0.0005;

    // Poisson-disk-like PCF with per-fragment rotation to reduce banding/ghosting
    const int SAMPLE_COUNT = 12;
    const vec3 poissonDisk[SAMPLE_COUNT] = vec3[](
        vec3( 0.5381,  0.1856,  0.1234), vec3( 0.1379,  0.2486, -0.1444), vec3( 0.3371, -0.5679,  0.3456),
        vec3(-0.7255,  0.2451,  0.5678), vec3(-0.1839, -0.3887, -0.2444), vec3( 0.1234,  0.4567, -0.3333),
        vec3( 0.6543, -0.2311,  0.1111), vec3(-0.4444,  0.3333, -0.2222), vec3( 0.2222, -0.1111,  0.4444),
        vec3(-0.1111,  0.7777, -0.3333), vec3( 0.8888, -0.4444,  0.2222), vec3(-0.6666, -0.2222,  0.5555)
    );

    if (!enableShadows) return 0.0;
    float occluded = 0.0;
    float radius = shadowRadius * (currentDepth / far_plane);

    // rotation angle per-fragment (based on FragPos hash)
    float rnd = fract(sin(dot(fragPos.xyz , vec3(12.9898,78.233,45.164))) * 43758.5453);
    float angle = rnd * 6.28318530718; // 2*pi
    vec3 axis = normalize(fragToLight);

    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        // rotate sample vector around the fragToLight axis using Rodrigues' rotation formula
        vec3 v = poissonDisk[i];
        vec3 v_rot = v * cos(angle) + cross(axis, v) * sin(angle) + axis * dot(axis, v) * (1.0 - cos(angle));
        vec3 dir = normalize(fragToLight + v_rot * radius);
        float sampleDepth = texture(shadowMap, dir).r * far_plane;
        if (currentDepth - bias > sampleDepth) occluded += 1.0;
    }

    float shadow = occluded / float(SAMPLE_COUNT);
    return shadow;
}