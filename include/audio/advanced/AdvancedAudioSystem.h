#ifndef ADVANCED_AUDIO_SYSTEM_H
#define ADVANCED_AUDIO_SYSTEM_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <array>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include "math/Vector2D.h"

namespace JJM {
namespace Audio {
namespace Advanced {

// Forward declarations
class AudioSource;
class AudioListener;
class AudioEffect;
class AudioBus;

// 3D Audio structures
struct Vector3D {
    float x, y, z;
    
    Vector3D() : x(0), y(0), z(0) {}
    Vector3D(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vector3D operator+(const Vector3D& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vector3D operator-(const Vector3D& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vector3D operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    
    float magnitude() const;
    Vector3D normalized() const;
    float dot(const Vector3D& other) const;
    Vector3D cross(const Vector3D& other) const;
    float distance(const Vector3D& other) const;
};

// Audio channel configurations
enum class AudioChannelConfig {
    Mono = 1,
    Stereo = 2,
    Surround5_1 = 6,
    Surround7_1 = 8,
    Ambisonic = 4  // First-order Ambisonic (4 channels: W, X, Y, Z)
};

// 3D Audio algorithms
enum class SpatializerType {
    Simple,         // Basic panning
    HRTF,          // Head-Related Transfer Function
    Ambisonics,    // Ambisonic encoding/decoding
    Binaural       // Binaural rendering
};

// Audio effect types
enum class AudioEffectType {
    Reverb,
    Echo,
    Chorus,
    Flanger,
    Distortion,
    Compressor,
    EQ,
    Filter,
    Pitch,
    Convolution,
    Custom
};

// Audio buffer format
struct AudioBuffer {
    float* samples;
    size_t frameCount;
    size_t channelCount;
    size_t sampleRate;
    
    AudioBuffer() : samples(nullptr), frameCount(0), channelCount(0), sampleRate(0) {}
    AudioBuffer(size_t frames, size_t channels, size_t rate);
    ~AudioBuffer();
    
    void allocate(size_t frames, size_t channels, size_t rate);
    void deallocate();
    void clear();
    void copyFrom(const AudioBuffer& other);
    void mixWith(const AudioBuffer& other, float gain = 1.0f);
    
    float* getChannelData(size_t channel);
    const float* getChannelData(size_t channel) const;
    size_t getSizeInBytes() const { return frameCount * channelCount * sizeof(float); }
};

// 3D Audio Listener (represents the "ears" in the 3D scene)
class AudioListener {
private:
    Vector3D position;
    Vector3D forward;
    Vector3D up;
    Vector3D right;
    Vector3D velocity;
    
    float gainMultiplier;
    float dopplerFactor;
    
public:
    AudioListener();
    
    // Position and orientation
    void setPosition(const Vector3D& pos) { position = pos; }
    void setOrientation(const Vector3D& forward, const Vector3D& up);
    void setVelocity(const Vector3D& vel) { velocity = vel; }
    
    const Vector3D& getPosition() const { return position; }
    const Vector3D& getForward() const { return forward; }
    const Vector3D& getUp() const { return up; }
    const Vector3D& getRight() const { return right; }
    const Vector3D& getVelocity() const { return velocity; }
    
    // Audio properties
    void setGain(float gain) { gainMultiplier = gain; }
    float getGain() const { return gainMultiplier; }
    
    void setDopplerFactor(float factor) { dopplerFactor = factor; }
    float getDopplerFactor() const { return dopplerFactor; }
    
    // Transform world coordinates to listener space
    Vector3D worldToListener(const Vector3D& worldPos) const;
};

// 3D Audio Source
class AudioSource {
public:
    enum class AttenuationModel {
        None,
        Linear,
        Inverse,
        Exponential,
        Custom
    };
    
    enum class DirectivityModel {
        Omnidirectional,
        Cardioid,
        Bidirectional,
        Custom
    };

private:
    Vector3D position;
    Vector3D velocity;
    Vector3D direction;
    
    float gain;
    float pitch;
    float referenceDistance;
    float maxDistance;
    float rolloffFactor;
    
    AttenuationModel attenuationModel;
    DirectivityModel directivityModel;
    std::function<float(float)> customAttenuationFunc;
    std::function<float(float)> customDirectivityFunc;
    
    bool looping;
    bool playing;
    bool spatialized;
    
    std::string audioClipId;
    double playbackPosition;  // in seconds
    
public:
    AudioSource();
    
    // Spatial properties
    void setPosition(const Vector3D& pos) { position = pos; }
    void setVelocity(const Vector3D& vel) { velocity = vel; }
    void setDirection(const Vector3D& dir) { direction = dir.normalized(); }
    
    const Vector3D& getPosition() const { return position; }
    const Vector3D& getVelocity() const { return velocity; }
    const Vector3D& getDirection() const { return direction; }
    
    // Audio properties
    void setGain(float g) { gain = g; }
    void setPitch(float p) { pitch = p; }
    void setReferenceDistance(float dist) { referenceDistance = dist; }
    void setMaxDistance(float dist) { maxDistance = dist; }
    void setRolloffFactor(float factor) { rolloffFactor = factor; }
    
    float getGain() const { return gain; }
    float getPitch() const { return pitch; }
    float getReferenceDistance() const { return referenceDistance; }
    float getMaxDistance() const { return maxDistance; }
    float getRolloffFactor() const { return rolloffFactor; }
    
    // Attenuation and directivity
    void setAttenuationModel(AttenuationModel model) { attenuationModel = model; }
    void setDirectivityModel(DirectivityModel model) { directivityModel = model; }
    void setCustomAttenuationFunc(std::function<float(float)> func) { customAttenuationFunc = func; }
    void setCustomDirectivityFunc(std::function<float(float)> func) { customDirectivityFunc = func; }
    
    // Playback control
    void play(const std::string& clipId);
    void pause();
    void stop();
    void setLooping(bool loop) { looping = loop; }
    void setSpatialized(bool spatial) { spatialized = spatial; }
    
    bool isPlaying() const { return playing; }
    bool isLooping() const { return looping; }
    bool isSpatialized() const { return spatialized; }
    
    // Calculate attenuation and directivity
    float calculateAttenuation(float distance) const;
    float calculateDirectivity(const Vector3D& listenerPos) const;
};

// Audio Effect base class
class AudioEffect {
protected:
    bool enabled;
    float wetLevel;  // 0.0 = dry, 1.0 = wet
    float dryLevel;  // 0.0 = no dry signal, 1.0 = full dry
    
public:
    AudioEffect() : enabled(true), wetLevel(1.0f), dryLevel(0.0f) {}
    virtual ~AudioEffect() = default;
    
    virtual void process(AudioBuffer& buffer) = 0;
    virtual void reset() {}
    virtual AudioEffectType getType() const = 0;
    
    void setEnabled(bool enable) { enabled = enable; }
    bool isEnabled() const { return enabled; }
    
    void setWetLevel(float level) { wetLevel = std::clamp(level, 0.0f, 1.0f); }
    void setDryLevel(float level) { dryLevel = std::clamp(level, 0.0f, 1.0f); }
    
    float getWetLevel() const { return wetLevel; }
    float getDryLevel() const { return dryLevel; }
};

// Reverb Effect
class ReverbEffect : public AudioEffect {
public:
    struct ReverbParameters {
        float roomSize;        // 0.0 - 1.0
        float damping;         // 0.0 - 1.0
        float earlyReflections; // 0.0 - 1.0
        float lateDiffusion;   // 0.0 - 1.0
        float preDelay;        // milliseconds
        float decayTime;       // seconds
        float highFreqDecay;   // multiplier for high frequencies
        float lowFreqDecay;    // multiplier for low frequencies
    };

private:
    ReverbParameters params;
    std::vector<float> delayLines;
    std::vector<size_t> delayIndices;
    std::vector<float> allpassDelays;
    std::vector<size_t> allpassIndices;
    std::vector<float> combGains;
    size_t sampleRate;
    
public:
    ReverbEffect(size_t sampleRate = 44100);
    
    void process(AudioBuffer& buffer) override;
    void reset() override;
    AudioEffectType getType() const override { return AudioEffectType::Reverb; }
    
    void setParameters(const ReverbParameters& p);
    const ReverbParameters& getParameters() const { return params; }
    
    // Preset configurations
    void setPreset(const std::string& presetName);
    static ReverbParameters getPresetParameters(const std::string& presetName);
};

// Echo Effect
class EchoEffect : public AudioEffect {
private:
    std::vector<float> delayBuffer;
    size_t delayBufferSize;
    size_t writeIndex;
    float delayTime;     // in seconds
    float feedback;      // 0.0 - 1.0
    size_t sampleRate;
    
public:
    EchoEffect(size_t sampleRate = 44100);
    
    void process(AudioBuffer& buffer) override;
    void reset() override;
    AudioEffectType getType() const override { return AudioEffectType::Echo; }
    
    void setDelayTime(float timeSeconds);
    void setFeedback(float fb) { feedback = std::clamp(fb, 0.0f, 0.99f); }
    
    float getDelayTime() const { return delayTime; }
    float getFeedback() const { return feedback; }
};

// Equalizer Effect
class EqualizerEffect : public AudioEffect {
public:
    struct EQBand {
        float frequency;    // Center frequency in Hz
        float gain;        // Gain in dB (-20 to +20 typical)
        float Q;          // Quality factor (bandwidth)
        bool enabled;
        
        EQBand() : frequency(1000), gain(0), Q(1.0f), enabled(true) {}
        EQBand(float freq, float g, float q) : frequency(freq), gain(g), Q(q), enabled(true) {}
    };

private:
    std::vector<EQBand> bands;
    std::vector<std::array<float, 3>> filterStates; // Biquad filter states
    size_t sampleRate;
    
    void updateFilterCoefficients(size_t bandIndex);
    float processBiquad(float input, size_t bandIndex);
    
public:
    EqualizerEffect(size_t sampleRate = 44100);
    
    void process(AudioBuffer& buffer) override;
    void reset() override;
    AudioEffectType getType() const override { return AudioEffectType::EQ; }
    
    void addBand(const EQBand& band);
    void removeBand(size_t index);
    void setBandGain(size_t index, float gainDB);
    void setBandFrequency(size_t index, float frequency);
    void setBandQ(size_t index, float Q);
    void enableBand(size_t index, bool enabled);
    
    const std::vector<EQBand>& getBands() const { return bands; }
    size_t getBandCount() const { return bands.size(); }
};

// Audio Bus (for grouping and processing multiple sources)
class AudioBus {
private:
    std::string name;
    float gain;
    bool muted;
    bool soloed;
    
    std::vector<std::unique_ptr<AudioEffect>> effects;
    std::vector<AudioSource*> sources;
    AudioBus* parentBus;
    std::vector<std::unique_ptr<AudioBus>> childBuses;
    
    AudioBuffer mixBuffer;
    
public:
    AudioBus(const std::string& name, size_t bufferSize = 1024, size_t channels = 2, size_t sampleRate = 44100);
    ~AudioBus();
    
    // Bus properties
    void setGain(float g) { gain = g; }
    void setMuted(bool m) { muted = m; }
    void setSoloed(bool s) { soloed = s; }
    
    float getGain() const { return gain; }
    bool isMuted() const { return muted; }
    bool isSoloed() const { return soloed; }
    const std::string& getName() const { return name; }
    
    // Source management
    void addSource(AudioSource* source);
    void removeSource(AudioSource* source);
    const std::vector<AudioSource*>& getSources() const { return sources; }
    
    // Effect chain
    void addEffect(std::unique_ptr<AudioEffect> effect);
    void removeEffect(size_t index);
    void clearEffects();
    AudioEffect* getEffect(size_t index);
    size_t getEffectCount() const { return effects.size(); }
    
    // Hierarchical bus structure
    void addChildBus(std::unique_ptr<AudioBus> childBus);
    void removeChildBus(const std::string& name);
    AudioBus* getChildBus(const std::string& name);
    const std::vector<std::unique_ptr<AudioBus>>& getChildBuses() const { return childBuses; }
    
    // Audio processing
    void process(AudioBuffer& outputBuffer, const AudioListener& listener);
    void reset();
};

// 3D Audio Spatializer
class AudioSpatializer {
public:
    struct HRTFData {
        std::vector<std::vector<float>> leftImpulseResponses;
        std::vector<std::vector<float>> rightImpulseResponses;
        std::vector<float> azimuthAngles;
        std::vector<float> elevationAngles;
        size_t impulseLength;
        size_t sampleRate;
    };
    
    struct SpatializationResult {
        float leftGain;
        float rightGain;
        float delay;           // in samples
        float dopplerShift;    // frequency multiplier
    };

private:
    SpatializerType type;
    std::unique_ptr<HRTFData> hrtfData;
    size_t sampleRate;
    float speedOfSound;  // meters per second
    
public:
    AudioSpatializer(SpatializerType type, size_t sampleRate = 44100);
    ~AudioSpatializer();
    
    // Main spatialization function
    SpatializationResult spatialize(const AudioSource& source, const AudioListener& listener);
    
    // HRTF management
    bool loadHRTFData(const std::string& hrtfFilePath);
    void setCustomHRTFData(std::unique_ptr<HRTFData> data);
    
    // Settings
    void setType(SpatializerType newType) { type = newType; }
    SpatializerType getType() const { return type; }
    
    void setSpeedOfSound(float speed) { speedOfSound = speed; }
    float getSpeedOfSound() const { return speedOfSound; }

private:
    SpatializationResult spatializeSimple(const AudioSource& source, const AudioListener& listener);
    SpatializationResult spatializeHRTF(const AudioSource& source, const AudioListener& listener);
    SpatializationResult spatializeAmbisonics(const AudioSource& source, const AudioListener& listener);
    SpatializationResult spatializeBinaural(const AudioSource& source, const AudioListener& listener);
    
    float calculateDopplerShift(const AudioSource& source, const AudioListener& listener);
};

// Advanced Audio Engine
class AdvancedAudioEngine {
private:
    static AdvancedAudioEngine* instance;
    
    bool initialized;
    size_t sampleRate;
    size_t bufferSize;
    AudioChannelConfig outputConfig;
    
    std::unique_ptr<AudioListener> listener;
    std::unique_ptr<AudioSpatializer> spatializer;
    std::unique_ptr<AudioBus> masterBus;
    
    std::vector<std::unique_ptr<AudioSource>> sources;
    std::unordered_map<std::string, std::unique_ptr<AudioBuffer>> audioClips;
    
    // Processing thread
    std::thread processingThread;
    std::atomic<bool> shouldStop;
    std::mutex audioMutex;
    std::condition_variable processingCV;
    
    // Performance monitoring
    std::atomic<float> cpuUsage;
    std::atomic<size_t> voiceCount;
    
public:
    static AdvancedAudioEngine* getInstance();
    ~AdvancedAudioEngine();
    
    // Initialization
    bool initialize(size_t sampleRate = 44100, size_t bufferSize = 1024, 
                   AudioChannelConfig config = AudioChannelConfig::Stereo);
    void shutdown();
    
    // Audio listener
    AudioListener* getListener() { return listener.get(); }
    
    // Audio source management
    AudioSource* createSource();
    void destroySource(AudioSource* source);
    std::vector<AudioSource*> getAllSources();
    
    // Audio clip management
    bool loadAudioClip(const std::string& id, const std::string& filePath);
    bool addAudioClip(const std::string& id, std::unique_ptr<AudioBuffer> buffer);
    void unloadAudioClip(const std::string& id);
    AudioBuffer* getAudioClip(const std::string& id);
    
    // Spatialization
    void setSpatializationType(SpatializerType type);
    SpatializerType getSpatializationType() const;
    bool loadHRTFData(const std::string& hrtfFilePath);
    
    // Bus system
    AudioBus* getMasterBus() { return masterBus.get(); }
    AudioBus* createBus(const std::string& name);
    void destroyBus(const std::string& name);
    AudioBus* getBus(const std::string& name);
    
    // Global settings
    void setMasterGain(float gain);
    float getMasterGain() const;
    
    void pauseAll();
    void resumeAll();
    void stopAll();
    
    // Performance monitoring
    float getCPUUsage() const { return cpuUsage; }
    size_t getActiveVoiceCount() const { return voiceCount; }
    size_t getSampleRate() const { return sampleRate; }
    size_t getBufferSize() const { return bufferSize; }
    
    // Audio streaming callback
    void processAudio(float* outputBuffer, size_t frameCount, size_t channelCount);
    
private:
    AdvancedAudioEngine();
    void audioProcessingLoop();
    void mixSources(AudioBuffer& outputBuffer);
    
    // Singleton pattern
    AdvancedAudioEngine(const AdvancedAudioEngine&) = delete;
    AdvancedAudioEngine& operator=(const AdvancedAudioEngine&) = delete;
};

// Utility functions for audio DSP
namespace AudioDSP {
    // Basic filters
    float lowpass(float input, float cutoff, float sampleRate, float& state);
    float highpass(float input, float cutoff, float sampleRate, float& state);
    float bandpass(float input, float centerFreq, float Q, float sampleRate, std::array<float, 2>& state);
    
    // Interpolation
    float linearInterpolate(float a, float b, float t);
    float cubicInterpolate(float a, float b, float c, float d, float t);
    
    // Window functions
    float hannWindow(size_t index, size_t length);
    float blackmanWindow(size_t index, size_t length);
    float gaussianWindow(size_t index, size_t length, float sigma = 0.5f);
    
    // Audio utilities
    float dbToLinear(float db);
    float linearToDb(float linear);
    float semitonesToRatio(float semitones);
    float ratioToSemitones(float ratio);
    
    // Sample rate conversion
    void resample(const AudioBuffer& input, AudioBuffer& output, float ratio);
    
    // Convolution
    void convolve(const AudioBuffer& signal, const std::vector<float>& impulse, AudioBuffer& output);
    void fftConvolve(const AudioBuffer& signal, const std::vector<float>& impulse, AudioBuffer& output);
}

} // namespace Advanced
} // namespace Audio
} // namespace JJM

#endif // ADVANCED_AUDIO_SYSTEM_H