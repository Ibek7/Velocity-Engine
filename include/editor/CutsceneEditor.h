#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <variant>
#include <optional>
#include <set>

// Cutscene editor and playback system
namespace Engine {

enum class CutsceneActionType {
    CameraMove,
    CameraLookAt,
    CameraShake,
    ShowDialog,
    HideDialog,
    PlayAnimation,
    StopAnimation,
    PlaySound,
    StopSound,
    PlayMusic,
    FadeMusic,
    SpawnObject,
    DestroyObject,
    MoveObject,
    RotateObject,
    ScaleObject,
    SetVisibility,
    TriggerEvent,
    SetVariable,
    BranchCondition,
    Wait,
    Parallel,
    Sequence,
    Custom
};

// Easing functions for interpolation
enum class EasingType {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    Bounce,
    Elastic,
    Back
};

// =============================================================================
// Timeline Track Types
// =============================================================================

/**
 * @brief Types of specialized timeline tracks
 */
enum class TrackType {
    Generic,            // Default track for any action
    Camera,             // Camera movement, rotation, FOV
    Animation,          // Skeletal/sprite animations
    Audio,              // Sound effects and music
    Dialogue,           // Character dialogue and subtitles
    Event,              // Game events and triggers
    Property,           // Object property animation
    Particle,           // Particle system control
    Light,              // Light intensity, color changes
    PostProcess,        // Post-processing effects
    Script,             // Custom script execution
    Group               // Container for other tracks
};

/**
 * @brief Track blending modes
 */
enum class TrackBlendMode {
    Override,           // Replace existing values
    Additive,           // Add to existing values
    Multiply,           // Multiply with existing values
    Average             // Average with other tracks
};

/**
 * @brief Keyframe interpolation types
 */
enum class KeyframeInterpolation {
    Constant,           // Step/hold value
    Linear,             // Linear interpolation
    Bezier,             // Bezier curve
    Hermite,            // Hermite spline
    CatmullRom          // Catmull-Rom spline
};

/**
 * @brief Keyframe data structure
 */
struct Keyframe {
    float time;
    std::vector<float> values;      // Can hold 1-4 float values (scalar, vec2, vec3, vec4)
    KeyframeInterpolation interpolation;
    
    // Bezier tangents (for Bezier interpolation)
    std::vector<float> inTangent;
    std::vector<float> outTangent;
    float inWeight;
    float outWeight;
    
    // Metadata
    std::string label;
    bool selected;
    
    Keyframe()
        : time(0.0f)
        , interpolation(KeyframeInterpolation::Linear)
        , inWeight(1.0f)
        , outWeight(1.0f)
        , selected(false)
    {}
    
    Keyframe(float t, const std::vector<float>& v)
        : time(t)
        , values(v)
        , interpolation(KeyframeInterpolation::Linear)
        , inWeight(1.0f)
        , outWeight(1.0f)
        , selected(false)
    {}
};

/**
 * @brief Animation curve for property animation
 */
class AnimationCurve {
private:
    std::vector<Keyframe> m_keyframes;
    bool m_preInfinity;     // Loop before first keyframe
    bool m_postInfinity;    // Loop after last keyframe
    
public:
    AnimationCurve();
    
    // Keyframe management
    int addKeyframe(const Keyframe& keyframe);
    void removeKeyframe(int index);
    void updateKeyframe(int index, const Keyframe& keyframe);
    Keyframe* getKeyframe(int index);
    const Keyframe* getKeyframe(int index) const;
    int getKeyframeCount() const { return static_cast<int>(m_keyframes.size()); }
    
    // Find keyframes
    int findKeyframeAtTime(float time, float tolerance = 0.001f) const;
    std::pair<int, int> findSurroundingKeyframes(float time) const;
    
    // Evaluation
    std::vector<float> evaluate(float time) const;
    float evaluateScalar(float time) const;
    
    // Utility
    float getStartTime() const;
    float getEndTime() const;
    float getDuration() const;
    void sortKeyframes();
    void clear();
    
