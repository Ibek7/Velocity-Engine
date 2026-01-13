/**
 * @file ShaderSystem.h
 * @brief Advanced shader management and variant system
 * @version 1.0.0
 * @date 2026-01-12
 */

#ifndef SHADER_SYSTEM_H
#define SHADER_SYSTEM_H

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <set>
#include <functional>
#include <bitset>
#include <optional>

namespace JJM {
namespace Graphics {

// =============================================================================
// Shader Variant System
// =============================================================================

/**
 * @brief Shader feature flags for variants
 */
enum class ShaderFeature : uint32_t {
    None = 0,
    Skinning = 1 << 0,
    NormalMapping = 1 << 1,
    ParallaxMapping = 1 << 2,
    Shadows = 1 << 3,
    SoftShadows = 1 << 4,
    AmbientOcclusion = 1 << 5,
    ScreenSpaceReflections = 1 << 6,
    GlobalIllumination = 1 << 7,
    HDR = 1 << 8,
    Bloom = 1 << 9,
    DepthOfField = 1 << 10,
    MotionBlur = 1 << 11,
    Fog = 1 << 12,
    Instancing = 1 << 13,
    Tessellation = 1 << 14,
    Wireframe = 1 << 15,
    AlphaTest = 1 << 16,
    AlphaBlend = 1 << 17,
    DoubleSided = 1 << 18,
    VertexColors = 1 << 19,
    UV2 = 1 << 20,
    Lightmapping = 1 << 21,
    RealtimeLighting = 1 << 22,
    PointLights = 1 << 23,
    SpotLights = 1 << 24,
    AreaLights = 1 << 25,
    GPU_Particles = 1 << 26,
    Compute = 1 << 27,
    Custom1 = 1 << 28,
    Custom2 = 1 << 29,
    Custom3 = 1 << 30,
    Custom4 = 1U << 31
};

inline ShaderFeature operator|(ShaderFeature a, ShaderFeature b) {
    return static_cast<ShaderFeature>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ShaderFeature operator&(ShaderFeature a, ShaderFeature b) {
    return static_cast<ShaderFeature>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

using ShaderFeatureSet = uint32_t;

/**
 * @brief Shader variant key
 */
struct ShaderVariantKey {
    ShaderFeatureSet features;
    std::string defines;            // Additional preprocessor defines
    int qualityLevel;               // 0-3 (low, medium, high, ultra)
    
    ShaderVariantKey()
        : features(0)
        , qualityLevel(2)
    {}
    
    bool operator==(const ShaderVariantKey& other) const {
        return features == other.features && 
               defines == other.defines && 
               qualityLevel == other.qualityLevel;
    }
    
    size_t hash() const;
};

/**
 * @brief Hash function for ShaderVariantKey
 */
struct ShaderVariantKeyHash {
    size_t operator()(const ShaderVariantKey& key) const {
        return key.hash();
    }
};

/**
 * @brief Shader compilation error information
 */
struct ShaderError {
    int lineNumber;
    int columnNumber;
    std::string message;
    std::string sourceLine;
    std::string severity;  // "error", "warning", "info"
    
    ShaderError() : lineNumber(-1), columnNumber(-1), severity("error") {}
};

/**
 * @brief Shader compilation result
 */
struct ShaderCompileResult {
    bool success;
    std::string errorMessage;
    std::vector<std::string> warnings;
    std::vector<ShaderError> detailedErrors;  // Parsed error information with line numbers
    float compileTimeMs;
    size_t binarySize;
    
    ShaderCompileResult() : success(false), compileTimeMs(0.0f), binarySize(0) {}
    
    /**
     * @brief Get formatted error report with line numbers and context
     */
    std::string getFormattedErrors() const;
};

/**
 * @brief Shader variant with specific feature combination
 */
class ShaderVariant {
private:
    ShaderVariantKey m_key;
    unsigned int m_program;
    std::vector<uint8_t> m_binary;
    bool m_compiled;
    std::chrono::steady_clock::time_point m_lastUsed;
    
    // Cached uniform locations
    std::unordered_map<std::string, int> m_uniformCache;
    
    // Uniform buffer object support
    std::unordered_map<std::string, unsigned int> m_uniformBlockCache;
    
public:
    ShaderVariant(const ShaderVariantKey& key);
    ~ShaderVariant();
    
    bool compile(const std::string& vertexSource, const std::string& fragmentSource,
                 ShaderCompileResult& result);
    bool loadFromBinary(const std::vector<uint8_t>& binary);
    std::vector<uint8_t> getBinary() const;
    
    void use() const;
    void unuse() const;
    
    int getUniformLocation(const std::string& name);
    unsigned int getUniformBlockIndex(const std::string& name);
    void bindUniformBlock(const std::string& name, unsigned int bindingPoint);
    
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, float x, float y);
    void setUniform(const std::string& name, float x, float y, float z);
    void setUniform(const std::string& name, float x, float y, float z, float w);
    void setUniformMatrix(const std::string& name, const float* matrix);
    void setUniformArray(const std::string& name, const float* values, int count);
    
    const ShaderVariantKey& getKey() const { return m_key; }
    unsigned int getProgram() const { return m_program; }
    bool isCompiled() const { return m_compiled; }
    void markUsed() { m_lastUsed = std::chrono::steady_clock::now(); }
    std::chrono::steady_clock::time_point getLastUsed() const { return m_lastUsed; }
};

/**
 * @brief Multi-pass shader configuration
 */
struct ShaderPass {
    std::string name;
    std::string vertexEntry;
    std::string fragmentEntry;
    ShaderFeatureSet requiredFeatures;
    
    // Render state
    bool depthWrite;
    bool depthTest;
    int cullMode;                   // 0 = none, 1 = back, 2 = front
    int blendMode;                  // 0 = opaque, 1 = alpha, 2 = additive
    int stencilOp;
    
    std::unordered_map<std::string, std::string> defines;
    
    ShaderPass()
        : depthWrite(true)
        , depthTest(true)
        , cullMode(1)
        , blendMode(0)
        , stencilOp(0)
    {}
};

class Shader {
public:
    Shader();
    ~Shader();
    
    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromStrings(const std::string& vertexSrc, const std::string& fragmentSrc);
    bool compile();
    
    void use() const;
    void unuse() const;
    
    // Uniform setters
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, float x, float y);
    void setUniform(const std::string& name, float x, float y, float z);
    void setUniform(const std::string& name, float x, float y, float z, float w);
    void setUniformMatrix(const std::string& name, const float* matrix);
    
    unsigned int getProgram() const { return program; }
    bool isValid() const { return program != 0; }
    
    // Variant system
    ShaderVariant* getVariant(const ShaderVariantKey& key);
    ShaderVariant* getVariant(ShaderFeatureSet features);
    void precompileVariant(const ShaderVariantKey& key);
    void precompileVariants(const std::vector<ShaderVariantKey>& keys);
    void clearVariantCache();
    void setMaxCachedVariants(size_t max) { m_maxCachedVariants = max; }
    
    // Multi-pass support
    void addPass(const ShaderPass& pass);
    const ShaderPass* getPass(const std::string& name) const;
    const std::vector<ShaderPass>& getPasses() const { return m_passes; }
    void removePass(const std::string& name);
    
    // Source management
    const std::string& getVertexSource() const { return vertexSource; }
    const std::string& getFragmentSource() const { return fragmentSource; }
    
private:
    unsigned int program;
    unsigned int vertexShader;
    unsigned int fragmentShader;
    std::string vertexSource;
    std::string fragmentSource;
    std::unordered_map<std::string, int> uniformCache;
    
    // Variant cache
    std::unordered_map<ShaderVariantKey, std::unique_ptr<ShaderVariant>, ShaderVariantKeyHash> m_variants;
    size_t m_maxCachedVariants;
    
    // Multi-pass
    std::vector<ShaderPass> m_passes;
    
    int getUniformLocation(const std::string& name);
    bool compileShader(unsigned int shader, const std::string& source);
    bool linkProgram();
    
    std::string preprocessSource(const std::string& source, const ShaderVariantKey& key) const;
    void evictLRUVariants();
};

/**
 * @brief Shader include resolver
 */
class ShaderIncludeResolver {
private:
    std::vector<std::string> m_includePaths;
    std::unordered_map<std::string, std::string> m_includeCache;
    
public:
    void addIncludePath(const std::string& path);
    void removeIncludePath(const std::string& path);
    void clearIncludePaths();
    
