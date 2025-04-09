#ifdef Render_Quad

#if defined(VERTEX) ////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ////////////////////////////////////////

struct Light
{
    int type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    int uLightCount;
    Light uLight[16];
};

in vec2 vTexCoord;

uniform sampler2D uColor;
uniform sampler2D uNormals;
uniform sampler2D uPosition;
uniform sampler2D uViewDir;

layout(location=0) out vec4 oColor;
vec3 CalcPointLight(Light light, vec3 normal, vec3 position, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - position);
    float distance = length(light.position - position);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));

    vec3 ambient = light.color * 0.1;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * diff;

    // REDUCE el specular temporalmente para ver si los "múltiples reflejos" desaparecen
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = light.color * spec * 0.05; // antes era * 0.5

    return (ambient + diffuse + specular) * attenuation;
}
vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir)
{
    // Light direction
    vec3 lightDir = normalize(-light.direction);
    
    // Diffuse component
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * diff;
    
    // Specular component
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normalize(viewDir), reflectDir), 0.0), 32.0);
    vec3 specular = light.color * spec * 0.5;
    
    return diffuse + specular; // No attenuation for directional lights
}

void main()
{
    // Texture sampling
    vec3 baseColor = texture(uColor, vTexCoord).rgb;
    vec3 normal = normalize(texture(uNormals, vTexCoord).rgb * 2.0 - 1.0);
    vec3 position = texture(uPosition, vTexCoord).rgb;
    vec3 viewDir = normalize(uCameraPosition - position);

    vec3 finalColor = vec3(0.0);
    
    for(int i = 0; i < uLightCount; ++i)
    {
        if(uLight[i].type == 0) // Directional light
        {
            finalColor += CalcDirLight(uLight[i], normal, viewDir) * baseColor;
        }
        else if(uLight[i].type == 1) // Point light
        {
            finalColor += CalcPointLight(uLight[i], normal, position, viewDir) * baseColor;
        }
    }
    
    oColor = vec4(finalColor, 1.0);
}

#endif
#endif