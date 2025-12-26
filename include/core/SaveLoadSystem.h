#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

// Save/load system with compression
namespace Engine {

enum class SaveFormat {
    Binary,
    JSON,
    Compressed
};

struct SaveMetadata {
    std::string saveName;
    std::string timestamp;
    int version;
    float playtime;
    std::string levelName;
    std::map<std::string, std::string> customData;
};

class SaveFile {
public:
    SaveFile(const std::string& filename);
    ~SaveFile();

    // Data writing
    void writeInt(const std::string& key, int value);
    void writeFloat(const std::string& key, float value);
    void writeString(const std::string& key, const std::string& value);
    void writeBool(const std::string& key, bool value);
    void writeBytes(const std::string& key, const unsigned char* data, size_t size);
    
    // Data reading
    int readInt(const std::string& key, int defaultValue = 0) const;
    float readFloat(const std::string& key, float defaultValue = 0.0f) const;
    std::string readString(const std::string& key, const std::string& defaultValue = "") const;
    bool readBool(const std::string& key, bool defaultValue = false) const;
    std::vector<unsigned char> readBytes(const std::string& key) const;
    
    // Metadata
    void setMetadata(const SaveMetadata& metadata);
    const SaveMetadata& getMetadata() const { return m_metadata; }
    
    // Query
    bool hasKey(const std::string& key) const;
    std::vector<std::string> getAllKeys() const;
    
    // File operations
    bool save(SaveFormat format = SaveFormat::Binary);
    bool load();
    void clear();
    
    const std::string& getFilename() const { return m_filename; }

private:
    bool saveBinary();
    bool saveJSON();
    bool saveCompressed();
    bool loadBinary();
    bool loadJSON();
    bool loadCompressed();

    std::string m_filename;
    SaveMetadata m_metadata;
    
    std::map<std::string, int> m_intData;
    std::map<std::string, float> m_floatData;
    std::map<std::string, std::string> m_stringData;
    std::map<std::string, bool> m_boolData;
    std::map<std::string, std::vector<unsigned char>> m_bytesData;
};

class SaveManager {
public:
    static SaveManager& getInstance();

    // Save management
    SaveFile* createSave(const std::string& saveName);
    SaveFile* loadSave(const std::string& saveName);
    bool deleteSave(const std::string& saveName);
    bool saveExists(const std::string& saveName) const;
    
    // Auto-save
    void enableAutoSave(bool enable) { m_autoSaveEnabled = enable; }
    void setAutoSaveInterval(float seconds) { m_autoSaveInterval = seconds; }
    void triggerAutoSave();
    void update(float deltaTime);
    
    // Quick save/load
    void quickSave();
    void quickLoad();
    SaveFile* getQuickSave() const { return m_quickSave; }
    
    // Save slots
    std::vector<std::string> getAllSaves() const;
    int getSaveCount() const;
    SaveMetadata getSaveMetadata(const std::string& saveName) const;
    
    // Settings
    void setSaveDirectory(const std::string& directory);
    const std::string& getSaveDirectory() const { return m_saveDirectory; }
    void setDefaultFormat(SaveFormat format) { m_defaultFormat = format; }
    
    // Callbacks
    void onSaveCreated(const std::function<void(const std::string&)>& callback);
    void onSaveLoaded(const std::function<void(const std::string&)>& callback);
    void onSaveDeleted(const std::function<void(const std::string&)>& callback);

private:
    SaveManager();
    SaveManager(const SaveManager&) = delete;
    SaveManager& operator=(const SaveManager&) = delete;

    std::string getSaveFilePath(const std::string& saveName) const;

    std::string m_saveDirectory;
    SaveFormat m_defaultFormat;
    
    bool m_autoSaveEnabled;
    float m_autoSaveInterval;
    float m_autoSaveTimer;
    
    SaveFile* m_quickSave;
    SaveFile* m_currentSave;
    
    std::function<void(const std::string&)> m_onSaveCreated;
    std::function<void(const std::string&)> m_onSaveLoaded;
    std::function<void(const std::string&)> m_onSaveDeleted;
};

} // namespace Engine
