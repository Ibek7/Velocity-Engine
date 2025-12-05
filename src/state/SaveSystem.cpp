#include "state/SaveSystem.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace JJM {
namespace Save {

int SaveData::getInt(const std::string& key, int defaultValue) const {
    auto it = intData.find(key);
    return it != intData.end() ? it->second : defaultValue;
}

float SaveData::getFloat(const std::string& key, float defaultValue) const {
    auto it = floatData.find(key);
    return it != floatData.end() ? it->second : defaultValue;
}

std::string SaveData::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = stringData.find(key);
    return it != stringData.end() ? it->second : defaultValue;
}

bool SaveData::getBool(const std::string& key, bool defaultValue) const {
    auto it = boolData.find(key);
    return it != boolData.end() ? it->second : defaultValue;
}

void SaveData::clear() {
    intData.clear();
    floatData.clear();
    stringData.clear();
    boolData.clear();
}

SaveSystem::SaveSystem()
    : saveDirectory("saves/"), autoSaveEnabled(false), autoSaveInterval(60.0f), timeSinceLastSave(0.0f) {
}

SaveSystem& SaveSystem::getInstance() {
    static SaveSystem instance;
    return instance;
}

std::string SaveSystem::getSaveFilePath(const std::string& slotName) const {
    return saveDirectory + slotName + ".sav";
}

bool SaveSystem::save(const std::string& slotName, const SaveData& data) {
    std::filesystem::create_directories(saveDirectory);
    
    std::ofstream file(getSaveFilePath(slotName), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to save to slot: " << slotName << std::endl;
        return false;
    }
    
    // Write int data
    size_t size = data.intData.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& [key, value] : data.intData) {
        size_t keyLen = key.size();
        file.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
        file.write(key.c_str(), keyLen);
        file.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
    
    // Write float data
    size = data.floatData.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& [key, value] : data.floatData) {
        size_t keyLen = key.size();
        file.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
        file.write(key.c_str(), keyLen);
        file.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
    
    // Write string data
    size = data.stringData.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& [key, value] : data.stringData) {
        size_t keyLen = key.size();
        file.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
        file.write(key.c_str(), keyLen);
        size_t valueLen = value.size();
        file.write(reinterpret_cast<const char*>(&valueLen), sizeof(valueLen));
        file.write(value.c_str(), valueLen);
    }
    
    // Write bool data
    size = data.boolData.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& [key, value] : data.boolData) {
        size_t keyLen = key.size();
        file.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
        file.write(key.c_str(), keyLen);
        file.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
    
    std::cout << "Saved game to slot: " << slotName << std::endl;
    return true;
}

bool SaveSystem::load(const std::string& slotName, SaveData& data) {
    std::ifstream file(getSaveFilePath(slotName), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to load from slot: " << slotName << std::endl;
        return false;
    }
    
    data.clear();
    
    // Read int data
    size_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    for (size_t i = 0; i < size; ++i) {
        size_t keyLen;
        file.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));
        std::string key(keyLen, '\0');
        file.read(&key[0], keyLen);
        int value;
        file.read(reinterpret_cast<char*>(&value), sizeof(value));
        data.intData[key] = value;
    }
    
    // Read float data
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    for (size_t i = 0; i < size; ++i) {
        size_t keyLen;
        file.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));
        std::string key(keyLen, '\0');
        file.read(&key[0], keyLen);
        float value;
        file.read(reinterpret_cast<char*>(&value), sizeof(value));
        data.floatData[key] = value;
    }
    
    // Read string data
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    for (size_t i = 0; i < size; ++i) {
        size_t keyLen;
        file.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));
        std::string key(keyLen, '\0');
        file.read(&key[0], keyLen);
        size_t valueLen;
        file.read(reinterpret_cast<char*>(&valueLen), sizeof(valueLen));
        std::string value(valueLen, '\0');
        file.read(&value[0], valueLen);
        data.stringData[key] = value;
    }
    
    // Read bool data
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    for (size_t i = 0; i < size; ++i) {
        size_t keyLen;
        file.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));
        std::string key(keyLen, '\0');
        file.read(&key[0], keyLen);
        bool value;
        file.read(reinterpret_cast<char*>(&value), sizeof(value));
        data.boolData[key] = value;
    }
    
    std::cout << "Loaded game from slot: " << slotName << std::endl;
    return true;
}

bool SaveSystem::deleteSave(const std::string& slotName) {
    return std::filesystem::remove(getSaveFilePath(slotName));
}

std::vector<std::string> SaveSystem::listSaves() const {
    std::vector<std::string> saves;
    
    if (!std::filesystem::exists(saveDirectory)) {
        return saves;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(saveDirectory)) {
        if (entry.path().extension() == ".sav") {
            saves.push_back(entry.path().stem().string());
        }
    }
    
    return saves;
}

void SaveSystem::update(float deltaTime) {
    if (!autoSaveEnabled) return;
    
    timeSinceLastSave += deltaTime;
    if (timeSinceLastSave >= autoSaveInterval) {
        // Auto-save would happen here
        timeSinceLastSave = 0.0f;
    }
}

} // namespace Save
} // namespace JJM
