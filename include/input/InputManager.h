#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <SDL.h>

#include <chrono>
#include <deque>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "input/InputEvents.h"
#include "math/Vector2D.h"

namespace JJM {
namespace Input {

enum class KeyState { UP, DOWN, PRESSED, RELEASED };

// =============================================================================
// Input Recording System - Captures and replays input sequences
// =============================================================================

enum class RecordedInputType {
    KeyDown,
    KeyUp,
    MouseButtonDown,
    MouseButtonUp,
    MouseMove,
    MouseWheel,
    GamepadButton,
    GamepadAxis,
    ActionTrigger,
    TextInput
};

// Single recorded input event with precise timing
struct RecordedInputEvent {
    RecordedInputType type;
    uint64_t timestamp;    // Microseconds from recording start
    uint32_t frameNumber;  // Frame when event occurred

    // Key/button data
    int keyCode;
    int buttonIndex;
    bool pressed;

    // Mouse data
    float mouseX, mouseY;
    float wheelX, wheelY;

    // Gamepad data
    int gamepadIndex;
    float axisValue;

    // Action/text data
    std::string actionName;
    std::string textData;

    RecordedInputEvent()
        : type(RecordedInputType::KeyDown),
          timestamp(0),
          frameNumber(0),
          keyCode(0),
          buttonIndex(0),
          pressed(false),
          mouseX(0),
          mouseY(0),
          wheelX(0),
          wheelY(0),
          gamepadIndex(0),
          axisValue(0) {}
};

// Recording session metadata
struct InputRecordingMetadata {
    std::string recordingName;
    std::string gameVersion;
    std::string levelName;
    uint64_t recordingDate;
    uint64_t totalDuration;
    uint32_t totalFrames;
    uint32_t eventCount;
    std::string checksum;  // For integrity verification

    // Recording settings
    bool includeMouseMovement;
    bool includeGamepadAxes;
    float mouseSampleRate;  // Hz for mouse position sampling

    InputRecordingMetadata()
        : recordingDate(0),
          totalDuration(0),
          totalFrames(0),
          eventCount(0),
          includeMouseMovement(true),
          includeGamepadAxes(true),
          mouseSampleRate(60.0f) {}
};

// Complete input recording
class InputRecording {
   public:
    InputRecordingMetadata metadata;
    std::vector<RecordedInputEvent> events;

    // Random seed for deterministic replay
    uint32_t randomSeed;

    // Initial game state snapshot hash
    std::string initialStateHash;

    InputRecording() : randomSeed(0) {}

    // Serialization
    bool saveToFile(const std::string& filepath) const {
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) return false;

        // Write header
        const char* magic = "JJMREC";
        file.write(magic, 6);
        uint32_t version = 1;
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));

        // Write metadata
        writeString(file, metadata.recordingName);
        writeString(file, metadata.gameVersion);
        writeString(file, metadata.levelName);
        file.write(reinterpret_cast<const char*>(&metadata.recordingDate),
                   sizeof(metadata.recordingDate));
        file.write(reinterpret_cast<const char*>(&metadata.totalDuration),
                   sizeof(metadata.totalDuration));
        file.write(reinterpret_cast<const char*>(&metadata.totalFrames),
                   sizeof(metadata.totalFrames));
        file.write(reinterpret_cast<const char*>(&randomSeed), sizeof(randomSeed));
        writeString(file, initialStateHash);

        // Write events
        uint32_t eventCount = static_cast<uint32_t>(events.size());
        file.write(reinterpret_cast<const char*>(&eventCount), sizeof(eventCount));

        for (const auto& event : events) {
            file.write(reinterpret_cast<const char*>(&event.type), sizeof(event.type));
            file.write(reinterpret_cast<const char*>(&event.timestamp), sizeof(event.timestamp));
            file.write(reinterpret_cast<const char*>(&event.frameNumber),
                       sizeof(event.frameNumber));
            file.write(reinterpret_cast<const char*>(&event.keyCode), sizeof(event.keyCode));
            file.write(reinterpret_cast<const char*>(&event.buttonIndex),
                       sizeof(event.buttonIndex));
            file.write(reinterpret_cast<const char*>(&event.pressed), sizeof(event.pressed));
            file.write(reinterpret_cast<const char*>(&event.mouseX), sizeof(event.mouseX));
            file.write(reinterpret_cast<const char*>(&event.mouseY), sizeof(event.mouseY));
            file.write(reinterpret_cast<const char*>(&event.wheelX), sizeof(event.wheelX));
            file.write(reinterpret_cast<const char*>(&event.wheelY), sizeof(event.wheelY));
            file.write(reinterpret_cast<const char*>(&event.gamepadIndex),
                       sizeof(event.gamepadIndex));
            file.write(reinterpret_cast<const char*>(&event.axisValue), sizeof(event.axisValue));
            writeString(file, event.actionName);
            writeString(file, event.textData);
        }