    std::string resolve(const std::string& source, int maxDepth = 10);
    void clearCache();
    
private:
    std::string loadInclude(const std::string& name);
    std::string processIncludes(const std::string& source, std::set<std::string>& included, int depth);
};

/**
 * @brief Shader permutation generator
 */
class ShaderPermutationGenerator {
public:
    struct PermutationAxis {
        std::string name;
        std::vector<std::string> values;
        bool required;
    };
    
    void addAxis(const std::string& name, const std::vector<std::string>& values, bool required = true);
    void removeAxis(const std::string& name);
    void clearAxes();
    
    std::vector<std::unordered_map<std::string, std::string>> generatePermutations() const;
    size_t getPermutationCount() const;
    
    // Generate shader variant keys from permutations
    std::vector<ShaderVariantKey> generateVariantKeys(const std::unordered_map<std::string, ShaderFeature>& featureMap) const;
    
private:
    std::vector<PermutationAxis> m_axes;
};

class ShaderLibrary {
public:
    static ShaderLibrary& getInstance();
    
    bool loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    std::shared_ptr<Shader> getShader(const std::string& name);
    void reloadAll();
    
    // Variant management
    void precompileAllVariants(const std::vector<ShaderVariantKey>& commonVariants);
    void clearAllVariantCaches();
    void setGlobalFeatures(ShaderFeatureSet features);
    ShaderFeatureSet getGlobalFeatures() const { return m_globalFeatures; }
    
