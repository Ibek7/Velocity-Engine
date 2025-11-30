#include "localization/LocalizationSystem.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <iomanip>
#include <ctime>

namespace JJM {
namespace Localization {

// -- StringFormatter implementation
void StringFormatter::setVariable(const std::string& name, const std::string& value) {
    variables[name] = value;
}

void StringFormatter::setVariable(const std::string& name, int value) {
    variables[name] = std::to_string(value);
}

void StringFormatter::setVariable(const std::string& name, float value) {
    variables[name] = std::to_string(value);
}

void StringFormatter::setVariable(const std::string& name, double value) {
    variables[name] = std::to_string(value);
}

void StringFormatter::clearVariables() {
    variables.clear();
}

std::string StringFormatter::format(const std::string& template_str) const {
    std::string result = template_str;
    for (const auto& var : variables) {
        std::string placeholder = "{" + var.first + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), var.second);
            pos += var.second.length();
        }
    }
    return result;
}

std::string StringFormatter::format(const std::string& template_str, const std::vector<std::string>& args) const {
    std::string result = template_str;
    for (size_t i = 0; i < args.size(); ++i) {
        std::string placeholder = "{" + std::to_string(i) + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), args[i]);
            pos += args[i].length();
        }
    }
    return result;
}

std::string StringFormatter::formatNamed(const std::string& template_str, 
                                        const std::unordered_map<std::string, std::string>& namedArgs) const {
    std::string result = template_str;
    for (const auto& arg : namedArgs) {
        std::string placeholder = "{" + arg.first + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), arg.second);
            pos += arg.second.length();
        }
    }
    return result;
}

// -- LocaleFormatter implementation
LocaleFormatter::LocaleFormatter(const std::string& locale)
    : localeCode(locale), decimalSeparator('.'), thousandsSeparator(','),
      currencySymbol("$"), dateFormat("MM/dd/yyyy"), timeFormat("HH:mm:ss") {
    setLocale(locale);
}

std::string LocaleFormatter::formatNumber(double number, int precision) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << number;
    std::string result = oss.str();
    
    // Add thousands separator (simple implementation)
    size_t decimal_pos = result.find('.');
    if (decimal_pos == std::string::npos) decimal_pos = result.length();
    
    for (int i = static_cast<int>(decimal_pos) - 3; i > 0; i -= 3) {
        result.insert(i, 1, thousandsSeparator);
    }
    
    // Replace decimal separator if needed
    if (decimalSeparator != '.') {
        size_t dot_pos = result.find('.');
        if (dot_pos != std::string::npos) {
            result[dot_pos] = decimalSeparator;
        }
    }
    
    return result;
}

std::string LocaleFormatter::formatCurrency(double amount) const {
    return currencySymbol + formatNumber(amount, 2);
}

std::string LocaleFormatter::formatPercent(double value) const {
    return formatNumber(value * 100.0, 1) + "%";
}

std::string LocaleFormatter::formatDate(int year, int month, int day) const {
    // Simple date formatting - in real implementation would use platform-specific date formatting
    std::ostringstream oss;
    if (dateFormat == "dd/MM/yyyy") {
        oss << std::setfill('0') << std::setw(2) << day << "/"
            << std::setw(2) << month << "/" << year;
    } else { // Default MM/dd/yyyy
        oss << std::setfill('0') << std::setw(2) << month << "/"
            << std::setw(2) << day << "/" << year;
    }
    return oss.str();
}

std::string LocaleFormatter::formatTime(int hour, int minute, int second) const {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hour << ":"
        << std::setw(2) << minute << ":" << std::setw(2) << second;
    return oss.str();
}

void LocaleFormatter::setLocale(const std::string& locale) {
    localeCode = locale;
    // Set locale-specific formatting based on language code
    if (locale.find("es") == 0) { // Spanish
        decimalSeparator = ',';
        thousandsSeparator = '.';
        currencySymbol = "€";
        dateFormat = "dd/MM/yyyy";
    } else if (locale.find("de") == 0) { // German
        decimalSeparator = ',';
        thousandsSeparator = '.';
        currencySymbol = "€";
        dateFormat = "dd.MM.yyyy";
    } else if (locale.find("fr") == 0) { // French
        decimalSeparator = ',';
        thousandsSeparator = ' ';
        currencySymbol = "€";
        dateFormat = "dd/MM/yyyy";
    }
    // Default English/US formatting already set in constructor
}

