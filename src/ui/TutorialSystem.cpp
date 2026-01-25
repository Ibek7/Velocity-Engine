#include "ui/TutorialSystem.h"
#include <algorithm>
#include <fstream>

namespace JJM {
namespace UI {

TutorialSystem::TutorialSystem()
    : m_activeTutorial(nullptr), m_stepElapsedTime(0.0f), m_stepDelayTime(0.0f),
      m_activeHint(nullptr), m_hintElapsedTime(0.0f),
      m_tutorialsEnabled(true), m_hintsEnabled(true), m_tutorialSpeed(1.0f) {
}

TutorialSystem::~TutorialSystem() {
    shutdown();
}

void TutorialSystem::initialize() {
    m_stats = Statistics();
}

void TutorialSystem::shutdown() {
    stopTutorial();
    m_tutorials.clear();
    m_hints.clear();
    m_completedTutorials.clear();
    m_shownHints.clear();
}

void TutorialSystem::update(float deltaTime) {
    if (!m_tutorialsEnabled && !m_hintsEnabled) return;
    
    deltaTime *= m_tutorialSpeed;
    
    if (m_tutorialsEnabled && m_activeTutorial) {
        updateTutorial(deltaTime);
    }
    
    if (m_hintsEnabled && m_activeHint) {
        updateHints(deltaTime);
    }
}

bool TutorialSystem::registerTutorial(const Tutorial& tutorial) {
    if (tutorial.id.empty()) return false;
    
    m_tutorials[tutorial.id] = tutorial;
    m_stats.totalTutorials++;
    
    return true;
}

int TutorialSystem::loadTutorials(const std::string& filepath) {
    // TODO: Implement JSON/XML loading
    return 0;
}

Tutorial* TutorialSystem::getTutorial(const std::string& id) {
    auto it = m_tutorials.find(id);
    return (it != m_tutorials.end()) ? &it->second : nullptr;
}

bool TutorialSystem::startTutorial(const std::string& id) {
    if (!m_tutorialsEnabled) return false;
    
    auto* tutorial = getTutorial(id);
    if (!tutorial) return false;
    
    // Check if already completed and should only show once
    if (tutorial->showOnce && isCompleted(id)) {
        return false;
    }
    
    // Stop current tutorial if any
    if (m_activeTutorial) {
        stopTutorial();
    }
    
    m_activeTutorial = tutorial;
    m_activeTutorial->currentStepIndex = 0;
    m_stepElapsedTime = 0.0f;
    m_stepDelayTime = 0.0f;
    
    if (m_tutorialStartCallback) {
        m_tutorialStartCallback(m_activeTutorial);
    }
    
    notifyStepChange();
    
    return true;
}

void TutorialSystem::stopTutorial() {
    if (!m_activeTutorial) return;
    
    m_activeTutorial = nullptr;
    m_stepElapsedTime = 0.0f;
    m_stepDelayTime = 0.0f;
}

void TutorialSystem::skipTutorial() {
    if (!m_activeTutorial) return;
    
    if (!getCurrentStep() || !getCurrentStep()->canSkip) {
        return;
    }
    
    m_activeTutorial->skipped = true;
    m_stats.skippedTutorials++;
    
    stopTutorial();
}

bool TutorialSystem::nextStep() {
    if (!m_activeTutorial) return false;
    
    int nextIndex = m_activeTutorial->currentStepIndex + 1;
    if (nextIndex >= static_cast<int>(m_activeTutorial->steps.size())) {
        finishTutorial();
        return false;
    }
    
    m_activeTutorial->currentStepIndex = nextIndex;
    m_stepElapsedTime = 0.0f;
    m_stepDelayTime = 0.0f;
    
    notifyStepChange();
    
    return true;
}

bool TutorialSystem::previousStep() {
    if (!m_activeTutorial) return false;
    
    int prevIndex = m_activeTutorial->currentStepIndex - 1;
    if (prevIndex < 0) return false;
    
    m_activeTutorial->currentStepIndex = prevIndex;
    m_stepElapsedTime = 0.0f;
    m_stepDelayTime = 0.0f;
    
    notifyStepChange();
    
    return true;
}

bool TutorialSystem::goToStep(int stepIndex) {
    if (!m_activeTutorial) return false;
    
    if (stepIndex < 0 || stepIndex >= static_cast<int>(m_activeTutorial->steps.size())) {
        return false;
    }
    
    m_activeTutorial->currentStepIndex = stepIndex;
    m_stepElapsedTime = 0.0f;
    m_stepDelayTime = 0.0f;
    
    notifyStepChange();
    
    return true;
}

Tutorial* TutorialSystem::getCurrentTutorial() {
    return m_activeTutorial;
}

TutorialStep* TutorialSystem::getCurrentStep() {
    if (!m_activeTutorial) return nullptr;
    
    if (m_activeTutorial->currentStepIndex < 0 ||
        m_activeTutorial->currentStepIndex >= static_cast<int>(m_activeTutorial->steps.size())) {
        return nullptr;
    }
    
    return &m_activeTutorial->steps[m_activeTutorial->currentStepIndex];
}

void TutorialSystem::completeAction(const std::string& actionName) {
    auto* step = getCurrentStep();
    if (!step) return;
    
    if (step->type == TutorialStepType::WAIT_FOR_ACTION) {
        if (step->requiredAction == actionName || step->requiredEventName == actionName) {
            advanceToNextStep();
        }
    }
}

void TutorialSystem::notifyInput(const std::string& inputName) {
    auto* step = getCurrentStep();
    if (!step) return;
    
    if (step->type == TutorialStepType::WAIT_FOR_INPUT) {
        if (step->requiredInput == inputName) {
            advanceToNextStep();
        }
    }
}

void TutorialSystem::registerHint(const Hint& hint) {
    m_hints[hint.id] = hint;
}

void TutorialSystem::showHint(const std::string& id) {
    if (!m_hintsEnabled) return;
    
    auto it = m_hints.find(id);
    if (it == m_hints.end()) return;
    
    Hint& hint = it->second;
    
    // Check if should only show once
    if (hint.showOnce && m_shownHints[id]) {
        return;
    }
    
    m_activeHint = &hint;
    m_hintElapsedTime = 0.0f;
    m_shownHints[id] = true;
    m_stats.hintsShown++;
    
    if (m_hintCallback) {
        m_hintCallback(m_activeHint);
    }
}

void TutorialSystem::hideHint() {
    m_activeHint = nullptr;
    m_hintElapsedTime = 0.0f;
}

void TutorialSystem::triggerHints(const std::string& eventName) {
    if (!m_hintsEnabled) return;
    
    // Find highest priority hint for this event
    Hint* bestHint = nullptr;
    int highestPriority = -1;
    
    for (auto& pair : m_hints) {
        Hint& hint = pair.second;
        if (hint.triggerEvent == eventName) {
            if (hint.showOnce && m_shownHints[hint.id]) {
                continue;
            }
            if (hint.priority > highestPriority) {
                highestPriority = hint.priority;
                bestHint = &hint;
            }
        }
    }
    
    if (bestHint) {
        showHint(bestHint->id);
    }
}

Hint* TutorialSystem::getActiveHint() {
    return m_activeHint;
}

void TutorialSystem::markCompleted(const std::string& id) {
    m_completedTutorials[id] = true;
    
    auto* tutorial = getTutorial(id);
    if (tutorial) {
        tutorial->completed = true;
        m_stats.completedTutorials++;
    }
}

bool TutorialSystem::isCompleted(const std::string& id) const {
    auto it = m_completedTutorials.find(id);
    return (it != m_completedTutorials.end()) && it->second;
}

void TutorialSystem::resetTutorial(const std::string& id) {
    m_completedTutorials[id] = false;
    
    auto* tutorial = getTutorial(id);
    if (tutorial) {
        tutorial->completed = false;
        tutorial->skipped = false;
        tutorial->currentStepIndex = 0;
    }
}

void TutorialSystem::resetAllProgress() {
    m_completedTutorials.clear();
    m_shownHints.clear();
    
    for (auto& pair : m_tutorials) {
        pair.second.completed = false;
        pair.second.skipped = false;
        pair.second.currentStepIndex = 0;
    }
    
    m_stats.completedTutorials = 0;
    m_stats.skippedTutorials = 0;
    m_stats.hintsShown = 0;
}

float TutorialSystem::getCompletionPercentage() const {
    if (m_stats.totalTutorials == 0) return 0.0f;
    return (static_cast<float>(m_stats.completedTutorials) / m_stats.totalTutorials) * 100.0f;
}

void TutorialSystem::setTutorialStartCallback(std::function<void(Tutorial*)> callback) {
    m_tutorialStartCallback = callback;
}

void TutorialSystem::setTutorialCompleteCallback(std::function<void(Tutorial*)> callback) {
    m_tutorialCompleteCallback = callback;
}

void TutorialSystem::setStepCallback(std::function<void(const TutorialEvent&)> callback) {
    m_stepCallback = callback;
}

void TutorialSystem::setHintCallback(std::function<void(Hint*)> callback) {
    m_hintCallback = callback;
}

bool TutorialSystem::saveProgress(const std::string& filepath) {
    // TODO: Implement JSON/binary serialization
    return false;
}

bool TutorialSystem::loadProgress(const std::string& filepath) {
    // TODO: Implement JSON/binary deserialization
    return false;
}

TutorialSystem::Statistics TutorialSystem::getStatistics() const {
    return m_stats;
}

void TutorialSystem::updateTutorial(float deltaTime) {
    if (!m_activeTutorial) return;
    
    auto* step = getCurrentStep();
    if (!step) return;
    
    // Handle delay
    if (m_stepDelayTime < step->delay) {
        m_stepDelayTime += deltaTime;
        return;
    }
    
    m_stepElapsedTime += deltaTime;
    
    // Check for auto-advance
    if (step->duration > 0.0f && m_stepElapsedTime >= step->duration) {
        advanceToNextStep();
        return;
    }
    
    // Check step-specific conditions
    checkStepCompletion();
}

void TutorialSystem::updateHints(float deltaTime) {
    if (!m_activeHint) return;
    
    m_hintElapsedTime += deltaTime;
    
    if (m_hintElapsedTime >= m_activeHint->displayDuration) {
        hideHint();
    }
}

void TutorialSystem::advanceToNextStep() {
    if (!nextStep()) {
        // Reached end of tutorial
        finishTutorial();
    }
}

void TutorialSystem::finishTutorial() {
    if (!m_activeTutorial) return;
    
    m_activeTutorial->completed = true;
    markCompleted(m_activeTutorial->id);
    
    if (m_tutorialCompleteCallback) {
        m_tutorialCompleteCallback(m_activeTutorial);
    }
    
    stopTutorial();
}

void TutorialSystem::notifyStepChange() {
    if (!m_stepCallback || !m_activeTutorial) return;
    
    TutorialEvent event;
    event.tutorial = m_activeTutorial;
    event.step = getCurrentStep();
    event.stepIndex = m_activeTutorial->currentStepIndex;
    event.completed = false;
    
    m_stepCallback(event);
}

void TutorialSystem::checkStepCompletion() {
    auto* step = getCurrentStep();
    if (!step) return;
    
    // This would check various conditions based on step type
    // For now, steps advance manually via completeAction() or notifyInput()
}

} // namespace UI
} // namespace JJM
