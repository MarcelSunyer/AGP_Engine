// FORWARD.glsl
#ifdef FORWARD
#if defined(VERTEX)
// Vertex shader code similar to your existing geometry pass
#elif defined(FRAGMENT)
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

uniform sampler2D uAlbedoTexture;
uniform sampler2D uNormalTexture;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 oColor;

vec3 CalcLighting(Light light, vec3 normal, vec3 viewDir, vec3 albedo) {

}

void main() {
    vec3 albedo = texture(uAlbedoTexture, vTexCoord).rgb;
    vec3 normal = normalize(vNormal);
    
    if (textureSize(uNormalTexture, 0).x > 1) {
        normal = texture(uNormalTexture, vTexCoord).rgb * 2.0 - 1.0;
    }

    vec3 viewDir = normalize(uCameraPosition - vPosition);
    vec3 result = vec3(0.0);

    for (int i = 0; i < uLightCount; ++i) {
        result += CalcLighting(uLight[i], normal, viewDir, albedo);
    }

    oColor = vec4(result, 1.0);
}
#endif
#endif