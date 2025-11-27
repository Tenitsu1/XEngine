#ifndef QBVH_H
#define QBVH_H
#include <external/glm/glm.hpp>
#include <vector>


struct Triangle;
struct Mesh;


namespace XEngine::OBVH {
    

    inline constexpr int MAX_LEAF_TRIANGLES = 2;
    inline constexpr int MAX_DEPTH = 32;
    inline constexpr float MIN_AABB_VOLUME = 1e-4f;

    struct OBVHNode {
        glm::vec3 aabbMin;
		float	padding1;
        glm::vec3 aabbMax;
		float	padding2;
        glm::ivec4 top; // 0~3號子節點索引，-1表示無
        glm::ivec4 bottom; // 4~7號子節點索引，-1表示無
        glm::ivec4 info;      // z=start, w=count
    };

    struct BVHNode {
        glm::vec3 aabbMin;
        glm::vec3 aabbMax;
        int left;   // 左子節點索引，-1表示無
        int right;  // 右子節點索引，-1表示無
        int start;  // 三角形起始索引（葉節點）
        int count;  // 三角形數量（葉節點）
    };

    std::vector<OBVHNode> buildOBVH(Mesh& mesh);
    Mesh mergeMeshes(const std::vector<Mesh>& meshes);
}
#endif