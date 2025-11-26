#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#include <string>
#include <map>
#include <vector>
#include <variant>
#include <fstream>

namespace JJM {
namespace Utils {

using SaveValue = std::variant<int, float, bool, std::string>;

class SaveData {
private:
    std::map<std::string, SaveValue> data;
    
public:
    SaveData();
    
    void setInt(const std::string& key, int value);
    void setFloat(const std::string& key, float value);
    void setBool(const std::string& key, bool value);
    void setString(const std::string& key, const std::string& value);
    
    int getInt(const std::string& key, int defaultValue = 0) const;
    float getFloat(const std::string& key, float defaultValue = 0.0f) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    
    bool hasKey(const std::string& key) const;
    void remove(const std::string& key);
    void clear();
    
    std::vector<std::string> getKeys() const;
    
    friend class SaveSystem;
};

class SaveSystem {
private:
    std::string savePath;
    SaveData currentSave;
    
    static SaveSystem* instance;
    SaveSystem();
    
public:
    static SaveSystem* getInstance();
    ~SaveSystem();
    
    void setSavePath(const std::string& path);
    std::string getSavePath() const { return savePath; }
    
    bool save(const std::string& filename);
    bool load(const std::string& filename);
    
    bool saveToSlot(int slot);
    bool loadFromSlot(int slot);
    
    bool deleteSave(const std::string& filename);
    bool saveExists(const std::string& filename) const;
    
    std::vector<std::string> listSaves() const;
    
    SaveData& getData() { return currentSave; }
    const SaveData& getData() const { return currentSave; }
    
private:
    std::string getFullPath(const std::string& filename) const;
    bool writeSaveFile(const std::string& filepath);
    bool readSaveFile(const std::string& filepath);
};

} // namespace Utils
} // namespace JJM

#endif // SAVE_SYSTEM_H
