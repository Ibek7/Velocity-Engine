#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>

namespace JJM {
namespace Scripting {

enum class ScriptType {
    LUA,
    JAVASCRIPT,
    PYTHON
};

class ScriptValue {
public:
    enum Type {
        NIL,
        BOOLEAN,
        NUMBER,
        STRING,
        TABLE
    };
    
private:
    Type type;
    union {
        bool boolValue;
        double numberValue;
    };
    std::string stringValue;
    std::map<std::string, ScriptValue> tableValue;
    
public:
    ScriptValue();
    ScriptValue(bool value);
    ScriptValue(int value);
    ScriptValue(double value);
    ScriptValue(const std::string& value);
    ScriptValue(const char* value);
    
    Type getType() const { return type; }
    
    bool asBool() const;
    int asInt() const;
    double asDouble() const;
    std::string asString() const;
    
    void setTableValue(const std::string& key, const ScriptValue& value);
    ScriptValue getTableValue(const std::string& key) const;
    bool hasTableKey(const std::string& key) const;
    
    bool isNil() const { return type == NIL; }
    bool isBool() const { return type == BOOLEAN; }
    bool isNumber() const { return type == NUMBER; }
    bool isString() const { return type == STRING; }
    bool isTable() const { return type == TABLE; }
};

class ScriptFunction {
public:
    using CppFunction = std::function<ScriptValue(const std::vector<ScriptValue>&)>;
    
private:
    std::string name;
    CppFunction function;
    
public:
    ScriptFunction(const std::string& name, CppFunction func);
    
    ScriptValue call(const std::vector<ScriptValue>& args) const;
    const std::string& getName() const { return name; }
};

class ScriptEngine {
private:
    ScriptType engineType;
    void* luaState; // lua_State* for Lua engine
    std::map<std::string, ScriptFunction> registeredFunctions;
    std::map<std::string, ScriptValue> globalVariables;
    bool initialized;
    
public:
    ScriptEngine(ScriptType type = ScriptType::LUA);
    ~ScriptEngine();
    
    bool initialize();
    void shutdown();
    
    bool executeString(const std::string& script);
    bool executeFile(const std::string& filename);
    
    void registerFunction(const std::string& name, ScriptFunction::CppFunction function);
    void setGlobalVariable(const std::string& name, const ScriptValue& value);
    ScriptValue getGlobalVariable(const std::string& name);
    
    ScriptValue callFunction(const std::string& name, const std::vector<ScriptValue>& args = {});
    
    bool isInitialized() const { return initialized; }
    ScriptType getType() const { return engineType; }
    
    void collectGarbage();
    
private:
    bool initializeLua();
    void shutdownLua();
    
    bool executeLuaString(const std::string& script);
    bool executeLuaFile(const std::string& filename);
    
    void registerLuaFunction(const std::string& name, ScriptFunction::CppFunction function);
    void setLuaGlobalVariable(const std::string& name, const ScriptValue& value);
    ScriptValue getLuaGlobalVariable(const std::string& name);
    ScriptValue callLuaFunction(const std::string& name, const std::vector<ScriptValue>& args);
    
    void pushValueToLua(const ScriptValue& value);
    ScriptValue getValueFromLua(int index);
    
    static int luaFunctionWrapper(void* L);
};

class ScriptManager {
private:
    std::map<std::string, std::unique_ptr<ScriptEngine>> engines;
    ScriptEngine* defaultEngine;
    
    static ScriptManager* instance;
    ScriptManager();
    
public:
    static ScriptManager* getInstance();
    ~ScriptManager();
    
    ScriptEngine* createEngine(const std::string& name, ScriptType type = ScriptType::LUA);
    ScriptEngine* getEngine(const std::string& name);
    void removeEngine(const std::string& name);
    
    void setDefaultEngine(const std::string& name);
    ScriptEngine* getDefaultEngine() { return defaultEngine; }
    
    bool executeScript(const std::string& script, const std::string& engineName = "");
    bool executeFile(const std::string& filename, const std::string& engineName = "");
    
    void registerGlobalFunction(const std::string& name, ScriptFunction::CppFunction function);
    
    void shutdown();
    
    std::vector<std::string> getEngineNames() const;
    int getEngineCount() const { return static_cast<int>(engines.size()); }
};

// Utility macros for easier script binding
#define SCRIPT_BIND_FUNCTION(engine, name, func) \
    engine->registerFunction(name, [](const std::vector<ScriptValue>& args) -> ScriptValue { \
        return func(args); \
    })

#define SCRIPT_BIND_METHOD(engine, name, obj, method) \
    engine->registerFunction(name, [obj](const std::vector<ScriptValue>& args) -> ScriptValue { \
        return obj->method(args); \
    })

} // namespace Scripting
} // namespace JJM

#endif // SCRIPT_ENGINE_H