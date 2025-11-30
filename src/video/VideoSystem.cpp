#include "video/VideoSystem.h"
#include <iostream>
#include <thread>
#include <cstring>
#include <cmath>

namespace JJM {
namespace Video {

// -- VideoBuffer implementation
VideoBuffer::VideoBuffer(size_t maxVideoFrames, size_t maxAudioFrames)
    : maxVideoFrames(maxVideoFrames), maxAudioFrames(maxAudioFrames),
      videoFrameCount(0), audioFrameCount(0) {}

bool VideoBuffer::pushVideoFrame(std::unique_ptr<VideoFrame> frame) {
    std::unique_lock<std::mutex> lock(videoMutex);
    if (videoFrameCount >= maxVideoFrames) return false;
    videoFrames.push(std::move(frame));
    videoFrameCount++;
    lock.unlock();
    videoCV.notify_one();
    return true;
}

bool VideoBuffer::popVideoFrame(std::unique_ptr<VideoFrame>& frame, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(videoMutex);
    if (!videoCV.wait_for(lock, timeout, [this]{ return !videoFrames.empty(); })) return false;
    frame = std::move(videoFrames.front());
    videoFrames.pop();
    videoFrameCount--;
    return true;
}

bool VideoBuffer::pushAudioFrame(std::unique_ptr<AudioFrame> frame) {
    std::unique_lock<std::mutex> lock(audioMutex);
    if (audioFrameCount >= maxAudioFrames) return false;
    audioFrames.push(std::move(frame));
    audioFrameCount++;
    lock.unlock();
    audioCV.notify_one();
    return true;
}

bool VideoBuffer::popAudioFrame(std::unique_ptr<AudioFrame>& frame, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(audioMutex);
    if (!audioCV.wait_for(lock, timeout, [this]{ return !audioFrames.empty(); })) return false;
    frame = std::move(audioFrames.front());
    audioFrames.pop();
    audioFrameCount--;
    return true;
}

void VideoBuffer::clear() {
    std::lock_guard<std::mutex> vlock(videoMutex);
    while (!videoFrames.empty()) videoFrames.pop();
    videoFrameCount = 0;
    std::lock_guard<std::mutex> alock(audioMutex);
    while (!audioFrames.empty()) audioFrames.pop();
    audioFrameCount = 0;
}

// -- VideoRenderer implementation
VideoRenderer::VideoRenderer()
    : videoTexture(nullptr), position(0,0), size(640,360), scalingMode(VideoScalingMode::AspectFit), alpha(1.0f), visible(true) {
    originalSize = Math::Vector2D(640,360);
    calculateDisplaySize();
}

VideoRenderer::~VideoRenderer() { shutdown(); }

void VideoRenderer::initialize(int width, int height) {
    originalSize = Math::Vector2D((float)width, (float)height);
    // Create or resize texture - placeholder using Graphics::Texture
    // videoTexture = Graphics::Texture::create(width, height, ...);
}

void VideoRenderer::shutdown() {
    // Destroy texture if present
    videoTexture = nullptr;
}

void VideoRenderer::updateFrame(const VideoFrame& frame) {
    // In a real implementation, we would upload pixels into GPU texture.
    // Here we mock by storing original size and updating timestamp
    (void)frame;
}

void VideoRenderer::render() {
    if (!visible) return;
    // Mock rendering by printing
    std::cout << "VideoRenderer::render at (" << position.x << "," << position.y << ") size "
              << displaySize.x << "x" << displaySize.y << " alpha=" << alpha << std::endl;
}

void VideoRenderer::calculateDisplaySize() {
    if (originalSize.x <= 0 || originalSize.y <= 0) {
        displaySize = size;
        return;
    }
    Math::Vector2D target = size;
    switch (scalingMode) {
        case VideoScalingMode::Stretch:
            displaySize = target;
            break;
        case VideoScalingMode::AspectFit:
            displaySize = VideoUtils::calculateAspectFitSize(originalSize, target);
            break;
        case VideoScalingMode::AspectFill:
            displaySize = VideoUtils::calculateAspectFillSize(originalSize, target);
            break;
        case VideoScalingMode::Crop:
        default:
            displaySize = target;
            break;
    }
}

void VideoRenderer::updateTexture(const VideoFrame& frame) {
    // Convert frame pixels -> texture. Placeholder.
    (void)frame;
}

// -- VideoControls minimal implementation
VideoControls::VideoControls()
    : visible(true), autoHide(true), autoHideDelay(std::chrono::milliseconds(3000)),
      isDraggingSeekBar(false), seekBarPosition(0.0f), volume(1.0f), muted(false) {
    resetAutoHideTimer();
}

void VideoControls::setAutoHide(bool autoHide_, std::chrono::milliseconds delay) {
    autoHide = autoHide_;
    autoHideDelay = delay;
}

void VideoControls::resetAutoHideTimer() { lastActivity = std::chrono::high_resolution_clock::now(); }

void VideoControls::update(float deltaTime) {
    (void)deltaTime;
    if (!visible) return;
    if (!autoHide) return;
    auto now = std::chrono::high_resolution_clock::now();
    if (now - lastActivity > autoHideDelay) {
        visible = false;
    }
}

void VideoControls::render() {
    if (!visible) return;
    std::cout << "Rendering video controls (seek=" << seekBarPosition << ", vol=" << volume << ")" << std::endl;
}

bool VideoControls::handleInput() { return false; }

void VideoControls::updatePlaybackTime(double currentTime, double duration) {
    if (duration <= 0.0) { seekBarPosition = 0.0f; return; }
    seekBarPosition = static_cast<float>(currentTime / duration);
}

void VideoControls::setVolume(float vol) { volume = std::clamp(vol, 0.0f, 1.0f); if (callbacks.onVolumeChange) callbacks.onVolumeChange(volume); }

void VideoControls::setMuted(bool mute) { muted = mute; if (callbacks.onToggleMute) callbacks.onToggleMute(); }

// -- VideoPlayer simplified mock implementation
VideoPlayer::VideoPlayer()
    : decoder(nullptr), buffer(std::make_unique<VideoBuffer>()), renderer(std::make_unique<VideoRenderer>()),
      controls(std::make_unique<VideoControls>()), state(VideoState::Stopped), playbackTime(0.0), videoDuration(0.0),
      playbackSpeed(1.0f), volume(1.0f), muted(false), looping(false), decodingActive(false), shouldStop(false), pausedDuration(0.0) {
    setupControls();
}

VideoPlayer::~VideoPlayer() { unloadVideo(); }

bool VideoPlayer::loadVideo(const std::string& filename) {
    unloadVideo();
    currentFile = filename;
    // For now we use a mocked "decoder" that simulates frames.
    decoder = nullptr; // No real decoder implemented here in mock
    // Simulate video info
    VideoInfo info;
    info.filename = filename;
    info.format = Video::VideoFormat::MP4;
    info.hasVideo = true;
    info.hasAudio = false;
    info.width = 640;
    info.height = 360;
    info.frameRate = 30.0f;
    info.duration = 10.0f; // 10 seconds
    videoDuration = info.duration;
    
    renderer->initialize(info.width, info.height);
    
    setState(VideoState::Stopped);
    return true;
}

void VideoPlayer::unloadVideo() {
    stopDecoding();
    if (renderer) renderer->shutdown();
    buffer->clear();
    decoder = nullptr;
    currentFile.clear();
}

bool VideoPlayer::play() {
    if (state == VideoState::Playing) return true;
    setState(VideoState::Playing);
    startDecoding();
    playbackStartTime = std::chrono::high_resolution_clock::now();
    return true;
}

void VideoPlayer::pause() {
    if (state != VideoState::Playing) return;
    setState(VideoState::Paused);
    pauseTime = std::chrono::high_resolution_clock::now();
    stopDecoding();
}

void VideoPlayer::stop() {
    setState(VideoState::Stopped);
    playbackTime = 0.0;
    stopDecoding();
    buffer->clear();
}

void VideoPlayer::seek(double time) {
    playbackTime = std::clamp(time, 0.0, videoDuration.load());
    if (controls) controls->updatePlaybackTime(playbackTime, videoDuration);
}

void VideoPlayer::setVolume(float vol) { volume = std::clamp(vol, 0.0f, 1.0f); }
void VideoPlayer::setMuted(bool mute) { muted = mute; }

void VideoPlayer::setPlaybackSpeed(float speed) { playbackSpeed = std::max(0.1f, speed); }

const VideoInfo* VideoPlayer::getVideoInfo() const { return (decoder) ? &decoder->getVideoInfo() : nullptr; }

void VideoPlayer::update(float deltaTime) {
    (void)deltaTime;
    // Pop a frame if available and render
    std::unique_ptr<VideoFrame> frame;
    if (buffer->popVideoFrame(frame, std::chrono::milliseconds(1))) {
        // Update renderer with frame
        renderer->updateFrame(*frame);
        playbackTime = frame->timestamp;
        if (controls) controls->updatePlaybackTime(playbackTime, videoDuration);
    }
    if (controls) controls->update(deltaTime);
    if (onTimeChanged) onTimeChanged(playbackTime);
}

void VideoPlayer::render() {
    if (renderer) renderer->render();
    if (controls && controls->isVisible()) controls->render();
}

bool VideoPlayer::handleInput() {
    // Hook controls input handling
    return false;
}

void VideoPlayer::setState(VideoState newState) {
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        state = newState;
    }
    if (onStateChanged) onStateChanged(newState);
}

void VideoPlayer::startDecoding() {
    if (decodingActive) return;
    decodingActive = true;
    shouldStop = false;
    decodingThread = std::thread(&VideoPlayer::decodingLoop, this);
}

void VideoPlayer::stopDecoding() {
    if (!decodingActive) return;
    shouldStop = true;
    if (decodingThread.joinable()) decodingThread.join();
    shouldStop = false;
    decodingActive = false;
}

void VideoPlayer::decodingLoop() {
    // Mock decoding loop: produce frames at fixed frame rate
    double fps = 30.0;
    double frameDuration = 1.0 / fps;
    double t = playbackTime;
    while (!shouldStop && t < videoDuration) {
        // Create a mock VideoFrame
        auto frame = std::make_unique<VideoFrame>();
        frame->width = 640;
        frame->height = 360;
        frame->timestamp = t;
        frame->pitch = frame->width * 4;
        frame->format = 0; // placeholder
        size_t pixelBytes = frame->pitch * frame->height;
        frame->pixels = std::malloc(pixelBytes);
        if (frame->pixels) std::memset(frame->pixels, (int)(fmod(t,1.0)*255), pixelBytes);

        // Push to buffer (block if full)
        while (!buffer->pushVideoFrame(std::move(frame))) {
            if (shouldStop) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        t += frameDuration * playbackSpeed.load();
        playbackTime = t;
        std::this_thread::sleep_for(std::chrono::duration<double>(frameDuration / playbackSpeed));
    }
    // End of file
    setState(VideoState::EndOfFile);
    if (onEndOfFile) onEndOfFile();
}

void VideoPlayer::updatePlaybackTime() {
    // In real implementation we would calculate based on audio clock or system clock
}

void VideoPlayer::synchronizeFrames() {}
bool VideoPlayer::isVideoFrameReady(double targetTime) const { (void)targetTime; return true; }
bool VideoPlayer::isAudioFrameReady(double targetTime) const { (void)targetTime; return true; }

void VideoPlayer::setupControls() {
    VideoControls::ControlCallbacks cb;
    cb.onPlay = [this]() { onControlPlay(); };
    cb.onPause = [this]() { onControlPause(); };
    cb.onStop = [this]() { onControlStop(); };
    cb.onSeek = [this](double t) { onControlSeek(t); };
    cb.onVolumeChange = [this](float v) { onControlVolumeChange(v); };
    cb.onToggleMute = [this]() { onControlToggleMute(); };
    controls->setCallbacks(cb);
}

void VideoPlayer::onControlPlay() { play(); }
void VideoPlayer::onControlPause() { pause(); }
void VideoPlayer::onControlStop() { stop(); }
void VideoPlayer::onControlSeek(double time) { seek(time); }
void VideoPlayer::onControlVolumeChange(float vol) { setVolume(vol); }
void VideoPlayer::onControlToggleMute() { setMuted(!isMuted()); }

// -- VideoManager implementation
VideoManager* VideoManager::instance = nullptr;

VideoManager::VideoManager() : initialized(false) {}

VideoManager* VideoManager::getInstance() {
    if (!instance) instance = new VideoManager();
    return instance;
}

VideoManager::~VideoManager() { shutdown(); }

void VideoManager::initialize() {
    if (initialized) return;
    initialized = true;
}

void VideoManager::shutdown() {
    std::lock_guard<std::mutex> lock(playersMutex);
    players.clear();
    initialized = false;
}

VideoPlayer* VideoManager::createPlayer() {
    std::lock_guard<std::mutex> lock(playersMutex);
    players.emplace_back(std::make_unique<VideoPlayer>());
    return players.back().get();
}

void VideoManager::destroyPlayer(VideoPlayer* player) {
    std::lock_guard<std::mutex> lock(playersMutex);
    auto it = std::remove_if(players.begin(), players.end(), [&](const std::unique_ptr<VideoPlayer>& p){ return p.get() == player; });
    if (it != players.end()) players.erase(it, players.end());
}

void VideoManager::update(float deltaTime) {
    std::lock_guard<std::mutex> lock(playersMutex);
    for (auto& p : players) p->update(deltaTime);
}

void VideoManager::render() {
    std::lock_guard<std::mutex> lock(playersMutex);
    for (auto& p : players) p->render();
}

void VideoManager::setGlobalVolume(float /*volume*/) {}
float VideoManager::getGlobalVolume() const { return 1.0f; }

void VideoManager::pauseAll() {
    std::lock_guard<std::mutex> lock(playersMutex);
    for (auto& p : players) if (p->getState() == VideoState::Playing) p->pause();
}

void VideoManager::resumeAll() {
    std::lock_guard<std::mutex> lock(playersMutex);
    for (auto& p : players) if (p->getState() == VideoState::Paused) p->play();
}

void VideoManager::stopAll() {
    std::lock_guard<std::mutex> lock(playersMutex);
    for (auto& p : players) p->stop();
}

size_t VideoManager::getPlayerCount() const { std::lock_guard<std::mutex> lock(playersMutex); return players.size(); }

VideoFormat VideoManager::detectVideoFormat(const std::string& filename) {
    std::string ext;
    size_t dot = filename.find_last_of('.');
    if (dot != std::string::npos) ext = filename.substr(dot+1);
    for (auto & c : ext) c = (char)std::tolower(c);
    if (ext == "mp4") return VideoFormat::MP4;
    if (ext == "webm") return VideoFormat::WebM;
    if (ext == "mov") return VideoFormat::MOV;
    if (ext == "avi") return VideoFormat::AVI;
    return VideoFormat::Unknown;
}

std::string VideoManager::formatTimeString(double seconds) {
    int s = (int)std::floor(seconds);
    int h = s / 3600;
    int m = (s % 3600) / 60;
    int sec = s % 60;
    char buf[64];
    if (h > 0) std::sprintf(buf, "%d:%02d:%02d", h, m, sec);
    else std::sprintf(buf, "%02d:%02d", m, sec);
    return std::string(buf);
}

bool VideoManager::isVideoFile(const std::string& filename) {
    return detectVideoFormat(filename) != VideoFormat::Unknown;
}

// -- VideoUtils implementations
Math::Vector2D VideoUtils::calculateAspectFitSize(const Math::Vector2D& sourceSize, const Math::Vector2D& targetSize) {
    if (sourceSize.x == 0 || sourceSize.y == 0) return targetSize;
    float scale = std::min(targetSize.x / sourceSize.x, targetSize.y / sourceSize.y);
    return Math::Vector2D(sourceSize.x * scale, sourceSize.y * scale);
}

Math::Vector2D VideoUtils::calculateAspectFillSize(const Math::Vector2D& sourceSize, const Math::Vector2D& targetSize) {
    if (sourceSize.x == 0 || sourceSize.y == 0) return targetSize;
    float scale = std::max(targetSize.x / sourceSize.x, targetSize.y / sourceSize.y);
    return Math::Vector2D(sourceSize.x * scale, sourceSize.y * scale);
}

bool VideoUtils::convertPixelFormat(const VideoFrame& /*source*/, VideoFrame& /*target*/, int /*targetFormat*/) { return false; }
void VideoUtils::resizeFrame(const VideoFrame& /*source*/, VideoFrame& /*target*/, int /*newWidth*/, int /*newHeight*/) {}

std::string VideoUtils::getVideoFormatName(VideoFormat format) {
    switch (format) {
        case VideoFormat::MP4: return "MP4";
        case VideoFormat::AVI: return "AVI";
        case VideoFormat::MOV: return "MOV";
        case VideoFormat::WebM: return "WebM";
        default: return "Unknown";
    }
}

std::string VideoUtils::getVideoCodecName(VideoCodec codec) {
    switch (codec) {
        case VideoCodec::H264: return "H.264";
        case VideoCodec::VP9: return "VP9";
        default: return "Unknown";
    }
}

std::string VideoUtils::getAudioCodecName(AudioCodec codec) {
    switch (codec) {
        case AudioCodec::AAC: return "AAC";
        case AudioCodec::MP3: return "MP3";
        default: return "Unknown";
    }
}

} // namespace Video
} // namespace JJM
