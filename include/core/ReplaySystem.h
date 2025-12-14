#ifndef JJM_REPLAY_SYSTEM_H
#define JJM_REPLAY_SYSTEM_H

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <fstream>

namespace JJM {
namespace Core {

/**
 * @brief Base class for replay events
 */
class ReplayEvent {
public:
    ReplayEvent(float timestamp);
    virtual ~ReplayEvent();

    virtual void execute() = 0;
    virtual void serialize(std::ostream& out) const = 0;
    virtual void deserialize(std::istream& in) = 0;
    
    float getTimestamp() const;
    void setTimestamp(float timestamp);

protected:
    float timestamp;
};

/**
 * @brief Input replay event
 */
class InputEvent : public ReplayEvent {
public:
    enum class Type {
        KeyDown,
        KeyUp,
        MouseMove,
        MouseDown,
        MouseUp,
        MouseWheel
    };

    InputEvent(float timestamp);
    ~InputEvent() override;

    void execute() override;
    void serialize(std::ostream& out) const override;
    void deserialize(std::istream& in) override;

    void setKeyEvent(Type type, int keyCode);
    void setMouseEvent(Type type, int button, float x, float y);
    void setMouseWheelEvent(float delta);

private:
    Type type;
    int keyCode;
    int button;
    float mouseX, mouseY;
    float wheelDelta;
};

/**
 * @brief Game state event for checkpoints
 */
class StateEvent : public ReplayEvent {
public:
    StateEvent(float timestamp);
    ~StateEvent() override;

    void execute() override;
    void serialize(std::ostream& out) const override;
    void deserialize(std::istream& in) override;

    void setStateData(const std::vector<uint8_t>& data);
    const std::vector<uint8_t>& getStateData() const;

private:
    std::vector<uint8_t> stateData;
};

/**
 * @brief Command event for recorded actions
 */
class CommandEvent : public ReplayEvent {
public:
    CommandEvent(float timestamp);
    ~CommandEvent() override;

    void execute() override;
    void serialize(std::ostream& out) const override;
    void deserialize(std::istream& in) override;

    void setCommand(const std::string& command);
    void setParameters(const std::vector<std::string>& params);

private:
    std::string command;
    std::vector<std::string> parameters;
};

/**
 * @brief Replay recording manager
 */
class ReplayRecorder {
public:
    ReplayRecorder();
    ~ReplayRecorder();

    void startRecording();
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    
    bool isRecording() const;
    bool isPaused() const;

    void recordEvent(std::unique_ptr<ReplayEvent> event);
    void recordInput(InputEvent::Type type, int keyCode);
    void recordMouseMove(float x, float y);
    void recordMouseButton(InputEvent::Type type, int button, float x, float y);
    void recordCheckpoint();

    void saveToFile(const std::string& filePath);
    void clearRecording();

    float getRecordingTime() const;
    size_t getEventCount() const;

private:
    std::vector<std::unique_ptr<ReplayEvent>> events;
    bool recording;
    bool paused;
    float recordingTime;
    float pauseStartTime;
};

/**
 * @brief Replay playback manager
 */
class ReplayPlayer {
public:
    ReplayPlayer();
    ~ReplayPlayer();

    void loadFromFile(const std::string& filePath);
    
    void startPlayback();
    void stopPlayback();
    void pausePlayback();
    void resumePlayback();
    
    void update(float deltaTime);
    
    bool isPlaying() const;
    bool isPaused() const;
    bool isComplete() const;

    void setPlaybackSpeed(float speed);
    float getPlaybackSpeed() const;
    
    void seekToTime(float time);
    void seekToEvent(size_t index);
    
    float getCurrentTime() const;
    float getTotalTime() const;
    size_t getCurrentEventIndex() const;
    size_t getTotalEventCount() const;

    void setOnEventExecuted(std::function<void(const ReplayEvent*)> callback);

private:
    std::vector<std::unique_ptr<ReplayEvent>> events;
    size_t currentEventIndex;
    float currentTime;
    float playbackSpeed;
    bool playing;
    bool paused;
    std::function<void(const ReplayEvent*)> onEventExecuted;
    
    void executeCurrentEvents();
};

/**
 * @brief Replay system combining recording and playback
 */
class ReplaySystem {
public:
    ReplaySystem();
    ~ReplaySystem();

    void update(float deltaTime);

    ReplayRecorder& getRecorder();
    ReplayPlayer& getPlayer();

    void startRecording();
    void stopRecording();
    void startPlayback();
    void stopPlayback();

    bool isRecording() const;
    bool isPlaying() const;

    void saveReplay(const std::string& filePath);
    void loadReplay(const std::string& filePath);

private:
    ReplayRecorder recorder;
    ReplayPlayer player;
};

/**
 * @brief Replay metadata for saved replays
 */
class ReplayMetadata {
public:
    ReplayMetadata();
    ~ReplayMetadata();

    void setTitle(const std::string& title);
    void setDescription(const std::string& description);
    void setTimestamp(long long timestamp);
    void setDuration(float duration);
    void setEventCount(size_t count);

    std::string getTitle() const;
    std::string getDescription() const;
    long long getTimestamp() const;
    float getDuration() const;
    size_t getEventCount() const;

    void serialize(std::ostream& out) const;
    void deserialize(std::istream& in);

private:
    std::string title;
    std::string description;
    long long timestamp;
    float duration;
    size_t eventCount;
};

/**
 * @brief Replay file format handler
 */
class ReplayFileHandler {
public:
    static bool save(const std::string& filePath,
                     const ReplayMetadata& metadata,
                     const std::vector<std::unique_ptr<ReplayEvent>>& events);
    
    static bool load(const std::string& filePath,
                     ReplayMetadata& metadata,
                     std::vector<std::unique_ptr<ReplayEvent>>& events);

private:
    static void writeHeader(std::ostream& out);
    static bool readHeader(std::istream& in);
    static void writeEvent(std::ostream& out, const ReplayEvent& event);
    static std::unique_ptr<ReplayEvent> readEvent(std::istream& in);
};

/**
 * @brief Replay compression for smaller file sizes
 */
class ReplayCompressor {
public:
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& data);

private:
    static std::vector<uint8_t> rleCompress(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> rleDecompress(const std::vector<uint8_t>& data);
};

/**
 * @brief Replay analyzer for statistics
 */
class ReplayAnalyzer {
public:
    ReplayAnalyzer();
    ~ReplayAnalyzer();

    void analyze(const std::vector<std::unique_ptr<ReplayEvent>>& events);

    size_t getTotalEvents() const;
    size_t getInputEventCount() const;
    size_t getStateEventCount() const;
    float getAverageEventInterval() const;
    float getTotalDuration() const;

private:
    size_t totalEvents;
    size_t inputEvents;
    size_t stateEvents;
    float averageInterval;
    float totalDuration;
};

/**
 * @brief Replay comparison for analyzing differences
 */
class ReplayComparator {
public:
    struct Difference {
        float timestamp;
        std::string description;
    };

    static std::vector<Difference> compare(
        const std::vector<std::unique_ptr<ReplayEvent>>& replay1,
        const std::vector<std::unique_ptr<ReplayEvent>>& replay2);
};

} // namespace Core
} // namespace JJM

#endif // JJM_REPLAY_SYSTEM_H
