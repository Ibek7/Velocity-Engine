#ifndef RENDERER_H
#define RENDERER_H

#include "math/Vector2D.h"
#include "graphics/Color.h"
#include <SDL2/SDL.h>
#include <string>
#include <memory>

namespace JJM {
namespace Graphics {

class Renderer {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    int windowWidth;
    int windowHeight;
    bool isInitialized;

public:
    Renderer();
    ~Renderer();

    // Initialization
    bool initialize(const std::string& title, int width, int height, bool fullscreen = false);
    void shutdown();

    // Window management
    void setWindowTitle(const std::string& title);
    void setWindowSize(int width, int height);
    void toggleFullscreen();
    int getWindowWidth() const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }

    // Frame management
    void clear();
    void clear(const Color& color);
    void present();

    // Drawing primitives
    void drawPoint(const Math::Vector2D& pos, const Color& color);
    void drawLine(const Math::Vector2D& start, const Math::Vector2D& end, const Color& color);
    void drawRect(const Math::Vector2D& pos, const Math::Vector2D& size, const Color& color, bool filled = false);
    void drawCircle(const Math::Vector2D& center, float radius, const Color& color, bool filled = false);
    void drawTriangle(const Math::Vector2D& p1, const Math::Vector2D& p2, const Math::Vector2D& p3, 
                     const Color& color, bool filled = false);

    // Advanced drawing
    void drawPolygon(const Math::Vector2D* points, int count, const Color& color, bool filled = false);
    void drawArc(const Math::Vector2D& center, float radius, float startAngle, float endAngle, 
                const Color& color);

    // Color and blend modes
    void setDrawColor(const Color& color);
    void setBlendMode(SDL_BlendMode blendMode);

    // Camera/viewport
    void setViewport(int x, int y, int width, int height);
    void resetViewport();

    // Getters
    SDL_Renderer* getSDLRenderer() { return renderer; }
    bool getInitialized() const { return isInitialized; }

private:
    void drawCircleHelper(int centerX, int centerY, int radius, const Color& color);
    void fillCircleHelper(int centerX, int centerY, int radius, const Color& color);
};

} // namespace Graphics
} // namespace JJM

#endif // RENDERER_H
