#include "accelerators/bvh.h"
#include "bounds/bound3.h"

#include "graphics/structs.hpp"
#include "XEngine/log.h"

#include <functional>
#include <algorithm>
#include <numeric>


namespace XEngine::BVH {
    // 定義分桶數量
    constexpr int SAH_BINS = 16;

    struct SAHBin {
        Bounds::Bound3 bounds;
        int count = 0;
    };

    std::vector<BVHNode> buildBVH(Mesh& mesh, std::vector<PackedTriangle>& packedTris) {
        std::vector<BVHNode> nodes;
        Bounds::Bound3 rootAABB;
        const int totalTriangles = (int)mesh.indices.size();
        // 預先保留記憶體以減少 resize
        nodes.reserve(totalTriangles * 2);

        // 預先計算所有三角形的 AABB，避免遞迴時重複計算
        std::vector<Bounds::Bound3> triangleAABBs(totalTriangles);
        for (size_t i = 0; i < totalTriangles; i++) {
            const glm::ivec4& face = mesh.indices[i];
            const Bounds::Bound3 triangleAABB(
                mesh.vertices[face.x],
                mesh.vertices[face.y],
                mesh.vertices[face.z]
            );
            triangleAABBs[i] = triangleAABB;
            rootAABB.Union(triangleAABB);
        }

        auto swapPrimitives = [&](int indexA, int indexB) {
            if (indexA == indexB) return;
            std::swap(mesh.indices[indexA], mesh.indices[indexB]);
            std::swap(mesh.faceNormals[indexA], mesh.faceNormals[indexB]);
            std::swap(mesh.materialIndices[indexA], mesh.materialIndices[indexB]);
            std::swap(packedTris[indexA], packedTris[indexB]);
            std::swap(triangleAABBs[indexA], triangleAABBs[indexB]);
        };


        std::function<int(int, int, int, Bounds::Bound3)> buildNode =
            [&](int start, int end, int depth, Bounds::Bound3 nodeAABB) -> int {

                const int count = end - start;
                const int currentIndex = (int)nodes.size();
                nodes.emplace_back();
                auto& node = nodes.back();

                // 更新節點資訊
                node.aabbMin = nodeAABB.min;
                node.aabbMax = nodeAABB.max;

                // 終止條件：三角形數量少或達到最大深度
                if (count <= MAX_LEAF_TRIANGLES || depth <= 0) {
                    node.start = start;
                    node.count = count;
                    return currentIndex;
                }

                // 計算當前節點的重心 AABB (Centroid AABB)
                Bounds::Bound3 centroidAABB;
                for (int i = start; i < end; i++) centroidAABB.Union(triangleAABBs[i].Center());

                // SAH 分割
                int bestAxis = -1;
                float bestCost = std::numeric_limits<float>::max();
                float bestSplitPos = 0.0f;
                Bounds::Bound3 bestAABB[2];

                // 遍歷三個軸 (0:X, 1:Y, 2:Z)
                for (int axis = 0; axis < 3; axis++) {
                    const float boundsMin = centroidAABB.min[axis];
                    const float boundsMax = centroidAABB.max[axis];
                    const float extent = boundsMax - boundsMin;

                    // 如果這個軸幾乎沒有寬度，跳過
                    if (extent < 1e-5f) continue;

                    SAHBin bins[SAH_BINS];
                    const float scale = SAH_BINS / extent;

                    // Pass 1: 將三角形填入桶中
                    for (int i = start; i < end; i++) {
                        auto& triangleAABB = triangleAABBs[i];
                        glm::vec3& center = triangleAABB.Center();
                        int binIdx = std::min(SAH_BINS - 1, (int)((center[axis] - boundsMin) * scale));
                        auto& bin = bins[binIdx];
                        bin.count++;
                        bin.bounds.Union(triangleAABB);
                    }

                    // Pass 2: 評估分割代價
                    // 使用前綴和 (Sweep) 快速計算左右面積
                    int leftCount[SAH_BINS - 1], rightCount[SAH_BINS - 1];
                    Bounds::Bound3 leftAABB[SAH_BINS - 1], rightAABB[SAH_BINS - 1];
                    Bounds::Bound3 leftBox, rightBox;
                    int leftSum = 0, rightSum = 0;

                    for (int i = 0, j = SAH_BINS - 2; i < SAH_BINS - 1; i++, j--) {
                        // 從左掃描
                        auto& leftBin = bins[i];
                        leftSum += leftBin.count;
                        leftAABB[i] = leftBox.Union(leftBin.bounds);
                        leftCount[i] = leftSum;

                        // 從右掃描
                        auto& rightBin = bins[j + 1];
                        rightSum += rightBin.count;
                        rightAABB[j] = rightBox.Union(rightBin.bounds);
                        rightCount[j] = rightSum;
                    }

                    // 尋找此軸上的最小 SAH
                    int temp = 0;
                    for (int i = 0; i < SAH_BINS - 1; i++) {
                        if (leftCount[i] == temp || rightCount[i] == 0) continue;
                        temp = leftCount[i];

                        Bounds::Bound3& leftBox = leftAABB[i];
                        Bounds::Bound3& rightBox = rightAABB[i];
                        float cost = leftCount[i] * leftBox.SurfaceArea() + rightCount[i] * rightBox.SurfaceArea();
                        if (cost < bestCost) {
                            bestCost = cost;
                            bestAxis = axis;
                            bestAABB[0] = leftBox;
                            bestAABB[1] = rightBox;
                            // 分割位置設為該桶的右邊界比例處
                            bestSplitPos = boundsMin + extent * (i + 1) / (float)SAH_BINS;
                        }
                    }
                }

                // 計算不分割的代價 (作為葉子節點)
                const float leafCost = count * nodeAABB.SurfaceArea();

                // 如果無法找到有效分割，或分割代價比直接做葉子還高，則終止
                if (bestAxis == -1 || bestCost >= leafCost) {
                    node.start = start;
                    node.count = count;
                    return currentIndex;
                }

                // In-Place Partitioning
                // 將中心點小於 splitPos 的放到左邊，大於的放到右邊
                int mid = start;
                for (int i = start; i < end; i++) {
                    glm::vec3& center = triangleAABBs[i].Center();
                    if (center[bestAxis] < bestSplitPos) swapPrimitives(i, mid++);
                }

                // 防止極端情況 (例如所有重心都在同一側，導致無限遞迴)
                if (mid == start || mid == end) {
                    // 如果幾何分割失敗，直接設為葉節點
                    node.start = start;
                    node.count = count;
                    return currentIndex;
                }

                // 遞迴建構子節點
                node.left = buildNode(start, mid, depth - 1, bestAABB[0]);
                node.right = buildNode(mid, end, depth - 1, bestAABB[1]);

                return currentIndex;
            };

        // 遞迴
        buildNode(0, totalTriangles, MAX_DEPTH, rootAABB);

        return nodes;
    }


