#pragma once

#include <vector>
#include <functional>

// Occlusion culling system
namespace Engine {

struct BoundingBox {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
};

struct BoundingSphere {
    float x, y, z;
    float radius;
};

struct OcclusionQuery {
    unsigned int queryId;
    int entityId;
    bool isActive;
    bool isVisible;
    int frameDelay;
};

enum class CullingMethod {
    FrustumOnly,
    OcclusionQuery,
    HierarchicalZ,
    SoftwareRasterization
};

class OcclusionCuller {
public:
    OcclusionCuller();
    ~OcclusionCuller();

    // Configuration
    void setMethod(CullingMethod method) { m_method = method; }
    CullingMethod getMethod() const { return m_method; }
    
    void setQueryFrameDelay(int frames) { m_queryFrameDelay = frames; }
    void setMinScreenSize(float size) { m_minScreenSize = size; }
    
    // Frustum
    void setFrustumPlanes(const float planes[6][4]);
    bool isInFrustum(const BoundingBox& box) const;
    bool isInFrustum(const BoundingSphere& sphere) const;
    
    // Occlusion queries
    void beginOcclusionQuery(int entityId);
    void endOcclusionQuery();
    bool isOccluded(int entityId) const;
    void updateQueries();
    
    // Hierarchical Z-buffer
    void initializeHiZ(int width, int height);
    void updateHiZ(unsigned int depthTexture);
    bool testHiZ(const BoundingBox& box) const;
    
    // Portal culling
    void addPortal(const float vertices[][3], int count, int roomA, int roomB);
    void setCurrentRoom(int roomId);
    bool isRoomVisible(int roomId) const;
    
    // Statistics
    struct CullingStats {
        int totalObjects;
        int visibleObjects;
        int frustumCulled;
        int occlusionCulled;
        int queryCount;
        float cullingTime;
    };
    
    const CullingStats& getStats() const { return m_stats; }
    void resetStats();
    
    // Update
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

class OcclusionSystem {
public:
    static OcclusionSystem& getInstance();

    void initialize();
    void shutdown();
    void update(float deltaTime);

    // Culler access
    OcclusionCuller& getCuller() { return m_culler; }
    const OcclusionCuller& getCuller() const { return m_culler; }
    
    // Entity registration
    void registerEntity(int entityId, const BoundingBox& bounds);
    void updateEntityBounds(int entityId, const BoundingBox& bounds);
    void unregisterEntity(int entityId);
    
    // Camera
    void setCameraViewProj(const float viewProj[16]);
    void setCameraPosition(float x, float y, float z);
    
    // Query
    bool isEntityVisible(int entityId) const;
    std::vector<int> getVisibleEntities() const;
    
    // Global settings
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    void setDebugVisualization(bool enable) { m_debugVisualization = enable; }
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
