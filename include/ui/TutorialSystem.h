#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace JJM {
namespace UI {

/**
 * Tutorial step types
 */
enum class TutorialStepType {
    MESSAGE,           // Simple text message
    HIGHLIGHT,         // Highlight UI element or game object
    WAIT_FOR_ACTION,   // Wait for player to perform an action
    WAIT_FOR_INPUT,    // Wait for specific input
    CINEMATIC,         // Play a cinematic sequence
    CHECKPOINT         // Mark a checkpoint in tutorial
};

/**
 * Tutorial step data
 */
struct TutorialStep {
    std::string id;
    TutorialStepType type;
    
    std::string title;
    std::string message;
    std::string iconPath;
    
    // For HIGHLIGHT type
    std::string targetObjectId;        // Object to highlight
    float highlightPosition[3] = {0,0,0};
    bool showArrow = true;
    
    // For WAIT_FOR_ACTION type
    std::string requiredAction;        // Action player must perform
    std::string requiredEventName;     // Event to listen for
    
    // For WAIT_FOR_INPUT type
    std::string requiredInput;         // Input key/button required
    
    // Timing
    float duration = 0.0f;             // Auto-advance after duration (0 = wait for user)
    float delay = 0.0f;                // Delay before showing step
    
    // Display options
    bool canSkip = true;
    bool pauseGame = false;
    bool dimScreen = false;
    float screenDimAmount = 0.5f;
};

/**
 * Complete tutorial sequence
 */
struct Tutorial {
    std::string id;
    std::string name;
    std::string description;
    
    std::vector<TutorialStep> steps;
    
    bool mandatory = false;            // Must complete to continue
    bool showOnce = true;              // Only show once
    std::string requiredLevel;         // Level where tutorial activates
    
    // Completion tracking
    bool completed = false;
    bool skipped = false;
    int currentStepIndex = 0;
};

/**
 * Tutorial progress event
 */
struct TutorialEvent {
    Tutorial* tutorial;
    TutorialStep* step;
    int stepIndex;
    bool completed;
};

/**
 * Context-sensitive hint
 */
struct Hint {
    std::string id;
    std::string message;
    std::string iconPath;
    
    float displayDuration = 5.0f;
    int priority = 0;                  // Higher priority hints show first
    
    // Trigger conditions
    std::string triggerEvent;          // Event that triggers hint
    bool showOnce = true;
    bool hasBeenShown = false;
};

/**
 * System for managing interactive tutorials and context hints
 */
class TutorialSystem {
public:
    TutorialSystem();
    ~TutorialSystem();
    
    /**
     * Initialize the tutorial system
     */
    void initialize();
    
    /**
     * Shutdown the system
     */
    void shutdown();
    
    /**
     * Update tutorial system (check triggers, advance steps, etc.)
     * @param deltaTime Time since last update
     */
    void update(float deltaTime);
    
    // Tutorial management
    
    /**
     * Register a tutorial
     * @param tutorial Tutorial to register
     * @return True if registered successfully
     */
    bool registerTutorial(const Tutorial& tutorial);
    
    /**
     * Load tutorials from file
     * @param filepath Path to tutorial definitions file
     * @return Number of tutorials loaded
     */
    int loadTutorials(const std::string& filepath);
    
    /**
     * Get tutorial by ID
     * @param id Tutorial identifier
     * @return Pointer to tutorial, or nullptr if not found
     */
    Tutorial* getTutorial(const std::string& id);
    
    /**
     * Start a tutorial
     * @param id Tutorial identifier
     * @return True if started successfully
     */
    bool startTutorial(const std::string& id);
    
    /**
     * Stop current tutorial
     */
    void stopTutorial();
    
    /**
     * Skip current tutorial
     */
    void skipTutorial();
    
    /**
     * Advance to next tutorial step
     * @return True if advanced successfully
     */
    bool nextStep();
    
    /**
     * Go back to previous tutorial step
     * @return True if went back successfully
     */
    bool previousStep();
    
    /**
     * Jump to specific tutorial step
     * @param stepIndex Step index to jump to
     * @return True if jumped successfully
     */
    bool goToStep(int stepIndex);
    
    /**
     * Get current tutorial
     * @return Pointer to current tutorial, or nullptr if none active
     */
    Tutorial* getCurrentTutorial();
    
    /**
     * Get current tutorial step
     * @return Pointer to current step, or nullptr if no tutorial active
     */
    TutorialStep* getCurrentStep();
    
    /**
     * Check if a tutorial is active
     * @return True if tutorial is running
     */
    bool isTutorialActive() const { return m_activeTutorial != nullptr; }
    
    /**
     * Mark a tutorial step as completed
     * @param actionName Action that was completed
     */
    void completeAction(const std::string& actionName);
    
    /**
     * Notify input received (for WAIT_FOR_INPUT steps)
     * @param inputName Input that was received
     */
    void notifyInput(const std::string& inputName);
    