    // Infinity modes
    void setPreInfinity(bool loop) { m_preInfinity = loop; }
    void setPostInfinity(bool loop) { m_postInfinity = loop; }
    
private:
    std::vector<float> interpolate(const Keyframe& k1, const Keyframe& k2, float t) const;
    std::vector<float> interpolateBezier(const Keyframe& k1, const Keyframe& k2, float t) const;
    std::vector<float> interpolateHermite(const Keyframe& k1, const Keyframe& k2, float t) const;
};

/**
 * @brief Property binding for animation
 */
struct PropertyBinding {
    std::string objectPath;         // Path to target object
    std::string propertyName;       // Name of property to animate
    std::string componentType;      // Component type (Transform, Material, etc.)
    int channelIndex;               // For multi-channel properties (x=0, y=1, z=2, w=3)
    
    PropertyBinding()
        : channelIndex(-1)      // -1 means all channels
    {}
};

/**
 * @brief Track layer for non-destructive editing
 */
struct TrackLayer {
    std::string name;
    float weight;
    bool enabled;
    TrackBlendMode blendMode;
    std::unique_ptr<AnimationCurve> curve;
    
    TrackLayer(const std::string& n = "Layer")
        : name(n)
        , weight(1.0f)
        , enabled(true)
        , blendMode(TrackBlendMode::Override)
    {}
};

// Action parameter types
using ActionParam = std::variant<float, int, bool, std::string, std::vector<float>>;

struct CutsceneAction {
    CutsceneActionType type;
    float timestamp;
    float duration;
    std::string targetId;
    float parameters[6];
    std::string stringParam;
    EasingType easing;
    std::unordered_map<std::string, ActionParam> namedParams;
    
    // For branching/conditional actions
    std::string condition;
    std::vector<std::shared_ptr<CutsceneAction>> childActions;
    
    CutsceneAction()
        : type(CutsceneActionType::Wait)
        , timestamp(0.0f)
        , duration(0.0f)
        , easing(EasingType::Linear)
    {
        for (int i = 0; i < 6; ++i) parameters[i] = 0.0f;
    }
};

/**
 * @brief Enhanced timeline track with curve support
 */
struct CutsceneTrack {
    std::string name;
    std::string targetEntity;
    bool muted;
    bool locked;
    std::vector<std::shared_ptr<CutsceneAction>> actions;
    
    // Track type and properties
    TrackType trackType;
    TrackBlendMode blendMode;
    float weight;
    
    // Color for UI
    std::string color;
    
    // Property animation
    PropertyBinding propertyBinding;
    std::unique_ptr<AnimationCurve> animationCurve;
    std::vector<std::unique_ptr<TrackLayer>> layers;
    
    // Parent/child hierarchy
    int parentTrackIndex;
    std::vector<int> childTrackIndices;
    bool collapsed;
    
    // Track-level effects
    float timeOffset;
    float timeScale;
    bool reversed;
    
    CutsceneTrack(const std::string& n = "Track")
        : name(n)
        , muted(false)
        , locked(false)
        , trackType(TrackType::Generic)
        , blendMode(TrackBlendMode::Override)
        , weight(1.0f)
        , color("#4A90D9")
        , parentTrackIndex(-1)
        , collapsed(false)
        , timeOffset(0.0f)
        , timeScale(1.0f)
        , reversed(false)
    {}
    
    // Layer management
    TrackLayer* addLayer(const std::string& layerName);
    void removeLayer(int index);
    TrackLayer* getLayer(int index);
    int getLayerCount() const { return static_cast<int>(layers.size()); }
    
    // Evaluate track at time
    std::vector<float> evaluate(float time) const;
    
    // Transform time with offset/scale/reverse
    float transformTime(float globalTime) const;
};

/**
 * @brief Track group for organization
 */
struct TrackGroup {
    std::string name;
    std::vector<int> trackIndices;
    std::string color;
    bool collapsed;
    bool solo;          // Only play tracks in this group
    bool muted;
    
