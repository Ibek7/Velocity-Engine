#include "scripting/ScriptBindings.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Scripting {

// ScriptValue implementation
ScriptValue::ScriptValue() : type(ValueType::Null) {}

ScriptValue::ScriptValue(bool value) : type(ValueType::Boolean) {
    data = value;
}

ScriptValue::ScriptValue(int value) : type(ValueType::Integer) {
    data = value;
}

ScriptValue::ScriptValue(float value) : type(ValueType::Float) {
    data = value;
}

ScriptValue::ScriptValue(const std::string& value) : type(ValueType::String) {
    data = value;
}

ScriptValue::ScriptValue(const char* value) : type(ValueType::String) {
    data = std::string(value);
}

ScriptValue::~ScriptValue() {}

ValueType ScriptValue::getType() const { return type; }

bool ScriptValue::isNull() const { return type == ValueType::Null; }
bool ScriptValue::isBool() const { return type == ValueType::Boolean; }
bool ScriptValue::isInt() const { return type == ValueType::Integer; }
bool ScriptValue::isFloat() const { return type == ValueType::Float; }
bool ScriptValue::isString() const { return type == ValueType::String; }
bool ScriptValue::isArray() const { return type == ValueType::Array; }
bool ScriptValue::isObject() const { return type == ValueType::Object; }
bool ScriptValue::isFunction() const { return type == ValueType::Function; }

bool ScriptValue::toBool() const {
    if (type == ValueType::Boolean) {
        return std::any_cast<bool>(data);
    }
    return false;
}

int ScriptValue::toInt() const {
    if (type == ValueType::Integer) {
        return std::any_cast<int>(data);
    } else if (type == ValueType::Float) {
        return static_cast<int>(std::any_cast<float>(data));
    }
    return 0;
}

float ScriptValue::toFloat() const {
    if (type == ValueType::Float) {
        return std::any_cast<float>(data);
    } else if (type == ValueType::Integer) {
        return static_cast<float>(std::any_cast<int>(data));
    }
    return 0.0f;
}

std::string ScriptValue::toString() const {
    if (type == ValueType::String) {
        return std::any_cast<std::string>(data);
    }
    return "";
}

void ScriptValue::setNull() { type = ValueType::Null; data.reset(); }
void ScriptValue::setBool(bool value) { type = ValueType::Boolean; data = value; }
void ScriptValue::setInt(int value) { type = ValueType::Integer; data = value; }
void ScriptValue::setFloat(float value) { type = ValueType::Float; data = value; }
void ScriptValue::setString(const std::string& value) { type = ValueType::String; data = value; }

// ScriptFunction implementation
ScriptFunction::ScriptFunction() {}

ScriptFunction::ScriptFunction(NativeFunction func) : function(func) {}

ScriptFunction::~ScriptFunction() {}

ScriptValue ScriptFunction::call(const std::vector<ScriptValue>& args) {
    if (function) {
        return function(args);
    }
    return ScriptValue();
}

bool ScriptFunction::isValid() const {
    return function != nullptr;
}

// ScriptClass implementation
ScriptClass::ScriptClass(const std::string& name) : name(name) {}

ScriptClass::~ScriptClass() {}

std::string ScriptClass::getName() const { return name; }

void ScriptClass::addMethod(const std::string& methodName, ScriptFunction::NativeFunction func) {
    methods[methodName] = ScriptFunction(func);
}

void ScriptClass::addProperty(const std::string& propertyName, ValueType propertyType) {
    properties[propertyName] = propertyType;
}

void ScriptClass::addStaticMethod(const std::string& methodName, ScriptFunction::NativeFunction func) {
    staticMethods[methodName] = ScriptFunction(func);
}

ScriptFunction* ScriptClass::getMethod(const std::string& methodName) {
    auto it = methods.find(methodName);
    if (it != methods.end()) {
        return &it->second;
    }
    if (parent) {
        return parent->getMethod(methodName);
    }
    return nullptr;
}

bool ScriptClass::hasMethod(const std::string& methodName) const {
    if (methods.find(methodName) != methods.end()) {
        return true;
    }
    return parent ? parent->hasMethod(methodName) : false;
}

void ScriptClass::setParent(std::shared_ptr<ScriptClass> p) { parent = p; }
std::shared_ptr<ScriptClass> ScriptClass::getParent() const { return parent; }

// ScriptObject implementation
ScriptObject::ScriptObject(std::shared_ptr<ScriptClass> classType)
    : classType(classType) {}

