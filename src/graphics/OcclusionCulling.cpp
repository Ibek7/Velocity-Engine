#include "graphics/OcclusionCulling.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <thread>
#include <future>

namespace Engine {

OcclusionCuller::OcclusionCuller()
    : m_method(CullingMethod::FrustumOnly)
    , m_queryFrameDelay(2)
    , m_minScreenSize(0.01f)
    , m_hizWidth(0)
    , m_hizHeight(0)
    , m_hizLevels(0)
    , m_currentRoom(0)
{
    std::memset(m_frustumPlanes, 0, sizeof(m_frustumPlanes));
    m_stats = CullingStats();
}

OcclusionCuller::~OcclusionCuller() {
}

void OcclusionCuller::setFrustumPlanes(const float planes[6][4]) {
    std::memcpy(m_frustumPlanes, planes, sizeof(m_frustumPlanes));
}

bool OcclusionCuller::isInFrustum(const BoundingBox& box) const {
    return testFrustumBox(box);
}

bool OcclusionCuller::isInFrustum(const BoundingSphere& sphere) const {
    return testFrustumSphere(sphere);
}

void OcclusionCuller::beginOcclusionQuery(int entityId) {
    if (m_method != CullingMethod::OcclusionQuery) {
        return;
    }
    
    // Find or create query for this entity
    OcclusionQuery* query = nullptr;
    for (auto& q : m_queries) {
        if (q.entityId == entityId) {
            query = &q;
            break;
        }
    }
    
    if (!query) {
        OcclusionQuery newQuery;
        newQuery.queryId = 0; // Generate OpenGL query object
        newQuery.entityId = entityId;
        newQuery.isActive = false;
        newQuery.isVisible = true;
        newQuery.frameDelay = 0;
        m_queries.push_back(newQuery);
        query = &m_queries.back();
    }
    
    query->isActive = true;
    // glBeginQuery(GL_SAMPLES_PASSED, query->queryId);
}

void OcclusionCuller::endOcclusionQuery() {
    if (m_method != CullingMethod::OcclusionQuery) {
        return;
    }
    
    // glEndQuery(GL_SAMPLES_PASSED);
}

bool OcclusionCuller::isOccluded(int entityId) const {
    for (const auto& query : m_queries) {
        if (query.entityId == entityId) {
            return !query.isVisible;
        }
    }
    return false;
}

void OcclusionCuller::updateQueries() {
    for (auto& query : m_queries) {
        if (!query.isActive) {
            continue;
        }
        
        if (query.frameDelay < m_queryFrameDelay) {
            query.frameDelay++;
            continue;
        }
        
        // Check query result
        // GLuint samplesPassed = 0;
        // glGetQueryObjectuiv(query.queryId, GL_QUERY_RESULT, &samplesPassed);
        // query.isVisible = samplesPassed > 0;
        
        query.frameDelay = 0;
    }
}

void OcclusionCuller::initializeQueryPool(int poolSize) {
    m_queryPool.maxPoolSize = poolSize;
    m_queryPool.availableQueries.clear();
    m_queryPool.activeQueries.clear();
    
    // Pre-allocate GPU query objects
    // Note: This would use actual GPU API calls (OpenGL, Vulkan, etc.)
    // For now, just prepare the pool structure
    m_queryPool.availableQueries.reserve(poolSize);
}

unsigned int OcclusionCuller::allocateQuery() {
    if (m_queryPool.availableQueries.empty()) {
        // Try to allocate new query if under max size
        if (m_queryPool.activeQueries.size() < static_cast<size_t>(m_queryPool.maxPoolSize)) {
            // In real implementation, would create GPU query object here
            unsigned int newQueryId = static_cast<unsigned int>(m_queryPool.activeQueries.size() + 1);
            m_queryPool.activeQueries.push_back(newQueryId);
            return newQueryId;
        }
        return 0; // Pool exhausted
    }
    
    unsigned int queryId = m_queryPool.availableQueries.back();
    m_queryPool.availableQueries.pop_back();
    m_queryPool.activeQueries.push_back(queryId);
    return queryId;
}

void OcclusionCuller::freeQuery(unsigned int queryId) {
    if (queryId == 0) return;
    
    // Remove from active list
    auto it = std::find(m_queryPool.activeQueries.begin(), m_queryPool.activeQueries.end(), queryId);
    if (it != m_queryPool.activeQueries.end()) {
        m_queryPool.activeQueries.erase(it);
        m_queryPool.availableQueries.push_back(queryId);
    }
}

void OcclusionCuller::getQueryPoolStats(int& available, int& active) const {
    available = static_cast<int>(m_queryPool.availableQueries.size());
    active = static_cast<int>(m_queryPool.activeQueries.size());
}

void OcclusionCuller::initializeHiZ(int width, int height) {
    m_hizWidth = width;
    m_hizHeight = height;
    
    // Calculate number of mipmap levels
    m_hizLevels = static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;
    
    m_hizPyramid.resize(m_hizLevels);
    
    int currentWidth = width;
    int currentHeight = height;
    
    for (int i = 0; i < m_hizLevels; ++i) {
        m_hizPyramid[i].resize(currentWidth * currentHeight, 1.0f);
        currentWidth = std::max(1, currentWidth / 2);
        currentHeight = std::max(1, currentHeight / 2);
    }
}

void OcclusionCuller::updateHiZ(unsigned int depthTexture) {
    // Copy depth buffer to level 0
    // Then generate mipmaps by taking max depth of 2x2 regions
    
    for (int level = 1; level < m_hizLevels; ++level) {
        generateHiZMipmap(level);
    }
}

bool OcclusionCuller::testHiZ(const BoundingBox& box) const {
    if (m_method != CullingMethod::HierarchicalZ) {
        return true;
    }
    
    // Project bounding box to screen space
    // Sample Hi-Z buffer at appropriate mipmap level
    // Compare depth
    
    return true; // Simplified
}

void OcclusionCuller::addPortal(const float vertices[][3], int count, int roomA, int roomB) {
    Portal portal;
    portal.roomA = roomA;
    portal.roomB = roomB;
    
    for (int i = 0; i < count; ++i) {
        portal.vertices.push_back(vertices[i][0]);
        portal.vertices.push_back(vertices[i][1]);
        portal.vertices.push_back(vertices[i][2]);
    }
    
    m_portals.push_back(portal);
    
    // Resize visible rooms array if needed
    int maxRoom = std::max(roomA, roomB) + 1;
    if (static_cast<int>(m_visibleRooms.size()) < maxRoom) {
        m_visibleRooms.resize(maxRoom, false);
    }
}

void OcclusionCuller::setCurrentRoom(int roomId) {
    m_currentRoom = roomId;
    
    // Mark current room as visible
    if (roomId >= 0 && roomId < static_cast<int>(m_visibleRooms.size())) {
        std::fill(m_visibleRooms.begin(), m_visibleRooms.end(), false);
        m_visibleRooms[roomId] = true;
        
        // Check visibility through portals
        for (const auto& portal : m_portals) {
            if (portal.roomA == roomId || portal.roomB == roomId) {
                int connectedRoom = (portal.roomA == roomId) ? portal.roomB : portal.roomA;
                
                // Test if portal is visible (simplified)
                m_visibleRooms[connectedRoom] = true;
            }
        }
    }
}

bool OcclusionCuller::isRoomVisible(int roomId) const {
    if (roomId >= 0 && roomId < static_cast<int>(m_visibleRooms.size())) {
        return m_visibleRooms[roomId];
    }
    return false;
}

float OcclusionCuller::estimateOcclusionProbability(const BoundingBox& box, const float cameraPos[3]) const {
    // Calculate center of bounding box
    float centerX = (box.minX + box.maxX) * 0.5f;
    float centerY = (box.minY + box.maxY) * 0.5f;
    float centerZ = (box.minZ + box.maxZ) * 0.5f;
    
    // Calculate distance from camera
    float dx = centerX - cameraPos[0];
    float dy = centerY - cameraPos[1];
    float dz = centerZ - cameraPos[2];
    float distanceSquared = dx * dx + dy * dy + dz * dz;
    float distance = std::sqrt(distanceSquared);
    
    // Calculate approximate screen space size
    float screenSize = calculateScreenSpaceSize(box, cameraPos);
    
    // Heuristic factors:
    // 1. Distance factor - further objects more likely occluded
    float distanceFactor = std::min(distance / 100.0f, 1.0f);
    
    // 2. Size factor - smaller objects more likely occluded
    float sizeFactor = std::max(0.0f, 1.0f - screenSize * 10.0f);
    
    // 3. Historical factor - if object was occluded recently, likely to stay occluded
    // (simplified - in real implementation would track per-entity history)
    float historicalFactor = 0.0f;
    
    // Combine factors with weights
    float probability = distanceFactor * 0.4f + sizeFactor * 0.4f + historicalFactor * 0.2f;
    
    return std::max(0.0f, std::min(1.0f, probability));
}

bool OcclusionCuller::isLikelyOccluded(const BoundingBox& box, const float cameraPos[3], float threshold) const {
    return estimateOcclusionProbability(box, cameraPos) >= threshold;
}

float OcclusionCuller::calculateScreenSpaceSize(const BoundingBox& box, const float cameraPos[3]) const {
    // Calculate bounding box dimensions
    float width = box.maxX - box.minX;
    float height = box.maxY - box.minY;
    float depth = box.maxZ - box.minZ;
    float radius = std::sqrt(width * width + height * height + depth * depth) * 0.5f;
    
    // Calculate distance to center
    float centerX = (box.minX + box.maxX) * 0.5f;
    float centerY = (box.minY + box.maxY) * 0.5f;
    float centerZ = (box.minZ + box.maxZ) * 0.5f;
    
    float dx = centerX - cameraPos[0];
    float dy = centerY - cameraPos[1];
    float dz = centerZ - cameraPos[2];
    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    
    // Approximate screen space size using perspective projection
    // Assumes FOV of ~60 degrees and screen height of 1.0
    if (distance < 0.001f) distance = 0.001f;
    return (radius / distance) * 1.732f; // tan(60 degrees) ≈ 1.732
}

float OcclusionCuller::getHistoricalOcclusionRate(int entityId) const {
    // Placeholder - would track occlusion history in real implementation
    return 0.0f;
}

void OcclusionCuller::resetStats() {
    m_stats.totalObjects = 0;
    m_stats.visibleObjects = 0;
    m_stats.frustumCulled = 0;
    m_stats.occlusionCulled = 0;
    m_stats.queryCount = static_cast<int>(m_queries.size());
    m_stats.cullingTime = 0.0f;
}

void OcclusionCuller::update(float deltaTime) {
    updateQueries();
}

void OcclusionCuller::updateCoherenceData(int entityId, bool isVisible, float distance) {
    // Find existing coherence data
    ObjectCoherenceData* data = nullptr;
    for (auto& cd : m_coherenceData) {
        if (cd.entityId == entityId) {
            data = &cd;
            break;
        }
    }
    
    // Create new entry if not found
    if (!data) {
        m_coherenceData.push_back({entityId, isVisible, 0, 0, distance});
        return;
    }
    
    // Update consecutive frame counters
    if (isVisible) {
        data->consecutiveVisibleFrames++;
        data->consecutiveOccludedFrames = 0;
    } else {
        data->consecutiveVisibleFrames = 0;
        data->consecutiveOccludedFrames++;
    }
    
    data->wasVisibleLastFrame = isVisible;
    data->lastTestedDistance = distance;
}

float OcclusionCuller::getPredictedVisibility(int entityId) const {
    for (const auto& cd : m_coherenceData) {
        if (cd.entityId == entityId) {
            // Strong temporal coherence - objects tend to remain in same state
            if (cd.consecutiveVisibleFrames > 5) {
                return 0.9f; // Likely visible
            } else if (cd.consecutiveOccludedFrames > 5) {
                return 0.1f; // Likely occluded
            } else if (cd.wasVisibleLastFrame) {
                return 0.6f; // Probably visible
            } else {
                return 0.4f; // Probably occluded
            }
        }
    }
    
    return 0.5f; // No history, 50/50 chance
}

void OcclusionCuller::clearCoherenceData() {
    m_coherenceData.clear();
}

void OcclusionCuller::getDebugVisualization(DebugVisualizationData& data, bool includePortals, bool includeCulled) const {
    data.frustumPlaneVertices.clear();
    data.culledBoundsVertices.clear();
    data.visibleBoundsVertices.clear();
    data.portalVertices.clear();
    
    // Note: Full implementation would extract frustum corners and build geometry
    // This is a stub that would be completed based on the rendering system
}

void OcclusionCuller::debugDrawFrustum(std::function<void(const float[3], const float[3], const float[4])> drawLineCallback) const {
    if (!drawLineCallback) {
        return;
    }
    
    // Extract frustum corners from planes
    // Note: Simplified implementation - full version would solve plane intersections
    // to get the 8 corners of the frustum and draw the 12 edges
    
    float colorGreen[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    
    // This would draw the frustum wireframe using the callback
    // For now, just a placeholder that shows the pattern
}

void OcclusionCuller::debugDrawBounds(const BoundingBox* boxes, const bool* isVisible, int count,
                                     std::function<void(const float[3], const float[3], const float[4])> drawBoxCallback) const {
    if (!drawBoxCallback || !boxes || !isVisible) {
        return;
    }
    
    for (int i = 0; i < count; ++i) {
        const BoundingBox& box = boxes[i];
        
        // Green for visible, red for culled
        float color[4];
        if (isVisible[i]) {
            color[0] = 0.0f; color[1] = 1.0f; color[2] = 0.0f; color[3] = 1.0f;
        } else {
            color[0] = 1.0f; color[1] = 0.0f; color[2] = 0.0f; color[3] = 0.5f;
        }
        
        float min[3] = {box.minX, box.minY, box.minZ};
        float max[3] = {box.maxX, box.maxY, box.maxZ};
        
        drawBoxCallback(min, max, color);
    }
}

bool OcclusionCuller::testFrustumBox(const BoundingBox& box) const {
    // Test box against all 6 frustum planes
    for (int i = 0; i < 6; ++i) {
        const float* plane = m_frustumPlanes[i];
        
        // Get positive vertex (vertex furthest in plane normal direction)
        float px = (plane[0] > 0.0f) ? box.maxX : box.minX;
        float py = (plane[1] > 0.0f) ? box.maxY : box.minY;
        float pz = (plane[2] > 0.0f) ? box.maxZ : box.minZ;
        
        // Test if positive vertex is outside
        float distance = plane[0] * px + plane[1] * py + plane[2] * pz + plane[3];
        if (distance < 0.0f) {
            return false; // Box is completely outside this plane
        }
    }
    
    return true; // Box is at least partially inside frustum
}

bool OcclusionCuller::testFrustumSphere(const BoundingSphere& sphere) const {
    for (int i = 0; i < 6; ++i) {
        const float* plane = m_frustumPlanes[i];
        
        float distance = plane[0] * sphere.x + plane[1] * sphere.y + plane[2] * sphere.z + plane[3];
        
        if (distance < -sphere.radius) {
            return false; // Sphere is completely outside this plane
        }
    }
    
    return true; // Sphere is at least partially inside frustum
}

void OcclusionCuller::batchTestFrustum(const BoundingBox* boxes, int count, bool* results) const {
    for (int i = 0; i < count; ++i) {
        results[i] = testFrustumBox(boxes[i]);
    }
}

void OcclusionCuller::batchTestFrustum(const BoundingSphere* spheres, int count, bool* results) const {
    for (int i = 0; i < count; ++i) {
        results[i] = testFrustumSphere(spheres[i]);
    }
}

void OcclusionCuller::batchTestFrustumParallel(const BoundingBox* boxes, int count, bool* results, int numThreads) const {
    if (count <= 0) {
        return;
    }
    
    // Determine thread count
    if (numThreads <= 0) {
        numThreads = static_cast<int>(std::thread::hardware_concurrency());
        if (numThreads <= 0) numThreads = 4;
    }
    
    // For small counts, use single-threaded version
    if (count < 100 || numThreads == 1) {
        batchTestFrustum(boxes, count, results);
        return;
    }
    
    // Divide work among threads
    int itemsPerThread = (count + numThreads - 1) / numThreads;
    std::vector<std::future<void>> futures;
    
    for (int t = 0; t < numThreads; ++t) {
        int start = t * itemsPerThread;
        int end = std::min(start + itemsPerThread, count);
        
        if (start >= count) break;
        
        futures.push_back(std::async(std::launch::async, [this, boxes, results, start, end]() {
            for (int i = start; i < end; ++i) {
                results[i] = testFrustumBox(boxes[i]);
            }
        }));
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        future.get();
    }
}

void OcclusionCuller::batchTestFrustumParallel(const BoundingSphere* spheres, int count, bool* results, int numThreads) const {
    if (count <= 0) {
        return;
    }
    
    // Determine thread count
    if (numThreads <= 0) {
        numThreads = static_cast<int>(std::thread::hardware_concurrency());
        if (numThreads <= 0) numThreads = 4;
    }
    
    // For small counts, use single-threaded version
    if (count < 100 || numThreads == 1) {
        batchTestFrustum(spheres, count, results);
        return;
    }
    
    // Divide work among threads
    int itemsPerThread = (count + numThreads - 1) / numThreads;
    std::vector<std::future<void>> futures;
    
    for (int t = 0; t < numThreads; ++t) {
        int start = t * itemsPerThread;
        int end = std::min(start + itemsPerThread, count);
        
        if (start >= count) break;
        
        futures.push_back(std::async(std::launch::async, [this, spheres, results, start, end]() {
            for (int i = start; i < end; ++i) {
                results[i] = testFrustumSphere(spheres[i]);
            }
        }));
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        future.get();
    }
}

void OcclusionCuller::generateHiZMipmap(int level) {
    if (level <= 0 || level >= m_hizLevels) {
        return;
    }
    
    int prevWidth = m_hizWidth >> (level - 1);
    int prevHeight = m_hizHeight >> (level - 1);
    int currentWidth = m_hizWidth >> level;
    int currentHeight = m_hizHeight >> level;
    
    const auto& prevLevel = m_hizPyramid[level - 1];
    auto& currentLevel = m_hizPyramid[level];
    
    for (int y = 0; y < currentHeight; ++y) {
        for (int x = 0; x < currentWidth; ++x) {
            int px = x * 2;
            int py = y * 2;
            
            float d00 = prevLevel[py * prevWidth + px];
            float d10 = (px + 1 < prevWidth) ? prevLevel[py * prevWidth + px + 1] : d00;
            float d01 = (py + 1 < prevHeight) ? prevLevel[(py + 1) * prevWidth + px] : d00;
            float d11 = (px + 1 < prevWidth && py + 1 < prevHeight) ? 
                        prevLevel[(py + 1) * prevWidth + px + 1] : d00;
            
            // Take maximum depth (furthest)
            float maxDepth = std::max({d00, d10, d01, d11});
            currentLevel[y * currentWidth + x] = maxDepth;
        }
    }
}

float OcclusionCuller::sampleHiZ(float x, float y, int level) const {
    if (level < 0 || level >= m_hizLevels) {
        return 1.0f;
    }
    
    int width = m_hizWidth >> level;
    int height = m_hizHeight >> level;
    
    int ix = static_cast<int>(x * width);
    int iy = static_cast<int>(y * height);
    
    ix = std::clamp(ix, 0, width - 1);
    iy = std::clamp(iy, 0, height - 1);
    
    return m_hizPyramid[level][iy * width + ix];
}

// OcclusionSystem implementation
OcclusionSystem::OcclusionSystem()
    : m_cameraX(0.0f)
    , m_cameraY(0.0f)
    , m_cameraZ(0.0f)
    , m_enabled(true)
    , m_debugVisualization(false)
{
}

OcclusionSystem& OcclusionSystem::getInstance() {
    static OcclusionSystem instance;
    return instance;
}

void OcclusionSystem::initialize() {
    m_culler.initializeHiZ(1024, 1024);
}

void OcclusionSystem::shutdown() {
    m_entities.clear();
}

void OcclusionSystem::update(float deltaTime) {
    if (!m_enabled) {
        return;
    }
    
    m_culler.update(deltaTime);
    m_culler.resetStats();
    
    // Test each entity
    for (auto& entity : m_entities) {
        entity.isVisible = true;
        
        // Frustum culling
        if (!m_culler.isInFrustum(entity.bounds)) {
            entity.isVisible = false;
            continue;
        }
        
        // Occlusion culling
        if (m_culler.isOccluded(entity.id)) {
            entity.isVisible = false;
        }
    }
}

void OcclusionSystem::registerEntity(int entityId, const BoundingBox& bounds) {
    EntityInfo info;
    info.id = entityId;
    info.bounds = bounds;
    info.isVisible = true;
    m_entities.push_back(info);
}

void OcclusionSystem::updateEntityBounds(int entityId, const BoundingBox& bounds) {
    for (auto& entity : m_entities) {
        if (entity.id == entityId) {
            entity.bounds = bounds;
            return;
        }
    }
}

void OcclusionSystem::unregisterEntity(int entityId) {
    m_entities.erase(
        std::remove_if(m_entities.begin(), m_entities.end(),
            [entityId](const EntityInfo& e) { return e.id == entityId; }),
        m_entities.end()
    );
}

void OcclusionSystem::setCameraViewProj(const float viewProj[16]) {
    extractFrustumPlanes(viewProj);
}

void OcclusionSystem::setCameraPosition(float x, float y, float z) {
    m_cameraX = x;
    m_cameraY = y;
    m_cameraZ = z;
}

bool OcclusionSystem::isEntityVisible(int entityId) const {
    for (const auto& entity : m_entities) {
        if (entity.id == entityId) {
            return entity.isVisible;
        }
    }
    return false;
}

std::vector<int> OcclusionSystem::getVisibleEntities() const {
    std::vector<int> visible;
    for (const auto& entity : m_entities) {
        if (entity.isVisible) {
            visible.push_back(entity.id);
        }
    }
    return visible;
}

void OcclusionSystem::extractFrustumPlanes(const float viewProj[16]) {
    // Extract frustum planes from view-projection matrix
    float planes[6][4];
    
    // Left plane
    planes[0][0] = viewProj[3] + viewProj[0];
    planes[0][1] = viewProj[7] + viewProj[4];
    planes[0][2] = viewProj[11] + viewProj[8];
    planes[0][3] = viewProj[15] + viewProj[12];
    
    // Right plane
    planes[1][0] = viewProj[3] - viewProj[0];
    planes[1][1] = viewProj[7] - viewProj[4];
    planes[1][2] = viewProj[11] - viewProj[8];
    planes[1][3] = viewProj[15] - viewProj[12];
    
    // Bottom plane
    planes[2][0] = viewProj[3] + viewProj[1];
    planes[2][1] = viewProj[7] + viewProj[5];
    planes[2][2] = viewProj[11] + viewProj[9];
    planes[2][3] = viewProj[15] + viewProj[13];
    
    // Top plane
    planes[3][0] = viewProj[3] - viewProj[1];
    planes[3][1] = viewProj[7] - viewProj[5];
    planes[3][2] = viewProj[11] - viewProj[9];
    planes[3][3] = viewProj[15] - viewProj[13];
    
    // Near plane
    planes[4][0] = viewProj[3] + viewProj[2];
    planes[4][1] = viewProj[7] + viewProj[6];
    planes[4][2] = viewProj[11] + viewProj[10];
    planes[4][3] = viewProj[15] + viewProj[14];
    
    // Far plane
    planes[5][0] = viewProj[3] - viewProj[2];
    planes[5][1] = viewProj[7] - viewProj[6];
    planes[5][2] = viewProj[11] - viewProj[10];
    planes[5][3] = viewProj[15] - viewProj[14];
    
    // Normalize planes
    for (int i = 0; i < 6; ++i) {
        float length = std::sqrt(planes[i][0] * planes[i][0] +
                                planes[i][1] * planes[i][1] +
                                planes[i][2] * planes[i][2]);
        if (length > 0.0f) {
            planes[i][0] /= length;
            planes[i][1] /= length;
            planes[i][2] /= length;
            planes[i][3] /= length;
        }
    }
    
    m_culler.setFrustumPlanes(planes);
}

} // namespace Engine
