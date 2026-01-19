#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <SDL_mixer.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <queue>

namespace JJM {
namespace Audio {

// Audio fade types
enum class FadeType {
    None,
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut
};

// Predefined fade curves for common scenarios
struct FadeCurve {
    FadeType type;
    float duration;
    std::string name;
    
    FadeCurve(FadeType t = FadeType::Linear, float d = 1.0f, const std::string& n = "")
        : type(t), duration(d), name(n) {}
    
    // Preset fade curves
    static FadeCurve QuickFadeOut() { return FadeCurve(FadeType::EaseIn, 0.5f, "QuickFadeOut"); }
    static FadeCurve QuickFadeIn() { return FadeCurve(FadeType::EaseOut, 0.5f, "QuickFadeIn"); }
    static FadeCurve SmoothTransition() { return FadeCurve(FadeType::EaseInOut, 2.0f, "SmoothTransition"); }
    static FadeCurve DramaticFadeOut() { return FadeCurve(FadeType::EaseIn, 3.0f, "DramaticFadeOut"); }
    static FadeCurve SubtleFadeIn() { return FadeCurve(FadeType::EaseOut, 4.0f, "SubtleFadeIn"); }
    static FadeCurve InstantCut() { return FadeCurve(FadeType::None, 0.0f, "InstantCut"); }
    static FadeCurve CinematicFade() { return FadeCurve(FadeType::EaseInOut, 5.0f, "CinematicFade"); }
    static FadeCurve BattleTransition() { return FadeCurve(FadeType::Linear, 0.3f, "BattleTransition"); }
};

// Active fade operation
struct FadeOperation {
    std::string trackName;
    int targetVolume;
    int startVolume;
    float duration;
    float elapsed;
    FadeType type;
    std::function<void()> onComplete;
    bool isMusic;
};

// Audio channel information
struct ChannelInfo {
    std::string soundName;
    int volume;
    bool isLooping;
    float pan;  // -1.0 (left) to 1.0 (right)
};

class AudioManager {
private:
    static AudioManager* instance;
    bool initialized;
    
    std::unordered_map<std::string, Mix_Music*> musicTracks;
    std::unordered_map<std::string, Mix_Chunk*> soundEffects;
    
    int musicVolume;
    int sfxVolume;
    int masterVolume;
    
    // Fade system
    std::vector<FadeOperation> activeFades;
    
    // Channel management
    std::unordered_map<int, ChannelInfo> channelInfo;
    int maxChannels;
    
    // Current music state
    std::string currentMusicTrack;
    bool musicPaused;
    
    AudioManager();
    ~AudioManager();
    
public:
    // Singleton access
    static AudioManager* getInstance();
    static void destroy();
    
    // =========================================================================
    // Initialization
    // =========================================================================
    
    /**
     * @brief Initialize the audio system with specified parameters
     * @param frequency Sample rate in Hz (default: 44100)
     * @param format Audio format (default: MIX_DEFAULT_FORMAT for 16-bit stereo)
     * @param channels Number of output channels: 1=mono, 2=stereo (default: 2)
     * @param chunksize Audio buffer size in bytes (default: 2048)
     * @return true if initialization succeeded, false otherwise
     * @note Must be called before any other audio operations
     * @note Smaller chunksize reduces latency but increases CPU usage
     */
    bool initialize(int frequency = 44100, Uint16 format = MIX_DEFAULT_FORMAT,
                   int channels = 2, int chunksize = 2048);
    
    /**
     * @brief Shutdown the audio system and free all resources
     * @note Automatically called by destructor
     */
    void shutdown();
    
    // =========================================================================
    // Update
    // =========================================================================
    
    /**
     * @brief Update audio system for fade operations
     * @param deltaTime Time elapsed since last frame in seconds
     * @note Must be called each frame to process active fades
     */
    void update(float deltaTime);
    
    // =========================================================================
    // Music Control
    // =========================================================================
    
    /**
     * @brief Load a music track from file
     * @param name Unique identifier for this music track
     * @param filePath Path to audio file (supports MP3, OGG, WAV, FLAC, etc.)
     * @return true if loading succeeded, false otherwise
     * @note Music is streamed from disk, not loaded into memory entirely
     * @note Can reload existing tracks to update them
     */
    bool loadMusic(const std::string& name, const std::string& filePath);
    
    /**
     * @brief Play a loaded music track
     * @param name Name of the music track to play
     * @param loops Number of times to loop (-1 for infinite loop)
     * @note Stops any currently playing music
     * @note Use fadeInMusic() for smoother transitions
     */
    void playMusic(const std::string& name, int loops = -1);
    
    /**
     * @brief Pause the currently playing music
     * @note Use resumeMusic() to continue playback
     * @see resumeMusic
     */
    void pauseMusic();
    
    /**
     * @brief Resume paused music playback
     * @note Only works if music was previously paused
     * @see pauseMusic
     */
    void resumeMusic();
    
