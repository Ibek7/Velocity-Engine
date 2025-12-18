#include "scripting/ScriptingEngine.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>

namespace JJM {
namespace Scripting {

// Template specializations for ScriptValue
template<>
ScriptValue::ScriptValue(bool value) : type(ScriptValueType::BOOLEAN), data(value) {}

template<>
ScriptValue::ScriptValue(int value) : type(ScriptValueType::INTEGER), data(value) {}

template<>
ScriptValue::ScriptValue(float value) : type(ScriptValueType::FLOAT), data(value) {}

template<>
ScriptValue::ScriptValue(const char* value) : type(ScriptValueType::STRING), data(std::string(value)) {}

template<>
ScriptValue::ScriptValue(std::string value) : type(ScriptValueType::STRING), data(value) {}

template<>
ScriptValue::ScriptValue(ScriptFunction value) : type(ScriptValueType::FUNCTION), data(value) {}

template<>
ScriptValue::ScriptValue(ScriptTable value) : type(ScriptValueType::TABLE), data(value) {}

template<>
bool ScriptValue::as<bool>() const {
    if (type != ScriptValueType::BOOLEAN) throw ScriptException("Type mismatch: expected boolean");
    return std::any_cast<bool>(data);
}

template<>
int ScriptValue::as<int>() const {
    if (type != ScriptValueType::INTEGER) throw ScriptException("Type mismatch: expected integer");
    return std::any_cast<int>(data);
}

template<>
float ScriptValue::as<float>() const {
    if (type != ScriptValueType::FLOAT) throw ScriptException("Type mismatch: expected float");
    return std::any_cast<float>(data);
}

template<>
std::string ScriptValue::as<std::string>() const {
    if (type != ScriptValueType::STRING) throw ScriptException("Type mismatch: expected string");
    return std::any_cast<std::string>(data);
}

template<>
ScriptFunction ScriptValue::as<ScriptFunction>() const {
    if (type != ScriptValueType::FUNCTION) throw ScriptException("Type mismatch: expected function");
    return std::any_cast<ScriptFunction>(data);
}

template<>
ScriptTable ScriptValue::as<ScriptTable>() const {
    if (type != ScriptValueType::TABLE) throw ScriptException("Type mismatch: expected table");
    return std::any_cast<ScriptTable>(data);
}

// ScriptTable implementation
void ScriptTable::set(const std::string& key, const ScriptValue& value) {
    data[key] = value;
}

ScriptValue ScriptTable::get(const std::string& key) const {
    auto it = data.find(key);
    return (it != data.end()) ? it->second : ScriptValue();
}

bool ScriptTable::has(const std::string& key) const {
    return data.find(key) != data.end();
}

std::vector<std::string> ScriptTable::keys() const {
    std::vector<std::string> result;
    for (const auto& pair : data) {
        result.push_back(pair.first);
    }
    return result;
}

// ScriptContext implementation
ScriptContext::ScriptContext() {
    // Register standard library functions
    registerFunction("print", [](const std::vector<ScriptValue>& args) {
        for (const auto& arg : args) {
            if (arg.isString()) {
                printf("%s", arg.as<std::string>().c_str());
            } else if (arg.isInt()) {
                printf("%d", arg.as<int>());
            } else if (arg.isFloat()) {
                printf("%f", arg.as<float>());
            } else if (arg.isBool()) {
                printf("%s", arg.as<bool>() ? "true" : "false");
            }
        }
        printf("\n");
        return ScriptValue();
    });
    
    registerFunction("type", [](const std::vector<ScriptValue>& args) {
        if (args.empty()) return ScriptValue("nil");
        switch (args[0].type) {
            case ScriptValueType::NIL: return ScriptValue("nil");
            case ScriptValueType::BOOLEAN: return ScriptValue("boolean");
            case ScriptValueType::INTEGER: return ScriptValue("integer");
            case ScriptValueType::FLOAT: return ScriptValue("float");
            case ScriptValueType::STRING: return ScriptValue("string");
            case ScriptValueType::FUNCTION: return ScriptValue("function");
            case ScriptValueType::TABLE: return ScriptValue("table");
        }
        return ScriptValue("unknown");
    });
}

ScriptContext::~ScriptContext() {}

void ScriptContext::setGlobal(const std::string& name, const ScriptValue& value) {
    globals[name] = value;
}

ScriptValue ScriptContext::getGlobal(const std::string& name) const {
    auto it = globals.find(name);
    return (it != globals.end()) ? it->second : ScriptValue();
}

void ScriptContext::registerFunction(const std::string& name, ScriptFunction func) {
    functions[name] = func;
}

void ScriptContext::loadScript(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        lastError = "Failed to open script file: " + filename;
        throw ScriptException(lastError);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    loadString(buffer.str());
}

void ScriptContext::loadString(const std::string& code) {
    try {
        executeCode(code);
        lastError.clear();
    } catch (const ScriptException& e) {
        lastError = e.what();
        throw;
    }
}

ScriptValue ScriptContext::callFunction(const std::string& name, const std::vector<ScriptValue>& args) {
    auto it = functions.find(name);
    if (it == functions.end()) {
        lastError = "Function not found: " + name;
        throw ScriptException(lastError);
    }
    
    try {
        return it->second(args);
    } catch (const ScriptException& e) {
        lastError = e.what();
        throw;
    }
}

void ScriptContext::executeCode(const std::string& code) {
    // Simple script interpreter (this is a minimal implementation)
    // In a real implementation, you would integrate Lua or Python here
    
    // Parse variable assignments: var_name = value
    size_t pos = 0;
    while ((pos = code.find('=', pos)) != std::string::npos) {
        // Find variable name
        size_t start = pos;
        while (start > 0 && (isalnum(code[start - 1]) || code[start - 1] == '_')) {
            start--;
        }
        std::string varName = code.substr(start, pos - start);
        
        // Trim whitespace
        varName.erase(0, varName.find_first_not_of(" \t\n\r"));
        varName.erase(varName.find_last_not_of(" \t\n\r") + 1);
        
        // Find value (simplified - just handles numbers and strings)
        size_t valueStart = pos + 1;
        while (valueStart < code.length() && isspace(code[valueStart])) {
            valueStart++;
        }
        
        size_t valueEnd = code.find_first_of(";\n", valueStart);
        if (valueEnd == std::string::npos) valueEnd = code.length();
        
        std::string valueStr = code.substr(valueStart, valueEnd - valueStart);
        valueStr.erase(0, valueStr.find_first_not_of(" \t\n\r"));
        valueStr.erase(valueStr.find_last_not_of(" \t\n\r") + 1);
        
        // Parse value
        ScriptValue value;
        if (valueStr[0] == '"' || valueStr[0] == '\'') {
            // String value
            value = ScriptValue(valueStr.substr(1, valueStr.length() - 2));
        } else if (valueStr == "true" || valueStr == "false") {
            // Boolean value
            value = ScriptValue(valueStr == "true");
        } else if (valueStr.find('.') != std::string::npos) {
            // Float value
            value = ScriptValue(std::stof(valueStr));
        } else {
            // Integer value
            value = ScriptValue(std::stoi(valueStr));
        }
        
        if (!varName.empty()) {
            setGlobal(varName, value);
        }
        
        pos = valueEnd;
    }
}

ScriptValue ScriptContext::evaluateExpression(const std::string& expr) {
    // Simplified expression evaluation
    std::string trimmed = expr;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
    
    // Check if it's a variable
    if (isalpha(trimmed[0]) || trimmed[0] == '_') {
        return getGlobal(trimmed);
    }
    
    // Check if it's a number
    if (isdigit(trimmed[0]) || trimmed[0] == '-') {
        if (trimmed.find('.') != std::string::npos) {
            return ScriptValue(std::stof(trimmed));
        } else {
            return ScriptValue(std::stoi(trimmed));
        }
    }
    
    // Check if it's a string
    if (trimmed[0] == '"' || trimmed[0] == '\'') {
        return ScriptValue(trimmed.substr(1, trimmed.length() - 2));
    }
    
    return ScriptValue();
}

// ScriptManager implementation
ScriptManager& ScriptManager::getInstance() {
    static ScriptManager instance;
    return instance;
}

ScriptManager::ScriptManager() {}

ScriptManager::~ScriptManager() {
    contexts.clear();
}

ScriptContext* ScriptManager::createContext(const std::string& name) {
    auto context = std::make_unique<ScriptContext>();
    
    // Register all global functions
    for (const auto& func : globalFunctions) {
        context->registerFunction(func.first, func.second);
    }
    
    ScriptContext* ptr = context.get();
    contexts[name] = std::move(context);
    return ptr;
}

ScriptContext* ScriptManager::getContext(const std::string& name) {
    auto it = contexts.find(name);
    return (it != contexts.end()) ? it->second.get() : nullptr;
}

void ScriptManager::destroyContext(const std::string& name) {
    contexts.erase(name);
}

void ScriptManager::registerGlobalFunction(const std::string& name, ScriptFunction func) {
    globalFunctions[name] = func;
    
    // Register to all existing contexts
    for (auto& ctx : contexts) {
        ctx.second->registerFunction(name, func);
    }
}

void ScriptManager::watchScript(const std::string& filename) {
    struct stat fileStat;
    if (stat(filename.c_str(), &fileStat) == 0) {
        watchedScripts[filename] = fileStat.st_mtime;
    }
}

void ScriptManager::unwatchScript(const std::string& filename) {
    watchedScripts.erase(filename);
}

void ScriptManager::checkForChanges() {
    for (auto& watch : watchedScripts) {
        struct stat fileStat;
        if (stat(watch.first.c_str(), &fileStat) == 0) {
            if (fileStat.st_mtime > watch.second) {
                watch.second = fileStat.st_mtime;
                // TODO: Trigger reload callback
            }
        }
    }
}

// Component script bindings
void ComponentScriptBindings::registerBindings(ScriptContext* context) {
    context->registerFunction("createEntity", createEntity);
    context->registerFunction("destroyEntity", destroyEntity);
    context->registerFunction("addComponent", addComponent);
    context->registerFunction("removeComponent", removeComponent);
    context->registerFunction("getComponent", getComponent);
    context->registerFunction("hasComponent", hasComponent);
}

ScriptValue ComponentScriptBindings::createEntity(const std::vector<ScriptValue>& args) {
    // TODO: Integrate with actual ECS system
    (void)args;
    return ScriptValue(12345); // Mock entity ID
}

ScriptValue ComponentScriptBindings::destroyEntity(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isInt()) {
        throw ScriptException("destroyEntity requires entity ID");
    }
    // TODO: Integrate with actual ECS system
    return ScriptValue();
}

ScriptValue ComponentScriptBindings::addComponent(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isInt() || !args[1].isString()) {
        throw ScriptException("addComponent requires entity ID and component type");
    }
    // TODO: Integrate with actual ECS system
    return ScriptValue();
}

