#include "physics/CollisionShapes.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Physics {

// CollisionShape implementation
CollisionShape::CollisionShape(Type type)
    : type(type), density(1.0f), friction(0.3f), restitution(0.0f) {}

CollisionShape::~CollisionShape() {}

CollisionShape::Type CollisionShape::getType() const { return type; }

void CollisionShape::setDensity(float d) { density = d; }
void CollisionShape::setFriction(float f) { friction = std::clamp(f, 0.0f, 1.0f); }
void CollisionShape::setRestitution(float r) { restitution = std::clamp(r, 0.0f, 1.0f); }

float CollisionShape::getDensity() const { return density; }
float CollisionShape::getFriction() const { return friction; }
float CollisionShape::getRestitution() const { return restitution; }

// CircleShape implementation
CircleShape::CircleShape(float radius)
    : CollisionShape(Type::Circle), radius(radius), center(0, 0) {}

CircleShape::~CircleShape() {}

void CircleShape::setRadius(float r) { radius = std::max(0.0f, r); }
float CircleShape::getRadius() const { return radius; }

void CircleShape::setCenter(float x, float y) { center.x = x; center.y = y; }
Math::Vector2D CircleShape::getCenter() const { return center; }

float CircleShape::calculateArea() const {
    return M_PI * radius * radius;
}

float CircleShape::calculateInertia(float mass) const {
    return 0.5f * mass * radius * radius;
}

void CircleShape::computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                              float posX, float posY, float rotation) const {
    (void)rotation;
    float cx = posX + center.x;
    float cy = posY + center.y;
    minX = cx - radius;
    minY = cy - radius;
    maxX = cx + radius;
    maxY = cy + radius;
}

// BoxShape implementation
BoxShape::BoxShape(float width, float height)
    : CollisionShape(Type::Box), width(width), height(height), center(0, 0) {}

BoxShape::~BoxShape() {}

void BoxShape::setSize(float w, float h) {
    width = std::max(0.0f, w);
    height = std::max(0.0f, h);
}

void BoxShape::setCenter(float x, float y) { center.x = x; center.y = y; }

float BoxShape::getWidth() const { return width; }
float BoxShape::getHeight() const { return height; }
Math::Vector2D BoxShape::getCenter() const { return center; }

float BoxShape::calculateArea() const {
    return width * height;
}

float BoxShape::calculateInertia(float mass) const {
    return mass * (width * width + height * height) / 12.0f;
}

void BoxShape::computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                          float posX, float posY, float rotation) const {
    float hw = width * 0.5f;
    float hh = height * 0.5f;
    float c = std::cos(rotation);
    float s = std::sin(rotation);
    
    float corners[4][2] = {
        {-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}
    };
    
    minX = maxX = posX + center.x + corners[0][0] * c - corners[0][1] * s;
    minY = maxY = posY + center.y + corners[0][0] * s + corners[0][1] * c;
    
    for (int i = 1; i < 4; ++i) {
        float x = posX + center.x + corners[i][0] * c - corners[i][1] * s;
        float y = posY + center.y + corners[i][0] * s + corners[i][1] * c;
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
    }
}

// PolygonShape implementation
PolygonShape::PolygonShape() : CollisionShape(Type::Polygon), centroid(0, 0) {}

PolygonShape::~PolygonShape() {}

void PolygonShape::setVertices(const std::vector<Math::Vector2D>& v) {
    vertices = v;
    if (validateVertices()) {
        computeCentroid();
    }
}

void PolygonShape::addVertex(const Math::Vector2D& vertex) {
    vertices.push_back(vertex);
}

void PolygonShape::clearVertices() {
    vertices.clear();
    centroid = Math::Vector2D(0, 0);
}

const std::vector<Math::Vector2D>& PolygonShape::getVertices() const {
    return vertices;
}

size_t PolygonShape::getVertexCount() const {
    return vertices.size();
}

void PolygonShape::setAsBox(float width, float height) {
    vertices.clear();
    float hw = width * 0.5f;
    float hh = height * 0.5f;
    vertices.push_back(Math::Vector2D(-hw, -hh));
    vertices.push_back(Math::Vector2D(hw, -hh));
    vertices.push_back(Math::Vector2D(hw, hh));
    vertices.push_back(Math::Vector2D(-hw, hh));
    computeCentroid();
}

