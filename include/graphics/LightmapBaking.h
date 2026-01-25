#ifndef LIGHTMAP_BAKING_H
#define LIGHTMAP_BAKING_H

#include <string>
#include <vector>

namespace JJM {
namespace Graphics {

struct LightmapSettings {
    int resolution;
    int samples;
    float maxDistance;
    bool enableAO;
    bool enableGI;
    float aoRadius;
    int bounces;
    
    LightmapSettings()
        : resolution(1024), samples(256), maxDistance(100.0f),
          enableAO(true), enableGI(false), aoRadius(1.0f), bounces(1) {}
};

class Lightmap {
public:
    Lightmap(int width, int height);
    ~Lightmap();
    
    void setPixel(int x, int y, float r, float g, float b);
    void save(const std::string& filename);
    
private:
    int m_width, m_height;
    std::vector<float> m_data;
};

class LightmapBaker {
public:
    LightmapBaker();
    ~LightmapBaker();
    
    void setSettings(const LightmapSettings& settings) { m_settings = settings; }
    const LightmapSettings& getSettings() const { return m_settings; }
    
    void bake();
    void cancel();
    
    float getProgress() const { return m_progress; }
    bool isDone() const { return m_done; }
    
private:
    LightmapSettings m_settings;
    float m_progress;
    bool m_done;
    
    void computeAmbientOcclusion();
    void computeGlobalIllumination();
};

} // namespace Graphics
} // namespace JJM

#endif
