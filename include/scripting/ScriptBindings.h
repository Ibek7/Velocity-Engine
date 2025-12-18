#ifndef JJM_SCRIPT_BINDINGS_H
#define JJM_SCRIPT_BINDINGS_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <any>

namespace JJM {
namespace Scripting {

/**
 * @brief Script value types
 */
enum class ValueType {
    Null,
    Boolean,
    Integer,
    Float,
    String,
    Array,
    Object,
    Function,
    UserData
};

/**
 * @brief Script value wrapper
 */
class ScriptValue {
public:
    ScriptValue();
    ScriptValue(bool value);
    ScriptValue(int value);
    ScriptValue(float value);
    ScriptValue(const std::string& value);
    ScriptValue(const char* value);
    ~ScriptValue();

    ValueType getType() const;
    
    bool isNull() const;
    bool isBool() const;
    bool isInt() const;
    bool isFloat() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;
    bool isFunction() const;
    
    bool toBool() const;
    int toInt() const;
    float toFloat() const;
    std::string toString() const;
    
    void setNull();
    void setBool(bool value);
    void setInt(int value);
    void setFloat(float value);
    void setString(const std::string& value);

private:
    ValueType type;
    std::any data;
};

/**
 * @brief Script function wrapper
 */
class ScriptFunction {
public:
    using NativeFunction = std::function<ScriptValue(const std::vector<ScriptValue>&)>;
    
    ScriptFunction();
    ScriptFunction(NativeFunction func);
    ~ScriptFunction();

    ScriptValue call(const std::vector<ScriptValue>& args);
    bool isValid() const;

private:
    NativeFunction function;
};

/**
 * @brief Script class definition
 */
class ScriptClass {
public:
    ScriptClass(const std::string& name);
    ~ScriptClass();

    std::string getName() const;
    
    void addMethod(const std::string& name, ScriptFunction::NativeFunction func);
    void addProperty(const std::string& name, ValueType type);
    void addStaticMethod(const std::string& name, ScriptFunction::NativeFunction func);
    
    ScriptFunction* getMethod(const std::string& name);
    bool hasMethod(const std::string& name) const;
    
    void setParent(std::shared_ptr<ScriptClass> parent);
    std::shared_ptr<ScriptClass> getParent() const;

private:
    std::string name;
    std::unordered_map<std::string, ScriptFunction> methods;
    std::unordered_map<std::string, ValueType> properties;
    std::unordered_map<std::string, ScriptFunction> staticMethods;
    std::shared_ptr<ScriptClass> parent;
};

/**
 * @brief Script object instance
 */
class ScriptObject {
public:
    ScriptObject(std::shared_ptr<ScriptClass> classType);
    ~ScriptObject();

    std::shared_ptr<ScriptClass> getClass() const;
    
    void setProperty(const std::string& name, const ScriptValue& value);
    ScriptValue getProperty(const std::string& name) const;
    bool hasProperty(const std::string& name) const;
    
    ScriptValue callMethod(const std::string& name, const std::vector<ScriptValue>& args);

private:
    std::shared_ptr<ScriptClass> classType;
    std::unordered_map<std::string, ScriptValue> properties;
};

/**
 * @brief Script module for organizing bindings
 */
class ScriptModule {
public:
    ScriptModule(const std::string& name);
    ~ScriptModule();

    std::string getName() const;
    
    void addClass(std::shared_ptr<ScriptClass> scriptClass);
    void addFunction(const std::string& name, ScriptFunction::NativeFunction func);
    void addConstant(const std::string& name, const ScriptValue& value);
    
    std::shared_ptr<ScriptClass> getClass(const std::string& name);
    ScriptFunction* getFunction(const std::string& name);
    ScriptValue getConstant(const std::string& name) const;
    