void PolygonShape::setAsBox(float width, float height, const Math::Vector2D& center, float angle) {
    setAsBox(width, height);
    float c = std::cos(angle);
    float s = std::sin(angle);
    for (auto& v : vertices) {
        float x = v.x * c - v.y * s + center.x;
        float y = v.x * s + v.y * c + center.y;
        v.x = x;
        v.y = y;
    }
    computeCentroid();
}

float PolygonShape::calculateArea() const {
    if (vertices.size() < 3) return 0.0f;
    
    float area = 0.0f;
    for (size_t i = 0; i < vertices.size(); ++i) {
        size_t j = (i + 1) % vertices.size();
        area += vertices[i].x * vertices[j].y - vertices[j].x * vertices[i].y;
    }
    return std::abs(area * 0.5f);
}

float PolygonShape::calculateInertia(float mass) const {
    if (vertices.size() < 3) return 0.0f;
    
    float inertia = 0.0f;
    for (size_t i = 0; i < vertices.size(); ++i) {
        size_t j = (i + 1) % vertices.size();
        float cross = std::abs(vertices[i].x * vertices[j].y - vertices[j].x * vertices[i].y);
        float term = (vertices[i].x * vertices[i].x + vertices[j].x * vertices[i].x +
                     vertices[j].x * vertices[j].x + vertices[i].y * vertices[i].y +
                     vertices[j].y * vertices[i].y + vertices[j].y * vertices[j].y);
        inertia += cross * term;
    }
    return mass * inertia / 36.0f;
}

void PolygonShape::computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                               float posX, float posY, float rotation) const {
    if (vertices.empty()) {
        minX = minY = maxX = maxY = 0;
        return;
    }
    
    float c = std::cos(rotation);
    float s = std::sin(rotation);
    
    float x = posX + vertices[0].x * c - vertices[0].y * s;
    float y = posY + vertices[0].x * s + vertices[0].y * c;
    minX = maxX = x;
    minY = maxY = y;
    
    for (size_t i = 1; i < vertices.size(); ++i) {
        x = posX + vertices[i].x * c - vertices[i].y * s;
        y = posY + vertices[i].x * s + vertices[i].y * c;
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
    }
}

void PolygonShape::computeCentroid() {
    if (vertices.size() < 3) {
        centroid = Math::Vector2D(0, 0);
        return;
    }
    
    float cx = 0.0f, cy = 0.0f;
    float area = 0.0f;
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        size_t j = (i + 1) % vertices.size();
        float cross = vertices[i].x * vertices[j].y - vertices[j].x * vertices[i].y;
        area += cross;
        cx += (vertices[i].x + vertices[j].x) * cross;
        cy += (vertices[i].y + vertices[j].y) * cross;
    }
    
    area *= 0.5f;
    if (std::abs(area) > 1e-6f) {
        cx /= (6.0f * area);
        cy /= (6.0f * area);
    }
    
    centroid = Math::Vector2D(cx, cy);
}

bool PolygonShape::validateVertices() const {
    return vertices.size() >= 3;
}

// CapsuleShape implementation
CapsuleShape::CapsuleShape(float radius, float height)
    : CollisionShape(Type::Capsule), radius(radius), height(height) {}

CapsuleShape::~CapsuleShape() {}

void CapsuleShape::setRadius(float r) { radius = std::max(0.0f, r); }
void CapsuleShape::setHeight(float h) { height = std::max(0.0f, h); }

float CapsuleShape::getRadius() const { return radius; }
float CapsuleShape::getHeight() const { return height; }

float CapsuleShape::calculateArea() const {
    return M_PI * radius * radius + 2.0f * radius * height;
}

float CapsuleShape::calculateInertia(float mass) const {
    float cylinderInertia = mass * (height * height / 12.0f + radius * radius / 4.0f);
    float hemisphereInertia = 0.4f * mass * radius * radius;
    return cylinderInertia + hemisphereInertia;
}

void CapsuleShape::computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                               float posX, float posY, float rotation) const {
    float c = std::cos(rotation);
    float s = std::sin(rotation);
    float hh = height * 0.5f;
    
    float x1 = posX - hh * s;
    float y1 = posY + hh * c;
    float x2 = posX + hh * s;
    float y2 = posY - hh * c;
    
    minX = std::min(x1, x2) - radius;
    minY = std::min(y1, y2) - radius;
    maxX = std::max(x1, x2) + radius;
    maxY = std::max(y1, y2) + radius;
}

