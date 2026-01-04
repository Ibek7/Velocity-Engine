#ifndef JJM_AI_STATEMACHINE_H
#define JJM_AI_STATEMACHINE_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>
#include <any>
#include <chrono>
#include <deque>
#include <sstream>
#include <fstream>
#include <typeindex>

namespace JJM {
namespace AI {

// Forward declarations
class State;
class StateMachine;
class StateMachineSerializer;

// =============================================================================
// State Machine Serialization System
// =============================================================================

// Blackboard value types
using BlackboardValue = std::variant<
    bool, 
    int, 
    float, 
    double, 
    std::string, 
    std::vector<int>,
    std::vector<float>,
    std::vector<std::string>
>;

// State machine snapshot for save/load
struct StateMachineSnapshot {
    std::string currentStateName;
    std::string previousStateName;
    std::unordered_map<std::string, BlackboardValue> blackboard;
    float timeInCurrentState;
    uint64_t transitionCount;
    std::chrono::system_clock::time_point snapshotTime;
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
    
    // JSON support
    std::string toJSON() const;
    bool fromJSON(const std::string& json);
};

// State history entry for debugging/replay
struct StateHistoryEntry {
    std::string fromState;
    std::string toState;
    std::string transitionName;
    float timestamp;
    std::unordered_map<std::string, BlackboardValue> blackboardSnapshot;
    
    StateHistoryEntry()
        : timestamp(0)
    {}
};

// =============================================================================
// State Machine Debugger
// =============================================================================

class StateMachineDebugger {
public:
    struct DebugConfig {
        bool enabled;
        bool logTransitions;
        bool logStateUpdates;
        bool trackHistory;
        size_t maxHistorySize;
        bool breakOnTransition;
        std::vector<std::string> watchedStates;
        std::vector<std::string> watchedBlackboardKeys;
        
        DebugConfig()
            : enabled(true)
            , logTransitions(true)
            , logStateUpdates(false)
            , trackHistory(true)
            , maxHistorySize(100)
            , breakOnTransition(false)
        {}
    };
    
private:
    DebugConfig config;
    std::deque<StateHistoryEntry> history;
    std::function<void(const std::string&)> logCallback;
    std::function<void(const StateHistoryEntry&)> breakpointCallback;
    
    // Performance tracking
    std::unordered_map<std::string, float> stateTimings;
    std::unordered_map<std::string, size_t> transitionCounts;
    
public:
    StateMachineDebugger() = default;
    
    void setConfig(const DebugConfig& cfg) { config = cfg; }
    const DebugConfig& getConfig() const { return config; }
    
    void setLogCallback(std::function<void(const std::string&)> cb) { logCallback = cb; }
    void setBreakpointCallback(std::function<void(const StateHistoryEntry&)> cb) { breakpointCallback = cb; }
    
    void onTransition(const StateHistoryEntry& entry) {
        if (!config.enabled) return;
        
        // Track history
        if (config.trackHistory) {
            history.push_back(entry);
            while (history.size() > config.maxHistorySize) {
                history.pop_front();
            }
        }
        
        // Track transition counts
        std::string transitionKey = entry.fromState + "->" + entry.toState;
        transitionCounts[transitionKey]++;
        
        // Log
        if (config.logTransitions && logCallback) {
            std::stringstream ss;
            ss << "[FSM] Transition: " << entry.fromState << " -> " << entry.toState;
            if (!entry.transitionName.empty()) {
                ss << " (" << entry.transitionName << ")";
            }
            logCallback(ss.str());
        }
        
        // Breakpoint
        if (config.breakOnTransition && breakpointCallback) {
            breakpointCallback(entry);
        }
    }
    
    void onStateUpdate(const std::string& stateName, float deltaTime) {
        if (!config.enabled) return;
        
        stateTimings[stateName] += deltaTime;
        
        if (config.logStateUpdates && logCallback) {
            logCallback("[FSM] Update: " + stateName + " (dt=" + std::to_string(deltaTime) + ")");
        }
    }
    
    // History access
    const std::deque<StateHistoryEntry>& getHistory() const { return history; }
    void clearHistory() { history.clear(); }
    
    // Statistics
    float getTimeInState(const std::string& stateName) const {
        auto it = stateTimings.find(stateName);
        return it != stateTimings.end() ? it->second : 0;
    }
    