    bool hasClass(const std::string& name) const;
    bool hasFunction(const std::string& name) const;
    bool hasConstant(const std::string& name) const;

private:
    std::string name;
    std::unordered_map<std::string, std::shared_ptr<ScriptClass>> classes;
    std::unordered_map<std::string, ScriptFunction> functions;
    std::unordered_map<std::string, ScriptValue> constants;
};

/**
 * @brief Script binding registry
 */
class BindingRegistry {
public:
    static BindingRegistry& getInstance();
    
    BindingRegistry(const BindingRegistry&) = delete;
    BindingRegistry& operator=(const BindingRegistry&) = delete;

    void registerModule(std::shared_ptr<ScriptModule> module);
    void unregisterModule(const std::string& name);
    
    std::shared_ptr<ScriptModule> getModule(const std::string& name);
    bool hasModule(const std::string& name) const;
    
    std::vector<std::string> getModuleNames() const;
    
    void clear();

private:
    BindingRegistry();
    ~BindingRegistry();
    
    std::unordered_map<std::string, std::shared_ptr<ScriptModule>> modules;
};

/**
 * @brief Type conversion helpers
 */
class TypeConverter {
public:
    template<typename T>
    static ScriptValue toScript(const T& value);
    
    template<typename T>
    static T fromScript(const ScriptValue& value);
    
    static ScriptValue vectorToArray(const std::vector<ScriptValue>& vec);
    static std::vector<ScriptValue> arrayToVector(const ScriptValue& array);
};

/**
 * @brief Binding builder for fluent API
 */
class BindingBuilder {
public:
    BindingBuilder(const std::string& moduleName);
    ~BindingBuilder();

    BindingBuilder& beginClass(const std::string& className);
    BindingBuilder& endClass();
    
    BindingBuilder& addMethod(const std::string& name, ScriptFunction::NativeFunction func);
    BindingBuilder& addStaticMethod(const std::string& name, ScriptFunction::NativeFunction func);
    BindingBuilder& addProperty(const std::string& name, ValueType type);
    
    BindingBuilder& addFunction(const std::string& name, ScriptFunction::NativeFunction func);
    BindingBuilder& addConstant(const std::string& name, const ScriptValue& value);
    
    std::shared_ptr<ScriptModule> build();

private:
    std::shared_ptr<ScriptModule> module;
    std::shared_ptr<ScriptClass> currentClass;
};

/**
 * @brief Engine bindings - Math module
 */
class MathBindings {
public:
    static void registerBindings();
    
private:
    static ScriptValue sin(const std::vector<ScriptValue>& args);
    static ScriptValue cos(const std::vector<ScriptValue>& args);
    static ScriptValue tan(const std::vector<ScriptValue>& args);
    static ScriptValue sqrt(const std::vector<ScriptValue>& args);
    static ScriptValue abs(const std::vector<ScriptValue>& args);
    static ScriptValue pow(const std::vector<ScriptValue>& args);
    static ScriptValue floor(const std::vector<ScriptValue>& args);
    static ScriptValue ceil(const std::vector<ScriptValue>& args);
    static ScriptValue round(const std::vector<ScriptValue>& args);
    static ScriptValue clamp(const std::vector<ScriptValue>& args);
    static ScriptValue lerp(const std::vector<ScriptValue>& args);
};

/**
 * @brief Engine bindings - Input module
 */
class InputBindings {
public:
    static void registerBindings();
    
private:
    static ScriptValue isKeyPressed(const std::vector<ScriptValue>& args);
    static ScriptValue isKeyDown(const std::vector<ScriptValue>& args);
    static ScriptValue isKeyReleased(const std::vector<ScriptValue>& args);
    static ScriptValue isMouseButtonPressed(const std::vector<ScriptValue>& args);
    static ScriptValue getMousePosition(const std::vector<ScriptValue>& args);
    static ScriptValue getMouseWheel(const std::vector<ScriptValue>& args);
};

/**
 * @brief Engine bindings - Graphics module
 */
class GraphicsBindings {
public:
    static void registerBindings();
    
private:
    static ScriptValue loadTexture(const std::vector<ScriptValue>& args);
    static ScriptValue drawSprite(const std::vector<ScriptValue>& args);
    static ScriptValue drawRectangle(const std::vector<ScriptValue>& args);
    static ScriptValue drawCircle(const std::vector<ScriptValue>& args);
    static ScriptValue drawLine(const std::vector<ScriptValue>& args);
    static ScriptValue setColor(const std::vector<ScriptValue>& args);
};

/**
 * @brief Engine bindings - Entity module
 */
class EntityBindings {
public:
    static void registerBindings();
    
private:
    static ScriptValue createEntity(const std::vector<ScriptValue>& args);
    static ScriptValue destroyEntity(const std::vector<ScriptValue>& args);
    static ScriptValue getEntity(const std::vector<ScriptValue>& args);
    static ScriptValue addComponent(const std::vector<ScriptValue>& args);
    static ScriptValue getComponent(const std::vector<ScriptValue>& args);
    static ScriptValue removeComponent(const std::vector<ScriptValue>& args);
};

/**
 * @brief Automatic binding generator macros helpers
 */
class BindingHelpers {
public:
    template<typename Ret, typename... Args>
    static ScriptFunction::NativeFunction wrapFunction(Ret(*func)(Args...));
    
