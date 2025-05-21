#ifdef RELIEF_MAPPING

#if defined(VERTEX)

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uViewPos;

out Data {
    vec2 texCoords;
    vec3 tangentFragPos;
    vec3 tangentViewPos;
    mat3 TBN;
    vec3 worldFragPos; // Añadido
} VSOut;

void main() {
    vec3 fragPos = vec3(uModel * vec4(position, 1.0));
    VSOut.texCoords = texCoords;

    vec3 T = normalize(mat3(uModel) * tangent);
    vec3 B = normalize(mat3(uModel) * bitangent);
    vec3 N = normalize(mat3(uModel) * normal);
    VSOut.TBN = mat3(T, B, N);
    VSOut.TBN = transpose(VSOut.TBN);
   

    VSOut.tangentFragPos = VSOut.TBN * fragPos;
    VSOut.tangentViewPos = VSOut.TBN * uViewPos;
    VSOut.worldFragPos = fragPos; // Añadido

    gl_Position = uProj * uView * vec4(fragPos, 1.0);
}

#elif defined(FRAGMENT)

uniform mat4 uView;    // ← Declaración faltante
uniform mat4 uProj; 
struct Light {
    int type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(binding = 0) uniform GlobalParams {
    vec3 uCameraPosition;
    int uLightCount;
    Light uLight[128];
};

vec3 CalcPointLight(Light light, vec3 normal, vec3 position, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - position);
    float distance = length(light.position - position);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

    vec3 ambient = light.color * 0.1;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * diff;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 1.5), 5.0);
    vec3 specular = light.color * spec * 0.5;

    return (ambient + diffuse + specular) * attenuation;
}

vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * diff;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = light.color * spec * 0.5;
    return diffuse + specular;
}

in Data {
    vec2 texCoords;
    vec3 tangentFragPos;
    vec3 tangentViewPos;
    mat3 TBN;
    vec3 worldFragPos; // Añadido
} FSIn;

uniform sampler2D uDiffuse;
uniform sampler2D uNormalMap;
uniform sampler2D uHeightMap;
uniform float uHeightScale;

layout(location = 0) out vec4 oColor;

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
    vec2 finalTexCoords = mix(currentTexCoords, prevTexCoords, weight);

    // Cálculo del desplazamiento 3D
    // Calcular desplazamiento en espacio tangente y convertir a mundo
    float totalLayerDepth = (currentLayerDepth - layerDepth) + weight * layerDepth;
    float t = totalLayerDepth * uHeightScale;
        vec3 displacementTS = viewDirTS * t; // Usar viewDirTS corregido
    vec3 displacementWS = FSIn.TBN * displacementTS;

    // Nueva posición del fragmento (en mundo)
    vec3 newFragPos = FSIn.worldFragPos + displacementWS;

    // Calcular profundidad corregida
    vec4 viewPos = uView * vec4(newFragPos, 1.0);
    vec4 clipPos = uProj * viewPos;
    gl_FragDepth = (clipPos.z / clipPos.w) * 0.5 + 0.5; // Normalizar a [0, 1]

    return finalTexCoords;
}

void main() {
    vec3 viewDirTS = normalize(FSIn.tangentViewPos - FSIn.tangentFragPos);

    vec2 displacedTexCoords = ParallaxOcclusionMapping(FSIn.texCoords, viewDirTS);

    if (displacedTexCoords.x < 0.0 || displacedTexCoords.x > 1.0 ||
        displacedTexCoords.y < 0.0 || displacedTexCoords.y > 1.0)
        discard;

    vec3 normalTS = texture(uNormalMap, displacedTexCoords).rgb * 1.0 - 2.0;

    vec3 normalWS = normalize(FSIn.TBN * (normalTS * 2.0 - 1.0));

    vec3 albedo = texture(uDiffuse, displacedTexCoords).rgb;

    //Todo
    vec3 lightDir = normalize(vec3(0.5, 1., 1.0));
    float diff = max(dot(normalWS, lightDir), 0.1);

    oColor = vec4(albedo * diff, 1.0);

    // Debug normal
    //oColor = vec4(normalWS * 0.5 + 0.5, 1.0);

    // Debug Heigh
    //oColor = vec4(texture(uHeightMap, displacedTexCoords).rrr, 1.0);
}

#endif
#endif
