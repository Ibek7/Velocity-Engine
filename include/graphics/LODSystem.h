#pragma once

#include <vector>
#include <string>

// Level of Detail (LOD) system
namespace Engine {

struct LODLevel {
    int meshIndex;
    float distance;
    int triangleCount;
    float screenCoverage;
};

struct LODTransition {
    float fadeStartDistance;
    float fadeEndDistance;
    float currentFade;
    bool isFading;
};

class LODMesh {
public:
    LODMesh();
    ~LODMesh();

    // LOD levels
    void addLODLevel(int meshIndex, float distance, int triangleCount);
    void setLODLevel(int index, int meshIndex, float distance);
    void removeLODLevel(int index);
    int getLODCount() const { return static_cast<int>(m_levels.size()); }
    
    const LODLevel& getLODLevel(int index) const { return m_levels[index]; }
    
    // Distance calculation
    int selectLOD(float distanceToCamera) const;
    int selectLODByScreenCoverage(float screenCoverage) const;
    
    // Transition
    void setTransitionSpeed(float speed) { m_transitionSpeed = speed; }
    void enableCrossFade(bool enable) { m_crossFadeEnabled = enable; }
    void setFadeRange(float range) { m_fadeRange = range; }
    
    const LODTransition& getTransition() const { return m_transition; }
    
    // Bias
    void setLODBias(float bias) { m_lodBias = bias; }
    float getLODBias() const { return m_lodBias; }
    
    // Update
    void update(float deltaTime, float currentDistance);

private:
    void sortLevels();

    std::vector<LODLevel> m_levels;
    LODTransition m_transition;
    
    int m_currentLOD;
    int m_previousLOD;
    
    float m_transitionSpeed;
    bool m_crossFadeEnabled;
    float m_fadeRange;
    float m_lodBias;
};

class LODGroup {
public:
    LODGroup(const std::string& name);
    ~LODGroup();

    // Configuration
    void setPosition(float x, float y, float z);
    void setBoundsRadius(float radius) { m_boundsRadius = radius; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    const std::string& getName() const { return m_name; }
    float getBoundsRadius() const { return m_boundsRadius; }
    bool isEnabled() const { return m_enabled; }
    
    // LOD meshes
    void addMesh(LODMesh* mesh);
    void removeMesh(LODMesh* mesh);
    int getMeshCount() const { return static_cast<int>(m_meshes.size()); }
    LODMesh* getMesh(int index);
    
    // Update
    void update(float deltaTime, float cameraX, float cameraY, float cameraZ);
    
    // Query
    float getDistanceToCamera() const { return m_distanceToCamera; }
    int getActiveLOD() const { return m_activeLOD; }

private:
    std::string m_name;
    float m_posX, m_posY, m_posZ;
    float m_boundsRadius;
    bool m_enabled;
    
    std::vector<LODMesh*> m_meshes;
    
    float m_distanceToCamera;
    int m_activeLOD;
};

class LODSystem {
public:
    static LODSystem& getInstance();

    void initialize();
    void shutdown();
    void update(float deltaTime);

    // Camera
    void setCameraPosition(float x, float y, float z);
    void getCameraPosition(float& x, float& y, float& z) const;
    
    // LOD groups
    LODGroup* createLODGroup(const std::string& name);
    void destroyLODGroup(LODGroup* group);
    LODGroup* getLODGroup(const std::string& name);
    
    // Global settings
    void setGlobalLODBias(float bias) { m_globalLODBias = bias; }
    float getGlobalLODBias() const { return m_globalLODBias; }
    
    void setMaxLODLevel(int level) { m_maxLODLevel = level; }
    int getMaxLODLevel() const { return m_maxLODLevel; }
    
    void setForceLOD(int level) { m_forceLOD = level; m_forceLODEnabled = level >= 0; }
    void disableForceLOD() { m_forceLODEnabled = false; }
    
    // Quality presets
    void setQuality(int quality); // 0=Low, 1=Medium, 2=High, 3=Ultra
    
    // Statistics
    struct LODStats {
        int totalLODGroups;
        int visibleLODGroups;
        int lod0Count;
        int lod1Count;
        int lod2Count;
        int lod3PlusCount;
        int totalTriangles;
        int savedTriangles;
    };
    
    const LODStats& getStats() const { return m_stats; }
    void resetStats();
    
    // Debugging
    void setDebugVisualization(bool enable) { m_debugVisualization = enable; }
    bool isDebugVisualizationEnabled() const { return m_debugVisualization; }

private:
    LODSystem();
    LODSystem(const LODSystem&) = delete;
    LODSystem& operator=(const LODSystem&) = delete;

    void updateStats();

    std::vector<LODGroup*> m_groups;
    
    float m_cameraX, m_cameraY, m_cameraZ;
    
    float m_globalLODBias;
    int m_maxLODLevel;
    
    bool m_forceLODEnabled;
    int m_forceLOD;
    
    LODStats m_stats;
    bool m_debugVisualization;
};

// Helper for automatic LOD generation
class LODGenerator {
public:
    // Generate LOD levels from high-res mesh
    static std::vector<int> generateLODLevels(
        int highResMeshIndex,
        const std::vector<float>& distances,
        const std::vector<float>& reductionFactors
    );
    
    // Mesh simplification
    static int simplifyMesh(int sourceMeshIndex, float targetReduction);
    
    // Automatic distance calculation
    static std::vector<float> calculateLODDistances(
        float objectSize,
        int lodCount,
        float minDistance = 5.0f,
        float maxDistance = 1000.0f
    );
};

} // namespace Engine
