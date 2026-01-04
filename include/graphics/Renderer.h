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

// =============================================================================
// RENDER GRAPH - Modern frame-graph based rendering architecture
// =============================================================================

/**
 * @brief Resource types managed by render graph
 */
enum class RenderResourceType {
    Texture2D,
    TextureCube,
    Texture3D,
    Buffer,
    RenderTarget,
    DepthStencil,
    Sampler,
    UniformBuffer,
    StorageBuffer
};

/**
 * @brief Resource usage flags
 */
enum class ResourceUsage : uint32_t {
    None            = 0,
    ShaderRead      = 1 << 0,
    ShaderWrite     = 1 << 1,
    RenderTarget    = 1 << 2,
    DepthStencil    = 1 << 3,
    CopySource      = 1 << 4,
    CopyDest        = 1 << 5,
    Present         = 1 << 6,
    ComputeRead     = 1 << 7,
    ComputeWrite    = 1 << 8
};
inline ResourceUsage operator|(ResourceUsage a, ResourceUsage b) {
    return static_cast<ResourceUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Render graph resource descriptor
 */
struct RenderResourceDesc {
    std::string name;
    RenderResourceType type{RenderResourceType::Texture2D};
    int width{0};
    int height{0};
    int depth{1};
    int mipLevels{1};
    int arrayLayers{1};
    uint32_t format{0};     // Platform-specific format
    ResourceUsage usage{ResourceUsage::None};
    bool persistent{false}; // Survives across frames
    
    static RenderResourceDesc Texture2D(const std::string& name, int w, int h, uint32_t fmt) {
        RenderResourceDesc desc;
        desc.name = name;
        desc.type = RenderResourceType::Texture2D;
        desc.width = w;
        desc.height = h;
        desc.format = fmt;
        return desc;
    }
    
    static RenderResourceDesc RenderTarget(const std::string& name, int w, int h, uint32_t fmt) {
        RenderResourceDesc desc;
        desc.name = name;
        desc.type = RenderResourceType::RenderTarget;
        desc.width = w;
        desc.height = h;
        desc.format = fmt;
        desc.usage = ResourceUsage::RenderTarget | ResourceUsage::ShaderRead;
        return desc;
    }
    
    static RenderResourceDesc DepthStencil(const std::string& name, int w, int h) {
        RenderResourceDesc desc;
        desc.name = name;
        desc.type = RenderResourceType::DepthStencil;
        desc.width = w;
        desc.height = h;
        desc.usage = ResourceUsage::DepthStencil | ResourceUsage::ShaderRead;
        return desc;
    }
};

/**
 * @brief Handle to render graph resource
 */
struct RenderResourceHandle {
    uint32_t index{UINT32_MAX};
    uint32_t version{0};
    
    bool isValid() const { return index != UINT32_MAX; }
    bool operator==(const RenderResourceHandle& other) const {
        return index == other.index && version == other.version;
    }
};

/**
 * @brief Render pass type
 */
enum class RenderPassType {
    Graphics,       // Regular graphics rendering
    Compute,        // Compute shader dispatch
    Copy,           // Resource copy/blit
    RayTracing,     // Ray tracing (if supported)
    Present         // Final presentation
};

/**
 * @brief Clear value for render targets
 */
struct ClearValue {
    union {
        float color[4];
        struct { float depth; uint8_t stencil; } depthStencil;
    };
    bool clearColor{false};
    bool clearDepth{false};
    bool clearStencil{false};
};

/**
 * @brief Render pass attachment descriptor
 */
struct PassAttachment {
    RenderResourceHandle resource;
    ResourceUsage usage{ResourceUsage::None};
    ClearValue clearValue;
    bool loadPrevious{true};    // Load previous contents
    bool storeResult{true};     // Store result for later passes
};

/**
 * @brief Render pass descriptor
 */
struct RenderPassDesc {
    std::string name;
    RenderPassType type{RenderPassType::Graphics};
    std::vector<PassAttachment> colorAttachments;
    PassAttachment depthStencilAttachment;
    std::vector<RenderResourceHandle> shaderInputs;
    std::vector<RenderResourceHandle> shaderOutputs;
    int queueFamily{0};         // GPU queue to execute on
    bool async{false};          // Can run async with other passes
};

/**
 * @brief Render pass execution callback
 */
using RenderPassCallback = std::function<void(class RenderGraphBuilder&, void* userData)>;

/**
 * @brief Render pass node in the graph
 */
class RenderPass {
public:
    RenderPass(const std::string& name, RenderPassType type)
        : m_name(name), m_type(type) {}
    virtual ~RenderPass() = default;
    
    const std::string& getName() const { return m_name; }
    RenderPassType getType() const { return m_type; }
    
    // Resource declarations
    void addColorOutput(RenderResourceHandle resource, const ClearValue& clear = {});
    void setDepthStencilOutput(RenderResourceHandle resource, const ClearValue& clear = {});
    void addShaderInput(RenderResourceHandle resource);
    void addShaderOutput(RenderResourceHandle resource);
    
    // Configuration
    void setAsync(bool async) { m_async = async; }
    bool isAsync() const { return m_async; }
    void setQueueFamily(int queue) { m_queueFamily = queue; }
    int getQueueFamily() const { return m_queueFamily; }
    
    // Execution
    void setExecuteCallback(RenderPassCallback callback) { m_callback = callback; }
    void execute(class RenderGraphBuilder& builder, void* userData);
    
    // Dependencies
    const std::vector<RenderResourceHandle>& getInputs() const { return m_inputs; }
    const std::vector<RenderResourceHandle>& getOutputs() const { return m_outputs; }
    
protected:
    std::string m_name;
    RenderPassType m_type;
    std::vector<RenderResourceHandle> m_inputs;
    std::vector<RenderResourceHandle> m_outputs;
    std::vector<PassAttachment> m_colorOutputs;
    PassAttachment m_depthStencilOutput;
    bool m_async{false};
    int m_queueFamily{0};
    RenderPassCallback m_callback;
};

/**
 * @brief Render graph builder for constructing frame graph
 */
class RenderGraphBuilder {
public:
    RenderGraphBuilder() = default;
    ~RenderGraphBuilder() = default;
    
    // Resource creation
    RenderResourceHandle createTexture(const RenderResourceDesc& desc);
    RenderResourceHandle createRenderTarget(const std::string& name, int width, int height, uint32_t format);
    RenderResourceHandle createDepthStencil(const std::string& name, int width, int height);
    RenderResourceHandle importResource(const std::string& name, void* externalResource, const RenderResourceDesc& desc);
    
    // Pass creation
    RenderPass& addPass(const std::string& name, RenderPassType type = RenderPassType::Graphics);
    
    // Graph construction
    void setBackbuffer(RenderResourceHandle resource);
    void compile();
    bool isCompiled() const { return m_compiled; }
    
    // Execution
    void execute(void* userData = nullptr);
    
    // Resource access (during pass execution)
    void* getResource(RenderResourceHandle handle);
    RenderResourceDesc getResourceDesc(RenderResourceHandle handle) const;
    bool isResourceValid(RenderResourceHandle handle) const;
    
    // Optimization info
    struct OptimizationInfo {
        size_t totalPasses;
        size_t culledPasses;
        size_t mergedPasses;
        size_t asyncPasses;
        size_t resourceAliases;
        size_t peakMemoryUsage;
    };
    OptimizationInfo getOptimizationInfo() const { return m_optimizationInfo; }
    
    // Debug
    std::string exportToDot() const;    // Export to GraphViz format
    void dumpGraph() const;
    
private:
    struct ResourceSlot {
        RenderResourceDesc desc;
        void* resource{nullptr};
        uint32_t version{0};
        int firstUse{-1};
        int lastUse{-1};
        bool imported{false};
    };
    
    std::vector<ResourceSlot> m_resources;
    std::vector<std::unique_ptr<RenderPass>> m_passes;
    std::vector<int> m_executionOrder;
    RenderResourceHandle m_backbuffer;
    bool m_compiled{false};
    OptimizationInfo m_optimizationInfo{};
    
    // Compilation
    void buildDependencyGraph();
    void topologicalSort();
    void cullUnusedPasses();
    void computeResourceLifetimes();
    void aliasResources();
    void insertBarriers();
};

/**
 * @brief Render graph for frame-based rendering
 */
class RenderGraph {
public:
    RenderGraph() = default;
    ~RenderGraph() = default;
    
    // Setup
    void initialize(int backbufferWidth, int backbufferHeight);
    void shutdown();
    void resize(int width, int height);
    
    // Graph building
    RenderGraphBuilder& beginFrame();
    void endFrame();
    
    // Preset passes
    void addGBufferPass(RenderGraphBuilder& builder, const std::string& name);
    void addLightingPass(RenderGraphBuilder& builder, const std::string& name,
                         RenderResourceHandle gbufferAlbedo,
                         RenderResourceHandle gbufferNormal,
                         RenderResourceHandle gbufferDepth);
    void addShadowPass(RenderGraphBuilder& builder, const std::string& name, int cascadeIndex);
    void addSSAOPass(RenderGraphBuilder& builder, const std::string& name,
                     RenderResourceHandle depthBuffer, RenderResourceHandle normalBuffer);
    void addBloomPass(RenderGraphBuilder& builder, const std::string& name,
                      RenderResourceHandle hdrInput);
    void addTonemapPass(RenderGraphBuilder& builder, const std::string& name,
                        RenderResourceHandle hdrInput, RenderResourceHandle output);
    void addFXAAPass(RenderGraphBuilder& builder, const std::string& name,
                     RenderResourceHandle input, RenderResourceHandle output);
    
    // Statistics
    struct FrameStats {
        float graphBuildTime;
        float graphCompileTime;
        float graphExecuteTime;
        size_t passCount;
        size_t resourceCount;
        size_t memoryUsed;
    };
    FrameStats getFrameStats() const { return m_frameStats; }
    
    // Debug
    void enableDebugLabels(bool enable) { m_debugLabels = enable; }
    void setProfilingEnabled(bool enable) { m_profiling = enable; }
    
private:
    std::unique_ptr<RenderGraphBuilder> m_currentBuilder;
    int m_backbufferWidth{0};
    int m_backbufferHeight{0};
    FrameStats m_frameStats{};
    bool m_debugLabels{false};
    bool m_profiling{false};
};

/**
 * @brief Resource barrier manager for render graph
 */
class ResourceBarrierManager {
public:
    enum class BarrierType {
        Transition,     // Resource state transition
        UAV,            // Unordered access view barrier
        Aliasing        // Resource aliasing barrier
    };
    
    struct Barrier {
        BarrierType type;
        RenderResourceHandle resource;
        ResourceUsage beforeUsage;
        ResourceUsage afterUsage;
    };
    
    void addBarrier(const Barrier& barrier);
    void flushBarriers();
    void optimizeBarriers();
    
    const std::vector<Barrier>& getPendingBarriers() const { return m_pending; }
    void clearPendingBarriers() { m_pending.clear(); }
    
private:
    std::vector<Barrier> m_pending;
};

/**
 * @brief Transient resource allocator for render graph
 */
class TransientResourceAllocator {
public:
    TransientResourceAllocator(size_t poolSize = 256 * 1024 * 1024);  // 256MB default
    ~TransientResourceAllocator();
    
    // Allocation
    void* allocate(const RenderResourceDesc& desc);
    void free(void* resource);
    void reset();  // Call at frame start
    
    // Memory info
    size_t getUsedMemory() const { return m_usedMemory; }
    size_t getPoolSize() const { return m_poolSize; }
    size_t getPeakMemory() const { return m_peakMemory; }
    
    // Aliasing support
    void* getAliasedResource(const RenderResourceDesc& desc, int frameLifetime);
    bool canAlias(const RenderResourceDesc& a, const RenderResourceDesc& b) const;
    
private:
    struct MemoryBlock {
        void* memory;
        size_t size;
        size_t offset;
        bool inUse;
        int lastUsedFrame;
    };
    
    std::vector<MemoryBlock> m_blocks;
    size_t m_poolSize;
    size_t m_usedMemory{0};
    size_t m_peakMemory{0};
    int m_currentFrame{0};
};

} // namespace Graphics
} // namespace JJM

#endif // RENDERER_H
