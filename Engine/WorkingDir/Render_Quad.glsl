#ifdef Render_Quad

#if defined(VERTEX) ////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;


out vec2 vTexCoord;

void main()
{
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 1.0);
   
}

#elif defined(FRAGMENT) ////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uAlbedo;
uniform sampler2D uNormals;
uniform sampler2D uPosition;
uniform sampler2D uViewDir;

layout(location=0) out vec4 oColor;

void main()
{
    oColor = texture(uAlbedo, vTexCoord);
}

#endif
#endif


