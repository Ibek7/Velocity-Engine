#pragma once

#include <vector>
#include <functional>

/**
 * @file OcclusionCulling.h
 * @brief Occlusion culling system for visibility determination
 * 
 * Provides multiple methods for determining object visibility including
 * frustum culling, GPU occlusion queries, hierarchical Z-buffer testing,
 * and portal-based culling. Helps optimize rendering by eliminating
 * objects that are not visible to the camera.
 */

namespace Engine {

/**
 * @struct BoundingBox
 * @brief Axis-aligned bounding box representation
 */
struct BoundingBox {
    float minX, minY, minZ;  ///< Minimum corner coordinates
    float maxX, maxY, maxZ;  ///< Maximum corner coordinates
};

/**
 * @struct BoundingSphere
 * @brief Spherical bounding volume representation
 */
struct BoundingSphere {
    float x, y, z;      ///< Center position
    float radius;       ///< Sphere radius
};

/**
 * @struct OcclusionQuery
 * @brief GPU occlusion query state
 */
struct OcclusionQuery {
    unsigned int queryId;  ///< GPU query object ID
    int entityId;          ///< Associated entity identifier
    bool isActive;         ///< Whether query is currently active
    bool isVisible;        ///< Last known visibility state
    int frameDelay;        ///< Frames to wait before reading result
};

/**
 * @enum CullingMethod
 * @brief Available occlusion culling techniques
 */
enum class CullingMethod {
    FrustumOnly,           ///< Basic frustum culling only
    OcclusionQuery,        ///< GPU-based occlusion queries
    HierarchicalZ,         ///< Hierarchical Z-buffer testing
    SoftwareRasterization  ///< CPU-based software rasterization
};

/**
 * @class OcclusionCuller
 * @brief Main occlusion culling implementation
 * 
 * Handles visibility determination using various techniques. Can be configured
 * to use different culling methods based on performance requirements.
 */
class OcclusionCuller {
public:
    OcclusionCuller();
    ~OcclusionCuller();

    // Configuration
    /**
     * @brief Set the culling method to use
     * @param method The culling technique to employ
     */
    void setMethod(CullingMethod method) { m_method = method; }
    
    /**
     * @brief Get the current culling method
     * @return The active culling technique
     */
    CullingMethod getMethod() const { return m_method; }
    
    /**
     * @brief Set frame delay for occlusion query results
     * @param frames Number of frames to wait before reading query results
     * 
     * Higher values reduce CPU-GPU synchronization stalls but increase latency
     */
    void setQueryFrameDelay(int frames) { m_queryFrameDelay = frames; }
    
    /**
     * @brief Set minimum screen size threshold for culling
     * @param size Minimum screen-space size (in pixels) to consider for culling
     * 
     * Objects smaller than this may be culled even if visible
     */
    void setMinScreenSize(float size) { m_minScreenSize = size; }
    
    // Frustum
    /**
     * @brief Update frustum planes for culling tests
     * @param planes Array of 6 plane equations [A, B, C, D] where Ax + By + Cz + D = 0
     */
    void setFrustumPlanes(const float planes[6][4]);
    
    /**
     * @brief Test if bounding box intersects view frustum
     * @param box The axis-aligned bounding box to test
     * @return true if box is at least partially inside frustum
     */
    bool isInFrustum(const BoundingBox& box) const;
    
    /**
     * @brief Test if bounding sphere intersects view frustum
     * @param sphere The bounding sphere to test
     * @return true if sphere is at least partially inside frustum
     */
    bool isInFrustum(const BoundingSphere& sphere) const;
    
    // Occlusion queries
    /**
     * @brief Begin GPU occlusion query for an entity
     * @param entityId Unique identifier for the entity being tested
     * 
     * Should be followed by rendering the entity's bounding volume, then endOcclusionQuery()
     */
    void beginOcclusionQuery(int entityId);
    
    /**
     * @brief End current GPU occlusion query
     */
    void endOcclusionQuery();
    
