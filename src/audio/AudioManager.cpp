#include "audio/AudioManager.h"
#include <iostream>

namespace JJM {
namespace Audio {

AudioManager* AudioManager::instance = nullptr;

AudioManager::AudioManager() 
    : initialized(false), musicVolume(MIX_MAX_VOLUME), sfxVolume(MIX_MAX_VOLUME) {}

AudioManager::~AudioManager() {
    shutdown();
}

AudioManager* AudioManager::getInstance() {
    if (!instance) {
        instance = new AudioManager();
    }
    return instance;
}

void AudioManager::destroy() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

bool AudioManager::initialize(int frequency, Uint16 format, int channels, int chunksize) {
    if (initialized) {
        std::cerr << "AudioManager already initialized!" << std::endl;
        return false;
    }
    
    if (Mix_OpenAudio(frequency, format, channels, chunksize) < 0) {
        std::cerr << "SDL_mixer initialization failed: " << Mix_GetError() << std::endl;
        return false;
    }
    
    initialized = true;
    std::cout << "AudioManager initialized successfully" << std::endl;
    return true;
}

void AudioManager::shutdown() {
    if (!initialized) return;
    
    unloadAll();
    Mix_CloseAudio();
    initialized = false;
}

bool AudioManager::loadMusic(const std::string& name, const std::string& filePath) {
    if (musicTracks.find(name) != musicTracks.end()) {
        std::cerr << "Music '" << name << "' already loaded!" << std::endl;
        return false;
    }
    
    Mix_Music* music = Mix_LoadMUS(filePath.c_str());
    if (!music) {
        std::cerr << "Failed to load music '" << filePath << "': " << Mix_GetError() << std::endl;
        return false;
    }
    
    musicTracks[name] = music;
    return true;
}

void AudioManager::playMusic(const std::string& name, int loops) {
    auto it = musicTracks.find(name);
    if (it == musicTracks.end()) {
        std::cerr << "Music '" << name << "' not found!" << std::endl;
        return;
    }
    
    if (Mix_PlayMusic(it->second, loops) < 0) {
        std::cerr << "Failed to play music: " << Mix_GetError() << std::endl;
    }
}

void AudioManager::pauseMusic() {
    Mix_PauseMusic();
}

void AudioManager::resumeMusic() {
    Mix_ResumeMusic();
}

void AudioManager::stopMusic() {
    Mix_HaltMusic();
}

void AudioManager::setMusicVolume(int volume) {
    musicVolume = std::max(0, std::min(MIX_MAX_VOLUME, volume));
    Mix_VolumeMusic(musicVolume);
}

bool AudioManager::isMusicPlaying() const {
    return Mix_PlayingMusic() != 0;
}

bool AudioManager::loadSoundEffect(const std::string& name, const std::string& filePath) {
    if (soundEffects.find(name) != soundEffects.end()) {
        std::cerr << "Sound effect '" << name << "' already loaded!" << std::endl;
        return false;
    }
    
    Mix_Chunk* chunk = Mix_LoadWAV(filePath.c_str());
    if (!chunk) {
        std::cerr << "Failed to load sound effect '" << filePath << "': " << Mix_GetError() << std::endl;
        return false;
    }
    
    soundEffects[name] = chunk;
    return true;
}

void AudioManager::playSoundEffect(const std::string& name, int loops) {
    auto it = soundEffects.find(name);
    if (it == soundEffects.end()) {
        std::cerr << "Sound effect '" << name << "' not found!" << std::endl;
        return;
    }
    
    if (Mix_PlayChannel(-1, it->second, loops) < 0) {
        std::cerr << "Failed to play sound effect: " << Mix_GetError() << std::endl;
    }
}

void AudioManager::setSFXVolume(int volume) {
    sfxVolume = std::max(0, std::min(MIX_MAX_VOLUME, volume));
    Mix_Volume(-1, sfxVolume);
}

void AudioManager::setSFXVolume(const std::string& name, int volume) {
    auto it = soundEffects.find(name);
    if (it == soundEffects.end()) {
        std::cerr << "Sound effect '" << name << "' not found!" << std::endl;
        return;
    }
    
    int vol = std::max(0, std::min(MIX_MAX_VOLUME, volume));
    Mix_VolumeChunk(it->second, vol);
}

void AudioManager::unloadMusic(const std::string& name) {
    auto it = musicTracks.find(name);
    if (it != musicTracks.end()) {
        Mix_FreeMusic(it->second);
        musicTracks.erase(it);
    }
}

void AudioManager::unloadSoundEffect(const std::string& name) {
    auto it = soundEffects.find(name);
    if (it != soundEffects.end()) {
        Mix_FreeChunk(it->second);
        soundEffects.erase(it);
    }
}

void AudioManager::unloadAll() {
    for (auto& pair : musicTracks) {
        Mix_FreeMusic(pair.second);
    }
    musicTracks.clear();
    
    for (auto& pair : soundEffects) {
        Mix_FreeChunk(pair.second);
    }
    soundEffects.clear();
}

} // namespace Audio
} // namespace JJM
