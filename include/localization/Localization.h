#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace JJM {
namespace Localization {

using LocaleID = std::string;
using StringID = std::string;

struct LocaleInfo {
    LocaleID id;
    std::string name;
    std::string nativeName;
    std::string languageCode;
    std::string countryCode;
    bool rightToLeft;
};

class StringTable {
public:
    StringTable();
    
    void addString(const StringID& id, const std::string& value);
    void removeString(const StringID& id);
    
    std::string getString(const StringID& id, const std::string& defaultValue = "") const;
    
    bool hasString(const StringID& id) const;
    
    void clear();
    size_t getStringCount() const { return strings.size(); }
    
    const std::unordered_map<StringID, std::string>& getAllStrings() const { return strings; }

private:
    std::unordered_map<StringID, std::string> strings;
};

class LocalizationManager {
public:
    LocalizationManager();
    ~LocalizationManager();
    
    bool loadLocale(const LocaleID& localeId, const std::string& filePath);
    void unloadLocale(const LocaleID& localeId);
    
    void setCurrentLocale(const LocaleID& localeId);
    const LocaleID& getCurrentLocale() const { return currentLocale; }
    
    std::string getString(const StringID& id, const std::string& defaultValue = "") const;
    
    std::string format(const StringID& id, const std::vector<std::string>& args) const;
    
    template<typename... Args>
    std::string format(const StringID& id, Args&&... args) const {
        std::vector<std::string> argVec = {toString(std::forward<Args>(args))...};
        return format(id, argVec);
    }
    
    bool hasString(const StringID& id) const;
    
    void registerLocale(const LocaleInfo& info);
    const LocaleInfo* getLocaleInfo(const LocaleID& localeId) const;
    
    std::vector<LocaleID> getAvailableLocales() const;
    
    void setFallbackLocale(const LocaleID& localeId) { fallbackLocale = localeId; }
    const LocaleID& getFallbackLocale() const { return fallbackLocale; }
    
    void setLocaleChangedCallback(std::function<void(const LocaleID&)> callback) {
        onLocaleChanged = callback;
    }

private:
    std::unordered_map<LocaleID, StringTable> localeTables;
    std::unordered_map<LocaleID, LocaleInfo> localeInfos;
    
    LocaleID currentLocale;
    LocaleID fallbackLocale;
    
    std::function<void(const LocaleID&)> onLocaleChanged;
    
    bool loadFromFile(const std::string& filePath, StringTable& table);
    
    template<typename T>
    std::string toString(T&& value) const {
        return std::to_string(std::forward<T>(value));
    }
    
    std::string toString(const std::string& value) const { return value; }
    std::string toString(const char* value) const { return std::string(value); }
};

class LocalizedString {
public:
    LocalizedString(const StringID& id, LocalizationManager* manager = nullptr);
    
    std::string get() const;
    
    operator std::string() const { return get(); }
    
    const StringID& getID() const { return id; }

private:
    StringID id;
    LocalizationManager* manager;
};

#define LOC(id) JJM::Localization::LocalizedString(id)

} // namespace Localization
} // namespace JJM
