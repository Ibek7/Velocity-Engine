#include "audio/StreamingAudio.h"
#include <algorithm>

namespace Engine {

// StreamingAudioPlayer Implementation
StreamingAudioPlayer::StreamingAudioPlayer()
    : m_state(StreamState::Idle)
    , m_volume(1.0f)
    , m_loop(false)
    , m_bufferSize(4096)
    , m_bufferCount(3)
    , m_fadeInDuration(0.0f)
    , m_fadeOutDuration(0.0f)
    , m_fadeTimer(0.0f)
    , m_fading(false)
    , m_audioSource(nullptr)
    , m_buffers(nullptr) {
}

StreamingAudioPlayer::~StreamingAudioPlayer() {
    stop();
    destroyBuffers();
}

bool StreamingAudioPlayer::load(const std::string& filename) {
    if (m_state == StreamState::Playing) {
        stop();
    }
    
    // In a real implementation, we'd detect format and create appropriate stream
    // For now, this is a stub
    m_state = StreamState::Idle;
    return true;
}

void StreamingAudioPlayer::play() {
    if (m_state == StreamState::Playing) {
        return;
    }
    
    if (m_state == StreamState::Paused) {
        m_state = StreamState::Playing;
        return;
    }
    
    if (!m_stream) {
        return;
    }
    
    initializeBuffers();
    m_state = StreamState::Playing;
    
    if (m_fadeInDuration > 0.0f) {
        m_fading = true;
        m_fadeTimer = 0.0f;
    }
}

void StreamingAudioPlayer::pause() {
    if (m_state == StreamState::Playing) {
        m_state = StreamState::Paused;
    }
}

void StreamingAudioPlayer::stop() {
    m_state = StreamState::Stopped;
    
    if (m_stream) {
        m_stream->reset();
    }
    
    m_fadeTimer = 0.0f;
    m_fading = false;
}

bool StreamingAudioPlayer::isPlaying() const {
    return m_state == StreamState::Playing;
}

bool StreamingAudioPlayer::isPaused() const {
    return m_state == StreamState::Paused;
}

void StreamingAudioPlayer::setVolume(float volume) {
    m_volume = std::max(0.0f, std::min(1.0f, volume));
}

void StreamingAudioPlayer::setLoop(bool loop) {
    m_loop = loop;
}

void StreamingAudioPlayer::seek(float seconds) {
    if (!m_stream) return;
    
    AudioStreamInfo info = m_stream->getInfo();
    long long samplePos = static_cast<long long>(seconds * info.sampleRate * info.channels);
    m_stream->seek(samplePos);
}

float StreamingAudioPlayer::getCurrentTime() const {
    if (!m_stream) return 0.0f;
    
    AudioStreamInfo info = m_stream->getInfo();
    long long pos = m_stream->tell();
    return static_cast<float>(pos) / (info.sampleRate * info.channels);
}

float StreamingAudioPlayer::getDuration() const {
    if (!m_stream) return 0.0f;
    
    return m_stream->getInfo().duration;
}

void StreamingAudioPlayer::setBufferSize(int sizeInSamples) {
    m_bufferSize = sizeInSamples;
}

void StreamingAudioPlayer::setBufferCount(int count) {
    m_bufferCount = std::max(2, std::min(count, 8));
}

void StreamingAudioPlayer::setCompletionCallback(std::function<void()> callback) {
    m_completionCallback = callback;
}

void StreamingAudioPlayer::setErrorCallback(std::function<void(const std::string&)> callback) {
    m_errorCallback = callback;
}

void StreamingAudioPlayer::update() {
    if (m_state != StreamState::Playing) {
        return;
    }
    
    processBuffers();
    applyFade();
    
    if (m_stream && m_stream->isEOF()) {
        if (m_loop) {
            m_stream->reset();
        } else {
            stop();
            if (m_completionCallback) {
                m_completionCallback();
            }
        }
    }
}

void StreamingAudioPlayer::setFadeIn(float duration) {
    m_fadeInDuration = duration;
}

void StreamingAudioPlayer::setFadeOut(float duration) {
    m_fadeOutDuration = duration;
}

void StreamingAudioPlayer::stopWithFade(float duration) {
    if (duration > 0.0f) {
        m_fadeOutDuration = duration;
        m_fading = true;
        m_fadeTimer = 0.0f;
    } else {
        stop();
    }
}

void StreamingAudioPlayer::initializeBuffers() {
    if (m_buffers) {
        destroyBuffers();
    }
    
    // In a real implementation, this would create OpenAL or other audio API buffers
    m_buffers = new unsigned int[m_bufferCount];
    
    // Fill initial buffers
    for (int i = 0; i < m_bufferCount; ++i) {
        fillBuffer(m_buffers[i]);
    }
}

void StreamingAudioPlayer::destroyBuffers() {
    if (m_buffers) {
        // In a real implementation, destroy audio API buffers
        delete[] m_buffers;
        m_buffers = nullptr;
    }
}

void StreamingAudioPlayer::fillBuffer(unsigned int buffer) {
    if (!m_stream) return;
    
    // In a real implementation:
    // 1. Allocate temp buffer
    // 2. Read samples from stream
    // 3. Upload to audio buffer
    // 4. Queue buffer for playback
}

void StreamingAudioPlayer::processBuffers() {
    // In a real implementation:
    // 1. Check which buffers have been processed
    // 2. Unqueue processed buffers
    // 3. Refill with new data
    // 4. Queue again
}

void StreamingAudioPlayer::applyFade() {
    if (!m_fading) return;
    
    // Simple linear fade implementation
    const float deltaTime = 1.0f / 60.0f;  // Assume 60 FPS for now
    m_fadeTimer += deltaTime;
    
    if (m_fadeInDuration > 0.0f && m_fadeTimer < m_fadeInDuration) {
        float fadeVolume = m_fadeTimer / m_fadeInDuration;
        setVolume(fadeVolume * m_volume);
    } else if (m_fadeOutDuration > 0.0f && m_fadeTimer < m_fadeOutDuration) {
        float fadeVolume = 1.0f - (m_fadeTimer / m_fadeOutDuration);
        setVolume(fadeVolume * m_volume);
        
        if (m_fadeTimer >= m_fadeOutDuration) {
            stop();
        }
    } else {
        m_fading = false;
    }
}

// StreamingAudioManager Implementation
StreamingAudioManager::StreamingAudioManager()
    : m_masterVolume(1.0f)
    , m_maxStreams(8) {
}

StreamingAudioManager::~StreamingAudioManager() {
    stopAll();
    m_players.clear();
}

StreamingAudioPlayer* StreamingAudioManager::createPlayer(const std::string& name) {
    // Check if player already exists
    for (auto& entry : m_players) {
        if (entry.name == name) {
            return entry.player.get();
        }
    }
    
    // Check stream limit
    if (getActiveStreamCount() >= m_maxStreams) {
        return nullptr;
    }
    
    PlayerEntry entry;
    entry.name = name;
    entry.player = std::make_unique<StreamingAudioPlayer>();
    
    StreamingAudioPlayer* player = entry.player.get();
    m_players.push_back(std::move(entry));
    
    return player;
}

StreamingAudioPlayer* StreamingAudioManager::getPlayer(const std::string& name) {
    for (auto& entry : m_players) {
        if (entry.name == name) {
            return entry.player.get();
        }
    }
    return nullptr;
}

void StreamingAudioManager::removePlayer(const std::string& name) {
    m_players.erase(
        std::remove_if(m_players.begin(), m_players.end(),
            [&name](const PlayerEntry& entry) {
                return entry.name == name;
            }),
        m_players.end()
    );
}

void StreamingAudioManager::updateAll() {
    for (auto& entry : m_players) {
        entry.player->update();
    }
}

void StreamingAudioManager::stopAll() {
    for (auto& entry : m_players) {
        entry.player->stop();
    }
}

void StreamingAudioManager::pauseAll() {
    for (auto& entry : m_players) {
        if (entry.player->isPlaying()) {
            entry.player->pause();
        }
    }
}

void StreamingAudioManager::resumeAll() {
    for (auto& entry : m_players) {
        if (entry.player->isPaused()) {
            entry.player->play();
        }
    }
}

void StreamingAudioManager::setMasterVolume(float volume) {
    m_masterVolume = std::max(0.0f, std::min(1.0f, volume));
    
    // Apply to all players
    for (auto& entry : m_players) {
        float playerVol = entry.player->getVolume();
        entry.player->setVolume(playerVol * m_masterVolume);
    }
}

int StreamingAudioManager::getActiveStreamCount() const {
    int count = 0;
    for (const auto& entry : m_players) {
        if (entry.player->isPlaying()) {
            ++count;
        }
    }
    return count;
}

void StreamingAudioManager::setMaxStreams(int maxStreams) {
    m_maxStreams = std::max(1, maxStreams);
}

} // namespace Engine