    /**
     * @brief Check if entity is occluded based on previous queries
     * @param entityId The entity to check
     * @return true if entity is determined to be occluded
     */
    bool isOccluded(int entityId) const;
    
    /**
     * @brief Update all active occlusion queries and retrieve results
     * 
     * Should be called once per frame to process query results
     */
    void updateQueries();
    
    /**
     * @brief Batch test multiple bounding boxes against frustum
     * @param boxes Array of bounding boxes to test
     * @param count Number of boxes in array
     * @param results Output array of visibility results (true = visible)
     * 
     * More efficient than testing boxes individually when testing many objects
     */
    void batchTestFrustum(const BoundingBox* boxes, int count, bool* results) const;
    
    /**
     * @brief Batch test multiple bounding spheres against frustum
     * @param spheres Array of bounding spheres to test
     * @param count Number of spheres in array
     * @param results Output array of visibility results (true = visible)
     * 
     * More efficient than testing spheres individually when testing many objects
     */
    void batchTestFrustum(const BoundingSphere* spheres, int count, bool* results) const;
    
    // Hierarchical Z-buffer
    /**
     * @brief Initialize hierarchical Z-buffer with given dimensions
     * @param width Width of the depth buffer
     * @param height Height of the depth buffer
     */
    void initializeHiZ(int width, int height);
    
    /**
     * @brief Update hierarchical Z-buffer from depth texture
     * @param depthTexture GPU depth texture to build HiZ pyramid from
     */
    void updateHiZ(unsigned int depthTexture);
    
    /**
     * @brief Test bounding box against hierarchical Z-buffer
     * @param box The bounding box to test
     * @return true if box is potentially visible (not occluded)
     */
    bool testHiZ(const BoundingBox& box) const;
    
    // Portal culling
    /**
     * @brief Add a portal between two rooms
     * @param vertices Portal polygon vertices in 3D space
     * @param count Number of vertices
     * @param roomA First room identifier
     * @param roomB Second room identifier
     */
    void addPortal(const float vertices[][3], int count, int roomA, int roomB);
    
    /**
     * @brief Set the current room containing the camera
     * @param roomId Room identifier
     */
    void setCurrentRoom(int roomId);
    
    /**
     * @brief Check if a room is visible from current room through portals
     * @param roomId Room identifier to test
     * @return true if room is potentially visible
     */
    bool isRoomVisible(int roomId) const;
    
    // Statistics
    /**
     * @struct CullingStats
     * @brief Performance and debugging statistics for occlusion culling
     */
    struct CullingStats {
        int totalObjects;       ///< Total objects tested this frame
        int visibleObjects;     ///< Objects determined to be visible
        int frustumCulled;      ///< Objects culled by frustum test
        int occlusionCulled;    ///< Objects culled by occlusion test
        int queryCount;         ///< Number of active GPU queries
        float cullingTime;      ///< Time spent on culling (milliseconds)
    };
    
    /**
     * @brief Get current culling statistics
     * @return Reference to statistics structure
     */
    const CullingStats& getStats() const { return m_stats; }
    
    /**
     * @brief Reset statistics counters to zero
     */
    void resetStats();
    
    // Update
    /**
     * @brief Update culling system state
     * @param deltaTime Time since last frame in seconds
     */
    void update(float deltaTime);

private:
    bool testFrustumBox(const BoundingBox& box) const;
    bool testFrustumSphere(const BoundingSphere& sphere) const;
    
    void generateHiZMipmap(int level);
    float sampleHiZ(float x, float y, int level) const;

    CullingMethod m_method;
    
    // Frustum planes (6 planes: left, right, top, bottom, near, far)
    float m_frustumPlanes[6][4];
    
    // Occlusion queries
    std::vector<OcclusionQuery> m_queries;
    int m_queryFrameDelay;
    float m_minScreenSize;
    
