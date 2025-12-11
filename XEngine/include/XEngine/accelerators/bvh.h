#ifndef QBVH_H
#define QBVH_H
#include <external/glm/glm.hpp>
#include <vector>


struct Triangle;
struct Mesh;
struct PackedTriangle;

namespace XEngine::BVH {
    

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
        int left;   // 負值表示左子節點索引，正值(或零)表示葉子節點 start
        glm::vec3 aabbMax;
        int right;  // 負值表示右子節點索引，正值表示葉子節點 count
    };

    struct Statistics {
        int maxValue;
        int minValue;
        int modeValue;
        float medianValue;
        float averageValue;
    };

    struct LeafNode {
        int count;
        std::vector<int> depths;
        std::vector<int> triangleCounts;
        Statistics depth;
        Statistics triangleCount;
    };

    std::vector<BVHNode> buildBVH(Mesh& mesh, std::vector<PackedTriangle>& packedTris);
    std::vector<OBVHNode> buildOBVH(Mesh& mesh);
    LeafNode getLeafNode(const std::vector<BVHNode>& nodes);
    Statistics getStatistics(const std::vector<int>& nodes);
    Mesh mergeMeshes(const std::vector<Mesh>& meshes);
}
#endif