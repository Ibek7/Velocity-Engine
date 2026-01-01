#ifndef RENDERER_H
#define RENDERER_H

#include "math/Vector2D.h"
#include "graphics/Color.h"
#include <SDL.h>
#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace JJM {
namespace Graphics {

// Render statistics
struct RenderStats {
    int drawCalls;
    int trianglesRendered;
    int batchCount;
    int texturesSwapped;
    float frameTime;
    
    void reset() {
        drawCalls = 0;
        trianglesRendered = 0;
        batchCount = 0;
        texturesSwapped = 0;
        frameTime = 0.0f;
    }
};

// Vertex for batched rendering
struct BatchVertex {
    float x, y;
    float u, v;
    uint32_t color;
};

// Batch render command
struct RenderCommand {
    enum class Type {
        Quad,
        Line,
        Triangle,
        Circle,
        Custom
    };
    
    Type type;
    SDL_Texture* texture;
    std::vector<BatchVertex> vertices;
    std::vector<uint32_t> indices;
    SDL_BlendMode blendMode;
    int layer;
};

// Render layer for sorting
struct RenderLayer {
    std::string name;
    int order;
    bool visible;
    float opacity;
    std::vector<RenderCommand> commands;
    
    RenderLayer(const std::string& n, int o)
        : name(n), order(o), visible(true), opacity(1.0f) {}
};

class Renderer {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    int windowWidth;
    int windowHeight;
    bool isInitialized;
    
    // Batching system
    std::vector<RenderCommand> batchedCommands;
    SDL_Texture* currentTexture;
    SDL_BlendMode currentBlendMode;
    static constexpr size_t MAX_BATCH_SIZE = 10000;
    
    // Layer system
    std::vector<std::unique_ptr<RenderLayer>> layers;
    int currentLayerIndex;
    
    // Statistics
    RenderStats stats;
    bool statsEnabled;
    
    // Render target stack
    std::vector<SDL_Texture*> renderTargetStack;
    
    // Post-processing callbacks
    std::vector<std::function<void(Renderer*)>> postProcessCallbacks;

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
    void beginFrame();
    void endFrame();

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
    void drawRoundedRect(const Math::Vector2D& pos, const Math::Vector2D& size, 
                         float radius, const Color& color, bool filled = false);
    void drawGradientRect(const Math::Vector2D& pos, const Math::Vector2D& size,
                          const Color& topLeft, const Color& topRight,
                          const Color& bottomLeft, const Color& bottomRight);
    
    // Batched rendering
    void beginBatch();
    void endBatch();
    void flushBatch();
    void batchQuad(const Math::Vector2D& pos, const Math::Vector2D& size, 
                   const Color& color, SDL_Texture* texture = nullptr,
                   const Math::Vector2D& uvMin = {0, 0}, const Math::Vector2D& uvMax = {1, 1});
    void batchLine(const Math::Vector2D& start, const Math::Vector2D& end, 
                   const Color& color, float thickness = 1.0f);
    void batchTriangle(const Math::Vector2D& p1, const Math::Vector2D& p2, 
                       const Math::Vector2D& p3, const Color& color);
    
    // Layer system
    RenderLayer* createLayer(const std::string& name, int order);
    RenderLayer* getLayer(const std::string& name);
    RenderLayer* getLayer(int index);
    void setCurrentLayer(const std::string& name);
    void setCurrentLayer(int index);
    void setLayerVisible(const std::string& name, bool visible);
    void setLayerOpacity(const std::string& name, float opacity);
    void sortLayers();
    void renderLayers();
    
    // Render targets
    SDL_Texture* createRenderTarget(int width, int height);
    void pushRenderTarget(SDL_Texture* target);
    void popRenderTarget();
    SDL_Texture* getCurrentRenderTarget() const;

    // Color and blend modes
    void setDrawColor(const Color& color);
    void setBlendMode(SDL_BlendMode blendMode);
    
    // Post-processing
    void addPostProcessCallback(std::function<void(Renderer*)> callback);
    void clearPostProcessCallbacks();
    void applyPostProcess();

    // Camera/viewport
    void setViewport(int x, int y, int width, int height);
    void resetViewport();

    // Statistics
    void setStatsEnabled(bool enabled) { statsEnabled = enabled; }
    const RenderStats& getStats() const { return stats; }
    void resetStats() { stats.reset(); }

    // Getters
    SDL_Renderer* getSDLRenderer() { return renderer; }
    bool getInitialized() const { return isInitialized; }

private:
    void drawCircleHelper(int centerX, int centerY, int radius, const Color& color);
    void fillCircleHelper(int centerX, int centerY, int radius, const Color& color);
    void processBatchedCommand(const RenderCommand& cmd);
    uint32_t colorToUint32(const Color& color) const;
};

} // namespace Graphics
} // namespace JJM

#endif // RENDERER_H
