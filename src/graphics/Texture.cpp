#include "graphics/Texture.h"
#include "graphics/Renderer.h"
#include <SDL2/SDL_image.h>
#include <iostream>

namespace JJM {
namespace Graphics {

Texture::Texture() : texture(nullptr), width(0), height(0) {}

Texture::~Texture() {
    free();
}

bool Texture::loadFromFile(const std::string& path, Renderer* renderer) {
    free();
    
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (!loadedSurface) {
        std::cerr << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }
    
    texture = SDL_CreateTextureFromSurface(renderer->getSDLRenderer(), loadedSurface);
    if (!texture) {
        std::cerr << "Unable to create texture from " << path << "! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(loadedSurface);
        return false;
    }
    
    width = loadedSurface->w;
    height = loadedSurface->h;
    filePath = path;
    
    SDL_FreeSurface(loadedSurface);
    return true;
}

void Texture::free() {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
        width = 0;
        height = 0;
    }
}

void Texture::render(Renderer* renderer, int x, int y) {
    SDL_Rect renderQuad = { x, y, width, height };
    SDL_RenderCopy(renderer->getSDLRenderer(), texture, nullptr, &renderQuad);
}

void Texture::render(Renderer* renderer, const Math::Vector2D& pos) {
    render(renderer, static_cast<int>(pos.x), static_cast<int>(pos.y));
}

void Texture::render(Renderer* renderer, int x, int y, SDL_Rect* clip) {
    SDL_Rect renderQuad = { x, y, width, height };
    
    if (clip != nullptr) {
        renderQuad.w = clip->w;
        renderQuad.h = clip->h;
    }
    
    SDL_RenderCopy(renderer->getSDLRenderer(), texture, clip, &renderQuad);
}

void Texture::renderEx(Renderer* renderer, int x, int y, double angle, SDL_Point* center, SDL_RendererFlip flip) {
    SDL_Rect renderQuad = { x, y, width, height };
    SDL_RenderCopyEx(renderer->getSDLRenderer(), texture, nullptr, &renderQuad, angle, center, flip);
}

void Texture::setColorMod(Uint8 r, Uint8 g, Uint8 b) {
    if (texture) {
        SDL_SetTextureColorMod(texture, r, g, b);
    }
}

void Texture::setBlendMode(SDL_BlendMode blending) {
    if (texture) {
        SDL_SetTextureBlendMode(texture, blending);
    }
}

void Texture::setAlpha(Uint8 alpha) {
    if (texture) {
        SDL_SetTextureAlphaMod(texture, alpha);
    }
}

} // namespace Graphics
} // namespace JJM
