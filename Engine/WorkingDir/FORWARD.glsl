#ifdef FORWARD

#if defined(VERTEX)

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vPosition;
out vec3 vNormal;
out vec2 vTexCoord;

void main()
{
    vec4 worldPosition = uModel * vec4(position, 1.0);
    vPosition = worldPosition.xyz;
    vTexCoord = texCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    vNormal = normalize(normalMatrix * normal);

    gl_Position = uProj * uView * worldPosition;
}

#elif defined(FRAGMENT)

struct Light {
    unsigned int type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(binding = 0, std140) uniform GlobalParams {
    vec3 uCameraPosition;
    unsigned int uLightCount;
    Light uLight[16];
};

uniform sampler2D uAlbedoTexture;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 oColor;

vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 2.0);
    
    vec3 ambient = light.color * 0.2;
    vec3 diffuse = light.color * diff;
    vec3 specular = light.color * spec * 0.1;
    
    return ambient + diffuse + specular;
}

vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 2.0);
    
    vec3 ambient = light.color * 0.2;
    vec3 diffuse = light.color * diff;
    vec3 specular = light.color * spec * 0.1;
    
    return (ambient + diffuse + specular) * attenuation;
}

void main()
{
    vec3 albedo = texture(uAlbedoTexture, vTexCoord).rgb;
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPosition - vPosition);
    
    vec3 result = vec3(0.0);
    
    for(int i = 0; i < uLightCount; ++i)
    {
        if(uLight[i].type == 0)
            result += CalcDirLight(uLight[i], normal, viewDir) * albedo;
        else
            result += CalcPointLight(uLight[i], normal, vPosition, viewDir) * albedo;
    }
    
    oColor = vec4(result, 1.0);
}

#endif
#endif