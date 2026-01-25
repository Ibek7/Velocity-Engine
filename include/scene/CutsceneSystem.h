#pragma once

#include <string>
#include <vector>
#include <functional>

namespace JJM {
namespace Scene {

/**
 * Cutscene event types
 */
enum class CutsceneEventType {
    CAMERA_MOVE,        // Move camera to position/target
    CAMERA_LOOK_AT,     // Look at target
    PLAY_ANIMATION,     // Play entity animation
    PLAY_AUDIO,         // Play sound/music
    SHOW_SUBTITLE,      // Display subtitle text
    FADE_IN,            // Fade from black
    FADE_OUT,           // Fade to black
    WAIT,               // Wait for duration
    TRIGGER_EVENT,      // Trigger game event
    TELEPORT_ENTITY,    // Move entity instantly
    SPAWN_ENTITY,       // Spawn entity
    DESTROY_ENTITY,     // Remove entity
    SET_VARIABLE,       // Set game variable
    CUSTOM              // Custom event
};

/**
 * Camera movement types
 */
enum class CameraMovementType {
    LINEAR,             // Linear interpolation
    SMOOTH,             // Smooth (ease in/out)
    BEZIER,             // Bezier curve
    SPLINE,             // Spline path
    FOLLOW              // Follow entity
};

/**
 * Camera configuration for cutscene
 */
struct CutsceneCamera {
    float position[3] = {0, 0, 0};
    float target[3] = {0, 0, 0};
    float fov = 60.0f;
    CameraMovementType movementType = CameraMovementType::SMOOTH;
    float duration = 1.0f;
};

/**
 * Subtitle display configuration
 */
struct Subtitle {
    std::string text;
    std::string speaker;
    float duration = 3.0f;
    float displayTime = 0.0f;
};

/**
 * Cutscene event
 */
struct CutsceneEvent {
    CutsceneEventType type;
    float startTime = 0.0f;         // When to trigger (relative to cutscene start)
    float duration = 0.0f;          // Event duration
    
    // Event-specific data
    std::string stringData;         // For audio paths, animation names, etc.
    int entityId = -1;              // Target entity
    float position[3] = {0, 0, 0};  // Position data
    float rotation[3] = {0, 0, 0};  // Rotation data
    float floatValue = 0.0f;        // Generic float parameter
    
    CutsceneCamera camera;          // Camera configuration (for camera events)
    Subtitle subtitle;              // Subtitle (for subtitle events)
    
    bool blocking = false;          // Wait for completion before next event
    std::function<void()> callback; // Optional callback when event triggers
};

/**
 * Cutscene playback state
 */
enum class CutsceneState {
    STOPPED,
    PLAYING,
    PAUSED,
    COMPLETED
};

/**
 * Complete cutscene definition
 */
struct Cutscene {
    std::string id;
    std::string name;
    float duration = 0.0f;
    bool skippable = true;
    bool pausable = true;
    
    std::vector<CutsceneEvent> events;
    
    // Playback control
    bool restorePlayerControl = true;   // Restore control after cutscene
    bool hideUI = true;                 // Hide game UI during cutscene
    bool fadeInStart = true;            // Fade in at start
    bool fadeOutEnd = true;             // Fade out at end
};

/**
 * Cutscene playback instance
 */
class CutsceneInstance {
public:
    CutsceneInstance(const Cutscene& cutscene);
    ~CutsceneInstance();
    
    /**
     * Update playback
     * @param deltaTime Time since last update
     */
    void update(float deltaTime);
    
    /**
     * Play cutscene
     */
    void play();
    
    /**
     * Pause cutscene
     */
    void pause();
    
    /**
     * Resume cutscene
     */
    void resume();
    
    /**
     * Stop cutscene
     */
    void stop();
    
    /**
     * Skip to end
     */
    void skip();
    
    /**
     * Seek to specific time
     * @param time Time in seconds
     */
    void seek(float time);
    
    /**
     * Get current playback time
     * @return Current time
     */
    float getCurrentTime() const { return m_currentTime; }
    
    /**
     * Get playback state
     * @return Current state
     */
    CutsceneState getState() const { return m_state; }
    
    /**
     * Check if cutscene is playing
     * @return True if playing
     */
    bool isPlaying() const { return m_state == CutsceneState::PLAYING; }
    
    /**
     * Check if cutscene is complete
     * @return True if completed
     */
    bool isComplete() const { return m_state == CutsceneState::COMPLETED; }
    
    /**
     * Get cutscene ID
     * @return Cutscene ID
     */
    const std::string& getId() const { return m_cutscene.id; }
    
    /**
     * Get progress (0-1)
     * @return Playback progress
     */
    float getProgress() const;
    
private:
    const Cutscene& m_cutscene;
    CutsceneState m_state;
    float m_currentTime;
    
    std::vector<bool> m_eventTriggered;     // Track which events have been triggered
    
    void triggerEvent(const CutsceneEvent& event);
    void processEvents(float deltaTime);
};

/**
 * System for managing and playing cutscenes
 */
class CutsceneSystem {
public:
    CutsceneSystem();
    ~CutsceneSystem();
    
    /**
     * Initialize the system
     */
    void initialize();
    
    /**
     * Shutdown the system
     */
    void shutdown();
    
    /**
     * Update all active cutscenes
     * @param deltaTime Time since last update
     */
    void update(float deltaTime);
    
