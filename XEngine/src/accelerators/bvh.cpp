#include "accelerators/bvh.h"
#include "bounds/bound3.h"

#include "graphics/structs.hpp"
#include "XEngine/log.h"

#include <functional>
#include <algorithm>
#include <numeric>


namespace XEngine::BVH {
    std::vector<BVHNode> buildBVH(Mesh& mesh, std::vector<PackedTriangle>& packedTris) {
        std::vector<BVHNode> nodes;
        std::vector<Bounds::Bound3> triangleAABBs(mesh.indices.size());
        Bounds::Bound3 AABB;

        for (size_t i = 0; i < mesh.indices.size(); ++i) {
            const glm::ivec4& face = mesh.indices[i];
            Bounds::Bound3 triangleAABB = Bounds::Bound3(
                mesh.vertices[face.x],
                mesh.vertices[face.y],
                mesh.vertices[face.z]
            );
            triangleAABBs[i] = triangleAABB;
            AABB = AABB.Union(triangleAABB);
        }

        std::function<int(int, int, int, Bounds::Bound3)> buildNode = [&](int start, int end, int depth, Bounds::Bound3 aabb) -> int {
            const int count = end - start;
            
            BVHNode node;
            node.aabbMin = aabb.min;
            node.aabbMax = aabb.max;

            const int currentIndex = (int)nodes.size();
            nodes.push_back(node);

            if (count <= MAX_LEAF_TRIANGLES || depth <= 0) {
                node.start = start;
                node.count = count;
                nodes[currentIndex] = node;
                return currentIndex;
            }

            // SAH 分割
            int bestAxis = -1, bestSplit = -1;
            Bounds::Bound3 bestLeftAABB, bestRightAABB;
            float bestCost = std::numeric_limits<float>::max();
            std::vector<glm::vec3> centers3(count);
            for (int i = 0; i < count; ++i) {
                const glm::ivec4& face = mesh.indices[start + i];
                glm::vec3 center = (mesh.vertices[face.x] + mesh.vertices[face.y] + mesh.vertices[face.z]) / 3.0f;
                centers3[i] = center;
            }

            std::vector<std::vector<std::pair<float, int>>> centersPerAxis(3);
            for (int axis = 0; axis < 3; ++axis) {
                // 根據三角形中心排序
                std::vector<std::pair<float, int>>& centers = centersPerAxis[axis];
                centers.resize(count);
                for (int i = 0; i < count; ++i) {
                    centers[i] = { centers3[i][axis], i };
                }
                std::sort(centers.begin(), centers.end());
            }

            for (int axis = 0; axis < 3; ++axis) {
                std::vector<std::pair<float, int>> centers = centersPerAxis[axis];

                // 前綴/後綴 AABB
                std::vector<Bounds::Bound3> leftAABBs(count), rightAABBs(count);
                {
                    std::vector<Bounds::Bound3> AABBs(count);
                    for (int i = 0; i < count; ++i) {
                        AABBs[i] = triangleAABBs[start + centers[i].second];
                    }
                    
                    Bounds::Bound3 leftAABB, rightAABB;
                    for (int i = 0, j = count - 1; i < count; ++i, --j) {
                        leftAABB = leftAABB.Union(AABBs[i]);
                        rightAABB = rightAABB.Union(AABBs[j]);
                        leftAABBs[i] = leftAABB;
                        rightAABBs[j] = rightAABB;
                    }
                }

                // 嘗試所有分割點
                for (int i = 1; i < count; ++i) {
                    Bounds::Bound3 leftAABB = leftAABBs[i - 1];
                    Bounds::Bound3 rightAABB = rightAABBs[i];
                    float leftArea = leftAABB.SurfaceArea();
                    float rightArea = rightAABB.SurfaceArea();
                    float cost = leftArea * i + rightArea * (count - i);
                    if (cost < bestCost) {
                        bestCost = cost;
                        bestAxis = axis;
                        bestSplit = i;
                        bestLeftAABB = leftAABB;
                        bestRightAABB = rightAABB;
                    }
                }
            }

            // 若無法有效分割，則為葉節點
            if (bestAxis == -1) {
                node.start = start;
                node.count = count;
                nodes[currentIndex] = node;
                return currentIndex;
            }

            // 依最佳分割重排
            std::vector<std::pair<float, int>> centers = centersPerAxis[bestAxis];

            // in-place 重排
            std::vector<glm::ivec4> newIndices(count);
            std::vector<glm::vec4> newNormals(count);
            std::vector<int> newMaterials(count);
            std::vector<PackedTriangle> newPackedTris(count);
            std::vector<Bounds::Bound3> newAABBs(count);
            for (int i = 0; i < count; ++i) {
                int idx = centers[i].second + start;
                newIndices[i] = mesh.indices[idx];
                newNormals[i] = mesh.faceNormals[idx];
                newMaterials[i] = mesh.materialIndices[idx];
                newPackedTris[i] = packedTris[idx];
                newAABBs[i] = triangleAABBs[idx];
            }
            for (int i = 0; i < count; ++i) {
                mesh.indices[start + i] = newIndices[i];
                mesh.faceNormals[start + i] = newNormals[i];
                mesh.materialIndices[start + i] = newMaterials[i];
                packedTris[start + i] = newPackedTris[i];
                triangleAABBs[start + i] = newAABBs[i];
            }

            // 遞迴建立左右子節點
            int mid = start + bestSplit;
            node.left = buildNode(start, mid, depth - 1, bestLeftAABB);
            node.right = buildNode(mid, end, depth - 1, bestRightAABB);
            nodes[currentIndex] = node;
            return currentIndex;
        };

        buildNode(0, (int)mesh.indices.size(), MAX_DEPTH, AABB);

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