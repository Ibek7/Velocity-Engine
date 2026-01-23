#pragma once

#include <string>
#include <functional>
#include <memory>

/**
 * @file SceneTransitions.h
 * @brief Scene transition effects system
 * 
 * Provides various transition effects for smooth scene changes,
 * including fades, wipes, and custom shader-based transitions.
 */

namespace Engine {

/**
 * @enum TransitionType
 * @brief Built-in transition effect types
 */
enum class TransitionType {
    None,
    Fade,
    CrossFade,
    WipeLeft,
    WipeRight,
    WipeUp,
    WipeDown,
    CircleWipe,
    DiamondWipe,
    Dissolve,
    Pixelate,
    ZoomIn,
    ZoomOut,
    Blur,
    Custom
};

/**
 * @enum TransitionState
 * @brief Current state of a transition
 */
enum class TransitionState {
    Idle,
    FadingOut,
    Switching,
    FadingIn,
    Complete
};

/**
 * @class SceneTransition
 * @brief Base class for scene transitions
 */
class SceneTransition {
public:
    virtual ~SceneTransition() = default;
    
    /**
     * @brief Initialize transition
     */
    virtual void init() {}
    
    /**
     * @brief Update transition
     * @param deltaTime Time step in seconds
     * @return Progress (0.0 - 1.0)
     */
    virtual float update(float deltaTime) = 0;
    
    /**
     * @brief Render transition effect
     * @param fromTexture Previous scene texture
     * @param toTexture Next scene texture
     * @param progress Transition progress (0.0 - 1.0)
     */
    virtual void render(unsigned int fromTexture, unsigned int toTexture, float progress) = 0;
    
    /**
     * @brief Cleanup transition
     */
    virtual void cleanup() {}
    
    /**
     * @brief Reset transition
     */
    virtual void reset() {}
};

/**
 * @class FadeTransition
 * @brief Simple fade to black transition
 */
class FadeTransition : public SceneTransition {
public:
    FadeTransition(float duration = 1.0f);
    
    float update(float deltaTime) override;
    void render(unsigned int fromTexture, unsigned int toTexture, float progress) override;
    void reset() override;

private:
    float m_duration;
    float m_elapsed;
};

/**
 * @class CrossFadeTransition
 * @brief Cross-fade between scenes
 */
class CrossFadeTransition : public SceneTransition {
public:
    CrossFadeTransition(float duration = 1.0f);
    
    float update(float deltaTime) override;
    void render(unsigned int fromTexture, unsigned int toTexture, float progress) override;
    void reset() override;

private:
    float m_duration;
    float m_elapsed;
};

/**
 * @class WipeTransition
 * @brief Directional wipe transition
 */
class WipeTransition : public SceneTransition {
public:
    enum class Direction { Left, Right, Up, Down };
    
    WipeTransition(Direction direction, float duration = 1.0f);
    
    float update(float deltaTime) override;
    void render(unsigned int fromTexture, unsigned int toTexture, float progress) override;
    void reset() override;

private:
    Direction m_direction;
    float m_duration;
    float m_elapsed;
};

/**
 * @class DissolveTransition
 * @brief Noise-based dissolve transition
 */
class DissolveTransition : public SceneTransition {
public:
    DissolveTransition(float duration = 1.0f);
    
    void init() override;
    float update(float deltaTime) override;
    void render(unsigned int fromTexture, unsigned int toTexture, float progress) override;
    void cleanup() override;
    void reset() override;

private:
    float m_duration;
    float m_elapsed;
    unsigned int m_noiseTexture;
    unsigned int m_shader;
};

/**
 * @class SceneTransitionManager
 * @brief Manages scene transitions
 */
class SceneTransitionManager {
public:
    SceneTransitionManager();
    ~SceneTransitionManager();
    
    /**
     * @brief Start a transition between scenes
     * @param fromScene Current scene identifier
     * @param toScene Next scene identifier
     * @param type Transition type
     * @param duration Transition duration in seconds
     */
    void startTransition(const std::string& fromScene, const std::string& toScene,
                        TransitionType type, float duration = 1.0f);
    
    /**
     * @brief Start a custom transition
     * @param fromScene Current scene identifier
     * @param toScene Next scene identifier
     * @param transition Custom transition object
     */
    void startCustomTransition(const std::string& fromScene, const std::string& toScene,
                              std::unique_ptr<SceneTransition> transition);
    
    /**
     * @brief Update current transition
     * @param deltaTime Time step in seconds
     */
    void update(float deltaTime);
    
    /**
     * @brief Render transition effect
     */
    void render();
    
    /**
     * @brief Check if transition is active
     * @return True if transition in progress
     */
    bool isTransitioning() const;
    
    /**
     * @brief Get current transition state
     * @return Transition state
     */
    TransitionState getState() const { return m_state; }
    
    /**
     * @brief Get transition progress
     * @return Progress (0.0 - 1.0)
     */
    float getProgress() const { return m_progress; }
    
    /**
     * @brief Set callback for when scenes should switch
     * @param callback Function to call during scene switch
     */
    void setSceneSwitchCallback(std::function<void(const std::string&)> callback);
    
    /**
     * @brief Set callback for transition completion
     * @param callback Function to call when transition completes
     */
    void setCompletionCallback(std::function<void()> callback);
    
    /**
     * @brief Skip current transition
     */
    void skipTransition();
    
    /**
     * @brief Cancel current transition
     */
    void cancelTransition();
    
    /**
     * @brief Set default transition duration
     * @param duration Duration in seconds
     */
    void setDefaultDuration(float duration) { m_defaultDuration = duration; }
    
    /**
     * @brief Get default transition duration
     * @return Duration in seconds
     */
    float getDefaultDuration() const { return m_defaultDuration; }

private:
    std::unique_ptr<SceneTransition> m_currentTransition;
    TransitionState m_state;
    float m_progress;
    float m_defaultDuration;
    
    std::string m_fromScene;
    std::string m_toScene;
    
    std::function<void(const std::string&)> m_sceneSwitchCallback;
    std::function<void()> m_completionCallback;
    
    unsigned int m_fromTexture;
    unsigned int m_toTexture;
    
    void updateState(float deltaTime);
    void performSceneSwitch();
    std::unique_ptr<SceneTransition> createTransition(TransitionType type, float duration);
};

/**
 * @class TransitionSequence
 * @brief Chains multiple transitions together
 */
class TransitionSequence {
public:
    /**
     * @brief Add a transition to the sequence
     * @param transition Transition to add
     */
    void addTransition(std::unique_ptr<SceneTransition> transition);
    
    /**
     * @brief Update the sequence
     * @param deltaTime Time step
     * @return Overall progress
     */
    float update(float deltaTime);
    
    /**
     * @brief Check if sequence is complete
     * @return True if all transitions done
     */
    bool isComplete() const;
    
    /**
     * @brief Reset sequence
     */
    void reset();

private:
    std::vector<std::unique_ptr<SceneTransition>> m_transitions;
    size_t m_currentIndex;
};

} // namespace Engine