ScriptObject::~ScriptObject() {}

std::shared_ptr<ScriptClass> ScriptObject::getClass() const { return classType; }

void ScriptObject::setProperty(const std::string& name, const ScriptValue& value) {
    properties[name] = value;
}

ScriptValue ScriptObject::getProperty(const std::string& name) const {
    auto it = properties.find(name);
    return it != properties.end() ? it->second : ScriptValue();
}

bool ScriptObject::hasProperty(const std::string& name) const {
    return properties.find(name) != properties.end();
}

ScriptValue ScriptObject::callMethod(const std::string& name, const std::vector<ScriptValue>& args) {
    if (classType) {
        ScriptFunction* method = classType->getMethod(name);
        if (method) {
            return method->call(args);
        }
    }
    return ScriptValue();
}

// ScriptModule implementation
ScriptModule::ScriptModule(const std::string& name) : name(name) {}

ScriptModule::~ScriptModule() {}

std::string ScriptModule::getName() const { return name; }

void ScriptModule::addClass(std::shared_ptr<ScriptClass> scriptClass) {
    classes[scriptClass->getName()] = scriptClass;
}

void ScriptModule::addFunction(const std::string& functionName, ScriptFunction::NativeFunction func) {
    functions[functionName] = ScriptFunction(func);
}

void ScriptModule::addConstant(const std::string& constantName, const ScriptValue& value) {
    constants[constantName] = value;
}

std::shared_ptr<ScriptClass> ScriptModule::getClass(const std::string& className) {
    auto it = classes.find(className);
    return it != classes.end() ? it->second : nullptr;
}

ScriptFunction* ScriptModule::getFunction(const std::string& functionName) {
    auto it = functions.find(functionName);
    return it != functions.end() ? &it->second : nullptr;
}

ScriptValue ScriptModule::getConstant(const std::string& constantName) const {
    auto it = constants.find(constantName);
    return it != constants.end() ? it->second : ScriptValue();
}

bool ScriptModule::hasClass(const std::string& className) const {
    return classes.find(className) != classes.end();
}

bool ScriptModule::hasFunction(const std::string& functionName) const {
    return functions.find(functionName) != functions.end();
}

bool ScriptModule::hasConstant(const std::string& constantName) const {
    return constants.find(constantName) != constants.end();
}

// BindingRegistry implementation
BindingRegistry::BindingRegistry() {}
BindingRegistry::~BindingRegistry() {}

BindingRegistry& BindingRegistry::getInstance() {
    static BindingRegistry instance;
    return instance;
}

void BindingRegistry::registerModule(std::shared_ptr<ScriptModule> module) {
    modules[module->getName()] = module;
}

void BindingRegistry::unregisterModule(const std::string& name) {
    modules.erase(name);
}

std::shared_ptr<ScriptModule> BindingRegistry::getModule(const std::string& name) {
    auto it = modules.find(name);
    return it != modules.end() ? it->second : nullptr;
}

bool BindingRegistry::hasModule(const std::string& name) const {
    return modules.find(name) != modules.end();
}

std::vector<std::string> BindingRegistry::getModuleNames() const {
    std::vector<std::string> names;
    for (const auto& pair : modules) {
        names.push_back(pair.first);
    }
    return names;
}

void BindingRegistry::clear() {
    modules.clear();
}

// TypeConverter implementation
ScriptValue TypeConverter::vectorToArray(const std::vector<ScriptValue>& vec) {
    (void)vec;
    ScriptValue array;
    // Stub: Convert vector to script array
    return array;
}

std::vector<ScriptValue> TypeConverter::arrayToVector(const ScriptValue& array) {
    (void)array;
    // Stub: Convert script array to vector
    return {};
}

// BindingBuilder implementation
BindingBuilder::BindingBuilder(const std::string& moduleName)
    : module(std::make_shared<ScriptModule>(moduleName)) {}

BindingBuilder::~BindingBuilder() {}

BindingBuilder& BindingBuilder::beginClass(const std::string& className) {
    currentClass = std::make_shared<ScriptClass>(className);
    return *this;
}

BindingBuilder& BindingBuilder::endClass() {
    if (currentClass) {
        module->addClass(currentClass);
        currentClass = nullptr;
    }
    return *this;
}

BindingBuilder& BindingBuilder::addMethod(const std::string& name, ScriptFunction::NativeFunction func) {
    if (currentClass) {
        currentClass->addMethod(name, func);
    }
    return *this;
}

