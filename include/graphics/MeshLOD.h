#ifndef MESH_LOD_H
#define MESH_LOD_H

#include <string>
#include <vector>
#include <memory>

namespace JJM {
namespace Graphics {

// Forward declarations
class Mesh;

/**
 * @brief LOD (Level of Detail) configuration
 */
struct LODLevel {
    float distance;           // Distance threshold for this LOD
    float screenCoverage;     // Screen coverage percentage (0-1)
    std::shared_ptr<Mesh> mesh;
    int triangleCount;
    
    LODLevel()
        : distance(0.0f)
        , screenCoverage(1.0f)
        , triangleCount(0)
    {}
};

/**
 * @brief LOD transition mode
 */
enum class LODTransitionMode {
    Instant,        // Immediate switch between LODs
    Fade,           // Crossfade between LODs
    Dither          // Dithered transition
};

/**
 * @brief Mesh LOD group
 * 
 * Manages multiple levels of detail for a mesh and automatically
 * switches between them based on distance or screen coverage.
 */
class MeshLOD {
private:
    std::string m_name;
    std::vector<LODLevel> m_levels;
    int m_currentLOD;
    LODTransitionMode m_transitionMode;
    float m_transitionSpeed;
    float m_transitionProgress;
    bool m_useForcedLOD;
    int m_forcedLOD;
    
    // Bias settings
    float m_lodBias;         // Global LOD bias (-1 to 1, negative = higher quality)
    float m_minScreenSize;   // Minimum screen size before culling
    
public:
    MeshLOD(const std::string& name = "");
    ~MeshLOD();
    
    /**
     * @brief Add LOD level
     * @param level LOD level configuration
     */
    void addLODLevel(const LODLevel& level);
    
    /**
     * @brief Add LOD level by distance
     * @param distance Distance threshold
     * @param mesh Mesh for this LOD
     */
    void addLODLevel(float distance, std::shared_ptr<Mesh> mesh);
    
    /**
     * @brief Get number of LOD levels
     */
    int getLODCount() const { return static_cast<int>(m_levels.size()); }
    
    /**
     * @brief Get LOD level by index
     */
    const LODLevel& getLODLevel(int index) const;
    
    /**
     * @brief Update LOD based on distance
     * @param distance Distance from camera
     * @param deltaTime Time delta for transitions
     * @return True if LOD changed
     */
    bool updateDistance(float distance, float deltaTime = 0.0f);
    
    /**
     * @brief Update LOD based on screen coverage
     * @param screenCoverage Percentage of screen covered (0-1)
     * @param deltaTime Time delta for transitions
     * @return True if LOD changed
     */
    bool updateScreenCoverage(float screenCoverage, float deltaTime = 0.0f);
    
    /**
     * @brief Get current LOD level index
     */
    int getCurrentLOD() const { return m_currentLOD; }
    
    /**
     * @brief Get current LOD mesh
     */
    std::shared_ptr<Mesh> getCurrentMesh() const;
    
    /**
     * @brief Force specific LOD level
     * @param lodLevel LOD level to force (-1 = auto)
     */
    void setForcedLOD(int lodLevel);
    
    /**
     * @brief Set LOD bias
     * @param bias Bias value (-1 to 1, negative = higher quality)
     */
    void setLODBias(float bias) { m_lodBias = bias; }
    float getLODBias() const { return m_lodBias; }
    
    /**
     * @brief Set transition mode
     */
    void setTransitionMode(LODTransitionMode mode) { m_transitionMode = mode; }
    LODTransitionMode getTransitionMode() const { return m_transitionMode; }
    
    /**
     * @brief Set transition speed (for fade/dither modes)
     */
    void setTransitionSpeed(float speed) { m_transitionSpeed = speed; }
    
    /**
     * @brief Get transition progress (0-1)
     */
    float getTransitionProgress() const { return m_transitionProgress; }
    
    /**
     * @brief Check if currently transitioning
     */
    bool isTransitioning() const;
    
    /**
     * @brief Set minimum screen size before culling
     */
    void setMinScreenSize(float size) { m_minScreenSize = size; }
    