    std::vector<OBVHNode> buildOBVH(Mesh& mesh) {
        std::vector<OBVHNode> nodes;

        std::function<int(int, int, int)> buildNode = [&](int start, int end, int depth) -> int {
            const int count = end - start;
            std::vector<Bounds::Bound3> childAABBs(count);
            OBVHNode node;
            node.top = glm::ivec4(-1);
            node.bottom = glm::ivec4(-1);
            node.info = glm::ivec4(-1);
            Bounds::Bound3 aabb;

            // 計算 AABB
            for (int i = start, j = 0; j < count; i++, j++) {
                const glm::ivec4& face = mesh.indices[i];
                const Bounds::Bound3 TriangleAABB(
                    mesh.vertices[face.x],
                    mesh.vertices[face.y],
                    mesh.vertices[face.z]
                );
                childAABBs[j] = TriangleAABB;
                aabb = aabb.Union(TriangleAABB);
            }
            node.aabbMin = aabb.min;
            node.aabbMax = aabb.max;

            const int currentIndex = (int)nodes.size();
            nodes.push_back(node);

            if (count <= MAX_LEAF_TRIANGLES || depth <= 0) {
                node.info = glm::ivec4(-1, -1, start, count);
                nodes[currentIndex] = node;
                return currentIndex; // 葉節點
            }

            // 預先計算 octant
            std::vector<int> octants(count);
            for (int i = 0; i < count; i++) {
                octants[i] = aabb.octant(childAABBs[i].Center());
            }

            // in-place partition
            int childEnds[8];
            int write = start;
            for (int oct = 0; oct < 8; oct++) {
                for (int i = 0, src = start; i < count; i++, src++) {
                    if (octants[i] != oct) continue;

                    if (src != write) {
                        std::swap(mesh.indices[write], mesh.indices[src]);
                        std::swap(mesh.faceNormals[write], mesh.faceNormals[src]);
                        std::swap(mesh.materialIndices[write], mesh.materialIndices[src]);
                    }
                    write++;
                }
                childEnds[oct] = write;
            }

            // 建立子節點
            int childStart = start;
            for (int i = 0; i < 8; i++) {
                int childEnd = childEnds[i];
                if (childStart < childEnd) {
                    int childIdx = buildNode(childStart, childEnd, depth - 1);
                    if (i < 4) node.top[i] = childIdx;
                    else       node.bottom[i - 4] = childIdx;
                    childStart = childEnd;
                }
            }
            nodes[currentIndex] = node;
            return currentIndex;
        };

        buildNode(0, (int)mesh.indices.size(), MAX_DEPTH);
        return nodes;
    }

