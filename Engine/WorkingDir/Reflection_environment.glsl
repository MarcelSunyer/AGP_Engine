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
uniform float reflectionIntensity = 1.02;
uniform int cubeMapType;
uniform float roughness = 0.2;
uniform float metallic = 0.8;
uniform float diffus_amb;

// Parámetros de iluminación
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];
uniform int numLights = 1;

const float PI = 3.14159265359;

// Función para calcular la distribución normal (Trowbridge-Reitz GGX)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// Función de geometría (Schlick GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// Función de geometría Smith
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Fresnel-Schlick aproximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    // Normalizar vectores
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPosition - vWorldPos);
    float cosTheta = dot(viewDir, normal);
    float fresnel = pow(1 - clamp(cosTheta, 0.0, 1.0), 5.0);

    // Calcular reflexión base
    vec3 reflectDir = reflect(-viewDir, normal);
    vec3 reflection = textureLod(skybox, reflectDir, roughness * 10.0).rgb;

    // Calcular refracción (solo para modo 1)
    vec3 refraction = vec3(0.0);
    if(cubeMapType == 1) {
        float ratio = 1.0 / reflectionIntensity;
        vec3 refractDir = refract(-viewDir, normal, ratio);
        if (length(refractDir) > 0.001) {
            refraction = texture(skybox, refractDir).rgb;
        } else {
            refraction = reflection; // Reflexión interna total
        }
    }

    vec3 Lo = vec3(0.0);
    vec3 ambient = vec3(0.0);
    vec3 F0 = vec3(0.04);
    vec3 diffuseEnv = vec3(0.0);
    vec3 specularEnv = vec3(0.0);

    // Solo calcular componentes PBR para modos que no son refracción
    if(cubeMapType != 1) {
        diffuseEnv = texture(skybox, normal).rgb;
        specularEnv = textureLod(skybox, reflectDir, roughness * 10.0).rgb;
        F0 = mix(F0, specularEnv, metallic);
        ambient = 0.03 * diffuseEnv;

        // Calcular iluminación PBR
        for(int i = 0; i < numLights; ++i) {
            vec3 lightDir = normalize(lightPositions[i] - vWorldPos);
            vec3 halfDir = normalize(viewDir + lightDir);
            
            float distance = length(lightPositions[i] - vWorldPos);
            float attenuation = 1.0 / (distance * distance);
            vec3 radiance = lightColors[i] * attenuation;
            
            float cosTheta = max(dot(halfDir, viewDir), 0.0);
            vec3 F = fresnelSchlick(cosTheta, F0);
            
            float NDF = DistributionGGX(normal, halfDir, roughness);
            float G = GeometrySmith(normal, viewDir, lightDir, roughness);
            
            vec3 nominator = NDF * G * F;
            float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.001;
            vec3 specular = nominator / denominator;
            
            vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
            float NdotL = max(dot(normal, lightDir), 0.0);
            
            Lo += (kD * diffuseEnv / PI + specular) * radiance * NdotL;
        }
    }

    // Combinar componentes según modo
    vec3 mainColor;
    if(cubeMapType == 0) {        // Reflexión con PBR
        mainColor = reflection * fresnel;
    } else if(cubeMapType == 1) { // Refracción pura
        mainColor = refraction * (0.55 - fresnel);
    } else if(cubeMapType == 2) { // Mixto con PBR
        mainColor = mix(refraction, reflection, fresnel);
    } else {
        mainColor = vec3(0.0);
    }

    // Combinar con iluminación (solo para modos no-refracción)
    vec3 color;
    if(cubeMapType == 1) {
        color = mainColor * diffus_amb / 8; 
    } else {
        color = (3* diffus_amb * ambient) + Lo + mainColor;
    }

    FragColor = vec4(color, 1.0);
}
#endif
#endif