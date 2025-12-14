#include "audio/AudioEffects.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Audio {

// AudioEffect implementation
AudioEffect::AudioEffect() : enabled(true), mix(1.0f) {}
AudioEffect::~AudioEffect() {}

void AudioEffect::setEnabled(bool enabled) {
    this->enabled = enabled;
}

bool AudioEffect::isEnabled() const {
    return enabled;
}

void AudioEffect::setMix(float mix) {
    this->mix = std::clamp(mix, 0.0f, 1.0f);
}

float AudioEffect::getMix() const {
    return mix;
}

// ReverbEffect implementation
ReverbEffect::ReverbEffect()
    : roomSize(0.5f), damping(0.5f), width(1.0f) {
    for (int i = 0; i < 8; ++i) {
        combIndices[i] = 0;
    }
    for (int i = 0; i < 4; ++i) {
        allpassIndices[i] = 0;
    }
    initializeBuffers(44100);
}

ReverbEffect::~ReverbEffect() {}

void ReverbEffect::process(float* buffer, int numSamples, int numChannels) {
    if (!enabled) return;
    
    for (int i = 0; i < numSamples * numChannels; ++i) {
        float input = buffer[i];
        float output = 0.0f;
        
        for (int c = 0; c < 8; ++c) {
            output += processComb(input, c);
        }
        output /= 8.0f;
        
        for (int a = 0; a < 4; ++a) {
            output = processAllpass(output, a);
        }
        
        buffer[i] = input * (1.0f - mix) + output * mix;
    }
}

void ReverbEffect::setRoomSize(float size) {
    roomSize = std::clamp(size, 0.0f, 1.0f);
}

void ReverbEffect::setDamping(float damp) {
    damping = std::clamp(damp, 0.0f, 1.0f);
}

void ReverbEffect::setWidth(float w) {
    width = std::clamp(w, 0.0f, 1.0f);
}

void ReverbEffect::initializeBuffers(int sampleRate) {
    int combLengths[] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    int allpassLengths[] = {556, 441, 341, 225};
    
    for (int i = 0; i < 8; ++i) {
        int length = combLengths[i] * sampleRate / 44100;
        combBuffers[i].resize(length, 0.0f);
    }
    
    for (int i = 0; i < 4; ++i) {
        int length = allpassLengths[i] * sampleRate / 44100;
        allpassBuffers[i].resize(length, 0.0f);
    }
}

float ReverbEffect::processComb(float input, int combIndex) {
    std::vector<float>& buffer = combBuffers[combIndex];
    int& index = combIndices[combIndex];
    
    float output = buffer[index];
    float filtered = output * (1.0f - damping) + input * damping;
    buffer[index] = input + filtered * roomSize;
    
    index = (index + 1) % buffer.size();
    return output;
}

float ReverbEffect::processAllpass(float input, int allpassIndex) {
    std::vector<float>& buffer = allpassBuffers[allpassIndex];
    int& index = allpassIndices[allpassIndex];
    
    float bufferOut = buffer[index];
    float output = -input + bufferOut;
    buffer[index] = input + bufferOut * 0.5f;
    
    index = (index + 1) % buffer.size();
    return output;
}

// EchoEffect implementation
EchoEffect::EchoEffect()
    : delayTime(0.5f), feedback(0.5f), decay(0.7f),
      writeIndex(0), sampleRate(44100) {
    resizeBuffer(static_cast<int>(delayTime * sampleRate));
}

EchoEffect::~EchoEffect() {}

void EchoEffect::process(float* buffer, int numSamples, int numChannels) {
    if (!enabled) return;
    
    for (int i = 0; i < numSamples * numChannels; ++i) {
        float input = buffer[i];
        
        int readIndex = writeIndex - static_cast<int>(delayTime * sampleRate);
        if (readIndex < 0) readIndex += delayBuffer.size();
        
        float delayed = delayBuffer[readIndex];
        float output = input + delayed * feedback;
        
        delayBuffer[writeIndex] = input + delayed * decay;
        writeIndex = (writeIndex + 1) % delayBuffer.size();
        
        buffer[i] = input * (1.0f - mix) + output * mix;
    }
}