BindingBuilder& BindingBuilder::addStaticMethod(const std::string& name, ScriptFunction::NativeFunction func) {
    if (currentClass) {
        currentClass->addStaticMethod(name, func);
    }
    return *this;
}

BindingBuilder& BindingBuilder::addProperty(const std::string& name, ValueType propertyType) {
    if (currentClass) {
        currentClass->addProperty(name, propertyType);
    }
    return *this;
}

BindingBuilder& BindingBuilder::addFunction(const std::string& name, ScriptFunction::NativeFunction func) {
    module->addFunction(name, func);
    return *this;
}

BindingBuilder& BindingBuilder::addConstant(const std::string& name, const ScriptValue& value) {
    module->addConstant(name, value);
    return *this;
}

std::shared_ptr<ScriptModule> BindingBuilder::build() {
    return module;
}

// MathBindings implementation
void MathBindings::registerBindings() {
    auto module = BindingBuilder("Math")
        .addFunction("sin", sin)
        .addFunction("cos", cos)
        .addFunction("tan", tan)
        .addFunction("sqrt", sqrt)
        .addFunction("abs", abs)
        .addFunction("pow", pow)
        .addFunction("floor", floor)
        .addFunction("ceil", ceil)
        .addFunction("round", round)
        .addFunction("clamp", clamp)
        .addFunction("lerp", lerp)
        .addConstant("PI", ScriptValue(3.14159265359f))
        .addConstant("E", ScriptValue(2.71828182846f))
        .build();
    
    BindingRegistry::getInstance().registerModule(module);
}

ScriptValue MathBindings::sin(const std::vector<ScriptValue>& args) {
    if (!args.empty()) return ScriptValue(std::sin(args[0].toFloat()));
    return ScriptValue(0.0f);
}

ScriptValue MathBindings::cos(const std::vector<ScriptValue>& args) {
    if (!args.empty()) return ScriptValue(std::cos(args[0].toFloat()));
    return ScriptValue(0.0f);
}

ScriptValue MathBindings::tan(const std::vector<ScriptValue>& args) {
    if (!args.empty()) return ScriptValue(std::tan(args[0].toFloat()));
    return ScriptValue(0.0f);
}

ScriptValue MathBindings::sqrt(const std::vector<ScriptValue>& args) {
    if (!args.empty()) return ScriptValue(std::sqrt(args[0].toFloat()));
    return ScriptValue(0.0f);
}

ScriptValue MathBindings::abs(const std::vector<ScriptValue>& args) {
    if (!args.empty()) return ScriptValue(std::abs(args[0].toFloat()));
    return ScriptValue(0.0f);
}

ScriptValue MathBindings::pow(const std::vector<ScriptValue>& args) {
    if (args.size() >= 2) return ScriptValue(std::pow(args[0].toFloat(), args[1].toFloat()));
    return ScriptValue(0.0f);
}

ScriptValue MathBindings::floor(const std::vector<ScriptValue>& args) {
    if (!args.empty()) return ScriptValue(std::floor(args[0].toFloat()));
    return ScriptValue(0.0f);
}

ScriptValue MathBindings::ceil(const std::vector<ScriptValue>& args) {
    if (!args.empty()) return ScriptValue(std::ceil(args[0].toFloat()));
    return ScriptValue(0.0f);
}

ScriptValue MathBindings::round(const std::vector<ScriptValue>& args) {
    if (!args.empty()) return ScriptValue(std::round(args[0].toFloat()));
    return ScriptValue(0.0f);
}

ScriptValue MathBindings::clamp(const std::vector<ScriptValue>& args) {
    if (args.size() >= 3) {
        float value = args[0].toFloat();
        float min = args[1].toFloat();
        float max = args[2].toFloat();
        return ScriptValue(std::clamp(value, min, max));
    }
    return ScriptValue(0.0f);
}

ScriptValue MathBindings::lerp(const std::vector<ScriptValue>& args) {
    if (args.size() >= 3) {
        float a = args[0].toFloat();
        float b = args[1].toFloat();
        float t = args[2].toFloat();
        return ScriptValue(a + (b - a) * t);
    }
    return ScriptValue(0.0f);
}

// InputBindings stubs
void InputBindings::registerBindings() {
    auto module = BindingBuilder("Input")
        .addFunction("isKeyPressed", isKeyPressed)
        .addFunction("isKeyDown", isKeyDown)
        .addFunction("isKeyReleased", isKeyReleased)
        .addFunction("isMouseButtonPressed", isMouseButtonPressed)
        .addFunction("getMousePosition", getMousePosition)
        .addFunction("getMouseWheel", getMouseWheel)
        .build();
    
    BindingRegistry::getInstance().registerModule(module);
}

