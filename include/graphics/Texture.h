#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL2/SDL.h>
#include <string>
#include "math/Vector2D.h"

namespace JJM {
namespace Graphics {

class Renderer;

class Texture {
private:
    SDL_Texture* texture;
    int width;
    int height;
    std::string filePath;

public:
    Texture();
    ~Texture();

    bool loadFromFile(const std::string& path, Renderer* renderer);
    void free();
    
    void render(Renderer* renderer, int x, int y);
    void render(Renderer* renderer, const Math::Vector2D& pos);
    void render(Renderer* renderer, int x, int y, SDL_Rect* clip);
    void renderEx(Renderer* renderer, int x, int y, double angle, SDL_Point* center, SDL_RendererFlip flip);
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    SDL_Texture* getSDLTexture() const { return texture; }
    const std::string& getFilePath() const { return filePath; }
    
    void setColorMod(Uint8 r, Uint8 g, Uint8 b);
    void setBlendMode(SDL_BlendMode blending);
    void setAlpha(Uint8 alpha);
};

} // namespace Graphics
} // namespace JJM

#endif // TEXTURE_H