        return true;
    }

    bool loadFromFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) return false;

        // Read and verify header
        char magic[7] = {0};
        file.read(magic, 6);
        if (std::string(magic) != "JJMREC") return false;

        uint32_t version;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        if (version != 1) return false;

        // Read metadata
        metadata.recordingName = readString(file);
        metadata.gameVersion = readString(file);
        metadata.levelName = readString(file);
        file.read(reinterpret_cast<char*>(&metadata.recordingDate), sizeof(metadata.recordingDate));
        file.read(reinterpret_cast<char*>(&metadata.totalDuration), sizeof(metadata.totalDuration));
        file.read(reinterpret_cast<char*>(&metadata.totalFrames), sizeof(metadata.totalFrames));
        file.read(reinterpret_cast<char*>(&randomSeed), sizeof(randomSeed));
        initialStateHash = readString(file);

        // Read events
        uint32_t eventCount;
        file.read(reinterpret_cast<char*>(&eventCount), sizeof(eventCount));
        events.resize(eventCount);

        for (auto& event : events) {
            file.read(reinterpret_cast<char*>(&event.type), sizeof(event.type));
            file.read(reinterpret_cast<char*>(&event.timestamp), sizeof(event.timestamp));
            file.read(reinterpret_cast<char*>(&event.frameNumber), sizeof(event.frameNumber));
            file.read(reinterpret_cast<char*>(&event.keyCode), sizeof(event.keyCode));
            file.read(reinterpret_cast<char*>(&event.buttonIndex), sizeof(event.buttonIndex));
            file.read(reinterpret_cast<char*>(&event.pressed), sizeof(event.pressed));
            file.read(reinterpret_cast<char*>(&event.mouseX), sizeof(event.mouseX));
            file.read(reinterpret_cast<char*>(&event.mouseY), sizeof(event.mouseY));
            file.read(reinterpret_cast<char*>(&event.wheelX), sizeof(event.wheelX));
            file.read(reinterpret_cast<char*>(&event.wheelY), sizeof(event.wheelY));
            file.read(reinterpret_cast<char*>(&event.gamepadIndex), sizeof(event.gamepadIndex));
            file.read(reinterpret_cast<char*>(&event.axisValue), sizeof(event.axisValue));
            event.actionName = readString(file);
            event.textData = readString(file);
        }

        metadata.eventCount = eventCount;
        return true;
    }

   private:
    void writeString(std::ofstream& file, const std::string& str) const {
        uint32_t len = static_cast<uint32_t>(str.length());
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(str.c_str(), len);
    }

    std::string readString(std::ifstream& file) {
        uint32_t len;
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string str(len, '\0');
        file.read(&str[0], len);
        return str;
    }
};

// Playback mode options
enum class PlaybackMode {
    Normal,       // Real-time playback
    FastForward,  // Accelerated playback
    StepFrame,    // Frame-by-frame
    SkipToFrame,  // Jump to specific frame
    Loop          // Loop continuously
};

// Input recorder/player
class InputRecorder {
   public:
    enum class State { Idle, Recording, Playing };

   private:
    State currentState;
    InputRecording currentRecording;

    // Recording state
    std::chrono::high_resolution_clock::time_point recordingStartTime;
    uint32_t recordingStartFrame;
    uint64_t lastMouseSampleTime;

    // Playback state
    size_t playbackEventIndex;
    uint64_t playbackStartTime;
    uint32_t playbackStartFrame;
    uint32_t currentPlaybackFrame;
    PlaybackMode playbackMode;
    float playbackSpeed;
    bool playbackPaused;

    // Settings
    bool recordMouseMovement;
    bool recordGamepadAxes;
    float mouseSampleInterval;  // Microseconds