void EchoEffect::setDelayTime(float timeInSeconds) {
    delayTime = std::clamp(timeInSeconds, 0.0f, 2.0f);
    resizeBuffer(static_cast<int>(delayTime * sampleRate));
}

void EchoEffect::setFeedback(float fb) {
    feedback = std::clamp(fb, 0.0f, 0.95f);
}

void EchoEffect::setDecay(float d) {
    decay = std::clamp(d, 0.0f, 1.0f);
}

void EchoEffect::resizeBuffer(int samples) {
    delayBuffer.resize(samples, 0.0f);
    writeIndex = 0;
}

// ChorusEffect implementation
ChorusEffect::ChorusEffect()
    : rate(1.5f), depth(0.02f), voices(3), phase(0.0f), writeIndex(0) {
    delayBuffer.resize(4410, 0.0f); // 100ms at 44.1kHz
}

ChorusEffect::~ChorusEffect() {}

void ChorusEffect::process(float* buffer, int numSamples, int numChannels) {
    if (!enabled) return;
    
    for (int i = 0; i < numSamples * numChannels; ++i) {
        float input = buffer[i];
        delayBuffer[writeIndex] = input;
        
        float output = 0.0f;
        for (int v = 0; v < voices; ++v) {
            float voicePhase = phase + (v * 2.0f * M_PI / voices);
            float lfo = getLFO(voicePhase);
            float delaySamples = depth * 44100 * (lfo + 1.0f) * 0.5f;
            
            int readIndex = writeIndex - static_cast<int>(delaySamples);
            if (readIndex < 0) readIndex += delayBuffer.size();
            
            output += delayBuffer[readIndex];
        }
        output /= voices;
        
        writeIndex = (writeIndex + 1) % delayBuffer.size();
        phase += rate * 2.0f * M_PI / 44100;
        if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        
        buffer[i] = input * (1.0f - mix) + (input + output) * 0.5f * mix;
    }
}

void ChorusEffect::setRate(float r) {
    rate = std::clamp(r, 0.1f, 10.0f);
}

void ChorusEffect::setDepth(float d) {
    depth = std::clamp(d, 0.0f, 0.1f);
}

void ChorusEffect::setVoices(int v) {
    voices = std::clamp(v, 1, 8);
}

float ChorusEffect::getLFO(float p) {
    return std::sin(p);
}

// DistortionEffect implementation
DistortionEffect::DistortionEffect()
    : drive(5.0f), tone(0.5f), level(1.0f) {}

DistortionEffect::~DistortionEffect() {}

void DistortionEffect::process(float* buffer, int numSamples, int numChannels) {
    if (!enabled) return;
    
    for (int i = 0; i < numSamples * numChannels; ++i) {
        float input = buffer[i];
        float distorted = applyDistortion(input * drive) * level;
        buffer[i] = input * (1.0f - mix) + distorted * mix;
    }
}

void DistortionEffect::setDrive(float d) {
    drive = std::clamp(d, 1.0f, 100.0f);
}

void DistortionEffect::setTone(float t) {
    tone = std::clamp(t, 0.0f, 1.0f);
}

void DistortionEffect::setLevel(float l) {
    level = std::clamp(l, 0.0f, 2.0f);
}

float DistortionEffect::applyDistortion(float sample) {
    return std::tanh(sample);
}

// CompressorEffect implementation
CompressorEffect::CompressorEffect()
    : threshold(0.5f), ratio(4.0f), attack(0.01f), release(0.1f), envelope(0.0f) {}

CompressorEffect::~CompressorEffect() {}

void CompressorEffect::process(float* buffer, int numSamples, int numChannels) {
    if (!enabled) return;
    
    for (int i = 0; i < numSamples * numChannels; ++i) {
        float input = buffer[i];
        float inputAbs = std::abs(input);
        
        if (inputAbs > envelope) {
            envelope += (inputAbs - envelope) * attack;
        } else {
            envelope += (inputAbs - envelope) * release;
        }
        
        float gain = computeGain(envelope);
        buffer[i] = input * gain;
    }
}

void CompressorEffect::setThreshold(float t) {
    threshold = std::clamp(t, 0.0f, 1.0f);
}

void CompressorEffect::setRatio(float r) {
    ratio = std::clamp(r, 1.0f, 20.0f);
}

