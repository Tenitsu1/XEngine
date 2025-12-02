#version 460 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;


// --- struct ---
struct BVHNode {
    vec4 aabbMin;
    vec4 aabbMax;
    int left;
    int right;
    int start;
    int count;
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
    int type;
};

struct HitRecord {
    vec3 point;
    vec3 color;
    int materialID;
    float t;
    vec3 shadingNormal;
    vec3 geoNormal;
    bool frontFace;
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
layout(binding = 20) uniform sampler2D u_textures[16]; 


uniform vec2 u_resolution;
uniform float u_time;
uniform int SAMPLES_PER_PIXEL;
uniform int MAX_DEPTH;
uniform bool useOBVH; 
uniform Camera camera;
uniform bool cameraUpdated;
uniform mat4 invViewProj;

// --- const ---
const int MAX_STACK_SIZE = 16; 
const float EPSILON = 1e-20;
const float PI = 3.14159265359;
const int LIGHT = 1;

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
    float a = RandomValue(state) * 6.28318530718;
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
    return normalize(n0 * (1.0 - u - v) + n1 * u + n2 * v);
}

vec3 getBaseColor(Material mat, vec2 uv) {
    vec3 color = mat.baseColorFactor.rgb;
    if (mat.baseColorTexture >= 0) {
        color *= texture(u_textures[min(mat.baseColorTexture, 15)], uv).rgb;
    }
    return color;
}

vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// PBR Sampling functions
vec3 cosine_weighted_direction(vec3 normal, inout uint state, vec3 geoNormal) {
    float r1 = RandomValue(state);
    float r2 = RandomValue(state);
    float phi = 2.0 * PI * r1;
    float cosTheta = sqrt(1.0 - r2);
    float sinTheta = sqrt(r2);
    vec3 tangent = normalize(abs(normal.x) > 0.1 ? cross(normal, vec3(0,1,0)) : cross(normal, vec3(1,0,0)));
    vec3 bitangent = cross(normal, tangent);
    vec3 dir = normalize(cosTheta * normal + sinTheta * cos(phi) * tangent + sinTheta * sin(phi) * bitangent);
    return dot(dir, geoNormal) < 0.0 ? -dir : dir;
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
vec3 intersectTriangle(Ray ray, vec3 p0, vec3 p1, vec3 p2) {
    const vec3 edge1 = p1 - p0;
    const vec3 edge2 = p2 - p0;
    const vec3 pvec = cross(ray.direction, edge2);

    const float det = dot(edge1, pvec);
    if (abs(det) < 1e-5) return vec3(-1.0);

    const float invDet = 1.0 / det;
    const vec3 tvec = ray.origin - p0;

    const float u = dot(tvec, pvec) * invDet;

    if (u < 0.0 || u > 1.0) return vec3(-1.0);

    const vec3 qvec = cross(tvec, edge1);
    const float v = dot(ray.direction, qvec) * invDet;

    if (v < 0.0 || u + v > 1.0) return vec3(-1.0);

    const float t = dot(edge2, qvec) * invDet;

    if (t < EPSILON) return vec3(-1.0);

    return vec3(t, u, v);
}

// --- AABB & BVH ---

bool aabb_hit(Ray ray, vec3 minB, vec3 maxB, float tMax) {
    const vec3 t0 = (minB - ray.origin) * ray.invDirection;
    const vec3 t1 = (maxB - ray.origin) * ray.invDirection;
    const vec3 tmin = min(t0, t1);
    const vec3 tmax = max(t0, t1);
    const float entry = max(max(tmin.x, tmin.y), tmin.z);
    const float exit  = min(min(tmax.x, tmax.y), tmax.z);
    return exit >= max(entry, 0.0) && entry < tMax;
}

bool hit_leaf(Ray ray, int i, inout float tMax, out vec3 hitResult) {
    const bool doubleSided = false;
    const ivec4 index = indices[i];
    const vec3 temp = intersectTriangle(ray, vertices[index.x].xyz, vertices[index.y].xyz, vertices[index.z].xyz);
    if (temp.x < tMax && (doubleSided || temp.x > EPSILON)) {
        tMax = temp.x;
        hitResult = temp;
        return true;
    }
    return false;
}

int hit_bvh(Ray ray, out vec3 hitResult) {
    float tMax = 1e9;
    int hitIdx = -1;
    int stack[MAX_STACK_SIZE];
    int stackPtr = 0;
    stack[stackPtr++] = 0; 

    while (stackPtr > 0) {
        const int nodeIdx = stack[--stackPtr];
        BVHNode node = bvhNodes[nodeIdx];

        if (!aabb_hit(ray, node.aabbMin.xyz, node.aabbMax.xyz, tMax)) continue;

        if (node.count > 0) { // Leaf
            for (int i = node.start; i < node.start + node.count; i++) {   
                vec3 temp;
                if (hit_leaf(ray, i, tMax, temp)) {
                    hitIdx = i;
                    hitResult = temp;
                }
            }
        } else {
            if (node.left >= 0) stack[stackPtr++] = node.left;
            if (node.right >= 0) stack[stackPtr++] = node.right;
        }
    }
    return hitIdx;
}

// --- hit function ---

int hit_triangle(Ray ray, out vec3 hitResult) {
    float tMax = 1e9;
    int hitIdx = -1;
    for (int i = 0; i < indices.length(); i++) {
        const ivec4 idx = indices[i];
        const vec3 temp = intersectTriangle(ray, vertices[idx.x].xyz, vertices[idx.y].xyz, vertices[idx.z].xyz);
        if (temp.x < tMax && temp.x > EPSILON) {
            tMax = temp.x;
            hitIdx = i;
            hitResult = temp;
        }
    }
    return hitIdx;
}

bool hit_world(Ray ray, out HitRecord hit) {
    vec3 hitResult;
    const int hitIdx = useOBVH ? hit_bvh(ray, hitResult) : hit_triangle(ray, hitResult);
    if (hitIdx == -1) return false;

    const ivec4 index = indices[hitIdx];
    
    hit.t = hitResult.x;
    hit.point = ray.origin + hit.t * ray.direction;
    
    const vec3 geoNormal = geoNormals[hitIdx].xyz;
    hit.frontFace = dot(ray.direction, geoNormal) < 0.0;
    hit.geoNormal = hit.frontFace ? geoNormal : -geoNormal;

    const vec3 n0 = vertexNormals[index.x].xyz;
    const vec3 n1 = vertexNormals[index.y].xyz;
    const vec3 n2 = vertexNormals[index.z].xyz;
    const vec3 normal = barycentric(n0, n1, n2, hitResult.y, hitResult.z);
    
    hit.shadingNormal = (dot(hit.geoNormal, normal) > 0.0) ? normal : -normal;
    
    hit.materialID = materialIndices[hitIdx];
    Material mat = materials[hit.materialID];
    
    const vec2 uv0 = texCoords[index.x];
    const vec2 uv1 = texCoords[index.y];
    const vec2 uv2 = texCoords[index.z];
    const vec2 hitUV = uv0 * (1.0 - hitResult.y - hitResult.z) + uv1 * hitResult.y + uv2 * hitResult.z;
    
    hit.color = getBaseColor(mat, hitUV);
    return true;
}

// ----------------------------------------------------
// Trace Ray
// ----------------------------------------------------
vec3 trace_ray(Ray ray, inout uint state, ivec2 pixel) {
    vec3 throughput = vec3(1.0); 
    vec3 finalColor = vec3(0.0);

    for (int depth = 0; depth < MAX_DEPTH; depth++) {
        if (dot(throughput, throughput) < 1e-6) break;

        HitRecord hit;
        if (!hit_world(ray, hit)) {
            // finalColor += throughput * vec3(0.05); // Skybox color
            break;
        }

        // gNormal (When Depth == 0)
        if (depth == 0) {
            vec2 screenUV = (vec2(pixel) + 0.5) / u_resolution;
            vec3 gN = texture(gNormal, screenUV).xyz;
            if (length(gN) > 0.1) {
                hit.shadingNormal = normalize(gN * 2.0 - 1.0);
            }
        }

        Material material = materials[hit.materialID];
        vec3 V = -ray.direction;
        vec3 N = hit.shadingNormal;

        // Emission
        vec3 emission = material.emissionFactor.rgb * material.emissionFactor.a;
        if (length(emission) > 0.0) {
            finalColor += int(hit.frontFace) * throughput * emission;

            if (material.type == LIGHT) break;
        }

        // If depth > 1 then attenuate throughput
        if (depth >= 2) {
            float p = max(throughput.r, max(throughput.g, throughput.b));
            if (RandomValue(state) > p) break;
            throughput *= 1.0 / p;
        }

        vec3 bias = hit.geoNormal * 1e-4;
        ray.origin = hit.point + bias;

        // Material (PBR / Glass)
        if (material.transmissionFactor > 0.0) {
            float ior = material.ior;
            float eta = hit.frontFace ? (1.0 / ior) : ior;
            vec3 refracted = refract(ray.direction, N, eta);

            ray.direction = reflect(ray.direction, N);

            if (length(refracted) != 0.0) {
                float R0 = (1.0 - ior) / (1.0 + ior);
                float R1 = R0 * R0;
                float cosTheta = min(dot(V, N), 1.0);
                float fresnel = R1 + (1.0 - R1) * pow(1.0 - cosTheta, 5);

                if (RandomValue(state) >= fresnel) {
                    ray.direction = refracted;
                    ray.origin = hit.point - bias;
                }
            }
            throughput *= hit.color;
        } else { // PBR
            float roughness = material.roughnessFactor;
            float metallic = material.metallicFactor;
            vec3 F0 = mix(vec3(0.04), hit.color, metallic);
            
            float cosTheta = max(dot(N, V), 0.0);
            vec3 F = F_Schlick(cosTheta, F0);
            float specularProb = max(F.r, max(F.g, F.b));
            specularProb = mix(specularProb, 1.0, metallic);
            specularProb = clamp(specularProb, 0.05, 1.0);

            if (RandomValue(state) < specularProb) {
                vec3 H = ImportanceSampleGGX(state, N, roughness);
                vec3 L = reflect(-V, H);
                ray.direction = L;

                if (dot(N, L) <= 0.0) break;

                vec3 specColor = mix(vec3(1.0), hit.color, metallic);
                throughput *= specColor;
                throughput /= specularProb;

            } else {
                if (metallic >= 1.0) break; 
                ray.direction = cosine_weighted_direction(N, state, hit.geoNormal);
                throughput *= hit.color;
                throughput *= max(dot(hit.shadingNormal, ray.direction), 0.0);
                throughput /= (1.0 - specularProb);
            }
        }
        ray.invDirection = 1.0 / ray.direction;
    }
    return min(finalColor, vec3(10.0));
}

// ----------------------------------------------------
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(u_resolution.x) || pixel.y >= int(u_resolution.y)) return;

    vec4 previousData = imageLoad(screenTexture, pixel);
    vec3 oldColor = previousData.rgb;
    float frameCount = previousData.a;

    frameCount = cameraUpdated ? 1.0 : frameCount + 1.0;

    uint state = getCurrentState(pixel, frameCount);

    vec2 jitter = RandomDirection2D(state) - 0.5;
    vec3 dir = getRayDir(vec2(pixel) + 0.5 + jitter);
    
    //Ray ray = createRay(camera.position, dir);

    vec3 currentFrameColor = vec3(0.0);
    for (int i = 0; i < SAMPLES_PER_PIXEL; i++) {
        Ray ray = createRay(camera.position, dir);
        currentFrameColor += trace_ray(ray, state, pixel);
    }
    currentFrameColor /= float(SAMPLES_PER_PIXEL);

    float weight = 1.0 / frameCount;
    vec3 finalColor = mix(oldColor, currentFrameColor, weight);
    
    if (any(isnan(finalColor)) || any(isinf(finalColor))) finalColor = oldColor;

    imageStore(screenTexture, pixel, vec4(finalColor, frameCount));
}