    LeafNode getLeafNode(const std::vector<BVHNode>& nodes) {
        LeafNode leafNode;
        leafNode.count = 0;

        std::function<void(int, int)> traverse = [&](int index, int depth) {
            const BVHNode& node = nodes[index];
            if (node.count > 0) {
                // 葉節點
                leafNode.count++;
                leafNode.depths.push_back(depth);
                leafNode.triangleCounts.push_back(node.count);
                return;
            }
            if (node.left != -1) traverse(node.left, depth + 1);
            if (node.right != -1) traverse(node.right, depth + 1);
        };

        traverse(0, 0);

        leafNode.depth = getStatistics(leafNode.depths);
        leafNode.triangleCount = getStatistics(leafNode.triangleCounts);
        return leafNode;
    }

    Statistics getStatistics(const std::vector<int>& nodes) {
        Statistics stats;
        if (nodes.empty()) {
            stats.maxValue = 0;
            stats.minValue = 0;
            stats.modeValue = 0;
            stats.medianValue = 0;
            stats.averageValue = 0.0;
            return stats;
        }

        std::vector<int> sortedNodes = nodes;
        std::sort(sortedNodes.begin(), sortedNodes.end());

        stats.maxValue = sortedNodes.back();
        stats.minValue = sortedNodes.front();

        // 計算眾數
        int mode = sortedNodes[0];
        int maxCount = 1, currentCount = 1;
        for (size_t i = 1; i < sortedNodes.size(); i++) {
            if (sortedNodes[i] == sortedNodes[i - 1]) {
                currentCount++;
                continue;
            }

            if (currentCount > maxCount) {
                maxCount = currentCount;
                mode = sortedNodes[i - 1];
            }

            currentCount = 1;
        }
        if (currentCount > maxCount) {
            mode = sortedNodes.back();
        }
        stats.modeValue = mode;

        // 計算中位數
        size_t midIndex = sortedNodes.size() / 2;
        if (sortedNodes.size() % 2 == 0) {
            stats.medianValue = (sortedNodes[midIndex - 1] + sortedNodes[midIndex]) * 0.5f;
        } else {
            stats.medianValue = static_cast<float>(sortedNodes[midIndex]);
        }

        // 計算平均值
        stats.averageValue = static_cast<float>(std::accumulate(sortedNodes.begin(), sortedNodes.end(), 0.0f)) / sortedNodes.size();
        return stats;
    }

    Mesh mergeMeshes(const std::vector<Mesh>& meshes) {
        Mesh mergedMesh;
        unsigned int vertexOffset = 0;

        for (const auto& mesh : meshes) {
            // 合併頂點
            mergedMesh.vertices.insert(mergedMesh.vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
            mergedMesh.faceNormals.insert(mergedMesh.faceNormals.end(), mesh.faceNormals.begin(), mesh.faceNormals.end());
            glm::ivec4 offset(vertexOffset, vertexOffset, vertexOffset, 0);

            // 合併索引並調整偏移量
            for (const auto& index : mesh.indices) {
                glm::ivec4 adjustedIndex = index + offset;
                mergedMesh.indices.push_back(adjustedIndex);
            }

            // 合併材質索引
            mergedMesh.materialIndices.insert(mergedMesh.materialIndices.end(), mesh.materialIndices.begin(), mesh.materialIndices.end());

            vertexOffset += static_cast<unsigned int>(mesh.vertices.size());
        }

        return mergedMesh;
    }
}