    /**
     * @brief Stop the currently playing music immediately
     * @note To fade out smoothly, use fadeOutMusic() instead
     * @see fadeOutMusic
     */
    void stopMusic();
    
    /**
     * @brief Set music volume
     * @param volume Volume level (0-128, where 128 is maximum)
     * @note Applies to all music playback
     * @note Affected by master volume setting
     */
    void setMusicVolume(int volume); // 0-128
    
    /**
     * @brief Check if music is currently playing
     * @return true if music is playing, false if stopped or paused
     */
    bool isMusicPlaying() const;
    
    /**
     * @brief Check if music is paused
     * @return true if music is paused, false otherwise
     */
    bool isMusicPaused() const { return musicPaused; }
    
    /**
     * @brief Get the name of currently playing music track
     * @return Name of current track, or empty string if none playing
     */
    const std::string& getCurrentMusicTrack() const { return currentMusicTrack; }
    
    // =========================================================================
    // Music Fade Transitions
    // =========================================================================
    
    /**
     * @brief Fade in music from silence over specified duration
     * @param name Name of music track to play
     * @param duration Fade duration in seconds
     * @param loops Number of times to loop (-1 for infinite)
     * @note Smoothly increases volume from 0 to current music volume
     */
    void fadeInMusic(const std::string& name, float duration, int loops = -1);
    
    /**
     * @brief Fade out currently playing music over specified duration
     * @param duration Fade duration in seconds
     * @param onComplete Optional callback to invoke when fade completes
     * @note Music is stopped automatically after fade completes
     */
    void fadeOutMusic(float duration, std::function<void()> onComplete = nullptr);
    
    /**
     * @brief Crossfade from current music to new track
     * @param name Name of music track to transition to
     * @param duration Crossfade duration in seconds
     * @param loops Number of times to loop new track (-1 for infinite)
     * @note Fades out current track while fading in new track simultaneously
     */
    void crossfadeMusic(const std::string& name, float duration, int loops = -1);
    
    /**
     * @brief Fade music to specific volume level
     * @param targetVolume Target volume (0-128)
     * @param duration Fade duration in seconds
     * @param type Fade curve type (Linear, EaseIn, EaseOut, EaseInOut)
     * @note Does not affect base music volume setting
     */
    void fadeToVolume(int targetVolume, float duration, FadeType type = FadeType::Linear);
    
    /**
     * @brief Fade in music using predefined curve preset
     * @param name Name of music track to play
     * @param curve Predefined fade curve (e.g., FadeCurve::QuickFadeIn())
     * @param loops Number of times to loop (-1 for infinite)
     */
    void fadeInMusic(const std::string& name, const FadeCurve& curve, int loops = -1);
    
    /**
     * @brief Fade out music using predefined curve preset
     * @param curve Predefined fade curve (e.g., FadeCurve::DramaticFadeOut())
     * @param onComplete Optional callback to invoke when fade completes
     */
    void fadeOutMusic(const FadeCurve& curve, std::function<void()> onComplete = nullptr);
    
    /**
     * @brief Crossfade to new track using predefined curve preset
     * @param name Name of music track to transition to
     * @param curve Predefined fade curve (e.g., FadeCurve::SmoothTransition())
     * @param loops Number of times to loop new track (-1 for infinite)
     */
    void crossfadeMusic(const std::string& name, const FadeCurve& curve, int loops = -1);
    
    // Sound effects
    bool loadSoundEffect(const std::string& name, const std::string& filePath);
    int playSoundEffect(const std::string& name, int loops = 0);
    int playSoundEffectPanned(const std::string& name, float pan, int loops = 0);
    void setSFXVolume(int volume); // 0-128
    void setSFXVolume(const std::string& name, int volume);
    
    // Channel control
    void setChannelVolume(int channel, int volume);
    void fadeChannel(int channel, int targetVolume, float duration, FadeType type = FadeType::Linear);
    void stopChannel(int channel);
    void pauseChannel(int channel);
    void resumeChannel(int channel);
    bool isChannelPlaying(int channel) const;
    void setChannelPan(int channel, float pan);
    
    // Master volume
    void setMasterVolume(int volume);
    int getMasterVolume() const { return masterVolume; }
    void muteAll();
    void unmuteAll();
    
    // Cleanup
    void unloadMusic(const std::string& name);
    void unloadSoundEffect(const std::string& name);
    void unloadAll();
    
    // Query
    bool hasMusic(const std::string& name) const;
    bool hasSoundEffect(const std::string& name) const;
    int getMusicVolume() const { return musicVolume; }
    int getSFXVolume() const { return sfxVolume; }
    size_t getMusicCount() const { return musicTracks.size(); }
    size_t getSoundEffectCount() const { return soundEffects.size(); }
    
    // Delete copy constructor and assignment operator
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    
private:
    float applyFadeCurve(float t, FadeType type) const;
    void processFades(float deltaTime);
};

} // namespace Audio
} // namespace JJM

#endif // AUDIO_MANAGER_H