    // Variant compilation queue for background compilation
    struct VariantCompileRequest {
        std::string shaderName;
        ShaderVariantKey variantKey;
        int priority{0};
    };
    void queueVariantCompilation(const VariantCompileRequest& request);
    void queueVariantCompilations(const std::vector<VariantCompileRequest>& requests);
    size_t getPendingVariantCount() const;
    void processPendingVariants(int maxPerFrame = 1);
    
    // Include system
    ShaderIncludeResolver& getIncludeResolver() { return m_includeResolver; }
    
    // Binary cache
    void enableBinaryCache(bool enable, const std::string& cachePath = "");
    bool loadBinaryCache();
    bool saveBinaryCache();
    
    // Hot-reload system
    void enableHotReload(bool enable);
    bool isHotReloadEnabled() const { return m_hotReloadEnabled; }
    void checkForChanges();  // Call each frame to detect shader file modifications
    void reloadShader(const std::string& name);  // Manually reload a specific shader
    
    // Statistics
    size_t getTotalVariantCount() const;
    size_t getCachedBinarySize() const;
    
private:
    ShaderLibrary() = default;
    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
    std::unordered_map<std::string, std::pair<std::string, std::string>> shaderPaths;
    
    ShaderFeatureSet m_globalFeatures;
    ShaderIncludeResolver m_includeResolver;
    
    bool m_binaryCacheEnabled;
    std::string m_binaryCachePath;
    std::unordered_map<size_t, std::vector<uint8_t>> m_binaryCache;
    
    // Hot-reload tracking
    bool m_hotReloadEnabled;
    std::unordered_map<std::string, std::time_t> m_fileModificationTimes;
    std::time_t getFileModificationTime(const std::string& filePath) const;
};

// =============================================================================
// SHADER HOT-RELOAD SYSTEM
// =============================================================================

/**
 * @brief File change event types
 */
enum class ShaderFileEvent {
    Created,
    Modified,
    Deleted,
    Renamed
};

/**
 * @brief Shader file change notification
 */
struct ShaderFileChange {
    std::string filePath;
    ShaderFileEvent event;
    std::chrono::steady_clock::time_point timestamp;
    std::vector<std::string> affectedShaders;
};

/**
 * @brief Hot-reload configuration
 */
struct HotReloadConfig {
    bool enabled{true};
    float pollIntervalMs{100.0f};
    float debounceTimeMs{200.0f};       // Wait for file to settle
    bool recompileOnError{true};        // Keep trying on compile errors
    bool notifyOnReload{true};
    bool backupOnReload{false};
    
    // Error handling
    int maxRetries{3};
    float retryDelayMs{500.0f};
};

/**
 * @brief Shader hot-reload manager
 */
class ShaderHotReloadManager {
public:
    static ShaderHotReloadManager* getInstance();
    static void destroyInstance();
    
