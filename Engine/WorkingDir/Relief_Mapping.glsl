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

out Data {
    vec2 texCoords;
    vec3 fragPosView;
    mat3 TBN;
} VSOut;

void main(void)
{
    vec3 T = normalize(mat3(uModel) * tangent);
    vec3 B = normalize(mat3(uModel) * bitangent);
    vec3 N = normalize(mat3(uModel) * normal);
    VSOut.TBN = mat3(T, B, N);

    vec4 worldPos = uModel * vec4(position, 1.0);
    VSOut.fragPosView = vec3(uView * worldPos);
    VSOut.texCoords = texCoords;

    gl_Position = uProj * uView * worldPos;
}

#elif defined(FRAGMENT)

in Data {
    vec2 texCoords;
    vec3 fragPosView;
    mat3 TBN;
} FSIn;

uniform sampler2D uDiffuse;
uniform sampler2D uBump;
uniform sampler2D uDepthMap;

uniform vec3 uViewPos;
uniform float uStrength;

layout(location = 0) out vec4 oColor;

// Relief mapping sampling
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));

    float layerDepth = 1.0 / numLayers;
    vec2 P = viewDir.xy * uStrength;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentDepth = 0.0;

    float depthFromMap = texture(uDepthMap, currentTexCoords).r;

    while (currentDepth < depthFromMap)
    {
        currentTexCoords -= deltaTexCoords;
        depthFromMap = texture(uDepthMap, currentTexCoords).r;
        currentDepth += layerDepth;
    }

    return currentTexCoords;
}

void main()
{
    vec3 viewDirTangent = normalize(FSIn.TBN * normalize(-FSIn.fragPosView));
vec2 displacedTexCoords = ParallaxMapping(FSIn.texCoords, viewDirTangent);

    vec3 albedo = texture(uDiffuse, displacedTexCoords).rgb;
    vec3 normalMap = texture(uBump, displacedTexCoords).rgb;
    vec3 normal = normalize(FSIn.TBN * (normalMap * 2.0 - 1.0));

    float depth = texture(uDepthMap, displacedTexCoords).r;

    oColor = vec4(albedo * (normal * 0.5 + 0.5), depth);
}

#endif
#endif
