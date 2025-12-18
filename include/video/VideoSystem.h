#ifndef VIDEO_SYSTEM_H
#define VIDEO_SYSTEM_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <chrono>
#include "math/Vector2D.h"
#include "graphics/Texture.h"

namespace JJM {
namespace Video {

enum class VideoFormat {
    Unknown,
    MP4,
    AVI,
    MOV,
    WMV,
    WebM,
    OGV
};

enum class VideoCodec {
    Unknown,
    H264,
    H265,
    VP8,
    VP9,
    AV1,
    MPEG4
};

enum class AudioCodec {
    Unknown,
    AAC,
    MP3,
    Vorbis,
    Opus,
    PCM
};

enum class VideoState {
    Stopped,
    Playing,
    Paused,
    Buffering,
    Seeking,
    Error,
    EndOfFile
};

enum class VideoScalingMode {
    Stretch,
    AspectFit,
    AspectFill,
    Crop
};

struct VideoInfo {
    std::string filename;
    VideoFormat format;
    VideoCodec videoCodec;
    AudioCodec audioCodec;
    
    int width;
    int height;
    float frameRate;
    float duration;
    int bitrate;
    
    bool hasVideo;
    bool hasAudio;
    int audioChannels;
    int audioSampleRate;
    
    VideoInfo() : format(VideoFormat::Unknown), videoCodec(VideoCodec::Unknown),
                  audioCodec(AudioCodec::Unknown), width(0), height(0), 
                  frameRate(0.0f), duration(0.0f), bitrate(0),
                  hasVideo(false), hasAudio(false), 
                  audioChannels(0), audioSampleRate(0) {}
};

struct VideoFrame {
    void* pixels;
    int width;
    int height;
    int pitch;
    double timestamp;
    int format; // Pixel format (RGB, YUV, etc.)
    
    VideoFrame() : pixels(nullptr), width(0), height(0), 
                   pitch(0), timestamp(0.0), format(0) {}
    
    ~VideoFrame() {
        if (pixels) {
            std::free(pixels);
        }
    }
};

struct AudioFrame {
    void* samples;
    int sampleCount;
    int channels;
    int sampleRate;
    double timestamp;
    int format; // Audio format (16-bit, float, etc.)
    
    AudioFrame() : samples(nullptr), sampleCount(0), channels(0),
                   sampleRate(0), timestamp(0.0), format(0) {}
    
    ~AudioFrame() {
        if (samples) {
            std::free(samples);
        }
    }
};

class VideoDecoder {
protected:
    std::string filename;
    VideoInfo videoInfo;
    bool initialized;
    
    std::atomic<double> currentTime;
    std::atomic<double> seekTarget;
    std::atomic<bool> seekRequested;
    
public:
    VideoDecoder();
    virtual ~VideoDecoder() = default;
    
    virtual bool initialize(const std::string& filename) = 0;
    virtual void shutdown() = 0;
    
    virtual bool readVideoFrame(std::unique_ptr<VideoFrame>& frame) = 0;
    virtual bool readAudioFrame(std::unique_ptr<AudioFrame>& frame) = 0;
    
    virtual void seek(double time) = 0;
    virtual double getCurrentTime() const { return currentTime; }
    
    const VideoInfo& getVideoInfo() const { return videoInfo; }
    bool isInitialized() const { return initialized; }
    
protected:
    virtual bool openFile(const std::string& filename) = 0;
    virtual void closeFile() = 0;
    virtual bool parseVideoInfo() = 0;
};

class FFmpegDecoder : public VideoDecoder {
private:
    void* formatContext;
    void* videoCodecContext;
    void* audioCodecContext;
    void* videoFrame;
    void* audioFrame;
    void* swsContext;
    void* swrContext;
    
    int videoStreamIndex;
    int audioStreamIndex;
    
    std::vector<uint8_t> videoBuffer;
    std::vector<uint8_t> audioBuffer;
    
public:
    FFmpegDecoder();
    ~FFmpegDecoder() override;
    
    bool initialize(const std::string& filename) override;
    void shutdown() override;
    
