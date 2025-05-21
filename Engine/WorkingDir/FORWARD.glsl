#ifdef FORWARD

#if defined(VERTEX)

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vPosition;
out vec3 vNormal;
out vec2 vTexCoord;
out mat3 TBN;

void main()
{
    vec4 worldPosition = uModel * vec4(position, 1.0);
    vPosition = worldPosition.xyz;
    vTexCoord = texCoords;
    
    // Normal matrix calculation
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    vNormal = normalMatrix * normal;

    // TBN matrix calculation
    vec3 T = normalize(normalMatrix * tangent);
    vec3 B = normalize(normalMatrix * bitangent);
    vec3 N = normalize(normalMatrix * normal);
    TBN = mat3(T, B, N);

    gl_Position = uProj * uView * worldPosition;
}

#elif defined(FRAGMENT)

struct Light {
    int type;
    vec3 color;
    vec3 direction;
    vec3 position;
    float intensity;
};

layout(binding = 0) uniform GlobalParams {
    vec3 uCameraPosition;
    int uLightCount;
    Light uLight[128];
};

uniform sampler2D uAlbedoTexture;
uniform sampler2D uNormalTexture;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;
in mat3 TBN;

out vec4 oColor;

vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * diff;
    
    // Specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = light.color * spec * 0.5;
    
    // Ambient
    vec3 ambient = light.color * 0.1;

    return (ambient + diffuse + specular) * attenuation;
}

vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * diff;
    
    // Specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = light.color * spec * 0.5;

    return diffuse + specular;
}

vec3 CalculateLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo) {
    // Dirección luz y distancia
    vec3 lightDir = light.type == 0 ? 
        normalize(-light.direction) : 
        normalize(light.position - fragPos);
    
    // Atenuación
    float attenuation = 1.0;
    if (light.type != 0) {
        float distance = length(light.position - fragPos);
        attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));
    }
    
    // Difuso
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * diff * albedo;
    
    // Especular (Blinn-Phong simplificado)
    vec3 halfwayDir = normalize(lightDir + viewDir); 
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = light.color * spec * 0.5;
    
    return (diffuse + specular) * attenuation * light.intensity;
}

void main()
{
    // Base color from texture
    vec3 albedo = texture(uAlbedoTexture, vTexCoord).rgb;
    
    // Normal mapping
    vec3 normal = normalize(vNormal);
    if(textureSize(uNormalTexture, 0).x > 1) {
        vec3 tangentNormal = texture(uNormalTexture, vTexCoord).rgb * 2.0 - 1.0;
        normal = normalize(TBN * tangentNormal);
    }

    vec3 viewDir = normalize(uCameraPosition - vPosition);
    vec3 result = vec3(0.1) * albedo; // Ambient
    
    for(int i = 0; i < uLightCount; ++i) {
        result += CalculateLight(uLight[i], normal, vPosition, viewDir, albedo);
    }
    
    // Gamma correction
    result = pow(result, vec3(1.0/2.2));
    oColor = vec4(result, 1.0);
}

#endif
#endif