    template<typename Class, typename Ret, typename... Args>
    static ScriptFunction::NativeFunction wrapMethod(Ret(Class::*method)(Args...), Class* instance);
    
    template<typename T>
    static void registerType(const std::string& name);
};

/**
 * @brief Script callback system
 */
class ScriptCallback {
public:
    ScriptCallback();
    ScriptCallback(const std::string& functionName);
    ~ScriptCallback();

    void setFunction(const std::string& functionName);
    std::string getFunction() const;
    
    bool isValid() const;
    
    ScriptValue invoke(const std::vector<ScriptValue>& args);

private:
    std::string functionName;
};

/**
 * @brief Script event system
 */
class ScriptEventSystem {
public:
    static ScriptEventSystem& getInstance();
    
    ScriptEventSystem(const ScriptEventSystem&) = delete;
    ScriptEventSystem& operator=(const ScriptEventSystem&) = delete;

    void addEventListener(const std::string& eventName, ScriptCallback callback);
    void removeEventListener(const std::string& eventName, const std::string& functionName);
    
    void emit(const std::string& eventName, const std::vector<ScriptValue>& args);
    
    void clear();

private:
    ScriptEventSystem();
    ~ScriptEventSystem();
    
    std::unordered_map<std::string, std::vector<ScriptCallback>> listeners;
};

/**
 * @brief Initialize all engine bindings
 */
class EngineBindings {
public:
    static void registerAll();
    static void unregisterAll();
};

// Template implementations
template<typename T>
ScriptValue TypeConverter::toScript(const T& value) {
    (void)value;
    return ScriptValue(); // Stub
}

template<typename T>
T TypeConverter::fromScript(const ScriptValue& value) {
    (void)value;
    return T(); // Stub
}

template<typename Ret, typename... Args>
ScriptFunction::NativeFunction BindingHelpers::wrapFunction(Ret(*func)(Args...)) {
    (void)func;
    return [](const std::vector<ScriptValue>&) { return ScriptValue(); }; // Stub
}

template<typename Class, typename Ret, typename... Args>
ScriptFunction::NativeFunction BindingHelpers::wrapMethod(Ret(Class::*method)(Args...), Class* instance) {
    (void)method; (void)instance;
    return [](const std::vector<ScriptValue>&) { return ScriptValue(); }; // Stub
}

template<typename T>
void BindingHelpers::registerType(const std::string& name) {
    (void)name;
    // Stub
}

} // namespace Scripting
} // namespace JJM

#endif // JJM_SCRIPT_BINDINGS_H