    bool readVideoFrame(std::unique_ptr<VideoFrame>& frame) override;
    bool readAudioFrame(std::unique_ptr<AudioFrame>& frame) override;
    
    void seek(double time) override;
    
protected:
    bool openFile(const std::string& filename) override;
    void closeFile() override;
    bool parseVideoInfo() override;
    
private:
    bool initializeVideoDecoder();
    bool initializeAudioDecoder();
    bool convertVideoFrame(void* srcFrame, std::unique_ptr<VideoFrame>& dstFrame);
    bool convertAudioFrame(void* srcFrame, std::unique_ptr<AudioFrame>& dstFrame);
};

class VideoBuffer {
private:
    std::queue<std::unique_ptr<VideoFrame>> videoFrames;
    std::queue<std::unique_ptr<AudioFrame>> audioFrames;
    
    std::mutex videoMutex;
    std::mutex audioMutex;
    std::condition_variable videoCV;
    std::condition_variable audioCV;
    
    size_t maxVideoFrames;
    size_t maxAudioFrames;
    std::atomic<size_t> videoFrameCount;
    std::atomic<size_t> audioFrameCount;
    
public:
    VideoBuffer(size_t maxVideoFrames = 30, size_t maxAudioFrames = 100);
    
    bool pushVideoFrame(std::unique_ptr<VideoFrame> frame);
    bool popVideoFrame(std::unique_ptr<VideoFrame>& frame, std::chrono::milliseconds timeout);
    
    bool pushAudioFrame(std::unique_ptr<AudioFrame> frame);
    bool popAudioFrame(std::unique_ptr<AudioFrame>& frame, std::chrono::milliseconds timeout);
    
    void clear();
    
    size_t getVideoFrameCount() const { return videoFrameCount; }
    size_t getAudioFrameCount() const { return audioFrameCount; }
    
    bool isVideoBufferFull() const { return videoFrameCount >= maxVideoFrames; }
    bool isAudioBufferFull() const { return audioFrameCount >= maxAudioFrames; }
    
    float getVideoBufferUsage() const { return static_cast<float>(videoFrameCount) / maxVideoFrames; }
    float getAudioBufferUsage() const { return static_cast<float>(audioFrameCount) / maxAudioFrames; }
};

class VideoRenderer {
private:
    Graphics::Texture* videoTexture;
    Math::Vector2D position;
    Math::Vector2D size;
    VideoScalingMode scalingMode;
    float alpha;
    bool visible;
    
    Math::Vector2D originalSize;
    Math::Vector2D displaySize;
    Math::Vector2D renderOffset;
    
public:
    VideoRenderer();
    ~VideoRenderer();
    
    void initialize(int width, int height);
    void shutdown();
    
    void updateFrame(const VideoFrame& frame);
    void render();
    
    void setPosition(const Math::Vector2D& pos) { position = pos; calculateDisplaySize(); }
    void setSize(const Math::Vector2D& sz) { size = sz; calculateDisplaySize(); }
    void setScalingMode(VideoScalingMode mode) { scalingMode = mode; calculateDisplaySize(); }
    void setAlpha(float a) { alpha = std::clamp(a, 0.0f, 1.0f); }
    void setVisible(bool vis) { visible = vis; }
    
    const Math::Vector2D& getPosition() const { return position; }
    const Math::Vector2D& getSize() const { return size; }
    const Math::Vector2D& getDisplaySize() const { return displaySize; }
    VideoScalingMode getScalingMode() const { return scalingMode; }
    float getAlpha() const { return alpha; }
    bool isVisible() const { return visible; }
    
private:
    void calculateDisplaySize();
    void updateTexture(const VideoFrame& frame);
};

class VideoControls {
public:
    struct ControlCallbacks {
        std::function<void()> onPlay;
        std::function<void()> onPause;
        std::function<void()> onStop;
        std::function<void(double)> onSeek;
        std::function<void(float)> onVolumeChange;
        std::function<void()> onToggleMute;
        std::function<void()> onToggleFullscreen;
    };
    
private:
    bool visible;
    bool autoHide;
    std::chrono::high_resolution_clock::time_point lastActivity;
    std::chrono::milliseconds autoHideDelay;
    
