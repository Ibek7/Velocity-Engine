#ifndef ADVANCED_POST_PROCESSING_H
#define ADVANCED_POST_PROCESSING_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace JJM {
namespace Graphics {

/**
 * @brief Advanced bloom effect with multiple passes
 */
class AdvancedBloom {
private:
    std::vector<unsigned int> m_mipChain;
    std::shared_ptr<class Shader> m_downsampleShader;
    std::shared_ptr<class Shader> m_upsampleShader;
    std::shared_ptr<class Shader> m_thresholdShader;
    
    float m_threshold;
    float m_knee;
    float m_intensity;
    int m_mipLevels;
    
public:
    AdvancedBloom();
    ~AdvancedBloom();
    
    void initialize(int width, int height);
    void shutdown();
    void render(unsigned int inputHDR, unsigned int output);
    
    void setThreshold(float threshold, float knee = 0.1f);
    void setIntensity(float intensity) { m_intensity = intensity; }
    void setMipLevels(int levels) { m_mipLevels = levels; }
    
private:
    void createMipChain(int width, int height);
    void downsamplePass(unsigned int source, unsigned int dest);
    void upsamplePass(unsigned int source, unsigned int dest);
};

/**
 * @brief ACES filmic tone mapping
 */
class ACESToneMapping {
private:
    std::shared_ptr<class Shader> m_shader;
    float m_exposure;
    
public:
    ACESToneMapping();
    ~ACESToneMapping();
    
    void initialize();
    void shutdown();
    void render(unsigned int inputHDR, unsigned int outputLDR);
    
    void setExposure(float exposure) { m_exposure = exposure; }
    float getExposure() const { return m_exposure; }
    
private:
    static float ACESFilmRec2020(float x);
};

/**
 * @brief LUT-based color grading
 */
class LUTColorGrading {
private:
    std::shared_ptr<class Shader> m_shader;
    unsigned int m_lutTexture;
    float m_intensity;
    
public:
    LUTColorGrading();
    ~LUTColorGrading();
    
    void initialize();
    void shutdown();
    void render(unsigned int input, unsigned int output);
    
    void loadLUT(const std::string& filepath);
    void setIntensity(float intensity) { m_intensity = intensity; }
    
private:
    unsigned int createLUTTexture(const unsigned char* data, int size);
};

/**
 * @brief Advanced post-processing stack
 */
class AdvancedPostProcessStack {
private:
    std::unique_ptr<AdvancedBloom> m_bloom;
    std::unique_ptr<ACESToneMapping> m_toneMapping;
    std::unique_ptr<LUTColorGrading> m_colorGrading;
    
    // Render targets for ping-pong
    unsigned int m_hdrTarget1;
    unsigned int m_hdrTarget2;
    unsigned int m_ldrTarget;
    
    int m_width;
    int m_height;
    bool m_initialized;
    
    // Effect enable flags
    bool m_bloomEnabled;
    bool m_toneMappingEnabled;
    bool m_colorGradingEnabled;
    
public:
    AdvancedPostProcessStack();
    ~AdvancedPostProcessStack();
    
    void initialize(int width, int height);
    void shutdown();
    void resize(int width, int height);
    
    /**
     * @brief Process HDR input through post-processing stack
     * @param inputHDR Input HDR texture
     * @param outputLDR Final LDR output texture
     */
    void process(unsigned int inputHDR, unsigned int outputLDR);
    
    // Access to individual effects
    AdvancedBloom* getBloom() { return m_bloom.get(); }
    ACESToneMapping* getToneMapping() { return m_toneMapping.get(); }
    LUTColorGrading* getColorGrading() { return m_colorGrading.get(); }
    
    // Enable/disable effects
    void setBloomEnabled(bool enabled) { m_bloomEnabled = enabled; }
    void setToneMappingEnabled(bool enabled) { m_toneMappingEnabled = enabled; }
    void setColorGradingEnabled(bool enabled) { m_colorGradingEnabled = enabled; }
    
    bool isBloomEnabled() const { return m_bloomEnabled; }
    bool isToneMappingEnabled() const { return m_toneMappingEnabled; }
    bool isColorGradingEnabled() const { return m_colorGradingEnabled; }
    
private:
    void createRenderTargets();
    void destroyRenderTargets();
};

} // namespace Graphics
} // namespace JJM

#endif // ADVANCED_POST_PROCESSING_H
