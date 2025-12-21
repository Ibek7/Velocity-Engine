#include "debug/CommandRegistry.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace JJM {
namespace Debug {

CommandRegistry& CommandRegistry::getInstance() {
    static CommandRegistry instance;
    return instance;
}

CommandRegistry::CommandRegistry() {
    registerCommand("help", "Display help information", "help [command]",
        [this](const std::vector<std::string>& args) {
            if (args.empty()) {
                std::cout << "Available commands:\n";
                for (const auto& pair : commands) {
                    std::cout << "  " << pair.first << " - " << pair.second.description << "\n";
                }
            } else {
                ConsoleCommand* cmd = getCommand(args[0]);
                if (cmd) {
                    std::cout << cmd->name << ": " << cmd->description << "\n";
                    std::cout << "Usage: " << cmd->usage << "\n";
                } else {
                    std::cout << "Unknown command: " << args[0] << "\n";
                }
            }
        }, 0, 1);
}

CommandRegistry::~CommandRegistry() {
}

void CommandRegistry::registerCommand(const std::string& name, const std::string& description,
                                     const std::string& usage, CommandCallback callback,
                                     int minArgs, int maxArgs) {
    ConsoleCommand cmd;
    cmd.name = name;
    cmd.description = description;
    cmd.usage = usage;
    cmd.callback = callback;
    cmd.minArgs = minArgs;
    cmd.maxArgs = maxArgs;
    
    commands[name] = cmd;
}

bool CommandRegistry::executeCommand(const std::string& commandLine) {
    if (commandLine.empty()) return false;
    
    std::vector<std::string> tokens = parseCommandLine(commandLine);
    if (tokens.empty()) return false;
    
    std::string cmdName = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    
    auto it = commands.find(cmdName);
    if (it == commands.end()) {
        std::cout << "Unknown command: " << cmdName << "\n";
        return false;
    }
    
    ConsoleCommand& cmd = it->second;
    
    int argCount = static_cast<int>(args.size());
    if (argCount < cmd.minArgs) {
        std::cout << "Too few arguments. Usage: " << cmd.usage << "\n";
        return false;
    }
    
    if (cmd.maxArgs >= 0 && argCount > cmd.maxArgs) {
        std::cout << "Too many arguments. Usage: " << cmd.usage << "\n";
        return false;
    }
    
    cmd.callback(args);
    return true;
}

std::vector<ConsoleCommand*> CommandRegistry::getAllCommands() {
    std::vector<ConsoleCommand*> result;
    for (auto& pair : commands) {
        result.push_back(&pair.second);
    }
    return result;
}

ConsoleCommand* CommandRegistry::getCommand(const std::string& name) {
    auto it = commands.find(name);
    return it != commands.end() ? &it->second : nullptr;
}

std::vector<std::string> CommandRegistry::getCommandSuggestions(const std::string& prefix) {
    std::vector<std::string> suggestions;
    for (const auto& pair : commands) {
        if (pair.first.find(prefix) == 0) {
            suggestions.push_back(pair.first);
        }
    }
    return suggestions;
}

std::vector<std::string> CommandRegistry::parseCommandLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

} // namespace Debug
} // namespace JJM
