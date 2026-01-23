#pragma once

#include <string>
#include <functional>
#include <memory>

/**
 * @file StreamingAudio.h
 * @brief Audio streaming system for large audio files
 * 
 * Provides efficient streaming of audio data from disk, supporting
 * various formats and compression schemes. Ideal for music, voice-over,
 * and ambient audio that's too large to fit in memory.
 */

namespace Engine {

enum class AudioFormat {
    WAV,
    MP3,
    OGG,
    FLAC,
    AAC
};

enum class StreamState {
    Idle,
    Loading,
    Playing,
    Paused,
    Stopped,
    Error
};

/**
 * @struct AudioStreamInfo
 * @brief Information about an audio stream
 */
struct AudioStreamInfo {
    AudioFormat format;
    int sampleRate;
    int channels;
    int bitsPerSample;
    long long totalSamples;
    float duration;
};

/**
 * @class AudioStream
 * @brief Base class for audio streaming
 */
class AudioStream {
public:
    virtual ~AudioStream() = default;
    
    /**
     * @brief Open an audio file for streaming
     * @param filename Path to audio file
     * @return True if successful
     */
    virtual bool open(const std::string& filename) = 0;
    
    /**
     * @brief Close the audio stream
     */
    virtual void close() = 0;
    
    /**
     * @brief Read audio samples into buffer
     * @param buffer Output buffer
     * @param numSamples Number of samples to read
     * @return Number of samples actually read
     */
    virtual int read(float* buffer, int numSamples) = 0;
    
    /**
     * @brief Seek to a specific position in the stream
     * @param samplePosition Sample position to seek to
     * @return True if successful
     */
    virtual bool seek(long long samplePosition) = 0;
    
    /**
     * @brief Get current playback position
     * @return Current sample position
     */
    virtual long long tell() const = 0;
    
    /**
     * @brief Get stream information
     * @return Stream info structure
     */
    virtual AudioStreamInfo getInfo() const = 0;
    
    /**
     * @brief Check if end of stream reached
     * @return True if at end
     */
    virtual bool isEOF() const = 0;
    
    /**
     * @brief Reset stream to beginning
     */
    virtual void reset() = 0;
};

/**
 * @class StreamingAudioPlayer
 * @brief Manages streaming audio playback
 */
class StreamingAudioPlayer {
public:
    StreamingAudioPlayer();
    ~StreamingAudioPlayer();
    
    /**
     * @brief Load an audio file for streaming
     * @param filename Path to audio file
     * @return True if successful
     */
    bool load(const std::string& filename);
    
    /**
     * @brief Start or resume playback
     */
    void play();
    
    /**
     * @brief Pause playback
     */
    void pause();
    
    /**
     * @brief Stop playback and reset position
     */
    void stop();
    
    /**
     * @brief Check if currently playing
     * @return True if playing
     */
    bool isPlaying() const;
    
    /**
     * @brief Check if paused
     * @return True if paused
     */
    bool isPaused() const;
    
    /**
     * @brief Set playback volume
     * @param volume Volume level (0.0 - 1.0)
     */
    void setVolume(float volume);
    
    /**
     * @brief Get current volume
     * @return Volume level
     */
    float getVolume() const { return m_volume; }
    
    /**
     * @brief Set looping behavior
     * @param loop True to enable looping
     */
    void setLoop(bool loop);
    
    /**
     * @brief Check if looping is enabled
     * @return True if looping
     */
    bool isLooping() const { return m_loop; }
    
    /**
     * @brief Seek to time position
     * @param seconds Time in seconds
     */
    void seek(float seconds);
    
    /**
     * @brief Get current playback time
     * @return Time in seconds
     */
    float getCurrentTime() const;
    
    /**
     * @brief Get total duration
     * @return Duration in seconds
     */
    float getDuration() const;
    
    /**
     * @brief Set buffer size for streaming
     * @param sizeInSamples Buffer size in samples
     */
    void setBufferSize(int sizeInSamples);
    
    /**
     * @brief Set number of streaming buffers
     * @param count Number of buffers (typically 2-4)
     */
    void setBufferCount(int count);
    
    /**
     * @brief Register callback for playback completion
     * @param callback Function to call when playback ends
     */
    void setCompletionCallback(std::function<void()> callback);
    
    /**
     * @brief Register callback for streaming errors
     * @param callback Function to call on error
     */
    void setErrorCallback(std::function<void(const std::string&)> callback);
    
    /**
     * @brief Get current stream state
     * @return Stream state
     */
    StreamState getState() const { return m_state; }
    
    /**
     * @brief Update streaming (call regularly from main thread)
     */
    void update();
    
    /**
     * @brief Set fade in duration
     * @param duration Fade duration in seconds
     */
    void setFadeIn(float duration);
    
    /**
     * @brief Set fade out duration
     * @param duration Fade duration in seconds
     */
    void setFadeOut(float duration);
    
    /**
     * @brief Stop with fade out
     * @param duration Fade out duration in seconds
     */
    void stopWithFade(float duration);

private:
    std::unique_ptr<AudioStream> m_stream;
    StreamState m_state;
    float m_volume;
    bool m_loop;
    int m_bufferSize;
    int m_bufferCount;
    
    float m_fadeInDuration;
    float m_fadeOutDuration;
    float m_fadeTimer;
    bool m_fading;
    
    std::function<void()> m_completionCallback;
    std::function<void(const std::string&)> m_errorCallback;
    
    void* m_audioSource;  // Platform-specific audio source
    unsigned int* m_buffers;  // Streaming buffers
    
    void initializeBuffers();
    void destroyBuffers();
    void fillBuffer(unsigned int buffer);
    void processBuffers();
    void applyFade();
};

/**
 * @class StreamingAudioManager
 * @brief Manages multiple streaming audio sources
 */
class StreamingAudioManager {
public:
    StreamingAudioManager();
    ~StreamingAudioManager();
    
    /**
     * @brief Create a new streaming player
     * @param name Unique identifier for the player
     * @return Pointer to new player
     */
    StreamingAudioPlayer* createPlayer(const std::string& name);
    
    /**
     * @brief Get player by name
     * @param name Player identifier
     * @return Pointer to player or nullptr
     */
    StreamingAudioPlayer* getPlayer(const std::string& name);
    
    /**
     * @brief Remove a player
     * @param name Player identifier
     */
    void removePlayer(const std::string& name);
    
    /**
     * @brief Update all streaming players
     */
    void updateAll();
    
    /**
     * @brief Stop all players
     */
    void stopAll();
    
    /**
     * @brief Pause all players
     */
    void pauseAll();
    
    /**
     * @brief Resume all paused players
     */
    void resumeAll();
    
    /**
     * @brief Set master volume for all players
     * @param volume Volume level (0.0 - 1.0)
     */
    void setMasterVolume(float volume);
    
    /**
     * @brief Get master volume
     * @return Volume level
     */
    float getMasterVolume() const { return m_masterVolume; }
    
    /**
     * @brief Get number of active streams
     * @return Active stream count
     */
    int getActiveStreamCount() const;
    
    /**
     * @brief Set maximum number of simultaneous streams
     * @param maxStreams Maximum stream count
     */
    void setMaxStreams(int maxStreams);

private:
    struct PlayerEntry {
        std::string name;
        std::unique_ptr<StreamingAudioPlayer> player;
    };
    
    std::vector<PlayerEntry> m_players;
    float m_masterVolume;
    int m_maxStreams;
};

} // namespace Engine
