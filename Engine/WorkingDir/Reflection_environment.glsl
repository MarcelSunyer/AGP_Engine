#ifdef REFLECTION_ENVIRONMENT
#if defined(VERTEX)
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

out vec3 vWorldPos;
out vec3 vNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    
    gl_Position = uProj * uView * worldPos;
}

#elif defined(FRAGMENT)
in vec3 vWorldPos;
in vec3 vNormal;


out vec4 FragColor;

uniform samplerCube skybox;
uniform vec3 uCameraPosition;
uniform float refractiveIndex = 1.02;
uniform int cubeMapType;

void main() {
    // Normalize vectors
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPosition - vWorldPos);

    float cosTheta = dot(viewDir, normal);
    
    // Fresnel effect (controls reflection/refraction blend)
    float fresnelReflection = pow(1.0 - clamp(cosTheta, 0.0, 1.0), 5.0);

    // Fresnel para refracción (mayor en ángulos frontales, inverso al de reflexión)
    float fresnelRefraction = 1.0 - fresnelReflection;

    // Reflexión (aplicar Fresnel)
    vec3 reflectDir = reflect(-viewDir, normal);
    vec3 reflection = (texture(skybox, reflectDir).rgb * fresnelRefraction) /2;

    // Refracción (aplicar Fresnel inverso)
    float ratio = refractiveIndex / 1.0; 
    vec3 refractDir = refract(-viewDir, normal, ratio);
    vec3 refraction = (texture(skybox, refractDir).rgb * fresnelRefraction) / 2;
    
    if (cubeMapType == 0)
    {
        FragColor = vec4(reflection /1.77, 1.0);
    }

    if (cubeMapType == 1)
    {
        FragColor = vec4(refraction/1.77, 1.0);
    
    }
    if (cubeMapType == 2)
    {
        FragColor = vec4((reflection * refraction) /10, 1.0);
    
    }

}
#endif
#endif