// EdgeShape implementation
EdgeShape::EdgeShape() : CollisionShape(Type::Edge), hasGhosts(false) {}

EdgeShape::~EdgeShape() {}

void EdgeShape::setVertices(const Math::Vector2D& v1, const Math::Vector2D& v2) {
    vertex1 = v1;
    vertex2 = v2;
}

void EdgeShape::setGhostVertices(const Math::Vector2D& v0, const Math::Vector2D& v3) {
    ghostVertex0 = v0;
    ghostVertex3 = v3;
    hasGhosts = true;
}

Math::Vector2D EdgeShape::getVertex1() const { return vertex1; }
Math::Vector2D EdgeShape::getVertex2() const { return vertex2; }

bool EdgeShape::hasGhostVertices() const { return hasGhosts; }

float EdgeShape::calculateArea() const { return 0.0f; }

float EdgeShape::calculateInertia(float mass) const {
    (void)mass;
    return 0.0f;
}

void EdgeShape::computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                           float posX, float posY, float rotation) const {
    float c = std::cos(rotation);
    float s = std::sin(rotation);
    
    float x1 = posX + vertex1.x * c - vertex1.y * s;
    float y1 = posY + vertex1.x * s + vertex1.y * c;
    float x2 = posX + vertex2.x * c - vertex2.y * s;
    float y2 = posY + vertex2.x * s + vertex2.y * c;
    
    minX = std::min(x1, x2);
    minY = std::min(y1, y2);
    maxX = std::max(x1, x2);
    maxY = std::max(y1, y2);
}

// ChainShape implementation
ChainShape::ChainShape() : CollisionShape(Type::Chain), loop(false) {}

ChainShape::~ChainShape() {}

void ChainShape::createChain(const std::vector<Math::Vector2D>& v) {
    vertices = v;
    loop = false;
}

void ChainShape::createLoop(const std::vector<Math::Vector2D>& v) {
    vertices = v;
    loop = true;
}

void ChainShape::clearVertices() {
    vertices.clear();
    loop = false;
}

const std::vector<Math::Vector2D>& ChainShape::getVertices() const {
    return vertices;
}

size_t ChainShape::getVertexCount() const {
    return vertices.size();
}

bool ChainShape::isLoop() const {
    return loop;
}

float ChainShape::calculateArea() const { return 0.0f; }

float ChainShape::calculateInertia(float mass) const {
    (void)mass;
    return 0.0f;
}

void ChainShape::computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                            float posX, float posY, float rotation) const {
    if (vertices.empty()) {
        minX = minY = maxX = maxY = 0;
        return;
    }
    
    float c = std::cos(rotation);
    float s = std::sin(rotation);
    
    float x = posX + vertices[0].x * c - vertices[0].y * s;
    float y = posY + vertices[0].x * s + vertices[0].y * c;
    minX = maxX = x;
    minY = maxY = y;
    
    for (size_t i = 1; i < vertices.size(); ++i) {
        x = posX + vertices[i].x * c - vertices[i].y * s;
        y = posY + vertices[i].x * s + vertices[i].y * c;
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
    }
}

// CompoundShape implementation
CompoundShape::CompoundShape() : CollisionShape(Type::Box) {}

CompoundShape::~CompoundShape() {}

void CompoundShape::addShape(std::unique_ptr<CollisionShape> shape,
                            const Math::Vector2D& offset, float rotation) {
    ShapeData data;
    data.shape = std::move(shape);
    data.offset = offset;
    data.rotation = rotation;
    shapes.push_back(std::move(data));
}

void CompoundShape::removeShape(size_t index) {
    if (index < shapes.size()) {
        shapes.erase(shapes.begin() + index);
    }
}

void CompoundShape::clearShapes() {
    shapes.clear();
}

size_t CompoundShape::getShapeCount() const {
    return shapes.size();
}

CollisionShape* CompoundShape::getShape(size_t index) {
    return index < shapes.size() ? shapes[index].shape.get() : nullptr;
}

Math::Vector2D CompoundShape::getShapeOffset(size_t index) const {
    return index < shapes.size() ? shapes[index].offset : Math::Vector2D(0, 0);
}

float CompoundShape::getShapeRotation(size_t index) const {
    return index < shapes.size() ? shapes[index].rotation : 0.0f;
}

