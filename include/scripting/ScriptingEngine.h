#ifndef SCRIPTING_ENGINE_H
#define SCRIPTING_ENGINE_H

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>
#include <any>

namespace JJM {
namespace Scripting {

// Script value types
enum class ScriptValueType {
    NIL,
    BOOLEAN,
    INTEGER,
    FLOAT,
    STRING,
    FUNCTION,
    TABLE
};

// Generic script value
struct ScriptValue {
    ScriptValueType type;
    std::any data;
    
    ScriptValue() : type(ScriptValueType::NIL) {}
    
    template<typename T>
    ScriptValue(T value);
    
    template<typename T>
    T as() const;
    
    bool isNil() const { return type == ScriptValueType::NIL; }
    bool isBool() const { return type == ScriptValueType::BOOLEAN; }
    bool isInt() const { return type == ScriptValueType::INTEGER; }
    bool isFloat() const { return type == ScriptValueType::FLOAT; }
    bool isString() const { return type == ScriptValueType::STRING; }
    bool isFunction() const { return type == ScriptValueType::FUNCTION; }
    bool isTable() const { return type == ScriptValueType::TABLE; }
};

// Script function signature
using ScriptFunction = std::function<ScriptValue(const std::vector<ScriptValue>&)>;

// Script table (associative array)
class ScriptTable {
public:
    void set(const std::string& key, const ScriptValue& value);
    ScriptValue get(const std::string& key) const;
    bool has(const std::string& key) const;
    std::vector<std::string> keys() const;
    
private:
    std::unordered_map<std::string, ScriptValue> data;
};

// Script error handling
class ScriptException : public std::exception {
public:
    ScriptException(const std::string& message) : msg(message) {}
    const char* what() const noexcept override { return msg.c_str(); }
    
private:
    std::string msg;
};

// Script context for execution
class ScriptContext {
public:
    ScriptContext();
    ~ScriptContext();
    
    // Variable management
    void setGlobal(const std::string& name, const ScriptValue& value);
    ScriptValue getGlobal(const std::string& name) const;
    
    // Function registration
    void registerFunction(const std::string& name, ScriptFunction func);
    
    // Script execution
    void loadScript(const std::string& filename);
    void loadString(const std::string& code);
    ScriptValue callFunction(const std::string& name, const std::vector<ScriptValue>& args = {});
    
    // Error handling
    std::string getLastError() const { return lastError; }
    
private:
    std::unordered_map<std::string, ScriptValue> globals;
    std::unordered_map<std::string, ScriptFunction> functions;
    std::string lastError;
    
    void executeCode(const std::string& code);
    ScriptValue evaluateExpression(const std::string& expr);
};

// Script manager
class ScriptManager {
public:
    static ScriptManager& getInstance();
    
    ScriptManager(const ScriptManager&) = delete;
    ScriptManager& operator=(const ScriptManager&) = delete;
    
    // Context management
    ScriptContext* createContext(const std::string& name);
    ScriptContext* getContext(const std::string& name);
    void destroyContext(const std::string& name);
    
    // Global function registration (available to all contexts)
    void registerGlobalFunction(const std::string& name, ScriptFunction func);
    
    // Script hot-reloading
    void watchScript(const std::string& filename);
    void unwatchScript(const std::string& filename);
    void checkForChanges();
    
private:
    ScriptManager();
    ~ScriptManager();
    
