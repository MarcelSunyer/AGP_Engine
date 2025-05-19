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
} VSOut;

void main() {
    // Calcular posición en mundo
    vec3 fragPos = vec3(uModel * vec4(position, 1.0));
    VSOut.texCoords = texCoords;

    // Calcular matriz TBN
    vec3 T = normalize(mat3(uModel) * tangent);
    vec3 B = normalize(mat3(uModel) * bitangent);
    vec3 N = normalize(mat3(uModel) * normal);
    VSOut.TBN = mat3(T, B, N);
    VSOut.TBN = transpose(VSOut.TBN);

    // Posiciones en espacio tangente
    VSOut.tangentFragPos = VSOut.TBN * fragPos;
    VSOut.tangentViewPos = VSOut.TBN * uViewPos;

    gl_Position = uProj * uView * vec4(fragPos, 1.0);
}

#elif defined(FRAGMENT)

in Data {
    vec2 texCoords;
    vec3 tangentFragPos;
    vec3 tangentViewPos;
    mat3 TBN;
} FSIn;

uniform sampler2D uDiffuse;
uniform sampler2D uNormalMap;
uniform sampler2D uHeightMap;
uniform float uHeightScale;

layout(location = 0) out vec4 oColor;

// Función de Parallax Occlusion Mapping con interpolación
vec2 ParallaxOcclusionMapping(vec2 texCoords, vec3 viewDirTS) {
    const float minLayers = 8.0;  // Reducido para mejor performance
const float maxLayers = 32.0;

    float ndotv = clamp(dot(vec3(0.0, 0.0, 1.0), normalize(viewDirTS)), 0.0, 1.0);
    float numLayers = mix(maxLayers, minLayers, ndotv);

    float layerDepth = 1.0 / numLayers;
    vec2 deltaTexCoords = viewDirTS.xy * uHeightScale / viewDirTS.z / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentDepth = 0.0;
    float depthFromMap = texture(uHeightMap, currentTexCoords).r;

    vec2 prevTexCoords = currentTexCoords;
    float prevDepthMap = depthFromMap;

    for (int i = 0; i < int(numLayers); ++i) {
        if (currentDepth >= depthFromMap)
            break;

        prevTexCoords = currentTexCoords;
        prevDepthMap = depthFromMap;

        currentTexCoords -= deltaTexCoords;
        currentDepth += layerDepth;
        depthFromMap = texture(uHeightMap, currentTexCoords).r;
    }

    // Interpolación suave entre los dos últimos pasos
    float afterDepth = depthFromMap - currentDepth;
    float beforeDepth = prevDepthMap - (currentDepth - layerDepth);
    float weight = beforeDepth / (beforeDepth - afterDepth);

    vec2 finalTexCoords = mix(currentTexCoords, prevTexCoords, clamp(weight, 0.0, 1.0));

    return finalTexCoords;
}

void main() {
    // Dirección de vista en espacio tangente
    vec3 viewDirTS = normalize(FSIn.tangentViewPos - FSIn.tangentFragPos);

    // Aplicar Relief Mapping
    vec2 displacedTexCoords = ParallaxOcclusionMapping(FSIn.texCoords, viewDirTS);

    // Recorte si se sale del UV
    if (displacedTexCoords.x < 0.0 || displacedTexCoords.x > 1.0 ||
        displacedTexCoords.y < 0.0 || displacedTexCoords.y > 1.0)
        discard;

    // Normal en espacio tangente y conversión a mundo
    vec3 normalTS = texture(uNormalMap, displacedTexCoords).rgb * 1.0 - 2.0;
    // normalTS.g *= -1.0; // Solo si tu normal map usa sistema Y-invertido
    vec3 normalWS = normalize(FSIn.TBN * normalTS);

    // Difusa
    vec3 albedo = texture(uDiffuse, displacedTexCoords).rgb;

    // Iluminación simple direccional
    vec3 lightDir = normalize(vec3(0.5, 1., 1.0));
    float diff = max(dot(normalWS, lightDir), 0.2); // 0.2 = luz ambiente mínima

    oColor = vec4(albedo * diff, 1.0);

    // Debug normal
    // oColor = vec4(normalWS * 0.5 + 0.5, 1.0);

    // Debug Heigh
    //oColor = vec4(texture(uHeightMap, displacedTexCoords).rrr, 1.0);
}

#endif
#endif
