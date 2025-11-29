#include "accelerators/obvh.h"
#include "bounds/bound3.h"
#include <functional>
#include <algorithm>
#include "graphics/structs.hpp"
#include "XEngine/log.h"


namespace XEngine::OBVH {
    std::vector<BVHNode> buildBVH(Mesh& mesh) {
        std::vector<BVHNode> nodes;

        std::function<int(int, int, int)> buildNode = [&](int start, int end, int depth) -> int {
            const int count = end - start;
            BVHNode node;
            Bounds::Bound3 aabb;

            // 計算 AABB
            for (int i = start; i < end; i++) {
                const glm::ivec4& face = mesh.indices[i];
                aabb = aabb.Union(Bounds::Bound3(
                    mesh.vertices[face.x],
                    mesh.vertices[face.y],
                    mesh.vertices[face.z]
                ));
            }
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
            float bestCost = std::numeric_limits<float>::max();
            for (int axis = 0; axis < 3; ++axis) {
                // 根據三角形中心排序
                std::vector<std::pair<float, int>> centers(count);
                for (int i = 0; i < count; ++i) {
                    const glm::ivec4& face = mesh.indices[start + i];
                    glm::vec3 center = (mesh.vertices[face.x] + mesh.vertices[face.y] + mesh.vertices[face.z]) / 3.0f;
                    centers[i] = { center[axis], i };
                }
                std::sort(centers.begin(), centers.end());

                // 前綴/後綴 AABB
                std::vector<Bounds::Bound3> leftAABBs(count), rightAABBs(count);
                Bounds::Bound3 leftAABB, rightAABB;
                for (int i = 0; i < count; ++i) {
                    const glm::ivec4& face = mesh.indices[start + centers[i].second];
                    leftAABB = leftAABB.Union(Bounds::Bound3(
                        mesh.vertices[face.x],
                        mesh.vertices[face.y],
                        mesh.vertices[face.z]
                    ));
                    leftAABBs[i] = leftAABB;
                }
                for (int i = count - 1; i >= 0; --i) {
                    const glm::ivec4& face = mesh.indices[start + centers[i].second];
                    rightAABB = rightAABB.Union(Bounds::Bound3(
                        mesh.vertices[face.x],
                        mesh.vertices[face.y],
                        mesh.vertices[face.z]
                    ));
                    rightAABBs[i] = rightAABB;
                }

                // 嘗試所有分割點
                for (int i = 1; i < count; ++i) {
                    float leftArea = leftAABBs[i - 1].SurfaceArea();
                    float rightArea = rightAABBs[i].SurfaceArea();
                    float cost = leftArea * i + rightArea * (count - i);
                    if (cost < bestCost) {
                        bestCost = cost;
                        bestAxis = axis;
                        bestSplit = i;
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
            std::vector<std::pair<float, int>> centers(count);
            for (int i = 0; i < count; ++i) {
                const glm::ivec4& face = mesh.indices[start + i];
                glm::vec3 center = (mesh.vertices[face.x] + mesh.vertices[face.y] + mesh.vertices[face.z]) / 3.0f;
                centers[i] = { center[bestAxis], i };
            }
            std::sort(centers.begin(), centers.end());

            // in-place 重排
            std::vector<glm::ivec4> newIndices(count);
            std::vector<glm::vec4> newNormals(count);
            std::vector<int> newMaterials(count);
            for (int i = 0; i < count; ++i) {
                int idx = centers[i].second + start;
                newIndices[i] = mesh.indices[idx];
                newNormals[i] = mesh.faceNormals[idx];
                newMaterials[i] = mesh.materialIndices[idx];
            }
            for (int i = 0; i < count; ++i) {
                mesh.indices[start + i] = newIndices[i];
                mesh.faceNormals[start + i] = newNormals[i];
                mesh.materialIndices[start + i] = newMaterials[i];
            }

            // 遞迴建立左右子節點
            int mid = start + bestSplit;
            node.left = buildNode(start, mid, depth - 1);
            node.right = buildNode(mid, end, depth - 1);
            nodes[currentIndex] = node;
            return currentIndex;
        };

        buildNode(0, (int)mesh.indices.size(), MAX_DEPTH);
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