    std::unordered_map<std::string, std::unique_ptr<ScriptContext>> contexts;
    std::unordered_map<std::string, ScriptFunction> globalFunctions;
    std::unordered_map<std::string, time_t> watchedScripts;
};

// Component script bindings
class ComponentScriptBindings {
public:
    static void registerBindings(ScriptContext* context);
    
private:
    static ScriptValue createEntity(const std::vector<ScriptValue>& args);
    static ScriptValue destroyEntity(const std::vector<ScriptValue>& args);
    static ScriptValue addComponent(const std::vector<ScriptValue>& args);
    static ScriptValue removeComponent(const std::vector<ScriptValue>& args);
    static ScriptValue getComponent(const std::vector<ScriptValue>& args);
    static ScriptValue hasComponent(const std::vector<ScriptValue>& args);
};

// Math script bindings
class MathScriptBindings {
public:
    static void registerBindings(ScriptContext* context);
    
private:
    static ScriptValue vec2New(const std::vector<ScriptValue>& args);
    static ScriptValue vec2Add(const std::vector<ScriptValue>& args);
    static ScriptValue vec2Sub(const std::vector<ScriptValue>& args);
    static ScriptValue vec2Mul(const std::vector<ScriptValue>& args);
    static ScriptValue vec2Dot(const std::vector<ScriptValue>& args);
    static ScriptValue vec2Length(const std::vector<ScriptValue>& args);
    static ScriptValue vec2Normalize(const std::vector<ScriptValue>& args);
    static ScriptValue vec2Distance(const std::vector<ScriptValue>& args);
    static ScriptValue lerp(const std::vector<ScriptValue>& args);
    static ScriptValue clamp(const std::vector<ScriptValue>& args);
};

// Input script bindings
class InputScriptBindings {
public:
    static void registerBindings(ScriptContext* context);
    
private:
    static ScriptValue isKeyDown(const std::vector<ScriptValue>& args);
    static ScriptValue isKeyPressed(const std::vector<ScriptValue>& args);
    static ScriptValue isKeyReleased(const std::vector<ScriptValue>& args);
    static ScriptValue isMouseButtonDown(const std::vector<ScriptValue>& args);
    static ScriptValue getMousePosition(const std::vector<ScriptValue>& args);
    static ScriptValue getMouseDelta(const std::vector<ScriptValue>& args);
};

// Physics script bindings
class PhysicsScriptBindings {
public:
    static void registerBindings(ScriptContext* context);
    
private:
    static ScriptValue applyForce(const std::vector<ScriptValue>& args);
    static ScriptValue applyImpulse(const std::vector<ScriptValue>& args);
    static ScriptValue setVelocity(const std::vector<ScriptValue>& args);
    static ScriptValue getVelocity(const std::vector<ScriptValue>& args);
    static ScriptValue raycast(const std::vector<ScriptValue>& args);
    static ScriptValue checkCollision(const std::vector<ScriptValue>& args);
};

// Audio script bindings
class AudioScriptBindings {
public:
    static void registerBindings(ScriptContext* context);
    
private:
    static ScriptValue playSound(const std::vector<ScriptValue>& args);
    static ScriptValue playMusic(const std::vector<ScriptValue>& args);
    static ScriptValue stopSound(const std::vector<ScriptValue>& args);
    static ScriptValue stopMusic(const std::vector<ScriptValue>& args);
    static ScriptValue setVolume(const std::vector<ScriptValue>& args);
    static ScriptValue setPitch(const std::vector<ScriptValue>& args);
};

// Scene script bindings
class SceneScriptBindings {
public:
    static void registerBindings(ScriptContext* context);
    
private:
    static ScriptValue loadScene(const std::vector<ScriptValue>& args);
    static ScriptValue unloadScene(const std::vector<ScriptValue>& args);
    static ScriptValue getCurrentScene(const std::vector<ScriptValue>& args);
    static ScriptValue instantiate(const std::vector<ScriptValue>& args);
};

// Utility functions for script integration
namespace ScriptUtils {
    // Convert C++ types to script values
    ScriptValue toScriptValue(bool value);
    ScriptValue toScriptValue(int value);
    ScriptValue toScriptValue(float value);
    ScriptValue toScriptValue(const std::string& value);
    ScriptValue toScriptValue(ScriptFunction func);
    
    // Convert script values to C++ types
    bool toBool(const ScriptValue& value);
    int toInt(const ScriptValue& value);
    float toFloat(const ScriptValue& value);
    std::string toString(const ScriptValue& value);
    
    // Validation
    bool checkArgCount(const std::vector<ScriptValue>& args, size_t expected);
    bool checkArgType(const ScriptValue& arg, ScriptValueType expected);
}

} // namespace Scripting
} // namespace JJM

#endif // SCRIPTING_ENGINE_H