    // Hints management
    
    /**
     * Register a hint
     * @param hint Hint to register
     */
    void registerHint(const Hint& hint);
    
    /**
     * Show a hint
     * @param id Hint identifier
     */
    void showHint(const std::string& id);
    
    /**
     * Hide current hint
     */
    void hideHint();
    
    /**
     * Trigger hints based on event
     * @param eventName Event name
     */
    void triggerHints(const std::string& eventName);
    
    /**
     * Get active hint
     * @return Pointer to active hint, or nullptr if none
     */
    Hint* getActiveHint();
    
    // Progress tracking
    
    /**
     * Mark tutorial as completed
     * @param id Tutorial identifier
     */
    void markCompleted(const std::string& id);
    
    /**
     * Check if tutorial has been completed
     * @param id Tutorial identifier
     * @return True if completed
     */
    bool isCompleted(const std::string& id) const;
    
    /**
     * Reset tutorial progress
     * @param id Tutorial identifier
     */
    void resetTutorial(const std::string& id);
    
    /**
     * Reset all tutorial progress
     */
    void resetAllProgress();
    
    /**
     * Get completion percentage
     * @return Percentage of tutorials completed (0-100)
     */
    float getCompletionPercentage() const;
    
    // Callbacks
    
    /**
     * Set callback for tutorial start
     * @param callback Function to call when tutorial starts
     */
    void setTutorialStartCallback(std::function<void(Tutorial*)> callback);
    
    /**
     * Set callback for tutorial completion
     * @param callback Function to call when tutorial completes
     */
    void setTutorialCompleteCallback(std::function<void(Tutorial*)> callback);
    
    /**
     * Set callback for step changes
     * @param callback Function to call when step changes
     */
    void setStepCallback(std::function<void(const TutorialEvent&)> callback);
    
    /**
     * Set callback for hint display
     * @param callback Function to call when hint is shown
     */
    void setHintCallback(std::function<void(Hint*)> callback);
    
    // Configuration
    
    /**
     * Enable or disable tutorials
     * @param enabled True to enable
     */
    void setTutorialsEnabled(bool enabled) { m_tutorialsEnabled = enabled; }
    
    /**
     * Check if tutorials are enabled
     * @return True if enabled
     */
    bool areTutorialsEnabled() const { return m_tutorialsEnabled; }
    
    /**
     * Enable or disable hints
     * @param enabled True to enable
     */
    void setHintsEnabled(bool enabled) { m_hintsEnabled = enabled; }
    
    /**
     * Check if hints are enabled
     * @return True if enabled
     */
    bool areHintsEnabled() const { return m_hintsEnabled; }
    
    /**
     * Set tutorial speed multiplier (for auto-advance timings)
     * @param speed Speed multiplier (1.0 = normal)
     */
    void setTutorialSpeed(float speed) { m_tutorialSpeed = speed; }
    
    // Save/Load progress
    
    /**
     * Save tutorial progress to file
     * @param filepath Path to save file
     * @return True if saved successfully
     */
    bool saveProgress(const std::string& filepath);
    
    /**
     * Load tutorial progress from file
     * @param filepath Path to load file
     * @return True if loaded successfully
     */
    bool loadProgress(const std::string& filepath);
    
    // Statistics
    
    /**
     * Statistics about tutorial system
     */
    struct Statistics {
        int totalTutorials = 0;
        int completedTutorials = 0;
        int skippedTutorials = 0;
        int hintsShown = 0;
        float averageCompletionTime = 0.0f;
    };
    
    /**
     * Get tutorial statistics
     * @return Current statistics
     */
    Statistics getStatistics() const;

private:
    // Tutorial definitions
    std::unordered_map<std::string, Tutorial> m_tutorials;
    
    // Hint definitions
    std::unordered_map<std::string, Hint> m_hints;
    
    // Active tutorial state
    Tutorial* m_activeTutorial;
    float m_stepElapsedTime;
    float m_stepDelayTime;
    
    // Active hint
    Hint* m_activeHint;
    float m_hintElapsedTime;
    
    // Completion tracking
    std::unordered_map<std::string, bool> m_completedTutorials;
    std::unordered_map<std::string, bool> m_shownHints;
    
    // Callbacks
    std::function<void(Tutorial*)> m_tutorialStartCallback;
    std::function<void(Tutorial*)> m_tutorialCompleteCallback;
    std::function<void(const TutorialEvent&)> m_stepCallback;
    std::function<void(Hint*)> m_hintCallback;
    
    // Settings
    bool m_tutorialsEnabled;
    bool m_hintsEnabled;
    float m_tutorialSpeed;
    
    // Statistics
    mutable Statistics m_stats;
    
    // Internal methods
    void updateTutorial(float deltaTime);
    void updateHints(float deltaTime);
    void advanceToNextStep();
    void finishTutorial();
    void notifyStepChange();
    void checkStepCompletion();
};

} // namespace UI
} // namespace JJM
