#include "scene/SceneTransitions.h"
#include <algorithm>

namespace Engine {

// FadeTransition Implementation
FadeTransition::FadeTransition(float duration)
    : m_duration(duration)
    , m_elapsed(0.0f) {
}

float FadeTransition::update(float deltaTime) {
    m_elapsed += deltaTime;
    return std::min(m_elapsed / m_duration, 1.0f);
}

void FadeTransition::render(unsigned int fromTexture, unsigned int toTexture, float progress) {
    // In a real implementation, render with fade effect
    // This is a stub for the interface
}

void FadeTransition::reset() {
    m_elapsed = 0.0f;
}

// CrossFadeTransition Implementation
CrossFadeTransition::CrossFadeTransition(float duration)
    : m_duration(duration)
    , m_elapsed(0.0f) {
}

float CrossFadeTransition::update(float deltaTime) {
    m_elapsed += deltaTime;
    return std::min(m_elapsed / m_duration, 1.0f);
}

void CrossFadeTransition::render(unsigned int fromTexture, unsigned int toTexture, float progress) {
    // In a real implementation, blend between textures based on progress
}

void CrossFadeTransition::reset() {
    m_elapsed = 0.0f;
}

// WipeTransition Implementation
WipeTransition::WipeTransition(Direction direction, float duration)
    : m_direction(direction)
    , m_duration(duration)
    , m_elapsed(0.0f) {
}

float WipeTransition::update(float deltaTime) {
    m_elapsed += deltaTime;
    return std::min(m_elapsed / m_duration, 1.0f);
}

void WipeTransition::render(unsigned int fromTexture, unsigned int toTexture, float progress) {
    // In a real implementation, render wipe effect based on direction
}

void WipeTransition::reset() {
    m_elapsed = 0.0f;
}

// DissolveTransition Implementation
DissolveTransition::DissolveTransition(float duration)
    : m_duration(duration)
    , m_elapsed(0.0f)
    , m_noiseTexture(0)
    , m_shader(0) {
}

void DissolveTransition::init() {
    // In a real implementation, generate noise texture and load shader
}

float DissolveTransition::update(float deltaTime) {
    m_elapsed += deltaTime;
    return std::min(m_elapsed / m_duration, 1.0f);
}

void DissolveTransition::render(unsigned int fromTexture, unsigned int toTexture, float progress) {
    // In a real implementation, use shader with noise texture for dissolve effect
}

void DissolveTransition::cleanup() {
    // In a real implementation, clean up noise texture and shader
    m_noiseTexture = 0;
    m_shader = 0;
}

void DissolveTransition::reset() {
    m_elapsed = 0.0f;
}

// SceneTransitionManager Implementation
SceneTransitionManager::SceneTransitionManager()
    : m_state(TransitionState::Idle)
    , m_progress(0.0f)
    , m_defaultDuration(1.0f)
    , m_fromTexture(0)
    , m_toTexture(0) {
}

SceneTransitionManager::~SceneTransitionManager() {
}

void SceneTransitionManager::startTransition(const std::string& fromScene,
                                            const std::string& toScene,
                                            TransitionType type,
                                            float duration) {
    if (m_state != TransitionState::Idle) {
        cancelTransition();
    }
    
    m_fromScene = fromScene;
    m_toScene = toScene;
    m_currentTransition = createTransition(type, duration);
    
    if (m_currentTransition) {
        m_currentTransition->init();
        m_state = TransitionState::FadingOut;
        m_progress = 0.0f;
    }
}

void SceneTransitionManager::startCustomTransition(const std::string& fromScene,
                                                  const std::string& toScene,
                                                  std::unique_ptr<SceneTransition> transition) {
    if (m_state != TransitionState::Idle) {
        cancelTransition();
    }
    
    m_fromScene = fromScene;
    m_toScene = toScene;
    m_currentTransition = std::move(transition);
    
    if (m_currentTransition) {
        m_currentTransition->init();
        m_state = TransitionState::FadingOut;
        m_progress = 0.0f;
    }
}

void SceneTransitionManager::update(float deltaTime) {
    if (m_state == TransitionState::Idle || !m_currentTransition) {
        return;
    }
    
    updateState(deltaTime);
}

void SceneTransitionManager::render() {
    if (m_state == TransitionState::Idle || !m_currentTransition) {
        return;
    }
    
    m_currentTransition->render(m_fromTexture, m_toTexture, m_progress);
}

bool SceneTransitionManager::isTransitioning() const {
    return m_state != TransitionState::Idle && m_state != TransitionState::Complete;
}

void SceneTransitionManager::setSceneSwitchCallback(std::function<void(const std::string&)> callback) {
    m_sceneSwitchCallback = callback;
}

void SceneTransitionManager::setCompletionCallback(std::function<void()> callback) {
    m_completionCallback = callback;
}

void SceneTransitionManager::skipTransition() {
    if (m_state != TransitionState::Idle) {
        performSceneSwitch();
        m_state = TransitionState::Complete;
        
        if (m_completionCallback) {
            m_completionCallback();
        }
        
        m_state = TransitionState::Idle;
        m_currentTransition.reset();
    }
}

void SceneTransitionManager::cancelTransition() {
    if (m_currentTransition) {
        m_currentTransition->cleanup();
        m_currentTransition.reset();
    }
    
    m_state = TransitionState::Idle;
    m_progress = 0.0f;
}

void SceneTransitionManager::updateState(float deltaTime) {
    m_progress = m_currentTransition->update(deltaTime);
    
    switch (m_state) {
        case TransitionState::FadingOut:
            if (m_progress >= 0.5f) {
                performSceneSwitch();
                m_state = TransitionState::FadingIn;
            }
            break;
            
        case TransitionState::FadingIn:
            if (m_progress >= 1.0f) {
                m_state = TransitionState::Complete;
                
                if (m_completionCallback) {
                    m_completionCallback();
                }
                
                m_currentTransition->cleanup();
                m_currentTransition.reset();
                m_state = TransitionState::Idle;
            }
            break;
            
        default:
            break;
    }
}

void SceneTransitionManager::performSceneSwitch() {
    if (m_sceneSwitchCallback) {
        m_sceneSwitchCallback(m_toScene);
    }
}

std::unique_ptr<SceneTransition> SceneTransitionManager::createTransition(
    TransitionType type, float duration) {
    
    switch (type) {
        case TransitionType::Fade:
            return std::make_unique<FadeTransition>(duration);
            
        case TransitionType::CrossFade:
            return std::make_unique<CrossFadeTransition>(duration);
            
        case TransitionType::WipeLeft:
            return std::make_unique<WipeTransition>(WipeTransition::Direction::Left, duration);
            
        case TransitionType::WipeRight:
            return std::make_unique<WipeTransition>(WipeTransition::Direction::Right, duration);
            
        case TransitionType::WipeUp:
            return std::make_unique<WipeTransition>(WipeTransition::Direction::Up, duration);
            
        case TransitionType::WipeDown:
            return std::make_unique<WipeTransition>(WipeTransition::Direction::Down, duration);
            
        case TransitionType::Dissolve:
            return std::make_unique<DissolveTransition>(duration);
            
        default:
            return std::make_unique<CrossFadeTransition>(duration);
    }
}

// TransitionSequence Implementation
void TransitionSequence::addTransition(std::unique_ptr<SceneTransition> transition) {
    m_transitions.push_back(std::move(transition));
}

float TransitionSequence::update(float deltaTime) {
    if (m_currentIndex >= m_transitions.size()) {
        return 1.0f;
    }
    
    float progress = m_transitions[m_currentIndex]->update(deltaTime);
    
    if (progress >= 1.0f) {
        m_currentIndex++;
    }
    
    // Return overall progress
    float overallProgress = static_cast<float>(m_currentIndex) / m_transitions.size();
    if (m_currentIndex < m_transitions.size()) {
        overallProgress += progress / m_transitions.size();
    }
    
    return overallProgress;
}

bool TransitionSequence::isComplete() const {
    return m_currentIndex >= m_transitions.size();
}

void TransitionSequence::reset() {
    m_currentIndex = 0;
    for (auto& transition : m_transitions) {
        transition->reset();
    }
}

} // namespace Engine
