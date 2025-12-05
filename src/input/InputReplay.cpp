#include "input/InputReplay.h"
#include <iostream>

namespace JJM {
namespace Input {

InputRecorder::InputRecorder() : recording(false) {}

void InputRecorder::startRecording() {
    recording = true;
    events.clear();
    std::cout << "Started input recording" << std::endl;
}

void InputRecorder::stopRecording() {
    recording = false;
    std::cout << "Stopped recording. Total events: " << events.size() << std::endl;
}

void InputRecorder::recordKeyDown(int keyCode, float timestamp) {
    if (!recording) return;
    RecordedInputEvent event;
    event.type = InputEventType::KeyDown;
    event.timestamp = timestamp;
    event.keyCode = keyCode;
    events.push_back(event);
}

void InputRecorder::recordKeyUp(int keyCode, float timestamp) {
    if (!recording) return;
    RecordedInputEvent event;
    event.type = InputEventType::KeyUp;
    event.timestamp = timestamp;
    event.keyCode = keyCode;
    events.push_back(event);
}

void InputRecorder::recordMouseMove(float x, float y, float timestamp) {
    if (!recording) return;
    RecordedInputEvent event;
    event.type = InputEventType::MouseMove;
    event.timestamp = timestamp;
    event.mouseX = x;
    event.mouseY = y;
    events.push_back(event);
}

void InputRecorder::recordMouseButton(int button, bool down, float timestamp) {
    if (!recording) return;
    RecordedInputEvent event;
    event.type = down ? InputEventType::MouseButtonDown : InputEventType::MouseButtonUp;
    event.timestamp = timestamp;
    event.mouseButton = button;
    events.push_back(event);
}

void InputRecorder::recordMouseWheel(float delta, float timestamp) {
    if (!recording) return;
    RecordedInputEvent event;
    event.type = InputEventType::MouseWheel;
    event.timestamp = timestamp;
    event.wheelDelta = delta;
    events.push_back(event);
}

bool InputRecorder::saveToFile(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to save replay: " << filename << std::endl;
        return false;
    }
    
    size_t count = events.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    file.write(reinterpret_cast<const char*>(events.data()), sizeof(RecordedInputEvent) * count);
    
    std::cout << "Saved " << count << " events to " << filename << std::endl;
    return true;
}

void InputRecorder::clear() {
    events.clear();
}

InputReplayer::InputReplayer()
    : currentEventIndex(0), currentTime(0), playing(false), paused(false), playbackSpeed(1.0f) {}

bool InputReplayer::loadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to load replay: " << filename << std::endl;
        return false;
    }
    
    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    events.resize(count);
    file.read(reinterpret_cast<char*>(events.data()), sizeof(RecordedInputEvent) * count);
    
    std::cout << "Loaded " << count << " events from " << filename << std::endl;
    return true;
}

void InputReplayer::startPlayback() {
    playing = true;
    paused = false;
    currentEventIndex = 0;
    currentTime = 0;
    std::cout << "Started playback" << std::endl;
}

void InputReplayer::stopPlayback() {
    playing = false;
    paused = false;
    currentEventIndex = 0;
    currentTime = 0;
    std::cout << "Stopped playback" << std::endl;
}

void InputReplayer::pausePlayback() {
    paused = true;
    std::cout << "Paused playback" << std::endl;
}

void InputReplayer::resumePlayback() {
    paused = false;
    std::cout << "Resumed playback" << std::endl;
}

void InputReplayer::update(float deltaTime) {
    if (!playing || paused || currentEventIndex >= events.size()) {
        return;
    }
    
    currentTime += deltaTime * playbackSpeed;
    
    while (currentEventIndex < events.size() && events[currentEventIndex].timestamp <= currentTime) {
        processEvent(events[currentEventIndex]);
        currentEventIndex++;
    }
    
    if (currentEventIndex >= events.size()) {
        std::cout << "Playback finished" << std::endl;
        playing = false;
    }
}

void InputReplayer::processEvent(const RecordedInputEvent& event) {
    switch (event.type) {
        case InputEventType::KeyDown:
            std::cout << "Replay KeyDown: " << event.keyCode << std::endl;
            break;
        case InputEventType::KeyUp:
            std::cout << "Replay KeyUp: " << event.keyCode << std::endl;
            break;
        case InputEventType::MouseMove:
            std::cout << "Replay MouseMove: " << event.mouseX << ", " << event.mouseY << std::endl;
            break;
        case InputEventType::MouseButtonDown:
            std::cout << "Replay MouseDown: " << event.mouseButton << std::endl;
            break;
        case InputEventType::MouseButtonUp:
            std::cout << "Replay MouseUp: " << event.mouseButton << std::endl;
            break;
        case InputEventType::MouseWheel:
            std::cout << "Replay MouseWheel: " << event.wheelDelta << std::endl;
            break;
    }
}

float InputReplayer::getProgress() const {
    if (events.empty()) return 0.0f;
    return currentTime / getDuration();
}

float InputReplayer::getDuration() const {
    if (events.empty()) return 0.0f;
    return events.back().timestamp;
}

void InputReplayer::clear() {
    events.clear();
    currentEventIndex = 0;
    currentTime = 0;
}

} // namespace Input
} // namespace JJM
