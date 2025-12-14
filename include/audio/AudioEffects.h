#ifndef JJM_AUDIO_EFFECTS_H
#define JJM_AUDIO_EFFECTS_H

#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace JJM {
namespace Audio {

/**
 * @brief Base class for audio effects
 */
class AudioEffect {
public:
    AudioEffect();
    virtual ~AudioEffect();

    virtual void process(float* buffer, int numSamples, int numChannels) = 0;
    
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    void setMix(float mix);
    float getMix() const;

protected:
    bool enabled;
    float mix; // 0.0 = dry, 1.0 = wet
};

/**
 * @brief Reverb effect for spatial depth
 */
class ReverbEffect : public AudioEffect {
public:
    ReverbEffect();
    ~ReverbEffect() override;

    void process(float* buffer, int numSamples, int numChannels) override;
    
    void setRoomSize(float size);
    void setDamping(float damping);
    void setWidth(float width);

private:
    float roomSize;
    float damping;
    float width;
    
    std::vector<float> combBuffers[8];
    std::vector<float> allpassBuffers[4];
    int combIndices[8];
    int allpassIndices[4];
    
    void initializeBuffers(int sampleRate);
    float processComb(float input, int combIndex);
    float processAllpass(float input, int allpassIndex);
};

/**
 * @brief Echo/Delay effect
 */
class EchoEffect : public AudioEffect {
public:
    EchoEffect();
    ~EchoEffect() override;

    void process(float* buffer, int numSamples, int numChannels) override;
    
    void setDelayTime(float timeInSeconds);
    void setFeedback(float feedback);
    void setDecay(float decay);

private:
    float delayTime;
    float feedback;
    float decay;
    
    std::vector<float> delayBuffer;
    int writeIndex;
    int sampleRate;
    
    void resizeBuffer(int samples);
};

/**
 * @brief Chorus effect for depth and width
 */
class ChorusEffect : public AudioEffect {
public:
    ChorusEffect();
    ~ChorusEffect() override;

    void process(float* buffer, int numSamples, int numChannels) override;
    
    void setRate(float rate);
    void setDepth(float depth);
    void setVoices(int voices);

private:
    float rate;
    float depth;
    int voices;
    float phase;
    
    std::vector<float> delayBuffer;
    int writeIndex;
    
    float getLFO(float phase);
};

/**
 * @brief Distortion effect
 */
class DistortionEffect : public AudioEffect {
public:
    DistortionEffect();
    ~DistortionEffect() override;

    void process(float* buffer, int numSamples, int numChannels) override;
    
    void setDrive(float drive);
    void setTone(float tone);
    void setLevel(float level);

private:
    float drive;
    float tone;
    float level;
    
    float applyDistortion(float sample);
};

/**
 * @brief Compressor effect for dynamic range control
 */
class CompressorEffect : public AudioEffect {
public:
    CompressorEffect();
    ~CompressorEffect() override;

    void process(float* buffer, int numSamples, int numChannels) override;
    
    void setThreshold(float threshold);
    void setRatio(float ratio);
    void setAttack(float attack);
    void setRelease(float release);

private:
    float threshold;
    float ratio;
    float attack;
    float release;
    float envelope;
    
    float computeGain(float input);
};

/**
 * @brief Equalizer effect with multiple bands
 */
class EqualizerEffect : public AudioEffect {
public:
    EqualizerEffect();
    ~EqualizerEffect() override;

    void process(float* buffer, int numSamples, int numChannels) override;
    
    void setBand(int band, float frequency, float gain, float q);
    void setGain(int band, float gain);

private:
    struct Band {
        float frequency;
        float gain;
        float q;
        float a0, a1, a2, b0, b1, b2;
        float x1, x2, y1, y2;
    };
    
    std::vector<Band> bands;
    
    void updateBandCoefficients(Band& band);
    float processBand(Band& band, float input);
};

/**
 * @brief Flanger effect
 */
class FlangerEffect : public AudioEffect {
public:
    FlangerEffect();
    ~FlangerEffect() override;

    void process(float* buffer, int numSamples, int numChannels) override;
    
    void setRate(float rate);
    void setDepth(float depth);
    void setFeedback(float feedback);

private:
    float rate;
    float depth;
    float feedback;
    float phase;
    
    std::vector<float> delayBuffer;
    int writeIndex;
};

/**
 * @brief Phaser effect
 */
class PhaserEffect : public AudioEffect {
public:
    PhaserEffect();
    ~PhaserEffect() override;

    void process(float* buffer, int numSamples, int numChannels) override;
    
    void setRate(float rate);
    void setDepth(float depth);
    void setStages(int stages);

private:
    float rate;
    float depth;
    int stages;
    float phase;
    
    struct AllpassFilter {
        float a1;
        float zm1;
    };
    
    std::vector<AllpassFilter> filters;
    
    float processAllpass(AllpassFilter& filter, float input);
};

/**
 * @brief Pitch shifter effect
 */
class PitchShifterEffect : public AudioEffect {
public:
    PitchShifterEffect();
    ~PitchShifterEffect() override;

    void process(float* buffer, int numSamples, int numChannels) override;
    
    void setPitchShift(float semitones);

private:
    float pitchShift;
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    int readIndex;
    int writeIndex;
};

/**
 * @brief Audio effect chain processor
 */
class AudioEffectChain {
public:
    AudioEffectChain();
    ~AudioEffectChain();

    void addEffect(std::unique_ptr<AudioEffect> effect);
    void removeEffect(size_t index);
    void clearEffects();
    
    void process(float* buffer, int numSamples, int numChannels);
    
    AudioEffect* getEffect(size_t index);
    size_t getEffectCount() const;

private:
    std::vector<std::unique_ptr<AudioEffect>> effects;
};

/**
 * @brief 3D audio spatialization
 */
class SpatializationEffect : public AudioEffect {
public:
    SpatializationEffect();
    ~SpatializationEffect() override;

    void process(float* buffer, int numSamples, int numChannels) override;
    
    void setPosition(float x, float y, float z);
    void setListenerPosition(float x, float y, float z);
    void setListenerOrientation(float forwardX, float forwardY, float forwardZ,
                                float upX, float upY, float upZ);
    
    void setDistanceModel(bool inverseDistance);
    void setRolloffFactor(float rolloff);
    void setMaxDistance(float distance);

private:
    float sourceX, sourceY, sourceZ;
    float listenerX, listenerY, listenerZ;
    float forwardX, forwardY, forwardZ;
    float upX, upY, upZ;
    
    bool inverseDistance;
    float rolloffFactor;
    float maxDistance;
    
    void calculateGains(float& leftGain, float& rightGain);
    float calculateAttenuation(float distance);
};

/**
 * @brief Convolution reverb using impulse responses
 */
class ConvolutionReverbEffect : public AudioEffect {
public:
    ConvolutionReverbEffect();
    ~ConvolutionReverbEffect() override;

    void process(float* buffer, int numSamples, int numChannels) override;
    
    void loadImpulseResponse(const std::string& filePath);
    void setImpulseResponse(const float* data, int length);

private:
    std::vector<float> impulseResponse;
    std::vector<float> inputHistory;
    int historyIndex;
    
    float convolve(const float* input, int inputLength);
};

} // namespace Audio
} // namespace JJM

#endif // JJM_AUDIO_EFFECTS_H
