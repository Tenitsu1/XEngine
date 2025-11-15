#ifndef QBVH_H
#define QBVH_H
#include <external/glm/glm.hpp>
#include <vector>


struct Triangle;
struct Mesh;


namespace XEngine::OBVH {
    

    inline constexpr int MAX_LEAF_TRIANGLES = 8;
    inline constexpr int MAX_DEPTH = 32;
    inline constexpr float MIN_AABB_VOLUME = 8.0f;

    struct OBVHNode {
        glm::vec3 aabbMin;
		float	padding1;
        glm::vec3 aabbMax;
		float	padding2;
        glm::ivec4 childrenA; // 0~3號子節點索引，-1表示無
        glm::ivec4 childrenB; // 4~7號子節點索引，-1表示無
        glm::ivec4 info;      // z=start, w=count
    };

    std::vector<OBVHNode> buildOBVH(std::vector<Triangle>& triangles);
    std::vector<OBVHNode> buildOBVH(Mesh& mesh);
}
#endif