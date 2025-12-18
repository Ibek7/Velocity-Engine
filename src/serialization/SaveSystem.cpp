#include "serialization/SaveSystem.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstring>

namespace JJM {
namespace Serialization {

// SaveData implementation
SaveData::SaveData() {}

SaveData::~SaveData() {}

void SaveData::setInt(const std::string& key, int value) {
    data[key] = std::to_string(value);
}

void SaveData::setFloat(const std::string& key, float value) {
    data[key] = std::to_string(value);
}

void SaveData::setString(const std::string& key, const std::string& value) {
    data[key] = value;
}

void SaveData::setBool(const std::string& key, bool value) {
    data[key] = value ? "1" : "0";
}

int SaveData::getInt(const std::string& key, int defaultValue) const {
    auto it = data.find(key);
    return it != data.end() ? std::stoi(it->second) : defaultValue;
}

float SaveData::getFloat(const std::string& key, float defaultValue) const {
    auto it = data.find(key);
    return it != data.end() ? std::stof(it->second) : defaultValue;
}

std::string SaveData::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = data.find(key);
    return it != data.end() ? it->second : defaultValue;
}

bool SaveData::getBool(const std::string& key, bool defaultValue) const {
    auto it = data.find(key);
    return it != data.end() ? (it->second == "1") : defaultValue;
}

bool SaveData::hasKey(const std::string& key) const {
    return data.find(key) != data.end();
}

void SaveData::removeKey(const std::string& key) {
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

// XOREncryptor implementation
XOREncryptor::XOREncryptor() {}

XOREncryptor::~XOREncryptor() {}

std::vector<uint8_t> XOREncryptor::encrypt(const std::vector<uint8_t>& data,
                                          const std::string& key) {
    if (key.empty()) return data;
    
    std::vector<uint8_t> result(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        result[i] = data[i] ^ key[i % key.length()];
    }
    return result;
}

std::vector<uint8_t> XOREncryptor::decrypt(const std::vector<uint8_t>& data,
                                          const std::string& key) {
    return encrypt(data, key); // XOR is symmetric
}

// AESEncryptor implementation
AESEncryptor::AESEncryptor(int keySize) : keySize(keySize) {}

AESEncryptor::~AESEncryptor() {}

std::vector<uint8_t> AESEncryptor::encrypt(const std::vector<uint8_t>& data,
                                          const std::string& key) {
    // Simplified AES implementation (actual AES would require crypto library)
    auto padded = padData(data);
    
    // Basic substitution cipher for demonstration
    std::vector<uint8_t> result(padded.size());
    for (size_t i = 0; i < padded.size(); ++i) {
        result[i] = padded[i] ^ key[i % key.length()];
    }
    
    return result;
}

std::vector<uint8_t> AESEncryptor::decrypt(const std::vector<uint8_t>& data,
                                          const std::string& key) {
    std::vector<uint8_t> result(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        result[i] = data[i] ^ key[i % key.length()];
    }
    
    return unpadData(result);
}

std::vector<uint8_t> AESEncryptor::padData(const std::vector<uint8_t>& data) {
    size_t blockSize = 16;
    size_t padding = blockSize - (data.size() % blockSize);
    
    std::vector<uint8_t> padded(data);
    padded.resize(data.size() + padding, static_cast<uint8_t>(padding));
    
    return padded;
}

std::vector<uint8_t> AESEncryptor::unpadData(const std::vector<uint8_t>& data) {
    if (data.empty()) return data;
    
    uint8_t padding = data.back();
    if (padding > 0 && padding <= 16) {
        return std::vector<uint8_t>(data.begin(), data.end() - padding);
    }
    
    return data;
}

// DeflateCompressor implementation
DeflateCompressor::DeflateCompressor() : compressionLevel(6) {}

DeflateCompressor::~DeflateCompressor() {}

std::vector<uint8_t> DeflateCompressor::compress(const std::vector<uint8_t>& data) {
    // Simplified compression (actual deflate would require zlib)
    // For demonstration, just return the data as-is
    return data;
}

std::vector<uint8_t> DeflateCompressor::decompress(const std::vector<uint8_t>& data) {
    return data;
}

// SaveSystem implementation
SaveSystem::SaveSystem()
    : encryptionType(EncryptionType::None),
      compressionType(CompressionType::None),
      saveDirectory("./saves") {
    updateEncryptor();
    updateCompressor();
}

SaveSystem::~SaveSystem() {}

bool SaveSystem::save(const std::string& filePath, const SaveData& data) {
    std::string serialized = serialize(data);
    std::vector<uint8_t> bytes = stringToBytes(serialized);
    
    // Apply compression
    if (compressor && compressionType != CompressionType::None) {
        bytes = compressor->compress(bytes);
    }
    
    // Apply encryption
    if (encryptor && encryptionType != EncryptionType::None) {
        bytes = encryptor->encrypt(bytes, encryptionKey);
    }
    
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}

bool SaveSystem::load(const std::string& filePath, SaveData& data) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    std::vector<uint8_t> bytes(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    
    // Apply decryption
    if (encryptor && encryptionType != EncryptionType::None) {
        bytes = encryptor->decrypt(bytes, encryptionKey);
    }
    
    // Apply decompression
    if (compressor && compressionType != CompressionType::None) {
        bytes = compressor->decompress(bytes);
    }
    
    std::string serialized = bytesToString(bytes);
    return deserialize(serialized, data);
}

bool SaveSystem::saveSlot(int slot, const SaveData& data) {
    return save(getSlotFilePath(slot), data);
}

bool SaveSystem::loadSlot(int slot, SaveData& data) {
    return load(getSlotFilePath(slot), data);
}

bool SaveSystem::hasSlot(int slot) const {
    std::ifstream file(getSlotFilePath(slot));
    return file.good();
}

void SaveSystem::deleteSlot(int slot) {
    std::remove(getSlotFilePath(slot).c_str());
}

void SaveSystem::deleteAllSlots() {
    for (int i = 0; i < 100; ++i) {
        if (hasSlot(i)) {
            deleteSlot(i);
        }
    }
}

std::vector<int> SaveSystem::getAvailableSlots() const {
    std::vector<int> slots;
    for (int i = 0; i < 100; ++i) {
        if (hasSlot(i)) {
            slots.push_back(i);
        }
    }
    return slots;
}

std::string SaveSystem::serialize(const SaveData& data) {
    std::ostringstream oss;
    auto keys = data.getKeys();
    
    for (const auto& key : keys) {
        oss << key << "=" << data.getString(key, "") << "\n";
    }
    
    return oss.str();
}

bool SaveSystem::deserialize(const std::string& str, SaveData& data) {
    std::istringstream iss(str);
    std::string line;
    
    while (std::getline(iss, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            data.setString(key, value);
        }
    }
    
    return true;
}

std::vector<uint8_t> SaveSystem::stringToBytes(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}

std::string SaveSystem::bytesToString(const std::vector<uint8_t>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

std::string SaveSystem::getSlotFilePath(int slot) const {
    return saveDirectory + "/save_" + std::to_string(slot) + ".sav";
}

void SaveSystem::updateEncryptor() {
    switch (encryptionType) {
        case EncryptionType::XOR:
            encryptor = std::make_unique<XOREncryptor>();
            break;
        case EncryptionType::AES128:
            encryptor = std::make_unique<AESEncryptor>(128);
            break;
        case EncryptionType::AES256:
            encryptor = std::make_unique<AESEncryptor>(256);
            break;
        default:
            encryptor = nullptr;
            break;
    }
}

void SaveSystem::updateCompressor() {
    switch (compressionType) {
        case CompressionType::Deflate:
        case CompressionType::LZ4:
            compressor = std::make_unique<DeflateCompressor>();
            break;
        default:
            compressor = nullptr;
            break;
    }
}

// SaveMetadata implementation
SaveMetadata::SaveMetadata() : slot(0), timestamp(0), playTime(0.0f), level(0) {}

SaveMetadata::~SaveMetadata() {}

std::string SaveMetadata::getCustomData(const std::string& key) const {
    auto it = customData.find(key);
    return it != customData.end() ? it->second : "";
}

// SaveManager implementation
SaveManager& SaveManager::getInstance() {
    static SaveManager instance;
    return instance;
}

void SaveManager::initialize(const std::string& saveDirectory) {
    saveSystem.setSaveDirectory(saveDirectory);
    quickSaveSlot = 0;
}

void SaveManager::shutdown() {
    // Cleanup if needed
}

bool SaveManager::quickSave() {
    SaveData data;
    SaveMetadata metadata;
    metadata.setSlot(quickSaveSlot);
    metadata.setTimestamp(
        std::chrono::system_clock::now().time_since_epoch().count());
    
    return save(quickSaveSlot, data, metadata);
}

bool SaveManager::quickLoad() {
    SaveData data;
    return load(quickSaveSlot, data);
}

bool SaveManager::save(int slot, const SaveData& data, const SaveMetadata& metadata) {
    return saveSystem.saveSlot(slot, data);
}

bool SaveManager::load(int slot, SaveData& data) {
    return saveSystem.loadSlot(slot, data);
}

bool SaveManager::hasSlot(int slot) const {
    return saveSystem.hasSlot(slot);
}

void SaveManager::deleteSlot(int slot) {
    saveSystem.deleteSlot(slot);
}

SaveMetadata SaveManager::getSlotMetadata(int slot) const {
    SaveMetadata metadata;
    metadata.setSlot(slot);
    return metadata;
}

std::vector<int> SaveManager::getAvailableSlots() const {
    return saveSystem.getAvailableSlots();
}

void SaveManager::update(float deltaTime) {
    if (!autoSaveEnabled) return;
    
    timeSinceLastAutoSave += deltaTime;
    if (timeSinceLastAutoSave >= autoSaveInterval) {
        quickSave();
        timeSinceLastAutoSave = 0.0f;
    }
}

} // namespace Serialization
} // namespace JJM
