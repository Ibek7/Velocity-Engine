#ifndef LOCALIZATION_SYSTEM_H
#define LOCALIZATION_SYSTEM_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <functional>
#include <fstream>
#include <mutex>
#include <atomic>

namespace JJM {
namespace Localization {

// Language code following ISO 639-1 (two-letter) and ISO 639-2 (three-letter) standards
struct LanguageInfo {
    std::string code;           // e.g., "en", "es", "zh-CN", "zh-TW"
    std::string name;           // e.g., "English", "Español", "中文（简体）"
    std::string nativeName;     // e.g., "English", "Español", "中文（简体）"
    std::string direction;      // "ltr" or "rtl"
    std::string family;         // e.g., "Germanic", "Romance", "Sino-Tibetan"
    bool isDefault;
    
    LanguageInfo() : direction("ltr"), isDefault(false) {}
    LanguageInfo(const std::string& code, const std::string& name, const std::string& nativeName = "", 
                 const std::string& direction = "ltr", bool isDefault = false)
        : code(code), name(name), nativeName(nativeName.empty() ? name : nativeName), 
          direction(direction), isDefault(isDefault) {}
};

// Text direction and formatting info
enum class TextDirection {
    LeftToRight,
    RightToLeft,
    Auto
};

enum class TextAlignment {
    Start,      // Respects text direction (left for LTR, right for RTL)
    End,        // Respects text direction (right for LTR, left for RTL)
    Left,       // Always left
    Right,      // Always right
    Center,
    Justify
};

// Pluralization rules - simplified implementation
enum class PluralForm {
    Zero,       // 0 items
    One,        // 1 item
    Two,        // 2 items
    Few,        // Few items (language-specific)
    Many,       // Many items (language-specific)
    Other       // Default/other cases
};

// Font fallback information
struct FontFallback {
    std::string primaryFont;
    std::vector<std::string> fallbackFonts;
    std::unordered_set<std::string> supportedLanguages;
    std::string unicodeRange;
    
    FontFallback() = default;
    FontFallback(const std::string& primary, const std::vector<std::string>& fallbacks = {})
        : primaryFont(primary), fallbackFonts(fallbacks) {}
};

// String table entry
struct LocalizedString {
    std::string key;
    std::string value;
    std::string context;        // Context for translators
    std::string comment;        // Additional comments/instructions
    bool isPlural;
    std::unordered_map<PluralForm, std::string> pluralForms;
    
    LocalizedString() : isPlural(false) {}
    LocalizedString(const std::string& key, const std::string& value, const std::string& context = "")
        : key(key), value(value), context(context), isPlural(false) {}
};

// Translation file format support
enum class TranslationFormat {
    JSON,
    CSV,
    XML,
    PO,         // GNU gettext Portable Object
    Properties,  // Java-style properties
    YAML,
    Custom
};

// String interpolation and formatting
class StringFormatter {
private:
    std::unordered_map<std::string, std::string> variables;
    
public:
    void setVariable(const std::string& name, const std::string& value);
    void setVariable(const std::string& name, int value);
    void setVariable(const std::string& name, float value);
    void setVariable(const std::string& name, double value);
    void clearVariables();
    
    std::string format(const std::string& template_str) const;
    
    // Support for positional arguments: "Hello {0}, you have {1} messages"
    std::string format(const std::string& template_str, const std::vector<std::string>& args) const;
    
    // Support for named arguments: "Hello {name}, you have {count} messages"
    std::string formatNamed(const std::string& template_str, 
                           const std::unordered_map<std::string, std::string>& namedArgs) const;
};

// Locale-specific number and date formatting
class LocaleFormatter {
private:
    std::string localeCode;
    char decimalSeparator;
    char thousandsSeparator;
    std::string currencySymbol;
    std::string dateFormat;
    std::string timeFormat;
    
public:
    LocaleFormatter(const std::string& locale = "en-US");
    
    std::string formatNumber(double number, int precision = 2) const;
    std::string formatCurrency(double amount) const;
    std::string formatPercent(double value) const;
    std::string formatDate(int year, int month, int day) const;
    std::string formatTime(int hour, int minute, int second = 0) const;
    
