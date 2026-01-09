#include "graphics/AdvancedOcclusionCulling.h"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace JJM {
namespace Graphics {

// =============================================================================
// EnhancedFrustumCuller Implementation
// =============================================================================

void EnhancedFrustumCuller::extractFromMatrix(const float* viewProjMatrix) {
    // Extract frustum planes from view-projection matrix
    // Left plane
    m_planes[0][0] = viewProjMatrix[3] + viewProjMatrix[0];
    m_planes[0][1] = viewProjMatrix[7] + viewProjMatrix[4];
    m_planes[0][2] = viewProjMatrix[11] + viewProjMatrix[8];
    m_planes[0][3] = viewProjMatrix[15] + viewProjMatrix[12];
    
    // Right plane
    m_planes[1][0] = viewProjMatrix[3] - viewProjMatrix[0];
    m_planes[1][1] = viewProjMatrix[7] - viewProjMatrix[4];
    m_planes[1][2] = viewProjMatrix[11] - viewProjMatrix[8];
    m_planes[1][3] = viewProjMatrix[15] - viewProjMatrix[12];
    
    // Bottom plane
    m_planes[2][0] = viewProjMatrix[3] + viewProjMatrix[1];
    m_planes[2][1] = viewProjMatrix[7] + viewProjMatrix[5];
    m_planes[2][2] = viewProjMatrix[11] + viewProjMatrix[9];
    m_planes[2][3] = viewProjMatrix[15] + viewProjMatrix[13];
    
    // Top plane
    m_planes[3][0] = viewProjMatrix[3] - viewProjMatrix[1];
    m_planes[3][1] = viewProjMatrix[7] - viewProjMatrix[5];
    m_planes[3][2] = viewProjMatrix[11] - viewProjMatrix[9];
    m_planes[3][3] = viewProjMatrix[15] - viewProjMatrix[13];
    
    // Near plane
    m_planes[4][0] = viewProjMatrix[3] + viewProjMatrix[2];
    m_planes[4][1] = viewProjMatrix[7] + viewProjMatrix[6];
    m_planes[4][2] = viewProjMatrix[11] + viewProjMatrix[10];
    m_planes[4][3] = viewProjMatrix[15] + viewProjMatrix[14];
    
    // Far plane
    m_planes[5][0] = viewProjMatrix[3] - viewProjMatrix[2];
    m_planes[5][1] = viewProjMatrix[7] - viewProjMatrix[6];
    m_planes[5][2] = viewProjMatrix[11] - viewProjMatrix[10];
    m_planes[5][3] = viewProjMatrix[15] - viewProjMatrix[14];
    
    // Normalize planes
    for (int i = 0; i < 6; ++i) {
        float length = std::sqrt(m_planes[i][0] * m_planes[i][0] +
                                m_planes[i][1] * m_planes[i][1] +
                                m_planes[i][2] * m_planes[i][2]);
        if (length > 0.0f) {
            m_planes[i][0] /= length;
            m_planes[i][1] /= length;
            m_planes[i][2] /= length;
            m_planes[i][3] /= length;
        }
    }
}

bool EnhancedFrustumCuller::testAABB(const float* min, const float* max) const {
    // Test AABB against all 6 frustum planes
    for (int i = 0; i < 6; ++i) {
        // Find positive vertex
        float px = (m_planes[i][0] >= 0) ? max[0] : min[0];
        float py = (m_planes[i][1] >= 0) ? max[1] : min[1];
        float pz = (m_planes[i][2] >= 0) ? max[2] : min[2];
        
        float distance = m_planes[i][0] * px + m_planes[i][1] * py +
                        m_planes[i][2] * pz + m_planes[i][3];
        
        if (distance < 0) {
            return false;  // Outside frustum
        }
    }
    
    return true;  // Inside or intersecting frustum
}

bool EnhancedFrustumCuller::testSphere(const float* center, float radius) const {
    for (int i = 0; i < 6; ++i) {
        float distance = m_planes[i][0] * center[0] + m_planes[i][1] * center[1] +
                        m_planes[i][2] * center[2] + m_planes[i][3];
        
        if (distance < -radius) {
            return false;
        }
    }
    
    return true;
}

bool EnhancedFrustumCuller::testAABBFast(const float* min, const float* max) const {
    // Fast approximate test - only check against near/far planes
    float center[3] = {
        (min[0] + max[0]) * 0.5f,
        (min[1] + max[1]) * 0.5f,
        (min[2] + max[2]) * 0.5f
    };
    
    float radius = std::sqrt(
        (max[0] - min[0]) * (max[0] - min[0]) +
        (max[1] - min[1]) * (max[1] - min[1]) +
        (max[2] - min[2]) * (max[2] - min[2])
    ) * 0.5f;
    
    return testSphere(center, radius);
}

// =============================================================================
// GPUOcclusionQueryManager Implementation
// =============================================================================

GPUOcclusionQueryManager::GPUOcclusionQueryManager()
    : m_currentFrame(0)
    , m_queryBudget(100)
    , m_conservativeRasterization(false)
{}

GPUOcclusionQueryManager::~GPUOcclusionQueryManager() {
    shutdown();
}

void GPUOcclusionQueryManager::initialize(int maxQueries) {
    m_queries.reserve(maxQueries);
    
    // Pre-allocate query objects
    for (int i = 0; i < maxQueries; ++i) {
        unsigned int queryID;
        glGenQueries(1, &queryID);
        m_freeQueries.push(queryID);
    }
}

void GPUOcclusionQueryManager::shutdown() {
    while (!m_freeQueries.empty()) {
        unsigned int queryID = m_freeQueries.front();
        m_freeQueries.pop();
        glDeleteQueries(1, &queryID);
    }
    
    for (auto& query : m_queries) {
        glDeleteQueries(1, &query.queryID);
    }
    m_queries.clear();
}

bool GPUOcclusionQueryManager::issueQuery(unsigned int objectID, const float* min, const float* max) {
    if (m_freeQueries.empty()) {
        return false;  // No queries available
    }
    
    unsigned int queryID = m_freeQueries.front();
    m_freeQueries.pop();
    
    // Begin occlusion query
    glBeginQuery(GL_ANY_SAMPLES_PASSED, queryID);
    
    // Render bounding box
    renderBoundingBox(min, max);
    
    // End query
    glEndQuery(GL_ANY_SAMPLES_PASSED);
    
    // Store query state
    QueryState state;
    state.queryID = queryID;
    state.objectID = objectID;
    state.frameIssued = m_currentFrame;
    state.resultAvailable = false;
    state.samplesPassed = 0;
    m_queries.push_back(state);
    
    return true;
}

void GPUOcclusionQueryManager::collectResults(std::unordered_map<unsigned int, bool>& visibilityMap) {
    auto it = m_queries.begin();
    while (it != m_queries.end()) {
        GLint available = 0;
        glGetQueryObjectiv(it->queryID, GL_QUERY_RESULT_AVAILABLE, &available);
        
        if (available) {
            GLuint samplesPassed = 0;
            glGetQueryObjectuiv(it->queryID, GL_QUERY_RESULT, &samplesPassed);
            
            visibilityMap[it->objectID] = (samplesPassed > 0);
            
            // Return query to pool
            m_freeQueries.push(it->queryID);
            it = m_queries.erase(it);
        } else {
            ++it;
        }
    }
}

void GPUOcclusionQueryManager::renderBoundingBox(const float* min, const float* max) {
    // Render simple bounding box for occlusion test
    // TODO: Implement efficient box rendering (index buffer, etc.)
    
    glDisable(GL_CULL_FACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    
    // Draw box vertices
    // ... box drawing code ...
    
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

// =============================================================================
// SoftwareHiZBuffer Implementation
// =============================================================================

SoftwareHiZBuffer::SoftwareHiZBuffer()
    : m_baseWidth(0)
    , m_baseHeight(0)
    , m_levels(0)
{}

SoftwareHiZBuffer::~SoftwareHiZBuffer() = default;

void SoftwareHiZBuffer::initialize(int width, int height) {
    m_baseWidth = width;
    m_baseHeight = height;
    m_levels = static_cast<int>(std::log2(std::max(width, height))) + 1;
    
    m_depthPyramid.resize(m_levels);
    
    int w = width;
    int h = height;
    for (int i = 0; i < m_levels; ++i) {
        m_depthPyramid[i].resize(w * h, 0.0f);
        w = std::max(1, w / 2);
        h = std::max(1, h / 2);
    }
}

void SoftwareHiZBuffer::buildFromDepth(const float* depthData) {
    // Copy base level
    std::memcpy(m_depthPyramid[0].data(), depthData,
               m_baseWidth * m_baseHeight * sizeof(float));
    
    // Build pyramid levels
    for (int i = 1; i < m_levels; ++i) {
        downsampleLevel(i);
    }
}

bool SoftwareHiZBuffer::testAABB(const float* screenMin, const float* screenMax, float minZ) const {
    // Calculate appropriate mip level based on screen size
    float width = screenMax[0] - screenMin[0];
    float height = screenMax[1] - screenMin[1];
    int level = getMipLevelForSize(width, height);
    
    // Sample Hi-Z buffer conservatively (max depth)
    int x0 = static_cast<int>(screenMin[0]) >> level;
    int y0 = static_cast<int>(screenMin[1]) >> level;
    int x1 = static_cast<int>(screenMax[0]) >> level;
    int y1 = static_cast<int>(screenMax[1]) >> level;
    
    float maxDepth = sampleDepthConservative(x0, y0, level);
    
    // If min Z is behind max depth in Hi-Z, object is occluded
    return minZ > maxDepth;
}

float SoftwareHiZBuffer::getDepth(int x, int y, int mipLevel) const {
    if (mipLevel >= m_levels) return 0.0f;
    
    int levelWidth = std::max(1, m_baseWidth >> mipLevel);
    if (x >= levelWidth || y >= (m_baseHeight >> mipLevel)) return 0.0f;
    
    return m_depthPyramid[mipLevel][y * levelWidth + x];
}

void SoftwareHiZBuffer::downsampleLevel(int level) {
    int srcWidth = std::max(1, m_baseWidth >> (level - 1));
    int srcHeight = std::max(1, m_baseHeight >> (level - 1));
    int dstWidth = std::max(1, m_baseWidth >> level);
    int dstHeight = std::max(1, m_baseHeight >> level);
    
    for (int y = 0; y < dstHeight; ++y) {
        for (int x = 0; x < dstWidth; ++x) {
            // Sample 2x2 from previous level and take maximum
            float d0 = m_depthPyramid[level - 1][(y * 2) * srcWidth + (x * 2)];
            float d1 = m_depthPyramid[level - 1][(y * 2) * srcWidth + (x * 2 + 1)];
            float d2 = m_depthPyramid[level - 1][(y * 2 + 1) * srcWidth + (x * 2)];
            float d3 = m_depthPyramid[level - 1][(y * 2 + 1) * srcWidth + (x * 2 + 1)];
            
            m_depthPyramid[level][y * dstWidth + x] = std::max({d0, d1, d2, d3});
        }
    }
}

float SoftwareHiZBuffer::sampleDepthConservative(int x, int y, int level) const {
    return getDepth(x, y, level);
}

int SoftwareHiZBuffer::getMipLevelForSize(float screenWidth, float screenHeight) const {
    float size = std::max(screenWidth, screenHeight);
    return std::max(0, static_cast<int>(std::log2(size)));
}

// =============================================================================
// AdvancedOcclusionCuller Implementation
// =============================================================================

AdvancedOcclusionCuller::AdvancedOcclusionCuller()
    : m_useFrustumCulling(true)
    , m_useOcclusionQueries(false)
    , m_useHiZ(false)
    , m_useTemporalCoherence(true)
    , m_currentFrame(0)
{
    m_queryManager = std::make_unique<GPUOcclusionQueryManager>();
    m_hiZBuffer = std::make_unique<SoftwareHiZBuffer>();
}

AdvancedOcclusionCuller::~AdvancedOcclusionCuller() {
    shutdown();
}

void AdvancedOcclusionCuller::initialize() {
    m_queryManager->initialize();
    m_hiZBuffer->initialize(1920, 1080);  // TODO: Get from viewport
}

void AdvancedOcclusionCuller::shutdown() {
    m_queryManager->shutdown();
    m_objects.clear();
}

void AdvancedOcclusionCuller::registerObject(unsigned int id, const float* min, const float* max, float importance) {
    ObjectState state;
    state.id = id;
    std::memcpy(state.bounds, min, sizeof(float) * 3);
    std::memcpy(state.bounds + 3, max, sizeof(float) * 3);
    state.lastVisibleFrame = m_currentFrame;
    state.visibilityHistory = 0xFFFFFFFF;  // Assume visible initially
    state.importance = importance;
    m_objects[id] = state;
}

void AdvancedOcclusionCuller::unregisterObject(unsigned int id) {
    m_objects.erase(id);
}

void AdvancedOcclusionCuller::updateBounds(unsigned int id, const float* min, const float* max) {
    auto it = m_objects.find(id);
    if (it != m_objects.end()) {
        std::memcpy(it->second.bounds, min, sizeof(float) * 3);
        std::memcpy(it->second.bounds + 3, max, sizeof(float) * 3);
    }
}

std::vector<unsigned int> AdvancedOcclusionCuller::cull(const float* viewProjMatrix, const float* depthBuffer) {
    clearStats();
    m_stats.totalObjects = static_cast<int>(m_objects.size());
    
    // Extract frustum
    if (m_useFrustumCulling) {
        m_frustumCuller.extractFromMatrix(viewProjMatrix);
    }
    
    // Build Hi-Z if provided
    if (m_useHiZ && depthBuffer) {
        m_hiZBuffer->buildFromDepth(depthBuffer);
    }
    
    // Gather candidates
    std::vector<ObjectState*> candidates;
    candidates.reserve(m_objects.size());
    for (auto& [id, obj] : m_objects) {
        candidates.push_back(&obj);
    }
    
    // Frustum culling
    if (m_useFrustumCulling) {
        performFrustumCulling(candidates);
    }
    
    // Hi-Z testing
    if (m_useHiZ && depthBuffer) {
        performHiZTest(candidates);
    }
    
    // Occlusion queries
    if (m_useOcclusionQueries) {
        performOcclusionQueries(candidates);
    }
    
    // Collect visible objects
    std::vector<unsigned int> visible;
    for (const auto* obj : candidates) {
        visible.push_back(obj->id);
    }
    
    m_stats.visible = static_cast<int>(visible.size());
    m_currentFrame++;
    m_queryManager->nextFrame();
    
    return visible;
}

void AdvancedOcclusionCuller::performFrustumCulling(std::vector<ObjectState*>& candidates) {
    auto it = candidates.begin();
    while (it != candidates.end()) {
        if (!m_frustumCuller.testAABB((*it)->bounds, (*it)->bounds + 3)) {
            m_stats.frustumCulled++;
            it = candidates.erase(it);
        } else {
            ++it;
        }
    }
}

void AdvancedOcclusionCuller::performOcclusionQueries(std::vector<ObjectState*>& candidates) {
    prioritizeQueries(candidates);
    
    int queriesIssued = 0;
    for (auto* obj : candidates) {
        if (queriesIssued >= m_queryManager->m_queryBudget) break;
        
        if (m_queryManager->issueQuery(obj->id, obj->bounds, obj->bounds + 3)) {
            queriesIssued++;
        }
    }
    
    m_stats.queriesIssued = queriesIssued;
    
    // Collect results from previous frames
    std::unordered_map<unsigned int, bool> visibilityMap;
    m_queryManager->collectResults(visibilityMap);
}

void AdvancedOcclusionCuller::performHiZTest(std::vector<ObjectState*>& candidates) {
    // TODO: Implement Hi-Z testing
    m_stats.hizTests = static_cast<int>(candidates.size());
}

void AdvancedOcclusionCuller::prioritizeQueries(std::vector<ObjectState*>& candidates) {
    std::sort(candidates.begin(), candidates.end(),
        [this](const ObjectState* a, const ObjectState* b) {
            return calculateImportance(*a) > calculateImportance(*b);
        });
}

float AdvancedOcclusionCuller::calculateImportance(const ObjectState& obj) const {
    float recency = static_cast<float>(m_currentFrame - obj.lastVisibleFrame);
    float historyScore = __builtin_popcount(obj.visibilityHistory) / 32.0f;
    return obj.importance * (1.0f / (1.0f + recency)) * historyScore;
}

void AdvancedOcclusionCuller::clearStats() {
    m_stats = CullStats();
}

void AdvancedOcclusionCuller::setQueryBudget(int budget) {
    m_queryManager->setQueryBudget(budget);
}

void AdvancedOcclusionCuller::updateTemporalCoherence() {
    // Update visibility history for temporal coherence
    for (auto& [id, obj] : m_objects) {
        obj.visibilityHistory <<= 1;
        // Bit 0 will be set by visibility tests
    }
}

} // namespace Graphics
} // namespace JJM
