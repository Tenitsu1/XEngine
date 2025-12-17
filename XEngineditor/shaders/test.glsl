#version 460 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;


// --- struct ---
struct BVHNode {
    vec3 aabbMin;
    int left;
    vec3 aabbMax;
    int right;
    #define start left
    #define count right
};

struct Material {
    vec4 baseColorFactor;
    vec4 emissionFactor;
    float metallicFactor;
    float roughnessFactor;
    float transmissionFactor;
    float ior;
    int baseColorTexture;
    int metallicRoughnessTexture;
    int normalTexture;
    int emissiveTexture;
};

struct HitRecord {
    vec3 point;
    vec3 color;
    int materialID;
    float t;
    vec3 shadingNormal;
    vec3 geoNormal;
    bool frontFace;
    vec2 uv;
    float metallic;
    float roughness;
};

struct Ray {
    vec3 origin;
    vec3 direction;
    vec3 invDirection;
};

struct Camera {
    vec3 position;
    vec3 lookat;
    vec3 up;
    float fov;
};

struct PackedTriangle {
    vec4 v0; // w = materialID
    vec4 e1;
    vec4 e2;
};


// --- SSBO 定義 ---
layout(rgba32f, binding = 0) uniform image2D screenTexture;
layout(std430, binding = 2) buffer BVHNodeBuffer               { BVHNode bvhNodes[]; };
layout(std430, binding = 3) buffer VerticesBuffer              { vec4 vertices[]; };
layout(std430, binding = 4) buffer IndicesBuffer               { ivec4 indices[]; };
layout(std430, binding = 5) buffer GeoNormalsBuffer            { vec4 geoNormals[]; };
layout(std430, binding = 6) buffer TexCoordsBuffer             { vec2 texCoords[]; };
layout(std430, binding = 7) buffer MaterialIndicesBuffer       { int materialIndices[]; };
layout(std430, binding = 8) buffer MaterialsBuffer             { Material materials[]; };
layout(std430, binding = 9) buffer VertexNormalsBuffer         { vec4 vertexNormals[]; };
layout(std430, binding = 15) buffer PackedTriBuffer            { PackedTriangle packedTris[]; };


// --- Uniform ---
layout(binding = 10) uniform sampler2D gPosition;
layout(binding = 11) uniform sampler2D gNormal;
layout(binding = 12) uniform sampler2D u_envMap;
layout(binding = 20) uniform sampler2DArray u_textures; 


uniform vec2 u_resolution;
uniform float u_time;
uniform int SAMPLES_PER_PIXEL;
uniform int MAX_DEPTH;
uniform bool useBVH; 
uniform Camera camera;
uniform bool cameraUpdated;
uniform mat4 invViewProj;
uniform bool useEnvMap;
uniform float envIntensity;
uniform bool TimeDenoise;
uniform bool RayTracing;
uniform vec3 u_sunDirection; 
uniform vec3 u_sunColor;
uniform float BackgroundColor;


// --- const ---
const int MAX_STACK_SIZE = 32;
const float EPSILON = 1e-20;
const float PI = 3.14159265359;
const float INF = 1.0 / 0.0;
const int LIGHT = 1;
const vec2 invAtan = vec2(0.1591, 0.3183); // 1/(2*PI), 1/PI

bool isNanOrInf(vec3 color) {
    return any(isnan(color)) || any(isinf(color));
}

// --- random number ---
uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

uint hash(uvec2 v) { return hash(v.x ^ hash(v.y)); }
uint hash(uvec3 v) { return hash(v.x ^ hash(v.y) ^ hash(v.z)); }
uint hash(uvec4 v) { return hash(v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w)); }

float floatConstruct(uint m) {
    const uint ieeeMantissa = 0x007FFFFFu;
    const uint ieeeOne      = 0x3F800000u;
    m &= ieeeMantissa;
    m |= ieeeOne;
    float f = uintBitsToFloat(m);
    return f - 1.0;
}