// -- JSONTranslationLoader implementation (simplified)
bool JSONTranslationLoader::loadFromFile(const std::string& filePath, 
                                         std::unordered_map<std::string, LocalizedString>& strings) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open translation file: " << filePath << std::endl;
        return false;
    }
    
    // Simple JSON parsing - in real implementation would use a proper JSON library
    std::string line;
    std::string key, value;
    bool inQuotes = false;
    char lastChar = 0;
    
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty() || line[0] == '{' || line[0] == '}') continue;
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            key = line.substr(0, colonPos);
            value = line.substr(colonPos + 1);
            
            // Remove quotes and trim
            key.erase(std::remove(key.begin(), key.end(), '"'), key.end());
            key.erase(std::remove(key.begin(), key.end(), ' '), key.end());
            
            value.erase(0, value.find_first_not_of(" \t"));
            if (!value.empty() && value[0] == '"') value.erase(0, 1);
            if (!value.empty() && (value.back() == '"' || value.back() == ',')) value.pop_back();
            if (!value.empty() && value.back() == '"') value.pop_back();
            
            LocalizedString locStr(key, value);
            strings[key] = locStr;
        }
    }
    
    return true;
}

bool JSONTranslationLoader::saveToFile(const std::string& filePath, 
                                       const std::unordered_map<std::string, LocalizedString>& strings) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to create translation file: " << filePath << std::endl;
        return false;
    }
    
    file << "{\n";
    bool first = true;
    for (const auto& pair : strings) {
        if (!first) file << ",\n";
        first = false;
        file << "  \"" << pair.first << "\": \"" << pair.second.value << "\"";
    }
    file << "\n}\n";
    
    return true;
}

// -- CSVTranslationLoader implementation
bool CSVTranslationLoader::loadFromFile(const std::string& filePath, 
                                        std::unordered_map<std::string, LocalizedString>& strings) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;
    
    std::string line;
    bool first_line = true;
    
    while (std::getline(file, line)) {
        if (first_line) { first_line = false; continue; } // Skip header
        
        std::istringstream ss(line);
        std::string key, value, context;
        
        if (std::getline(ss, key, ',') && std::getline(ss, value, ',')) {
            std::getline(ss, context, ','); // Optional context
            
            // Remove quotes if present
            if (!key.empty() && key.front() == '"' && key.back() == '"') {
                key = key.substr(1, key.length() - 2);
            }
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            LocalizedString locStr(key, value, context);
            strings[key] = locStr;
        }
    }
    
    return true;
}

bool CSVTranslationLoader::saveToFile(const std::string& filePath, 
                                      const std::unordered_map<std::string, LocalizedString>& strings) {
    std::ofstream file(filePath);
    if (!file.is_open()) return false;
    
    file << "Key,Value,Context\n";
    for (const auto& pair : strings) {
        file << "\"" << pair.first << "\",\"" << pair.second.value << "\",\"" << pair.second.context << "\"\n";
    }
    
    return true;
}

// -- LanguageDetector implementation
void LanguageDetector::addPreferredLanguage(const std::string& languageCode) {
    preferredLanguages.push_back(languageCode);
}

void LanguageDetector::setPreferredLanguages(const std::vector<std::string>& languages) {
    preferredLanguages = languages;
}

void LanguageDetector::addAvailableLanguage(const LanguageInfo& language) {
    availableLanguages[language.code] = language;
}

std::string LanguageDetector::detectBestLanguage() const {
    // Try to match preferred languages with available ones
    for (const auto& preferred : preferredLanguages) {
        if (availableLanguages.find(preferred) != availableLanguages.end()) {
            return preferred;
        }
        
        // Try language without region (e.g., "en" for "en-US")
        std::string lang_only = preferred.substr(0, preferred.find('-'));
        if (availableLanguages.find(lang_only) != availableLanguages.end()) {
            return lang_only;
        }
    }
    
    // Return default language or first available
    for (const auto& lang : availableLanguages) {
        if (lang.second.isDefault) return lang.first;
    }
    
    if (!availableLanguages.empty()) {
        return availableLanguages.begin()->first;
    }
    
    return "en"; // Ultimate fallback
}

std::vector<std::string> LanguageDetector::getFallbackChain(const std::string& preferredLanguage) const {
    std::vector<std::string> chain;
    chain.push_back(preferredLanguage);
    
    // Add language without region
    std::string lang_only = preferredLanguage.substr(0, preferredLanguage.find('-'));
    if (lang_only != preferredLanguage) {
        chain.push_back(lang_only);
    }
    
    // Add English as universal fallback if not already present
    if (std::find(chain.begin(), chain.end(), "en") == chain.end()) {
        chain.push_back("en");
    }
    
    return chain;
}

std::string LanguageDetector::getSystemLocale() const {
    // Platform-specific implementation would go here
    // For now, return a default
    return "en-US";
}