void CompressorEffect::setAttack(float a) {
    attack = std::clamp(a, 0.001f, 1.0f);
}

void CompressorEffect::setRelease(float r) {
    release = std::clamp(r, 0.01f, 1.0f);
}

float CompressorEffect::computeGain(float input) {
    if (input <= threshold) {
        return 1.0f;
    }
    float excess = input - threshold;
    return threshold + excess / ratio;
}

// EqualizerEffect implementation
EqualizerEffect::EqualizerEffect() {
    bands.resize(3);
    setBand(0, 100.0f, 0.0f, 1.0f);   // Low
    setBand(1, 1000.0f, 0.0f, 1.0f);  // Mid
    setBand(2, 10000.0f, 0.0f, 1.0f); // High
}

EqualizerEffect::~EqualizerEffect() {}

void EqualizerEffect::process(float* buffer, int numSamples, int numChannels) {
    if (!enabled) return;
    
    for (int i = 0; i < numSamples * numChannels; ++i) {
        float input = buffer[i];
        float output = input;
        
        for (auto& band : bands) {
            output = processBand(band, output);
        }
        
        buffer[i] = output;
    }
}

void EqualizerEffect::setBand(int band, float frequency, float gain, float q) {
    if (band >= 0 && band < static_cast<int>(bands.size())) {
        bands[band].frequency = frequency;
        bands[band].gain = gain;
        bands[band].q = q;
        updateBandCoefficients(bands[band]);
    }
}

void EqualizerEffect::setGain(int band, float gain) {
    if (band >= 0 && band < static_cast<int>(bands.size())) {
        bands[band].gain = gain;
        updateBandCoefficients(bands[band]);
    }
}

void EqualizerEffect::updateBandCoefficients(Band& band) {
    float omega = 2.0f * M_PI * band.frequency / 44100.0f;
    float sn = std::sin(omega);
    float cs = std::cos(omega);
    float alpha = sn / (2.0f * band.q);
    float A = std::pow(10.0f, band.gain / 40.0f);
    
    band.b0 = 1.0f + alpha * A;
    band.b1 = -2.0f * cs;
    band.b2 = 1.0f - alpha * A;
    band.a0 = 1.0f + alpha / A;
    band.a1 = -2.0f * cs;
    band.a2 = 1.0f - alpha / A;
    
    band.x1 = band.x2 = band.y1 = band.y2 = 0.0f;
}

float EqualizerEffect::processBand(Band& band, float input) {
    float output = (band.b0 / band.a0) * input +
                   (band.b1 / band.a0) * band.x1 +
                   (band.b2 / band.a0) * band.x2 -
                   (band.a1 / band.a0) * band.y1 -
                   (band.a2 / band.a0) * band.y2;
    
    band.x2 = band.x1;
    band.x1 = input;
    band.y2 = band.y1;
    band.y1 = output;
    
    return output;
}

// FlangerEffect implementation
FlangerEffect::FlangerEffect()
    : rate(0.5f), depth(0.002f), feedback(0.5f), phase(0.0f), writeIndex(0) {
    delayBuffer.resize(4410, 0.0f);
}

FlangerEffect::~FlangerEffect() {}

void FlangerEffect::process(float* buffer, int numSamples, int numChannels) {
    if (!enabled) return;
    
    for (int i = 0; i < numSamples * numChannels; ++i) {
        float input = buffer[i];
        
        float lfo = std::sin(phase);
        float delaySamples = depth * 44100 * (lfo + 1.0f) * 0.5f;
        
        int readIndex = writeIndex - static_cast<int>(delaySamples);
        if (readIndex < 0) readIndex += delayBuffer.size();
        
        float delayed = delayBuffer[readIndex];
        delayBuffer[writeIndex] = input + delayed * feedback;
        
        writeIndex = (writeIndex + 1) % delayBuffer.size();
        phase += rate * 2.0f * M_PI / 44100;
        if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        
        buffer[i] = input * (1.0f - mix) + (input + delayed) * 0.5f * mix;
    }
}

void FlangerEffect::setRate(float r) {
    rate = std::clamp(r, 0.1f, 10.0f);
}