    // Callbacks
    std::function<void()> onRecordingStarted;
    std::function<void(const InputRecording&)> onRecordingFinished;
    std::function<void()> onPlaybackStarted;
    std::function<void()> onPlaybackFinished;
    std::function<void(const RecordedInputEvent&)> onEventPlayback;

   public:
    InputRecorder()
        : currentState(State::Idle),
          playbackEventIndex(0),
          playbackStartTime(0),
          playbackStartFrame(0),
          currentPlaybackFrame(0),
          playbackMode(PlaybackMode::Normal),
          playbackSpeed(1.0f),
          playbackPaused(false),
          recordMouseMovement(true),
          recordGamepadAxes(true),
          mouseSampleInterval(16667)  // ~60Hz
          ,
          lastMouseSampleTime(0),
          recordingStartFrame(0) {}

    // Recording control
    void startRecording(const std::string& name, const std::string& gameVersion = "") {
        if (currentState != State::Idle) return;

        currentRecording = InputRecording();
        currentRecording.metadata.recordingName = name;
        currentRecording.metadata.gameVersion = gameVersion;
        currentRecording.metadata.recordingDate =
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        currentRecording.metadata.includeMouseMovement = recordMouseMovement;
        currentRecording.metadata.includeGamepadAxes = recordGamepadAxes;

        recordingStartTime = std::chrono::high_resolution_clock::now();
        recordingStartFrame = 0;
        lastMouseSampleTime = 0;
        currentState = State::Recording;

        if (onRecordingStarted) onRecordingStarted();
    }

    void stopRecording() {
        if (currentState != State::Recording) return;

        auto now = std::chrono::high_resolution_clock::now();
        currentRecording.metadata.totalDuration =
            std::chrono::duration_cast<std::chrono::microseconds>(now - recordingStartTime).count();
        currentRecording.metadata.eventCount =
            static_cast<uint32_t>(currentRecording.events.size());

        currentState = State::Idle;

        if (onRecordingFinished) onRecordingFinished(currentRecording);
    }

    // Record events
    void recordKeyEvent(int keyCode, bool pressed, uint32_t frame) {
        if (currentState != State::Recording) return;

        RecordedInputEvent event;
        event.type = pressed ? RecordedInputType::KeyDown : RecordedInputType::KeyUp;
        event.timestamp = getRecordingTimestamp();
        event.frameNumber = frame;
        event.keyCode = keyCode;
        event.pressed = pressed;

        currentRecording.events.push_back(event);
    }

    void recordMouseButtonEvent(int button, bool pressed, float x, float y, uint32_t frame) {
        if (currentState != State::Recording) return;

        RecordedInputEvent event;
        event.type =
            pressed ? RecordedInputType::MouseButtonDown : RecordedInputType::MouseButtonUp;
        event.timestamp = getRecordingTimestamp();
        event.frameNumber = frame;
        event.buttonIndex = button;
        event.pressed = pressed;
        event.mouseX = x;
        event.mouseY = y;

        currentRecording.events.push_back(event);
    }

    void recordMouseMove(float x, float y, uint32_t frame) {
        if (currentState != State::Recording || !recordMouseMovement) return;

        uint64_t now = getRecordingTimestamp();
        if (now - lastMouseSampleTime < mouseSampleInterval) return;
        lastMouseSampleTime = now;

        RecordedInputEvent event;
        event.type = RecordedInputType::MouseMove;
        event.timestamp = now;
        event.frameNumber = frame;
        event.mouseX = x;
        event.mouseY = y;

        currentRecording.events.push_back(event);
    }

    void recordMouseWheel(float x, float y, uint32_t frame) {
        if (currentState != State::Recording) return;

        RecordedInputEvent event;
        event.type = RecordedInputType::MouseWheel;
        event.timestamp = getRecordingTimestamp();
        event.frameNumber = frame;
        event.wheelX = x;
        event.wheelY = y;

        currentRecording.events.push_back(event);
    }

    void recordGamepadButton(int padIndex, int button, bool pressed, uint32_t frame) {
        if (currentState != State::Recording) return;

        RecordedInputEvent event;
        event.type = RecordedInputType::GamepadButton;
        event.timestamp = getRecordingTimestamp();
        event.frameNumber = frame;
        event.gamepadIndex = padIndex;
        event.buttonIndex = button;
        event.pressed = pressed;

        currentRecording.events.push_back(event);
    }

