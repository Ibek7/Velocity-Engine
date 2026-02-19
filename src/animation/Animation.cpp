#include "animation/Animation.h"

#include <cmath>
#include <stdexcept>

namespace JJM {
namespace Animation {

// AnimationClip implementation
AnimationClip::AnimationClip(const std::string& name, bool loop)
    : name(name), looping(loop), compressed(false) {}

void AnimationClip::addFrame(const SpriteFrame& frame) { frames.push_back(frame); }

void AnimationClip::addFrame(int x, int y, int w, int h, float duration) {
    frames.emplace_back(x, y, w, h, duration);
}

const SpriteFrame& AnimationClip::getFrame(int index) const {
    if (index < 0 || index >= static_cast<int>(frames.size())) {
        throw std::out_of_range("Frame index out of range");
    }
    return frames[index];
}

void AnimationClip::compress(float positionTolerance, float timeTolerance) {
    if (compressed || frames.size() < 3) {
        return;
    }

    std::vector<SpriteFrame> compressedFrames;
    compressedFrames.push_back(frames[0]);  // Always keep first frame

    for (size_t i = 1; i < frames.size() - 1; ++i) {
        const auto& prev = frames[i - 1];
        const auto& current = frames[i];
        const auto& next = frames[i + 1];

        // Check if current frame can be interpolated from prev and next
        bool canSkip = true;

        // Position check
        if (std::abs(current.x - (prev.x + next.x) / 2) > positionTolerance ||
            std::abs(current.y - (prev.y + next.y) / 2) > positionTolerance) {
            canSkip = false;
        }

        // Duration check
        if (std::abs(current.duration - prev.duration) > timeTolerance) {
            canSkip = false;
        }

        if (!canSkip) {
            compressedFrames.push_back(current);
        }
    }

    compressedFrames.push_back(frames.back());  // Always keep last frame

    frames = std::move(compressedFrames);
    compressed = true;
}

void AnimationClip::decompress() {
    // In a real implementation, would restore original frames
    // For now, just mark as decompressed
    compressed = false;
}

size_t AnimationClip::getUncompressedSize() const {
    return frames.capacity() * sizeof(SpriteFrame);
}

size_t AnimationClip::getCompressedSize() const { return frames.size() * sizeof(SpriteFrame); }

float AnimationClip::getCompressionRatio() const {
    if (getUncompressedSize() == 0) return 1.0f;
    return static_cast<float>(getCompressedSize()) / static_cast<float>(getUncompressedSize());
}

// Animator implementation
Animator::Animator() : currentFrame(0), frameTimer(0), playing(false), finished(false) {}

void Animator::addClip(const AnimationClip& clip) { clips[clip.getName()] = clip; }

void Animator::play(const std::string& clipName, bool restart) {
    auto it = clips.find(clipName);
    if (it == clips.end()) {
        return;
    }

    if (currentClipName != clipName || restart) {
        currentClipName = clipName;
        currentFrame = 0;
        frameTimer = 0;
        finished = false;
    }

    playing = true;
}

void Animator::pause() { playing = false; }

void Animator::resume() {
    if (!finished) {
        playing = true;
    }
}

void Animator::stop() {
    playing = false;
    currentFrame = 0;
    frameTimer = 0;
}

void Animator::reset() {
    currentFrame = 0;
    frameTimer = 0;
    finished = false;
}

void Animator::update(float deltaTime) {
    if (!playing || currentClipName.empty()) return;

    auto it = clips.find(currentClipName);
    if (it == clips.end()) return;

    const AnimationClip& clip = it->second;
    if (clip.getFrameCount() == 0) return;

    frameTimer += deltaTime;
    const SpriteFrame& currentSpriteFrame = clip.getFrame(currentFrame);

    if (frameTimer >= currentSpriteFrame.duration) {
        frameTimer = 0;
        currentFrame++;

        if (currentFrame >= clip.getFrameCount()) {
            if (clip.isLooping()) {
                currentFrame = 0;
            } else {
                currentFrame = clip.getFrameCount() - 1;
                finished = true;
                playing = false;

                if (onAnimationComplete) {
                    onAnimationComplete();
                }
            }
        }
    }
}

const SpriteFrame* Animator::getCurrentFrame() const {
    if (currentClipName.empty()) return nullptr;

    auto it = clips.find(currentClipName);
    if (it == clips.end()) return nullptr;

    const AnimationClip& clip = it->second;
    if (currentFrame < 0 || currentFrame >= clip.getFrameCount()) return nullptr;

    return &clip.getFrame(currentFrame);
}

// Ease implementations
float Ease::linear(float t) { return t; }

float Ease::quadIn(float t) { return t * t; }

float Ease::quadOut(float t) { return t * (2.0f - t); }

float Ease::quadInOut(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }

float Ease::cubicIn(float t) { return t * t * t; }

float Ease::cubicOut(float t) {
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}

float Ease::cubicInOut(float t) {
    return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

float Ease::sineIn(float t) { return 1.0f - std::cos(t * M_PI / 2.0f); }

float Ease::sineOut(float t) { return std::sin(t * M_PI / 2.0f); }

float Ease::sineInOut(float t) { return -(std::cos(M_PI * t) - 1.0f) / 2.0f; }

float Ease::expoIn(float t) { return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f); }

float Ease::expoOut(float t) { return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); }

float Ease::expoInOut(float t) {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f
                    : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}

float Ease::circIn(float t) { return 1.0f - std::sqrt(1.0f - t * t); }

float Ease::circOut(float t) { return std::sqrt((2.0f - t) * t); }

float Ease::circInOut(float t) {
    if (t < 0.5f) {
        return (1.0f - std::sqrt(1.0f - 4.0f * t * t)) / 2.0f;
    }
    return (std::sqrt(-((2.0f * t) - 3.0f) * ((2.0f * t) - 1.0f)) + 1.0f) / 2.0f;
}

float Ease::elasticIn(float t) {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    return -std::pow(2.0f, 10.0f * t - 10.0f) *
           std::sin((t * 10.0f - 10.75f) * (2.0f * M_PI) / 3.0f);
}

float Ease::elasticOut(float t) {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * (2.0f * M_PI) / 3.0f) + 1.0f;
}

