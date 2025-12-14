#include "core/ReplaySystem.h"
#include <algorithm>
#include <cstring>
#include <chrono>

namespace JJM {
namespace Core {

// ReplayEvent implementation
ReplayEvent::ReplayEvent(float timestamp) : timestamp(timestamp) {}
ReplayEvent::~ReplayEvent() {}

float ReplayEvent::getTimestamp() const { return timestamp; }
void ReplayEvent::setTimestamp(float ts) { timestamp = ts; }

// InputEvent implementation
InputEvent::InputEvent(float timestamp)
    : ReplayEvent(timestamp), type(Type::KeyDown), keyCode(0),
      button(0), mouseX(0), mouseY(0), wheelDelta(0) {}

InputEvent::~InputEvent() {}

void InputEvent::execute() {
    // Stub - would trigger input system
}

void InputEvent::serialize(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
    out.write(reinterpret_cast<const char*>(&type), sizeof(type));
    out.write(reinterpret_cast<const char*>(&keyCode), sizeof(keyCode));
    out.write(reinterpret_cast<const char*>(&button), sizeof(button));
    out.write(reinterpret_cast<const char*>(&mouseX), sizeof(mouseX));
    out.write(reinterpret_cast<const char*>(&mouseY), sizeof(mouseY));
    out.write(reinterpret_cast<const char*>(&wheelDelta), sizeof(wheelDelta));
}

void InputEvent::deserialize(std::istream& in) {
    in.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
    in.read(reinterpret_cast<char*>(&type), sizeof(type));
    in.read(reinterpret_cast<char*>(&keyCode), sizeof(keyCode));
    in.read(reinterpret_cast<char*>(&button), sizeof(button));
    in.read(reinterpret_cast<char*>(&mouseX), sizeof(mouseX));
    in.read(reinterpret_cast<char*>(&mouseY), sizeof(mouseY));
    in.read(reinterpret_cast<char*>(&wheelDelta), sizeof(wheelDelta));
}

void InputEvent::setKeyEvent(Type t, int code) {
    type = t;
    keyCode = code;
}

void InputEvent::setMouseEvent(Type t, int btn, float x, float y) {
    type = t;
    button = btn;
    mouseX = x;
    mouseY = y;
}

void InputEvent::setMouseWheelEvent(float delta) {
    type = Type::MouseWheel;
    wheelDelta = delta;
}

// StateEvent implementation
StateEvent::StateEvent(float timestamp) : ReplayEvent(timestamp) {}
StateEvent::~StateEvent() {}

void StateEvent::execute() {
    // Stub - would restore game state
}

void StateEvent::serialize(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
    size_t dataSize = stateData.size();
    out.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
    out.write(reinterpret_cast<const char*>(stateData.data()), dataSize);
}

void StateEvent::deserialize(std::istream& in) {
    in.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
    size_t dataSize;
    in.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
    stateData.resize(dataSize);
    in.read(reinterpret_cast<char*>(stateData.data()), dataSize);
}

void StateEvent::setStateData(const std::vector<uint8_t>& data) {
    stateData = data;
}

const std::vector<uint8_t>& StateEvent::getStateData() const {
    return stateData;
}

// CommandEvent implementation
CommandEvent::CommandEvent(float timestamp) : ReplayEvent(timestamp) {}
CommandEvent::~CommandEvent() {}

void CommandEvent::execute() {
    // Stub - would execute command
}

void CommandEvent::serialize(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
    
    size_t cmdSize = command.size();
    out.write(reinterpret_cast<const char*>(&cmdSize), sizeof(cmdSize));
    out.write(command.data(), cmdSize);
    
    size_t paramCount = parameters.size();
    out.write(reinterpret_cast<const char*>(&paramCount), sizeof(paramCount));
    for (const auto& param : parameters) {
        size_t paramSize = param.size();
        out.write(reinterpret_cast<const char*>(&paramSize), sizeof(paramSize));
        out.write(param.data(), paramSize);
    }
}

void CommandEvent::deserialize(std::istream& in) {
    in.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
    
    size_t cmdSize;
    in.read(reinterpret_cast<char*>(&cmdSize), sizeof(cmdSize));
    command.resize(cmdSize);
    in.read(&command[0], cmdSize);
    
    size_t paramCount;
    in.read(reinterpret_cast<char*>(&paramCount), sizeof(paramCount));
    parameters.clear();
    for (size_t i = 0; i < paramCount; ++i) {
        size_t paramSize;
        in.read(reinterpret_cast<char*>(&paramSize), sizeof(paramSize));
        std::string param(paramSize, '\0');
        in.read(&param[0], paramSize);
        parameters.push_back(param);
    }
}

void CommandEvent::setCommand(const std::string& cmd) {
    command = cmd;
}

void CommandEvent::setParameters(const std::vector<std::string>& params) {
    parameters = params;
}

// ReplayRecorder implementation
ReplayRecorder::ReplayRecorder()
    : recording(false), paused(false), recordingTime(0.0f), pauseStartTime(0.0f) {}

ReplayRecorder::~ReplayRecorder() {}

void ReplayRecorder::startRecording() {
    recording = true;
    paused = false;
    recordingTime = 0.0f;
    events.clear();
}

void ReplayRecorder::stopRecording() {
    recording = false;
    paused = false;
}

void ReplayRecorder::pauseRecording() {
    if (recording && !paused) {
        paused = true;
        pauseStartTime = recordingTime;
    }
}

void ReplayRecorder::resumeRecording() {
    if (recording && paused) {
        paused = false;
    }
}

bool ReplayRecorder::isRecording() const { return recording; }
bool ReplayRecorder::isPaused() const { return paused; }

void ReplayRecorder::recordEvent(std::unique_ptr<ReplayEvent> event) {
    if (recording && !paused) {
        event->setTimestamp(recordingTime);
        events.push_back(std::move(event));
    }
}

void ReplayRecorder::recordInput(InputEvent::Type type, int keyCode) {
    auto event = std::make_unique<InputEvent>(recordingTime);
    event->setKeyEvent(type, keyCode);
    recordEvent(std::move(event));
}

void ReplayRecorder::recordMouseMove(float x, float y) {
    auto event = std::make_unique<InputEvent>(recordingTime);
    event->setMouseEvent(InputEvent::Type::MouseMove, 0, x, y);
    recordEvent(std::move(event));
}

void ReplayRecorder::recordMouseButton(InputEvent::Type type, int button, float x, float y) {
    auto event = std::make_unique<InputEvent>(recordingTime);
    event->setMouseEvent(type, button, x, y);
    recordEvent(std::move(event));
}

void ReplayRecorder::recordCheckpoint() {
    auto event = std::make_unique<StateEvent>(recordingTime);
    // Would capture game state here
    recordEvent(std::move(event));
}

void ReplayRecorder::saveToFile(const std::string& filePath) {
    ReplayMetadata metadata;
    metadata.setDuration(recordingTime);
    metadata.setEventCount(events.size());
    ReplayFileHandler::save(filePath, metadata, events);
}

void ReplayRecorder::clearRecording() {
    events.clear();
    recordingTime = 0.0f;
}

float ReplayRecorder::getRecordingTime() const { return recordingTime; }
size_t ReplayRecorder::getEventCount() const { return events.size(); }

// ReplayPlayer implementation
ReplayPlayer::ReplayPlayer()
    : currentEventIndex(0), currentTime(0.0f), playbackSpeed(1.0f),
      playing(false), paused(false) {}

ReplayPlayer::~ReplayPlayer() {}

void ReplayPlayer::loadFromFile(const std::string& filePath) {
    ReplayMetadata metadata;
    events.clear();
    ReplayFileHandler::load(filePath, metadata, events);
    currentEventIndex = 0;
    currentTime = 0.0f;
}

void ReplayPlayer::startPlayback() {
    playing = true;
    paused = false;
    currentEventIndex = 0;
    currentTime = 0.0f;
}

void ReplayPlayer::stopPlayback() {
    playing = false;
    paused = false;
}

void ReplayPlayer::pausePlayback() {
    if (playing) paused = true;
}

void ReplayPlayer::resumePlayback() {
    if (playing) paused = false;
}

void ReplayPlayer::update(float deltaTime) {
    if (!playing || paused) return;
    
    currentTime += deltaTime * playbackSpeed;
    executeCurrentEvents();
}

bool ReplayPlayer::isPlaying() const { return playing; }
bool ReplayPlayer::isPaused() const { return paused; }
bool ReplayPlayer::isComplete() const {
    return currentEventIndex >= events.size();
}

void ReplayPlayer::setPlaybackSpeed(float speed) {
    playbackSpeed = std::max(0.1f, speed);
}

float ReplayPlayer::getPlaybackSpeed() const { return playbackSpeed; }

void ReplayPlayer::seekToTime(float time) {
    currentTime = time;
    currentEventIndex = 0;
    for (size_t i = 0; i < events.size(); ++i) {
        if (events[i]->getTimestamp() > currentTime) {
            currentEventIndex = i;
            break;
        }
    }
}

void ReplayPlayer::seekToEvent(size_t index) {
    if (index < events.size()) {
        currentEventIndex = index;
        currentTime = events[index]->getTimestamp();
    }
}

float ReplayPlayer::getCurrentTime() const { return currentTime; }

float ReplayPlayer::getTotalTime() const {
    return events.empty() ? 0.0f : events.back()->getTimestamp();
}

size_t ReplayPlayer::getCurrentEventIndex() const { return currentEventIndex; }
size_t ReplayPlayer::getTotalEventCount() const { return events.size(); }

void ReplayPlayer::setOnEventExecuted(std::function<void(const ReplayEvent*)> callback) {
    onEventExecuted = callback;
}

void ReplayPlayer::executeCurrentEvents() {
    while (currentEventIndex < events.size() &&
           events[currentEventIndex]->getTimestamp() <= currentTime) {
        events[currentEventIndex]->execute();
        if (onEventExecuted) {
            onEventExecuted(events[currentEventIndex].get());
        }
        ++currentEventIndex;
    }
}

// ReplaySystem implementation
ReplaySystem::ReplaySystem() {}
ReplaySystem::~ReplaySystem() {}

void ReplaySystem::update(float deltaTime) {
    if (recorder.isRecording()) {
        // Update recording time
    }
    player.update(deltaTime);
}

ReplayRecorder& ReplaySystem::getRecorder() { return recorder; }
ReplayPlayer& ReplaySystem::getPlayer() { return player; }

void ReplaySystem::startRecording() { recorder.startRecording(); }
void ReplaySystem::stopRecording() { recorder.stopRecording(); }
void ReplaySystem::startPlayback() { player.startPlayback(); }
void ReplaySystem::stopPlayback() { player.stopPlayback(); }

bool ReplaySystem::isRecording() const { return recorder.isRecording(); }
bool ReplaySystem::isPlaying() const { return player.isPlaying(); }

void ReplaySystem::saveReplay(const std::string& filePath) {
    recorder.saveToFile(filePath);
}

void ReplaySystem::loadReplay(const std::string& filePath) {
    player.loadFromFile(filePath);
}

// ReplayMetadata implementation
ReplayMetadata::ReplayMetadata()
    : timestamp(0), duration(0.0f), eventCount(0) {}

ReplayMetadata::~ReplayMetadata() {}

void ReplayMetadata::setTitle(const std::string& t) { title = t; }
void ReplayMetadata::setDescription(const std::string& d) { description = d; }
void ReplayMetadata::setTimestamp(long long ts) { timestamp = ts; }
void ReplayMetadata::setDuration(float d) { duration = d; }
void ReplayMetadata::setEventCount(size_t count) { eventCount = count; }

std::string ReplayMetadata::getTitle() const { return title; }
std::string ReplayMetadata::getDescription() const { return description; }
long long ReplayMetadata::getTimestamp() const { return timestamp; }
float ReplayMetadata::getDuration() const { return duration; }
size_t ReplayMetadata::getEventCount() const { return eventCount; }

void ReplayMetadata::serialize(std::ostream& out) const {
    size_t titleSize = title.size();
    out.write(reinterpret_cast<const char*>(&titleSize), sizeof(titleSize));
    out.write(title.data(), titleSize);
    
    size_t descSize = description.size();
    out.write(reinterpret_cast<const char*>(&descSize), sizeof(descSize));
    out.write(description.data(), descSize);
    
    out.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
    out.write(reinterpret_cast<const char*>(&duration), sizeof(duration));
    out.write(reinterpret_cast<const char*>(&eventCount), sizeof(eventCount));
}

void ReplayMetadata::deserialize(std::istream& in) {
    size_t titleSize;
    in.read(reinterpret_cast<char*>(&titleSize), sizeof(titleSize));
    title.resize(titleSize);
    in.read(&title[0], titleSize);
    
    size_t descSize;
    in.read(reinterpret_cast<char*>(&descSize), sizeof(descSize));
    description.resize(descSize);
    in.read(&description[0], descSize);
    
    in.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
    in.read(reinterpret_cast<char*>(&duration), sizeof(duration));
    in.read(reinterpret_cast<char*>(&eventCount), sizeof(eventCount));
}

// ReplayFileHandler implementation
bool ReplayFileHandler::save(const std::string& filePath,
                             const ReplayMetadata& metadata,
                             const std::vector<std::unique_ptr<ReplayEvent>>& events) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;
    
