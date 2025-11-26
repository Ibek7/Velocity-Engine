#include "graphics/SpriteBatch.h"
#include <algorithm>

namespace JJM {
namespace Graphics {

SpriteBatch::SpriteBatch(Renderer* renderer)
    : renderer(renderer), begun(false), needsSort(false) {
    sprites.reserve(1000); // Pre-allocate for performance
}

SpriteBatch::~SpriteBatch() {
    clear();
}

void SpriteBatch::begin() {
    if (begun) {
        return;
    }
    begun = true;
    needsSort = false;
}

void SpriteBatch::end() {
    if (!begun) {
        return;
    }
    
    flush();
    begun = false;
}

void SpriteBatch::draw(Texture* texture, const Math::Vector2D& position) {
    if (!texture) return;
    draw(texture, position, Math::Vector2D(texture->getWidth(), texture->getHeight()));
}

void SpriteBatch::draw(Texture* texture, const Math::Vector2D& position, const Math::Vector2D& size) {
    draw(texture, position, size, 0, Math::Vector2D(0, 0), Color::White());
}

void SpriteBatch::draw(Texture* texture, const Math::Vector2D& position, const Math::Vector2D& size,
                      float rotation, const Math::Vector2D& origin, const Color& tint) {
    draw(texture, position, size, nullptr, rotation, origin, tint, 0);
}

void SpriteBatch::draw(Texture* texture, const Math::Vector2D& position, const Math::Vector2D& size,
                      SDL_Rect* sourceRect, float rotation, const Math::Vector2D& origin,
                      const Color& tint, int layer) {
    if (!begun || !texture) {
        return;
    }
    
    SpriteData sprite;
    sprite.texture = texture;
    sprite.position = position;
    sprite.size = size;
    sprite.rotation = rotation;
    sprite.origin = origin;
    sprite.tint = tint;
    sprite.sourceRect = sourceRect;
    sprite.layer = layer;
    
    sprites.push_back(sprite);
    
    if (layer != 0) {
        needsSort = true;
    }
}

void SpriteBatch::flush() {
    if (sprites.empty()) {
        return;
    }
    
    if (needsSort) {
        sortSprites();
    }
    
    // Batch render sprites
    for (const auto& sprite : sprites) {
        if (!sprite.texture) continue;
        
        sprite.texture->setColor(sprite.tint);
        
        if (sprite.rotation != 0 || sprite.origin.x != 0 || sprite.origin.y != 0) {
            sprite.texture->renderEx(renderer, sprite.position, sprite.size,
                                    sprite.rotation, sprite.origin);
        } else {
            sprite.texture->render(renderer, sprite.position, sprite.size);
        }
    }
    
    clear();
}

void SpriteBatch::clear() {
    sprites.clear();
    needsSort = false;
}

void SpriteBatch::sortSprites() {
    std::sort(sprites.begin(), sprites.end(),
        [](const SpriteData& a, const SpriteData& b) {
            if (a.layer != b.layer) {
                return a.layer < b.layer;
            }
            // Secondary sort by texture to minimize texture switching
            return a.texture < b.texture;
        });
    needsSort = false;
}

} // namespace Graphics
} // namespace JJM
