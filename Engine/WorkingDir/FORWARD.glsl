#ifdef FORWARD

#if defined(VERTEX)

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec3 tangent;      // NEW
layout(location = 4) in vec3 bitangent;    // NEW

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vPosition;
out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vTangent;    // NEW
out vec3 vBitangent;  // NEW

void main()
{
    vec4 worldPosition = uModel * vec4(position, 1.0);
    vPosition = worldPosition.xyz;
    vTexCoord = texCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    vNormal = normalize(normalMatrix * normal);
    vTangent = normalize(normalMatrix * tangent);       // NEW
    vBitangent = normalize(normalMatrix * bitangent);   // NEW

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

// Material textures
uniform sampler2D uAlbedoTexture;
uniform sampler2D uNormalMap;
uniform sampler2D uHeightMap;
uniform float uHeightScale;

// Environment mapping
uniform samplerCube uSkybox;
uniform float uReflectionIntensity;
uniform float uFresnelPower = 5.0;

// NEW: Feature enable flags
uniform int uNormalMapAvailable;  // 0 = off, 1 = on
uniform int uEnvironmentEnabled;  // 0 = off, 1 = on

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vTangent;   
in vec3 vBitangent;  

out vec4 oColor;

// Relief mapping function
vec2 ParallaxOcclusionMapping(vec2 texCoords, vec3 viewDirTS) {
    const float minLayers = 32.0; 
    const float maxLayers = 64.0;
    
    float ndotv = clamp(dot(vec3(0.0, 0.0, 1.0), normalize(viewDirTS)), 0.0, 1.0);
    float numLayers = mix(maxLayers, minLayers, ndotv);
    
    float layerDepth = 1.0 / numLayers;
    vec2 deltaTexCoords = viewDirTS.xy * uHeightScale / (viewDirTS.z * numLayers);
    
    vec2 currentTexCoords = texCoords;
    float currentLayerDepth = 0.0;
    float currentDepthMapValue = texture(uHeightMap, currentTexCoords).r;
    
    for (int i = 0; i < int(numLayers); ++i) {
        if (currentLayerDepth >= currentDepthMapValue)
            break;
        
        currentTexCoords -= deltaTexCoords;
        currentLayerDepth += layerDepth;
        currentDepthMapValue = texture(uHeightMap, currentTexCoords).r;
    }
    
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float beforeDepth = texture(uHeightMap, prevTexCoords).r - (currentLayerDepth - layerDepth);
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    
    float weight = afterDepth / (afterDepth - beforeDepth);
    return mix(currentTexCoords, prevTexCoords, weight);
}

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
    // 1. Reconstruct TBN matrix
    vec3 T = normalize(vTangent);
    vec3 B = normalize(vBitangent);
    vec3 N = normalize(vNormal);
    mat3 TBN = mat3(T, B, N);
    
    // 2. Calculate view direction
    vec3 viewDirWS = normalize(uCameraPosition - vPosition);
    vec3 viewDirTS = transpose(TBN) * viewDirWS;
    
    // 3. Apply relief mapping if enabled
    vec2 displacedTexCoords = vTexCoord;
    if (uHeightScale > 0.0) {
        displacedTexCoords = ParallaxOcclusionMapping(vTexCoord, viewDirTS);
    }
    
    // 4. Sample textures with new coordinates
    vec3 albedo = texture(uAlbedoTexture, displacedTexCoords).rgb;
    vec3 normalTS = vec3(0.0, 0.0, 1.0); // Default flat normal
    
    // Only sample normal map if available
    if (uNormalMapAvailable > 0) {
        normalTS = texture(uNormalMap, displacedTexCoords).rgb * 2.0 - 1.0;
    }
    vec3 normalWS = normalize(TBN * normalTS);
    
        // 5. Lighting calculations
    vec3 lighting = vec3(0.0);
    for(int i = 0; i < uLightCount; ++i) {
        if(uLight[i].type == 0) {
            lighting += CalcDirLight(uLight[i], normalWS, viewDirWS) * albedo;
        } else {
            lighting += CalcPointLight(uLight[i], normalWS, vPosition, viewDirWS) * albedo;
        }
    }
    
    // 6. Environment mapping (solo si está habilitado)
    vec3 finalColor = lighting;
    if (uEnvironmentEnabled > 0) {
        vec3 viewDir = normalize(uCameraPosition - vPosition);
        vec3 reflectDir = reflect(-viewDir, normalWS);
        vec3 reflection = texture(uSkybox, reflectDir).rgb;
        
        float fresnel = pow(max(0.0, 1.0 - dot(normalWS, viewDir)), uFresnelPower);
        
        // CORRECCIÓN: Mezcla adecuada entre iluminación y reflexión
        finalColor = lighting * (1.0 - fresnel * uReflectionIntensity) + 
                    reflection * fresnel * uReflectionIntensity;
    }
    
    oColor = vec4(finalColor, 1.0);
}

#endif
#endif