#pragma once

#include <external/glm/glm.hpp>
#include <vector>
#include <cfloat>

namespace XEngine::Bounds {
    class Bounds3 {
    public:
        glm::vec3 min, max;

        Bounds3();
        Bounds3(const glm::vec3& p1, const glm::vec3& p2);
        Bounds3(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);

        Bounds3 Union(const glm::vec3& other);
        Bounds3 Union(const Bounds3& other);
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

    Bounds3 Union(const Bounds3& b, const glm::vec3& p);
    Bounds3 Union(const Bounds3& b1, const Bounds3& b2);
}