ScriptValue ComponentScriptBindings::removeComponent(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isInt() || !args[1].isString()) {
        throw ScriptException("removeComponent requires entity ID and component type");
    }
    // TODO: Integrate with actual ECS system
    return ScriptValue();
}

ScriptValue ComponentScriptBindings::getComponent(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isInt() || !args[1].isString()) {
        throw ScriptException("getComponent requires entity ID and component type");
    }
    // TODO: Integrate with actual ECS system
    return ScriptValue();
}

ScriptValue ComponentScriptBindings::hasComponent(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isInt() || !args[1].isString()) {
        throw ScriptException("hasComponent requires entity ID and component type");
    }
    // TODO: Integrate with actual ECS system
    return ScriptValue(false);
}

// Math script bindings
void MathScriptBindings::registerBindings(ScriptContext* context) {
    context->registerFunction("vec2", vec2New);
    context->registerFunction("vec2Add", vec2Add);
    context->registerFunction("vec2Sub", vec2Sub);
    context->registerFunction("vec2Mul", vec2Mul);
    context->registerFunction("vec2Dot", vec2Dot);
    context->registerFunction("vec2Length", vec2Length);
    context->registerFunction("vec2Normalize", vec2Normalize);
    context->registerFunction("vec2Distance", vec2Distance);
    context->registerFunction("lerp", lerp);
    context->registerFunction("clamp", clamp);
}

