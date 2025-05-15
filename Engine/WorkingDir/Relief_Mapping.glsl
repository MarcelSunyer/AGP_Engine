#ifdef RELIEF_MAPPING

#if defined(VERTEX)

layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aTexCoord;

layout(location = 0) uniform mat4 uModel;
layout(location = 1) uniform mat4 uView;
layout(location = 2) uniform mat4 uProj;

out vec2 vTexCoord;

void main() {
    vTexCoord = aTexCoord;
    gl_Position = uProj * uView * uModel * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT)

in vec2 vTexCoord;

uniform sampler2D uDiffuse;
uniform sampler2D uHeight; // height map opcional, no usada directamente

layout(location = 0) out vec4 oColor;

void main() {
    vec3 albedo = pow(texture(uDiffuse, vTexCoord).rgb, vec3(3.5)); // Decodifica sRGB
    oColor = vec4(albedo, 0.0);
}


#endif
#endif