std::vector<std::string> LanguageDetector::getSystemPreferredLanguages() const {
    // Platform-specific implementation would go here
    return { getSystemLocale() };
}

// -- LocalizationManager implementation
LocalizationManager* LocalizationManager::instance = nullptr;

LocalizationManager::LocalizationManager()
    : currentLanguage("en"), fallbackLanguage("en"), initialized(false) {
    stringFormatter = std::make_unique<StringFormatter>();
    localeFormatter = std::make_unique<LocaleFormatter>();
    languageDetector = std::make_unique<LanguageDetector>();
}

LocalizationManager* LocalizationManager::getInstance() {
    if (!instance) {
        instance = new LocalizationManager();
    }
    return instance;
}

LocalizationManager::~LocalizationManager() {
    shutdown();
}

bool LocalizationManager::initialize(const std::string& defaultLanguage) {
    if (initialized) return true;
    
    registerDefaultLoaders();
    loadDefaultFontFallbacks();
    
    currentLanguage = defaultLanguage;
    fallbackLanguage = defaultLanguage;
    
    // Add default language info
    addLanguage(LanguageInfo(defaultLanguage, "Default", "", "ltr", true));
    
    initialized = true;
    return true;
}

void LocalizationManager::shutdown() {
    std::lock_guard<std::mutex> lock(stringTableMutex);
    stringTables.clear();
    supportedLanguages.clear();
    fontFallbacks.clear();
    languageChangeCallbacks.clear();
    initialized = false;
}