ScriptValue MathScriptBindings::vec2New(const std::vector<ScriptValue>& args) {
    if (args.size() < 2) throw ScriptException("vec2 requires x and y");
    
    ScriptTable table;
    table.set("x", args[0]);
    table.set("y", args[1]);
    return ScriptValue(table);
}

ScriptValue MathScriptBindings::vec2Add(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isTable() || !args[1].isTable()) {
        throw ScriptException("vec2Add requires two vec2 tables");
    }
    
    auto v1 = args[0].as<ScriptTable>();
    auto v2 = args[1].as<ScriptTable>();
    
    float x1 = v1.get("x").as<float>();
    float y1 = v1.get("y").as<float>();
    float x2 = v2.get("x").as<float>();
    float y2 = v2.get("y").as<float>();
    
    ScriptTable result;
    result.set("x", ScriptValue(x1 + x2));
    result.set("y", ScriptValue(y1 + y2));
    return ScriptValue(result);
}

ScriptValue MathScriptBindings::vec2Sub(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isTable() || !args[1].isTable()) {
        throw ScriptException("vec2Sub requires two vec2 tables");
    }
    
    auto v1 = args[0].as<ScriptTable>();
    auto v2 = args[1].as<ScriptTable>();
    
    float x1 = v1.get("x").as<float>();
    float y1 = v1.get("y").as<float>();
    float x2 = v2.get("x").as<float>();
    float y2 = v2.get("y").as<float>();
    
    ScriptTable result;
    result.set("x", ScriptValue(x1 - x2));
    result.set("y", ScriptValue(y1 - y2));
    return ScriptValue(result);
}

