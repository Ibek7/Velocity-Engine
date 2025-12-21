#ifndef JJM_CONSOLE_COMMANDS_H
#define JJM_CONSOLE_COMMANDS_H

#include <string>
#include <map>
#include <vector>
#include <functional>

namespace JJM {
namespace Debug {

using CommandCallback = std::function<void(const std::vector<std::string>&)>;

struct ConsoleCommand {
    std::string name;
    std::string description;
    std::string usage;
    CommandCallback callback;
    int minArgs;
    int maxArgs;
};

class CommandRegistry {
public:
    static CommandRegistry& getInstance();
    
    void registerCommand(const std::string& name, const std::string& description,
                        const std::string& usage, CommandCallback callback,
                        int minArgs = 0, int maxArgs = -1);
    
    bool executeCommand(const std::string& commandLine);
    
    std::vector<ConsoleCommand*> getAllCommands();
    ConsoleCommand* getCommand(const std::string& name);
    
    std::vector<std::string> getCommandSuggestions(const std::string& prefix);

private:
    CommandRegistry();
    ~CommandRegistry();
    
    std::vector<std::string> parseCommandLine(const std::string& line);
    
    std::map<std::string, ConsoleCommand> commands;
};

} // namespace Debug
} // namespace JJM

#endif