    writeHeader(file);
    metadata.serialize(file);
    
    size_t eventCount = events.size();
    file.write(reinterpret_cast<const char*>(&eventCount), sizeof(eventCount));
    
    for (const auto& event : events) {
        writeEvent(file, *event);
    }
    
    return true;
}

bool ReplayFileHandler::load(const std::string& filePath,
                             ReplayMetadata& metadata,
                             std::vector<std::unique_ptr<ReplayEvent>>& events) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;
    
    if (!readHeader(file)) return false;
    metadata.deserialize(file);
    
    size_t eventCount;
    file.read(reinterpret_cast<char*>(&eventCount), sizeof(eventCount));
    
    events.clear();
    for (size_t i = 0; i < eventCount; ++i) {
        auto event = readEvent(file);
        if (event) events.push_back(std::move(event));
    }
    
    return true;
}

void ReplayFileHandler::writeHeader(std::ostream& out) {
    const char* magic = "JJMR";
    out.write(magic, 4);
    uint32_t version = 1;
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
}

bool ReplayFileHandler::readHeader(std::istream& in) {
    char magic[4];
    in.read(magic, 4);
    if (std::memcmp(magic, "JJMR", 4) != 0) return false;
    
    uint32_t version;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    return version == 1;
}

void ReplayFileHandler::writeEvent(std::ostream& out, const ReplayEvent& event) {
    event.serialize(out);
}