    TrackGroup(const std::string& n = "Group")
        : name(n)
        , color("#666666")
        , collapsed(false)
        , solo(false)
        , muted(false)
    {}
};

/**
 * @brief Timeline ruler settings
 */
struct TimelineRulerSettings {
    bool showFrameNumbers;
    bool showTimecode;
    float frameRate;
    int majorTickInterval;
    int minorTicksPerMajor;
    
    TimelineRulerSettings()
        : showFrameNumbers(true)
        , showTimecode(true)
        , frameRate(30.0f)
        , majorTickInterval(30)
        , minorTicksPerMajor(5)
    {}
    
    std::string formatTime(float seconds) const;
    int timeToFrame(float seconds) const;
    float frameToTime(int frame) const;
};

// Cutscene marker for navigation
struct CutsceneMarker {
    std::string name;
    float timestamp;
    std::string color;
    
    CutsceneMarker(const std::string& n, float t, const std::string& c = "#FFFFFF")
        : name(n), timestamp(t), color(c) {}
};

// Cutscene variable for runtime state
struct CutsceneVariable {
    std::string name;
    ActionParam value;
    ActionParam defaultValue;
};

class Cutscene {
public:
    Cutscene(const std::string& name);
    ~Cutscene();

    // Actions (legacy single-track)
    void addAction(const CutsceneAction& action);
    void removeAction(int index);
    void clearActions();
    
    // Track management
    CutsceneTrack* addTrack(const std::string& name);
    CutsceneTrack* getTrack(const std::string& name);
    CutsceneTrack* getTrack(int index);
    void removeTrack(const std::string& name);
    void removeTrack(int index);
    int getTrackCount() const { return static_cast<int>(m_tracks.size()); }
    void reorderTrack(int fromIndex, int toIndex);
    
    // Action management on tracks
    void addActionToTrack(const std::string& trackName, std::shared_ptr<CutsceneAction> action);
    void removeActionFromTrack(const std::string& trackName, int actionIndex);
    void moveActionToTrack(const std::string& fromTrack, int actionIndex, const std::string& toTrack);
    
    // Markers
    void addMarker(const std::string& name, float timestamp, const std::string& color = "#FFFFFF");
    void removeMarker(const std::string& name);
    CutsceneMarker* getMarker(const std::string& name);
    const std::vector<CutsceneMarker>& getMarkers() const { return m_markers; }
    void jumpToMarker(const std::string& name);
    
    // Variables
    void setVariable(const std::string& name, const ActionParam& value);
    ActionParam getVariable(const std::string& name) const;
    bool hasVariable(const std::string& name) const;
    void resetVariables();
    
    // Playback
    void play();
    void pause();
    void stop();
    void update(float deltaTime);
    void seek(float time);
    void seekToMarker(const std::string& markerName);
    void setPlaybackSpeed(float speed);
    float getPlaybackSpeed() const { return m_playbackSpeed; }
    
    // Looping
    void setLooping(bool loop);
    bool isLooping() const { return m_looping; }
    void setLoopRange(float start, float end);
    
    // State
    bool isPlaying() const { return m_playing; }
    bool isPaused() const { return m_paused; }
    bool isFinished() const { return m_finished; }
    float getCurrentTime() const { return m_currentTime; }
    float getDuration() const;
    float getProgress() const;
    
    // Properties
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    int getActionCount() const { return static_cast<int>(m_actions.size()); }
    const CutsceneAction* getAction(int index) const;
    
    // Callbacks
    void setOnStart(std::function<void()> callback) { m_onStart = callback; }
    void setOnEnd(std::function<void()> callback) { m_onEnd = callback; }
    void setOnMarkerReached(std::function<void(const std::string&)> callback) { m_onMarkerReached = callback; }
    void setOnActionStart(std::function<void(const CutsceneAction&)> callback) { m_onActionStart = callback; }
    void setOnActionEnd(std::function<void(const CutsceneAction&)> callback) { m_onActionEnd = callback; }

private:
    void executeAction(const CutsceneAction& action);
    void updateAction(CutsceneAction& action, float deltaTime);
    float applyEasing(float t, EasingType easing) const;
    bool evaluateCondition(const std::string& condition) const;
    void checkMarkers(float oldTime, float newTime);

