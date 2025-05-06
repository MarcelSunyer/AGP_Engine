#ifdef Render_Quad

#if defined(VERTEX)

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT)

struct Light {
    int type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(binding = 0) uniform GlobalParams {
    vec3 uCameraPosition;
    int uLightCount;
    Light uLight[128];
};

in vec2 vTexCoord;

uniform sampler2D uColor;
uniform sampler2D uNormals;
uniform sampler2D uPosition;
uniform sampler2D uViewDir;
uniform sampler2D uDepth;

uniform float uNear = 0.01;
uniform float uFar = 5.0;
uniform int uViewMode; // 0=main, 1=albedo, 2=normals, 3=position, 4=viewdir, 5=depth
uniform int uShowDepth;
uniform float uDepthIntensity = 0.5;

layout(location = 0) out vec4 oColor;

float LinearizeDepth(float depth) {
    float z = depth * 5.0 - 1.0;
    return (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear));
}

vec3 CalcPointLight(Light light, vec3 normal, vec3 position, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - position);
    float distance = length(light.position - position);
    float attenuation = 1.6 / (1.0+ 0.1 * (distance * distance));

    vec3 ambient = light.color * 0.5;
    float diff = max(dot(normal, lightDir), 1.2);
    vec3 diffuse = light.color * diff;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); // Implicit normalization like Code 1
    vec3 specular = 0.5 * spec * light.color; // Higher intensity from Code 2

    return (ambient + diffuse + specular) * attenuation;
}
vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * diff;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = light.color * spec * 0.5;
    return diffuse + specular;
}

void main() {
    
    // Main render mode
    vec3 baseColor = texture(uColor, vTexCoord).rgb;
    vec3 normal = normalize(texture(uNormals, vTexCoord).rgb * 2.0 - 1.0);
    vec3 position = texture(uPosition, vTexCoord).rgb;
    vec3 viewDir = normalize(uCameraPosition - position);
   
    // Buffer visualization modes
    if (uViewMode == 1) { // Albedo
        oColor = texture(uColor, vTexCoord);
        return;
    }
    else if (uViewMode == 2) { // Normals
        normal = texture(uNormals, vTexCoord).rgb * 2.0 - 1.0;
        oColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }
    else if (uViewMode == 3) { // Position
        oColor = texture(uPosition, vTexCoord);
        return;
    }
    else if (uViewMode == 4) { // ViewDir
        oColor = texture(uViewDir, vTexCoord);
        return;
    }
    else if (uViewMode == 5) { // Depth visualization
        float depth = texture(uDepth, vTexCoord).r;
        float linearDepth = LinearizeDepth(depth) / uFar;
        // Exaggerate contrast using a power function; adjust exponent as needed
        linearDepth = pow(linearDepth,0.75);
        oColor = vec4(vec3(linearDepth), 1.0);
        return;
    }
    
    // Lighting calculations
    vec3 finalColor = vec3(0.0);
    for (int i = 0; i < uLightCount; ++i) {
        if (uLight[i].type == 0) {
            finalColor += CalcDirLight(uLight[i], normal, viewDir) * baseColor;
        } else {
            finalColor += CalcPointLight(uLight[i], normal, position, viewDir) * baseColor;
        }
    }

    // Apply depth visualization on top of the lighting if enabled
    if (uShowDepth == 1) {
        float depth = texture(uDepth, vTexCoord).r;
        float linearDepth = LinearizeDepth(depth) / uFar;
        // Exaggerate contrast on depth values for a more dramatic black and white effect
        linearDepth = pow(linearDepth, 3.0);
        finalColor = mix(finalColor, vec3(linearDepth), uDepthIntensity);
    }

    oColor = vec4(finalColor, 1.0);
}

#endif
#endif
