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

vec3 getBaseColor(Material mat, vec2 uv) {
    vec3 color = mat.baseColorFactor.rgb;
    if (mat.baseColorTexture >= 0) {
        float layer = float(mat.baseColorTexture); 
        // [修改] 使用 textureLod 強制讀取 Level 0
        color = textureLod(u_textures, vec3(uv, layer), 0.0).rgb;

        color *= sRGBToLinear(color);
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
            if (childIndices.x != -1) idxStack[stackPtr++] = childIndices.x;
            if (childIndices.y != -1) idxStack[stackPtr++] = childIndices.y;
        }
    }
    return hitResult;
}

// --- hit function ---
vec4 RayNoBVH(Ray ray) {
    vec4 hitResult = vec4(INF, 0.0, 0.0, -1.0);
    for (int i = 0; i < indices.length(); i++) RayTrianglePacked(ray, i, hitResult);
    return hitResult;
}

bool hit_world(Ray ray, out HitRecord hit) {
    // 遍歷 BVH 找到最近交點
    vec4 hitData = useBVH ? RayBVH(ray) : RayNoBVH(ray);
    const int hitIdx = int(hitData.w);

    // 無交點
    if (hitIdx == -1) return false;

    const ivec4 index = indices[hitIdx];
    const float t = hitData.x, u = hitData.y, v = hitData.z;
    hit.t = t;
    hit.point = ray.origin + t * ray.direction;
    
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
    hit.color = getBaseColor(mat, hitUV);

    // Metallic && Roughness
    vec2 mr = getMetallicRoughness(mat, hitUV);
    hit.metallic = mr.x;
    hit.roughness = max(mr.y, 0.04); 

    return true;
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
           //          // // 背景色
           //  float a = 0.5 * (ray.direction.y + 1.0);
           //  vec3 sky = mix(WHITE, vec3(0.5, 0.7, 1.0), a);
           //  finalColor += throughput * sky;
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



// ----------------------------------------------------
// 停止根據時間降噪
bool stopDenoise = true;
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= u_resolution.x || pixel.y >= u_resolution.y) return;

    vec4 previousData = imageLoad(screenTexture, pixel);
    vec3 oldColor = previousData.rgb;
    float frameCount = previousData.a;
    if (stopDenoise) frameCount = 0.0;
    frameCount = cameraUpdated ? 1.0 : frameCount + 1.0;

    if (frameCount > 1000.0) {
        imageStore(screenTexture, pixel, vec4(oldColor, frameCount));
        return;
    }

    uint state = getCurrentState(pixel, frameCount);

    vec2 jitter = stopDenoise ? vec2(0.0) : RandomDirection2D(state) - 0.5;
    vec3 dir = getRayDir(vec2(pixel) + 0.5 + jitter);

    //Ray ray = createRay(camera.position, dir);

    vec3 currentFrameColor = vec3(0.0);
    for (int i = 0; i < SAMPLES_PER_PIXEL; i++) {
        Ray ray = createRay(camera.position, dir);
        vec3 rayColor = RayTrace(ray, state, pixel);

        float maxRadiance = 50.0; 
        float lum = dot(rayColor, vec3(1));
        if (lum > maxRadiance) rayColor *= maxRadiance / lum;

        currentFrameColor += rayColor;
    }
    currentFrameColor /= float(SAMPLES_PER_PIXEL);

    float weight = 1.0 / frameCount;
    vec3 finalColor = mix(oldColor, currentFrameColor, weight);

    if (isNanOrInf(finalColor)) finalColor = oldColor;

    imageStore(screenTexture, pixel, vec4(finalColor, frameCount));
}