float Ease::elasticInOut(float t) {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;

    if (t < 0.5f) {
        return -(std::pow(2.0f, 20.0f * t - 10.0f) *
                 std::sin((20.0f * t - 11.125f) * (2.0f * M_PI) / 4.5f)) /
               2.0f;
    }
    return (std::pow(2.0f, -20.0f * t + 10.0f) *
            std::sin((20.0f * t - 11.125f) * (2.0f * M_PI) / 4.5f)) /
               2.0f +
           1.0f;
}

float Ease::backIn(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return c3 * t * t * t - c1 * t * t;
}

float Ease::backOut(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
}

float Ease::backInOut(float t) {
    const float c1 = 1.70158f;
    const float c2 = c1 * 1.525f;

    return t < 0.5f
               ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
               : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) /
                     2.0f;
}

float Ease::bounceOut(float t) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

float Ease::bounceIn(float t) { return 1.0f - bounceOut(1.0f - t); }

float Ease::bounceInOut(float t) {
    return t < 0.5f ? (1.0f - bounceOut(1.0f - 2.0f * t)) / 2.0f
                    : (1.0f + bounceOut(2.0f * t - 1.0f)) / 2.0f;
}

// Tween template specializations for common types
template <>
float Tween<float>::applyEasing(float t) {
    switch (easeType) {
        case EaseType::Linear:
            return Ease::linear(t);
        case EaseType::QuadIn:
            return Ease::quadIn(t);
        case EaseType::QuadOut:
            return Ease::quadOut(t);
        case EaseType::QuadInOut:
            return Ease::quadInOut(t);
        case EaseType::CubicIn:
            return Ease::cubicIn(t);
        case EaseType::CubicOut:
            return Ease::cubicOut(t);
        case EaseType::CubicInOut:
            return Ease::cubicInOut(t);
        case EaseType::SineIn:
            return Ease::sineIn(t);
        case EaseType::SineOut:
            return Ease::sineOut(t);
        case EaseType::SineInOut:
            return Ease::sineInOut(t);
        default:
            return t;
    }
}

template <>
float Tween<float>::interpolate(const float& a, const float& b, float t) {
    return a + (b - a) * t;
}

template <>
Math::Vector2D Tween<Math::Vector2D>::interpolate(const Math::Vector2D& a, const Math::Vector2D& b,
                                                  float t) {
    return a.lerp(b, t);
}

template <>
float Tween<Math::Vector2D>::applyEasing(float t) {
    switch (easeType) {
        case EaseType::Linear:
            return Ease::linear(t);
        case EaseType::QuadIn:
            return Ease::quadIn(t);
        case EaseType::QuadOut:
            return Ease::quadOut(t);
        case EaseType::QuadInOut:
            return Ease::quadInOut(t);
        default:
            return t;
    }
}

// Explicit template instantiations
template class Tween<float>;
template class Tween<Math::Vector2D>;

}  // namespace Animation
}  // namespace JJM
