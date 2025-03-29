#ifdef RENDER_GEOMETRY

#if defined(VERTEX) ////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in vec3 aTangent;
layout(location=4) in vec3 aBitangent;

struct Light
{
    int type;
    vec3 color;
    vec3 direction;
    vec3 position;

};

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    int uLightCount;
    Light uLight[16];
};

layout(binding = 1, std140) uniform EntityParams
{
    mat4 uWorldMatrix;
    mat4 uWorldViewProjectionMatrix;
};


out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vViewDir;

void main()
{
    vTexCoord = aTexCoord;
    vPosition = vec3(uWorldMatrix * vec4(aPosition,1.0));
    vNormal = vec3(uWorldMatrix * vec4(aNormal,0.0));
    vViewDir = uCameraPosition - vPosition;
    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ////////////////////////////////////////

struct Light
{
    int type;
    vec3 color;
    vec3 direction;
    vec3 position;

};

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    int uLightCount;
    Light uLight[16];
};

in vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vViewDir;

uniform sampler2D uTexture;
layout(location=0) out vec4 oColor;

void main()
{
    vec3 lightDir = normalize(-uLight[0].direction);
    float diff = max(dot(vNormal,lightDir),0.0);
    vec3 reflectDir = reflect(-lightDir,vNormal);
    float spec = pow(max(dot(vViewDir,reflectDir),0.0),0.0);
    vec3 ambient = uLight[0].color * vec3(texture(uTexture,vTexCoord));

    oColor = vec4(ambient,1.0);
}

#endif
#endif

