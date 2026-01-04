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

// =============================================================================
// Level of Detail (LOD) System
// =============================================================================

/**
 * @brief LOD level configuration
 */
struct LODLevel {
    int level;                      // LOD level (0 = highest detail)
    float screenSizeThreshold;      // Switch when object is smaller than this (0-1)
    float distanceThreshold;        // Alternative: switch at this distance
    float transitionRange;          // Range for smooth transitions
    std::string meshPath;           // Path to LOD mesh
    float triangleReduction;        // Percentage of original triangles
    bool useImpostors;              // Use billboard impostors instead of mesh
    
    LODLevel()
        : level(0)
        , screenSizeThreshold(1.0f)
        , distanceThreshold(0.0f)
        , transitionRange(5.0f)
        , triangleReduction(1.0f)
        , useImpostors(false)
    {}
};

/**
 * @brief LOD selection method
 */
enum class LODSelectionMethod {
    Distance,           // Based on distance from camera
    ScreenSize,         // Based on projected screen size
    Combined,           // Use both factors
    Manual              // Manually set LOD level
};

/**
 * @brief LOD transition mode
 */
enum class LODTransitionMode {
    Instant,            // Immediate switch
    CrossFade,          // Fade between LODs
    Dither,             // Dithered transition
    GeometryMorph       // Morph geometry between LODs
};

/**
 * @brief LOD group configuration
 */
struct LODGroupConfig {
    LODSelectionMethod selectionMethod;
    LODTransitionMode transitionMode;
    float lodBias;                  // Bias towards higher/lower LODs
    float maxDistance;              // Force lowest LOD beyond this
    bool animateCrossFade;          // Animate cross-fade transitions
    float crossFadeDuration;        // Duration of cross-fade in seconds
    bool useCameraVelocity;         // Adjust LOD based on camera speed
    
    LODGroupConfig()
        : selectionMethod(LODSelectionMethod::Distance)
        , transitionMode(LODTransitionMode::CrossFade)
        , lodBias(1.0f)
        , maxDistance(1000.0f)
        , animateCrossFade(true)
        , crossFadeDuration(0.2f)
        , useCameraVelocity(false)
    {}
};

/**
 * @brief LOD group for managing mesh LODs
 */
class LODGroup {
private:
    std::vector<LODLevel> m_levels;
    LODGroupConfig m_config;
    int m_currentLOD;
    int m_targetLOD;
    float m_transitionProgress;
    float m_lastScreenSize;
    float m_lastDistance;
    bool m_forcedLOD;
    int m_forcedLODLevel;
    
public:
    LODGroup();
    ~LODGroup();
    
    // LOD level management
    void addLOD(const LODLevel& level);
    void removeLOD(int level);
    void clearLODs();
    LODLevel* getLOD(int level);
    const LODLevel* getLOD(int level) const;
    int getLODCount() const { return static_cast<int>(m_levels.size()); }
    
    // Configuration
    void setConfig(const LODGroupConfig& config) { m_config = config; }
    const LODGroupConfig& getConfig() const { return m_config; }
    
    // LOD calculation
    int calculateLOD(float distance, float screenSize, float cameraVelocity = 0.0f) const;
    void update(float deltaTime, float distance, float screenSize, float cameraVelocity = 0.0f);
    
    // Current state
    int getCurrentLOD() const { return m_currentLOD; }
    int getTargetLOD() const { return m_targetLOD; }
    float getTransitionProgress() const { return m_transitionProgress; }
    bool isTransitioning() const { return m_transitionProgress < 1.0f && m_currentLOD != m_targetLOD; }
    
    // Forced LOD
    void forceLOD(int level);
    void clearForcedLOD();
    bool isForcedLOD() const { return m_forcedLOD; }
    
    // Utility
    float getTriangleReduction(int level) const;
    float estimateScreenSize(float distance, float boundingSphereRadius, float fov, float screenHeight) const;
};

/**
 * @brief LOD manager for global LOD settings
 */
class LODManager {
private:
    static LODManager* instance;
    
    // Global settings
    float m_globalLODBias;
    float m_maxRenderDistance;
    int m_maxLODLevel;
    bool m_enabled;
    
    // Quality presets
    struct QualityPreset {
        std::string name;
        float lodBias;
        float maxDistance;
        int maxLOD;
    };
    std::vector<QualityPreset> m_presets;
    int m_currentPreset;
    
    // Statistics
    struct LODStats {
        int objectsAtLOD[8];        // Count per LOD level
        int totalTransitions;
        float averageLOD;
        int culledObjects;
    };
    mutable LODStats m_stats;
    
    // Registered LOD groups
    std::vector<LODGroup*> m_groups;
    
    LODManager();
    
public:
    static LODManager* getInstance();
    static void cleanup();
    
    // Global settings
    void setGlobalLODBias(float bias) { m_globalLODBias = bias; }
    float getGlobalLODBias() const { return m_globalLODBias; }
    void setMaxRenderDistance(float distance) { m_maxRenderDistance = distance; }
    void setMaxLODLevel(int level) { m_maxLODLevel = level; }
    void setEnabled(bool enable) { m_enabled = enable; }
    bool isEnabled() const { return m_enabled; }
    
    // Quality presets
    void addPreset(const std::string& name, float bias, float maxDist, int maxLOD);
    void setPreset(const std::string& name);
    void setPreset(int index);
    std::vector<std::string> getPresetNames() const;
    
    // LOD group management
    void registerGroup(LODGroup* group);
    void unregisterGroup(LODGroup* group);
    void updateAllGroups(float deltaTime, const Math::Vector2D& cameraPos, float cameraVelocity = 0.0f);
    
    // Force LOD globally
    void forceGlobalLOD(int level);
    void clearGlobalForcedLOD();
    
    // Statistics
    const LODStats& getStats() const { return m_stats; }
    void resetStats();
    void updateStats(int lodLevel, bool transitioned);
};

/**
 * @brief Impostor generator for billboard LODs
 */
class ImpostorGenerator {
public:
    struct ImpostorConfig {
        int atlasSize;              // Size of impostor atlas texture
        int viewCount;              // Number of view angles to capture
        bool includeNormals;        // Generate normal map
        bool includeMask;           // Generate alpha mask
        float padding;              // Padding between views
    };
    
    static bool generateImpostor(const std::string& meshPath, const std::string& outputPath, 
                                  const ImpostorConfig& config);
    static bool generateOctahedralImpostor(const std::string& meshPath, const std::string& outputPath,
                                            int resolution);
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
