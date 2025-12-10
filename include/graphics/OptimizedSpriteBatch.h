#pragma once

#include "graphics/SpriteBatch.h"
#include "graphics/Texture.h"
#include "math/Vector2D.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace JJM {
namespace Graphics {

struct BatchVertex {
    float x, y;
    float u, v;
    uint8_t r, g, b, a;
    
    BatchVertex() : x(0), y(0), u(0), v(0), r(255), g(255), b(255), a(255) {}
};

struct BatchQuad {
    BatchVertex vertices[4];
    Texture* texture;
    int layer;
    float depth;
    
    BatchQuad() : texture(nullptr), layer(0), depth(0) {}
};

enum class BatchSortMode {
    Deferred,           // No sorting, drawn in order submitted
    Texture,            // Sort by texture to minimize state changes
    BackToFront,        // Sort by depth, back to front
    FrontToBack,        // Sort by depth, front to back
    Immediate          // Draw immediately, no batching
};

class OptimizedSpriteBatch {
public:
    OptimizedSpriteBatch(size_t initialCapacity = 2048);
    ~OptimizedSpriteBatch();
    
    void begin(BatchSortMode sortMode = BatchSortMode::Deferred);
    void end();
    
    void drawQuad(Texture* texture, const Math::Vector2D& position,
                  const Math::Vector2D& size, const Color& color,
                  float depth = 0.0f, int layer = 0);
    
    void drawQuadRotated(Texture* texture, const Math::Vector2D& position,
                        const Math::Vector2D& size, float rotation,
                        const Math::Vector2D& origin, const Color& color,
                        float depth = 0.0f, int layer = 0);
    
    void drawQuadUV(Texture* texture, const Math::Vector2D& position,
                   const Math::Vector2D& size, const Math::Vector2D& uvMin,
                   const Math::Vector2D& uvMax, const Color& color,
                   float depth = 0.0f, int layer = 0);
    
    void flush();
    
    size_t getBatchCount() const { return batchCount; }
    size_t getQuadCount() const { return quadCount; }
    size_t getDrawCallCount() const { return drawCallCount; }
    
    void reset();

private:
    std::vector<BatchQuad> quads;
    size_t quadCount;
    size_t batchCount;
    size_t drawCallCount;
    
    BatchSortMode currentSortMode;
    bool begun;
    
    std::vector<BatchVertex> vertexBuffer;
    std::vector<unsigned int> indexBuffer;
    
    void sortQuads();
    void renderBatch(size_t start, size_t count, Texture* texture);
    void setupQuad(BatchQuad& quad, Texture* texture,
                  const Math::Vector2D& position, const Math::Vector2D& size,
                  const Color& color, float depth, int layer);
};

class DynamicBatchRenderer {
public:
    DynamicBatchRenderer();
    ~DynamicBatchRenderer();
    
    void begin();
    void end();
    
    void submitSprite(Texture* texture, const Math::Vector2D& position,
                     const Math::Vector2D& size, const Color& color);
    
    void flush();
    
    size_t getVertexCount() const { return vertexCount; }
    size_t getIndexCount() const { return indexCount; }

private:
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    size_t vertexCount;
    size_t indexCount;
    
    Texture* currentTexture;
    
    void addQuad(Texture* texture, const Math::Vector2D& position,
                const Math::Vector2D& size, const Color& color);
};

class TextureBatcher {
public:
    TextureBatcher();
    ~TextureBatcher();
    
    void addSprite(Texture* texture, const Math::Vector2D& position,
                  const Math::Vector2D& size);
    
    void render();
    void clear();
    
    size_t getBatchCount() const;

private:
    struct TextureBatch {
        Texture* texture;
        std::vector<Math::Vector2D> positions;
        std::vector<Math::Vector2D> sizes;
    };
    
    std::unordered_map<Texture*, TextureBatch> batches;
};

class InstancedSpriteBatch {
public:
    InstancedSpriteBatch(size_t maxInstances = 10000);
    ~InstancedSpriteBatch();
    
    void begin();
    void end();
    
    void addInstance(Texture* texture, const Math::Vector2D& position,
                    const Math::Vector2D& size, const Color& color,
                    float rotation = 0.0f);
    
    void flush();
    void render();

private:
    struct InstanceData {
        Math::Vector2D position;
        Math::Vector2D size;
        Color color;
        float rotation;
    };
    
    std::unordered_map<Texture*, std::vector<InstanceData>> instanceMap;
    size_t maxInstances;
    
    void renderInstanced(Texture* texture, const std::vector<InstanceData>& instances);
};

} // namespace Graphics
} // namespace JJM
