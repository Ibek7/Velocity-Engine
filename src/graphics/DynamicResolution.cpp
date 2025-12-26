#include "graphics/DynamicResolution.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace Engine {

DynamicResolutionScaler::DynamicResolutionScaler()
    : m_scalingMode(ScalingMode::Dynamic)
    , m_scalingQuality(ScalingQuality::Balanced)
    , m_nativeWidth(1920)
    , m_nativeHeight(1080)
    , m_currentWidth(1920)
    , m_currentHeight(1080)
    , m_currentScaleFactor(1.0f)
    , m_minScaleFactor(0.5f)
    , m_maxScaleFactor(1.0f)
    , m_scaleStep(0.1f)
    , m_targetFrameTime(16.67f)
    , m_historySize(60)
    , m_historyIndex(0)
    , m_upscaleMethod("bilinear")
    , m_sharpeningEnabled(false)
    , m_sharpeningAmount(0.5f)
    , m_scaleTimer(0.0f)
    , m_scaleDelay(0.5f)
{
    m_frameTimeHistory.resize(m_historySize, m_targetFrameTime);
    
    // Initialize default resolution levels
    addResolutionLevel({0.5f, 960, 540, "Low"});
    addResolutionLevel({0.67f, 1280, 720, "Medium"});
    addResolutionLevel({0.75f, 1440, 810, "High"});
    addResolutionLevel({1.0f, 1920, 1080, "Ultra"});
}

DynamicResolutionScaler::~DynamicResolutionScaler() {
}

void DynamicResolutionScaler::initialize(int nativeWidth, int nativeHeight) {
    m_nativeWidth = nativeWidth;
    m_nativeHeight = nativeHeight;
    m_currentWidth = nativeWidth;
    m_currentHeight = nativeHeight;
    m_currentScaleFactor = 1.0f;
    
    // Update resolution levels based on native resolution
    m_resolutionLevels.clear();
    addResolutionLevel({0.5f, nativeWidth / 2, nativeHeight / 2, "Low"});
    addResolutionLevel({0.67f, static_cast<int>(nativeWidth * 0.67f), static_cast<int>(nativeHeight * 0.67f), "Medium"});
    addResolutionLevel({0.75f, static_cast<int>(nativeWidth * 0.75f), static_cast<int>(nativeHeight * 0.75f), "High"});
    addResolutionLevel({1.0f, nativeWidth, nativeHeight, "Ultra"});
}

void DynamicResolutionScaler::shutdown() {
    m_frameTimeHistory.clear();
    m_resolutionLevels.clear();
}

void DynamicResolutionScaler::update(float deltaTime, float frameTime) {
    if (m_scalingMode == ScalingMode::Fixed) {
        return;
    }
    
    updateFrameTimeHistory(frameTime);
    
    m_scaleTimer += deltaTime;
    if (m_scaleTimer < m_scaleDelay) {
        return;
    }
    
    switch (m_scalingMode) {
        case ScalingMode::Dynamic:
            updateDynamic(frameTime);
            break;
        case ScalingMode::Adaptive:
            updateAdaptive(frameTime);
            break;
        case ScalingMode::TargetFramerate:
            updateTargetFramerate(frameTime);
            break;
        default:
            break;
    }
    
    // Update stats
    m_stats.currentFrameTime = frameTime;
    m_stats.averageFrameTime = calculateAverageFrameTime();
    m_stats.currentFPS = 1000.0f / frameTime;
    m_stats.averageFPS = 1000.0f / m_stats.averageFrameTime;
    m_stats.currentScaleFactor = m_currentScaleFactor;
}

void DynamicResolutionScaler::updateDynamic(float frameTime) {
    float avgFrameTime = calculateAverageFrameTime();
    
    // Determine thresholds based on quality setting
    float scaleUpThreshold = m_targetFrameTime * 0.9f;
    float scaleDownThreshold = m_targetFrameTime * 1.1f;
    
    if (m_scalingQuality == ScalingQuality::Performance) {
        scaleUpThreshold = m_targetFrameTime * 0.85f;
        scaleDownThreshold = m_targetFrameTime * 1.05f;
    } else if (m_scalingQuality == ScalingQuality::Quality) {
        scaleUpThreshold = m_targetFrameTime * 0.95f;
        scaleDownThreshold = m_targetFrameTime * 1.15f;
    }
    
    if (avgFrameTime > scaleDownThreshold && m_currentScaleFactor > m_minScaleFactor) {
        scaleDown();
    } else if (avgFrameTime < scaleUpThreshold && m_currentScaleFactor < m_maxScaleFactor) {
        scaleUp();
    }
}