    std::string m_name;
    std::vector<CutsceneAction> m_actions;
    std::vector<std::unique_ptr<CutsceneTrack>> m_tracks;
    std::vector<CutsceneMarker> m_markers;
    std::unordered_map<std::string, CutsceneVariable> m_variables;
    
    float m_currentTime;
    float m_playbackSpeed;
    bool m_playing;
    bool m_paused;
    bool m_finished;
    bool m_looping;
    float m_loopStart;
    float m_loopEnd;
    int m_currentActionIndex;
    
    // Active actions being interpolated
    std::vector<std::pair<CutsceneAction*, float>> m_activeActions;
    
    // Callbacks
    std::function<void()> m_onStart;
    std::function<void()> m_onEnd;
    std::function<void(const std::string&)> m_onMarkerReached;
    std::function<void(const CutsceneAction&)> m_onActionStart;
    std::function<void(const CutsceneAction&)> m_onActionEnd;
};

class CutsceneEditor {
public:
    static CutsceneEditor& getInstance();

    // Cutscene management
    Cutscene* createCutscene(const std::string& name);
    void deleteCutscene(const std::string& name);
    Cutscene* getCutscene(const std::string& name);
    Cutscene* duplicateCutscene(const std::string& name, const std::string& newName);
    
    // Playback
    void playCutscene(const std::string& name);
    void stopCurrentCutscene();
    void pauseCurrentCutscene();
    void resumeCurrentCutscene();
    void update(float deltaTime);
    
    // Timeline editing
    void setTimelineZoom(float zoom);
    float getTimelineZoom() const { return m_timelineZoom; }
    void setTimelinePosition(float position);
    float getTimelinePosition() const { return m_timelinePosition; }
    void setSnapToGrid(bool snap);
    void setGridSize(float size);
    
    // Selection
    void selectAction(Cutscene* cutscene, int trackIndex, int actionIndex);
    void deselectAll();
    void copySelectedActions();
    void pasteActions(float timestamp);
    void deleteSelectedActions();
    
    // Undo/Redo
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    void clearHistory();
    
    // Preview
    void startPreview(float fromTime = 0.0f);
    void stopPreview();
    bool isPreviewing() const { return m_previewing; }
    
    // Save/Load
    bool saveCutscene(const std::string& name, const std::string& filePath);
    bool loadCutscene(const std::string& filePath);
    bool exportCutscene(const std::string& name, const std::string& format, const std::string& filePath);
    
    // Query
    Cutscene* getCurrentCutscene() const { return m_currentCutscene; }
    bool isCutscenePlaying() const;
    std::vector<std::string> getAllCutsceneNames() const;
    
    // Custom action registration
    void registerCustomAction(const std::string& name, 
                             std::function<void(const CutsceneAction&)> executor);
    void unregisterCustomAction(const std::string& name);

private:
    CutsceneEditor();
    CutsceneEditor(const CutsceneEditor&) = delete;
    CutsceneEditor& operator=(const CutsceneEditor&) = delete;
    
    void pushUndoState();

    std::unordered_map<std::string, std::unique_ptr<Cutscene>> m_cutscenes;
    Cutscene* m_currentCutscene;
    
    // Timeline state
    float m_timelineZoom;
    float m_timelinePosition;
    bool m_snapToGrid;
    float m_gridSize;
    
    // Selection state
    struct Selection {
        Cutscene* cutscene;
        int trackIndex;
        int actionIndex;
    };
    std::vector<Selection> m_selectedActions;
    std::vector<CutsceneAction> m_clipboard;
    
    // Undo/Redo stacks
    std::vector<std::string> m_undoStack;
    std::vector<std::string> m_redoStack;
    static constexpr size_t MAX_UNDO_HISTORY = 50;
    
    // Preview state
    bool m_previewing;
    float m_previewStartTime;
    
    // Custom action handlers
    std::unordered_map<std::string, std::function<void(const CutsceneAction&)>> m_customActions;
};

} // namespace Engine