    void recordGamepadAxis(int padIndex, int axis, float value, uint32_t frame) {
        if (currentState != State::Recording || !recordGamepadAxes) return;

        RecordedInputEvent event;
        event.type = RecordedInputType::GamepadAxis;
        event.timestamp = getRecordingTimestamp();
        event.frameNumber = frame;
        event.gamepadIndex = padIndex;
        event.buttonIndex = axis;
        event.axisValue = value;

        currentRecording.events.push_back(event);
    }

    void recordActionTrigger(const std::string& actionName, bool pressed, uint32_t frame) {
        if (currentState != State::Recording) return;

        RecordedInputEvent event;
        event.type = RecordedInputType::ActionTrigger;
        event.timestamp = getRecordingTimestamp();
        event.frameNumber = frame;
        event.actionName = actionName;
        event.pressed = pressed;

        currentRecording.events.push_back(event);
    }

    void recordTextInput(const std::string& text, uint32_t frame) {
        if (currentState != State::Recording) return;

        RecordedInputEvent event;
        event.type = RecordedInputType::TextInput;
        event.timestamp = getRecordingTimestamp();
        event.frameNumber = frame;
        event.textData = text;

        currentRecording.events.push_back(event);
    }

    // Playback control
    void startPlayback(const InputRecording& recording) {
        if (currentState != State::Idle) return;

        currentRecording = recording;
        playbackEventIndex = 0;
        playbackStartTime = getCurrentTimeMicros();
        playbackStartFrame = 0;
        currentPlaybackFrame = 0;
        playbackPaused = false;
        currentState = State::Playing;

        if (onPlaybackStarted) onPlaybackStarted();
    }

    void stopPlayback() {
        if (currentState != State::Playing) return;

        currentState = State::Idle;
        if (onPlaybackFinished) onPlaybackFinished();
    }

    void pausePlayback() { playbackPaused = true; }
    void resumePlayback() { playbackPaused = false; }
    bool isPlaybackPaused() const { return playbackPaused; }

    void setPlaybackSpeed(float speed) { playbackSpeed = std::max(0.1f, speed); }
    float getPlaybackSpeed() const { return playbackSpeed; }

    void setPlaybackMode(PlaybackMode mode) { playbackMode = mode; }
    PlaybackMode getPlaybackMode() const { return playbackMode; }

    // Update playback - returns events that should fire this frame
    std::vector<RecordedInputEvent> updatePlayback(uint32_t currentFrame) {
        std::vector<RecordedInputEvent> eventsToFire;

        if (currentState != State::Playing || playbackPaused) {
            return eventsToFire;
        }

        currentPlaybackFrame = currentFrame;
        uint64_t currentTime = getCurrentTimeMicros() - playbackStartTime;
        currentTime = static_cast<uint64_t>(currentTime * playbackSpeed);

        while (playbackEventIndex < currentRecording.events.size()) {
            const auto& event = currentRecording.events[playbackEventIndex];

            if (event.timestamp <= currentTime) {
                eventsToFire.push_back(event);
                if (onEventPlayback) onEventPlayback(event);
                ++playbackEventIndex;
            } else {
                break;
            }
        }

        // Check for end of playback
        if (playbackEventIndex >= currentRecording.events.size()) {
            if (playbackMode == PlaybackMode::Loop) {
                // Reset for loop
                playbackEventIndex = 0;
                playbackStartTime = getCurrentTimeMicros();
            } else {
                stopPlayback();
            }
        }

        return eventsToFire;
    }

    // Step one frame forward (for frame-by-frame mode)
    std::vector<RecordedInputEvent> stepFrame(uint32_t targetFrame) {
        std::vector<RecordedInputEvent> eventsToFire;

        if (currentState != State::Playing) return eventsToFire;

        while (playbackEventIndex < currentRecording.events.size()) {
            const auto& event = currentRecording.events[playbackEventIndex];

            if (event.frameNumber <= targetFrame) {
                eventsToFire.push_back(event);
                if (onEventPlayback) onEventPlayback(event);
                ++playbackEventIndex;
            } else {
                break;
            }
        }

        return eventsToFire;
    }

    // Skip to specific frame
    void skipToFrame(uint32_t frame) {
        if (currentState != State::Playing) return;

        // Find first event at or after target frame
        for (size_t i = 0; i < currentRecording.events.size(); ++i) {
            if (currentRecording.events[i].frameNumber >= frame) {
                playbackEventIndex = i;
                return;
            }
        }

        playbackEventIndex = currentRecording.events.size();
    }