    void setLocale(const std::string& locale);
    std::string getLocale() const { return localeCode; }
};

// File loader interface for different translation formats
class TranslationLoader {
public:
    virtual ~TranslationLoader() = default;
    virtual bool loadFromFile(const std::string& filePath, 
                             std::unordered_map<std::string, LocalizedString>& strings) = 0;
    virtual bool saveToFile(const std::string& filePath, 
                           const std::unordered_map<std::string, LocalizedString>& strings) = 0;
    virtual TranslationFormat getFormat() const = 0;
};

// Concrete loaders for different formats
class JSONTranslationLoader : public TranslationLoader {
public:
    bool loadFromFile(const std::string& filePath, 
                     std::unordered_map<std::string, LocalizedString>& strings) override;
    bool saveToFile(const std::string& filePath, 
                   const std::unordered_map<std::string, LocalizedString>& strings) override;
    TranslationFormat getFormat() const override { return TranslationFormat::JSON; }
};

class CSVTranslationLoader : public TranslationLoader {
public:
    bool loadFromFile(const std::string& filePath, 
                     std::unordered_map<std::string, LocalizedString>& strings) override;
    bool saveToFile(const std::string& filePath, 
                   const std::unordered_map<std::string, LocalizedString>& strings) override;
    TranslationFormat getFormat() const override { return TranslationFormat::CSV; }
};

// Language detection and fallback system
class LanguageDetector {
private:
    std::vector<std::string> preferredLanguages;
    std::unordered_map<std::string, LanguageInfo> availableLanguages;
    
public:
    void addPreferredLanguage(const std::string& languageCode);
    void setPreferredLanguages(const std::vector<std::string>& languages);
    void addAvailableLanguage(const LanguageInfo& language);
    
    std::string detectBestLanguage() const;
    std::vector<std::string> getFallbackChain(const std::string& preferredLanguage) const;
    
    // System locale detection (platform-specific)
    std::string getSystemLocale() const;
    std::vector<std::string> getSystemPreferredLanguages() const;
};

// Main localization manager
class LocalizationManager {
private:
    static LocalizationManager* instance;
    
    std::string currentLanguage;
    std::string fallbackLanguage;
    std::unordered_map<std::string, LanguageInfo> supportedLanguages;
    
    // String tables for each language
    std::unordered_map<std::string, std::unordered_map<std::string, LocalizedString>> stringTables;
    
    // Translation loaders
    std::unordered_map<TranslationFormat, std::unique_ptr<TranslationLoader>> loaders;
    
    // Font management
    std::unordered_map<std::string, FontFallback> fontFallbacks;
    
    // Formatters
    std::unique_ptr<StringFormatter> stringFormatter;
    std::unique_ptr<LocaleFormatter> localeFormatter;
    std::unique_ptr<LanguageDetector> languageDetector;
    
    // Thread safety
    mutable std::mutex stringTableMutex;
    std::atomic<bool> initialized;
    
    // Callbacks
    std::vector<std::function<void(const std::string&, const std::string&)>> languageChangeCallbacks;
    
    LocalizationManager();
    
public:
    static LocalizationManager* getInstance();
    ~LocalizationManager();
    
    // Initialization
    bool initialize(const std::string& defaultLanguage = "en");
    void shutdown();
    
    // Language management
    bool loadLanguage(const std::string& languageCode, const std::string& filePath, 
                     TranslationFormat format = TranslationFormat::JSON);
    bool addLanguage(const LanguageInfo& language);
    bool setCurrentLanguage(const std::string& languageCode);
    void setFallbackLanguage(const std::string& languageCode);
    
    std::string getCurrentLanguage() const { return currentLanguage; }
    std::string getFallbackLanguage() const { return fallbackLanguage; }
    std::vector<LanguageInfo> getSupportedLanguages() const;
    
    bool isLanguageSupported(const std::string& languageCode) const;
    LanguageInfo getLanguageInfo(const std::string& languageCode) const;
    
    // String retrieval and formatting
    std::string getString(const std::string& key) const;
    std::string getString(const std::string& key, const std::string& defaultValue) const;
    std::string getFormattedString(const std::string& key, const std::vector<std::string>& args) const;
    std::string getFormattedString(const std::string& key, 
                                  const std::unordered_map<std::string, std::string>& namedArgs) const;
    
    // Pluralization support
    std::string getPluralString(const std::string& key, int count) const;
    std::string getFormattedPluralString(const std::string& key, int count, 
                                        const std::vector<std::string>& args) const;
    