ScriptValue InputBindings::isKeyPressed(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue(false);
}

ScriptValue InputBindings::isKeyDown(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue(false);
}

ScriptValue InputBindings::isKeyReleased(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue(false);
}

ScriptValue InputBindings::isMouseButtonPressed(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue(false);
}

ScriptValue InputBindings::getMousePosition(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

ScriptValue InputBindings::getMouseWheel(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue(0.0f);
}

// GraphicsBindings stubs
void GraphicsBindings::registerBindings() {
    auto module = BindingBuilder("Graphics")
        .addFunction("loadTexture", loadTexture)
        .addFunction("drawSprite", drawSprite)
        .addFunction("drawRectangle", drawRectangle)
        .addFunction("drawCircle", drawCircle)
        .addFunction("drawLine", drawLine)
        .addFunction("setColor", setColor)
        .build();
    
    BindingRegistry::getInstance().registerModule(module);
}

ScriptValue GraphicsBindings::loadTexture(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

ScriptValue GraphicsBindings::drawSprite(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

ScriptValue GraphicsBindings::drawRectangle(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

ScriptValue GraphicsBindings::drawCircle(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

ScriptValue GraphicsBindings::drawLine(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

ScriptValue GraphicsBindings::setColor(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

// EntityBindings stubs
void EntityBindings::registerBindings() {
    auto module = BindingBuilder("Entity")
        .addFunction("createEntity", createEntity)
        .addFunction("destroyEntity", destroyEntity)
        .addFunction("getEntity", getEntity)
        .addFunction("addComponent", addComponent)
        .addFunction("getComponent", getComponent)
        .addFunction("removeComponent", removeComponent)
        .build();
    
    BindingRegistry::getInstance().registerModule(module);
}

ScriptValue EntityBindings::createEntity(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

ScriptValue EntityBindings::destroyEntity(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

ScriptValue EntityBindings::getEntity(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

ScriptValue EntityBindings::addComponent(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

ScriptValue EntityBindings::getComponent(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

ScriptValue EntityBindings::removeComponent(const std::vector<ScriptValue>& args) {
    (void)args; return ScriptValue();
}

// ScriptCallback implementation
ScriptCallback::ScriptCallback() {}

ScriptCallback::ScriptCallback(const std::string& functionName)
    : functionName(functionName) {}

ScriptCallback::~ScriptCallback() {}

void ScriptCallback::setFunction(const std::string& name) { functionName = name; }
std::string ScriptCallback::getFunction() const { return functionName; }

bool ScriptCallback::isValid() const { return !functionName.empty(); }

ScriptValue ScriptCallback::invoke(const std::vector<ScriptValue>& args) {
    (void)args;
    // Stub: Call script function
    return ScriptValue();
}

// ScriptEventSystem implementation
ScriptEventSystem::ScriptEventSystem() {}
ScriptEventSystem::~ScriptEventSystem() {}

ScriptEventSystem& ScriptEventSystem::getInstance() {
    static ScriptEventSystem instance;
    return instance;
}

void ScriptEventSystem::addEventListener(const std::string& eventName, ScriptCallback callback) {
    listeners[eventName].push_back(callback);
}

void ScriptEventSystem::removeEventListener(const std::string& eventName, const std::string& functionName) {
    auto it = listeners.find(eventName);
    if (it != listeners.end()) {
        auto& callbacks = it->second;
        callbacks.erase(
            std::remove_if(callbacks.begin(), callbacks.end(),
                [&functionName](const ScriptCallback& cb) {
                    return cb.getFunction() == functionName;
                }),
            callbacks.end()
        );
    }
}

void ScriptEventSystem::emit(const std::string& eventName, const std::vector<ScriptValue>& args) {
    auto it = listeners.find(eventName);
    if (it != listeners.end()) {
        for (auto& callback : it->second) {
            callback.invoke(args);
        }
    }
}

void ScriptEventSystem::clear() {
    listeners.clear();
}

// EngineBindings implementation
void EngineBindings::registerAll() {
    MathBindings::registerBindings();
    InputBindings::registerBindings();
    GraphicsBindings::registerBindings();
    EntityBindings::registerBindings();
}

void EngineBindings::unregisterAll() {
    BindingRegistry::getInstance().clear();
}

} // namespace Scripting
} // namespace JJM
