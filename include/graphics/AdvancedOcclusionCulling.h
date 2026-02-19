#ifndef ADVANCED_OCCLUSION_CULLING_H
#define ADVANCED_OCCLUSION_CULLING_H

#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

namespace JJM {
namespace Graphics {

/**
 * @brief Enhanced frustum culling with early rejection
 */
class EnhancedFrustumCuller {
   private:
    float m_planes[6][4];
    float m_corners[8][3];

   public:
    void extractFromMatrix(const float* viewProjMatrix);
    bool testAABB(const float* min, const float* max) const;
    bool testSphere(const float* center, float radius) const;
    bool testAABBFast(const float* min, const float* max) const;  // Fast approximate test
};

/**
 * @brief GPU-based occlusion query manager
 */
class GPUOcclusionQueryManager {
   private:
    struct QueryState {
        unsigned int queryID;
        unsigned int objectID;
        int frameIssued;
        bool resultAvailable;
        unsigned int samplesPassed;
    };

    std::vector<QueryState> m_queries;
    std::queue<unsigned int> m_freeQueries;
    int m_currentFrame;
    int m_queryBudget;
    bool m_conservativeRasterization;

   public:
    GPUOcclusionQueryManager();
    ~GPUOcclusionQueryManager();

    void initialize(int maxQueries = 1000);
    void shutdown();

    /**
     * @brief Issue occlusion query for object
     */
    bool issueQuery(unsigned int objectID, const float* min, const float* max);

    /**
     * @brief Collect available query results
     */
    void collectResults(std::unordered_map<unsigned int, bool>& visibilityMap);

    /**
     * @brief Advance to next frame
     */
    void nextFrame() { m_currentFrame++; }

    /**
     * @brief Set query budget per frame
     */
    void setQueryBudget(int budget) { m_queryBudget = budget; }
    int getQueryBudget() const { return m_queryBudget; }

   private:
    void renderBoundingBox(const float* min, const float* max);
};

/**
 * @brief Software Hi-Z buffer implementation
 */
class SoftwareHiZBuffer {
   private:
    std::vector<std::vector<float>> m_depthPyramid;
    int m_baseWidth;
    int m_baseHeight;
    int m_levels;

   public:
    SoftwareHiZBuffer();
    ~SoftwareHiZBuffer();

    void initialize(int width, int height);
    void buildFromDepth(const float* depthData);
    bool testAABB(const float* screenMin, const float* screenMax, float minZ) const;

   private:
    void downsampleLevel(int level);
    float sampleDepthConservative(int x, int y, int level) const;
    float getDepth(int x, int y, int mipLevel) const;
    int getMipLevelForSize(float screenWidth, float screenHeight) const;
};

/**
 * @brief Advanced occlusion culling with multiple strategies
 */
class AdvancedOcclusionCuller {
   private:
    struct ObjectState {
        unsigned int id;
        float bounds[6];  // min[3], max[3]
        float lastVisibleFrame;
        int visibilityHistory;  // Bit field of last N frames
        float importance;       // Higher = more likely to query
    };

    std::unordered_map<unsigned int, ObjectState> m_objects;
    EnhancedFrustumCuller m_frustumCuller;
    std::unique_ptr<GPUOcclusionQueryManager> m_queryManager;
    std::unique_ptr<SoftwareHiZBuffer> m_hiZBuffer;

    // Culling settings
    bool m_useFrustumCulling;
    bool m_useOcclusionQueries;
    bool m_useHiZ;
    bool m_useTemporalCoherence;

    // Statistics
    struct CullStats {
        int totalObjects;
        int frustumCulled;
        int occlusionCulled;
        int visible;
        int queriesIssued;
        int hizTests;

        CullStats()
            : totalObjects(0),
              frustumCulled(0),
              occlusionCulled(0),
              visible(0),
              queriesIssued(0),
              hizTests(0) {}
    };

    CullStats m_stats;
    int m_currentFrame;

   public:
    AdvancedOcclusionCuller();
    ~AdvancedOcclusionCuller();

    void initialize();
    void shutdown();

    /**
     * @brief Register object for culling
     */
    void registerObject(unsigned int id, const float* min, const float* max,
                        float importance = 1.0f);

    /**
     * @brief Unregister object
     */
    void unregisterObject(unsigned int id);

    /**
     * @brief Update object bounds
     */
    void updateBounds(unsigned int id, const float* min, const float* max);

    /**
     * @brief Perform culling for current frame
     * @param viewProjMatrix View-projection matrix
     * @param depthBuffer Optional depth buffer for Hi-Z
     * @return Vector of visible object IDs
     */
    std::vector<unsigned int> cull(const float* viewProjMatrix, const float* depthBuffer = nullptr);

    /**
     * @brief Enable/disable culling methods
     */
    void setUseFrustumCulling(bool use) { m_useFrustumCulling = use; }
    void setUseOcclusionQueries(bool use) { m_useOcclusionQueries = use; }
    void setUseHiZ(bool use) { m_useHiZ = use; }
    void setUseTemporalCoherence(bool use) { m_useTemporalCoherence = use; }

    /**
     * @brief Get statistics
     */
    const CullStats& getStats() const { return m_stats; }
    void clearStats();

    /**
     * @brief Set query budget
     */
    void setQueryBudget(int budget);

   private:
    void performFrustumCulling(std::vector<ObjectState*>& candidates);
    void performOcclusionQueries(std::vector<ObjectState*>& candidates);
    void performHiZTest(std::vector<ObjectState*>& candidates);
    void updateTemporalCoherence();
    void prioritizeQueries(std::vector<ObjectState*>& candidates);
    float calculateImportance(const ObjectState& obj) const;
};

}  // namespace Graphics
}  // namespace JJM

#endif  // ADVANCED_OCCLUSION_CULLING_H
