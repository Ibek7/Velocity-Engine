#ifndef SCRIPT_BINDING_H
#define SCRIPT_BINDING_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <any>

namespace JJM {
namespace Scripting {

// Forward declarations
class ScriptEngine;
class ScriptContext;
class ScriptValue;
class ScriptFunction;

// Supported scripting languages
enum class ScriptLanguage {
    LUA,
    PYTHON,
    JAVASCRIPT
};

// Script value types
enum class ScriptValueType {
    NIL,
    BOOLEAN,
    INTEGER,
    FLOAT,
    STRING,
    FUNCTION,
    TABLE,
    USERDATA,
    LIGHTUSERDATA
};

// Script value wrapper
class ScriptValue {
public:
    ScriptValue();
    ScriptValue(bool value);
    ScriptValue(int value);
    ScriptValue(float value);
    ScriptValue(const char* value);
    ScriptValue(const std::string& value);
    ~ScriptValue();
    
    ScriptValueType getType() const { return m_type; }
    
    bool isNil() const { return m_type == ScriptValueType::NIL; }
    bool isBool() const { return m_type == ScriptValueType::BOOLEAN; }
    bool isInt() const { return m_type == ScriptValueType::INTEGER; }
    bool isFloat() const { return m_type == ScriptValueType::FLOAT; }
    bool isString() const { return m_type == ScriptValueType::STRING; }
    bool isFunction() const { return m_type == ScriptValueType::FUNCTION; }
    bool isTable() const { return m_type == ScriptValueType::TABLE; }
    
    bool asBool(bool defaultValue = false) const;
    int asInt(int defaultValue = 0) const;
    float asFloat(float defaultValue = 0.0f) const;
    std::string asString(const std::string& defaultValue = "") const;
    
private:
    ScriptValueType m_type;
    std::any m_value;
};

// C++ function that can be called from script
using ScriptNativeFunction = std::function<ScriptValue(const std::vector<ScriptValue>&)>;

// Script function wrapper
class ScriptFunction {
public:
    virtual ~ScriptFunction() = default;
    virtual ScriptValue call(const std::vector<ScriptValue>& args) = 0;
    virtual bool isValid() const = 0;
};

// Script context for execution
class ScriptContext {
public:
    virtual ~ScriptContext() = default;
    
    // Variable access
    virtual void setGlobal(const std::string& name, const ScriptValue& value) = 0;
    virtual ScriptValue getGlobal(const std::string& name) = 0;
    
    // Function registration
    virtual void registerFunction(const std::string& name, ScriptNativeFunction function) = 0;
    
    // Execution
    virtual bool executeString(const std::string& code) = 0;
    virtual bool executeFile(const std::string& filename) = 0;
    virtual ScriptValue evaluate(const std::string& expression) = 0;
    
    // Function calling
    virtual ScriptValue callFunction(const std::string& name, const std::vector<ScriptValue>& args) = 0;
    
    // Error handling
    virtual std::string getLastError() const = 0;
    virtual bool hasError() const = 0;
    virtual void clearError() = 0;
};

// Lua-specific context
class LuaContext : public ScriptContext {
public:
    LuaContext();
    ~LuaContext() override;
    
    void setGlobal(const std::string& name, const ScriptValue& value) override;
    ScriptValue getGlobal(const std::string& name) override;
    void registerFunction(const std::string& name, ScriptNativeFunction function) override;
    bool executeString(const std::string& code) override;
    bool executeFile(const std::string& filename) override;
    ScriptValue evaluate(const std::string& expression) override;
    ScriptValue callFunction(const std::string& name, const std::vector<ScriptValue>& args) override;
    
    std::string getLastError() const override { return m_lastError; }
    bool hasError() const override { return !m_lastError.empty(); }
    void clearError() override { m_lastError.clear(); }
    
    void* getLuaState() { return m_luaState; }
    
private:
    void* m_luaState;  // lua_State*
    std::string m_lastError;
    std::unordered_map<std::string, ScriptNativeFunction> m_nativeFunctions;
    
    void pushValue(const ScriptValue& value);
    ScriptValue popValue();
    void setError(const std::string& error);
};

// Python-specific context
class PythonContext : public ScriptContext {
public:
    PythonContext();
    ~PythonContext() override;
    
    void setGlobal(const std::string& name, const ScriptValue& value) override;
    ScriptValue getGlobal(const std::string& name) override;
    void registerFunction(const std::string& name, ScriptNativeFunction function) override;
    bool executeString(const std::string& code) override;
    bool executeFile(const std::string& filename) override;
    ScriptValue evaluate(const std::string& expression) override;
    ScriptValue callFunction(const std::string& name, const std::vector<ScriptValue>& args) override;
    
    std::string getLastError() const override { return m_lastError; }
    bool hasError() const override { return !m_lastError.empty(); }
    void clearError() override { m_lastError.clear(); }
    
private:
    void* m_mainModule;       // PyObject*
    void* m_globalDict;       // PyObject*
    std::string m_lastError;
    std::unordered_map<std::string, ScriptNativeFunction> m_nativeFunctions;
    
    void setError(const std::string& error);
};

// Main script engine
class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();
    
    // Context management
    ScriptContext* createContext(ScriptLanguage language);
    void destroyContext(ScriptContext* context);
    ScriptContext* getDefaultContext() { return m_defaultContext.get(); }
    
