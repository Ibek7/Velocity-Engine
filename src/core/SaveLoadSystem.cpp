#include "core/SaveLoadSystem.h"
#include <fstream>
#include <ctime>
#include <algorithm>

namespace Engine {

// SaveFile implementation
SaveFile::SaveFile(const std::string& filename)
    : m_filename(filename)
{
    m_metadata.version = 1;
    m_metadata.playtime = 0.0f;
}

SaveFile::~SaveFile() {
}

void SaveFile::writeInt(const std::string& key, int value) {
    m_intData[key] = value;
}

void SaveFile::writeFloat(const std::string& key, float value) {
    m_floatData[key] = value;
}

void SaveFile::writeString(const std::string& key, const std::string& value) {
    m_stringData[key] = value;
}

void SaveFile::writeBool(const std::string& key, bool value) {
    m_boolData[key] = value;
}

void SaveFile::writeBytes(const std::string& key, const unsigned char* data, size_t size) {
    std::vector<unsigned char> bytes(data, data + size);
    m_bytesData[key] = bytes;
}

int SaveFile::readInt(const std::string& key, int defaultValue) const {
    auto it = m_intData.find(key);
    if (it != m_intData.end()) {
        return it->second;
    }
    return defaultValue;
}

float SaveFile::readFloat(const std::string& key, float defaultValue) const {
    auto it = m_floatData.find(key);
    if (it != m_floatData.end()) {
        return it->second;
    }
    return defaultValue;
}

std::string SaveFile::readString(const std::string& key, const std::string& defaultValue) const {
    auto it = m_stringData.find(key);
    if (it != m_stringData.end()) {
        return it->second;
    }
    return defaultValue;
}

bool SaveFile::readBool(const std::string& key, bool defaultValue) const {
    auto it = m_boolData.find(key);
    if (it != m_boolData.end()) {
        return it->second;
    }
    return defaultValue;
}

std::vector<unsigned char> SaveFile::readBytes(const std::string& key) const {
    auto it = m_bytesData.find(key);
    if (it != m_bytesData.end()) {
        return it->second;
    }
    return std::vector<unsigned char>();
}

void SaveFile::setMetadata(const SaveMetadata& metadata) {
    m_metadata = metadata;
}

bool SaveFile::hasKey(const std::string& key) const {
    return m_intData.find(key) != m_intData.end() ||
           m_floatData.find(key) != m_floatData.end() ||
           m_stringData.find(key) != m_stringData.end() ||
           m_boolData.find(key) != m_boolData.end() ||
           m_bytesData.find(key) != m_bytesData.end();
}

std::vector<std::string> SaveFile::getAllKeys() const {
    std::vector<std::string> keys;
    
    for (const auto& pair : m_intData) {
        keys.push_back(pair.first);
    }
    for (const auto& pair : m_floatData) {
        keys.push_back(pair.first);
    }
    for (const auto& pair : m_stringData) {
        keys.push_back(pair.first);
    }
    for (const auto& pair : m_boolData) {
        keys.push_back(pair.first);
    }
    for (const auto& pair : m_bytesData) {
        keys.push_back(pair.first);
    }
    
    return keys;
}

bool SaveFile::save(SaveFormat format) {
    // Set timestamp
    time_t now = time(nullptr);
    m_metadata.timestamp = std::string(ctime(&now));
    
    switch (format) {
        case SaveFormat::Binary:
            return saveBinary();
        case SaveFormat::JSON:
            return saveJSON();
        case SaveFormat::Compressed:
            return saveCompressed();
    }
    
    return false;
}

bool SaveFile::load() {
    // Try loading with different formats
    if (loadBinary()) return true;
    if (loadJSON()) return true;
    if (loadCompressed()) return true;
    
    return false;
}

void SaveFile::clear() {
    m_intData.clear();
    m_floatData.clear();
    m_stringData.clear();
    m_boolData.clear();
    m_bytesData.clear();
}

bool SaveFile::saveBinary() {
    std::ofstream file(m_filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Write metadata
    size_t nameLen = m_metadata.saveName.length();
    file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    file.write(m_metadata.saveName.c_str(), nameLen);
    
    file.write(reinterpret_cast<const char*>(&m_metadata.version), sizeof(m_metadata.version));
    file.write(reinterpret_cast<const char*>(&m_metadata.playtime), sizeof(m_metadata.playtime));
    
    // Write int data
    size_t intCount = m_intData.size();
    file.write(reinterpret_cast<const char*>(&intCount), sizeof(intCount));
    for (const auto& pair : m_intData) {
        size_t keyLen = pair.first.length();
        file.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
        file.write(pair.first.c_str(), keyLen);
        file.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second));
    }
    
    // Similar for other data types...
    
    file.close();
    return true;
}

bool SaveFile::saveJSON() {
    // TODO: Implement JSON serialization
    return false;
}

bool SaveFile::saveCompressed() {
    // TODO: Implement compression (e.g., zlib)
    return false;
}

