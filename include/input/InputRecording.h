#ifndef JJM_INPUT_RECORDING_H
#define JJM_INPUT_RECORDING_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <fstream>

namespace JJM {
namespace Input {

/**
 * @brief Input event types
 */
enum class InputEventType {
    KeyPress,
    KeyRelease,
    MouseMove,
    MouseButtonPress,
    MouseButtonRelease,
    MouseWheel,
    GamepadButton,
    GamepadAxis,
    TouchDown,
    TouchUp,
    TouchMove
};

/**
 * @brief Recorded input event
 */
struct InputEvent {
    InputEventType type;
    uint32_t timestamp;
    int keyCode;
    int mouseX;
    int mouseY;
    int mouseButton;
    float wheelDelta;
    int gamepadId;
    int gamepadButton;
    float gamepadAxisValue;
    int touchId;
    float touchX;
    float touchY;
};

/**
 * @brief Input recording session
 */
class InputRecording {
public:
    InputRecording();
    ~InputRecording();

    void clear();
    
    void addEvent(const InputEvent& event);
    size_t getEventCount() const;
    
    const std::vector<InputEvent>& getEvents() const;
    InputEvent getEvent(size_t index) const;
    
    bool saveToFile(const std::string& filename);
    bool loadFromFile(const std::string& filename);
    
    void setMetadata(const std::string& key, const std::string& value);
    std::string getMetadata(const std::string& key) const;
    
    uint32_t getDuration() const;

private:
    std::vector<InputEvent> events;
    std::unordered_map<std::string, std::string> metadata;
    uint32_t startTime;
};

/**
 * @brief Input recorder
 */
class InputRecorder {
public:
    static InputRecorder& getInstance();
    
    InputRecorder(const InputRecorder&) = delete;
    InputRecorder& operator=(const InputRecorder&) = delete;

    void startRecording();
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    
    bool isRecording() const;
    bool isPaused() const;
    
    void recordKeyPress(int keyCode);
    void recordKeyRelease(int keyCode);
    void recordMouseMove(int x, int y);
    void recordMouseButtonPress(int button, int x, int y);
    void recordMouseButtonRelease(int button, int x, int y);
    void recordMouseWheel(float delta);
    void recordGamepadButton(int gamepadId, int button);
    void recordGamepadAxis(int gamepadId, int axis, float value);
    void recordTouchDown(int touchId, float x, float y);
    void recordTouchUp(int touchId, float x, float y);
    void recordTouchMove(int touchId, float x, float y);
    
    std::shared_ptr<InputRecording> getCurrentRecording();
    
    bool saveRecording(const std::string& filename);
    
    void clear();

private:
    InputRecorder();
    ~InputRecorder();
    
    std::shared_ptr<InputRecording> currentRecording;
    bool recording;
    bool paused;
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    
    uint32_t getElapsedTime() const;
};

/**
 * @brief Input playback
 */
class InputPlayback {
public:
    static InputPlayback& getInstance();
    
    InputPlayback(const InputPlayback&) = delete;
    InputPlayback& operator=(const InputPlayback&) = delete;

    bool loadRecording(const std::string& filename);
    bool loadRecording(std::shared_ptr<InputRecording> recording);
    
    void startPlayback();
    void stopPlayback();
    void pausePlayback();
    void resumePlayback();
    
    bool isPlaying() const;
    bool isPaused() const;
    
    void update(float deltaTime);
    
    void setPlaybackSpeed(float speed);
    float getPlaybackSpeed() const;
    
    void setLooping(bool loop);
    bool isLooping() const;
    
    void seekToTime(uint32_t time);
    void seekToEvent(size_t eventIndex);
    
    uint32_t getCurrentTime() const;
    size_t getCurrentEventIndex() const;
    
    std::shared_ptr<InputRecording> getRecording();

private:
    InputPlayback();
    ~InputPlayback();
    
    std::shared_ptr<InputRecording> recording;
    bool playing;
    bool paused;
    bool looping;
    float playbackSpeed;
    uint32_t currentTime;
    size_t currentEventIndex;
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    
    void processEvents();
    void executeEvent(const InputEvent& event);
};

/**
 * @brief Input replay system
 */
class InputReplaySystem {
public:
    static InputReplaySystem& getInstance();
    
    InputReplaySystem(const InputReplaySystem&) = delete;
    InputReplaySystem& operator=(const InputReplaySystem&) = delete;

    void update(float deltaTime);
    
    InputRecorder& getRecorder();
    InputPlayback& getPlayback();
    