    Math::Vector2D controlsPosition;
    Math::Vector2D controlsSize;
    
    bool isDraggingSeekBar;
    float seekBarPosition;
    float volume;
    bool muted;
    
    ControlCallbacks callbacks;
    
public:
    VideoControls();
    
    void setVisible(bool vis) { visible = vis; }
    bool isVisible() const { return visible; }
    
    void setAutoHide(bool autoHide, std::chrono::milliseconds delay = std::chrono::milliseconds(3000));
    void resetAutoHideTimer();
    
    void setPosition(const Math::Vector2D& pos) { controlsPosition = pos; }
    void setSize(const Math::Vector2D& size) { controlsSize = size; }
    
    void setCallbacks(const ControlCallbacks& cb) { callbacks = cb; }
    
    void update(float deltaTime);
    void render();
    bool handleInput(); // Returns true if input was consumed
    
    void updatePlaybackTime(double currentTime, double duration);
    void setVolume(float vol);
    float getVolume() const { return volume; }
    
    void setMuted(bool mute);
    bool isMuted() const { return muted; }
    
private:
    void renderPlayPauseButton();
    void renderSeekBar();
    void renderVolumeControls();
    void renderTimeDisplay();
    void renderFullscreenButton();
    
    bool isPointInSeekBar(const Math::Vector2D& point) const;
    float getSeekBarProgress(const Math::Vector2D& point) const;
};

class VideoPlayer {
private:
    std::unique_ptr<VideoDecoder> decoder;
    std::unique_ptr<VideoBuffer> buffer;
    std::unique_ptr<VideoRenderer> renderer;
    std::unique_ptr<VideoControls> controls;
    
    VideoState state;
    std::string currentFile;
    
    std::thread decodingThread;
    std::atomic<bool> decodingActive;
    std::atomic<bool> shouldStop;
    
    std::atomic<double> playbackTime;
    std::atomic<double> videoDuration;
    std::atomic<float> playbackSpeed;
    std::atomic<float> volume;
    std::atomic<bool> muted;
    std::atomic<bool> looping;
    
    std::chrono::high_resolution_clock::time_point playbackStartTime;
    std::chrono::high_resolution_clock::time_point pauseTime;
    double pausedDuration;
    
    // Events
    std::function<void(VideoState)> onStateChanged;
    std::function<void(double)> onTimeChanged;
    std::function<void()> onEndOfFile;
    std::function<void(const std::string&)> onError;
    
    std::mutex stateMutex;
    
public:
    VideoPlayer();
    ~VideoPlayer();
    
    bool loadVideo(const std::string& filename);
    void unloadVideo();
    
    bool play();
    void pause();
    void stop();
    void seek(double time);
    
    void setVolume(float volume);
    float getVolume() const { return volume; }
    
    void setMuted(bool mute);
    bool isMuted() const { return muted; }
    
    void setLooping(bool loop) { looping = loop; }
    bool isLooping() const { return looping; }
    
    void setPlaybackSpeed(float speed);
    float getPlaybackSpeed() const { return playbackSpeed; }
    
    VideoState getState() const { return state; }
    double getCurrentTime() const { return playbackTime; }
    double getDuration() const { return videoDuration; }
    
    const VideoInfo* getVideoInfo() const;
    
    // Renderer control
    VideoRenderer* getRenderer() { return renderer.get(); }
    VideoControls* getControls() { return controls.get(); }
    
    // Event callbacks
    void setStateChangedCallback(std::function<void(VideoState)> callback) { onStateChanged = callback; }
    void setTimeChangedCallback(std::function<void(double)> callback) { onTimeChanged = callback; }
    void setEndOfFileCallback(std::function<void()> callback) { onEndOfFile = callback; }
    void setErrorCallback(std::function<void(const std::string&)> callback) { onError = callback; }
    