bool SaveFile::loadBinary() {
    std::ifstream file(m_filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Read metadata
    size_t nameLen;
    file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    m_metadata.saveName.resize(nameLen);
    file.read(&m_metadata.saveName[0], nameLen);
    
    file.read(reinterpret_cast<char*>(&m_metadata.version), sizeof(m_metadata.version));
    file.read(reinterpret_cast<char*>(&m_metadata.playtime), sizeof(m_metadata.playtime));
    
    // Read int data
    size_t intCount;
    file.read(reinterpret_cast<char*>(&intCount), sizeof(intCount));
    for (size_t i = 0; i < intCount; ++i) {
        size_t keyLen;
        file.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));
        std::string key;
        key.resize(keyLen);
        file.read(&key[0], keyLen);
        
        int value;
        file.read(reinterpret_cast<char*>(&value), sizeof(value));
        m_intData[key] = value;
    }
    
    // Similar for other data types...
    
    file.close();
    return true;
}

bool SaveFile::loadJSON() {
    // TODO: Implement JSON deserialization
    return false;
}

bool SaveFile::loadCompressed() {
    // TODO: Implement decompression
    return false;
}

// SaveManager implementation
SaveManager::SaveManager()
    : m_saveDirectory("./saves/")
    , m_defaultFormat(SaveFormat::Binary)
    , m_autoSaveEnabled(false)
    , m_autoSaveInterval(300.0f)
    , m_autoSaveTimer(0.0f)
    , m_quickSave(nullptr)
    , m_currentSave(nullptr)
{
}

SaveManager& SaveManager::getInstance() {
    static SaveManager instance;
    return instance;
}

SaveFile* SaveManager::createSave(const std::string& saveName) {
    std::string filepath = getSaveFilePath(saveName);
    SaveFile* saveFile = new SaveFile(filepath);
    
    SaveMetadata metadata;
    metadata.saveName = saveName;
    saveFile->setMetadata(metadata);
    
    m_currentSave = saveFile;
    
    if (m_onSaveCreated) {
        m_onSaveCreated(saveName);
    }
    
    return saveFile;
}

SaveFile* SaveManager::loadSave(const std::string& saveName) {
    std::string filepath = getSaveFilePath(saveName);
    SaveFile* saveFile = new SaveFile(filepath);
    
    if (saveFile->load()) {
        m_currentSave = saveFile;
        
        if (m_onSaveLoaded) {
            m_onSaveLoaded(saveName);
        }
        
        return saveFile;
    }
    
    delete saveFile;
    return nullptr;
}

bool SaveManager::deleteSave(const std::string& saveName) {
    std::string filepath = getSaveFilePath(saveName);
    
    if (std::remove(filepath.c_str()) == 0) {
        if (m_onSaveDeleted) {
            m_onSaveDeleted(saveName);
        }
        return true;
    }
    
    return false;
}

bool SaveManager::saveExists(const std::string& saveName) const {
    std::string filepath = getSaveFilePath(saveName);
    std::ifstream file(filepath);
    return file.good();
}

void SaveManager::triggerAutoSave() {
    if (m_currentSave) {
        m_currentSave->save(m_defaultFormat);
    }
}

void SaveManager::update(float deltaTime) {
    if (!m_autoSaveEnabled) {
        return;
    }
    
    m_autoSaveTimer += deltaTime;
    
    if (m_autoSaveTimer >= m_autoSaveInterval) {
        triggerAutoSave();
        m_autoSaveTimer = 0.0f;
    }
}

void SaveManager::quickSave() {
    if (!m_quickSave) {
        m_quickSave = createSave("quicksave");
    }
    
    m_quickSave->save(m_defaultFormat);
}

void SaveManager::quickLoad() {
    if (saveExists("quicksave")) {
        m_quickSave = loadSave("quicksave");
    }
}

std::vector<std::string> SaveManager::getAllSaves() const {
    // TODO: Scan save directory for save files
    return std::vector<std::string>();
}

int SaveManager::getSaveCount() const {
    return static_cast<int>(getAllSaves().size());
}

SaveMetadata SaveManager::getSaveMetadata(const std::string& saveName) const {
    std::string filepath = getSaveFilePath(saveName);
    SaveFile tempFile(filepath);
    
    if (tempFile.load()) {
        return tempFile.getMetadata();
    }
    
    return SaveMetadata();
}

void SaveManager::setSaveDirectory(const std::string& directory) {
    m_saveDirectory = directory;
}

void SaveManager::onSaveCreated(const std::function<void(const std::string&)>& callback) {
    m_onSaveCreated = callback;
}

void SaveManager::onSaveLoaded(const std::function<void(const std::string&)>& callback) {
    m_onSaveLoaded = callback;
}

void SaveManager::onSaveDeleted(const std::function<void(const std::string&)>& callback) {
    m_onSaveDeleted = callback;
}

std::string SaveManager::getSaveFilePath(const std::string& saveName) const {
    return m_saveDirectory + saveName + ".sav";
}

} // namespace Engine
