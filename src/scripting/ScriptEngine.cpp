#include "scripting/ScriptEngine.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Mock Lua implementation (in real implementation, you'd link with lua library)
struct MockLuaState {
    std::map<std::string, JJM::Scripting::ScriptValue> globals;
    std::vector<JJM::Scripting::ScriptValue> stack;
};

namespace JJM {
namespace Scripting {

// ScriptValue implementation
ScriptValue::ScriptValue() : type(NIL) {}

ScriptValue::ScriptValue(bool value) : type(BOOLEAN), boolValue(value) {}

ScriptValue::ScriptValue(int value) : type(NUMBER), numberValue(static_cast<double>(value)) {}

ScriptValue::ScriptValue(double value) : type(NUMBER), numberValue(value) {}

ScriptValue::ScriptValue(const std::string& value) : type(STRING), stringValue(value) {}

ScriptValue::ScriptValue(const char* value) : type(STRING), stringValue(value) {}

bool ScriptValue::asBool() const {
    switch (type) {
        case BOOLEAN: return boolValue;
        case NUMBER: return numberValue != 0.0;
        case STRING: return !stringValue.empty();
        default: return false;
    }
}

int ScriptValue::asInt() const {
    switch (type) {
        case NUMBER: return static_cast<int>(numberValue);
        case BOOLEAN: return boolValue ? 1 : 0;
        case STRING: return std::stoi(stringValue);
        default: return 0;
    }
}

double ScriptValue::asDouble() const {
    switch (type) {
        case NUMBER: return numberValue;
        case BOOLEAN: return boolValue ? 1.0 : 0.0;
        case STRING: return std::stod(stringValue);
        default: return 0.0;
    }
}

std::string ScriptValue::asString() const {
    switch (type) {
        case STRING: return stringValue;
        case NUMBER: return std::to_string(numberValue);
        case BOOLEAN: return boolValue ? "true" : "false";
        case NIL: return "nil";
        default: return "";
    }
}

void ScriptValue::setTableValue(const std::string& key, const ScriptValue& value) {
    if (type != TABLE) {
        type = TABLE;
        tableValue.clear();
    }
    tableValue[key] = value;
}

ScriptValue ScriptValue::getTableValue(const std::string& key) const {
    if (type == TABLE) {
        auto it = tableValue.find(key);
        if (it != tableValue.end()) {
            return it->second;
        }
    }
    return ScriptValue();
}

bool ScriptValue::hasTableKey(const std::string& key) const {
    if (type == TABLE) {
        return tableValue.find(key) != tableValue.end();
    }
    return false;
}

// ScriptFunction implementation
ScriptFunction::ScriptFunction(const std::string& name, CppFunction func)
    : name(name), function(func) {}

ScriptValue ScriptFunction::call(const std::vector<ScriptValue>& args) const {
    if (function) {
        return function(args);
    }
    return ScriptValue();
}

// ScriptEngine implementation
ScriptEngine::ScriptEngine(ScriptType type)
    : engineType(type), luaState(nullptr), initialized(false) {}

ScriptEngine::~ScriptEngine() {
    shutdown();
}

bool ScriptEngine::initialize() {
    if (initialized) return true;
    
    switch (engineType) {
        case ScriptType::LUA:
            return initializeLua();
        default:
            std::cerr << "Unsupported script engine type" << std::endl;
            return false;
    }
}

void ScriptEngine::shutdown() {
    if (!initialized) return;
    
    switch (engineType) {
        case ScriptType::LUA:
            shutdownLua();
            break;
        default:
            break;
    }
    
    initialized = false;
}

bool ScriptEngine::executeString(const std::string& script) {
    if (!initialized) return false;
    
    switch (engineType) {
        case ScriptType::LUA:
            return executeLuaString(script);
        default:
            return false;
    }
}

bool ScriptEngine::executeFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open script file: " << filename << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return executeString(buffer.str());
}

void ScriptEngine::registerFunction(const std::string& name, ScriptFunction::CppFunction function) {
    registeredFunctions[name] = ScriptFunction(name, function);
    
    if (initialized) {
        switch (engineType) {
            case ScriptType::LUA:
                registerLuaFunction(name, function);
                break;
            default:
                break;
        }
    }
}

void ScriptEngine::setGlobalVariable(const std::string& name, const ScriptValue& value) {
    globalVariables[name] = value;
    
    if (initialized) {
        switch (engineType) {
            case ScriptType::LUA:
                setLuaGlobalVariable(name, value);
                break;
            default:
                break;
        }
    }
}

ScriptValue ScriptEngine::getGlobalVariable(const std::string& name) {
    if (initialized) {
        switch (engineType) {
            case ScriptType::LUA:
                return getLuaGlobalVariable(name);
            default:
                break;
        }
    }
    
    auto it = globalVariables.find(name);
    if (it != globalVariables.end()) {
        return it->second;
    }
    
    return ScriptValue();
}

ScriptValue ScriptEngine::callFunction(const std::string& name, const std::vector<ScriptValue>& args) {
    if (!initialized) return ScriptValue();
    
    switch (engineType) {
        case ScriptType::LUA:
            return callLuaFunction(name, args);
        default:
            break;
    }
    
    auto it = registeredFunctions.find(name);
    if (it != registeredFunctions.end()) {
        return it->second.call(args);
    }
    
    return ScriptValue();
}

void ScriptEngine::collectGarbage() {
    // In real Lua implementation: lua_gc(L, LUA_GCCOLLECT, 0);
    // For mock implementation, this is a no-op
}

// Mock Lua implementation
bool ScriptEngine::initializeLua() {
    luaState = new MockLuaState();
    initialized = true;
    
    // Register all pending functions and variables
    for (const auto& pair : registeredFunctions) {
        registerLuaFunction(pair.first, pair.second.getFunction());
    }
    
    for (const auto& pair : globalVariables) {
        setLuaGlobalVariable(pair.first, pair.second);
    }
    
    return true;
}

void ScriptEngine::shutdownLua() {
    if (luaState) {
        delete static_cast<MockLuaState*>(luaState);
        luaState = nullptr;
    }
}

bool ScriptEngine::executeLuaString(const std::string& script) {
    // Mock execution - in real implementation, use luaL_dostring
    std::cout << "Executing Lua script: " << script << std::endl;
    
    // Simple variable assignment parsing
    if (script.find("=") != std::string::npos) {
        size_t pos = script.find("=");
        std::string varName = script.substr(0, pos);
        std::string varValue = script.substr(pos + 1);
        
        // Trim whitespace
        varName.erase(0, varName.find_first_not_of(" \t"));
        varName.erase(varName.find_last_not_of(" \t") + 1);
        varValue.erase(0, varValue.find_first_not_of(" \t"));
        varValue.erase(varValue.find_last_not_of(" \t") + 1);
        
        // Parse value
        ScriptValue value;
        if (varValue == "true") {
            value = ScriptValue(true);
        } else if (varValue == "false") {
            value = ScriptValue(false);
        } else if (varValue[0] == '"' && varValue.back() == '"') {
            value = ScriptValue(varValue.substr(1, varValue.length() - 2));
        } else {
            try {
                double num = std::stod(varValue);
                value = ScriptValue(num);
            } catch (...) {
                value = ScriptValue(varValue);
            }
        }
        
        setGlobalVariable(varName, value);
    }
    
    return true;
}

bool ScriptEngine::executeLuaFile(const std::string& filename) {
    return executeFile(filename);
}

void ScriptEngine::registerLuaFunction(const std::string& name, ScriptFunction::CppFunction function) {
    // In real implementation, register C function with Lua
    registeredFunctions[name] = ScriptFunction(name, function);
}

void ScriptEngine::setLuaGlobalVariable(const std::string& name, const ScriptValue& value) {
    if (luaState) {
        MockLuaState* state = static_cast<MockLuaState*>(luaState);
        state->globals[name] = value;
    }
}

ScriptValue ScriptEngine::getLuaGlobalVariable(const std::string& name) {
    if (luaState) {
        MockLuaState* state = static_cast<MockLuaState*>(luaState);
        auto it = state->globals.find(name);
        if (it != state->globals.end()) {
            return it->second;
        }
    }
    return ScriptValue();
}

ScriptValue ScriptEngine::callLuaFunction(const std::string& name, const std::vector<ScriptValue>& args) {
    auto it = registeredFunctions.find(name);
    if (it != registeredFunctions.end()) {
        return it->second.call(args);
    }
    return ScriptValue();
}

void ScriptEngine::pushValueToLua(const ScriptValue& value) {
    // Mock implementation
}

ScriptValue ScriptEngine::getValueFromLua(int index) {
    // Mock implementation
    return ScriptValue();
}

int ScriptEngine::luaFunctionWrapper(void* L) {
    // Mock implementation
    return 0;
}

// ScriptManager implementation
ScriptManager* ScriptManager::instance = nullptr;

ScriptManager::ScriptManager() : defaultEngine(nullptr) {}

ScriptManager* ScriptManager::getInstance() {
    if (!instance) {
        instance = new ScriptManager();
    }
    return instance;
}

ScriptManager::~ScriptManager() {
    shutdown();
}

ScriptEngine* ScriptManager::createEngine(const std::string& name, ScriptType type) {
    auto engine = std::make_unique<ScriptEngine>(type);
    if (engine->initialize()) {
        ScriptEngine* ptr = engine.get();
        engines[name] = std::move(engine);
        
        if (!defaultEngine) {
            defaultEngine = ptr;
        }
        
        return ptr;
    }
    
    return nullptr;
}

ScriptEngine* ScriptManager::getEngine(const std::string& name) {
    auto it = engines.find(name);
    if (it != engines.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ScriptManager::removeEngine(const std::string& name) {
    auto it = engines.find(name);
    if (it != engines.end()) {
        if (defaultEngine == it->second.get()) {
            defaultEngine = nullptr;
        }
        engines.erase(it);
    }
}

void ScriptManager::setDefaultEngine(const std::string& name) {
    auto* engine = getEngine(name);
    if (engine) {
        defaultEngine = engine;
    }
}

bool ScriptManager::executeScript(const std::string& script, const std::string& engineName) {
    ScriptEngine* engine = engineName.empty() ? defaultEngine : getEngine(engineName);
    if (engine) {
        return engine->executeString(script);
    }
    return false;
}

bool ScriptManager::executeFile(const std::string& filename, const std::string& engineName) {
    ScriptEngine* engine = engineName.empty() ? defaultEngine : getEngine(engineName);
    if (engine) {
        return engine->executeFile(filename);
    }
    return false;
}

void ScriptManager::registerGlobalFunction(const std::string& name, ScriptFunction::CppFunction function) {
    for (auto& pair : engines) {
        pair.second->registerFunction(name, function);
    }
}

void ScriptManager::shutdown() {
    engines.clear();
    defaultEngine = nullptr;
}

std::vector<std::string> ScriptManager::getEngineNames() const {
    std::vector<std::string> names;
    for (const auto& pair : engines) {
        names.push_back(pair.first);
    }
    return names;
}

} // namespace Scripting
} // namespace JJM