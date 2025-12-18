#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <SDL_mixer.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace JJM {
namespace Audio {

class AudioManager {
private:
    static AudioManager* instance;
    bool initialized;
    
    std::unordered_map<std::string, Mix_Music*> musicTracks;
    std::unordered_map<std::string, Mix_Chunk*> soundEffects;
    
    int musicVolume;
    int sfxVolume;
    
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
    
    // Music control
    bool loadMusic(const std::string& name, const std::string& filePath);
    void playMusic(const std::string& name, int loops = -1);
    void pauseMusic();
    void resumeMusic();
    void stopMusic();
    void setMusicVolume(int volume); // 0-128
    bool isMusicPlaying() const;
    
    // Sound effects
    bool loadSoundEffect(const std::string& name, const std::string& filePath);
    void playSoundEffect(const std::string& name, int loops = 0);
    void setSFXVolume(int volume); // 0-128
    void setSFXVolume(const std::string& name, int volume);
    
    // Cleanup
    void unloadMusic(const std::string& name);
    void unloadSoundEffect(const std::string& name);
    void unloadAll();
    
    // Delete copy constructor and assignment operator
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
};

} // namespace Audio
} // namespace JJM

#endif // AUDIO_MANAGER_H