void FlangerEffect::setDepth(float d) {
    depth = std::clamp(d, 0.0f, 0.01f);
}

void FlangerEffect::setFeedback(float fb) {
    feedback = std::clamp(fb, 0.0f, 0.95f);
}

// PhaserEffect implementation
PhaserEffect::PhaserEffect()
    : rate(0.5f), depth(1.0f), stages(4), phase(0.0f) {
    filters.resize(stages);
    for (auto& filter : filters) {
        filter.a1 = 0.0f;
        filter.zm1 = 0.0f;
    }
}

PhaserEffect::~PhaserEffect() {}

void PhaserEffect::process(float* buffer, int numSamples, int numChannels) {
    if (!enabled) return;
    
    for (int i = 0; i < numSamples * numChannels; ++i) {
        float input = buffer[i];
        
        float lfo = std::sin(phase);
        float allpassFreq = 440.0f + lfo * depth * 1000.0f;
        
        for (auto& filter : filters) {
            filter.a1 = (std::tan(M_PI * allpassFreq / 44100) - 1.0f) /
                       (std::tan(M_PI * allpassFreq / 44100) + 1.0f);
        }
        
        float output = input;
        for (auto& filter : filters) {
            output = processAllpass(filter, output);
        }
        
        phase += rate * 2.0f * M_PI / 44100;
        if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        
        buffer[i] = input * (1.0f - mix) + (input + output) * 0.5f * mix;
    }
}

void PhaserEffect::setRate(float r) {
    rate = std::clamp(r, 0.1f, 10.0f);
}

void PhaserEffect::setDepth(float d) {
    depth = std::clamp(d, 0.0f, 1.0f);
}

void PhaserEffect::setStages(int s) {
    stages = std::clamp(s, 2, 12);
    filters.resize(stages);
}

float PhaserEffect::processAllpass(AllpassFilter& filter, float input) {
    float output = filter.a1 * input + filter.zm1;
    filter.zm1 = input - filter.a1 * output;
    return output;
}

// PitchShifterEffect implementation
PitchShifterEffect::PitchShifterEffect()
    : pitchShift(0.0f), readIndex(0), writeIndex(0) {
    inputBuffer.resize(8192, 0.0f);
    outputBuffer.resize(8192, 0.0f);
}

PitchShifterEffect::~PitchShifterEffect() {}

void PitchShifterEffect::process(float* buffer, int numSamples, int numChannels) {
    if (!enabled) return;
    
    float pitchRatio = std::pow(2.0f, pitchShift / 12.0f);
    
    for (int i = 0; i < numSamples * numChannels; ++i) {
        inputBuffer[writeIndex] = buffer[i];
        
        float readPos = readIndex * pitchRatio;
        int index1 = static_cast<int>(readPos) % inputBuffer.size();
        int index2 = (index1 + 1) % inputBuffer.size();
        float frac = readPos - std::floor(readPos);
        
        float interpolated = inputBuffer[index1] * (1.0f - frac) +
                            inputBuffer[index2] * frac;
        
        buffer[i] = buffer[i] * (1.0f - mix) + interpolated * mix;
        
        writeIndex = (writeIndex + 1) % inputBuffer.size();
        readIndex = (readIndex + 1) % inputBuffer.size();
    }
}

void PitchShifterEffect::setPitchShift(float semitones) {
    pitchShift = std::clamp(semitones, -12.0f, 12.0f);
}

// AudioEffectChain implementation
AudioEffectChain::AudioEffectChain() {}
AudioEffectChain::~AudioEffectChain() {}

void AudioEffectChain::addEffect(std::unique_ptr<AudioEffect> effect) {
    effects.push_back(std::move(effect));
}

void AudioEffectChain::removeEffect(size_t index) {
    if (index < effects.size()) {
        effects.erase(effects.begin() + index);
    }
}

void AudioEffectChain::clearEffects() {
    effects.clear();
}

void AudioEffectChain::process(float* buffer, int numSamples, int numChannels) {
    for (auto& effect : effects) {
        if (effect->isEnabled()) {
            effect->process(buffer, numSamples, numChannels);
        }
    }
}

AudioEffect* AudioEffectChain::getEffect(size_t index) {
    return index < effects.size() ? effects[index].get() : nullptr;
}

