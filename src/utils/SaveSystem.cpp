#include "utils/SaveSystem.h"
#include <iostream>
#include <sstream>
#include <filesystem>

namespace JJM {
namespace Utils {

// SaveData implementation
SaveData::SaveData() {
}

void SaveData::setInt(const std::string& key, int value) {
    data[key] = value;
}

void SaveData::setFloat(const std::string& key, float value) {
    data[key] = value;
}

void SaveData::setBool(const std::string& key, bool value) {
    data[key] = value;
}

void SaveData::setString(const std::string& key, const std::string& value) {
    data[key] = value;
}

int SaveData::getInt(const std::string& key, int defaultValue) const {
    auto it = data.find(key);
    if (it != data.end() && std::holds_alternative<int>(it->second)) {
        return std::get<int>(it->second);
    }
    return defaultValue;
}

float SaveData::getFloat(const std::string& key, float defaultValue) const {
    auto it = data.find(key);
    if (it != data.end() && std::holds_alternative<float>(it->second)) {
        return std::get<float>(it->second);
    }
    return defaultValue;
}

bool SaveData::getBool(const std::string& key, bool defaultValue) const {
    auto it = data.find(key);
    if (it != data.end() && std::holds_alternative<bool>(it->second)) {
        return std::get<bool>(it->second);
    }
    return defaultValue;
}

std::string SaveData::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = data.find(key);
    if (it != data.end() && std::holds_alternative<std::string>(it->second)) {
        return std::get<std::string>(it->second);
    }
    return defaultValue;
}

bool SaveData::hasKey(const std::string& key) const {
    return data.find(key) != data.end();
}

void SaveData::remove(const std::string& key) {
    data.erase(key);
}

void SaveData::clear() {
    data.clear();
}

std::vector<std::string> SaveData::getKeys() const {
    std::vector<std::string> keys;
    for (const auto& pair : data) {
        keys.push_back(pair.first);
    }
    return keys;
}

// SaveSystem implementation
SaveSystem* SaveSystem::instance = nullptr;

SaveSystem::SaveSystem() : savePath("saves/") {
}

SaveSystem* SaveSystem::getInstance() {
    if (!instance) {
        instance = new SaveSystem();
    }
    return instance;
}

SaveSystem::~SaveSystem() {
}

void SaveSystem::setSavePath(const std::string& path) {
    savePath = path;
    if (!savePath.empty() && savePath.back() != '/') {
        savePath += '/';
    }
}

bool SaveSystem::save(const std::string& filename) {
    std::string fullPath = getFullPath(filename);
    return writeSaveFile(fullPath);
}

bool SaveSystem::load(const std::string& filename) {
    std::string fullPath = getFullPath(filename);
    return readSaveFile(fullPath);
}

bool SaveSystem::saveToSlot(int slot) {
    std::string filename = "save_" + std::to_string(slot) + ".sav";
    return save(filename);
}

bool SaveSystem::loadFromSlot(int slot) {
    std::string filename = "save_" + std::to_string(slot) + ".sav";
    return load(filename);
}

bool SaveSystem::deleteSave(const std::string& filename) {
    std::string fullPath = getFullPath(filename);
    try {
        return std::filesystem::remove(fullPath);
    } catch (const std::exception& e) {
        std::cerr << "Error deleting save: " << e.what() << std::endl;
        return false;
    }
}

bool SaveSystem::saveExists(const std::string& filename) const {
    std::string fullPath = getFullPath(filename);
    return std::filesystem::exists(fullPath);
}

std::vector<std::string> SaveSystem::listSaves() const {
    std::vector<std::string> saves;
    
    try {
        if (std::filesystem::exists(savePath)) {
            for (const auto& entry : std::filesystem::directory_iterator(savePath)) {
                if (entry.is_regular_file()) {
                    saves.push_back(entry.path().filename().string());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error listing saves: " << e.what() << std::endl;
    }
    
    return saves;
}

std::string SaveSystem::getFullPath(const std::string& filename) const {
    return savePath + filename;
}

bool SaveSystem::writeSaveFile(const std::string& filepath) {
    // Create directory if it doesn't exist
    try {
        std::filesystem::path path(filepath);
        std::filesystem::create_directories(path.parent_path());
    } catch (const std::exception& e) {
        std::cerr << "Error creating save directory: " << e.what() << std::endl;
        return false;
    }
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open save file for writing: " << filepath << std::endl;
        return false;
    }
    
    // Write data
    for (const auto& pair : currentSave.data) {
        file << pair.first << "=";
        
        std::visit([&file](auto&& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, int>) {
                file << "int:" << value;
            } else if constexpr (std::is_same_v<T, float>) {
                file << "float:" << value;
            } else if constexpr (std::is_same_v<T, bool>) {
                file << "bool:" << (value ? "1" : "0");
            } else if constexpr (std::is_same_v<T, std::string>) {
                file << "string:" << value;
            }
        }, pair.second);
        
        file << "\n";
    }
    
    file.close();
    return true;
}

bool SaveSystem::readSaveFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open save file for reading: " << filepath << std::endl;
        return false;
    }
    
    currentSave.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        size_t equalPos = line.find('=');
        if (equalPos == std::string::npos) continue;
        
        std::string key = line.substr(0, equalPos);
        std::string valueStr = line.substr(equalPos + 1);
        
        size_t colonPos = valueStr.find(':');
        if (colonPos == std::string::npos) continue;
        
        std::string type = valueStr.substr(0, colonPos);
        std::string value = valueStr.substr(colonPos + 1);
        
        if (type == "int") {
            currentSave.setInt(key, std::stoi(value));
        } else if (type == "float") {
            currentSave.setFloat(key, std::stof(value));
        } else if (type == "bool") {
            currentSave.setBool(key, value == "1");
        } else if (type == "string") {
            currentSave.setString(key, value);
        }
    }
    
    file.close();
    return true;
}

} // namespace Utils
} // namespace JJM