float random(float x) { return floatConstruct(hash(floatBitsToUint(x))); }
float random(vec2  v) { return floatConstruct(hash(floatBitsToUint(v))); }
float random(vec3  v) { return floatConstruct(hash(floatBitsToUint(v))); }
float random(vec4  v) { return floatConstruct(hash(floatBitsToUint(v))); }

uint getCurrentState(ivec2 texelCoords, float frameCounter) {
    return hash(uvec3(texelCoords.x, texelCoords.y, uint(frameCounter)));
}


float RandomValue(inout uint state) {
    state = hash(state);
    return floatConstruct(state);
}

vec2 RandomDirection2D(inout uint state) {
    return vec2(RandomValue(state), RandomValue(state));
}

vec3 RandomDirection(inout uint state) {
    float z = RandomValue(state) * 2.0 - 1.0;
    float a = RandomValue(state) * 2.0 * PI;
    float r = sqrt(1.0 - z * z);
    return vec3(r * cos(a), r * sin(a), z);
}

// --- 輔助函數 ---
vec3 ACES_FilmicToneMapping(vec3 color) {
    // ACES Filmic Tone Mapping
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

Ray createRay(vec3 origin, vec3 dir) {
    Ray r;
    r.origin = origin;
    r.direction = dir;
    r.invDirection = 1.0 / dir;
    // r.invDirection = 1.0 / (dir + vec3(1e-6) * sign(dir)); 
    return r;
}

vec3 getRayDir(vec2 pixel) {
    const vec2 ndc = (pixel) / u_resolution * 2.0 - 1.0;
    const vec4 clip = vec4(ndc, -1.0, 1.0);
    vec4 eye = invViewProj * clip;
    eye /= eye.w;
    return normalize(eye.xyz - camera.position);
}

vec3 barycentric(vec3 n0, vec3 n1, vec3 n2, float u, float v) {
    return n0 * (1.0 - u - v) + n1 * u + n2 * v;
}

vec2 barycentric(vec2 n0, vec2 n1, vec2 n2, float u, float v) {
    return n0 * (1.0 - u - v) + n1 * u + n2 * v;
}

vec3 sRGBToLinear(vec3 color) {
    return pow(color, vec3(2.2));
}

vec4 getBaseColor(Material mat, vec2 uv) {
    vec4 color = mat.baseColorFactor;
    if (mat.baseColorTexture >= 0) {
        float layer = float(mat.baseColorTexture); 
        // [修改] 使用 textureLod 強制讀取 Level 0
        color = textureLod(u_textures, vec3(uv, layer), 0.0);

        color = vec4(sRGBToLinear(color.rgb), color.a);
    }
    return color;
}


vec2 getMetallicRoughness(Material mat, vec2 uv) {
    float m = mat.metallicFactor;
    float r = mat.roughnessFactor;
    
    if (mat.metallicRoughnessTexture >= 0) {
        float layer = float(mat.metallicRoughnessTexture);
        // [修改] 使用 textureLod
        vec4 mrSample = textureLod(u_textures, vec3(uv, layer), 0.0);
        
        m *= mrSample.b; // Blue channel for Metallic
        r *= mrSample.g; // Green channel for Roughness
    }
    
    return vec2(m, r);
}


vec3 getEmission(Material mat, vec2 uv) {
    vec3 emission = mat.emissionFactor.rgb;
    
    // 如果有自發光貼圖
    if (mat.emissiveTexture >= 0) {
        float layer = float(mat.emissiveTexture);
        // 取樣並轉 Linear (假設貼圖是 sRGB 編碼)
        // [修改] 使用 textureLod
        vec3 texColor = textureLod(u_textures, vec3(uv, layer), 0.0).rgb;
        emission *= sRGBToLinear(texColor);
    }
    
    // 乘上強度 (emissionFactor.a 儲存了強度)
    return emission * mat.emissionFactor.a;
}

vec3 GetEnvironmentColor(vec3 dir) {
    vec3 d = normalize(dir);
    vec2 uv = vec2(atan(d.z, d.x), asin(d.y)); 
    uv *= invAtan;
    uv += 0.5;
    // [修改] textureLod
    return textureLod(u_envMap, uv, 0.0).rgb; 
}

vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// PBR Sampling functions
vec3 cosine_weighted_direction(vec3 normal, inout uint state) {
    float r1 = RandomValue(state);
    float r2 = RandomValue(state);
    float phi = 2.0 * PI * r1;
    float cosTheta = sqrt(1.0 - r2);
    float sinTheta = sqrt(r2);
    vec3 H; H.x = sinTheta * cos(phi); H.y = sinTheta * sin(phi); H.z = cosTheta;
    vec3 U = abs(normal.x) > 0.1 ? vec3(0,1,0) : vec3(1,0,0);
    vec3 Tangent = normalize(cross(U, normal));
    vec3 Bitangent = cross(normal, Tangent);
    return normalize(Tangent * H.x + Bitangent * H.y + normal * H.z);
}

vec3 ImportanceSampleGGX(inout uint state, vec3 N, float roughness) {
    float r1 = RandomValue(state);
    float r2 = RandomValue(state);
    float a = roughness * roughness;
    float phi = 2.0 * PI * r1;
    float cosTheta = sqrt((1.0 - r2) / (1.0 + (a*a - 1.0) * r2));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
    vec3 H; H.x = sinTheta * cos(phi); H.y = sinTheta * sin(phi); H.z = cosTheta;
    vec3 Up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 Tangent = normalize(cross(Up, N));
    vec3 Bitangent = cross(N, Tangent);
    return normalize(Tangent * H.x + Bitangent * H.y + N * H.z);
}

// --- PBR Helper Functions for Direct Lighting ---
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / max(denom, 0.0000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// 評估 BRDF (計算太陽光對此表面的貢獻)
vec3 EvalPBR(vec3 N, vec3 V, vec3 L, vec3 F0, vec3 albedo, float roughness, float metallic) {
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = F_Schlick(max(dot(H, V), 0.0), F0);
       
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * NdotV * NdotL + 0.0001; // +0.0001 防止除以零
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;   

    return (kD * albedo / PI + specular) * NdotL; 
}

// --- Color Temperature Helper ---

// 將開爾文 (Kelvin) 轉換為 RGB (線性空間 approximation)
// 算法參考自 Tanner Helland
vec3 KelvinToRGB(float k) {
    vec3 color;
    float temp = k / 100.0;

    // --- Red ---
    if (temp <= 66.0) {
        color.r = 255.0;
    } else {
        color.r = 329.698727446 * pow(temp - 60.0, -0.1332047592);
    }

    // --- Green ---
    if (temp <= 66.0) {
        color.g = 99.4708025861 * log(temp) - 161.1195681661;
    } else {
        color.g = 288.1221695283 * pow(temp - 60.0, -0.0755148492);
    }

    // --- Blue ---
    if (temp >= 66.0) {
        color.b = 255.0;
    } else {
        if (temp <= 19.0) {
            color.b = 0.0;
        } else {
            color.b = 138.5177312231 * log(temp - 10.0) - 305.0447927307;
        }
    }

    // 歸一化並轉為 Linear Space (因為演算法產生的是 sRGB 範圍的 0-255)
    vec3 finalColor = clamp(color / 255.0, 0.0, 1.0);
    
    // 如果你的渲染器是 Linear Workflow (通常 Path Tracer 都是)，
    // 這裡建議將 sRGB 轉回 Linear，否則顏色會太淡/太白
    return pow(finalColor, vec3(2.2)); 
}

// 主函數：輸入 0.0 ~ 100.0，輸出對應的顏色
// 0.0   = 1000K  (燭光/深紅)
// 50.0  = 6500K  (標準白光)
// 100.0 = 15000K (藍天)
vec3 GetColorFromTempSlider(float value) {
    // 限制輸入範圍
    float v = clamp(value, 0.0, 100.0);
    
    // 映射策略：
    // 我們不使用線性映射，因為色溫在低數值時變化較劇烈。
    // 這裡使用簡單的線性混合來映射到 1000K - 15000K
    // 你可以根據喜好調整 minK 和 maxK
    float minK = 1000.0;
    float maxK = 15000.0;
    
    // 如果想要 50 剛好對應 6500K (標準白)，我們可以分段映射
    float kelvin;
    if (v < 50.0) {
        // 0~50 對應 1000K ~ 6500K
        kelvin = mix(1000.0, 6500.0, v / 50.0);
    } else {
        // 50~100 對應 6500K ~ 15000K
        kelvin = mix(6500.0, 15000.0, (v - 50.0) / 50.0);
    }

    return KelvinToRGB(kelvin);
}

// --- Triangle intersect ---
void RayTrianglePacked(Ray ray, int i, inout vec4 hitResult) {
    PackedTriangle tri = packedTris[i];
    const vec3 edge1 = tri.e1.xyz;
    const vec3 edge2 = tri.e2.xyz;
    const bool doubleSided = bool(tri.v0.w);

    const vec3 pvec = cross(ray.direction, edge2);
    const float det = dot(edge1, pvec);
    const float DET = doubleSided ? abs(det) : det;
    if (DET < EPSILON) return;

    const float invDet = 1.0 / det;
    const vec3 tvec = ray.origin - tri.v0.xyz;
    const float u = dot(tvec, pvec) * invDet;
    if (u < 0.0 || u > 1.0) return;

    const vec3 qvec = cross(tvec, edge1);
    const float v = dot(ray.direction, qvec) * invDet;
    if (v < 0.0 || u + v > 1.0) return;

    const float t = dot(edge2, qvec) * invDet;
    if (t >= hitResult.x || t < EPSILON) return;
    hitResult = vec4(t, u, v, i);
}

void RayTriangle(Ray ray, int i, inout vec4 hitResult) {
    const ivec4 index = indices[i];
    const vec3 p0 = vertices[index.x].xyz;
    const vec3 p1 = vertices[index.y].xyz;
    const vec3 p2 = vertices[index.z].xyz;
    const bool doubleSided = bool(index.w);

    const vec3 edge1 = p1 - p0;
    const vec3 edge2 = p2 - p0;
    const vec3 pvec = cross(ray.direction, edge2);

    const float det = dot(edge1, pvec);
    const float DET = doubleSided ? abs(det) : det;
    if (DET < EPSILON) return;

    const float invDet = 1.0 / det;
    const vec3 tvec = ray.origin - p0;
    const float u = dot(tvec, pvec) * invDet;
    if (u < 0.0 || u > 1.0) return;

    const vec3 qvec = cross(tvec, edge1);
    const float v = dot(ray.direction, qvec) * invDet;
    if (v < 0.0 || u + v > 1.0) return;

    const float t = dot(edge2, qvec) * invDet;
    if (t >= hitResult.x || t < EPSILON) return;
    hitResult = vec4(t, u, v, i);
}

// --- AABB & BVH ---
float RayBoundingBox_t(Ray ray, BVHNode node) {
    const vec3 t0 = (node.aabbMin - ray.origin) * ray.invDirection;
    const vec3 t1 = (node.aabbMax - ray.origin) * ray.invDirection;
    const vec3 tmin = min(t0, t1);
    const vec3 tmax = max(t0, t1);
    const float entry = max(max(tmin.x, tmin.y), tmin.z);
    const float exit  = min(min(tmax.x, tmax.y), tmax.z);
    const bool hit = exit >= entry && exit > 0.0;
    return hit ? entry : INF;
}

ivec2 GetClosestChildren(Ray ray, BVHNode node, float tMax) {
    const int leftIdx = abs(node.left);
    const int rightIdx = abs(node.right);
    const float tLeft = RayBoundingBox_t(ray, bvhNodes[leftIdx]);
    const float tRight = RayBoundingBox_t(ray, bvhNodes[rightIdx]);

    const ivec2 children = ivec2(tLeft < tMax ? leftIdx : -1, tRight < tMax ? rightIdx : -1);
    return (tLeft < tRight) ? children.yx : children;
}

vec4 RayBVH(Ray ray) {
    vec4 hitResult = vec4(INF, 0.0, 0.0, -1.0);
    // 根節點 AABB 檢查
    if (RayBoundingBox_t(ray, bvhNodes[0]) == INF) return hitResult;

    int idxStack[MAX_STACK_SIZE];
    int stackPtr = 0;
    idxStack[stackPtr++] = 0; 

    while (stackPtr > 0) {
        const int idx = idxStack[--stackPtr];
        BVHNode node = bvhNodes[idx];

        if (node.count >= 0) {
            // 葉節點
            for (int i = node.start; i < node.start + node.count; i++) {
                RayTrianglePacked(ray, i, hitResult);
            }
        } else {
            // 內部節點
            const ivec2 childIndices = GetClosestChildren(ray, node, hitResult.x);
            // 先加入較遠的子節點，後加入較近的子節點
            if (childIndices.x != -1) idxStack[stackPtr++] = childIndices.x;
            if (childIndices.y != -1) idxStack[stackPtr++] = childIndices.y;
        }
    }
    return hitResult;
}

vec4 RayNoBVH(Ray ray) {
    vec4 hitResult = vec4(INF, 0.0, 0.0, -1.0);
    for (int i = 0; i < indices.length(); i++) RayTrianglePacked(ray, i, hitResult);
    return hitResult;
}

bool HitShadow(Ray ray, float maxDist) {
    const int MAX_TRANSPARENT_BOUNCES = 16;
    
    ray.origin += ray.direction * 1e-4;

    for (int i = 0; i < MAX_TRANSPARENT_BOUNCES; i++) {
        vec4 hitData = useBVH ? RayBVH(ray) : RayNoBVH(ray);
        int hitIdx = int(hitData.w);

        // 沒打中任何東西，表示通往太陽的路是通的
        if (hitIdx == -1) return false;

        // 打中了，檢查距離
        if (hitData.x > maxDist) return false;

        // 檢查 Alpha (透明度)
        const int matID = materialIndices[hitIdx];
        Material mat = materials[matID];
        
        // 算出 UV 來查透明度
        const ivec4 index = indices[hitIdx];
        const vec2 uv0 = texCoords[index.x];
        const vec2 uv1 = texCoords[index.y];
        const vec2 uv2 = texCoords[index.z];
        const vec2 hitUV = barycentric(uv0, uv1, uv2, hitData.y, hitData.z);

        float alpha = getBaseColor(mat, hitUV).a;

        // 如果是透明的 (Alpha < 0.5)
        if (alpha < 0.5) {
            // 穿透：把光線起點移到交點後方，繼續檢查
            ray.origin = ray.origin + ray.direction * (hitData.x + 1e-4);
            continue;
        }

        // 如果是不透明的，表示被遮擋了
        return true; 
    }
    return false;
}

// ----------------------------------------------------
// Hit World
// ----------------------------------------------------
bool hit_world(Ray ray, out HitRecord hit) {
    for (int i = 0; i < 16; i++) {
        // 遍歷 BVH 找到最近交點
        const vec4 hitData = useBVH ? RayBVH(ray) : RayNoBVH(ray);
        const int hitIdx = int(hitData.w);

        // 無交點
        if (hitIdx == -1) return false;

        const ivec4 index = indices[hitIdx];
        const float t = hitData.x, u = hitData.y, v = hitData.z;
        hit.t = t;
        hit.point = ray.origin + t * ray.direction;

        // 材質 顏色
        const int matID = materialIndices[hitIdx];
        hit.materialID = matID;
        Material mat = materials[matID];
        
        // 顏色插值
        const vec2 uv0 = texCoords[index.x];
        const vec2 uv1 = texCoords[index.y];
        const vec2 uv2 = texCoords[index.z];
        const vec2 hitUV = barycentric(uv0, uv1, uv2, u, v);

        hit.uv = hitUV; // [新增] 將 UV 存入 HitRecord
        vec4 baseColor = getBaseColor(mat, hitUV);
        if (baseColor.a < 0.05) {
            // 透明材質，繼續追蹤
            ray.origin = hit.point + ray.direction * 1e-4;
            continue;
        }
        hit.color = baseColor.rgb;

        // 法線
        const vec3 geoNormal = geoNormals[hitIdx].xyz;
        const bool frontFace = dot(ray.direction, geoNormal) < 0.0;
        hit.frontFace = frontFace;
        hit.geoNormal = frontFace ? geoNormal : -geoNormal;

        // 插值法線
        const vec3 n0 = vertexNormals[index.x].xyz;
        const vec3 n1 = vertexNormals[index.y].xyz;
        const vec3 n2 = vertexNormals[index.z].xyz;
        hit.shadingNormal = normalize(barycentric(n0, n1, n2, u, v));
        // hit.shadingNormal = vec3(0.0); // [測試] 關閉插值法線

        // Metallic && Roughness
        vec2 mr = getMetallicRoughness(mat, hitUV);
        hit.metallic = mr.x;
        hit.roughness = max(mr.y, 0.04);
        return true;
    }
    return false;
}

// ----------------------------------------------------
// Trace Ray
// ----------------------------------------------------
vec3 RayTrace(Ray ray, inout uint state, ivec2 pixel) {
    vec3 throughput = vec3(1.0); 
    vec3 finalColor = vec3(0.0);
    const vec3 WHITE = vec3(1.0, 1.0, 1.0);
    const vec2 screenUV = (pixel + 0.5) / u_resolution;
    const vec3 gN = texture(gNormal, screenUV).xyz;
    const vec3 shadingNormal = normalize(gN * 2.0 - 1.0);

    for (int depth = 0; depth < MAX_DEPTH; depth++) {
        if (length(throughput) < 1e-6) break;

        HitRecord hit;
        if (!hit_world(ray, hit))
        {
            // 背景色
            finalColor += throughput * GetColorFromTempSlider(BackgroundColor);
            break;
        }

        // gNormal (When Depth == 0)
        if (depth == 0 && length(gN) > 0.1) hit.shadingNormal = shadingNormal;

        Material material = materials[hit.materialID];
        vec3 V = -ray.direction;
        vec3 N = dot(hit.shadingNormal, hit.geoNormal) > 0.0 ? hit.shadingNormal : -hit.shadingNormal;

        // Emission

        vec3 emission = getEmission(material, hit.uv);  // 可傳入 UV

        // 2. 如果打到正面，就加上發光顏色
        // 不再因為是光源就 Break，而是繼續進行下方的散射計算

        if (length(emission) > 0.0 && hit.frontFace) {
            finalColor += throughput * emission;
        }

        if (material.transmissionFactor <= 0.0) {
            vec3 L = normalize(u_sunDirection); // 指向太陽的向量
            float NdotL = dot(N, L);

            // 只有當表面面向太陽時才計算
            if (NdotL > 0.0) {
                // 發射陰影射線
                Ray shadowRay = createRay(hit.point, L);
                // 距離設為無限大 (INF) 因為太陽是平行光
                bool blocked = HitShadow(shadowRay, INF);

                if (!blocked) {
                    vec3 F0 = mix(vec3(0.04), hit.color, hit.metallic);
                    
                    // 計算 PBR 光照貢獻
                    vec3 directLight = EvalPBR(N, -ray.direction, L, F0, hit.color, hit.roughness, hit.metallic);
                    
                    // 累加到最終顏色
                    finalColor += throughput * directLight * u_sunColor;
                }
            }
        }

        // 3. 能量守恆與 Russian Roulette
        if (depth >= 2) {
            float p = max(throughput.r, max(throughput.g, throughput.b));
            if (p < 0.0001) p = 0.0001; 
            if (RandomValue(state) > p) break;
            throughput /= p;
        }


        // Material (PBR / Glass)
        if (material.transmissionFactor > 0.0) {
            const float ior = material.ior;
            const float eta = hit.frontFace ? (1.0 / ior) : ior;
            const vec3 refracted = refract(ray.direction, N, eta);

            float fresnel = 1.0;
            float RV = 0.0;

            if (length(refracted) > 1e-6) {
                // 無全反射時計算 Fresnel 機率
                float R0 = (1.0 - ior) / (1.0 + ior);
                float R1 = R0 * R0;
                float cosTheta = min(dot(V, N), 1.0);
                fresnel = R1 + (1.0 - R1) * pow(1.0 - cosTheta, 5);
                RV = RandomValue(state);
            }

            // 全反射或 Fresnel 機率下反射
            // 否則折射
            ray.direction = RV < fresnel ? reflect(ray.direction, N) : refracted;
            throughput *= hit.color * material.transmissionFactor;
        } else { // PBR
            // const float metallic = material.metallicFactor;
            const float metallic = hit.metallic; 

            const vec3 F0 = mix(vec3(0.04), hit.color, metallic);
            
            const float cosTheta = max(dot(N, V), 0.0);
            const vec3 F = F_Schlick(cosTheta, F0);

            float specularProb = max(F.r, max(F.g, F.b));
            specularProb = mix(specularProb, 1.0, metallic);
            specularProb = clamp(specularProb, 0.05, 1.0);

            if (RandomValue(state) < specularProb) {
                // const float roughness = material.roughnessFactor;
                const vec3 H = ImportanceSampleGGX(state, N, hit.roughness);
                const vec3 L = reflect(-V, H);
                ray.direction = L;

                if (dot(N, L) <= 0.0) break;

                throughput *= mix(vec3(1.0), hit.color, metallic);
                throughput /= specularProb; // balance energy

            } else {
                if (metallic >= 1.0) break;
                ray.direction = cosine_weighted_direction(N, state);
                throughput *= hit.color;
                // throughput *= max(dot(N, ray.direction), 0.0);  // lambertian cosine term
                throughput /= (1.0 - specularProb); // balance energy
            }
        }
        ray.invDirection = 1.0 / ray.direction;
        ray.origin = hit.point + ray.direction * 1e-4;
    }
    return finalColor;
}

vec3 RayTraceOnce(Ray ray, ivec2 pixel) {
    HitRecord hit;
    if (!hit_world(ray, hit)) return vec3(0.0);
    return hit.color;
}

// ----------------------------------------------------
// Main function
// ----------------------------------------------------
void RayTraceMain(ivec2 pixel)
{
    vec4 previousData = imageLoad(screenTexture, pixel);
    vec3 oldColor = previousData.rgb;
    float frameCount = previousData.a;
    frameCount = cameraUpdated ? 1.0 : frameCount + 1.0;
    if (!TimeDenoise) frameCount = 1.0;

    uint state = getCurrentState(pixel, frameCount);

    vec2 jitter = TimeDenoise ? RandomDirection2D(state) - 0.5 : vec2(0.0);
    vec3 dir = getRayDir(vec2(pixel) + 0.5 + jitter);

    //Ray ray = createRay(camera.position, dir);

    vec3 currentFrameColor = vec3(0.0);
    for (int i = 0; i < SAMPLES_PER_PIXEL; i++) {
        Ray ray = createRay(camera.position, dir);
        vec3 rayColor = RayTrace(ray, state, pixel);

        float maxRadiance = 10.0;
        float lum = dot(rayColor, vec3(1));
        if (lum > maxRadiance) rayColor *= maxRadiance / lum;

        currentFrameColor += rayColor;
    }
    currentFrameColor /= float(SAMPLES_PER_PIXEL);

    float weight = frameCount > 1000.0 ? 0.0 : 1.0 / frameCount;
    vec3 finalColor = mix(oldColor, currentFrameColor, weight);

    if (isNanOrInf(finalColor)) finalColor = oldColor;

    float maxColor = max(max(finalColor.r, finalColor.g), finalColor.b);
    if (maxColor > 1.0) finalColor /= maxColor;

    imageStore(screenTexture, pixel, vec4(finalColor, frameCount));
}

void RayTraceOnceMain(ivec2 pixel)
{
    vec3 dir = getRayDir(vec2(pixel) + 0.5);
    Ray ray = createRay(camera.position, dir);
    vec3 color = RayTraceOnce(ray, pixel);
    imageStore(screenTexture, pixel, vec4(color, 1.0));
}

// ----------------------------------------------------
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= u_resolution.x || pixel.y >= u_resolution.y) return;

    RayTracing ? RayTraceMain(pixel) : RayTraceOnceMain(pixel);
}