    // Configuration
    void setConfig(const HotReloadConfig& config);
    const HotReloadConfig& getConfig() const { return m_config; }
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_config.enabled; }
    
    // Watch paths
    void addWatchPath(const std::string& path);
    void removeWatchPath(const std::string& path);
    void clearWatchPaths();
    std::vector<std::string> getWatchPaths() const;
    
    // Shader registration
    void registerShader(const std::string& name, Shader* shader,
                        const std::string& vertPath, const std::string& fragPath);
    void unregisterShader(const std::string& name);
    void registerIncludeFile(const std::string& includePath, const std::vector<std::string>& dependentShaders);
    
    // Update (call each frame)
    void update();
    void forceReload(const std::string& shaderName);
    void forceReloadAll();
    
    // Callbacks
    using ReloadCallback = std::function<void(const std::string& shaderName, bool success)>;
    using ErrorCallback = std::function<void(const std::string& shaderName, const std::string& error)>;
    
    void setReloadCallback(ReloadCallback callback) { m_reloadCallback = callback; }
    void setErrorCallback(ErrorCallback callback) { m_errorCallback = callback; }
    
    // Statistics
    struct HotReloadStats {
        size_t totalReloads;
        size_t successfulReloads;
        size_t failedReloads;
        float lastReloadTime;
        std::string lastReloadedShader;
        std::vector<std::string> pendingReloads;
    };
    HotReloadStats getStatistics() const;
    
private:
    static ShaderHotReloadManager* s_instance;
    
    HotReloadConfig m_config;
    
    struct WatchedShader {
        Shader* shader;
        std::string vertexPath;
        std::string fragmentPath;
        std::chrono::steady_clock::time_point lastVertexModTime;
        std::chrono::steady_clock::time_point lastFragmentModTime;
        int retryCount;
    };
    
    std::unordered_map<std::string, WatchedShader> m_watchedShaders;
    std::unordered_map<std::string, std::vector<std::string>> m_includeDependencies;
    std::vector<std::string> m_watchPaths;
    
    std::vector<ShaderFileChange> m_pendingChanges;
    std::chrono::steady_clock::time_point m_lastPollTime;
    
    ReloadCallback m_reloadCallback;
    ErrorCallback m_errorCallback;
    
    HotReloadStats m_stats{};
    
    ShaderHotReloadManager();
    ~ShaderHotReloadManager();
    
    void pollFileChanges();
    void processChanges();
    bool reloadShader(const std::string& name, WatchedShader& watched);
    std::chrono::steady_clock::time_point getFileModTime(const std::string& path);
    std::vector<std::string> findAffectedShaders(const std::string& changedFile);
    
    // Async compilation support for hot-reload
    void reloadAsync(const std::string& name);
    AsyncShaderCompiler* m_asyncCompiler{nullptr};
    bool m_asyncEnabled{false};
};

// =============================================================================
// ASYNCHRONOUS SHADER COMPILATION
// =============================================================================

/**
 * @brief Async compilation request
 */
struct AsyncCompileRequest {
    int requestId;
    std::string shaderName;
    ShaderVariantKey variantKey;
    std::string vertexSource;
    std::string fragmentSource;
    int priority;
    std::function<void(bool success, const ShaderCompileResult&)> callback;
};

/**
 * @brief Asynchronous shader compiler
 */
class AsyncShaderCompiler {
public:
    AsyncShaderCompiler(int threadCount = 2);
    ~AsyncShaderCompiler();
    
    // Submission
    int submitCompile(const std::string& shaderName,
                      const ShaderVariantKey& key,
                      const std::string& vertexSrc,
                      const std::string& fragmentSrc,
                      std::function<void(bool, const ShaderCompileResult&)> callback,
                      int priority = 0);
    
    int submitBatchCompile(const std::vector<AsyncCompileRequest>& requests);
    
    // Status
    bool isCompiling() const { return m_pendingCount > 0; }
    size_t getPendingCount() const { return m_pendingCount; }
    void cancelRequest(int requestId);
    void cancelAll();
    
    // Process completed compilations (call from main thread)
    void processCompleted();
    
    // Wait for specific or all compilations
    void waitForRequest(int requestId);
    void waitForAll();
    