    size_t getTransitionCount(const std::string& from, const std::string& to) const {
        std::string key = from + "->" + to;
        auto it = transitionCounts.find(key);
        return it != transitionCounts.end() ? it->second : 0;
    }
    
    void resetStatistics() {
        stateTimings.clear();
        transitionCounts.clear();
    }
};

// =============================================================================
// Enhanced Blackboard with Type Safety
// =============================================================================

class EnhancedBlackboard {
private:
    std::unordered_map<std::string, BlackboardValue> data;
    std::unordered_map<std::string, std::function<void(const std::string&)>> watchers;
    
public:
    template<typename T>
    void set(const std::string& key, const T& value) {
        data[key] = value;
        notifyWatchers(key);
    }
    
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const {
        auto it = data.find(key);
        if (it == data.end()) return defaultValue;
        
        if (auto* val = std::get_if<T>(&it->second)) {
            return *val;
        }
        return defaultValue;
    }
    
    bool has(const std::string& key) const {
        return data.find(key) != data.end();
    }
    
    void remove(const std::string& key) {
        data.erase(key);
    }
    
    void clear() {
        data.clear();
    }
    
    // Watch for changes
    void watch(const std::string& key, std::function<void(const std::string&)> callback) {
        watchers[key] = callback;
    }
    
    void unwatch(const std::string& key) {
        watchers.erase(key);
    }
    
    // Serialization
    std::unordered_map<std::string, BlackboardValue>& getData() { return data; }
    const std::unordered_map<std::string, BlackboardValue>& getData() const { return data; }
    
    void setData(const std::unordered_map<std::string, BlackboardValue>& newData) {
        data = newData;
    }
    
private:
    void notifyWatchers(const std::string& key) {
        auto it = watchers.find(key);
        if (it != watchers.end() && it->second) {
            it->second(key);
        }
    }
};

// =============================================================================
// State with Serialization Support
// =============================================================================

class SerializableState {
public:
    virtual ~SerializableState() = default;
    
    // Override these for custom serialization
    virtual std::vector<uint8_t> serializeStateData() const { return {}; }
    virtual void deserializeStateData(const std::vector<uint8_t>& data) {}
    
    // JSON serialization
    virtual std::string serializeToJSON() const { return "{}"; }
    virtual void deserializeFromJSON(const std::string& json) {}
};

// =============================================================================
// State Machine Serializer
// =============================================================================

class StateMachineSerializer {
public:
    // Binary serialization
    static std::vector<uint8_t> serialize(const StateMachine& machine);
    static bool deserialize(StateMachine& machine, const std::vector<uint8_t>& data);
    
    // File I/O
    static bool saveToFile(const StateMachine& machine, const std::string& filepath);
    static bool loadFromFile(StateMachine& machine, const std::string& filepath);
    
    // JSON serialization
    static std::string toJSON(const StateMachine& machine);
    static bool fromJSON(StateMachine& machine, const std::string& json);
    
    // Snapshot management
    static StateMachineSnapshot createSnapshot(const StateMachine& machine);
    static bool restoreSnapshot(StateMachine& machine, const StateMachineSnapshot& snapshot);
    
private:
    // Helper functions
    static void writeString(std::vector<uint8_t>& buffer, const std::string& str);
    static std::string readString(const std::vector<uint8_t>& buffer, size_t& offset);
    static void writeValue(std::vector<uint8_t>& buffer, const BlackboardValue& value);
    static BlackboardValue readValue(const std::vector<uint8_t>& buffer, size_t& offset);
};

// =============================================================================
// Visual Editor Support - State Machine Graph Data
// =============================================================================

struct StateNodeVisual {
    std::string stateName;
    float posX, posY;           // Position in editor
    float width, height;        // Node size
    uint32_t color;             // Display color
    std::string comment;        // Editor comment
    bool collapsed;             // Collapsed in editor
    
    StateNodeVisual()
        : posX(0), posY(0)
        , width(150), height(80)
        , color(0xFFFFFF)
        , collapsed(false)
    {}
};

struct TransitionVisual {
    std::string fromState;
    std::string toState;
    std::string conditionText;  // Human-readable condition
    std::vector<std::pair<float, float>> controlPoints; // Bezier curve points
    uint32_t color;
    