float CompoundShape::calculateArea() const {
    float totalArea = 0.0f;
    for (const auto& data : shapes) {
        totalArea += data.shape->calculateArea();
    }
    return totalArea;
}

float CompoundShape::calculateInertia(float mass) const {
    float totalInertia = 0.0f;
    for (const auto& data : shapes) {
        totalInertia += data.shape->calculateInertia(mass / shapes.size());
    }
    return totalInertia;
}

void CompoundShape::computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                                float posX, float posY, float rotation) const {
    if (shapes.empty()) {
        minX = minY = maxX = maxY = 0;
        return;
    }
    
    shapes[0].shape->computeAABB(minX, minY, maxX, maxY, posX, posY, rotation);
    
    for (size_t i = 1; i < shapes.size(); ++i) {
        float sMinX, sMinY, sMaxX, sMaxY;
        shapes[i].shape->computeAABB(sMinX, sMinY, sMaxX, sMaxY, posX, posY, rotation);
        minX = std::min(minX, sMinX);
        minY = std::min(minY, sMinY);
        maxX = std::max(maxX, sMaxX);
        maxY = std::max(maxY, sMaxY);
    }
}

// ShapeFactory implementation
std::unique_ptr<CircleShape> ShapeFactory::createCircle(float radius) {
    return std::make_unique<CircleShape>(radius);
}

std::unique_ptr<BoxShape> ShapeFactory::createBox(float width, float height) {
    return std::make_unique<BoxShape>(width, height);
}

std::unique_ptr<PolygonShape> ShapeFactory::createPolygon(const std::vector<Math::Vector2D>& vertices) {
    auto shape = std::make_unique<PolygonShape>();
    shape->setVertices(vertices);
    return shape;
}

std::unique_ptr<CapsuleShape> ShapeFactory::createCapsule(float radius, float height) {
    return std::make_unique<CapsuleShape>(radius, height);
}

std::unique_ptr<EdgeShape> ShapeFactory::createEdge(const Math::Vector2D& v1, const Math::Vector2D& v2) {
    auto shape = std::make_unique<EdgeShape>();
    shape->setVertices(v1, v2);
    return shape;
}

std::unique_ptr<ChainShape> ShapeFactory::createChain(const std::vector<Math::Vector2D>& vertices, bool loop) {
    auto shape = std::make_unique<ChainShape>();
    if (loop) {
        shape->createLoop(vertices);
    } else {
        shape->createChain(vertices);
    }
    return shape;
}

// ShapeUtils implementation
MassData ShapeUtils::computeMass(const CollisionShape& shape) {
    MassData data;
    data.mass = shape.calculateArea() * shape.getDensity();
    data.center = Math::Vector2D(0, 0);
    data.inertia = shape.calculateInertia(data.mass);
    return data;
}

bool ShapeUtils::testPoint(const CollisionShape& shape, const Math::Vector2D& point,
                           const Math::Vector2D& position, float rotation) {
    (void)shape; (void)point; (void)position; (void)rotation;
    return false; // Stub
}

bool ShapeUtils::testOverlap(const CollisionShape& shape1, const Math::Vector2D& pos1, float rot1,
                             const CollisionShape& shape2, const Math::Vector2D& pos2, float rot2) {
    (void)shape1; (void)pos1; (void)rot1; (void)shape2; (void)pos2; (void)rot2;
    return false; // Stub
}

float ShapeUtils::computeDistance(const CollisionShape& shape1, const Math::Vector2D& pos1,
                                  const CollisionShape& shape2, const Math::Vector2D& pos2) {
    (void)shape1; (void)shape2;
    float dx = pos2.x - pos1.x;
    float dy = pos2.y - pos1.y;
    return std::sqrt(dx * dx + dy * dy);
}

bool ShapeUtils::rayCast(const CollisionShape& shape, const Math::Vector2D& position, float rotation,
                         const Math::Vector2D& rayStart, const Math::Vector2D& rayEnd,
                         Math::Vector2D& hitPoint, Math::Vector2D& hitNormal, float& fraction) {
    (void)shape; (void)position; (void)rotation; (void)rayStart; (void)rayEnd;
    (void)hitPoint; (void)hitNormal; (void)fraction;
    return false; // Stub
}

} // namespace Physics
} // namespace JJM
