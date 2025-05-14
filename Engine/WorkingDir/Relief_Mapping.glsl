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

in vec2 vTexCoord;
in vec3 vViewDirTS;

uniform sampler2D uDiffuse;
uniform sampler2D uBump; // RGB = normal, A = height

uniform float heightScale = 0.05;

layout(location = 0) out vec4 oColor;

void main()
{
    // De momento: no hay parallax aún, simplemente muestreamos los valores

    // Diffuse color
    vec3 color = texture(uDiffuse, vTexCoord).rgb;

    // Normal map (de -1 a 1)
    vec3 normalTS = texture(uBump, vTexCoord).rgb;
    normalTS = normalize(normalTS * 2.0 - 1.0);

    // Height (en canal alpha)
    float height = texture(uBump, vTexCoord).a;

    // Visualización simple: devolver color + normal en RGB + height en alpha
    oColor = vec4(color * 1.5 + normalTS * 0.5, height);
}

#endif
#endif