    // State queries
    State getState() const { return currentState; }
    bool isRecording() const { return currentState == State::Recording; }
    bool isPlaying() const { return currentState == State::Playing; }

    float getPlaybackProgress() const {
        if (currentRecording.events.empty()) return 0.0f;
        return static_cast<float>(playbackEventIndex) / currentRecording.events.size();
    }

    uint32_t getCurrentPlaybackFrame() const { return currentPlaybackFrame; }
    size_t getRecordedEventCount() const { return currentRecording.events.size(); }

    // Access recording
    const InputRecording& getRecording() const { return currentRecording; }
    InputRecording& getRecording() { return currentRecording; }

    // Settings
    void setRecordMouseMovement(bool record) { recordMouseMovement = record; }
    void setRecordGamepadAxes(bool record) { recordGamepadAxes = record; }
    void setMouseSampleRate(float hz) {
        mouseSampleInterval = static_cast<uint64_t>(1000000.0f / hz);
    }

    // Callbacks
    void setOnRecordingStarted(std::function<void()> cb) { onRecordingStarted = cb; }
    void setOnRecordingFinished(std::function<void(const InputRecording&)> cb) {
        onRecordingFinished = cb;
    }
    void setOnPlaybackStarted(std::function<void()> cb) { onPlaybackStarted = cb; }
    void setOnPlaybackFinished(std::function<void()> cb) { onPlaybackFinished = cb; }
    void setOnEventPlayback(std::function<void(const RecordedInputEvent&)> cb) {
        onEventPlayback = cb;
    }

   private:
    uint64_t getRecordingTimestamp() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - recordingStartTime)
            .count();
    }

    uint64_t getCurrentTimeMicros() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    }
};

// Input macro system for automated input sequences
struct InputMacroStep {
    RecordedInputEvent event;
    uint64_t delayMicros;  // Delay before this step
};

class InputMacro {
   public:
    std::string name;
    std::string description;
    std::vector<InputMacroStep> steps;
    bool looping;

    InputMacro() : looping(false) {}

    void addKeyPress(int keyCode, uint64_t delayMs = 0) {
        InputMacroStep stepDown, stepUp;
        stepDown.event.type = RecordedInputType::KeyDown;
        stepDown.event.keyCode = keyCode;
        stepDown.event.pressed = true;
        stepDown.delayMicros = delayMs * 1000;

        stepUp.event.type = RecordedInputType::KeyUp;
        stepUp.event.keyCode = keyCode;
        stepUp.event.pressed = false;
        stepUp.delayMicros = 50000;  // 50ms hold

        steps.push_back(stepDown);
        steps.push_back(stepUp);
    }

    void addMouseClick(int button, float x, float y, uint64_t delayMs = 0) {
        InputMacroStep stepDown, stepUp;
        stepDown.event.type = RecordedInputType::MouseButtonDown;
        stepDown.event.buttonIndex = button;
        stepDown.event.mouseX = x;
        stepDown.event.mouseY = y;
        stepDown.event.pressed = true;
        stepDown.delayMicros = delayMs * 1000;

        stepUp.event.type = RecordedInputType::MouseButtonUp;
        stepUp.event.buttonIndex = button;
        stepUp.event.mouseX = x;
        stepUp.event.mouseY = y;
        stepUp.event.pressed = false;
        stepUp.delayMicros = 50000;

        steps.push_back(stepDown);
        steps.push_back(stepUp);
    }

    void addDelay(uint64_t delayMs) {
        if (!steps.empty()) {
            steps.back().delayMicros += delayMs * 1000;
        }
    }
};

class InputMacroPlayer {
   private:
    std::unordered_map<std::string, InputMacro> macros;
    const InputMacro* currentMacro;
    size_t currentStep;
    uint64_t stepStartTime;
    bool playing;
    bool paused;

    std::function<void(const RecordedInputEvent&)> onMacroEvent;

   public:
    InputMacroPlayer()
        : currentMacro(nullptr), currentStep(0), stepStartTime(0), playing(false), paused(false) {}

    void registerMacro(const InputMacro& macro) { macros[macro.name] = macro; }

    void unregisterMacro(const std::string& name) { macros.erase(name); }