    // Engine initialization
    bool initialize(ScriptLanguage defaultLanguage = ScriptLanguage::LUA);
    void shutdown();
    bool isInitialized() const { return m_initialized; }
    
    // Quick execution (uses default context)
    bool execute(const std::string& code);
    bool executeFile(const std::string& filename);
    ScriptValue evaluate(const std::string& expression);
    
    // Global function registration (all contexts)
    void registerGlobalFunction(const std::string& name, ScriptNativeFunction function);
    
    // Binding helpers for common types
    template<typename T>
    void bindClass(const std::string& className);
    
    template<typename T, typename... Args>
    void bindConstructor(const std::string& className);
    
    template<typename T, typename Ret, typename... Args>
    void bindMethod(const std::string& className, const std::string& methodName,
                   Ret(T::*method)(Args...));
    
    template<typename T>
    void bindProperty(const std::string& className, const std::string& propertyName,
                     std::function<ScriptValue(T*)> getter,
                     std::function<void(T*, const ScriptValue&)> setter = nullptr);
    
    // Module loading
    void addModulePath(const std::string& path);
    bool loadModule(const std::string& moduleName);
    
    // Error handling
    std::string getLastError() const;
    bool hasError() const;
    void clearError();
    
    // Hot reload support
    void enableHotReload(bool enable) { m_hotReloadEnabled = enable; }
    bool isHotReloadEnabled() const { return m_hotReloadEnabled; }
    void watchFile(const std::string& filename);
    void unwatchFile(const std::string& filename);
    void checkForChanges();  // Call periodically to detect file changes
    
private:
    std::unique_ptr<ScriptContext> m_defaultContext;
    std::vector<std::unique_ptr<ScriptContext>> m_contexts;
    ScriptLanguage m_defaultLanguage;
    bool m_initialized;
    bool m_hotReloadEnabled;
    std::vector<std::string> m_modulePaths;
    std::unordered_map<std::string, uint64_t> m_watchedFiles;  // filename -> last modified time
    std::unordered_map<std::string, ScriptNativeFunction> m_globalFunctions;
};

// Helper macros for binding
#define SCRIPT_BIND_FUNCTION(engine, func) \
    engine->registerGlobalFunction(#func, [](const std::vector<ScriptValue>& args) { \
        return ScriptValue(func()); \
    })

#define SCRIPT_BIND_FUNCTION_1(engine, func, T1) \
    engine->registerGlobalFunction(#func, [](const std::vector<ScriptValue>& args) { \
        if (args.size() < 1) return ScriptValue(); \
        return ScriptValue(func(args[0].as<T1>())); \
    })

// Common engine bindings
class EngineBindings {
public:
    static void registerMathBindings(ScriptEngine* engine);
    static void registerInputBindings(ScriptEngine* engine);
    static void registerGraphicsBindings(ScriptEngine* engine);
    static void registerAudioBindings(ScriptEngine* engine);
    static void registerPhysicsBindings(ScriptEngine* engine);
    static void registerECSBindings(ScriptEngine* engine);
    static void registerAllBindings(ScriptEngine* engine);
};

// Script component for ECS integration
struct ScriptComponent {
    std::string scriptFile;
    ScriptContext* context;
    bool autoUpdate;
    bool initialized;
    
    // Lifecycle callbacks (function names in script)
    std::string onInitFunction;
    std::string onUpdateFunction;
    std::string onDestroyFunction;
    
    ScriptComponent() 
        : context(nullptr)
        , autoUpdate(true)
        , initialized(false)
        , onInitFunction("onInit")
        , onUpdateFunction("onUpdate")
        , onDestroyFunction("onDestroy") {}
};

// Script system for ECS
class ScriptSystem {
public:
    ScriptSystem(ScriptEngine* engine);
    ~ScriptSystem();
    
    void initialize();
    void update(float deltaTime);
    void shutdown();
    
    // Component management
    void addScriptComponent(int entityId, const std::string& scriptFile);
    void removeScriptComponent(int entityId);
    ScriptComponent* getScriptComponent(int entityId);
    
    // Script reloading
    void reloadScript(int entityId);
    void reloadAllScripts();
    
private:
    ScriptEngine* m_engine;
    std::unordered_map<int, ScriptComponent> m_components;
    
    void initializeScript(int entityId, ScriptComponent& component);
    void updateScript(int entityId, ScriptComponent& component, float deltaTime);
    void destroyScript(int entityId, ScriptComponent& component);
};

// Utility functions
namespace ScriptUtils {
    // Convert C++ values to script values
    ScriptValue toScriptValue(bool value);
    ScriptValue toScriptValue(int value);
    ScriptValue toScriptValue(float value);
    ScriptValue toScriptValue(const std::string& value);
    
    // Vector/array conversion
    ScriptValue vectorToTable(const std::vector<float>& vec);
    ScriptValue vectorToTable(const std::vector<int>& vec);
    std::vector<float> tableToFloatVector(const ScriptValue& table);
    std::vector<int> tableToIntVector(const ScriptValue& table);
    
    // File utilities
    std::string loadScriptFile(const std::string& filename);
    bool saveScriptFile(const std::string& filename, const std::string& content);
    std::string getScriptExtension(ScriptLanguage language);
    ScriptLanguage detectLanguageFromExtension(const std::string& filename);
}

} // namespace Scripting
} // namespace JJM

#endif // SCRIPT_BINDING_H
