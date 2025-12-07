#include "localization/Localization.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace JJM {
namespace Localization {

StringTable::StringTable() {}

void StringTable::addString(const StringID& id, const std::string& value) {
    strings[id] = value;
}

void StringTable::removeString(const StringID& id) {
    strings.erase(id);
}

std::string StringTable::getString(const StringID& id, const std::string& defaultValue) const {
    auto it = strings.find(id);
    return (it != strings.end()) ? it->second : defaultValue;
}

bool StringTable::hasString(const StringID& id) const {
    return strings.find(id) != strings.end();
}

void StringTable::clear() {
    strings.clear();
}

LocalizationManager::LocalizationManager()
    : currentLocale("en_US"), fallbackLocale("en_US") {}

LocalizationManager::~LocalizationManager() {}

bool LocalizationManager::loadLocale(const LocaleID& localeId, const std::string& filePath) {
    StringTable table;
    
    if (!loadFromFile(filePath, table)) {
        return false;
    }
    
    localeTables[localeId] = table;
    return true;
}

void LocalizationManager::unloadLocale(const LocaleID& localeId) {
    localeTables.erase(localeId);
}

void LocalizationManager::setCurrentLocale(const LocaleID& localeId) {
    if (localeTables.find(localeId) != localeTables.end()) {
        currentLocale = localeId;
        
        if (onLocaleChanged) {
            onLocaleChanged(currentLocale);
        }
    }
}

std::string LocalizationManager::getString(const StringID& id, const std::string& defaultValue) const {
    auto it = localeTables.find(currentLocale);
    if (it != localeTables.end()) {
        if (it->second.hasString(id)) {
            return it->second.getString(id);
        }
    }
    
    if (!fallbackLocale.empty() && fallbackLocale != currentLocale) {
        auto fallbackIt = localeTables.find(fallbackLocale);
        if (fallbackIt != localeTables.end()) {
            if (fallbackIt->second.hasString(id)) {
                return fallbackIt->second.getString(id);
            }
        }
    }
    
    return defaultValue.empty() ? id : defaultValue;
}

std::string LocalizationManager::format(const StringID& id, const std::vector<std::string>& args) const {
    std::string str = getString(id);
    
    for (size_t i = 0; i < args.size(); ++i) {
        std::string placeholder = "{" + std::to_string(i) + "}";
        
        size_t pos = 0;
        while ((pos = str.find(placeholder, pos)) != std::string::npos) {
            str.replace(pos, placeholder.length(), args[i]);
            pos += args[i].length();
        }
    }
    
    return str;
}

bool LocalizationManager::hasString(const StringID& id) const {
    auto it = localeTables.find(currentLocale);
    if (it != localeTables.end() && it->second.hasString(id)) {
        return true;
    }
    
    if (!fallbackLocale.empty() && fallbackLocale != currentLocale) {
        auto fallbackIt = localeTables.find(fallbackLocale);
        if (fallbackIt != localeTables.end() && fallbackIt->second.hasString(id)) {
            return true;
        }
    }
    
    return false;
}

void LocalizationManager::registerLocale(const LocaleInfo& info) {
    localeInfos[info.id] = info;
}

const LocaleInfo* LocalizationManager::getLocaleInfo(const LocaleID& localeId) const {
    auto it = localeInfos.find(localeId);
    return (it != localeInfos.end()) ? &it->second : nullptr;
}

std::vector<LocaleID> LocalizationManager::getAvailableLocales() const {
    std::vector<LocaleID> locales;
    for (const auto& pair : localeTables) {
        locales.push_back(pair.first);
    }
    return locales;
}

bool LocalizationManager::loadFromFile(const std::string& filePath, StringTable& table) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t separatorPos = line.find('=');
        if (separatorPos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, separatorPos);
        std::string value = line.substr(separatorPos + 1);
        
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        size_t pos = 0;
        while ((pos = value.find("\\n", pos)) != std::string::npos) {
            value.replace(pos, 2, "\n");
            pos += 1;
        }
        
        table.addString(key, value);
    }
    
    file.close();
    return true;
}

LocalizedString::LocalizedString(const StringID& id, LocalizationManager* manager)
    : id(id), manager(manager) {}

std::string LocalizedString::get() const {
    if (manager) {
        return manager->getString(id);
    }
    return id;
}

} // namespace Localization
} // namespace JJM
