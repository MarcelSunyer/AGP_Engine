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
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(vWorldPos - uCameraPosition);

    vec3 reflectDir = reflect(viewDir, normal);
    vec3 reflection = texture(skybox, reflectDir).rgb;

    vec3 diffuse = texture(skybox, normal).rgb;
    float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 5.0);
    fresnel = mix(0.1, 1.0, fresnel); // para evitar valores muy bajos
     
    vec3 specular = reflection * fresnel;

    vec3 finalColor = diffuse * 0.45 + specular * 0.02;

    FragColor = vec4(finalColor, 1.0);
}

#endif
#endif