    /**
     * @brief Check if mesh should be culled based on screen size
     */
    bool shouldCull(float screenCoverage) const;
    
    /**
     * @brief Get total triangle count across all LODs
     */
    int getTotalTriangleCount() const;
    
    /**
     * @brief Sort LOD levels by distance
     */
    void sortLODLevels();
    
    /**
     * @brief Get name
     */
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
private:
    int selectLODByDistance(float distance) const;
    int selectLODByScreenCoverage(float coverage) const;
    void updateTransition(int targetLOD, float deltaTime);
};

/**
 * @brief LOD system for managing all mesh LODs
 */
class LODSystem {
private:
    std::vector<MeshLOD*> m_lodGroups;
    float m_globalLODBias;
    float m_lodDistanceScale;
    bool m_enableLOD;
    
    // Statistics
    struct Stats {
        int totalLODGroups;
        int visibleObjects;
        int lod0Count;
        int lod1Count;
        int lod2Count;
        int lod3PlusCount;
        int culledCount;
        
        Stats() : totalLODGroups(0), visibleObjects(0), 
                  lod0Count(0), lod1Count(0), lod2Count(0), 
                  lod3PlusCount(0), culledCount(0) {}
    };
    
    Stats m_stats;
    
public:
    LODSystem();
    ~LODSystem();
    
    /**
     * @brief Register LOD group with the system
     */
    void registerLODGroup(MeshLOD* lodGroup);
    
    /**
     * @brief Unregister LOD group
     */
    void unregisterLODGroup(MeshLOD* lodGroup);
    
    /**
     * @brief Update all LOD groups based on camera
     * @param cameraPosition Camera position in world space
     * @param cameraFrustum Camera frustum for screen coverage calculation
     * @param deltaTime Time delta for transitions
     */
    void update(const float* cameraPosition, float deltaTime = 0.0f);
    
    /**
     * @brief Set global LOD bias
     * @param bias Bias value (-1 to 1, negative = higher quality)
     */
    void setGlobalLODBias(float bias) { m_globalLODBias = bias; }
    float getGlobalLODBias() const { return m_globalLODBias; }
    
    /**
     * @brief Set LOD distance scale
     * @param scale Scale factor for all LOD distances
     */
    void setLODDistanceScale(float scale) { m_lodDistanceScale = scale; }
    float getLODDistanceScale() const { return m_lodDistanceScale; }
    
    /**
     * @brief Enable or disable LOD system
     */
    void setEnabled(bool enabled) { m_enableLOD = enabled; }
    bool isEnabled() const { return m_enableLOD; }
    
    /**
     * @brief Get LOD statistics
     */
    const Stats& getStats() const { return m_stats; }
    
    /**
     * @brief Clear statistics
     */
    void clearStats();
    
private:
    void updateStats();
};

/**
 * @brief Utility functions for LOD generation
 */
class LODGenerator {
public:
    /**
     * @brief Generate LOD levels from a base mesh
     * @param baseMesh High-resolution base mesh
     * @param numLevels Number of LOD levels to generate
     * @param reductionRates Polygon reduction rate per level (e.g., {0.5, 0.25, 0.1})
     * @return Vector of generated LOD meshes
     */
    static std::vector<std::shared_ptr<Mesh>> generateLODs(
        const Mesh& baseMesh,
        int numLevels,
        const std::vector<float>& reductionRates
    );
    
    /**
     * @brief Calculate recommended LOD distances
     * @param objectSize Approximate object size
     * @param numLevels Number of LOD levels
     * @return Vector of recommended distances
     */
    static std::vector<float> calculateLODDistances(float objectSize, int numLevels);
    
    /**
     * @brief Simplify mesh by target polygon count
     * @param mesh Input mesh
     * @param targetTriangleCount Target triangle count
     * @return Simplified mesh
     */
    static std::shared_ptr<Mesh> simplifyMesh(const Mesh& mesh, int targetTriangleCount);
};

} // namespace Graphics
} // namespace JJM

#endif // MESH_LOD_H
