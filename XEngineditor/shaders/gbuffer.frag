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

vec3 sRGBToLinear(vec3 color) {
    return pow(color, vec3(1));
}

vec3 getNormalFromMap()
{
    if (!material.useNormalMap) return normalize(Normal);

    // 這裡讀取的 texture_normal 必須是線性的 (在 Loader 中修正)
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
    // Position 保持原樣 (World Space)
    gPosition = vec4(FragPos, 1.0);

    // --- Albedo (Base Color) ---
    vec3 albedo = material.baseColorFactor.rgb;
    if (material.useBaseColorMap) {
        // 取樣並手動轉為 Linear 空間，因為我們稍後會將 Loader 改為 GL_RGBA
        vec3 texColor = texture(texture_baseColor, TexCoords).rgb;
        albedo *= sRGBToLinear(texColor); 
    }

    // --- Metallic & Roughness ---
    float metallic = material.metallicFactor;
    float roughness = material.roughnessFactor;
    
    if (material.useMetallicRoughnessMap) {
        // 這些貼圖本身就是 Linear 的，直接讀取即可
        vec4 mrSample = texture(texture_metallicRoughness, TexCoords);
        roughness *= mrSample.g;
        metallic  *= mrSample.b;
    }

    // --- Normal ---
    vec3 N = getNormalFromMap();

    // --- Emission ---
    vec3 emission = material.emissionFactor.rgb;
    if (material.useEmissiveMap) {
        vec3 emissiveSample = texture(texture_emissive, TexCoords).rgb;
        emission *= sRGBToLinear(emissiveSample);
    }

    // --- G-Buffer Output ---
    
    // [修正重點] 將法線從 [-1, 1] 映射到 [0, 1]
    // 這樣 Debug 視窗就不會出現全黑，同時解決 RGBA8 格式截斷負數的問題
    // 注意：在 Lighting Pass 讀取時，記得解碼： normal = texture(gNormal).xyz * 2.0 - 1.0;
    gNormal = vec4(N * 0.5 + 0.5, roughness);       

    gAlbedoSpec = vec4(albedo, metallic);   
    gEmission   = vec4(emission, 1.0);   
}