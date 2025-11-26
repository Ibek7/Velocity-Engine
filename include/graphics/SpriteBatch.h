#ifndef SPRITE_BATCH_H
#define SPRITE_BATCH_H

#include "math/Vector2D.h"
#include "graphics/Color.h"
#include "graphics/Texture.h"
#include "graphics/Renderer.h"
#include <vector>
#include <SDL2/SDL.h>

namespace JJM {
namespace Graphics {

struct SpriteData {
    Texture* texture;
    Math::Vector2D position;
    Math::Vector2D size;
    float rotation;
    Math::Vector2D origin;
    Color tint;
    SDL_Rect* sourceRect;
    int layer;
    
    SpriteData()
        : texture(nullptr), position(0, 0), size(0, 0),
          rotation(0), origin(0, 0), tint(Color::White()),
          sourceRect(nullptr), layer(0) {}
};

class SpriteBatch {
private:
    std::vector<SpriteData> sprites;
    Renderer* renderer;
    bool begun;
    bool needsSort;
    
public:
    SpriteBatch(Renderer* renderer);
    ~SpriteBatch();
    
    void begin();
    void end();
    
    void draw(Texture* texture, const Math::Vector2D& position);
    void draw(Texture* texture, const Math::Vector2D& position, const Math::Vector2D& size);
    void draw(Texture* texture, const Math::Vector2D& position, const Math::Vector2D& size,
             float rotation, const Math::Vector2D& origin, const Color& tint);
    void draw(Texture* texture, const Math::Vector2D& position, const Math::Vector2D& size,
             SDL_Rect* sourceRect, float rotation, const Math::Vector2D& origin,
             const Color& tint, int layer = 0);
    
    void flush();
    void clear();
    
    int getSpriteCount() const { return static_cast<int>(sprites.size()); }
    
private:
    void sortSprites();
};

} // namespace Graphics
} // namespace JJM

#endif // SPRITE_BATCH_H
