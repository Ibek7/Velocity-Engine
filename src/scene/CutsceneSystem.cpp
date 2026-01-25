#include "scene/CutsceneSystem.h"
#include <algorithm>

namespace JJM {
namespace Scene {

// CutsceneInstance implementation

CutsceneInstance::CutsceneInstance(const Cutscene& cutscene)
    : m_cutscene(cutscene)
    , m_state(CutsceneState::STOPPED)
    , m_currentTime(0.0f)
{
    m_eventTriggered.resize(cutscene.events.size(), false);
}

CutsceneInstance::~CutsceneInstance() {
}

void CutsceneInstance::update(float deltaTime) {
    if (m_state != CutsceneState::PLAYING) return;
    
    m_currentTime += deltaTime;
    
    // Check if cutscene is complete
    if (m_currentTime >= m_cutscene.duration) {
        m_state = CutsceneState::COMPLETED;
        return;
    }
    
    // Process events
    processEvents(deltaTime);
}

void CutsceneInstance::play() {
    m_state = CutsceneState::PLAYING;
    m_currentTime = 0.0f;
    std::fill(m_eventTriggered.begin(), m_eventTriggered.end(), false);
}

void CutsceneInstance::pause() {
    if (m_state == CutsceneState::PLAYING) {
        m_state = CutsceneState::PAUSED;
    }
}

void CutsceneInstance::resume() {
    if (m_state == CutsceneState::PAUSED) {
        m_state = CutsceneState::PLAYING;
    }
}

void CutsceneInstance::stop() {
    m_state = CutsceneState::STOPPED;
    m_currentTime = 0.0f;
    std::fill(m_eventTriggered.begin(), m_eventTriggered.end(), false);
}

void CutsceneInstance::skip() {
    m_currentTime = m_cutscene.duration;
    m_state = CutsceneState::COMPLETED;
}

void CutsceneInstance::seek(float time) {
    m_currentTime = std::max(0.0f, std::min(time, m_cutscene.duration));
    
    // Reset event triggers for events before current time
    for (size_t i = 0; i < m_cutscene.events.size(); i++) {
        if (m_cutscene.events[i].startTime > m_currentTime) {
            m_eventTriggered[i] = false;
        }
    }
}

float CutsceneInstance::getProgress() const {
    if (m_cutscene.duration <= 0.0f) return 0.0f;
    return m_currentTime / m_cutscene.duration;
}

void CutsceneInstance::triggerEvent(const CutsceneEvent& event) {
    // Call callback if set
    if (event.callback) {
        event.callback();
    }
    
    // Process event based on type
    // Note: Actual implementation would integrate with game systems
    // This is just the framework
}

void CutsceneInstance::processEvents(float deltaTime) {
    for (size_t i = 0; i < m_cutscene.events.size(); i++) {
        if (m_eventTriggered[i]) continue;
        
        const CutsceneEvent& event = m_cutscene.events[i];
        
        // Check if event should trigger
        if (m_currentTime >= event.startTime) {
            triggerEvent(event);
            m_eventTriggered[i] = true;
            
            // If blocking, pause until event completes
            if (event.blocking && event.duration > 0.0f) {
                // TODO: Handle blocking events
            }
        }
    }
}

// CutsceneSystem implementation

CutsceneSystem::CutsceneSystem()
    : m_currentCutscene(nullptr)
    , m_enabled(true)
    , m_defaultSkippable(true)
    , m_timeScale(1.0f)
{
}

CutsceneSystem::~CutsceneSystem() {
    shutdown();
}

void CutsceneSystem::initialize() {
    m_cutscenes.clear();
    m_currentCutscene = nullptr;
}

void CutsceneSystem::shutdown() {
    if (m_currentCutscene) {
        delete m_currentCutscene;
        m_currentCutscene = nullptr;
    }
    
    m_cutscenes.clear();
    
    m_cutsceneStartedCallback = nullptr;
    m_cutsceneCompletedCallback = nullptr;
    m_cutsceneSkippedCallback = nullptr;
    m_cameraEventCallback = nullptr;
    m_subtitleEventCallback = nullptr;
}

void CutsceneSystem::update(float deltaTime) {
    if (!m_enabled) return;
    
    if (m_currentCutscene) {
        m_currentCutscene->update(deltaTime * m_timeScale);
        
        // Check if completed
        if (m_currentCutscene->isComplete()) {
            std::string cutsceneId = m_currentCutscene->getId();
            
            // Callback
            if (m_cutsceneCompletedCallback) {
                m_cutsceneCompletedCallback(cutsceneId);
            }
            
            // Clean up
            delete m_currentCutscene;
            m_currentCutscene = nullptr;
        }
    }
}

void CutsceneSystem::registerCutscene(const Cutscene& cutscene) {
    m_cutscenes[cutscene.id] = cutscene;
}

int CutsceneSystem::loadCutscenes(const std::string& filepath) {
    // TODO: Implement file loading
    // Would parse cutscene definitions from JSON/XML
    return 0;
}

const Cutscene* CutsceneSystem::getCutscene(const std::string& id) const {
    auto it = m_cutscenes.find(id);
    if (it != m_cutscenes.end()) {
        return &it->second;
    }
    return nullptr;
}

bool CutsceneSystem::hasCutscene(const std::string& id) const {
    return m_cutscenes.find(id) != m_cutscenes.end();
}

bool CutsceneSystem::playCutscene(const std::string& id) {
    if (!m_enabled) return false;
    
    // Stop current cutscene if playing
    if (m_currentCutscene) {
        stopCurrentCutscene();
    }
    
    // Find cutscene
    const Cutscene* cutscene = getCutscene(id);
    if (!cutscene) return false;
    
    // Create instance
    m_currentCutscene = new CutsceneInstance(*cutscene);
    m_currentCutscene->play();
    
    // Callback
    if (m_cutsceneStartedCallback) {
        m_cutsceneStartedCallback(id);
    }
    
    return true;
}

void CutsceneSystem::stopCurrentCutscene() {
    if (m_currentCutscene) {
        m_currentCutscene->stop();
        delete m_currentCutscene;
        m_currentCutscene = nullptr;
    }
}

void CutsceneSystem::pauseCurrentCutscene() {
    if (m_currentCutscene) {
        m_currentCutscene->pause();
    }
}

void CutsceneSystem::resumeCurrentCutscene() {
    if (m_currentCutscene) {
        m_currentCutscene->resume();
    }
}

void CutsceneSystem::skipCurrentCutscene() {
    if (m_currentCutscene) {
        std::string cutsceneId = m_currentCutscene->getId();
        
        m_currentCutscene->skip();
        
        // Callback
        if (m_cutsceneSkippedCallback) {
            m_cutsceneSkippedCallback(cutsceneId);
        }
        
        // Clean up
        delete m_currentCutscene;
        m_currentCutscene = nullptr;
    }
}

bool CutsceneSystem::isPlayingCutscene() const {
    return m_currentCutscene != nullptr && m_currentCutscene->isPlaying();
}

std::string CutsceneSystem::getCurrentCutsceneId() const {
    if (m_currentCutscene) {
        return m_currentCutscene->getId();
    }
    return "";
}

float CutsceneSystem::getCurrentProgress() const {
    if (m_currentCutscene) {
        return m_currentCutscene->getProgress();
    }
    return 0.0f;
}

// Event builders

CutsceneEvent CutsceneSystem::createCameraMoveEvent(
    float startTime,
    const float position[3],
    float duration,
    CameraMovementType movementType)
{
    CutsceneEvent event;
    event.type = CutsceneEventType::CAMERA_MOVE;
    event.startTime = startTime;
    event.duration = duration;
    event.position[0] = position[0];
    event.position[1] = position[1];
    event.position[2] = position[2];
    event.camera.movementType = movementType;
    event.camera.duration = duration;
    return event;
}

CutsceneEvent CutsceneSystem::createSubtitleEvent(
    float startTime,
    const std::string& text,
    const std::string& speaker,
    float duration)
{
    CutsceneEvent event;
    event.type = CutsceneEventType::SHOW_SUBTITLE;
    event.startTime = startTime;
    event.duration = duration;
    event.subtitle.text = text;
    event.subtitle.speaker = speaker;
    event.subtitle.duration = duration;
    return event;
}

CutsceneEvent CutsceneSystem::createAnimationEvent(
    float startTime,
    int entityId,
    const std::string& animationName,
    bool blocking)
{
    CutsceneEvent event;
    event.type = CutsceneEventType::PLAY_ANIMATION;
    event.startTime = startTime;
    event.entityId = entityId;
    event.stringData = animationName;
    event.blocking = blocking;
    return event;
}

CutsceneEvent CutsceneSystem::createWaitEvent(float startTime, float duration) {
    CutsceneEvent event;
    event.type = CutsceneEventType::WAIT;
    event.startTime = startTime;
    event.duration = duration;
    event.blocking = true;
    return event;
}

CutsceneEvent CutsceneSystem::createFadeEvent(float startTime, bool fadeOut, float duration) {
    CutsceneEvent event;
    event.type = fadeOut ? CutsceneEventType::FADE_OUT : CutsceneEventType::FADE_IN;
    event.startTime = startTime;
    event.duration = duration;
    return event;
}

// Callbacks

void CutsceneSystem::setCutsceneStartedCallback(std::function<void(const std::string&)> callback) {
    m_cutsceneStartedCallback = callback;
}

void CutsceneSystem::setCutsceneCompletedCallback(std::function<void(const std::string&)> callback) {
    m_cutsceneCompletedCallback = callback;
}

void CutsceneSystem::setCutsceneSkippedCallback(std::function<void(const std::string&)> callback) {
    m_cutsceneSkippedCallback = callback;
}

void CutsceneSystem::setCameraEventCallback(std::function<void(const CutsceneCamera&)> callback) {
    m_cameraEventCallback = callback;
}

void CutsceneSystem::setSubtitleEventCallback(std::function<void(const Subtitle&)> callback) {
    m_subtitleEventCallback = callback;
}

} // namespace Scene
} // namespace JJM