bool LocalizationManager::loadLanguage(const std::string& languageCode, const std::string& filePath, 
                                      TranslationFormat format) {
    auto loaderIt = loaders.find(format);
    if (loaderIt == loaders.end()) {
        std::cerr << "No loader available for format" << std::endl;
        return false;
    }
    
    std::unordered_map<std::string, LocalizedString> strings;
    if (!loaderIt->second->loadFromFile(filePath, strings)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(stringTableMutex);
    stringTables[languageCode] = std::move(strings);
    
    return true;
}

bool LocalizationManager::addLanguage(const LanguageInfo& language) {
    supportedLanguages[language.code] = language;
    languageDetector->addAvailableLanguage(language);
    return true;
}

bool LocalizationManager::setCurrentLanguage(const std::string& languageCode) {
    if (!isLanguageSupported(languageCode)) {
        std::cerr << "Language not supported: " << languageCode << std::endl;
        return false;
    }
    
    std::string oldLanguage = currentLanguage;
    currentLanguage = languageCode;
    localeFormatter->setLocale(languageCode);
    
    notifyLanguageChange(oldLanguage, currentLanguage);
    
    return true;
}

void LocalizationManager::setFallbackLanguage(const std::string& languageCode) {
    fallbackLanguage = languageCode;
}

std::vector<LanguageInfo> LocalizationManager::getSupportedLanguages() const {
    std::vector<LanguageInfo> languages;
    for (const auto& pair : supportedLanguages) {
        languages.push_back(pair.second);
    }
    return languages;
}

bool LocalizationManager::isLanguageSupported(const std::string& languageCode) const {
    return supportedLanguages.find(languageCode) != supportedLanguages.end();
}

LanguageInfo LocalizationManager::getLanguageInfo(const std::string& languageCode) const {
    auto it = supportedLanguages.find(languageCode);
    if (it != supportedLanguages.end()) {
        return it->second;
    }
    return LanguageInfo(); // Empty language info
}

std::string LocalizationManager::getString(const std::string& key) const {
    return getString(key, key); // Use key as default if not found
}

std::string LocalizationManager::getString(const std::string& key, const std::string& defaultValue) const {
    std::string result = findStringInternal(key);
    return result.empty() ? defaultValue : result;
}

std::string LocalizationManager::getFormattedString(const std::string& key, const std::vector<std::string>& args) const {
    std::string template_str = getString(key);
    return stringFormatter->format(template_str, args);
}

std::string LocalizationManager::getFormattedString(const std::string& key, 
                                                   const std::unordered_map<std::string, std::string>& namedArgs) const {
    std::string template_str = getString(key);
    return stringFormatter->formatNamed(template_str, namedArgs);
}

std::string LocalizationManager::getPluralString(const std::string& key, int count) const {
    std::lock_guard<std::mutex> lock(stringTableMutex);
    
    // Try current language first
    auto langIt = stringTables.find(currentLanguage);
    if (langIt != stringTables.end()) {
        auto strIt = langIt->second.find(key);
        if (strIt != langIt->second.end() && strIt->second.isPlural) {
            PluralForm form = getPluralForm(count, currentLanguage);
            auto pluralIt = strIt->second.pluralForms.find(form);
            if (pluralIt != strIt->second.pluralForms.end()) {
                return pluralIt->second;
            }
        }
    }
    
    // Fallback to regular string
    return getString(key);
}

std::string LocalizationManager::getFormattedPluralString(const std::string& key, int count, 
                                                         const std::vector<std::string>& args) const {
    std::string template_str = getPluralString(key, count);
    return stringFormatter->format(template_str, args);
}

bool LocalizationManager::addString(const std::string& languageCode, const LocalizedString& locString) {
    std::lock_guard<std::mutex> lock(stringTableMutex);
    stringTables[languageCode][locString.key] = locString;
    return true;
}

bool LocalizationManager::updateString(const std::string& languageCode, const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(stringTableMutex);
    auto langIt = stringTables.find(languageCode);
    if (langIt != stringTables.end()) {
        auto strIt = langIt->second.find(key);
        if (strIt != langIt->second.end()) {
            strIt->second.value = value;
            return true;
        }
    }
    return false;
}

bool LocalizationManager::removeString(const std::string& languageCode, const std::string& key) {
    std::lock_guard<std::mutex> lock(stringTableMutex);
    auto langIt = stringTables.find(languageCode);
    if (langIt != stringTables.end()) {
        return langIt->second.erase(key) > 0;
    }
    return false;
}

void LocalizationManager::clearStrings(const std::string& languageCode) {
    std::lock_guard<std::mutex> lock(stringTableMutex);
    stringTables[languageCode].clear();
}

bool LocalizationManager::exportLanguage(const std::string& languageCode, const std::string& filePath, 
                                         TranslationFormat format) {
    auto loaderIt = loaders.find(format);
    if (loaderIt == loaders.end()) return false;
    
    std::lock_guard<std::mutex> lock(stringTableMutex);
    auto langIt = stringTables.find(languageCode);
    if (langIt == stringTables.end()) return false;
    
    return loaderIt->second->saveToFile(filePath, langIt->second);
}

TextDirection LocalizationManager::getTextDirection(const std::string& languageCode) const {
    std::string lang = languageCode.empty() ? currentLanguage : languageCode;
    auto langInfo = getLanguageInfo(lang);
    return (langInfo.direction == "rtl") ? TextDirection::RightToLeft : TextDirection::LeftToRight;
}

TextAlignment LocalizationManager::getDefaultAlignment(const std::string& languageCode) const {
    return (getTextDirection(languageCode) == TextDirection::RightToLeft) ? 
           TextAlignment::End : TextAlignment::Start;
}

void LocalizationManager::addLanguageChangeCallback(std::function<void(const std::string&, const std::string&)> callback) {
    languageChangeCallbacks.push_back(callback);
}

void LocalizationManager::removeLanguageChangeCallbacks() {
    languageChangeCallbacks.clear();
}

size_t LocalizationManager::getStringCount(const std::string& languageCode) const {
    std::lock_guard<std::mutex> lock(stringTableMutex);
    std::string lang = languageCode.empty() ? currentLanguage : languageCode;
    auto langIt = stringTables.find(lang);
    return (langIt != stringTables.end()) ? langIt->second.size() : 0;
}

std::string LocalizationManager::findStringInternal(const std::string& key) const {
    std::lock_guard<std::mutex> lock(stringTableMutex);
    
    // Try current language first
    auto langIt = stringTables.find(currentLanguage);
    if (langIt != stringTables.end()) {
        auto strIt = langIt->second.find(key);
        if (strIt != langIt->second.end()) {
            return strIt->second.value;
        }
    }
    
    // Try fallback language
    if (currentLanguage != fallbackLanguage) {
        auto fallbackIt = stringTables.find(fallbackLanguage);
        if (fallbackIt != stringTables.end()) {
            auto strIt = fallbackIt->second.find(key);
            if (strIt != fallbackIt->second.end()) {
                return strIt->second.value;
            }
        }
    }
    
    return handleMissingString(key);
}

PluralForm LocalizationManager::getPluralForm(int count, const std::string& languageCode) const {
    // Simplified pluralization rules - real implementation would be more comprehensive
    if (count == 0) return PluralForm::Zero;
    if (count == 1) return PluralForm::One;
    if (count == 2) return PluralForm::Two;
    return PluralForm::Other;
}

void LocalizationManager::registerDefaultLoaders() {
    loaders[TranslationFormat::JSON] = std::make_unique<JSONTranslationLoader>();
    loaders[TranslationFormat::CSV] = std::make_unique<CSVTranslationLoader>();
}

void LocalizationManager::notifyLanguageChange(const std::string& oldLanguage, const std::string& newLanguage) {
    for (auto& callback : languageChangeCallbacks) {
        callback(oldLanguage, newLanguage);
    }
}

void LocalizationManager::loadDefaultFontFallbacks() {
    // Add common font fallbacks for different languages
    fontFallbacks["en"] = FontFallback("Arial", {"Helvetica", "sans-serif"});
    fontFallbacks["zh"] = FontFallback("SimHei", {"Microsoft YaHei", "WenQuanYi Micro Hei", "sans-serif"});
    fontFallbacks["ja"] = FontFallback("Meiryo", {"Yu Gothic", "Hiragino Sans", "sans-serif"});
    fontFallbacks["ko"] = FontFallback("Malgun Gothic", {"Dotum", "sans-serif"});
    fontFallbacks["ar"] = FontFallback("Tahoma", {"Arial Unicode MS", "sans-serif"});
}

std::string LocalizationManager::handleMissingString(const std::string& key) const {
    // In development, you might want to return a visible indicator
    // In production, might return empty string or log the missing key
    return "[MISSING: " + key + "]";
}

// -- LocalizationUtils implementation
namespace LocalizationUtils {

std::string normalizeLanguageCode(const std::string& code) {
    std::string normalized = code;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Convert underscores to hyphens
    std::replace(normalized.begin(), normalized.end(), '_', '-');
    
    return normalized;
}

std::string getLanguageFromLocale(const std::string& locale) {
    size_t hyphenPos = locale.find('-');
    return (hyphenPos != std::string::npos) ? locale.substr(0, hyphenPos) : locale;
}

std::string getRegionFromLocale(const std::string& locale) {
    size_t hyphenPos = locale.find('-');
    return (hyphenPos != std::string::npos && hyphenPos + 1 < locale.length()) ? 
           locale.substr(hyphenPos + 1) : "";
}

bool isValidLanguageCode(const std::string& code) {
    // Basic validation - should be 2-5 characters, optionally with region
    if (code.length() < 2 || code.length() > 10) return false;
    
    // Check format: language[-region]
    size_t hyphenPos = code.find('-');
    if (hyphenPos != std::string::npos) {
        return hyphenPos >= 2 && hyphenPos + 3 <= code.length();
    }
    
    return code.length() >= 2 && code.length() <= 3;
}

bool isRTLLanguage(const std::string& languageCode) {
    // Common RTL languages
    std::vector<std::string> rtlLanguages = {"ar", "he", "fa", "ur", "yi"};
    std::string lang = getLanguageFromLocale(languageCode);
    return std::find(rtlLanguages.begin(), rtlLanguages.end(), lang) != rtlLanguages.end();
}

std::string reverseRTLText(const std::string& text) {
    // Simple reversal - real implementation would handle bidi algorithm
    std::string reversed = text;
    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}

std::string normalizeUnicode(const std::string& text) {
    // Unicode normalization would go here - placeholder implementation
    return text;
}

size_t getDisplayStringLength(const std::string& utf8String) {
    // Count actual characters, not bytes (simplified)
    size_t count = 0;
    for (size_t i = 0; i < utf8String.length(); ++i) {
        if ((utf8String[i] & 0xC0) != 0x80) ++count;
    }
    return count;
}

std::string getTranslationFilePath(const std::string& baseDir, const std::string& languageCode, 
                                  TranslationFormat format) {
    std::string extension;
    switch (format) {
        case TranslationFormat::JSON: extension = ".json"; break;
        case TranslationFormat::CSV: extension = ".csv"; break;
        case TranslationFormat::XML: extension = ".xml"; break;
        case TranslationFormat::PO: extension = ".po"; break;
        case TranslationFormat::Properties: extension = ".properties"; break;
        case TranslationFormat::YAML: extension = ".yaml"; break;
        default: extension = ".txt"; break;
    }
    
    return baseDir + "/" + languageCode + extension;
}

void generateTranslationTemplate(const std::vector<std::string>& keys, 
                                const std::string& outputPath, 
                                TranslationFormat format) {
    std::unordered_map<std::string, LocalizedString> strings;
    for (const auto& key : keys) {
        strings[key] = LocalizedString(key, "[TODO: Translate]", "");
    }
    
    auto manager = LocalizationManager::getInstance();
    if (format == TranslationFormat::JSON) {
        JSONTranslationLoader loader;
        loader.saveToFile(outputPath, strings);
    } else if (format == TranslationFormat::CSV) {
        CSVTranslationLoader loader;
        loader.saveToFile(outputPath, strings);
    }
}

} // namespace LocalizationUtils

} // namespace Localization
} // namespace JJM