    TransitionVisual() : color(0xAAAAAA) {}
};

class StateMachineVisualData {
private:
    std::unordered_map<std::string, StateNodeVisual> nodes;
    std::vector<TransitionVisual> transitions;
    float viewOffsetX, viewOffsetY;
    float zoom;
    std::string name;
    std::string description;
    
public:
    StateMachineVisualData()
        : viewOffsetX(0), viewOffsetY(0)
        , zoom(1.0f)
    {}
    
    void addNode(const StateNodeVisual& node) {
        nodes[node.stateName] = node;
    }
    
    void removeNode(const std::string& stateName) {
        nodes.erase(stateName);
    }
    
    StateNodeVisual* getNode(const std::string& stateName) {
        auto it = nodes.find(stateName);
        return it != nodes.end() ? &it->second : nullptr;
    }
    
    void addTransition(const TransitionVisual& transition) {
        transitions.push_back(transition);
    }
    
    std::unordered_map<std::string, StateNodeVisual>& getNodes() { return nodes; }
    std::vector<TransitionVisual>& getTransitions() { return transitions; }
    
    void setName(const std::string& n) { name = n; }
    void setDescription(const std::string& desc) { description = desc; }
    
    // Serialization
    std::string toJSON() const;
    bool fromJSON(const std::string& json);
    
    bool saveToFile(const std::string& filepath) const;
    bool loadFromFile(const std::string& filepath);
};

// =============================================================================
// State Machine Replay System
// =============================================================================

class StateMachineReplay {
public:
    struct ReplayConfig {
        float playbackSpeed;
        bool pauseOnTransition;
        bool loop;
        
        ReplayConfig()
            : playbackSpeed(1.0f)
            , pauseOnTransition(false)
            , loop(false)
        {}
    };
    
private:
    std::vector<StateHistoryEntry> recording;
    size_t currentIndex;
    float currentTime;
    bool playing;
    bool paused;
    ReplayConfig config;
    
    std::function<void(const StateHistoryEntry&)> onReplayEntry;
    
public:
    StateMachineReplay()
        : currentIndex(0)
        , currentTime(0)
        , playing(false)
        , paused(false)
    {}
    
    // Recording
    void startRecording(StateMachineDebugger& debugger);
    void stopRecording();
    void setRecording(const std::vector<StateHistoryEntry>& entries) { recording = entries; }
    const std::vector<StateHistoryEntry>& getRecording() const { return recording; }
    
    // Playback
    void play() { playing = true; paused = false; }
    void pause() { paused = true; }
    void stop() { playing = false; currentIndex = 0; currentTime = 0; }
    void stepForward();
    void stepBackward();
    void seekTo(size_t index);
    void seekToTime(float time);
    
    void update(float deltaTime);
    
    // State
    bool isPlaying() const { return playing; }
    bool isPaused() const { return paused; }
    size_t getCurrentIndex() const { return currentIndex; }
    float getCurrentTime() const { return currentTime; }
    float getTotalDuration() const;
    
    // Callbacks
    void setOnReplayEntry(std::function<void(const StateHistoryEntry&)> cb) { onReplayEntry = cb; }
    
    void setConfig(const ReplayConfig& cfg) { config = cfg; }
};

// =============================================================================
// Original State Machine Classes
// =============================================================================

/**
 * @brief Transition between states
 */
class StateTransition {
public:
    StateTransition(const std::string& targetState);
    ~StateTransition();

    void setCondition(std::function<bool()> condition);
    bool checkCondition() const;
    std::string getTargetState() const;
    
    // For serialization/debugging
    void setName(const std::string& n) { name = n; }
    const std::string& getName() const { return name; }

private:
    std::string targetState;
    std::string name;
    std::function<bool()> condition;
};

/**
 * @brief Base class for state machine states
 */
class State : public SerializableState {
public:
    State(const std::string& name);
    virtual ~State();

    virtual void onEnter();
    virtual void onExit();
    virtual void update(float deltaTime);

    void addTransition(std::unique_ptr<StateTransition> transition);
    StateTransition* checkTransitions();

    std::string getName() const;
    void setStateMachine(StateMachine* machine);
    
    // Time tracking
    float getTimeInState() const { return timeInState; }
    void resetTimeInState() { timeInState = 0; }

protected:
    std::string name;
    StateMachine* stateMachine;
    std::vector<std::unique_ptr<StateTransition>> transitions;
    float timeInState;
};

/**
 * @brief State machine for managing state transitions
 */
class StateMachine {
public:
    StateMachine();
    ~StateMachine();