void DynamicResolutionScaler::updateAdaptive(float frameTime) {
    // Similar to dynamic but considers GPU load
    float avgFrameTime = calculateAverageFrameTime();
    float frameDiff = avgFrameTime - m_targetFrameTime;
    
    // Calculate desired scale factor
    float desiredScale = m_currentScaleFactor;
    if (frameDiff > 2.0f) {
        desiredScale -= m_scaleStep;
    } else if (frameDiff < -2.0f) {
        desiredScale += m_scaleStep;
    }
    
    desiredScale = std::clamp(desiredScale, m_minScaleFactor, m_maxScaleFactor);
    
    if (desiredScale != m_currentScaleFactor) {
        applyScaleFactor(desiredScale);
        m_scaleTimer = 0.0f;
    }
}

void DynamicResolutionScaler::updateTargetFramerate(float frameTime) {
    float avgFrameTime = calculateAverageFrameTime();
    
    if (avgFrameTime > m_targetFrameTime && m_currentScaleFactor > m_minScaleFactor) {
        scaleDown();
    } else if (avgFrameTime < m_targetFrameTime * 0.9f && m_currentScaleFactor < m_maxScaleFactor) {
        scaleUp();
    }
}

void DynamicResolutionScaler::scaleUp() {
    float newScale = std::min(m_currentScaleFactor + m_scaleStep, m_maxScaleFactor);
    applyScaleFactor(newScale);
    m_stats.scaleUpCount++;
    m_scaleTimer = 0.0f;
}

void DynamicResolutionScaler::scaleDown() {
    float newScale = std::max(m_currentScaleFactor - m_scaleStep, m_minScaleFactor);
    applyScaleFactor(newScale);
    m_stats.scaleDownCount++;
    m_scaleTimer = 0.0f;
}

void DynamicResolutionScaler::applyScaleFactor(float factor) {
    m_currentScaleFactor = std::clamp(factor, m_minScaleFactor, m_maxScaleFactor);
    m_currentWidth = static_cast<int>(m_nativeWidth * m_currentScaleFactor);
    m_currentHeight = static_cast<int>(m_nativeHeight * m_currentScaleFactor);
    
    // Ensure even dimensions for better compatibility
    m_currentWidth = (m_currentWidth / 2) * 2;
    m_currentHeight = (m_currentHeight / 2) * 2;
}

void DynamicResolutionScaler::forceScaleFactor(float factor) {
    applyScaleFactor(factor);
    m_scaleTimer = 0.0f;
}

ResolutionLevel DynamicResolutionScaler::getCurrentResolution() const {
    ResolutionLevel level;
    level.scaleFactor = m_currentScaleFactor;
    level.width = m_currentWidth;
    level.height = m_currentHeight;
    level.label = "Custom";
    
    // Find matching preset
    for (const auto& preset : m_resolutionLevels) {
        if (std::abs(preset.scaleFactor - m_currentScaleFactor) < 0.01f) {
            level.label = preset.label;
            break;
        }
    }
    
    return level;
}

float DynamicResolutionScaler::getAverageFrameTime() const {
    return calculateAverageFrameTime();
}

float DynamicResolutionScaler::getAverageFPS() const {
    float avgFrameTime = calculateAverageFrameTime();
    return avgFrameTime > 0.0f ? 1000.0f / avgFrameTime : 0.0f;
}

void DynamicResolutionScaler::addResolutionLevel(const ResolutionLevel& level) {
    m_resolutionLevels.push_back(level);
    
    // Sort by scale factor
    std::sort(m_resolutionLevels.begin(), m_resolutionLevels.end(),
        [](const ResolutionLevel& a, const ResolutionLevel& b) {
            return a.scaleFactor < b.scaleFactor;
        });
}

void DynamicResolutionScaler::setResolutionLevel(int index) {
    if (index >= 0 && index < static_cast<int>(m_resolutionLevels.size())) {
        const auto& level = m_resolutionLevels[index];
        applyScaleFactor(level.scaleFactor);
    }
}

void DynamicResolutionScaler::resetStats() {
    m_stats = Stats();
}

void DynamicResolutionScaler::updateFrameTimeHistory(float frameTime) {
    m_frameTimeHistory[m_historyIndex] = frameTime;
    m_historyIndex = (m_historyIndex + 1) % m_historySize;
}

float DynamicResolutionScaler::calculateAverageFrameTime() const {
    float sum = std::accumulate(m_frameTimeHistory.begin(), m_frameTimeHistory.end(), 0.0f);
    return sum / m_historySize;
}

// DynamicResolutionSystem implementation
DynamicResolutionSystem::DynamicResolutionSystem()
    : m_enabled(true)
{
}

DynamicResolutionSystem& DynamicResolutionSystem::getInstance() {
    static DynamicResolutionSystem instance;
    return instance;
}

void DynamicResolutionSystem::initialize(int nativeWidth, int nativeHeight) {
    m_scaler.initialize(nativeWidth, nativeHeight);
}

void DynamicResolutionSystem::shutdown() {
    m_scaler.shutdown();
}

void DynamicResolutionSystem::update(float deltaTime, float frameTime) {
    if (m_enabled) {
        m_scaler.update(deltaTime, frameTime);
    }
}

} // namespace Engine