    // Statistics
    struct CompileStats {
        size_t totalCompilations;
        size_t successfulCompilations;
        size_t failedCompilations;
        float averageCompileTimeMs;
        float maxCompileTimeMs;
        size_t currentQueueSize;
    };
    CompileStats getStatistics() const;
    
private:
    struct CompileJob {
        AsyncCompileRequest request;
        bool cancelled{false};
    };
    
    struct CompletedJob {
        int requestId;
        bool success;
        ShaderCompileResult result;
        std::function<void(bool, const ShaderCompileResult&)> callback;
    };
    
    std::vector<std::thread> m_workers;
    std::priority_queue<CompileJob, std::vector<CompileJob>,
                        std::function<bool(const CompileJob&, const CompileJob&)>> m_queue;
    std::vector<CompletedJob> m_completed;
    
    std::mutex m_queueMutex;
    std::mutex m_completedMutex;
    std::condition_variable m_queueCondition;
    
    std::atomic<bool> m_shutdown{false};
    std::atomic<size_t> m_pendingCount{0};
    int m_nextRequestId{0};
    
    mutable CompileStats m_stats{};
    
    void workerThread();
};

// =============================================================================
// SHADER REFLECTION
// =============================================================================

/**
 * @brief Uniform types
 */
enum class UniformType {
    Unknown,
    Bool,
    Int,
    UInt,
    Float,
    Double,
    Vec2,
    Vec3,
    Vec4,
    IVec2,
    IVec3,
    IVec4,
    UVec2,
    UVec3,
    UVec4,
    Mat2,
    Mat3,
    Mat4,
    Mat2x3,
    Mat2x4,
    Mat3x2,
    Mat3x4,
    Mat4x2,
    Mat4x3,
    Sampler1D,
    Sampler2D,
    Sampler3D,
    SamplerCube,
    Sampler2DArray,
    SamplerCubeArray,
    Image2D,
    Image3D,
    StorageBuffer,
    UniformBuffer
};

/**
 * @brief Reflected uniform info
 */
struct UniformInfo {
    std::string name;
    UniformType type{UniformType::Unknown};
    int location{-1};
    int binding{-1};
    int arraySize{1};
    int offset{0};              // For uniform buffer members
    int size{0};                // Size in bytes
    std::string blockName;      // Parent block name (empty if not in block)
};

/**
 * @brief Uniform buffer info
 */
struct UniformBlockInfo {
    std::string name;
    int binding{-1};
    int size{0};
    std::vector<UniformInfo> members;
};

/**
 * @brief Shader storage buffer info
 */
struct StorageBlockInfo {
    std::string name;
    int binding{-1};
    int size{0};
    bool readable{true};
    bool writable{true};
    std::vector<UniformInfo> members;
};

/**
 * @brief Vertex attribute info
 */
struct AttributeInfo {
    std::string name;
    UniformType type{UniformType::Unknown};
    int location{-1};
    int arraySize{1};
};

/**
 * @brief Shader reflection data
 */
struct ShaderReflection {
    // Uniforms
    std::vector<UniformInfo> uniforms;
    std::vector<UniformBlockInfo> uniformBlocks;
    std::vector<StorageBlockInfo> storageBlocks;
    
    // Attributes
    std::vector<AttributeInfo> attributes;
    
    // Samplers and images
    std::vector<UniformInfo> samplers;
    std::vector<UniformInfo> images;
    
    // Compute shader info
    int computeWorkGroupSizeX{0};
    int computeWorkGroupSizeY{0};
    int computeWorkGroupSizeZ{0};
    
    // Lookup helpers
    const UniformInfo* findUniform(const std::string& name) const;
    const UniformBlockInfo* findUniformBlock(const std::string& name) const;
    const AttributeInfo* findAttribute(const std::string& name) const;
    int getUniformLocation(const std::string& name) const;
    int getUniformBlockBinding(const std::string& name) const;
};

/**
 * @brief Shader reflection utility
 */
class ShaderReflector {
public:
    static ShaderReflection reflect(unsigned int program);
    static ShaderReflection reflectFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);
    
    // Type conversion utilities
    static UniformType glTypeToUniformType(unsigned int glType);
    static std::string uniformTypeToString(UniformType type);
    static int getUniformTypeSize(UniformType type);
    static int getUniformTypeComponents(UniformType type);
    