    bool hasMacro(const std::string& name) const { return macros.find(name) != macros.end(); }

    void playMacro(const std::string& name) {
        auto it = macros.find(name);
        if (it == macros.end()) return;

        currentMacro = &it->second;
        currentStep = 0;
        stepStartTime = getCurrentTimeMicros();
        playing = true;
        paused = false;
    }

    void stopMacro() {
        currentMacro = nullptr;
        playing = false;
    }

    void pause() { paused = true; }
    void resume() { paused = false; }

    std::vector<RecordedInputEvent> update() {
        std::vector<RecordedInputEvent> events;

        if (!playing || paused || !currentMacro) return events;

        uint64_t now = getCurrentTimeMicros();

        while (currentStep < currentMacro->steps.size()) {
            const auto& step = currentMacro->steps[currentStep];

            if (now - stepStartTime >= step.delayMicros) {
                events.push_back(step.event);
                if (onMacroEvent) onMacroEvent(step.event);

                stepStartTime = now;
                ++currentStep;
            } else {
                break;
            }
        }

        // Check for completion
        if (currentStep >= currentMacro->steps.size()) {
            if (currentMacro->looping) {
                currentStep = 0;
                stepStartTime = now;
            } else {
                stopMacro();
            }
        }

        return events;
    }

    bool isPlaying() const { return playing; }
    bool isPaused() const { return paused; }

    void setOnMacroEvent(std::function<void(const RecordedInputEvent&)> cb) { onMacroEvent = cb; }

    std::vector<std::string> getMacroNames() const {
        std::vector<std::string> names;
        for (const auto& pair : macros) {
            names.push_back(pair.first);
        }
        return names;
    }

   private:
    uint64_t getCurrentTimeMicros() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    }
};

enum class MouseButton {
    LEFT = SDL_BUTTON_LEFT,
    MIDDLE = SDL_BUTTON_MIDDLE,
    RIGHT = SDL_BUTTON_RIGHT
};

// Gamepad button mapping
enum class GamepadButton {
    A,
    B,
    X,
    Y,
    LeftBumper,
    RightBumper,
    Back,
    Start,
    Guide,
    LeftStick,
    RightStick,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
    Count
};

// Gamepad axis mapping
enum class GamepadAxis { LeftX, LeftY, RightX, RightY, LeftTrigger, RightTrigger, Count };

// Gamepad state
struct GamepadState {
    bool connected;
    std::string name;
    SDL_GameController* controller;
    std::unordered_map<GamepadButton, KeyState> buttons;
    std::unordered_map<GamepadAxis, float> axes;
    float deadzone;
    int playerIndex;

    GamepadState() : connected(false), controller(nullptr), deadzone(0.15f), playerIndex(-1) {}
};

// Input action binding
struct InputAction {
    std::string name;
    std::vector<SDL_Keycode> keyBindings;
    std::vector<MouseButton> mouseBindings;
    std::vector<std::pair<int, GamepadButton>> gamepadBindings;
};

// Callbacks
using InputCallback = std::function<void()>;
using AxisCallback = std::function<void(float)>;

class InputManager {
   private:
    std::unordered_map<SDL_Keycode, KeyState> keyStates;
    std::unordered_map<Uint8, KeyState> mouseButtonStates;
    Math::Vector2D mousePosition;
    Math::Vector2D mouseMotion;
    Math::Vector2D mouseWheel;
    bool quitRequested;

    // Gamepad support
    std::vector<GamepadState> gamepads;
    static constexpr int MAX_GAMEPADS = 4;

    // Input action system
    std::unordered_map<std::string, InputAction> actions;
    std::unordered_map<std::string, std::vector<InputCallback>> actionPressedCallbacks;
    std::unordered_map<std::string, std::vector<InputCallback>> actionReleasedCallbacks;

    // Text input
    bool textInputActive;
    std::string textInputBuffer;
    std::function<void(const std::string&)> onTextInput;

   public:
    InputManager();
    ~InputManager();

    // Update input state
    void update();
    void handleEvent(const SDL_Event& event);

    // Keyboard input
    bool isKeyDown(SDL_Keycode key) const;
    bool isKeyUp(SDL_Keycode key) const;
    bool isKeyPressed(SDL_Keycode key) const;
    bool isKeyReleased(SDL_Keycode key) const;