    // Cutscene management
    
    /**
     * Register a cutscene
     * @param cutscene Cutscene to register
     */
    void registerCutscene(const Cutscene& cutscene);
    
    /**
     * Load cutscenes from file
     * @param filepath Path to cutscenes file
     * @return Number loaded
     */
    int loadCutscenes(const std::string& filepath);
    
    /**
     * Get cutscene by ID
     * @param id Cutscene ID
     * @return Pointer to cutscene, or nullptr
     */
    const Cutscene* getCutscene(const std::string& id) const;
    
    /**
     * Check if cutscene exists
     * @param id Cutscene ID
     * @return True if registered
     */
    bool hasCutscene(const std::string& id) const;
    
    // Playback control
    
    /**
     * Play a cutscene
     * @param id Cutscene ID
     * @return True if started successfully
     */
    bool playCutscene(const std::string& id);
    
    /**
     * Stop current cutscene
     */
    void stopCurrentCutscene();
    
    /**
     * Pause current cutscene
     */
    void pauseCurrentCutscene();
    
    /**
     * Resume current cutscene
     */
    void resumeCurrentCutscene();
    
    /**
     * Skip current cutscene
     */
    void skipCurrentCutscene();
    
    /**
     * Check if a cutscene is playing
     * @return True if any cutscene is playing
     */
    bool isPlayingCutscene() const;
    
    /**
     * Get current cutscene ID
     * @return ID of playing cutscene, or empty string
     */
    std::string getCurrentCutsceneId() const;
    
    /**
     * Get current cutscene progress
     * @return Progress (0-1)
     */
    float getCurrentProgress() const;
    
    // Event building helpers
    
    /**
     * Create camera move event
     * @param startTime When to start
     * @param position Target position
     * @param duration Move duration
     * @param movementType Movement interpolation
     * @return Camera event
     */
    static CutsceneEvent createCameraMoveEvent(
        float startTime,
        const float position[3],
        float duration = 1.0f,
        CameraMovementType movementType = CameraMovementType::SMOOTH
    );
    
    /**
     * Create subtitle event
     * @param startTime When to show
     * @param text Subtitle text
     * @param speaker Speaker name
     * @param duration Display duration
     * @return Subtitle event
     */
    static CutsceneEvent createSubtitleEvent(
        float startTime,
        const std::string& text,
        const std::string& speaker = "",
        float duration = 3.0f
    );
    
    /**
     * Create animation event
     * @param startTime When to play
     * @param entityId Entity to animate
     * @param animationName Animation to play
     * @param blocking Wait for completion
     * @return Animation event
     */
    static CutsceneEvent createAnimationEvent(
        float startTime,
        int entityId,
        const std::string& animationName,
        bool blocking = false
    );
    
    /**
     * Create wait event
     * @param startTime When to wait
     * @param duration Wait duration
     * @return Wait event
     */
    static CutsceneEvent createWaitEvent(float startTime, float duration);
    
    /**
     * Create fade event
     * @param startTime When to fade
     * @param fadeOut True for fade out, false for fade in
     * @param duration Fade duration
     * @return Fade event
     */
    static CutsceneEvent createFadeEvent(float startTime, bool fadeOut, float duration = 1.0f);
    
    // Callbacks
    
    /**
     * Set callback for cutscene started
     * @param callback Function(cutsceneId)
     */
    void setCutsceneStartedCallback(std::function<void(const std::string&)> callback);
    
    /**
     * Set callback for cutscene completed
     * @param callback Function(cutsceneId)
     */
    void setCutsceneCompletedCallback(std::function<void(const std::string&)> callback);
    
    /**
     * Set callback for cutscene skipped
     * @param callback Function(cutsceneId)
     */
    void setCutsceneSkippedCallback(std::function<void(const std::string&)> callback);
    
    /**
     * Set callback for camera events
     * @param callback Function(camera)
     */
    void setCameraEventCallback(std::function<void(const CutsceneCamera&)> callback);
    
    /**
     * Set callback for subtitle events
     * @param callback Function(subtitle)
     */
    void setSubtitleEventCallback(std::function<void(const Subtitle&)> callback);
    
    // Configuration
    
    /**
     * Enable or disable cutscene system
     * @param enabled True to enable
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    /**
     * Check if enabled
     * @return True if enabled
     */
    bool isEnabled() const { return m_enabled; }
    
    /**
     * Set whether cutscenes are skippable by default
     * @param skippable True if skippable
     */
    void setDefaultSkippable(bool skippable) { m_defaultSkippable = skippable; }
    
    /**
     * Set time scale for cutscenes
     * @param scale Time scale (1.0 = normal)
     */
    void setTimeScale(float scale) { m_timeScale = scale; }

private:
    std::unordered_map<std::string, Cutscene> m_cutscenes;
    CutsceneInstance* m_currentCutscene;
    
    bool m_enabled;
    bool m_defaultSkippable;
    float m_timeScale;
    
    // Callbacks
    std::function<void(const std::string&)> m_cutsceneStartedCallback;
    std::function<void(const std::string&)> m_cutsceneCompletedCallback;
    std::function<void(const std::string&)> m_cutsceneSkippedCallback;
    std::function<void(const CutsceneCamera&)> m_cameraEventCallback;
    std::function<void(const Subtitle&)> m_subtitleEventCallback;
};

} // namespace Scene
} // namespace JJM
