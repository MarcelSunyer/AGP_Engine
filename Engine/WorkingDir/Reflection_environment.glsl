#ifdef REFLECTION_ENVIRONMENT

#if defined(VERTEX)
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;  // Añadido: Entrada UV

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;  // Añadido: Salida UV

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vUV = aUV;  // Añadido: Pasar UV al fragment shader

    gl_Position = uProj * uView * worldPos;
}

#elif defined(FRAGMENT)

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV; 
out vec4 FragColor;

uniform samplerCube skybox;
uniform vec3 uCameraPosition;
uniform float intensity;  // Corregido: Typo "intesity" -> "intensity"

uniform sampler2D albedo;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(vWorldPos - uCameraPosition);

    vec3 reflectDir = reflect(viewDir, normal);
    vec3 reflection = texture(skybox, reflectDir).rgb;
    vec3 diffuse = texture(skybox, normal).rgb;

    vec3 finalColor = reflection * 0.45 + diffuse * 0.02;
    
    vec3 albedoColor = texture(albedo, vUV).rgb;
    finalColor *= albedoColor;

    FragColor = vec4(finalColor, 1.0);  // Añadido: Asignar color final
}

#endif
#endif