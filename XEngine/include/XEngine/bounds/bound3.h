#pragma once

#include <external/glm/glm.hpp>
#include <vector>
#include <cfloat>

namespace XEngine::Bounds {
    class Bound3 {
    public:
        glm::vec3 min, max;

        Bound3();
        Bound3(const glm::vec3& p1, const glm::vec3& p2);
        Bound3(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);

        Bound3 Union(const glm::vec3& other);
        Bound3 Union(const Bound3& other);
        Bound3 childOctant(int octant) const;
        glm::vec3 Diagonal() const;
        glm::vec3 Center() const;
        int octant(const glm::vec3& point) const;
        int MaximumExtent() const;
        float Volume() const;
        float VolumeWithMin(float minValue) const;
    private:
        // --- 快取欄位 ---
        mutable glm::vec3 cachedCenter{};
        mutable glm::vec3 cachedDiagonal{};
        mutable glm::vec3 lastMin{};
        mutable glm::vec3 lastMax{};
        mutable bool cacheInit{ false };

        void updateCache() const;
    };

    Bound3 Union(const Bound3& b, const glm::vec3& p);
    Bound3 Union(const Bound3& b1, const Bound3& b2);
    bool Intersect(const Bound3& b1, const Bound3& b2);
}