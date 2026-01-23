#include "../../include/scripting/ScriptBinding.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/stat.h>

namespace JJM {
namespace Scripting {

// ============================================================================
// ScriptValue Implementation
// ============================================================================

ScriptValue::ScriptValue() : m_type(ScriptValueType::NIL) {}

ScriptValue::ScriptValue(bool value) : m_type(ScriptValueType::BOOLEAN) {
    m_value = value;
}

ScriptValue::ScriptValue(int value) : m_type(ScriptValueType::INTEGER) {
    m_value = value;
}

ScriptValue::ScriptValue(float value) : m_type(ScriptValueType::FLOAT) {
    m_value = value;
}

ScriptValue::ScriptValue(const char* value) : m_type(ScriptValueType::STRING) {
    m_value = std::string(value);
}

ScriptValue::ScriptValue(const std::string& value) : m_type(ScriptValueType::STRING) {
    m_value = value;
}

ScriptValue::~ScriptValue() {}

bool ScriptValue::asBool(bool defaultValue) const {
    if (m_type == ScriptValueType::BOOLEAN) {
        return std::any_cast<bool>(m_value);
    }
    return defaultValue;
}

int ScriptValue::asInt(int defaultValue) const {
    if (m_type == ScriptValueType::INTEGER) {
        return std::any_cast<int>(m_value);
    }
    if (m_type == ScriptValueType::FLOAT) {
        return static_cast<int>(std::any_cast<float>(m_value));
    }
    return defaultValue;
}

float ScriptValue::asFloat(float defaultValue) const {
    if (m_type == ScriptValueType::FLOAT) {
        return std::any_cast<float>(m_value);
    }
    if (m_type == ScriptValueType::INTEGER) {
        return static_cast<float>(std::any_cast<int>(m_value));
    }
    return defaultValue;
}

std::string ScriptValue::asString(const std::string& defaultValue) const {
    if (m_type == ScriptValueType::STRING) {
        return std::any_cast<std::string>(m_value);
    }
    return defaultValue;
}

// ============================================================================
// LuaContext Implementation (Stub)
// ============================================================================

LuaContext::LuaContext() : m_luaState(nullptr) {
    // In real implementation: m_luaState = luaL_newstate();
    // luaL_openlibs(m_luaState);
}

LuaContext::~LuaContext() {
    // In real implementation: lua_close(m_luaState);
}

void LuaContext::setGlobal(const std::string& name, const ScriptValue& value) {
    // Push value to Lua stack and set as global
}

ScriptValue LuaContext::getGlobal(const std::string& name) {
    // Get global from Lua and convert to ScriptValue
    return ScriptValue();
}

void LuaContext::registerFunction(const std::string& name, ScriptNativeFunction function) {
    m_nativeFunctions[name] = function;
    // Register with Lua
}

bool LuaContext::executeString(const std::string& code) {
    // luaL_dostring(m_luaState, code.c_str())
    return true;
}

bool LuaContext::executeFile(const std::string& filename) {
    // luaL_dofile(m_luaState, filename.c_str())
    return true;
}

ScriptValue LuaContext::evaluate(const std::string& expression) {
    executeString("return " + expression);
    return popValue();
}

ScriptValue LuaContext::callFunction(const std::string& name, 
                                    const std::vector<ScriptValue>& args) {
    // Get function, push args, pcall, get result
    return ScriptValue();
}

void LuaContext::pushValue(const ScriptValue& value) {
    // Convert ScriptValue to Lua value on stack
}

ScriptValue LuaContext::popValue() {
    // Convert top of Lua stack to ScriptValue
    return ScriptValue();
}

void LuaContext::setError(const std::string& error) {
    m_lastError = error;
}

// ============================================================================
// PythonContext Implementation (Stub)
// ============================================================================

PythonContext::PythonContext() : m_mainModule(nullptr), m_globalDict(nullptr) {
    // In real implementation: Py_Initialize()
    // m_mainModule = PyImport_AddModule("__main__");
    // m_globalDict = PyModule_GetDict(m_mainModule);
}

PythonContext::~PythonContext() {
    // In real implementation: cleanup Python refs
}

void PythonContext::setGlobal(const std::string& name, const ScriptValue& value) {
    // Convert to PyObject and add to dict
}

ScriptValue PythonContext::getGlobal(const std::string& name) {
    // Get from dict and convert to ScriptValue
    return ScriptValue();
}

void PythonContext::registerFunction(const std::string& name, ScriptNativeFunction function) {
    m_nativeFunctions[name] = function;
    // Register with Python
}

bool PythonContext::executeString(const std::string& code) {
    // PyRun_SimpleString(code.c_str())
    return true;
}

bool PythonContext::executeFile(const std::string& filename) {
    // PyRun_SimpleFile(fp, filename.c_str())
    return true;
}

ScriptValue PythonContext::evaluate(const std::string& expression) {
    // PyRun_String with Py_eval_input
    return ScriptValue();
}

ScriptValue PythonContext::callFunction(const std::string& name,
                                       const std::vector<ScriptValue>& args) {
    // PyObject_CallObject
    return ScriptValue();
}

void PythonContext::setError(const std::string& error) {
    m_lastError = error;
}

// ============================================================================
// ScriptEngine Implementation
// ============================================================================

ScriptEngine::ScriptEngine() 
    : m_defaultLanguage(ScriptLanguage::LUA)
    , m_initialized(false)
    , m_hotReloadEnabled(false) {
}

ScriptEngine::~ScriptEngine() {
    shutdown();
}

bool ScriptEngine::initialize(ScriptLanguage defaultLanguage) {
    if (m_initialized) return true;
    
    m_defaultLanguage = defaultLanguage;
    m_defaultContext = std::unique_ptr<ScriptContext>(createContext(defaultLanguage));
    
    if (!m_defaultContext) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void ScriptEngine::shutdown() {
    m_contexts.clear();
    m_defaultContext.reset();
    m_watchedFiles.clear();
    m_initialized = false;
}

ScriptContext* ScriptEngine::createContext(ScriptLanguage language) {
    ScriptContext* context = nullptr;
    
    switch (language) {
        case ScriptLanguage::LUA:
            context = new LuaContext();
            break;
        case ScriptLanguage::PYTHON:
            context = new PythonContext();
            break;
        case ScriptLanguage::JAVASCRIPT:
            // Would create JS context
            break;
    }
    
    if (context) {
        // Register global functions in new context
        for (const auto& pair : m_globalFunctions) {
            context->registerFunction(pair.first, pair.second);
        }
        m_contexts.push_back(std::unique_ptr<ScriptContext>(context));
    }
    
    return context;
}

void ScriptEngine::destroyContext(ScriptContext* context) {
    m_contexts.erase(
        std::remove_if(m_contexts.begin(), m_contexts.end(),
            [context](const std::unique_ptr<ScriptContext>& ctx) {
                return ctx.get() == context;
            }),
        m_contexts.end()
    );
}

bool ScriptEngine::execute(const std::string& code) {
    if (!m_defaultContext) return false;
    return m_defaultContext->executeString(code);
}

bool ScriptEngine::executeFile(const std::string& filename) {
    if (!m_defaultContext) return false;
    return m_defaultContext->executeFile(filename);
}

ScriptValue ScriptEngine::evaluate(const std::string& expression) {
    if (!m_defaultContext) return ScriptValue();
    return m_defaultContext->evaluate(expression);
}

void ScriptEngine::registerGlobalFunction(const std::string& name, 
                                         ScriptNativeFunction function) {
    m_globalFunctions[name] = function;
    
    // Register in all existing contexts
    if (m_defaultContext) {
        m_defaultContext->registerFunction(name, function);
    }
    for (auto& context : m_contexts) {
        context->registerFunction(name, function);
    }
}

void ScriptEngine::addModulePath(const std::string& path) {
    m_modulePaths.push_back(path);
}

bool ScriptEngine::loadModule(const std::string& moduleName) {
    // Search module paths and load
    for (const auto& path : m_modulePaths) {
        std::string fullPath = path + "/" + moduleName;
        // Try to load
    }
    return false;
}

std::string ScriptEngine::getLastError() const {
    if (m_defaultContext) {
        return m_defaultContext->getLastError();
    }
    return "";
}

bool ScriptEngine::hasError() const {
    if (m_defaultContext) {
        return m_defaultContext->hasError();
    }
    return false;
}

void ScriptEngine::clearError() {
    if (m_defaultContext) {
        m_defaultContext->clearError();
    }
}

void ScriptEngine::watchFile(const std::string& filename) {
    struct stat statbuf;
    if (stat(filename.c_str(), &statbuf) == 0) {
        m_watchedFiles[filename] = statbuf.st_mtime;
    }
}

void ScriptEngine::unwatchFile(const std::string& filename) {
    m_watchedFiles.erase(filename);
}

void ScriptEngine::checkForChanges() {
    if (!m_hotReloadEnabled) return;
    
    for (auto& pair : m_watchedFiles) {
        struct stat statbuf;
        if (stat(pair.first.c_str(), &statbuf) == 0) {
            if (statbuf.st_mtime > pair.second) {
                // File changed, reload
                executeFile(pair.first);
                pair.second = statbuf.st_mtime;
            }
        }
    }
}

// ============================================================================
// EngineBindings Implementation
// ============================================================================

void EngineBindings::registerMathBindings(ScriptEngine* engine) {
    // Register math functions
    engine->registerGlobalFunction("sin", [](const std::vector<ScriptValue>& args) {
        if (args.empty()) return ScriptValue(0.0f);
        return ScriptValue(std::sin(args[0].asFloat()));
    });
    
    engine->registerGlobalFunction("cos", [](const std::vector<ScriptValue>& args) {
        if (args.empty()) return ScriptValue(0.0f);
        return ScriptValue(std::cos(args[0].asFloat()));
    });
    
    engine->registerGlobalFunction("sqrt", [](const std::vector<ScriptValue>& args) {
        if (args.empty()) return ScriptValue(0.0f);
        return ScriptValue(std::sqrt(args[0].asFloat()));
    });
}

void EngineBindings::registerInputBindings(ScriptEngine* engine) {
    // Register input functions
    engine->registerGlobalFunction("isKeyPressed", [](const std::vector<ScriptValue>& args) {
        // Check key state
        return ScriptValue(false);
    });
}

void EngineBindings::registerGraphicsBindings(ScriptEngine* engine) {
    // Register graphics functions
    engine->registerGlobalFunction("drawText", [](const std::vector<ScriptValue>& args) {
        // Draw text
        return ScriptValue();
    });
}

void EngineBindings::registerAudioBindings(ScriptEngine* engine) {
    // Register audio functions
    engine->registerGlobalFunction("playSound", [](const std::vector<ScriptValue>& args) {
        // Play sound
        return ScriptValue();
    });
}

void EngineBindings::registerPhysicsBindings(ScriptEngine* engine) {
    // Register physics functions
}

void EngineBindings::registerECSBindings(ScriptEngine* engine) {
    // Register ECS functions
    engine->registerGlobalFunction("createEntity", [](const std::vector<ScriptValue>& args) {
        // Create entity
        return ScriptValue(0);
    });
}

void EngineBindings::registerAllBindings(ScriptEngine* engine) {
    registerMathBindings(engine);
    registerInputBindings(engine);
    registerGraphicsBindings(engine);
    registerAudioBindings(engine);
    registerPhysicsBindings(engine);
    registerECSBindings(engine);
}

// ============================================================================
// ScriptSystem Implementation
// ============================================================================

ScriptSystem::ScriptSystem(ScriptEngine* engine) : m_engine(engine) {
}

ScriptSystem::~ScriptSystem() {
    shutdown();
}

void ScriptSystem::initialize() {
    // Initialize script system
}

void ScriptSystem::update(float deltaTime) {
    for (auto& pair : m_components) {
        ScriptComponent& component = pair.second;
        
        if (!component.initialized) {
            initializeScript(pair.first, component);
        }
        
        if (component.autoUpdate && component.initialized) {
            updateScript(pair.first, component, deltaTime);
        }
    }
    
    // Check for hot reload
    if (m_engine->isHotReloadEnabled()) {
        m_engine->checkForChanges();
    }
}

void ScriptSystem::shutdown() {
    for (auto& pair : m_components) {
        destroyScript(pair.first, pair.second);
    }
    m_components.clear();
}

void ScriptSystem::addScriptComponent(int entityId, const std::string& scriptFile) {
    ScriptComponent component;
    component.scriptFile = scriptFile;
    component.context = m_engine->createContext(
        ScriptUtils::detectLanguageFromExtension(scriptFile));
    
    m_components[entityId] = component;
    
    // Load script file
    if (component.context) {
        component.context->executeFile(scriptFile);
    }
    
    // Enable hot reload for this file
    if (m_engine->isHotReloadEnabled()) {
        m_engine->watchFile(scriptFile);
    }
}

void ScriptSystem::removeScriptComponent(int entityId) {
    auto it = m_components.find(entityId);
    if (it != m_components.end()) {
        destroyScript(entityId, it->second);
        m_engine->destroyContext(it->second.context);
        m_components.erase(it);
    }
}

ScriptComponent* ScriptSystem::getScriptComponent(int entityId) {
    auto it = m_components.find(entityId);
    return (it != m_components.end()) ? &it->second : nullptr;
}

void ScriptSystem::reloadScript(int entityId) {
    auto* component = getScriptComponent(entityId);
    if (component && component->context) {
        component->context->executeFile(component->scriptFile);
        component->initialized = false;
    }
}

void ScriptSystem::reloadAllScripts() {
    for (auto& pair : m_components) {
        reloadScript(pair.first);
    }
}

void ScriptSystem::initializeScript(int entityId, ScriptComponent& component) {
    if (!component.context) return;
    
    // Set entity ID as global
    component.context->setGlobal("entityId", ScriptValue(entityId));
    
    // Call onInit if it exists
    component.context->callFunction(component.onInitFunction, {});
    
    component.initialized = true;
}

void ScriptSystem::updateScript(int entityId, ScriptComponent& component, 
                               float deltaTime) {
    if (!component.context) return;
    
    // Call onUpdate with deltaTime
    std::vector<ScriptValue> args = { ScriptValue(deltaTime) };
    component.context->callFunction(component.onUpdateFunction, args);
}

void ScriptSystem::destroyScript(int entityId, ScriptComponent& component) {
    if (!component.context || !component.initialized) return;
    
    // Call onDestroy
    component.context->callFunction(component.onDestroyFunction, {});
}

// ============================================================================
// ScriptUtils Implementation
// ============================================================================

namespace ScriptUtils {

ScriptValue toScriptValue(bool value) {
    return ScriptValue(value);
}

ScriptValue toScriptValue(int value) {
    return ScriptValue(value);
}

ScriptValue toScriptValue(float value) {
    return ScriptValue(value);
}

ScriptValue toScriptValue(const std::string& value) {
    return ScriptValue(value);
}

ScriptValue vectorToTable(const std::vector<float>& vec) {
    // Convert vector to script table/array
    return ScriptValue();
}

ScriptValue vectorToTable(const std::vector<int>& vec) {
    // Convert vector to script table/array
    return ScriptValue();
}

std::vector<float> tableToFloatVector(const ScriptValue& table) {
    // Convert script table to vector
    return std::vector<float>();
}

std::vector<int> tableToIntVector(const ScriptValue& table) {
    // Convert script table to vector
    return std::vector<int>();
}

std::string loadScriptFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return "";
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool saveScriptFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    file << content;
    return true;
}

std::string getScriptExtension(ScriptLanguage language) {
    switch (language) {
        case ScriptLanguage::LUA: return ".lua";
        case ScriptLanguage::PYTHON: return ".py";
        case ScriptLanguage::JAVASCRIPT: return ".js";
        default: return ".txt";
    }
}

ScriptLanguage detectLanguageFromExtension(const std::string& filename) {
    if (filename.ends_with(".lua")) return ScriptLanguage::LUA;
    if (filename.ends_with(".py")) return ScriptLanguage::PYTHON;
    if (filename.ends_with(".js")) return ScriptLanguage::JAVASCRIPT;
    return ScriptLanguage::LUA;  // Default
}

} // namespace ScriptUtils

} // namespace Scripting
} // namespace JJM
