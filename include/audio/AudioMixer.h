#ifndef JJM_AUDIO_MIXER_H
#define JJM_AUDIO_MIXER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace JJM {
namespace Audio {

/**
 * @brief Audio channel for mixing
 */
class AudioChannel {
public:
    AudioChannel(const std::string& name);
    ~AudioChannel();

    void setVolume(float volume);
    float getVolume() const;
    
    void setPan(float pan);
    float getPan() const;
    
    void setMuted(bool muted);
    bool isMuted() const;
    
    void setSolo(bool solo);
    bool isSolo() const;
    
    std::string getName() const;

private:
    std::string name;
    float volume;
    float pan;
    bool muted;
    bool solo;
};

/**
 * @brief Audio bus for routing
 */
class AudioBus {
public:
    AudioBus(const std::string& name);
    ~AudioBus();

    void addChannel(std::shared_ptr<AudioChannel> channel);
    void removeChannel(std::shared_ptr<AudioChannel> channel);
    
    void setVolume(float volume);
    float getVolume() const;
    
    void setMuted(bool muted);
    bool isMuted() const;
    
    std::string getName() const;
    
    const std::vector<std::shared_ptr<AudioChannel>>& getChannels() const;

private:
    std::string name;
    std::vector<std::shared_ptr<AudioChannel>> channels;
    float volume;
    bool muted;
};

/**
 * @brief Audio effect type
 */
enum class AudioEffectType {
    Reverb,
    Echo,
    Delay,
    Compressor,
    Equalizer,
    Distortion,
    Chorus,
    Flanger
};

/**
 * @brief Audio effect base class
 */
class AudioEffect {
public:
    AudioEffect(AudioEffectType type);
    virtual ~AudioEffect();

    virtual void process(float* buffer, int samples) = 0;
    
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    AudioEffectType getType() const;

protected:
    AudioEffectType type;
    bool enabled;
};

/**
 * @brief Reverb effect
 */
class ReverbEffect : public AudioEffect {
public:
    ReverbEffect();
    ~ReverbEffect();

    void process(float* buffer, int samples) override;
    
    void setRoomSize(float size);
    void setDamping(float damping);
    void setWetLevel(float wet);
    void setDryLevel(float dry);

private:
    float roomSize;
    float damping;
    float wetLevel;
    float dryLevel;
};

/**
 * @brief Equalizer effect
 */
class EqualizerEffect : public AudioEffect {
public:
    EqualizerEffect();
    ~EqualizerEffect();

    void process(float* buffer, int samples) override;
    
    void setBandGain(int band, float gain);
    float getBandGain(int band) const;
    
    void setNumBands(int bands);
    int getNumBands() const;

private:
    std::vector<float> bandGains;
    int numBands;
};

/**
 * @brief Audio mixer
 */
class AudioMixer {
public:
    static AudioMixer& getInstance();
    
    AudioMixer(const AudioMixer&) = delete;
    AudioMixer& operator=(const AudioMixer&) = delete;

    void update();
    void process(float* outputBuffer, int samples);
    
    std::shared_ptr<AudioChannel> createChannel(const std::string& name);
    void destroyChannel(const std::string& name);
    std::shared_ptr<AudioChannel> getChannel(const std::string& name);
    
    std::shared_ptr<AudioBus> createBus(const std::string& name);
    void destroyBus(const std::string& name);
    std::shared_ptr<AudioBus> getBus(const std::string& name);
    
    void addEffect(const std::string& channelName, std::shared_ptr<AudioEffect> effect);
    void removeEffect(const std::string& channelName, std::shared_ptr<AudioEffect> effect);
    
    void setMasterVolume(float volume);
    float getMasterVolume() const;
    
    void setMasterMuted(bool muted);
    bool isMasterMuted() const;

private:
    AudioMixer();
    ~AudioMixer();
    
    std::unordered_map<std::string, std::shared_ptr<AudioChannel>> channels;
    std::unordered_map<std::string, std::shared_ptr<AudioBus>> buses;
    std::unordered_map<std::string, std::vector<std::shared_ptr<AudioEffect>>> effects;
    float masterVolume;
    bool masterMuted;
};

/**
 * @brief Audio mixer snapshot for quick state changes
 */
class MixerSnapshot {
public:
    MixerSnapshot(const std::string& name);
    ~MixerSnapshot();

    void capture();
    void restore();
    void blend(const MixerSnapshot& other, float t);
    
    std::string getName() const;

private:
    std::string name;
    std::unordered_map<std::string, float> channelVolumes;
    std::unordered_map<std::string, float> busVolumes;
};

/**
 * @brief Audio ducking for automatic volume adjustment
 */
class AudioDucking {
public:
    AudioDucking();
    ~AudioDucking();

    void setThreshold(float threshold);
    void setRatio(float ratio);
    void setAttack(float attack);
    void setRelease(float release);
    
    void process(float* buffer, int samples, const float* sidechain);

private:
    float threshold;
    float ratio;
    float attack;
    float release;
    float envelope;
};

} // namespace Audio
} // namespace JJM

#endif // JJM_AUDIO_MIXER_H