    // Mouse input
    bool isMouseButtonDown(MouseButton button) const;
    bool isMouseButtonUp(MouseButton button) const;
    bool isMouseButtonPressed(MouseButton button) const;
    bool isMouseButtonReleased(MouseButton button) const;

    // Mouse position and motion
    Math::Vector2D getMousePosition() const { return mousePosition; }
    Math::Vector2D getMouseMotion() const { return mouseMotion; }
    Math::Vector2D getMouseWheel() const { return mouseWheel; }
    void setMousePosition(int x, int y);
    void setMouseVisible(bool visible);
    void setMouseRelativeMode(bool enabled);

    // Gamepad support
    void initializeGamepads();
    void shutdownGamepads();
    bool isGamepadConnected(int playerIndex) const;
    int getConnectedGamepadCount() const;
    std::string getGamepadName(int playerIndex) const;

    // Gamepad buttons
    bool isGamepadButtonDown(int playerIndex, GamepadButton button) const;
    bool isGamepadButtonPressed(int playerIndex, GamepadButton button) const;
    bool isGamepadButtonReleased(int playerIndex, GamepadButton button) const;

    // Gamepad axes
    float getGamepadAxis(int playerIndex, GamepadAxis axis) const;
    Math::Vector2D getGamepadLeftStick(int playerIndex) const;
    Math::Vector2D getGamepadRightStick(int playerIndex) const;
    float getGamepadLeftTrigger(int playerIndex) const;
    float getGamepadRightTrigger(int playerIndex) const;
    void setGamepadDeadzone(int playerIndex, float deadzone);

    // Gamepad rumble/haptics
    void setGamepadRumble(int playerIndex, float lowFreq, float highFreq, float duration);
    void stopGamepadRumble(int playerIndex);

    // Input actions (binding system)
    void registerAction(const std::string& name);
    void bindKeyToAction(const std::string& action, SDL_Keycode key);
    void bindMouseToAction(const std::string& action, MouseButton button);
    void bindGamepadToAction(const std::string& action, int playerIndex, GamepadButton button);
    void unbindAction(const std::string& action);

    bool isActionPressed(const std::string& action) const;
    bool isActionDown(const std::string& action) const;
    bool isActionReleased(const std::string& action) const;

    void onActionPressed(const std::string& action, InputCallback callback);
    void onActionReleased(const std::string& action, InputCallback callback);

    // Input rebinding system
    void rebindKey(const std::string& action, SDL_Keycode newKey);
    void rebindMouseButton(const std::string& action, MouseButton newButton);
    void rebindGamepadButton(const std::string& action, int playerIndex, GamepadButton newButton);

    // Get current bindings
    std::vector<SDL_Keycode> getKeysForAction(const std::string& action) const;
    std::vector<MouseButton> getMouseButtonsForAction(const std::string& action) const;
    std::vector<GamepadButton> getGamepadButtonsForAction(const std::string& action,
                                                          int playerIndex) const;

    // Save/load bindings
    bool saveBindings(const std::string& filePath) const;
    bool loadBindings(const std::string& filePath);
    void resetBindingsToDefault();
    void setDefaultBindings(
        const std::unordered_map<std::string, std::vector<SDL_Keycode>>& defaults);

    // Text input
    void startTextInput();
    void stopTextInput();
    bool isTextInputActive() const { return textInputActive; }
    const std::string& getTextInputBuffer() const { return textInputBuffer; }
    void clearTextInputBuffer() { textInputBuffer.clear(); }
    void setTextInputCallback(std::function<void(const std::string&)> callback);

    // Utility
    bool shouldQuit() const { return quitRequested; }
    void reset();
    bool anyKeyPressed() const;
    bool anyGamepadButtonPressed(int playerIndex) const;

   private:
    void updateKeyState(SDL_Keycode key, bool down);
    void updateMouseButtonState(Uint8 button, bool down);
    void updateGamepadButtonState(int index, GamepadButton button, bool down);
    KeyState getKeyState(SDL_Keycode key) const;
    KeyState getMouseButtonState(Uint8 button) const;
    void handleGamepadEvent(const SDL_Event& event);
    float applyDeadzone(float value, float deadzone) const;
    void triggerActionCallbacks(const std::string& action, bool pressed);
};

}  // namespace Input
}  // namespace JJM

#endif  // INPUT_MANAGER_H
