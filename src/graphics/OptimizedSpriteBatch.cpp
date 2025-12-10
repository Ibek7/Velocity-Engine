#include "graphics/OptimizedSpriteBatch.h"
#include <algorithm>
#include <cmath>

namespace JJM {
namespace Graphics {

// OptimizedSpriteBatch implementation
OptimizedSpriteBatch::OptimizedSpriteBatch(size_t initialCapacity)
    : quadCount(0), batchCount(0), drawCallCount(0),
      currentSortMode(BatchSortMode::Deferred), begun(false) {
    quads.reserve(initialCapacity);
    vertexBuffer.reserve(initialCapacity * 4);
    indexBuffer.reserve(initialCapacity * 6);
}

OptimizedSpriteBatch::~OptimizedSpriteBatch() {}

void OptimizedSpriteBatch::begin(BatchSortMode sortMode) {
    if (begun) {
        flush();
    }
    
    currentSortMode = sortMode;
    begun = true;
    quadCount = 0;
    batchCount = 0;
    drawCallCount = 0;
}

void OptimizedSpriteBatch::end() {
    if (!begun) return;
    
    flush();
    begun = false;
}

void OptimizedSpriteBatch::drawQuad(Texture* texture, const Math::Vector2D& position,
                                   const Math::Vector2D& size, const Color& color,
                                   float depth, int layer) {
    if (!begun) return;
    
    if (currentSortMode == BatchSortMode::Immediate) {
        // Render immediately
        BatchQuad quad;
        setupQuad(quad, texture, position, size, color, depth, layer);
        renderBatch(0, 1, texture);
        drawCallCount++;
        return;
    }
    
    if (quadCount >= quads.size()) {
        quads.resize(quads.size() + 256);
    }
    
    setupQuad(quads[quadCount], texture, position, size, color, depth, layer);
    quadCount++;
}

void OptimizedSpriteBatch::drawQuadRotated(Texture* texture, const Math::Vector2D& position,
                                          const Math::Vector2D& size, float rotation,
                                          const Math::Vector2D& origin, const Color& color,
                                          float depth, int layer) {
    if (!begun) return;
    
    if (quadCount >= quads.size()) {
        quads.resize(quads.size() + 256);
    }
    
    BatchQuad& quad = quads[quadCount];
    quad.texture = texture;
    quad.layer = layer;
    quad.depth = depth;
    
    float cos_r = std::cos(rotation);
    float sin_r = std::sin(rotation);
    
    Math::Vector2D corners[4] = {
        Math::Vector2D(-origin.x, -origin.y),
        Math::Vector2D(size.x - origin.x, -origin.y),
        Math::Vector2D(size.x - origin.x, size.y - origin.y),
        Math::Vector2D(-origin.x, size.y - origin.y)
    };
    
    for (int i = 0; i < 4; ++i) {
        float x = corners[i].x * cos_r - corners[i].y * sin_r + position.x;
        float y = corners[i].x * sin_r + corners[i].y * cos_r + position.y;
        
        quad.vertices[i].x = x;
        quad.vertices[i].y = y;
        quad.vertices[i].r = color.r;
        quad.vertices[i].g = color.g;
        quad.vertices[i].b = color.b;
        quad.vertices[i].a = color.a;
    }
    
    quad.vertices[0].u = 0; quad.vertices[0].v = 0;
    quad.vertices[1].u = 1; quad.vertices[1].v = 0;
    quad.vertices[2].u = 1; quad.vertices[2].v = 1;
    quad.vertices[3].u = 0; quad.vertices[3].v = 1;
    
    quadCount++;
}

void OptimizedSpriteBatch::drawQuadUV(Texture* texture, const Math::Vector2D& position,
                                     const Math::Vector2D& size, const Math::Vector2D& uvMin,
                                     const Math::Vector2D& uvMax, const Color& color,
                                     float depth, int layer) {
    if (!begun) return;
    
    if (quadCount >= quads.size()) {
        quads.resize(quads.size() + 256);
    }
    
    BatchQuad& quad = quads[quadCount];
    setupQuad(quad, texture, position, size, color, depth, layer);
    
    quad.vertices[0].u = uvMin.x; quad.vertices[0].v = uvMin.y;
    quad.vertices[1].u = uvMax.x; quad.vertices[1].v = uvMin.y;
    quad.vertices[2].u = uvMax.x; quad.vertices[2].v = uvMax.y;
    quad.vertices[3].u = uvMin.x; quad.vertices[3].v = uvMax.y;
    
    quadCount++;
}

void OptimizedSpriteBatch::flush() {
    if (quadCount == 0) return;
    
    sortQuads();
    
    size_t start = 0;
    Texture* currentTexture = quads[0].texture;
    
    for (size_t i = 1; i <= quadCount; ++i) {
        if (i == quadCount || quads[i].texture != currentTexture) {
            renderBatch(start, i - start, currentTexture);
            drawCallCount++;
            
            if (i < quadCount) {
                start = i;
                currentTexture = quads[i].texture;
            }
        }
    }
    
    batchCount++;
    quadCount = 0;
}

void OptimizedSpriteBatch::reset() {
    quadCount = 0;
    batchCount = 0;
    drawCallCount = 0;
}

void OptimizedSpriteBatch::sortQuads() {
    if (quadCount <= 1) return;
    
    switch (currentSortMode) {
        case BatchSortMode::Deferred:
            // No sorting needed
            break;
            
        case BatchSortMode::Texture:
            std::sort(quads.begin(), quads.begin() + quadCount,
                     [](const BatchQuad& a, const BatchQuad& b) {
                         return a.texture < b.texture;
                     });
            break;
            
        case BatchSortMode::BackToFront:
            std::sort(quads.begin(), quads.begin() + quadCount,
                     [](const BatchQuad& a, const BatchQuad& b) {
                         return a.depth > b.depth;
                     });
            break;
            
        case BatchSortMode::FrontToBack:
            std::sort(quads.begin(), quads.begin() + quadCount,
                     [](const BatchQuad& a, const BatchQuad& b) {
                         return a.depth < b.depth;
                     });
            break;
            
        case BatchSortMode::Immediate:
            // Already rendered
            break;
    }
}

void OptimizedSpriteBatch::renderBatch(size_t start, size_t count, Texture* texture) {
    // Platform-specific rendering would go here
    // This would typically bind the texture and render the quads
}

void OptimizedSpriteBatch::setupQuad(BatchQuad& quad, Texture* texture,
                                    const Math::Vector2D& position,
                                    const Math::Vector2D& size,
                                    const Color& color, float depth, int layer) {
    quad.texture = texture;
    quad.layer = layer;
    quad.depth = depth;
    
    quad.vertices[0].x = position.x;
    quad.vertices[0].y = position.y;
    quad.vertices[0].u = 0; quad.vertices[0].v = 0;
    
    quad.vertices[1].x = position.x + size.x;
    quad.vertices[1].y = position.y;
    quad.vertices[1].u = 1; quad.vertices[1].v = 0;
    
    quad.vertices[2].x = position.x + size.x;
    quad.vertices[2].y = position.y + size.y;
    quad.vertices[2].u = 1; quad.vertices[2].v = 1;
    
    quad.vertices[3].x = position.x;
    quad.vertices[3].y = position.y + size.y;
    quad.vertices[3].u = 0; quad.vertices[3].v = 1;
    
    for (int i = 0; i < 4; ++i) {
        quad.vertices[i].r = color.r;
        quad.vertices[i].g = color.g;
        quad.vertices[i].b = color.b;
        quad.vertices[i].a = color.a;
    }
}

// DynamicBatchRenderer implementation
DynamicBatchRenderer::DynamicBatchRenderer()
    : vertexCount(0), indexCount(0), currentTexture(nullptr) {}

DynamicBatchRenderer::~DynamicBatchRenderer() {}

void DynamicBatchRenderer::begin() {
    vertices.clear();
    indices.clear();
    vertexCount = 0;
    indexCount = 0;
    currentTexture = nullptr;
}

void DynamicBatchRenderer::end() {
    flush();
}

void DynamicBatchRenderer::submitSprite(Texture* texture, const Math::Vector2D& position,
                                       const Math::Vector2D& size, const Color& color) {
    if (currentTexture != nullptr && currentTexture != texture) {
        flush();
    }
    
    currentTexture = texture;
    addQuad(texture, position, size, color);
}

void DynamicBatchRenderer::flush() {
    if (vertexCount == 0) return;
    
    // Platform-specific rendering would go here
    
    vertices.clear();
    indices.clear();
    vertexCount = 0;
    indexCount = 0;
    currentTexture = nullptr;
}

void DynamicBatchRenderer::addQuad(Texture* texture, const Math::Vector2D& position,
                                  const Math::Vector2D& size, const Color& color) {
    unsigned int baseVertex = vertexCount;
    
    // Add vertices (x, y, u, v, r, g, b, a)
    vertices.insert(vertices.end(), {
        position.x, position.y, 0.0f, 0.0f,
        static_cast<float>(color.r), static_cast<float>(color.g),
        static_cast<float>(color.b), static_cast<float>(color.a)
    });
    
    vertices.insert(vertices.end(), {
        position.x + size.x, position.y, 1.0f, 0.0f,
        static_cast<float>(color.r), static_cast<float>(color.g),
        static_cast<float>(color.b), static_cast<float>(color.a)
    });
    
    vertices.insert(vertices.end(), {
        position.x + size.x, position.y + size.y, 1.0f, 1.0f,
        static_cast<float>(color.r), static_cast<float>(color.g),
        static_cast<float>(color.b), static_cast<float>(color.a)
    });
    
    vertices.insert(vertices.end(), {
        position.x, position.y + size.y, 0.0f, 1.0f,
        static_cast<float>(color.r), static_cast<float>(color.g),
        static_cast<float>(color.b), static_cast<float>(color.a)
    });
    
    // Add indices (two triangles)
    indices.insert(indices.end(), {
        baseVertex, baseVertex + 1, baseVertex + 2,
        baseVertex, baseVertex + 2, baseVertex + 3
    });
    
    vertexCount += 4;
    indexCount += 6;
}

// TextureBatcher implementation
TextureBatcher::TextureBatcher() {}

TextureBatcher::~TextureBatcher() {}

void TextureBatcher::addSprite(Texture* texture, const Math::Vector2D& position,
                              const Math::Vector2D& size) {
    auto& batch = batches[texture];
    batch.texture = texture;
    batch.positions.push_back(position);
    batch.sizes.push_back(size);
}

void TextureBatcher::render() {
    for (auto& pair : batches) {
        // Platform-specific rendering would go here
        // Render all sprites with the same texture in one batch
    }
}

void TextureBatcher::clear() {
    batches.clear();
}

size_t TextureBatcher::getBatchCount() const {
    return batches.size();
}

// InstancedSpriteBatch implementation
InstancedSpriteBatch::InstancedSpriteBatch(size_t maxInstances)
    : maxInstances(maxInstances) {}

InstancedSpriteBatch::~InstancedSpriteBatch() {}

void InstancedSpriteBatch::begin() {
    instanceMap.clear();
}

void InstancedSpriteBatch::end() {
    render();
}

void InstancedSpriteBatch::addInstance(Texture* texture, const Math::Vector2D& position,
                                      const Math::Vector2D& size, const Color& color,
                                      float rotation) {
    InstanceData data;
    data.position = position;
    data.size = size;
    data.color = color;
    data.rotation = rotation;
    
    instanceMap[texture].push_back(data);
}

void InstancedSpriteBatch::flush() {
    render();
    instanceMap.clear();
}

void InstancedSpriteBatch::render() {
    for (auto& pair : instanceMap) {
        renderInstanced(pair.first, pair.second);
    }
}

void InstancedSpriteBatch::renderInstanced(Texture* texture,
                                          const std::vector<InstanceData>& instances) {
    // Platform-specific instanced rendering would go here
    // This would use GPU instancing to render all instances in one draw call
}

} // namespace Graphics
} // namespace JJM
