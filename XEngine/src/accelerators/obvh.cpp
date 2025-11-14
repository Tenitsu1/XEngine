#include "accelerators/obvh.h"
#include "bounds/bound3.h"
#include <functional>
#include <algorithm>
#include "graphics/structs.h"



namespace XEngine::OBVH {
    std::vector<OBVHNode> buildOBVH(std::vector<graphics::Triangle>& triangles) {
        std::vector<OBVHNode> nodes;

        std::function<int(int, int, int)> buildNode = [&](int start, int end, int depth) -> int {
            int count = end - start;
            OBVHNode node;
            node.childrenA = glm::ivec4(-1);
            node.childrenB = glm::ivec4(-1);
            node.info = glm::ivec4(-1, -1, start, count);
            Bounds::Bound3 aabb;

            // 計算 AABB
            for (int i = start; i < end; i++) {
                graphics::Triangle triangle = triangles[i];
                Bounds::Bound3 TriangleAABB(
					triangle.v1,
					triangle.v2,
					triangle.v3
                );
                aabb = Bounds::Union(aabb, TriangleAABB);
            }
            node.aabbMin = aabb.min;
            node.aabbMax = aabb.max;

            float aabbVolume = aabb.VolumeWithMin(1.0f);

            int currentIndex = (int)nodes.size();
            nodes.push_back(node);

            if (count <= MAX_LEAF_TRIANGLES || depth >= MAX_DEPTH || aabbVolume < MIN_AABB_VOLUME)
                return currentIndex; // 葉節點

            // 分配到8個象限
            std::vector<std::vector<int>> childLists(8);
            for (int i = start; i < end; i++) {
                graphics::Triangle triangle = triangles[i];
                int oct = aabb.octant((triangle.v1 + triangle.v2 + triangle.v3) / 3.0f);
                childLists[oct].push_back(i);
            }

            int childEnds[8];
            {
                std::vector<graphics::Triangle> tmp(triangles.begin() + start, triangles.begin() + end);
                int write = start;
                for (int i = 0; i < 8; i++) {
                    for (int origIdx : childLists[i]) {
                        triangles[write++] = std::move(tmp[origIdx - start]);
                    }
                    childEnds[i] = write;
                }
            }

            {   // 建立子節點
                int childStart = start;
                for (int i = 0; i < 8; i++) {
                    int childEnd = childEnds[i];
                    if (childStart < childEnd) {
                        int childIdx = buildNode(childStart, childEnd, depth + 1);
                        if (i < 4) node.childrenA[i] = childIdx;
                        else       node.childrenB[i - 4] = childIdx;
                        childStart = childEnd;
                    }
                }
            }
            nodes[currentIndex] = node;
            return currentIndex;
            };

        buildNode(0, (int)triangles.size(), 0);
        return nodes;
    }
}