    void addState(std::unique_ptr<State> state);
    void setInitialState(const std::string& stateName);
    void changeState(const std::string& stateName);
    void update(float deltaTime);

    State* getCurrentState() const;
    State* getState(const std::string& name);
    State* getPreviousState() const { return previousState; }
    
    // State queries
    std::vector<std::string> getStateNames() const;
    bool hasState(const std::string& name) const;

    // Enhanced blackboard
    EnhancedBlackboard& getBlackboard() { return blackboard; }
    const EnhancedBlackboard& getBlackboard() const { return blackboard; }
    
    // Legacy blackboard interface
    void setBlackboardValue(const std::string& key, int value);
    int getBlackboardValue(const std::string& key, int defaultValue = 0) const;
    
    // Debugging
    void setDebugger(StateMachineDebugger* dbg) { debugger = dbg; }
    StateMachineDebugger* getDebugger() const { return debugger; }
    
    // Visual data
    void setVisualData(std::shared_ptr<StateMachineVisualData> data) { visualData = data; }
    std::shared_ptr<StateMachineVisualData> getVisualData() const { return visualData; }
    
    // Statistics
    uint64_t getTransitionCount() const { return transitionCount; }
    float getTotalRuntime() const { return totalRuntime; }

private:
    std::unordered_map<std::string, std::unique_ptr<State>> states;
    State* currentState;
    State* previousState;
    EnhancedBlackboard blackboard;
    
    // Debugging
    StateMachineDebugger* debugger;
    std::shared_ptr<StateMachineVisualData> visualData;
    
    // Statistics
    uint64_t transitionCount;
    float totalRuntime;
    
    friend class StateMachineSerializer;
};

/**
 * @brief Hierarchical state machine with sub-states
 */
class HierarchicalStateMachine : public State {
public:
    HierarchicalStateMachine(const std::string& name);
    ~HierarchicalStateMachine() override;

    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;

    void addSubState(std::unique_ptr<State> state);
    void setInitialSubState(const std::string& stateName);

private:
    StateMachine subMachine;
};

/**
 * @brief Parallel state machine running multiple states simultaneously
 */
class ParallelStateMachine {
public:
    ParallelStateMachine();
    ~ParallelStateMachine();

    void addStateMachine(std::unique_ptr<StateMachine> machine);
    void update(float deltaTime);

    std::vector<StateMachine*> getActiveMachines();

private:
    std::vector<std::unique_ptr<StateMachine>> machines;
};

/**
 * @brief Pushdown automaton for state stacking
 */
class StateMachineStack {
public:
    StateMachineStack();
    ~StateMachineStack();

    void pushState(std::unique_ptr<State> state);
    void popState();
    void changeState(std::unique_ptr<State> state);
    void update(float deltaTime);

    State* getCurrentState() const;
    size_t getStackSize() const;

private:
    std::vector<std::unique_ptr<State>> stateStack;
};

/**
 * @brief State machine builder for fluent API
 */
class StateMachineBuilder {
public:
    StateMachineBuilder();
    ~StateMachineBuilder();

    StateMachineBuilder& addState(const std::string& name,
                                   std::unique_ptr<State> state);
    
    StateMachineBuilder& addTransition(const std::string& fromState,
                                       const std::string& toState,
                                       std::function<bool()> condition);
    
    StateMachineBuilder& setInitialState(const std::string& stateName);
    
    std::unique_ptr<StateMachine> build();

private:
    std::unique_ptr<StateMachine> machine;
    std::unordered_map<std::string, std::vector<
        std::pair<std::string, std::function<bool()>>>> pendingTransitions;
};

/**
 * @brief AI behavior state for common patterns
 */
class AIBehaviorState : public State {
public:
    AIBehaviorState(const std::string& name);
    ~AIBehaviorState() override;

    void setUpdateCallback(std::function<void(float)> callback);
    void setEnterCallback(std::function<void()> callback);
    void setExitCallback(std::function<void()> callback);

    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;

private:
    std::function<void(float)> updateCallback;
    std::function<void()> enterCallback;
    std::function<void()> exitCallback;
};

} // namespace AI
} // namespace JJM

#endif // JJM_AI_STATEMACHINE_H
