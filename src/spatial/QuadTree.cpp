#include "spatial/QuadTree.h"

namespace JJM {
namespace Spatial {

bool AABB::contains(const Math::Vector2D& point) const {
    return point.x >= min.x && point.x <= max.x &&
           point.y >= min.y && point.y <= max.y;
}

bool AABB::intersects(const AABB& other) const {
    return !(max.x < other.min.x || min.x > other.max.x ||
             max.y < other.min.y || min.y > other.max.y);
}

Math::Vector2D AABB::getCenter() const {
    return Math::Vector2D((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
}

Math::Vector2D AABB::getSize() const {
    return Math::Vector2D(max.x - min.x, max.y - min.y);
}

} // namespace Spatial
} // namespace JJM