    void startRecording();
    void stopRecording();
    
    void startPlayback(const std::string& filename);
    void stopPlayback();
    
    bool isRecording() const;
    bool isPlaying() const;

private:
    InputReplaySystem();
    ~InputReplaySystem();
};

/**
 * @brief Input macro
 */
class InputMacro {
public:
    InputMacro(const std::string& name);
    ~InputMacro();

    void addEvent(const InputEvent& event);
    void clear();
    
    void execute();
    void stop();
    
    bool isExecuting() const;
    
    void setRepeatCount(int count);
    int getRepeatCount() const;
    
    void setDelay(uint32_t delayMs);
    uint32_t getDelay() const;
    
    std::string getName() const;

private:
    std::string name;
    std::vector<InputEvent> events;
    bool executing;
    int repeatCount;
    uint32_t delay;
};

/**
 * @brief Input macro manager
 */
class MacroManager {
public:
    static MacroManager& getInstance();
    
    MacroManager(const MacroManager&) = delete;
    MacroManager& operator=(const MacroManager&) = delete;

    void addMacro(const std::string& name, std::shared_ptr<InputMacro> macro);
    void removeMacro(const std::string& name);
    
    std::shared_ptr<InputMacro> getMacro(const std::string& name);
    bool hasMacro(const std::string& name) const;
    
    void executeMacro(const std::string& name);
    void stopMacro(const std::string& name);
    
    void update(float deltaTime);
    
    std::vector<std::string> getMacroNames() const;

private:
    MacroManager();
    ~MacroManager();
    
    std::unordered_map<std::string, std::shared_ptr<InputMacro>> macros;
};

/**
 * @brief Input recording format
 */
class RecordingFormat {
public:
    static bool saveJSON(const InputRecording& recording, const std::string& filename);
    static bool loadJSON(InputRecording& recording, const std::string& filename);
    
    static bool saveBinary(const InputRecording& recording, const std::string& filename);
    static bool loadBinary(InputRecording& recording, const std::string& filename);
    
    static std::string eventToString(const InputEvent& event);
    static InputEvent stringToEvent(const std::string& str);

private:
    static void writeEvent(std::ofstream& file, const InputEvent& event);
    static bool readEvent(std::ifstream& file, InputEvent& event);
};

/**
 * @brief Input statistics
 */
struct InputStats {
    size_t totalEvents;
    size_t keyEvents;
    size_t mouseEvents;
    size_t gamepadEvents;
    size_t touchEvents;
    uint32_t duration;
    float eventsPerSecond;
};

/**
 * @brief Input analyzer
 */
class InputAnalyzer {
public:
    InputAnalyzer();
    ~InputAnalyzer();

    void analyze(const InputRecording& recording);
    
    InputStats getStats() const;
    
    std::vector<InputEvent> getEventsByType(InputEventType type) const;
    
    float getAverageEventsPerSecond() const;
    
    void generateReport(std::ostream& out);

private:
    InputStats stats;
    std::vector<InputEvent> events;
};

/**
 * @brief Input comparison
 */
class InputComparison {
public:
    static float compareRecordings(const InputRecording& recording1,
                                  const InputRecording& recording2);
    
    static std::vector<size_t> findDifferences(const InputRecording& recording1,
                                              const InputRecording& recording2);
    
    static bool areIdentical(const InputRecording& recording1,
                            const InputRecording& recording2);

private:
    static float compareEvents(const InputEvent& event1, const InputEvent& event2);
};

/**
 * @brief Input recording compression
 */
class RecordingCompression {
public:
    static std::vector<InputEvent> compress(const std::vector<InputEvent>& events);
    static std::vector<InputEvent> decompress(const std::vector<InputEvent>& events);
    
    static size_t estimateCompressedSize(const std::vector<InputEvent>& events);

private:
    static void mergeConsecutiveEvents(std::vector<InputEvent>& events);
    static void removeDuplicates(std::vector<InputEvent>& events);
};

/**
 * @brief Input recording filter
 */
class RecordingFilter {
public:
    RecordingFilter();
    ~RecordingFilter();

    void setTypeFilter(InputEventType type, bool enabled);
    bool isTypeEnabled(InputEventType type) const;
    
    void setTimeRange(uint32_t startTime, uint32_t endTime);
    
    InputRecording filter(const InputRecording& recording);

private:
    std::unordered_map<InputEventType, bool> typeFilters;
    uint32_t startTime;
    uint32_t endTime;
};

} // namespace Input
} // namespace JJM

#endif // JJM_INPUT_RECORDING_H