size_t AudioEffectChain::getEffectCount() const {
    return effects.size();
}

// SpatializationEffect implementation
SpatializationEffect::SpatializationEffect()
    : sourceX(0), sourceY(0), sourceZ(0),
      listenerX(0), listenerY(0), listenerZ(0),
      forwardX(0), forwardY(0), forwardZ(1),
      upX(0), upY(1), upZ(0),
      inverseDistance(true), rolloffFactor(1.0f), maxDistance(100.0f) {}

SpatializationEffect::~SpatializationEffect() {}

void SpatializationEffect::process(float* buffer, int numSamples, int numChannels) {
    if (!enabled || numChannels < 2) return;
    
    float leftGain, rightGain;
    calculateGains(leftGain, rightGain);
    
    for (int i = 0; i < numSamples; ++i) {
        float mono = (buffer[i * 2] + buffer[i * 2 + 1]) * 0.5f;
        buffer[i * 2] = mono * leftGain;
        buffer[i * 2 + 1] = mono * rightGain;
    }
}

void SpatializationEffect::setPosition(float x, float y, float z) {
    sourceX = x; sourceY = y; sourceZ = z;
}

void SpatializationEffect::setListenerPosition(float x, float y, float z) {
    listenerX = x; listenerY = y; listenerZ = z;
}

void SpatializationEffect::setListenerOrientation(float fx, float fy, float fz,
                                                   float ux, float uy, float uz) {
    forwardX = fx; forwardY = fy; forwardZ = fz;
    upX = ux; upY = uy; upZ = uz;
}

void SpatializationEffect::setDistanceModel(bool inverse) {
    inverseDistance = inverse;
}

void SpatializationEffect::setRolloffFactor(float rolloff) {
    rolloffFactor = std::max(0.0f, rolloff);
}

void SpatializationEffect::setMaxDistance(float distance) {
    maxDistance = std::max(0.1f, distance);
}

void SpatializationEffect::calculateGains(float& leftGain, float& rightGain) {
    float dx = sourceX - listenerX;
    float dy = sourceY - listenerY;
    float dz = sourceZ - listenerZ;
    float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    float attenuation = calculateAttenuation(distance);
    
    float angle = std::atan2(dx, dz);
    leftGain = attenuation * (1.0f + std::sin(angle)) * 0.5f;
    rightGain = attenuation * (1.0f - std::sin(angle)) * 0.5f;
}

float SpatializationEffect::calculateAttenuation(float distance) {
    if (distance >= maxDistance) return 0.0f;
    
    if (inverseDistance) {
        return 1.0f / (1.0f + rolloffFactor * distance);
    } else {
        return 1.0f - (distance / maxDistance);
    }
}

// ConvolutionReverbEffect implementation
ConvolutionReverbEffect::ConvolutionReverbEffect() : historyIndex(0) {}
ConvolutionReverbEffect::~ConvolutionReverbEffect() {}

void ConvolutionReverbEffect::process(float* buffer, int numSamples, int numChannels) {
    if (!enabled || impulseResponse.empty()) return;
    
    for (int i = 0; i < numSamples * numChannels; ++i) {
        float input = buffer[i];
        
        inputHistory[historyIndex] = input;
        float output = convolve(inputHistory.data(), inputHistory.size());
        
        historyIndex = (historyIndex + 1) % inputHistory.size();
        buffer[i] = input * (1.0f - mix) + output * mix;
    }
}

void ConvolutionReverbEffect::loadImpulseResponse(const std::string& filePath) {
    (void)filePath; // Stub
}

void ConvolutionReverbEffect::setImpulseResponse(const float* data, int length) {
    impulseResponse.assign(data, data + length);
    inputHistory.resize(length, 0.0f);
    historyIndex = 0;
}

float ConvolutionReverbEffect::convolve(const float* input, int inputLength) {
    float result = 0.0f;
    int irLength = std::min(inputLength, static_cast<int>(impulseResponse.size()));
    
    for (int i = 0; i < irLength; ++i) {
        int idx = (historyIndex - i + inputLength) % inputLength;
        result += input[idx] * impulseResponse[i];
    }
    
    return result;
}

} // namespace Audio
} // namespace JJM
