#ifdef RELIEF_MAPPING

#if defined(VERTEX)

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;

out vec2 vTexCoord;
out vec3 vViewDirTS;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uCameraPosition;

void main()
{
    vTexCoord = aTexCoord;

    // Build TBN matrix for tangent space
    vec3 T = normalize(mat3(uModel) * aTangent);
    vec3 N = normalize(mat3(uModel) * aNormal);
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    vec3 fragPosWorld = vec3(uModel * vec4(aPosition, 1.0));
    vec3 viewDir = normalize(uCameraPosition - fragPosWorld);
    vViewDirTS = TBN * viewDir;

    gl_Position = uProj * uView * uModel * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT)

uniform sampler2D uDiffuse;
uniform sampler2D uBump; // RGB = normal, A = height

in vec2 vTexCoord;
layout(location = 0) out vec4 oColor;

void main() {
    vec3 albedo = texture(uDiffuse, vTexCoord).rgb;
    vec3 normal = texture(uBump, vTexCoord).rgb; // usa uBump explícitamente

    // Simple mezcla de albedo y normal para evitar optimización
    vec3 color = mix(albedo, normal, 0.5);
    oColor = vec4(color, 1.0);
}


#endif
#endif