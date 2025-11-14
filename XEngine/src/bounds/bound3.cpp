#include "bounds/bound3.h"
#include <cfloat>

namespace XEngine::Bounds {
    Bound3::Bound3()
        : min(glm::vec3(FLT_MAX)), max(glm::vec3(-FLT_MAX)), cacheInit(false) 
    {
    }

    Bound3::Bound3(const glm::vec3& p1, const glm::vec3& p2)
        : cacheInit(false) {
        min = glm::min(p1, p2);
        max = glm::max(p1, p2);
    }

    Bound3::Bound3(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3)
        : cacheInit(false) {
        min = glm::min(glm::min(p1, p2), p3);
        max = glm::max(glm::max(p1, p2), p3);
    }

    Bound3 Bound3::Union(const glm::vec3& other) {
        min = glm::min(min, other);
        max = glm::max(max, other);
        return *this;
    }

    Bound3 Bound3::Union(const Bound3& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
        return *this;
    }

    glm::vec3 Bound3::Diagonal() const {
        updateCache();
        return cachedDiagonal;
    }

    glm::vec3 Bound3::Center() const {
        updateCache();
        return cachedCenter;
    }

    int Bound3::octant(const glm::vec3& point) const {
        glm::vec3 center = Center();
        return (point.x > center.x) |
            ((point.y > center.y) << 1) |
            ((point.z > center.z) << 2);
    }

    int Bound3::MaximumExtent() const {
        glm::vec3 d = Diagonal();
        if (d.x > d.y && d.x > d.z)
            return 0;
        else if (d.y > d.z && d.y)
            return 1;
        else
            return 2;
    }

    float Bound3::Volume() const {
        glm::vec3 d = Diagonal();
        return d.x * d.y * d.z;
    }

    float Bound3::VolumeWithMin(float minValue) const {
        glm::vec3 d = glm::max(Diagonal(), glm::vec3(minValue));
        return d.x * d.y * d.z;
    }

    Bound3 Union(const Bound3& b, const glm::vec3& p) {
        glm::vec3 newMin = glm::min(b.min, p);
        glm::vec3 newMax = glm::max(b.max, p);
        return Bound3(newMin, newMax);
    }

    Bound3 Union(const Bound3& b1, const Bound3& b2) {
        glm::vec3 newMin = glm::min(b1.min, b2.min);
        glm::vec3 newMax = glm::max(b1.max, b2.max);
        return Bound3(newMin, newMax);
    }

    // --- 快取機制 ---
    void Bound3::updateCache() const {
        if (!cacheInit || min != lastMin || max != lastMax) {
            cachedDiagonal = max - min;
            cachedCenter = (min + max) * 0.5f;
            lastMin = min;
            lastMax = max;
            cacheInit = true;
        }
    }
}