std::unique_ptr<ReplayEvent> ReplayFileHandler::readEvent(std::istream& in) {
    // Simplified - would determine event type and create appropriate event
    auto event = std::make_unique<InputEvent>(0.0f);
    event->deserialize(in);
    return event;
}

// ReplayCompressor implementation
std::vector<uint8_t> ReplayCompressor::compress(const std::vector<uint8_t>& data) {
    return rleCompress(data);
}

std::vector<uint8_t> ReplayCompressor::decompress(const std::vector<uint8_t>& data) {
    return rleDecompress(data);
}

std::vector<uint8_t> ReplayCompressor::rleCompress(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> compressed;
    if (data.empty()) return compressed;
    
    uint8_t current = data[0];
    uint8_t count = 1;
    
    for (size_t i = 1; i < data.size(); ++i) {
        if (data[i] == current && count < 255) {
            ++count;
        } else {
            compressed.push_back(count);
            compressed.push_back(current);
            current = data[i];
            count = 1;
        }
    }
    
    compressed.push_back(count);
    compressed.push_back(current);
    return compressed;
}

std::vector<uint8_t> ReplayCompressor::rleDecompress(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> decompressed;
    
    for (size_t i = 0; i < data.size(); i += 2) {
        uint8_t count = data[i];
        uint8_t value = data[i + 1];
        for (uint8_t j = 0; j < count; ++j) {
            decompressed.push_back(value);
        }
    }
    
    return decompressed;
}

