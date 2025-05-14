#ifdef RELIEF_MAPPING


#if defined(VERTEX)

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

layout(std140, binding = 0) uniform GlobalData {
    vec3 cameraPos;
    int numLights;
    // otros datos...
};

layout(std140, binding = 1) uniform EntityData {
    mat4 model;
    mat4 vp;
    mat4 normalMatrix;
};

out VS_OUT {
    vec2 TexCoords;
    vec3 FragPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    mat3 TBN;
} vs_out;

void main()
{
    vs_out.TexCoords = aTexCoords;
    vec3 T = normalize(mat3(model) * aTangent);
    vec3 B = normalize(mat3(model) * aBitangent);
    vec3 N = normalize(mat3(model) * aNormal);
    vs_out.TBN = transpose(mat3(T, B, N)); // para pasar a espacio tangente

    vec3 fragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.FragPos = fragPos;
    vs_out.TangentFragPos = vs_out.TBN * fragPos;
    vs_out.TangentViewPos = vs_out.TBN * cameraPos;

    gl_Position = vp * model * vec4(aPos, 1.0);
}
#endif

#elif defined(FRAGMENT)

in VS_OUT {
    vec2 TexCoords;
    vec3 FragPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    mat3 TBN;
} fs_in;

uniform sampler2D uDiffuse;
uniform sampler2D uNormal;
uniform sampler2D uHeight;

uniform float heightScale;

out vec4 FragColor;

// Relief Mapping
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(uHeight, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(uHeight, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    return currentTexCoords;
}

void main()
{
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec2 texCoords = ParallaxMapping(fs_in.TexCoords, viewDir);

    if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;

    vec3 normal = texture(uNormal, texCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(fs_in.TBN * normal);

    vec3 color = texture(uDiffuse, texCoords).rgb;

    vec3 lightDir = normalize(vec3(0.0, 0.0, 1.0));
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * color;

    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // todo rojo

}
#endif