    void update(float deltaTime);
    void render();
    bool handleInput(); // Returns true if input was consumed
    
private:
    void setState(VideoState newState);
    void startDecoding();
    void stopDecoding();
    void decodingLoop();
    
    void updatePlaybackTime();
    void synchronizeFrames();
    
    bool isVideoFrameReady(double targetTime) const;
    bool isAudioFrameReady(double targetTime) const;
    
    void setupControls();
    void onControlPlay();
    void onControlPause();
    void onControlStop();
    void onControlSeek(double time);
    void onControlVolumeChange(float volume);
    void onControlToggleMute();
};

class VideoManager {
private:
    static VideoManager* instance;
    
    std::vector<std::unique_ptr<VideoPlayer>> players;
    std::mutex playersMutex;
    
    bool initialized;
    
    VideoManager();
    
public:
    static VideoManager* getInstance();
    ~VideoManager();
    
    void initialize();
    void shutdown();
    
    VideoPlayer* createPlayer();
    void destroyPlayer(VideoPlayer* player);
    
    void update(float deltaTime);
    void render();
    
    // Global settings
    void setGlobalVolume(float volume);
    float getGlobalVolume() const;
    
    void pauseAll();
    void resumeAll();
    void stopAll();
    
    size_t getPlayerCount() const;
    
    // Utility functions
    static VideoFormat detectVideoFormat(const std::string& filename);
    static std::string formatTimeString(double seconds);
    static bool isVideoFile(const std::string& filename);
};

// Video utilities and effects
namespace VideoUtils {
    struct VideoTransition {
        enum Type {
            None,
            Fade,
            Dissolve,
            Wipe,
            Slide
        };
        
        Type type;
        float duration;
        float progress;
        
        VideoTransition() : type(None), duration(1.0f), progress(0.0f) {}
    };
    
    class VideoEffect {
    public:
        virtual ~VideoEffect() = default;
        virtual void apply(VideoFrame& frame) = 0;
        virtual bool isEnabled() const = 0;
        virtual void setEnabled(bool enabled) = 0;
    };
    
    class ColorAdjustmentEffect : public VideoEffect {
    private:
        bool enabled;
        float brightness;
        float contrast;
        float saturation;
        float hue;
        
    public:
        ColorAdjustmentEffect();
        
        void apply(VideoFrame& frame) override;
        bool isEnabled() const override { return enabled; }
        void setEnabled(bool en) override { enabled = en; }
        
        void setBrightness(float b) { brightness = std::clamp(b, -1.0f, 1.0f); }
        void setContrast(float c) { contrast = std::clamp(c, 0.0f, 2.0f); }
        void setSaturation(float s) { saturation = std::clamp(s, 0.0f, 2.0f); }
        void setHue(float h) { hue = h; }
        
        float getBrightness() const { return brightness; }
        float getContrast() const { return contrast; }
        float getSaturation() const { return saturation; }
        float getHue() const { return hue; }
    };
    
    class BlurEffect : public VideoEffect {
    private:
        bool enabled;
        float radius;
        
    public:
        BlurEffect();
        
        void apply(VideoFrame& frame) override;
        bool isEnabled() const override { return enabled; }
        void setEnabled(bool en) override { enabled = en; }
        
        void setRadius(float r) { radius = std::clamp(r, 0.0f, 10.0f); }
        float getRadius() const { return radius; }
    };
    
    Math::Vector2D calculateAspectFitSize(const Math::Vector2D& sourceSize, const Math::Vector2D& targetSize);
    Math::Vector2D calculateAspectFillSize(const Math::Vector2D& sourceSize, const Math::Vector2D& targetSize);
    
    bool convertPixelFormat(const VideoFrame& source, VideoFrame& target, int targetFormat);
    void resizeFrame(const VideoFrame& source, VideoFrame& target, int newWidth, int newHeight);
    
    std::string getVideoFormatName(VideoFormat format);
    std::string getVideoCodecName(VideoCodec codec);
    std::string getAudioCodecName(AudioCodec codec);
}

} // namespace Video
} // namespace JJM

#endif // VIDEO_SYSTEM_H