ScriptValue MathScriptBindings::vec2Mul(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isTable()) {
        throw ScriptException("vec2Mul requires vec2 and scalar");
    }
    
    auto v = args[0].as<ScriptTable>();
    float scalar = args[1].as<float>();
    
    float x = v.get("x").as<float>();
    float y = v.get("y").as<float>();
    
    ScriptTable result;
    result.set("x", ScriptValue(x * scalar));
    result.set("y", ScriptValue(y * scalar));
    return ScriptValue(result);
}

ScriptValue MathScriptBindings::vec2Dot(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isTable() || !args[1].isTable()) {
        throw ScriptException("vec2Dot requires two vec2 tables");
    }
    
    auto v1 = args[0].as<ScriptTable>();
    auto v2 = args[1].as<ScriptTable>();
    
    float x1 = v1.get("x").as<float>();
    float y1 = v1.get("y").as<float>();
    float x2 = v2.get("x").as<float>();
    float y2 = v2.get("y").as<float>();
    
    return ScriptValue(x1 * x2 + y1 * y2);
}

ScriptValue MathScriptBindings::vec2Length(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isTable()) {
        throw ScriptException("vec2Length requires vec2 table");
    }
    
    auto v = args[0].as<ScriptTable>();
    float x = v.get("x").as<float>();
    float y = v.get("y").as<float>();
    
    return ScriptValue(std::sqrt(x * x + y * y));
}

ScriptValue MathScriptBindings::vec2Normalize(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isTable()) {
        throw ScriptException("vec2Normalize requires vec2 table");
    }
    
    auto v = args[0].as<ScriptTable>();
    float x = v.get("x").as<float>();
    float y = v.get("y").as<float>();
    float len = std::sqrt(x * x + y * y);
    
    if (len > 0.0f) {
        x /= len;
        y /= len;
    }
    
    ScriptTable result;
    result.set("x", ScriptValue(x));
    result.set("y", ScriptValue(y));
    return ScriptValue(result);
}

ScriptValue MathScriptBindings::vec2Distance(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isTable() || !args[1].isTable()) {
        throw ScriptException("vec2Distance requires two vec2 tables");
    }
    
    auto v1 = args[0].as<ScriptTable>();
    auto v2 = args[1].as<ScriptTable>();
    
    float x1 = v1.get("x").as<float>();
    float y1 = v1.get("y").as<float>();
    float x2 = v2.get("x").as<float>();
    float y2 = v2.get("y").as<float>();
    
    float dx = x2 - x1;
    float dy = y2 - y1;
    
    return ScriptValue(std::sqrt(dx * dx + dy * dy));
}

ScriptValue MathScriptBindings::lerp(const std::vector<ScriptValue>& args) {
    if (args.size() < 3) {
        throw ScriptException("lerp requires start, end, and t");
    }
    
    float a = args[0].as<float>();
    float b = args[1].as<float>();
    float t = args[2].as<float>();
    
    return ScriptValue(a + (b - a) * t);
}

