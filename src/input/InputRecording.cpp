#include "input/InputRecording.h"
#include <fstream>
#include <algorithm>

namespace JJM {
namespace Input {

// InputRecording implementation
InputRecording::InputRecording() : startTime(0) {
}

InputRecording::~InputRecording() {
}

void InputRecording::clear() {
    events.clear();
    metadata.clear();
    startTime = 0;
}

void InputRecording::addEvent(const InputEvent& event) {
    events.push_back(event);
}

size_t InputRecording::getEventCount() const {
    return events.size();
}

const std::vector<InputEvent>& InputRecording::getEvents() const {
    return events;
}

InputEvent InputRecording::getEvent(size_t index) const {
    return events[index];
}

bool InputRecording::saveToFile(const std::string& filename) {
    return RecordingFormat::saveBinary(*this, filename);
}

bool InputRecording::loadFromFile(const std::string& filename) {
    return RecordingFormat::loadBinary(*this, filename);
}

void InputRecording::setMetadata(const std::string& key, const std::string& value) {
    metadata[key] = value;
}

std::string InputRecording::getMetadata(const std::string& key) const {
    auto it = metadata.find(key);
    return it != metadata.end() ? it->second : "";
}

uint32_t InputRecording::getDuration() const {
    if (events.empty()) return 0;
    return events.back().timestamp;
}

// InputRecorder implementation
InputRecorder& InputRecorder::getInstance() {
    static InputRecorder instance;
    return instance;
}

InputRecorder::InputRecorder() : recording(false), paused(false) {
    currentRecording = std::make_shared<InputRecording>();
}

InputRecorder::~InputRecorder() {
}

void InputRecorder::startRecording() {
    recording = true;
    paused = false;
    currentRecording->clear();
    startTime = std::chrono::steady_clock::now();
}

void InputRecorder::stopRecording() {
    recording = false;
    paused = false;
}

void InputRecorder::pauseRecording() {
    paused = true;
}

void InputRecorder::resumeRecording() {
    paused = false;
}

bool InputRecorder::isRecording() const {
    return recording;
}

bool InputRecorder::isPaused() const {
    return paused;
}

void InputRecorder::recordKeyPress(int keyCode) {
    if (!recording || paused) return;
    
    InputEvent event;
    event.type = InputEventType::KeyPress;
    event.timestamp = getElapsedTime();
    event.keyCode = keyCode;
    currentRecording->addEvent(event);
}

void InputRecorder::recordKeyRelease(int keyCode) {
    if (!recording || paused) return;
    
    InputEvent event;
    event.type = InputEventType::KeyRelease;
    event.timestamp = getElapsedTime();
    event.keyCode = keyCode;
    currentRecording->addEvent(event);
}

void InputRecorder::recordMouseMove(int x, int y) {
    if (!recording || paused) return;
    
    InputEvent event;
    event.type = InputEventType::MouseMove;
    event.timestamp = getElapsedTime();
    event.mouseX = x;
    event.mouseY = y;
    currentRecording->addEvent(event);
}

void InputRecorder::recordMouseButtonPress(int button, int x, int y) {
    if (!recording || paused) return;
    
    InputEvent event;
    event.type = InputEventType::MouseButtonPress;
    event.timestamp = getElapsedTime();
    event.mouseButton = button;
    event.mouseX = x;
    event.mouseY = y;
    currentRecording->addEvent(event);
}

void InputRecorder::recordMouseButtonRelease(int button, int x, int y) {
    if (!recording || paused) return;
    
    InputEvent event;
    event.type = InputEventType::MouseButtonRelease;
    event.timestamp = getElapsedTime();
    event.mouseButton = button;
    event.mouseX = x;
    event.mouseY = y;
    currentRecording->addEvent(event);
}

void InputRecorder::recordMouseWheel(float delta) {
    if (!recording || paused) return;
    
    InputEvent event;
    event.type = InputEventType::MouseWheel;
    event.timestamp = getElapsedTime();
    event.wheelDelta = delta;
    currentRecording->addEvent(event);
}

void InputRecorder::recordGamepadButton(int gamepadId, int button) {
    if (!recording || paused) return;
    
    InputEvent event;
    event.type = InputEventType::GamepadButton;
    event.timestamp = getElapsedTime();
    event.gamepadId = gamepadId;
    event.gamepadButton = button;
    currentRecording->addEvent(event);
}

void InputRecorder::recordGamepadAxis(int gamepadId, int axis, float value) {
    if (!recording || paused) return;
    
    InputEvent event;
    event.type = InputEventType::GamepadAxis;
    event.timestamp = getElapsedTime();
    event.gamepadId = gamepadId;
    event.gamepadButton = axis;
    event.gamepadAxisValue = value;
    currentRecording->addEvent(event);
}

void InputRecorder::recordTouchDown(int touchId, float x, float y) {
    if (!recording || paused) return;
    
    InputEvent event;
    event.type = InputEventType::TouchDown;
    event.timestamp = getElapsedTime();
    event.touchId = touchId;
    event.touchX = x;
    event.touchY = y;
    currentRecording->addEvent(event);
}

void InputRecorder::recordTouchUp(int touchId, float x, float y) {
    if (!recording || paused) return;
    
    InputEvent event;
    event.type = InputEventType::TouchUp;
    event.timestamp = getElapsedTime();
    event.touchId = touchId;
    event.touchX = x;
    event.touchY = y;
    currentRecording->addEvent(event);
}

void InputRecorder::recordTouchMove(int touchId, float x, float y) {
    if (!recording || paused) return;
    
    InputEvent event;
    event.type = InputEventType::TouchMove;
    event.timestamp = getElapsedTime();
    event.touchId = touchId;
    event.touchX = x;
    event.touchY = y;
    currentRecording->addEvent(event);
}

std::shared_ptr<InputRecording> InputRecorder::getCurrentRecording() {
    return currentRecording;
}

bool InputRecorder::saveRecording(const std::string& filename) {
    return currentRecording->saveToFile(filename);
}

void InputRecorder::clear() {
    currentRecording->clear();
}

uint32_t InputRecorder::getElapsedTime() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
    return static_cast<uint32_t>(duration.count());
}

// InputPlayback implementation
InputPlayback& InputPlayback::getInstance() {
    static InputPlayback instance;
    return instance;
}

InputPlayback::InputPlayback()
    : playing(false), paused(false), looping(false),
      playbackSpeed(1.0f), currentTime(0), currentEventIndex(0) {
}

InputPlayback::~InputPlayback() {
}

bool InputPlayback::loadRecording(const std::string& filename) {
    auto newRecording = std::make_shared<InputRecording>();
    if (newRecording->loadFromFile(filename)) {
        recording = newRecording;
        return true;
    }
    return false;
}

bool InputPlayback::loadRecording(std::shared_ptr<InputRecording> recording) {
    this->recording = recording;
    return recording != nullptr;
}

void InputPlayback::startPlayback() {
    if (!recording) return;
    
    playing = true;
    paused = false;
    currentTime = 0;
    currentEventIndex = 0;
    startTime = std::chrono::steady_clock::now();
}

void InputPlayback::stopPlayback() {
    playing = false;
    paused = false;
    currentTime = 0;
    currentEventIndex = 0;
}

void InputPlayback::pausePlayback() {
    paused = true;
}

void InputPlayback::resumePlayback() {
    paused = false;
    startTime = std::chrono::steady_clock::now();
}

bool InputPlayback::isPlaying() const {
    return playing;
}

bool InputPlayback::isPaused() const {
    return paused;
}

void InputPlayback::update(float deltaTime) {
    if (!playing || paused || !recording) return;
    
    currentTime += static_cast<uint32_t>(deltaTime * 1000.0f * playbackSpeed);
    
    processEvents();
    
    if (currentEventIndex >= recording->getEventCount()) {
        if (looping) {
            currentTime = 0;
            currentEventIndex = 0;
            startTime = std::chrono::steady_clock::now();
        } else {
            stopPlayback();
        }
    }
}

void InputPlayback::setPlaybackSpeed(float speed) {
    playbackSpeed = speed;
}

float InputPlayback::getPlaybackSpeed() const {
    return playbackSpeed;
}

void InputPlayback::setLooping(bool loop) {
    looping = loop;
}

bool InputPlayback::isLooping() const {
    return looping;
}

void InputPlayback::seekToTime(uint32_t time) {
    currentTime = time;
    currentEventIndex = 0;
    
    const auto& events = recording->getEvents();
    for (size_t i = 0; i < events.size(); i++) {
        if (events[i].timestamp > time) break;
        currentEventIndex = i;
    }
}

void InputPlayback::seekToEvent(size_t eventIndex) {
    if (eventIndex >= recording->getEventCount()) return;
    
    currentEventIndex = eventIndex;
    currentTime = recording->getEvent(eventIndex).timestamp;
}

uint32_t InputPlayback::getCurrentTime() const {
    return currentTime;
}

size_t InputPlayback::getCurrentEventIndex() const {
    return currentEventIndex;
}

std::shared_ptr<InputRecording> InputPlayback::getRecording() {
    return recording;
}

void InputPlayback::processEvents() {
    if (!recording) return;
    
    const auto& events = recording->getEvents();
    while (currentEventIndex < events.size() &&
           events[currentEventIndex].timestamp <= currentTime) {
        executeEvent(events[currentEventIndex]);
        currentEventIndex++;
    }
}

void InputPlayback::executeEvent(const InputEvent& event) {
    // Stub implementation - would inject input event
    (void)event;
}

// InputReplaySystem implementation
InputReplaySystem& InputReplaySystem::getInstance() {
    static InputReplaySystem instance;
    return instance;
}

InputReplaySystem::InputReplaySystem() {
}

InputReplaySystem::~InputReplaySystem() {
}

void InputReplaySystem::update(float deltaTime) {
    InputPlayback::getInstance().update(deltaTime);
}

InputRecorder& InputReplaySystem::getRecorder() {
    return InputRecorder::getInstance();
}

InputPlayback& InputReplaySystem::getPlayback() {
    return InputPlayback::getInstance();
}

void InputReplaySystem::startRecording() {
    InputRecorder::getInstance().startRecording();
}

void InputReplaySystem::stopRecording() {
    InputRecorder::getInstance().stopRecording();
}

void InputReplaySystem::startPlayback(const std::string& filename) {
    auto& playback = InputPlayback::getInstance();
    if (playback.loadRecording(filename)) {
        playback.startPlayback();
    }
}

void InputReplaySystem::stopPlayback() {
    InputPlayback::getInstance().stopPlayback();
}

bool InputReplaySystem::isRecording() const {
    return InputRecorder::getInstance().isRecording();
}

bool InputReplaySystem::isPlaying() const {
    return InputPlayback::getInstance().isPlaying();
}

// InputMacro implementation
InputMacro::InputMacro(const std::string& name)
    : name(name), executing(false), repeatCount(1), delay(0) {
}

InputMacro::~InputMacro() {
}

void InputMacro::addEvent(const InputEvent& event) {
    events.push_back(event);
}

void InputMacro::clear() {
    events.clear();
}

void InputMacro::execute() {
    executing = true;
}

void InputMacro::stop() {
    executing = false;
}

bool InputMacro::isExecuting() const {
    return executing;
}

void InputMacro::setRepeatCount(int count) {
    repeatCount = count;
}

int InputMacro::getRepeatCount() const {
    return repeatCount;
}

void InputMacro::setDelay(uint32_t delayMs) {
    delay = delayMs;
}

uint32_t InputMacro::getDelay() const {
    return delay;
}

std::string InputMacro::getName() const {
    return name;
}

// MacroManager implementation
MacroManager& MacroManager::getInstance() {
    static MacroManager instance;
    return instance;
}

MacroManager::MacroManager() {
}

MacroManager::~MacroManager() {
}

void MacroManager::addMacro(const std::string& name, std::shared_ptr<InputMacro> macro) {
    macros[name] = macro;
}

void MacroManager::removeMacro(const std::string& name) {
    macros.erase(name);
}

std::shared_ptr<InputMacro> MacroManager::getMacro(const std::string& name) {
    auto it = macros.find(name);
    return it != macros.end() ? it->second : nullptr;
}

bool MacroManager::hasMacro(const std::string& name) const {
    return macros.find(name) != macros.end();
}

void MacroManager::executeMacro(const std::string& name) {
    auto macro = getMacro(name);
    if (macro) {
        macro->execute();
    }
}

void MacroManager::stopMacro(const std::string& name) {
    auto macro = getMacro(name);
    if (macro) {
        macro->stop();
    }
}

void MacroManager::update(float deltaTime) {
    (void)deltaTime;
    // Stub implementation - would update executing macros
}

std::vector<std::string> MacroManager::getMacroNames() const {
    std::vector<std::string> names;
    names.reserve(macros.size());
    for (const auto& pair : macros) {
        names.push_back(pair.first);
    }
    return names;
}

// RecordingFormat implementation
bool RecordingFormat::saveJSON(const InputRecording& recording, const std::string& filename) {
    // Stub implementation
    (void)recording;
    (void)filename;
    return true;
}

bool RecordingFormat::loadJSON(InputRecording& recording, const std::string& filename) {
    // Stub implementation
    (void)recording;
    (void)filename;
    return true;
}

bool RecordingFormat::saveBinary(const InputRecording& recording, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;
    
    const auto& events = recording.getEvents();
    size_t count = events.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    for (const auto& event : events) {
        writeEvent(file, event);
    }
    
    return true;
}

bool RecordingFormat::loadBinary(InputRecording& recording, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    
    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    recording.clear();
    for (size_t i = 0; i < count; i++) {
        InputEvent event;
        if (readEvent(file, event)) {
            recording.addEvent(event);
        }
    }
    
    return true;
}

std::string RecordingFormat::eventToString(const InputEvent& event) {
    // Stub implementation
    (void)event;
    return "";
}

InputEvent RecordingFormat::stringToEvent(const std::string& str) {
    // Stub implementation
    (void)str;
    return InputEvent{};
}

void RecordingFormat::writeEvent(std::ofstream& file, const InputEvent& event) {
    file.write(reinterpret_cast<const char*>(&event), sizeof(InputEvent));
}

bool RecordingFormat::readEvent(std::ifstream& file, InputEvent& event) {
    file.read(reinterpret_cast<char*>(&event), sizeof(InputEvent));
    return file.good();
}

// InputAnalyzer implementation
InputAnalyzer::InputAnalyzer() {
    stats = InputStats{};
}

InputAnalyzer::~InputAnalyzer() {
}

void InputAnalyzer::analyze(const InputRecording& recording) {
    events = recording.getEvents();
    stats.totalEvents = events.size();
    stats.duration = recording.getDuration();
    stats.keyEvents = 0;
    stats.mouseEvents = 0;
    stats.gamepadEvents = 0;
    stats.touchEvents = 0;
    
    for (const auto& event : events) {
        switch (event.type) {
            case InputEventType::KeyPress:
            case InputEventType::KeyRelease:
                stats.keyEvents++;
                break;
            case InputEventType::MouseMove:
            case InputEventType::MouseButtonPress:
            case InputEventType::MouseButtonRelease:
            case InputEventType::MouseWheel:
                stats.mouseEvents++;
                break;
            case InputEventType::GamepadButton:
            case InputEventType::GamepadAxis:
                stats.gamepadEvents++;
                break;
            case InputEventType::TouchDown:
            case InputEventType::TouchUp:
            case InputEventType::TouchMove:
                stats.touchEvents++;
                break;
        }
    }
    
    stats.eventsPerSecond = stats.duration > 0 ?
        (static_cast<float>(stats.totalEvents) / stats.duration * 1000.0f) : 0.0f;
}

InputStats InputAnalyzer::getStats() const {
    return stats;
}

std::vector<InputEvent> InputAnalyzer::getEventsByType(InputEventType type) const {
    std::vector<InputEvent> filtered;
    for (const auto& event : events) {
        if (event.type == type) {
            filtered.push_back(event);
        }
    }
    return filtered;
}

float InputAnalyzer::getAverageEventsPerSecond() const {
    return stats.eventsPerSecond;
}

void InputAnalyzer::generateReport(std::ostream& out) {
    out << "Input Recording Analysis Report\n";
    out << "================================\n";
    out << "Total Events: " << stats.totalEvents << "\n";
    out << "Duration: " << stats.duration << " ms\n";
    out << "Key Events: " << stats.keyEvents << "\n";
    out << "Mouse Events: " << stats.mouseEvents << "\n";
    out << "Gamepad Events: " << stats.gamepadEvents << "\n";
    out << "Touch Events: " << stats.touchEvents << "\n";
    out << "Events/Second: " << stats.eventsPerSecond << "\n";
}

// InputComparison implementation
float InputComparison::compareRecordings(const InputRecording& recording1,
                                        const InputRecording& recording2) {
    const auto& events1 = recording1.getEvents();
    const auto& events2 = recording2.getEvents();
    
    if (events1.size() != events2.size()) {
        return 0.0f;
    }
    
    float totalSimilarity = 0.0f;
    for (size_t i = 0; i < events1.size(); i++) {
        totalSimilarity += compareEvents(events1[i], events2[i]);
    }
    
    return totalSimilarity / events1.size();
}

std::vector<size_t> InputComparison::findDifferences(const InputRecording& recording1,
                                                     const InputRecording& recording2) {
    std::vector<size_t> differences;
    const auto& events1 = recording1.getEvents();
    const auto& events2 = recording2.getEvents();
    
    size_t minSize = std::min(events1.size(), events2.size());
    for (size_t i = 0; i < minSize; i++) {
        if (compareEvents(events1[i], events2[i]) < 1.0f) {
            differences.push_back(i);
        }
    }
    
    return differences;
}

bool InputComparison::areIdentical(const InputRecording& recording1,
                                  const InputRecording& recording2) {
    return findDifferences(recording1, recording2).empty() &&
           recording1.getEventCount() == recording2.getEventCount();
}

float InputComparison::compareEvents(const InputEvent& event1, const InputEvent& event2) {
    if (event1.type != event2.type) return 0.0f;
    
    // Simple comparison - could be more sophisticated
    return (event1.timestamp == event2.timestamp &&
            event1.keyCode == event2.keyCode &&
            event1.mouseX == event2.mouseX &&
            event1.mouseY == event2.mouseY) ? 1.0f : 0.5f;
}

// RecordingCompression implementation
std::vector<InputEvent> RecordingCompression::compress(const std::vector<InputEvent>& events) {
    std::vector<InputEvent> compressed = events;
    mergeConsecutiveEvents(compressed);
    removeDuplicates(compressed);
    return compressed;
}

std::vector<InputEvent> RecordingCompression::decompress(const std::vector<InputEvent>& events) {
    // Stub implementation - compression is lossy in this simple version
    return events;
}

size_t RecordingCompression::estimateCompressedSize(const std::vector<InputEvent>& events) {
    return compress(events).size();
}

void RecordingCompression::mergeConsecutiveEvents(std::vector<InputEvent>& events) {
    // Stub implementation - would merge similar consecutive events
    (void)events;
}

void RecordingCompression::removeDuplicates(std::vector<InputEvent>& events) {
    // Stub implementation - would remove duplicate events
    (void)events;
}

// RecordingFilter implementation
RecordingFilter::RecordingFilter() : startTime(0), endTime(UINT32_MAX) {
}

RecordingFilter::~RecordingFilter() {
}

void RecordingFilter::setTypeFilter(InputEventType type, bool enabled) {
    typeFilters[type] = enabled;
}

bool RecordingFilter::isTypeEnabled(InputEventType type) const {
    auto it = typeFilters.find(type);
    return it != typeFilters.end() ? it->second : true;
}

void RecordingFilter::setTimeRange(uint32_t startTime, uint32_t endTime) {
    this->startTime = startTime;
    this->endTime = endTime;
}

InputRecording RecordingFilter::filter(const InputRecording& recording) {
    InputRecording filtered;
    
    const auto& events = recording.getEvents();
    for (const auto& event : events) {
        if (event.timestamp >= startTime && event.timestamp <= endTime &&
            isTypeEnabled(event.type)) {
            filtered.addEvent(event);
        }
    }
    
    return filtered;
}

} // namespace Input
} // namespace JJM
