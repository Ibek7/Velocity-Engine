#ifndef INPUT_REPLAY_H
#define INPUT_REPLAY_H

#include <vector>
#include <string>
#include <fstream>

namespace JJM {
namespace Input {

enum class InputEventType {
    KeyDown,
    KeyUp,
    MouseMove,
    MouseButtonDown,
    MouseButtonUp,
    MouseWheel
};

struct RecordedInputEvent {
    InputEventType type;
    float timestamp;
    int keyCode;
    float mouseX, mouseY;
    int mouseButton;
    float wheelDelta;
    
    RecordedInputEvent() : type(InputEventType::KeyDown), timestamp(0), keyCode(0), 
                          mouseX(0), mouseY(0), mouseButton(0), wheelDelta(0) {}
};

class InputRecorder {
public:
    InputRecorder();
    
    void startRecording();
    void stopRecording();
    bool isRecording() const { return recording; }
    
    void recordKeyDown(int keyCode, float timestamp);
    void recordKeyUp(int keyCode, float timestamp);
    void recordMouseMove(float x, float y, float timestamp);
    void recordMouseButton(int button, bool down, float timestamp);
    void recordMouseWheel(float delta, float timestamp);
    
    bool saveToFile(const std::string& filename);
    void clear();
    
    const std::vector<RecordedInputEvent>& getEvents() const { return events; }
    
private:
    bool recording;
    std::vector<RecordedInputEvent> events;
};

class InputReplayer {
public:
    InputReplayer();
    
    bool loadFromFile(const std::string& filename);
    void startPlayback();
    void stopPlayback();
    void pausePlayback();
    void resumePlayback();
    
    void update(float deltaTime);
    
    bool isPlaying() const { return playing; }
    bool isPaused() const { return paused; }
    
    void setPlaybackSpeed(float speed) { playbackSpeed = speed; }
    float getPlaybackSpeed() const { return playbackSpeed; }
    
    float getProgress() const;
    float getDuration() const;
    
    void clear();
    
private:
    std::vector<RecordedInputEvent> events;
    size_t currentEventIndex;
    float currentTime;
    bool playing;
    bool paused;
    float playbackSpeed;
    
    void processEvent(const RecordedInputEvent& event);
};

} // namespace Input
} // namespace JJM

#endif