ScriptValue MathScriptBindings::clamp(const std::vector<ScriptValue>& args) {
    if (args.size() < 3) {
        throw ScriptException("clamp requires value, min, and max");
    }
    
    float value = args[0].as<float>();
    float min = args[1].as<float>();
    float max = args[2].as<float>();
    
    if (value < min) return ScriptValue(min);
    if (value > max) return ScriptValue(max);
    return ScriptValue(value);
}

// Input script bindings
void InputScriptBindings::registerBindings(ScriptContext* context) {
    context->registerFunction("isKeyDown", isKeyDown);
    context->registerFunction("isKeyPressed", isKeyPressed);
    context->registerFunction("isKeyReleased", isKeyReleased);
    context->registerFunction("isMouseButtonDown", isMouseButtonDown);
    context->registerFunction("getMousePosition", getMousePosition);
    context->registerFunction("getMouseDelta", getMouseDelta);
}

ScriptValue InputScriptBindings::isKeyDown(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isString()) {
        throw ScriptException("isKeyDown requires key name");
    }
    // TODO: Integrate with actual input system
    return ScriptValue(false);
}

ScriptValue InputScriptBindings::isKeyPressed(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isString()) {
        throw ScriptException("isKeyPressed requires key name");
    }
    // TODO: Integrate with actual input system
    return ScriptValue(false);
}

ScriptValue InputScriptBindings::isKeyReleased(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isString()) {
        throw ScriptException("isKeyReleased requires key name");
    }
    // TODO: Integrate with actual input system
    return ScriptValue(false);
}

ScriptValue InputScriptBindings::isMouseButtonDown(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isInt()) {
        throw ScriptException("isMouseButtonDown requires button index");
    }
    // TODO: Integrate with actual input system
    return ScriptValue(false);
}

ScriptValue InputScriptBindings::getMousePosition(const std::vector<ScriptValue>& args) {
    (void)args;
    // TODO: Integrate with actual input system
    ScriptTable result;
    result.set("x", ScriptValue(0.0f));
    result.set("y", ScriptValue(0.0f));
    return ScriptValue(result);
}

ScriptValue InputScriptBindings::getMouseDelta(const std::vector<ScriptValue>& args) {
    (void)args;
    // TODO: Integrate with actual input system
    ScriptTable result;
    result.set("x", ScriptValue(0.0f));
    result.set("y", ScriptValue(0.0f));
    return ScriptValue(result);
}

// Physics script bindings
void PhysicsScriptBindings::registerBindings(ScriptContext* context) {
    context->registerFunction("applyForce", applyForce);
    context->registerFunction("applyImpulse", applyImpulse);
    context->registerFunction("setVelocity", setVelocity);
    context->registerFunction("getVelocity", getVelocity);
    context->registerFunction("raycast", raycast);
    context->registerFunction("checkCollision", checkCollision);
}

ScriptValue PhysicsScriptBindings::applyForce(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isInt() || !args[1].isTable()) {
        throw ScriptException("applyForce requires entity ID and force vector");
    }
    // TODO: Integrate with actual physics system
    return ScriptValue();
}

ScriptValue PhysicsScriptBindings::applyImpulse(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isInt() || !args[1].isTable()) {
        throw ScriptException("applyImpulse requires entity ID and impulse vector");
    }
    // TODO: Integrate with actual physics system
    return ScriptValue();
}

ScriptValue PhysicsScriptBindings::setVelocity(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isInt() || !args[1].isTable()) {
        throw ScriptException("setVelocity requires entity ID and velocity vector");
    }
    // TODO: Integrate with actual physics system
    return ScriptValue();
}

ScriptValue PhysicsScriptBindings::getVelocity(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isInt()) {
        throw ScriptException("getVelocity requires entity ID");
    }
    // TODO: Integrate with actual physics system
    ScriptTable result;
    result.set("x", ScriptValue(0.0f));
    result.set("y", ScriptValue(0.0f));
    return ScriptValue(result);
}

ScriptValue PhysicsScriptBindings::raycast(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isTable() || !args[1].isTable()) {
        throw ScriptException("raycast requires origin and direction vectors");
    }
    // TODO: Integrate with actual physics system
    return ScriptValue();
}

