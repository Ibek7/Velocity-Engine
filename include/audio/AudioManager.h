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
    
    // Initialization
    bool initialize(int frequency = 44100, Uint16 format = MIX_DEFAULT_FORMAT,
                   int channels = 2, int chunksize = 2048);
    void shutdown();
    
    // Update (call each frame for fades)
    void update(float deltaTime);
    
    // Music control
    bool loadMusic(const std::string& name, const std::string& filePath);
    void playMusic(const std::string& name, int loops = -1);
    void pauseMusic();
    void resumeMusic();
    void stopMusic();
    void setMusicVolume(int volume); // 0-128
    bool isMusicPlaying() const;
    bool isMusicPaused() const { return musicPaused; }
    const std::string& getCurrentMusicTrack() const { return currentMusicTrack; }
    
    // Music fade transitions
    void fadeInMusic(const std::string& name, float duration, int loops = -1);
    void fadeOutMusic(float duration, std::function<void()> onComplete = nullptr);
    void crossfadeMusic(const std::string& name, float duration, int loops = -1);
    void fadeToVolume(int targetVolume, float duration, FadeType type = FadeType::Linear);
    
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
