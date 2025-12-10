#include "debug/DebugConsole.h"
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iostream>

namespace JJM {
namespace Debug {

// ConsoleCommand implementation
ConsoleCommand::ConsoleCommand(const std::string& name, const std::string& description,
                              CommandFunc func)
    : name(name), description(description), function(func) {}

ConsoleCommand::~ConsoleCommand() {}

void ConsoleCommand::execute(const std::vector<std::string>& args) const {
    if (function) {
        function(args);
    }
}

// DebugConsole implementation
DebugConsole::DebugConsole()
    : visible(false), maxMessages(1000), minLogLevel(ConsoleLogLevel::Trace),
      currentTime(0.0), currentInput(""), historyIndex(0) {
    initializeBuiltinCommands();
}

DebugConsole::~DebugConsole() {}

DebugConsole& DebugConsole::getInstance() {
    static DebugConsole instance;
    return instance;
}

void DebugConsole::log(const std::string& message, ConsoleLogLevel level) {
    if (level < minLogLevel) return;
    
    addMessage(message, level);
}

void DebugConsole::logTrace(const std::string& message) {
    log(message, ConsoleLogLevel::Trace);
}

void DebugConsole::logDebug(const std::string& message) {
    log(message, ConsoleLogLevel::Debug);
}

void DebugConsole::logInfo(const std::string& message) {
    log(message, ConsoleLogLevel::Info);
}

void DebugConsole::logWarning(const std::string& message) {
    log(message, ConsoleLogLevel::Warning);
}

void DebugConsole::logError(const std::string& message) {
    log(message, ConsoleLogLevel::Error);
}

void DebugConsole::logFatal(const std::string& message) {
    log(message, ConsoleLogLevel::Fatal);
}

void DebugConsole::registerCommand(const std::string& name, const std::string& description,
                                  ConsoleCommand::CommandFunc func) {
    commands.emplace(name, ConsoleCommand(name, description, func));
}

void DebugConsole::executeCommand(const std::string& commandLine) {
    if (commandLine.empty()) return;
    
    commandHistory.push_back(commandLine);
    
    auto parts = parseCommandLine(commandLine);
    if (parts.empty()) return;
    
    std::string commandName = parts[0];
    std::vector<std::string> args(parts.begin() + 1, parts.end());
    
    auto it = commands.find(commandName);
    if (it != commands.end()) {
        it->second.execute(args);
    } else {
        logError("Unknown command: " + commandName);
    }
}

void DebugConsole::clear() {
    messages.clear();
}

void DebugConsole::render() {
    if (!visible) return;
    
    // Rendering would be done by the graphics system
    std::cout << "=== Debug Console ===" << std::endl;
    
    for (const auto& msg : messages) {
        std::cout << "[" << static_cast<int>(msg.level) << "] " 
                  << msg.text << std::endl;
    }
}

void DebugConsole::update(float deltaTime) {
    currentTime += deltaTime;
}

void DebugConsole::addMessage(const std::string& text, ConsoleLogLevel level) {
    messages.emplace_back(text, level, currentTime);
    
    while (messages.size() > maxMessages) {
        messages.pop_front();
    }
}

std::vector<std::string> DebugConsole::parseCommandLine(const std::string& commandLine) {
    std::vector<std::string> parts;
    std::stringstream ss(commandLine);
    std::string part;
    
    while (ss >> part) {
        parts.push_back(part);
    }
    
    return parts;
}

void DebugConsole::initializeBuiltinCommands() {
    registerCommand("help", "Show all available commands", 
        [this](const std::vector<std::string>& args) {
            logInfo("Available commands:");
            for (const auto& pair : commands) {
                logInfo("  " + pair.first + " - " + pair.second.getDescription());
            }
        });
    
    registerCommand("clear", "Clear the console",
        [this](const std::vector<std::string>& args) {
            clear();
        });
    
    registerCommand("echo", "Echo a message",
        [this](const std::vector<std::string>& args) {
            std::string message;
            for (const auto& arg : args) {
                message += arg + " ";
            }
            logInfo(message);
        });
}

// ConsoleVariable implementation
ConsoleVariable::ConsoleVariable(const std::string& name, int value)
    : name(name), type(Type::Int), intValue(value),
      floatValue(0.0f), stringValue(""), boolValue(false) {}

ConsoleVariable::ConsoleVariable(const std::string& name, float value)
    : name(name), type(Type::Float), intValue(0),
      floatValue(value), stringValue(""), boolValue(false) {}

ConsoleVariable::ConsoleVariable(const std::string& name, const std::string& value)
    : name(name), type(Type::String), intValue(0),
      floatValue(0.0f), stringValue(value), boolValue(false) {}

ConsoleVariable::ConsoleVariable(const std::string& name, bool value)
    : name(name), type(Type::Bool), intValue(0),
      floatValue(0.0f), stringValue(""), boolValue(value) {}

ConsoleVariable::~ConsoleVariable() {}

void ConsoleVariable::setInt(int value) {
    if (type == Type::Int) {
        intValue = value;
    }
}

void ConsoleVariable::setFloat(float value) {
    if (type == Type::Float) {
        floatValue = value;
    }
}

void ConsoleVariable::setString(const std::string& value) {
    if (type == Type::String) {
        stringValue = value;
    }
}

void ConsoleVariable::setBool(bool value) {
    if (type == Type::Bool) {
        boolValue = value;
    }
}

std::string ConsoleVariable::toString() const {
    switch (type) {
        case Type::Int:
            return std::to_string(intValue);
        case Type::Float:
            return std::to_string(floatValue);
        case Type::String:
            return stringValue;
        case Type::Bool:
            return boolValue ? "true" : "false";
    }
    return "";
}

// ConsoleVariableManager implementation
ConsoleVariableManager& ConsoleVariableManager::getInstance() {
    static ConsoleVariableManager instance;
    return instance;
}

void ConsoleVariableManager::registerVariable(const std::string& name, int value) {
    variables.emplace(name, ConsoleVariable(name, value));
}

void ConsoleVariableManager::registerVariable(const std::string& name, float value) {
    variables.emplace(name, ConsoleVariable(name, value));
}

void ConsoleVariableManager::registerVariable(const std::string& name, const std::string& value) {
    variables.emplace(name, ConsoleVariable(name, value));
}

void ConsoleVariableManager::registerVariable(const std::string& name, bool value) {
    variables.emplace(name, ConsoleVariable(name, value));
}

ConsoleVariable* ConsoleVariableManager::getVariable(const std::string& name) {
    auto it = variables.find(name);
    if (it != variables.end()) {
        return &it->second;
    }
    return nullptr;
}

void ConsoleVariableManager::setValue(const std::string& name, int value) {
    auto* var = getVariable(name);
    if (var) var->setInt(value);
}

void ConsoleVariableManager::setValue(const std::string& name, float value) {
    auto* var = getVariable(name);
    if (var) var->setFloat(value);
}

void ConsoleVariableManager::setValue(const std::string& name, const std::string& value) {
    auto* var = getVariable(name);
    if (var) var->setString(value);
}

void ConsoleVariableManager::setValue(const std::string& name, bool value) {
    auto* var = getVariable(name);
    if (var) var->setBool(value);
}

int ConsoleVariableManager::getInt(const std::string& name, int defaultValue) {
    auto* var = getVariable(name);
    return var ? var->getInt() : defaultValue;
}

float ConsoleVariableManager::getFloat(const std::string& name, float defaultValue) {
    auto* var = getVariable(name);
    return var ? var->getFloat() : defaultValue;
}

std::string ConsoleVariableManager::getString(const std::string& name, const std::string& defaultValue) {
    auto* var = getVariable(name);
    return var ? var->getString() : defaultValue;
}

bool ConsoleVariableManager::getBool(const std::string& name, bool defaultValue) {
    auto* var = getVariable(name);
    return var ? var->getBool() : defaultValue;
}

// ConsoleAutoComplete implementation
ConsoleAutoComplete::ConsoleAutoComplete() {}

ConsoleAutoComplete::~ConsoleAutoComplete() {}

void ConsoleAutoComplete::addCommand(const std::string& command) {
    commands.push_back(command);
}

void ConsoleAutoComplete::addVariable(const std::string& variable) {
    variables.push_back(variable);
}

std::vector<std::string> ConsoleAutoComplete::getSuggestions(const std::string& input) const {
    std::vector<std::string> suggestions;
    
    for (const auto& cmd : commands) {
        if (cmd.find(input) == 0) {
            suggestions.push_back(cmd);
        }
    }
    
    for (const auto& var : variables) {
        if (var.find(input) == 0) {
            suggestions.push_back(var);
        }
    }
    
    return suggestions;
}

std::string ConsoleAutoComplete::getCompletion(const std::string& input) const {
    auto suggestions = getSuggestions(input);
    
    if (suggestions.empty()) {
        return input;
    }
    
    return suggestions[0];
}

} // namespace Debug
} // namespace JJM