    // Validation
    static bool validateInterface(const ShaderReflection& vertexShader, const ShaderReflection& fragmentShader);
    static std::vector<std::string> findUnusedUniforms(const ShaderReflection& reflection,
                                                       const std::set<std::string>& usedUniforms);
};

// =============================================================================
// COMPUTE SHADER SUPPORT
// =============================================================================

/**
 * @brief Compute shader dispatch info
 */
struct ComputeDispatch {
    uint32_t groupsX{1};
    uint32_t groupsY{1};
    uint32_t groupsZ{1};
    
    // Indirect dispatch
    bool indirect{false};
    unsigned int indirectBuffer{0};
    size_t indirectOffset{0};
};

/**
 * @brief Memory barrier types
 */
enum class MemoryBarrier : uint32_t {
    None = 0,
    VertexAttrib = 1 << 0,
    ElementArray = 1 << 1,
    Uniform = 1 << 2,
    TextureFetch = 1 << 3,
    ShaderImageAccess = 1 << 4,
    Command = 1 << 5,
    PixelBuffer = 1 << 6,
    TextureUpdate = 1 << 7,
    BufferUpdate = 1 << 8,
    Framebuffer = 1 << 9,
    TransformFeedback = 1 << 10,
    AtomicCounter = 1 << 11,
    ShaderStorage = 1 << 12,
    All = 0xFFFFFFFF
};

inline MemoryBarrier operator|(MemoryBarrier a, MemoryBarrier b) {
    return static_cast<MemoryBarrier>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Compute shader class
 */
class ComputeShader {
public:
    ComputeShader();
    ~ComputeShader();
    
    // Loading
    bool loadFromFile(const std::string& path);
    bool loadFromString(const std::string& source);
    bool compile(ShaderCompileResult* result = nullptr);
    
    // Execution
    void use() const;
    void unuse() const;
    void dispatch(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1);
    void dispatch(const ComputeDispatch& dispatch);
    void dispatchIndirect(unsigned int buffer, size_t offset = 0);
    
    // Barriers
    static void memoryBarrier(MemoryBarrier barriers);
    static void memoryBarrierByRegion(MemoryBarrier barriers);
    
    // Uniforms
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, float x, float y, float z);
    void setUniform(const std::string& name, float x, float y, float z, float w);
    void setUniformMatrix(const std::string& name, const float* matrix);
    
    // Buffers
    void bindStorageBuffer(unsigned int buffer, int binding);
    void bindUniformBuffer(unsigned int buffer, int binding);
    void bindImage(unsigned int texture, int unit, int access, int format);
    
    // Queries
    unsigned int getProgram() const { return m_program; }
    bool isValid() const { return m_program != 0; }
    const ShaderReflection& getReflection() const { return m_reflection; }
    void getWorkGroupSize(int& x, int& y, int& z) const;
    
    // Source
    const std::string& getSource() const { return m_source; }
    const std::string& getPath() const { return m_path; }
    
private:
    unsigned int m_program{0};
    unsigned int m_shader{0};
    std::string m_source;
    std::string m_path;
    ShaderReflection m_reflection;
    std::unordered_map<std::string, int> m_uniformCache;
    
    int getUniformLocation(const std::string& name);
};

// =============================================================================
// PIPELINE STATE OBJECTS
// =============================================================================

/**
 * @brief Blend factors
 */
enum class BlendFactor {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    SrcAlphaSaturate
};

/**
 * @brief Blend operations
 */
enum class BlendOp {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

/**
 * @brief Comparison functions
 */
enum class CompareFunc {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

/**
 * @brief Stencil operations
 */
enum class StencilOp {
    Keep,
    Zero,
    Replace,
    Increment,
    IncrementWrap,
    Decrement,
    DecrementWrap,
    Invert
};

/**
 * @brief Cull modes
 */
enum class CullMode {
    None,
    Front,
    Back,
    FrontAndBack
};

/**
 * @brief Fill modes
 */
enum class FillMode {
    Solid,
    Wireframe,
    Point
};

/**
 * @brief Blend state configuration
 */
struct BlendState {
    bool enabled{false};
    BlendFactor srcColorFactor{BlendFactor::One};
    BlendFactor dstColorFactor{BlendFactor::Zero};
    BlendOp colorOp{BlendOp::Add};
    BlendFactor srcAlphaFactor{BlendFactor::One};
    BlendFactor dstAlphaFactor{BlendFactor::Zero};
    BlendOp alphaOp{BlendOp::Add};
    float constantColor[4]{0.0f, 0.0f, 0.0f, 0.0f};
    uint8_t colorWriteMask{0x0F};    // RGBA bits
    
