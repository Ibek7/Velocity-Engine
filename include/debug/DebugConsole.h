#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <deque>

namespace JJM {
namespace Debug {

enum class ConsoleLogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

struct ConsoleMessage {
    std::string text;
    ConsoleLogLevel level;
    double timestamp;
    
    ConsoleMessage() : text(""), level(ConsoleLogLevel::Info), timestamp(0.0) {}
    ConsoleMessage(const std::string& text, ConsoleLogLevel level, double timestamp)
        : text(text), level(level), timestamp(timestamp) {}
};

class ConsoleCommand {
public:
    using CommandFunc = std::function<void(const std::vector<std::string>&)>;
    
    ConsoleCommand(const std::string& name, const std::string& description, CommandFunc func);
    ~ConsoleCommand();
    
    const std::string& getName() const { return name; }
    const std::string& getDescription() const { return description; }
    
    void execute(const std::vector<std::string>& args) const;

private:
    std::string name;
    std::string description;
    CommandFunc function;
};

class DebugConsole {
public:
    DebugConsole();
    ~DebugConsole();
    
    static DebugConsole& getInstance();
    
    void log(const std::string& message, ConsoleLogLevel level = ConsoleLogLevel::Info);
    void logTrace(const std::string& message);
    void logDebug(const std::string& message);
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
    void logFatal(const std::string& message);
    
    void registerCommand(const std::string& name, const std::string& description,
                        ConsoleCommand::CommandFunc func);
    
    void executeCommand(const std::string& commandLine);
    
    void setVisible(bool visible) { this->visible = visible; }
    bool isVisible() const { return visible; }
    void toggle() { visible = !visible; }
    
    void clear();
    
    const std::deque<ConsoleMessage>& getMessages() const { return messages; }
    const std::vector<std::string>& getCommandHistory() const { return commandHistory; }
    
    void setMaxMessages(size_t max) { maxMessages = max; }
    void setLogLevel(ConsoleLogLevel level) { minLogLevel = level; }
    
    void render();
    void update(float deltaTime);

private:
    bool visible;
    std::deque<ConsoleMessage> messages;
    std::unordered_map<std::string, ConsoleCommand> commands;
    std::vector<std::string> commandHistory;
    size_t maxMessages;
    ConsoleLogLevel minLogLevel;
    double currentTime;
    
    std::string currentInput;
    size_t historyIndex;
    
    void addMessage(const std::string& text, ConsoleLogLevel level);
    std::vector<std::string> parseCommandLine(const std::string& commandLine);
    void initializeBuiltinCommands();
};

class ConsoleVariable {
public:
    enum class Type {
        Int,
        Float,
        String,
        Bool
    };
    
    ConsoleVariable(const std::string& name, int value);
    ConsoleVariable(const std::string& name, float value);
    ConsoleVariable(const std::string& name, const std::string& value);
    ConsoleVariable(const std::string& name, bool value);
    ~ConsoleVariable();
    
    const std::string& getName() const { return name; }
    Type getType() const { return type; }
    
    int getInt() const { return intValue; }
    float getFloat() const { return floatValue; }
    const std::string& getString() const { return stringValue; }
    bool getBool() const { return boolValue; }
    
    void setInt(int value);
    void setFloat(float value);
    void setString(const std::string& value);
    void setBool(bool value);
    
    std::string toString() const;

private:
    std::string name;
    Type type;
    
    int intValue;
    float floatValue;
    std::string stringValue;
    bool boolValue;
};

class ConsoleVariableManager {
public:
    static ConsoleVariableManager& getInstance();
    
    void registerVariable(const std::string& name, int value);
    void registerVariable(const std::string& name, float value);
    void registerVariable(const std::string& name, const std::string& value);
    void registerVariable(const std::string& name, bool value);
    
    ConsoleVariable* getVariable(const std::string& name);
    
    void setValue(const std::string& name, int value);
    void setValue(const std::string& name, float value);
    void setValue(const std::string& name, const std::string& value);
    void setValue(const std::string& name, bool value);
    
    int getInt(const std::string& name, int defaultValue = 0);
    float getFloat(const std::string& name, float defaultValue = 0.0f);
    std::string getString(const std::string& name, const std::string& defaultValue = "");
    bool getBool(const std::string& name, bool defaultValue = false);

private:
    ConsoleVariableManager() = default;
    std::unordered_map<std::string, ConsoleVariable> variables;
};

class ConsoleAutoComplete {
public:
    ConsoleAutoComplete();
    ~ConsoleAutoComplete();
    
    void addCommand(const std::string& command);
    void addVariable(const std::string& variable);
    
    std::vector<std::string> getSuggestions(const std::string& input) const;
    std::string getCompletion(const std::string& input) const;

private:
    std::vector<std::string> commands;
    std::vector<std::string> variables;
};

} // namespace Debug
} // namespace JJM
