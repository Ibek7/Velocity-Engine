#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>

namespace JJM {
namespace Save {

class SaveData {
public:
    void setInt(const std::string& key, int value) { intData[key] = value; }
    void setFloat(const std::string& key, float value) { floatData[key] = value; }
    void setString(const std::string& key, const std::string& value) { stringData[key] = value; }
    void setBool(const std::string& key, bool value) { boolData[key] = value; }
    
    int getInt(const std::string& key, int defaultValue = 0) const;
    float getFloat(const std::string& key, float defaultValue = 0.0f) const;
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    
    void clear();
    
private:
    friend class SaveSystem;
    std::unordered_map<std::string, int> intData;
    std::unordered_map<std::string, float> floatData;
    std::unordered_map<std::string, std::string> stringData;
    std::unordered_map<std::string, bool> boolData;
};

class SaveSystem {
public:
    static SaveSystem& getInstance();
    
    bool save(const std::string& slotName, const SaveData& data);
    bool load(const std::string& slotName, SaveData& data);
    bool deleteSave(const std::string& slotName);
    std::vector<std::string> listSaves() const;
    
    void setAutoSaveEnabled(bool enabled) { autoSaveEnabled = enabled; }
    void setAutoSaveInterval(float seconds) { autoSaveInterval = seconds; }
    void update(float deltaTime);
    
    void setSaveDirectory(const std::string& dir) { saveDirectory = dir; }
    std::string getSaveDirectory() const { return saveDirectory; }
    
private:
    SaveSystem();
    std::string saveDirectory;
    bool autoSaveEnabled;
    float autoSaveInterval;
    float timeSinceLastSave;
    
    std::string getSaveFilePath(const std::string& slotName) const;
};

} // namespace Save
} // namespace JJM

#endif