ScriptValue PhysicsScriptBindings::checkCollision(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isInt() || !args[1].isInt()) {
        throw ScriptException("checkCollision requires two entity IDs");
    }
    // TODO: Integrate with actual physics system
    return ScriptValue(false);
}

// Audio script bindings
void AudioScriptBindings::registerBindings(ScriptContext* context) {
    context->registerFunction("playSound", playSound);
    context->registerFunction("playMusic", playMusic);
    context->registerFunction("stopSound", stopSound);
    context->registerFunction("stopMusic", stopMusic);
    context->registerFunction("setVolume", setVolume);
    context->registerFunction("setPitch", setPitch);
}

ScriptValue AudioScriptBindings::playSound(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isString()) {
        throw ScriptException("playSound requires sound file path");
    }
    // TODO: Integrate with actual audio system
    return ScriptValue(0); // Sound ID
}

ScriptValue AudioScriptBindings::playMusic(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isString()) {
        throw ScriptException("playMusic requires music file path");
    }
    // TODO: Integrate with actual audio system
    return ScriptValue();
}

ScriptValue AudioScriptBindings::stopSound(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isInt()) {
        throw ScriptException("stopSound requires sound ID");
    }
    // TODO: Integrate with actual audio system
    return ScriptValue();
}

ScriptValue AudioScriptBindings::stopMusic(const std::vector<ScriptValue>& args) {
    (void)args;
    // TODO: Integrate with actual audio system
    return ScriptValue();
}

ScriptValue AudioScriptBindings::setVolume(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isInt()) {
        throw ScriptException("setVolume requires sound ID and volume");
    }
    // TODO: Integrate with actual audio system
    return ScriptValue();
}

ScriptValue AudioScriptBindings::setPitch(const std::vector<ScriptValue>& args) {
    if (args.size() < 2 || !args[0].isInt()) {
        throw ScriptException("setPitch requires sound ID and pitch");
    }
    // TODO: Integrate with actual audio system
    return ScriptValue();
}

// Scene script bindings
void SceneScriptBindings::registerBindings(ScriptContext* context) {
    context->registerFunction("loadScene", loadScene);
    context->registerFunction("unloadScene", unloadScene);
    context->registerFunction("getCurrentScene", getCurrentScene);
    context->registerFunction("instantiate", instantiate);
}

ScriptValue SceneScriptBindings::loadScene(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isString()) {
        throw ScriptException("loadScene requires scene name");
    }
    // TODO: Integrate with actual scene system
    return ScriptValue();
}

ScriptValue SceneScriptBindings::unloadScene(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isString()) {
        throw ScriptException("unloadScene requires scene name");
    }
    // TODO: Integrate with actual scene system
    return ScriptValue();
}

ScriptValue SceneScriptBindings::getCurrentScene(const std::vector<ScriptValue>& args) {
    (void)args;
    // TODO: Integrate with actual scene system
    return ScriptValue("DefaultScene");
}

ScriptValue SceneScriptBindings::instantiate(const std::vector<ScriptValue>& args) {
    if (args.empty() || !args[0].isString()) {
        throw ScriptException("instantiate requires prefab name");
    }
    // TODO: Integrate with actual scene system
    return ScriptValue(0); // Entity ID
}

// ScriptUtils implementation
namespace ScriptUtils {
    ScriptValue toScriptValue(bool value) { return ScriptValue(value); }
    ScriptValue toScriptValue(int value) { return ScriptValue(value); }
    ScriptValue toScriptValue(float value) { return ScriptValue(value); }
    ScriptValue toScriptValue(const std::string& value) { return ScriptValue(value); }
    ScriptValue toScriptValue(ScriptFunction func) { return ScriptValue(func); }
    
    bool toBool(const ScriptValue& value) { return value.as<bool>(); }
    int toInt(const ScriptValue& value) { return value.as<int>(); }
    float toFloat(const ScriptValue& value) { return value.as<float>(); }
    std::string toString(const ScriptValue& value) { return value.as<std::string>(); }
    
    bool checkArgCount(const std::vector<ScriptValue>& args, size_t expected) {
        return args.size() >= expected;
    }
    
    bool checkArgType(const ScriptValue& arg, ScriptValueType expected) {
        return arg.type == expected;
    }
}

} // namespace Scripting
} // namespace JJM
