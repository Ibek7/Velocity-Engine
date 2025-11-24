#include "graphics/Renderer.h"
#include <iostream>
#include <cmath>

namespace JJM {
namespace Graphics {

Renderer::Renderer() 
    : window(nullptr), renderer(nullptr), 
      windowWidth(0), windowHeight(0), isInitialized(false) {}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize(const std::string& title, int width, int height, bool fullscreen) {
    if (isInitialized) {
        std::cerr << "Renderer already initialized!" << std::endl;
        return false;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }

    Uint32 flags = SDL_WINDOW_SHOWN;
    if (fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width, height,
        flags
    );

    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    windowWidth = width;
    windowHeight = height;
    isInitialized = true;

    // Set default blend mode
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    std::cout << "Renderer initialized: " << width << "x" << height << std::endl;
    return true;
}

void Renderer::shutdown() {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    if (isInitialized) {
        SDL_Quit();
        isInitialized = false;
    }
}

void Renderer::setWindowTitle(const std::string& title) {
    if (window) {
        SDL_SetWindowTitle(window, title.c_str());
    }
}

void Renderer::setWindowSize(int width, int height) {
    if (window) {
        SDL_SetWindowSize(window, width, height);
        windowWidth = width;
        windowHeight = height;
    }
}

void Renderer::toggleFullscreen() {
    if (window) {
        Uint32 flags = SDL_GetWindowFlags(window);
        if (flags & SDL_WINDOW_FULLSCREEN) {
            SDL_SetWindowFullscreen(window, 0);
        } else {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        }
    }
}

void Renderer::clear() {
    if (renderer) {
        SDL_RenderClear(renderer);
    }
}

void Renderer::clear(const Color& color) {
    if (renderer) {
        setDrawColor(color);
        SDL_RenderClear(renderer);
    }
}

void Renderer::present() {
    if (renderer) {
        SDL_RenderPresent(renderer);
    }
}

void Renderer::drawPoint(const Math::Vector2D& pos, const Color& color) {
    if (renderer) {
        setDrawColor(color);
        SDL_RenderDrawPoint(renderer, static_cast<int>(pos.x), static_cast<int>(pos.y));
    }
}

void Renderer::drawLine(const Math::Vector2D& start, const Math::Vector2D& end, const Color& color) {
    if (renderer) {
        setDrawColor(color);
        SDL_RenderDrawLine(renderer, 
            static_cast<int>(start.x), static_cast<int>(start.y),
            static_cast<int>(end.x), static_cast<int>(end.y));
    }
}

void Renderer::drawRect(const Math::Vector2D& pos, const Math::Vector2D& size, 
                       const Color& color, bool filled) {
    if (renderer) {
        setDrawColor(color);
        SDL_Rect rect = {
            static_cast<int>(pos.x), static_cast<int>(pos.y),
            static_cast<int>(size.x), static_cast<int>(size.y)
        };
        if (filled) {
            SDL_RenderFillRect(renderer, &rect);
        } else {
            SDL_RenderDrawRect(renderer, &rect);
        }
    }
}

void Renderer::drawCircle(const Math::Vector2D& center, float radius, 
                         const Color& color, bool filled) {
    if (renderer) {
        setDrawColor(color);
        int r = static_cast<int>(radius);
        int cx = static_cast<int>(center.x);
        int cy = static_cast<int>(center.y);
        
        if (filled) {
            fillCircleHelper(cx, cy, r, color);
        } else {
            drawCircleHelper(cx, cy, r, color);
        }
    }
}

void Renderer::drawCircleHelper(int centerX, int centerY, int radius, const Color& color) {
    setDrawColor(color);
    int x = radius;
    int y = 0;
    int decisionOver2 = 1 - x;

    while (y <= x) {
        SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
        SDL_RenderDrawPoint(renderer, centerX + y, centerY + x);
        SDL_RenderDrawPoint(renderer, centerX - y, centerY + x);
        SDL_RenderDrawPoint(renderer, centerX - x, centerY + y);
        SDL_RenderDrawPoint(renderer, centerX - x, centerY - y);
        SDL_RenderDrawPoint(renderer, centerX - y, centerY - x);
        SDL_RenderDrawPoint(renderer, centerX + y, centerY - x);
        SDL_RenderDrawPoint(renderer, centerX + x, centerY - y);
        y++;

        if (decisionOver2 <= 0) {
            decisionOver2 += 2 * y + 1;
        } else {
            x--;
            decisionOver2 += 2 * (y - x) + 1;
        }
    }
}

void Renderer::fillCircleHelper(int centerX, int centerY, int radius, const Color& color) {
    setDrawColor(color);
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
            }
        }
    }
}

void Renderer::drawTriangle(const Math::Vector2D& p1, const Math::Vector2D& p2, 
                           const Math::Vector2D& p3, const Color& color, bool filled) {
    if (renderer) {
        setDrawColor(color);
        if (filled) {
            // Simple scan-line fill algorithm
            // For simplicity, we'll just draw the outline for now
            drawLine(p1, p2, color);
            drawLine(p2, p3, color);
            drawLine(p3, p1, color);
        } else {
            drawLine(p1, p2, color);
            drawLine(p2, p3, color);
            drawLine(p3, p1, color);
        }
    }
}

void Renderer::drawPolygon(const Math::Vector2D* points, int count, 
                          const Color& color, bool filled) {
    if (renderer && count >= 3) {
        setDrawColor(color);
        for (int i = 0; i < count; i++) {
            drawLine(points[i], points[(i + 1) % count], color);
        }
    }
}

void Renderer::drawArc(const Math::Vector2D& center, float radius, 
                      float startAngle, float endAngle, const Color& color) {
    if (renderer) {
        setDrawColor(color);
        const int segments = 32;
        float angleStep = (endAngle - startAngle) / segments;
        
        for (int i = 0; i < segments; i++) {
            float angle1 = startAngle + i * angleStep;
            float angle2 = startAngle + (i + 1) * angleStep;
            
            Math::Vector2D p1(
                center.x + radius * std::cos(angle1),
                center.y + radius * std::sin(angle1)
            );
            Math::Vector2D p2(
                center.x + radius * std::cos(angle2),
                center.y + radius * std::sin(angle2)
            );
            
            drawLine(p1, p2, color);
        }
    }
}

void Renderer::setDrawColor(const Color& color) {
    if (renderer) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    }
}

void Renderer::setBlendMode(SDL_BlendMode blendMode) {
    if (renderer) {
        SDL_SetRenderDrawBlendMode(renderer, blendMode);
    }
}

void Renderer::setViewport(int x, int y, int width, int height) {
    if (renderer) {
        SDL_Rect viewport = { x, y, width, height };
        SDL_RenderSetViewport(renderer, &viewport);
    }
}

void Renderer::resetViewport() {
    if (renderer) {
        SDL_RenderSetViewport(renderer, nullptr);
    }
}

} // namespace Graphics
} // namespace JJM
