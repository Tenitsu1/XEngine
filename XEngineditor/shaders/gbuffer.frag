#version 460 core

layout (location = 0) out vec4 gPosition;   
layout (location = 1) out vec4 gNormal;     
layout (location = 2) out vec4 gAlbedoSpec; 
layout (location = 3) out vec4 gEmission;   

in vec3 FragPos;
in vec2 TexCoords;
in vec3 Normal;


struct Material {
    vec4 baseColorFactor;
    vec4 emissionFactor;
    float metallicFactor;
    float roughnessFactor;
    
    bool useBaseColorMap;
    bool useMetallicRoughnessMap;
    bool useNormalMap;
    bool useEmissiveMap;
};

uniform Material material;


uniform sampler2D texture_baseColor;        // Base Color
uniform sampler2D texture_metallicRoughness;// G: Roughness, B: Metallic
uniform sampler2D texture_normal;           // Normal Map
uniform sampler2D texture_emissive;         // Emissive Map

vec3 getNormalFromMap()
{
    if (!material.useNormalMap) return normalize(Normal);

    vec3 tangentNormal = texture(texture_normal, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(FragPos);
    vec3 Q2  = dFdy(FragPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{    

    gPosition = vec4(FragPos, 1.0);


    vec3 albedo = material.baseColorFactor.rgb;
    if (material.useBaseColorMap) {
        albedo *= texture(texture_baseColor, TexCoords).rgb;
    }

    // --- Metallic & Roughness ---
    float metallic = material.metallicFactor;
    float roughness = material.roughnessFactor;
    
    if (material.useMetallicRoughnessMap) {
        vec4 mrSample = texture(texture_metallicRoughness, TexCoords);
        roughness *= mrSample.g;
        metallic  *= mrSample.b;
    }

    // --- Normal ---
    vec3 N = getNormalFromMap();

    // --- Emission ---
    vec3 emission = material.emissionFactor.rgb;
    if (material.useEmissiveMap) {
        emission *= texture(texture_emissive, TexCoords).rgb;
    }

    // G-Buffer
    gNormal     = vec4(N, roughness);       // Normal + Roughness
    gAlbedoSpec = vec4(albedo, metallic);   // Albedo + Metallic
    gEmission   = vec4(emission, 1.0);      // Emission
}