    // String management (for editors/tools)
    bool addString(const std::string& languageCode, const LocalizedString& locString);
    bool updateString(const std::string& languageCode, const std::string& key, const std::string& value);
    bool removeString(const std::string& languageCode, const std::string& key);
    void clearStrings(const std::string& languageCode);
    
    // File operations
    bool exportLanguage(const std::string& languageCode, const std::string& filePath, 
                       TranslationFormat format = TranslationFormat::JSON);
    bool reloadLanguage(const std::string& languageCode);
    bool reloadCurrentLanguage();
    
    // Font fallback system
    void addFontFallback(const std::string& languageCode, const FontFallback& fallback);
    FontFallback getFontFallback(const std::string& languageCode) const;
    std::string selectFontForLanguage(const std::string& languageCode) const;
    std::vector<std::string> getFontFallbackChain(const std::string& languageCode) const;
    
    // Text direction and formatting
    TextDirection getTextDirection(const std::string& languageCode = "") const;
    TextAlignment getDefaultAlignment(const std::string& languageCode = "") const;
    
    // Locale-specific formatting
    LocaleFormatter* getLocaleFormatter() { return localeFormatter.get(); }
    StringFormatter* getStringFormatter() { return stringFormatter.get(); }
    
    // Language detection and auto-switching
    void enableAutoDetection(bool enable = true);
    std::string detectBestLanguage() const;
    bool switchToDetectedLanguage();
    
    // Callbacks for language change events
    void addLanguageChangeCallback(std::function<void(const std::string&, const std::string&)> callback);
    void removeLanguageChangeCallbacks();
    
    // Debugging and statistics
    size_t getStringCount(const std::string& languageCode = "") const;
    std::vector<std::string> getMissingStrings(const std::string& languageCode = "") const;
    std::vector<std::string> getUnusedStrings(const std::string& languageCode = "") const;
    void logLanguageStats() const;
    
    // Hot reloading for development
    void enableHotReload(const std::string& watchDirectory);
    void disableHotReload();
    
    // Singleton pattern
    LocalizationManager(const LocalizationManager&) = delete;
    LocalizationManager& operator=(const LocalizationManager&) = delete;

private:
    std::string findStringInternal(const std::string& key) const;
    PluralForm getPluralForm(int count, const std::string& languageCode) const;
    void registerDefaultLoaders();
    void notifyLanguageChange(const std::string& oldLanguage, const std::string& newLanguage);
    void loadDefaultFontFallbacks();
    
    // Helper for missing string handling
    std::string handleMissingString(const std::string& key) const;
};

// Utility functions
namespace LocalizationUtils {
    // Language code utilities
    std::string normalizeLanguageCode(const std::string& code);
    std::string getLanguageFromLocale(const std::string& locale);
    std::string getRegionFromLocale(const std::string& locale);
    bool isValidLanguageCode(const std::string& code);
    
    // Unicode and text utilities
    bool isRTLLanguage(const std::string& languageCode);
    std::string reverseRTLText(const std::string& text);
    std::string normalizeUnicode(const std::string& text);
    size_t getDisplayStringLength(const std::string& utf8String);
    
    // File and path utilities
    std::string getTranslationFilePath(const std::string& baseDir, const std::string& languageCode, 
                                      TranslationFormat format);
    std::vector<std::string> findTranslationFiles(const std::string& directory);
    
    // Development helpers
    void generateTranslationTemplate(const std::vector<std::string>& keys, 
                                    const std::string& outputPath, 
                                    TranslationFormat format = TranslationFormat::JSON);
    void compareTranslations(const std::string& referenceFile, const std::string& targetFile);
}

// Convenience macros for easier usage
#define LOC(key) JJM::Localization::LocalizationManager::getInstance()->getString(key)
#define LOC_DEF(key, def) JJM::Localization::LocalizationManager::getInstance()->getString(key, def)
#define LOC_FMT(key, ...) JJM::Localization::LocalizationManager::getInstance()->getFormattedString(key, {__VA_ARGS__})
#define LOC_PLURAL(key, count) JJM::Localization::LocalizationManager::getInstance()->getPluralString(key, count)

} // namespace Localization
} // namespace JJM

#endif // LOCALIZATION_SYSTEM_H