    // Hierarchical Z-buffer
    std::vector<std::vector<float>> m_hizPyramid;
    int m_hizWidth, m_hizHeight;
    int m_hizLevels;
    
    // Portal system
    struct Portal {
        std::vector<float> vertices;
        int roomA, roomB;
    };
    std::vector<Portal> m_portals;
    std::vector<bool> m_visibleRooms;
    int m_currentRoom;
    
    CullingStats m_stats;
};

/**
 * @class OcclusionSystem
 * @brief High-level occlusion culling system manager (Singleton)
 * 
 * Provides a centralized interface for managing occlusion culling across
 * the engine. Handles entity registration, camera updates, and visibility queries.
 */
class OcclusionSystem {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to the global OcclusionSystem instance
     */
    static OcclusionSystem& getInstance();

    /**
     * @brief Initialize the occlusion system
     */
    void initialize();
    
    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();
    
    /**
     * @brief Update occlusion culling state
     * @param deltaTime Time since last frame in seconds
     */
    void update(float deltaTime);

    // Culler access
    /**
     * @brief Get access to underlying culler
     * @return Reference to OcclusionCuller
     */
    OcclusionCuller& getCuller() { return m_culler; }
    
    /**
     * @brief Get const access to underlying culler
     * @return Const reference to OcclusionCuller
     */
    const OcclusionCuller& getCuller() const { return m_culler; }
    
    // Entity registration
    /**
     * @brief Register entity for occlusion culling
     * @param entityId Unique identifier for the entity
     * @param bounds Bounding volume for the entity
     */
    void registerEntity(int entityId, const BoundingBox& bounds);
    
    /**
     * @brief Update entity's bounding volume
     * @param entityId Entity identifier
     * @param bounds New bounding volume
     */
    void updateEntityBounds(int entityId, const BoundingBox& bounds);
    
    /**
     * @brief Unregister entity from culling system
     * @param entityId Entity identifier to remove
     */
    void unregisterEntity(int entityId);
    
    // Camera
    /**
     * @brief Update camera view-projection matrix
     * @param viewProj 4x4 view-projection matrix in column-major order
     */
    void setCameraViewProj(const float viewProj[16]);
    
    /**
     * @brief Update camera position for distance-based culling
     * @param x Camera X coordinate
     * @param y Camera Y coordinate
     * @param z Camera Z coordinate
     */
    void setCameraPosition(float x, float y, float z);
    
    // Query
    /**
     * @brief Check if specific entity is visible
     * @param entityId Entity identifier to check
     * @return true if entity is visible
     */
    bool isEntityVisible(int entityId) const;
    
    /**
     * @brief Get list of all visible entities
     * @return Vector of visible entity identifiers
     */
    std::vector<int> getVisibleEntities() const;
    
    // Global settings
    /**
     * @brief Enable or disable occlusion culling
     * @param enabled true to enable culling
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    /**
     * @brief Check if occlusion culling is enabled
     * @return true if culling is active
     */
    bool isEnabled() const { return m_enabled; }
    
    /**
     * @brief Enable or disable debug visualization
     * @param enable true to enable debug rendering
     */
    void setDebugVisualization(bool enable) { m_debugVisualization = enable; }
    
    /**
     * @brief Check if debug visualization is enabled
     * @return true if debug visualization is active
     */
    bool isDebugVisualizationEnabled() const { return m_debugVisualization; }

private:
    OcclusionSystem();
    OcclusionSystem(const OcclusionSystem&) = delete;
    OcclusionSystem& operator=(const OcclusionSystem&) = delete;

    void extractFrustumPlanes(const float viewProj[16]);

    OcclusionCuller m_culler;
    
    struct EntityInfo {
        int id;
        BoundingBox bounds;
        bool isVisible;
    };
    std::vector<EntityInfo> m_entities;
    
    float m_cameraX, m_cameraY, m_cameraZ;
    bool m_enabled;
    bool m_debugVisualization;
};

} // namespace Engine