// ReplayAnalyzer implementation
ReplayAnalyzer::ReplayAnalyzer()
    : totalEvents(0), inputEvents(0), stateEvents(0),
      averageInterval(0.0f), totalDuration(0.0f) {}

ReplayAnalyzer::~ReplayAnalyzer() {}

void ReplayAnalyzer::analyze(const std::vector<std::unique_ptr<ReplayEvent>>& events) {
    totalEvents = events.size();
    inputEvents = 0;
    stateEvents = 0;
    
    if (events.empty()) {
        totalDuration = 0.0f;
        averageInterval = 0.0f;
        return;
    }
    
    for (const auto& event : events) {
        if (dynamic_cast<InputEvent*>(event.get())) ++inputEvents;
        if (dynamic_cast<StateEvent*>(event.get())) ++stateEvents;
    }
    
    totalDuration = events.back()->getTimestamp();
    averageInterval = totalDuration / totalEvents;
}

size_t ReplayAnalyzer::getTotalEvents() const { return totalEvents; }
size_t ReplayAnalyzer::getInputEventCount() const { return inputEvents; }
size_t ReplayAnalyzer::getStateEventCount() const { return stateEvents; }
float ReplayAnalyzer::getAverageEventInterval() const { return averageInterval; }
float ReplayAnalyzer::getTotalDuration() const { return totalDuration; }

// ReplayComparator implementation
std::vector<ReplayComparator::Difference> ReplayComparator::compare(
    const std::vector<std::unique_ptr<ReplayEvent>>& replay1,
    const std::vector<std::unique_ptr<ReplayEvent>>& replay2) {
    
    std::vector<Difference> differences;
    
    size_t minSize = std::min(replay1.size(), replay2.size());
    for (size_t i = 0; i < minSize; ++i) {
        if (replay1[i]->getTimestamp() != replay2[i]->getTimestamp()) {
            Difference diff;
            diff.timestamp = replay1[i]->getTimestamp();
            diff.description = "Timestamp mismatch";
            differences.push_back(diff);
        }
    }
    
    if (replay1.size() != replay2.size()) {
        Difference diff;
        diff.timestamp = 0.0f;
        diff.description = "Different event counts";
        differences.push_back(diff);
    }
    
    return differences;
}

} // namespace Core
} // namespace JJM
