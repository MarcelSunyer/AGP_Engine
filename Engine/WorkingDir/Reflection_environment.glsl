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

    float _intesity = intesity +1;
    // Calcular dirección de reflexión
    vec3 viewDir = normalize(vWorldPos - uCameraPosition);
    vec3 normal = normalize(vNormal);
    vec3 reflectDir = reflect(viewDir, normal);
    
    // Muestrear el cubemap
    vec3 reflection = texture(skybox, reflectDir).rgb;

    // Mezclar con color base (opcional)
    vec3 baseColor = vec3(0.8, 0.8, 0.8);
    FragColor = vec4(mix(baseColor, reflection / (3 * _intesity) , 1), 1);
}

#endif
#endif