    static BlendState Opaque();
    static BlendState AlphaBlend();
    static BlendState Additive();
    static BlendState Multiply();
    static BlendState Premultiplied();
};

/**
 * @brief Depth-stencil state configuration
 */
struct DepthStencilState {
    // Depth
    bool depthTestEnabled{true};
    bool depthWriteEnabled{true};
    CompareFunc depthFunc{CompareFunc::Less};
    
    // Stencil
    bool stencilEnabled{false};
    uint8_t stencilReadMask{0xFF};
    uint8_t stencilWriteMask{0xFF};
    
    struct StencilFaceState {
        StencilOp failOp{StencilOp::Keep};
        StencilOp depthFailOp{StencilOp::Keep};
        StencilOp passOp{StencilOp::Keep};
        CompareFunc func{CompareFunc::Always};
    };
    StencilFaceState frontFace;
    StencilFaceState backFace;
    int stencilRef{0};
    
    static DepthStencilState Default();
    static DepthStencilState DepthReadOnly();
    static DepthStencilState NoDepth();
};

/**
 * @brief Rasterizer state configuration
 */
struct RasterizerState {
    CullMode cullMode{CullMode::Back};
    FillMode fillMode{FillMode::Solid};
    bool frontCounterClockwise{false};
    bool scissorEnabled{false};
    bool depthClipEnabled{true};
    bool multisampleEnabled{false};
    bool antialiasedLineEnabled{false};
    float depthBias{0.0f};
    float depthBiasClamp{0.0f};
    float slopeScaledDepthBias{0.0f};
    float lineWidth{1.0f};
    float pointSize{1.0f};
    
    static RasterizerState Default();
    static RasterizerState NoCull();
    static RasterizerState Wireframe();
    static RasterizerState Shadow();
};

/**
 * @brief Complete pipeline state
 */
struct PipelineState {
    Shader* shader{nullptr};
    ShaderVariantKey variantKey;
    BlendState blendState;
    DepthStencilState depthStencilState;
    RasterizerState rasterizerState;
    int primitiveType{0};           // GL_TRIANGLES, etc.
    
    size_t hash() const;
    bool operator==(const PipelineState& other) const;
};

/**
 * @brief Pipeline state hash
 */
struct PipelineStateHash {
    size_t operator()(const PipelineState& state) const {
        return state.hash();
    }
};

/**
 * @brief Compiled pipeline state object
 */
class PipelineStateObject {
public:
    PipelineStateObject(const PipelineState& state);
    ~PipelineStateObject();
    
    void bind() const;
    const PipelineState& getState() const { return m_state; }
    
private:
    PipelineState m_state;
    
    void applyBlendState() const;
    void applyDepthStencilState() const;
    void applyRasterizerState() const;
};

/**
 * @brief Pipeline state cache
 */
class PipelineStateCache {
public:
    static PipelineStateCache* getInstance();
    static void destroyInstance();
    
    // Get or create PSO
    PipelineStateObject* getPSO(const PipelineState& state);
    
    // Presets
    PipelineStateObject* getOpaquePSO(Shader* shader, const ShaderVariantKey& key = {});
    PipelineStateObject* getTransparentPSO(Shader* shader, const ShaderVariantKey& key = {});
    PipelineStateObject* getAdditivePSO(Shader* shader, const ShaderVariantKey& key = {});
    PipelineStateObject* getShadowPSO(Shader* shader);
    
    // Management
    void clear();
    void evictUnused(float maxIdleTimeSeconds);
    size_t getCacheSize() const { return m_cache.size(); }
    
private:
    static PipelineStateCache* s_instance;
    
    struct CachedPSO {
        std::unique_ptr<PipelineStateObject> pso;
        std::chrono::steady_clock::time_point lastUsed;
    };
    
    std::unordered_map<PipelineState, CachedPSO, PipelineStateHash> m_cache;
    
    PipelineStateCache() = default;
};

// =============================================================================
// SHADER PREPROCESSING
// =============================================================================

/**
 * @brief Shader preprocessor directives
 */
class ShaderPreprocessor {
public:
    ShaderPreprocessor();
    
