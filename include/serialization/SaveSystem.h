#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace JJM {
namespace Serialization {

enum class EncryptionType {
    None,
    XOR,
    AES128,
    AES256
};

enum class CompressionType {
    None,
    Deflate,
    LZ4
};

class SaveData {
public:
    SaveData();
    ~SaveData();
    
    void setInt(const std::string& key, int value);
    void setFloat(const std::string& key, float value);
    void setString(const std::string& key, const std::string& value);
    void setBool(const std::string& key, bool value);
    
    int getInt(const std::string& key, int defaultValue = 0) const;
    float getFloat(const std::string& key, float defaultValue = 0.0f) const;
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    
    bool hasKey(const std::string& key) const;
    void removeKey(const std::string& key);
    void clear();
    
    std::vector<std::string> getKeys() const;

private:
    std::unordered_map<std::string, std::string> data;
};

class Encryptor {
public:
    virtual ~Encryptor() {}
    
    virtual std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data,
                                         const std::string& key) = 0;
    
    virtual std::vector<uint8_t> decrypt(const std::vector<uint8_t>& data,
                                         const std::string& key) = 0;
};

class XOREncryptor : public Encryptor {
public:
    XOREncryptor();
    ~XOREncryptor();
    
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data,
                                const std::string& key) override;
    
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& data,
                                const std::string& key) override;
};

class AESEncryptor : public Encryptor {
public:
    AESEncryptor(int keySize = 128);
    ~AESEncryptor();
    
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data,
                                const std::string& key) override;
    
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& data,
                                const std::string& key) override;

private:
    int keySize;
    
    std::vector<uint8_t> padData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> unpadData(const std::vector<uint8_t>& data);
};

class Compressor {
public:
    virtual ~Compressor() {}
    
    virtual std::vector<uint8_t> compress(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> decompress(const std::vector<uint8_t>& data) = 0;
};

class DeflateCompressor : public Compressor {
public:
    DeflateCompressor();
    ~DeflateCompressor();
    
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data) override;
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& data) override;

private:
    int compressionLevel;
};

class SaveSystem {
public:
    SaveSystem();
    ~SaveSystem();
    
    void setEncryptionType(EncryptionType type) { encryptionType = type; }
    EncryptionType getEncryptionType() const { return encryptionType; }
    
    void setCompressionType(CompressionType type) { compressionType = type; }
    CompressionType getCompressionType() const { return compressionType; }
    
    void setEncryptionKey(const std::string& key) { encryptionKey = key; }
    
    bool save(const std::string& filePath, const SaveData& data);
    bool load(const std::string& filePath, SaveData& data);
    
    bool saveSlot(int slot, const SaveData& data);
    bool loadSlot(int slot, SaveData& data);
    
    bool hasSlot(int slot) const;
    void deleteSlot(int slot);
    void deleteAllSlots();
    
    std::vector<int> getAvailableSlots() const;
    
    void setSaveDirectory(const std::string& directory) {
        saveDirectory = directory;
    }
    
    std::string getSaveDirectory() const { return saveDirectory; }

private:
    EncryptionType encryptionType;
    CompressionType compressionType;
    std::string encryptionKey;
    std::string saveDirectory;
    
    std::unique_ptr<Encryptor> encryptor;
    std::unique_ptr<Compressor> compressor;
    
    std::string serialize(const SaveData& data);
    bool deserialize(const std::string& str, SaveData& data);
    
    std::vector<uint8_t> stringToBytes(const std::string& str);
    std::string bytesToString(const std::vector<uint8_t>& bytes);
    
    std::string getSlotFilePath(int slot) const;
    
    void updateEncryptor();
    void updateCompressor();
};

class SaveMetadata {
public:
    SaveMetadata();
    ~SaveMetadata();
    
    void setSlot(int slot) { this->slot = slot; }
    int getSlot() const { return slot; }
    
    void setTimestamp(long long timestamp) { this->timestamp = timestamp; }
    long long getTimestamp() const { return timestamp; }
    
    void setDescription(const std::string& desc) { description = desc; }
    std::string getDescription() const { return description; }
    
    void setPlayTime(float time) { playTime = time; }
    float getPlayTime() const { return playTime; }
    
    void setLevel(int level) { this->level = level; }
    int getLevel() const { return level; }
    
    void setCustomData(const std::string& key, const std::string& value) {
        customData[key] = value;
    }
    
    std::string getCustomData(const std::string& key) const;

private:
    int slot;
    long long timestamp;
    std::string description;
    float playTime;
    int level;
    std::unordered_map<std::string, std::string> customData;
};

class SaveManager {
public:
    static SaveManager& getInstance();
    
    void initialize(const std::string& saveDirectory);
    void shutdown();
    
    bool quickSave();
    bool quickLoad();
    
    bool save(int slot, const SaveData& data, const SaveMetadata& metadata);
    bool load(int slot, SaveData& data);
    
    bool hasSlot(int slot) const;
    void deleteSlot(int slot);
    
    SaveMetadata getSlotMetadata(int slot) const;
    std::vector<int> getAvailableSlots() const;
    
    void setAutoSaveEnabled(bool enabled) { autoSaveEnabled = enabled; }
    bool isAutoSaveEnabled() const { return autoSaveEnabled; }
    
    void setAutoSaveInterval(float interval) { autoSaveInterval = interval; }
    float getAutoSaveInterval() const { return autoSaveInterval; }
    
    void update(float deltaTime);

private:
    SaveManager() : autoSaveEnabled(false), autoSaveInterval(300.0f),
                   timeSinceLastAutoSave(0.0f) {}
    ~SaveManager() {}
    SaveManager(const SaveManager&) = delete;
    SaveManager& operator=(const SaveManager&) = delete;
    
    SaveSystem saveSystem;
    bool autoSaveEnabled;
    float autoSaveInterval;
    float timeSinceLastAutoSave;
    int quickSaveSlot;
};

} // namespace Serialization
} // namespace JJM
