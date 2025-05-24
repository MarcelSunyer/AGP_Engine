#ifdef CUBEMAP

#if defined(VERTEX)

layout(location = 0) in vec3 position;

out vec3 texCoords;

uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    vec4 pos = uProj * uView * vec4(position, 1.0);
    gl_Position = pos.xyww;
    texCoords = position;
}

#elif defined(FRAGMENT)

out vec4 oColor;

in vec3 texCoords;

uniform samplerCube skybox;

void main()
{
    oColor = texture(skybox, texCoords) *0.5;
}
#endif
#endif