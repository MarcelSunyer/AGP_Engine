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
uniform float intesity;

void main() {
    // Normalizar vectores
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPosition - vWorldPos);
    
    // Calcular Fresnel corregido
    float fresnel = pow(1.0 - clamp(dot(viewDir, normal), 0.0, 1.0), 5.0); // Paréntesis y parámetros correctos
    
    // Calcular reflexión
    vec3 reflectDir = reflect(-viewDir, normal);
    vec3 reflection = texture(skybox, reflectDir).rgb;
    
    // Aplicar intensidad y Fresnel
    FragColor = vec4(reflection * (fresnel * intesity + 0.2), 1.0); // Base reflectante + mínimo brillo
}

#endif
#endif