in Data {
    vec2 texCoords;
    vec3 fragPos;
    vec3 tangentFragPos;
    vec3 tangentViewPos;
    mat3 TBN;
} FSIn;

uniform sampler2D uDiffuse;
uniform sampler2D uNormalMap;
uniform sampler2D uHeightMap;
uniform float uHeightScale = 0.05;  // Intensidad del height map

layout(location = 0) out vec4 oColor;

// Parallax Mapping mejorado
vec2 ParallaxOcclusionMapping(vec2 texCoords, vec3 viewDir) { 
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.4));
    
    float layerDepth = 1.0 / numLayers;
    vec2 deltaTexCoords = viewDir.xy * uHeightScale / (viewDir.z * numLayers);
    
    // Ray marching
    vec2 currentTexCoords = texCoords;
    float currentDepth = 0.0;
    float depthFromMap = 1.0 - texture(uHeightMap, currentTexCoords).r;
    
    while(currentDepth < depthFromMap) {
        currentTexCoords -= deltaTexCoords;
        depthFromMap = 1.0 - texture(uHeightMap, currentTexCoords).r;
        currentDepth += layerDepth;
    }
    
    // Interpolación para suavizar
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = depthFromMap - currentDepth;
    float beforeDepth = 1.0 - texture(uHeightMap, prevTexCoords).r - (currentDepth - layerDepth);
    float weight = afterDepth / (afterDepth - beforeDepth);
    
    return prevTexCoords * weight + currentTexCoords * (1.0 - weight);
}

void main() {
    // 1. Calcular dirección de vista en espacio tangente
    vec3 viewDir = normalize(FSIn.tangentViewPos - FSIn.tangentFragPos);
    
    // 2. Aplicar Parallax Occlusion Mapping
    vec2 displacedTexCoords = ParallaxOcclusionMapping(FSIn.texCoords, viewDir);
    if(displacedTexCoords.x < 0.0 || displacedTexCoords.x > 1.0 || 
       displacedTexCoords.y < 0.0 || displacedTexCoords.y > 1.0)
        discard;
    
    // 3. Obtener normal desde el normal map (con corrección de espacio)
    vec3 normalMap = texture(uNormalMap, displacedTexCoords).rgb * 2.0 - 1.0;
    normalMap.g *= -1.0;  // Invertir canal Y si es necesario
    vec3 worldNormal = normalize(FSIn.TBN * normalMap);
    
    // 4. Muestra de textura difusa
    vec3 albedo = texture(uDiffuse, displacedTexCoords).rgb;
    
    // 5. Iluminación básica (ejemplo con luz direccional)
    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));
    float diff = max(dot(worldNormal, lightDir), 0.2);  // 0.2 = luz ambiental
    
    // 6. Resultado final
    oColor = vec4(albedo * diff, 1.0);
    
    // Opcional: Visualización de normales para debug
    // oColor = vec4(worldNormal * 0.5 + 0.5, 1.0);
}