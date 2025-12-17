#ifndef JJM_COLLISION_SHAPES_H
#define JJM_COLLISION_SHAPES_H

#include "math/Vector2D.h"
#include <vector>
#include <memory>

namespace JJM {
namespace Physics {

/**
 * @brief Base class for collision shapes
 */
class CollisionShape {
public:
    enum class Type {
        Circle,
        Box,
        Polygon,
        Capsule,
        Edge,
        Chain
    };

    CollisionShape(Type type);
    virtual ~CollisionShape();

    Type getType() const;
    
    virtual void setDensity(float density);
    virtual void setFriction(float friction);
    virtual void setRestitution(float restitution);
    
    float getDensity() const;
    float getFriction() const;
    float getRestitution() const;
    
    virtual float calculateArea() const = 0;
    virtual float calculateInertia(float mass) const = 0;
    virtual void computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                            float posX, float posY, float rotation) const = 0;

protected:
    Type type;
    float density;
    float friction;
    float restitution;
};

/**
 * @brief Circle collision shape
 */
class CircleShape : public CollisionShape {
public:
    CircleShape(float radius = 1.0f);
    ~CircleShape() override;

    void setRadius(float radius);
    float getRadius() const;
    
    void setCenter(float x, float y);
    Math::Vector2D getCenter() const;

    float calculateArea() const override;
    float calculateInertia(float mass) const override;
    void computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                    float posX, float posY, float rotation) const override;

private:
    float radius;
    Math::Vector2D center;
};

/**
 * @brief Box collision shape
 */
class BoxShape : public CollisionShape {
public:
    BoxShape(float width = 1.0f, float height = 1.0f);
    ~BoxShape() override;

    void setSize(float width, float height);
    void setCenter(float x, float y);
    
    float getWidth() const;
    float getHeight() const;
    Math::Vector2D getCenter() const;

    float calculateArea() const override;
    float calculateInertia(float mass) const override;
    void computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                    float posX, float posY, float rotation) const override;

private:
    float width;
    float height;
    Math::Vector2D center;
};

/**
 * @brief Polygon collision shape
 */
class PolygonShape : public CollisionShape {
public:
    PolygonShape();
    ~PolygonShape() override;

    void setVertices(const std::vector<Math::Vector2D>& vertices);
    void addVertex(const Math::Vector2D& vertex);
    void clearVertices();
    
    const std::vector<Math::Vector2D>& getVertices() const;
    size_t getVertexCount() const;
    
    void setAsBox(float width, float height);
    void setAsBox(float width, float height, const Math::Vector2D& center, float angle);

    float calculateArea() const override;
    float calculateInertia(float mass) const override;
    void computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                    float posX, float posY, float rotation) const override;

private:
    std::vector<Math::Vector2D> vertices;
    Math::Vector2D centroid;
    
    void computeCentroid();
    bool validateVertices() const;
};

/**
 * @brief Capsule collision shape (pill shape)
 */
class CapsuleShape : public CollisionShape {
public:
    CapsuleShape(float radius = 0.5f, float height = 1.0f);
    ~CapsuleShape() override;

    void setRadius(float radius);
    void setHeight(float height);
    
    float getRadius() const;
    float getHeight() const;

    float calculateArea() const override;
    float calculateInertia(float mass) const override;
    void computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                    float posX, float posY, float rotation) const override;

private:
    float radius;
    float height;
};

/**
 * @brief Edge collision shape (line segment)
 */
class EdgeShape : public CollisionShape {
public:
    EdgeShape();
    ~EdgeShape() override;

    void setVertices(const Math::Vector2D& v1, const Math::Vector2D& v2);
    void setGhostVertices(const Math::Vector2D& v0, const Math::Vector2D& v3);
    
    Math::Vector2D getVertex1() const;
    Math::Vector2D getVertex2() const;
    
    bool hasGhostVertices() const;

    float calculateArea() const override;
    float calculateInertia(float mass) const override;
    void computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                    float posX, float posY, float rotation) const override;

private:
    Math::Vector2D vertex1;
    Math::Vector2D vertex2;
    Math::Vector2D ghostVertex0;
    Math::Vector2D ghostVertex3;
    bool hasGhosts;
};

/**
 * @brief Chain collision shape (connected edges)
 */
class ChainShape : public CollisionShape {
public:
    ChainShape();
    ~ChainShape() override;

    void createChain(const std::vector<Math::Vector2D>& vertices);
    void createLoop(const std::vector<Math::Vector2D>& vertices);
    void clearVertices();
    
    const std::vector<Math::Vector2D>& getVertices() const;
    size_t getVertexCount() const;
    bool isLoop() const;

    float calculateArea() const override;
    float calculateInertia(float mass) const override;
    void computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                    float posX, float posY, float rotation) const override;

private:
    std::vector<Math::Vector2D> vertices;
    bool loop;
};

/**
 * @brief Compound collision shape (multiple shapes)
 */
class CompoundShape : public CollisionShape {
public:
    CompoundShape();
    ~CompoundShape() override;

    void addShape(std::unique_ptr<CollisionShape> shape, 
                  const Math::Vector2D& offset, float rotation);
    void removeShape(size_t index);
    void clearShapes();
    
    size_t getShapeCount() const;
    CollisionShape* getShape(size_t index);
    Math::Vector2D getShapeOffset(size_t index) const;
    float getShapeRotation(size_t index) const;

    float calculateArea() const override;
    float calculateInertia(float mass) const override;
    void computeAABB(float& minX, float& minY, float& maxX, float& maxY,
                    float posX, float posY, float rotation) const override;

private:
    struct ShapeData {
        std::unique_ptr<CollisionShape> shape;
        Math::Vector2D offset;
        float rotation;
    };
    
    std::vector<ShapeData> shapes;
};

/**
 * @brief Shape factory for creating collision shapes
 */
class ShapeFactory {
public:
    static std::unique_ptr<CircleShape> createCircle(float radius);
    static std::unique_ptr<BoxShape> createBox(float width, float height);
    static std::unique_ptr<PolygonShape> createPolygon(const std::vector<Math::Vector2D>& vertices);
    static std::unique_ptr<CapsuleShape> createCapsule(float radius, float height);
    static std::unique_ptr<EdgeShape> createEdge(const Math::Vector2D& v1, const Math::Vector2D& v2);
    static std::unique_ptr<ChainShape> createChain(const std::vector<Math::Vector2D>& vertices, bool loop);
};

/**
 * @brief Mass data computed from collision shape
 */
struct MassData {
    float mass;
    Math::Vector2D center;
    float inertia;
};

/**
 * @brief Utilities for collision shapes
 */
class ShapeUtils {
public:
    static MassData computeMass(const CollisionShape& shape);
    static bool testPoint(const CollisionShape& shape, const Math::Vector2D& point,
                         const Math::Vector2D& position, float rotation);
    static bool testOverlap(const CollisionShape& shape1, const Math::Vector2D& pos1, float rot1,
                           const CollisionShape& shape2, const Math::Vector2D& pos2, float rot2);
    
    static float computeDistance(const CollisionShape& shape1, const Math::Vector2D& pos1,
                                const CollisionShape& shape2, const Math::Vector2D& pos2);
    
    static bool rayCast(const CollisionShape& shape, const Math::Vector2D& position, float rotation,
                       const Math::Vector2D& rayStart, const Math::Vector2D& rayEnd,
                       Math::Vector2D& hitPoint, Math::Vector2D& hitNormal, float& fraction);
};

} // namespace Physics
} // namespace JJM

#endif // JJM_COLLISION_SHAPES_H