    // Define management
    void define(const std::string& name);
    void define(const std::string& name, const std::string& value);
    void define(const std::string& name, int value);
    void define(const std::string& name, float value);
    void undefine(const std::string& name);
    void clearDefines();
    
    // Global defines (applied to all shaders)
    static void defineGlobal(const std::string& name, const std::string& value = "");
    static void undefineGlobal(const std::string& name);
    static void clearGlobalDefines();
    
    // Processing
    std::string process(const std::string& source, std::vector<std::string>* errors = nullptr);
    
    // Include resolver
    void setIncludeResolver(ShaderIncludeResolver* resolver) { m_resolver = resolver; }
    
    // Feature flags to defines
    std::unordered_map<std::string, std::string> featuresToDefines(ShaderFeatureSet features);
    
    // Macro expansion with parameters
    struct MacroDefinition {
        std::string name;
        std::vector<std::string> parameters;
        std::string body;
        bool hasParameters;
    };
    
    void defineMacro(const MacroDefinition& macro);
    void defineMacro(const std::string& name, const std::string& body);
    void defineMacro(const std::string& name, const std::vector<std::string>& params, const std::string& body);
    
private:
    std::unordered_map<std::string, std::string> m_defines;
    std::unordered_map<std::string, MacroDefinition> m_macros;
    static std::unordered_map<std::string, std::string> s_globalDefines;
    ShaderIncludeResolver* m_resolver{nullptr};
    
    std::string evaluateConditionals(const std::string& source);
    bool evaluateCondition(const std::string& condition);
    std::string expandMacros(const std::string& source);
    std::string expandMacroCall(const std::string& name, const std::vector<std::string>& args);
};

// =============================================================================
// SHADER CACHE MANAGER
// =============================================================================

/**
 * @brief Shader validation rules and diagnostics
 */
class ShaderValidator {
public:
    enum class Severity {
        Info,
        Warning,
        Error
    };
    
    struct ValidationIssue {
        Severity severity;
        std::string message;
        std::string file;
        int line{-1};
        std::string code;
    };
    
    // Validation rules
    bool validateSource(const std::string& source, std::vector<ValidationIssue>& issues);
    bool validateUniformUsage(const std::string& source, const std::vector<std::string>& uniforms, 
                             std::vector<ValidationIssue>& issues);
    bool checkDeprecatedFunctions(const std::string& source, std::vector<ValidationIssue>& issues);
    bool validateGLSLVersion(const std::string& source, int minVersion, std::vector<ValidationIssue>& issues);
    
    // Performance hints
    bool checkPerformanceHints(const std::string& source, std::vector<ValidationIssue>& issues);
    
    // Configuration
    void setStrictMode(bool strict) { m_strictMode = strict; }
    void setTargetVersion(int version) { m_targetVersion = version; }
    
private:
    bool m_strictMode{false};
    int m_targetVersion{450};  // GLSL 4.5 by default
};

/**
 * @brief Disk cache for compiled shader binaries
 */
class ShaderCacheManager {
public:
    static ShaderCacheManager& getInstance();
    
    // Cache operations
    bool saveToCache(const std::string& shaderKey, const ShaderVariantKey& variantKey, 
                     const std::vector<uint8_t>& binary);
    bool loadFromCache(const std::string& shaderKey, const ShaderVariantKey& variantKey,
                      std::vector<uint8_t>& outBinary);
    
    // Configuration
    void setCacheDirectory(const std::string& path);
    std::string getCacheDirectory() const { return m_cacheDir; }
    void setMaxCacheSize(size_t sizeInMB);
    void enableCompression(bool enable) { m_compressionEnabled = enable; }
    
    // Management
    void clearCache();
    void pruneCache();  // Remove old/unused entries
    size_t getCacheSizeBytes() const;
    size_t getCacheEntryCount() const;
    
private:
    ShaderCacheManager() = default;
    std::string m_cacheDir{".shader_cache"};
    size_t m_maxCacheSizeMB{512};
    bool m_compressionEnabled{true};
    
    std::string generateCacheKey(const std::string& shaderKey, const ShaderVariantKey& variantKey) const;
    std::string getCacheFilePath(const std::string& cacheKey) const;
};

} // namespace Graphics
} // namespace JJM

#endif
