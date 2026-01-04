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
 * @brief Shader compilation result
 */
struct ShaderCompileResult {
    bool success;
    std::string errorMessage;
    std::vector<std::string> warnings;
    float compileTimeMs;
    size_t binarySize;
    
    ShaderCompileResult() : success(false), compileTimeMs(0.0f), binarySize(0) {}
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
    
    // Include system
    ShaderIncludeResolver& getIncludeResolver() { return m_includeResolver; }
    
    // Binary cache
    void enableBinaryCache(bool enable, const std::string& cachePath = "");
    bool loadBinaryCache();
    bool saveBinaryCache();
    
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
};

} // namespace Graphics
} // namespace JJM

#endif
