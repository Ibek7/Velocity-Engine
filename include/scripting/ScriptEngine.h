#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <queue>
#include <chrono>
#include <optional>
#include <variant>

namespace JJM {
namespace Scripting {

enum class ScriptType {
    LUA,
    JAVASCRIPT,
    PYTHON
};

// =============================================================================
// Script Value
// =============================================================================

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
    
    ScriptFunction() = default;
    ScriptFunction(const std::string& name, CppFunction func);
    
    ScriptValue call(const std::vector<ScriptValue>& args) const;
    const std::string& getName() const { return name; }
    const CppFunction& getFunction() const { return function; }
    
private:
    std::string name;
    CppFunction function;
};

// =============================================================================
// Coroutine System
// =============================================================================

/**
 * @brief Coroutine execution state
 */
enum class CoroutineState {
    Created,        // Just created, not started
    Running,        // Currently executing
    Suspended,      // Yielded, waiting to resume
    Waiting,        // Waiting for condition/timer
    Completed,      // Finished successfully
    Error,          // Finished with error
    Cancelled       // Manually cancelled
};

/**
 * @brief Yield instruction types
 */
enum class YieldType {
    None,           // No yield, continue execution
    Frame,          // Wait until next frame
    Seconds,        // Wait for duration
    Frames,         // Wait for N frames
    Condition,      // Wait until condition is true
    All,            // Wait for all coroutines to complete
    Any,            // Wait for any coroutine to complete
    Custom          // Custom yield handler
};

/**
 * @brief Yield instruction returned by coroutines
 */
struct YieldInstruction {
    YieldType type;
    float waitSeconds;
    int waitFrames;
    std::function<bool()> condition;
    std::vector<uint64_t> waitingForCoroutines;
    std::string customYieldName;
    
    YieldInstruction()
        : type(YieldType::None)
        , waitSeconds(0.0f)
        , waitFrames(0)
    {}
    
    static YieldInstruction WaitForNextFrame() {
        YieldInstruction yi;
        yi.type = YieldType::Frame;
        return yi;
    }
    
    static YieldInstruction WaitForSeconds(float seconds) {
        YieldInstruction yi;
        yi.type = YieldType::Seconds;
        yi.waitSeconds = seconds;
        return yi;
    }
    
    static YieldInstruction WaitForFrames(int frames) {
        YieldInstruction yi;
        yi.type = YieldType::Frames;
        yi.waitFrames = frames;
        return yi;
    }
    
    static YieldInstruction WaitUntil(std::function<bool()> cond) {
        YieldInstruction yi;
        yi.type = YieldType::Condition;
        yi.condition = std::move(cond);
        return yi;
    }
    
    static YieldInstruction WaitWhile(std::function<bool()> cond) {
        YieldInstruction yi;
        yi.type = YieldType::Condition;
        yi.condition = [cond]() { return !cond(); };
        return yi;
    }
};

/**
 * @brief Coroutine context containing state and local variables
 */
struct CoroutineContext {
    uint64_t id;
    std::string name;
    CoroutineState state;
    YieldInstruction currentYield;
    
    // Timing
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point yieldTime;
    std::chrono::steady_clock::time_point resumeTime;
    float elapsedTime;
    int frameCount;
    int framesWaited;
    
    // Results
    ScriptValue returnValue;
    std::string errorMessage;
    
    // Parent/child relationships
    std::optional<uint64_t> parentCoroutine;
    std::vector<uint64_t> childCoroutines;
    
    // Local storage
    std::map<std::string, ScriptValue> locals;
    
    CoroutineContext()
        : id(0)
        , state(CoroutineState::Created)
        , elapsedTime(0.0f)
        , frameCount(0)
        , framesWaited(0)
    {}
};

/**
 * @brief Function type for coroutine body
 */
using CoroutineFunction = std::function<YieldInstruction(CoroutineContext&)>;

/**
 * @brief Async operation result
 */
template<typename T>
class AsyncResult {
private:
    std::optional<T> value;
    std::optional<std::string> error;
    bool completed;
    
public:
    AsyncResult() : completed(false) {}
    
    void setResult(const T& val) {
        value = val;
        completed = true;
    }
    
    void setError(const std::string& err) {
        error = err;
        completed = true;
    }
    
    bool isCompleted() const { return completed; }
    bool hasError() const { return error.has_value(); }
    const std::string& getError() const { return error.value(); }
    const T& getValue() const { return value.value(); }
};

/**
 * @brief Coroutine scheduler and manager
 */
class CoroutineScheduler {
private:
    static CoroutineScheduler* instance;
    
    std::map<uint64_t, std::unique_ptr<CoroutineContext>> coroutines;
    std::map<uint64_t, CoroutineFunction> coroutineFunctions;
    std::vector<uint64_t> activeCoroutines;
    std::queue<uint64_t> pendingStart;
    
    uint64_t nextCoroutineId;
    int currentFrame;
    float deltaTime;
    
    // Custom yield handlers
    std::map<std::string, std::function<bool(CoroutineContext&)>> customYieldHandlers;
    
    // Statistics
    struct CoroutineStats {
        uint64_t totalCreated;
        uint64_t totalCompleted;
        uint64_t totalCancelled;
        uint64_t totalErrors;
        uint64_t currentActive;
        float averageLifetime;
    };
    CoroutineStats stats;
    
    CoroutineScheduler();
    
public:
    ~CoroutineScheduler();
    
    static CoroutineScheduler* getInstance();
    static void cleanup();
    
    // Coroutine creation
    uint64_t startCoroutine(const std::string& name, CoroutineFunction func);
    uint64_t startCoroutine(CoroutineFunction func);
    uint64_t startChildCoroutine(uint64_t parentId, const std::string& name, CoroutineFunction func);
    
    // Coroutine control
    void stopCoroutine(uint64_t id);
    void stopAllCoroutines();
    void pauseCoroutine(uint64_t id);
    void resumeCoroutine(uint64_t id);
    
    // Query
    bool isRunning(uint64_t id) const;
    bool isCompleted(uint64_t id) const;
    CoroutineState getState(uint64_t id) const;
    CoroutineContext* getContext(uint64_t id);
    const CoroutineContext* getContext(uint64_t id) const;
    
    // Update (call once per frame)
    void update(float dt);
    
    // Custom yield handlers
    void registerYieldHandler(const std::string& name, std::function<bool(CoroutineContext&)> handler);
    void unregisterYieldHandler(const std::string& name);
    
    // Statistics
    const CoroutineStats& getStats() const { return stats; }
    void resetStats();
    size_t getActiveCount() const { return activeCoroutines.size(); }
    std::vector<std::string> getActiveCoroutineNames() const;
    
private:
    void processCoroutine(uint64_t id);
    bool checkYieldCondition(CoroutineContext& ctx);
    void completeCoroutine(uint64_t id, bool success);
    void removeCoroutine(uint64_t id);
};

/**
 * @brief Async operation wrapper for script integration
 */
class AsyncOperation {
private:
    uint64_t coroutineId;
    std::function<void()> onComplete;
    std::function<void(const std::string&)> onError;
    
public:
    AsyncOperation(uint64_t coroId);
    
    AsyncOperation& then(std::function<void()> callback);
    AsyncOperation& catchError(std::function<void(const std::string&)> callback);
    
    void cancel();
    bool isComplete() const;
    bool hasError() const;
};

/**
 * @brief Sequence builder for chaining coroutines
 */
class CoroutineSequence {
private:
    std::vector<CoroutineFunction> steps;
    std::string name;
    bool parallel;
    
public:
    CoroutineSequence(const std::string& sequenceName = "");
    
    CoroutineSequence& then(CoroutineFunction step);
    CoroutineSequence& wait(float seconds);
    CoroutineSequence& waitFrames(int frames);
    CoroutineSequence& waitUntil(std::function<bool()> condition);
    
    uint64_t start();
    uint64_t startParallel();
};

/**
 * @brief Sandboxed script execution environment with controlled API access
 */
class ScriptSandbox {
public:
    enum class AccessLevel {
        NONE,           // No access
        READ_ONLY,      // Can read but not modify
        LIMITED,        // Limited write access (safe operations only)
        FULL            // Full access
    };
    
    struct SandboxPermissions {
        AccessLevel fileSystem;       // File I/O access level
        AccessLevel network;          // Network operations
        AccessLevel system;           // System calls (os.execute, etc.)
        AccessLevel memoryManagement; // GC control, memory limits
        AccessLevel engineAPI;        // Game engine functions
        
        size_t maxMemoryMB;           // Memory limit in megabytes
        size_t maxExecutionTimeMs;    // Execution time limit
        size_t maxInstructions;       // Instruction count limit
        bool allowCoroutines;         // Allow coroutine creation
        bool allowModuleLoading;      // Allow require/import
        
        SandboxPermissions()
            : fileSystem(AccessLevel::NONE)
            , network(AccessLevel::NONE)
            , system(AccessLevel::NONE)
            , memoryManagement(AccessLevel::LIMITED)
            , engineAPI(AccessLevel::LIMITED)
            , maxMemoryMB(16)
            , maxExecutionTimeMs(1000)
            , maxInstructions(100000)
            , allowCoroutines(true)
            , allowModuleLoading(false)
        {}
    };
    
    struct ExecutionResult {
        bool success;
        ScriptValue returnValue;
        std::string errorMessage;
        
        // Execution statistics
        size_t instructionsExecuted;
        size_t memoryUsedBytes;
        float executionTimeMs;
        bool hitMemoryLimit;
        bool hitTimeLimit;
        bool hitInstructionLimit;
    };
    
private:
    ScriptEngine* m_engine;
    SandboxPermissions m_permissions;
    std::map<std::string, bool> m_allowedFunctions;
    std::map<std::string, bool> m_allowedModules;
    
    // Execution monitoring
    std::chrono::steady_clock::time_point m_executionStartTime;
    size_t m_instructionCount;
    bool m_isExecuting;
    
public:
    ScriptSandbox(ScriptType type = ScriptType::LUA);
    ~ScriptSandbox();
    
    // Sandbox configuration
    void setPermissions(const SandboxPermissions& perms) { m_permissions = perms; }
    const SandboxPermissions& getPermissions() const { return m_permissions; }
    
    void allowFunction(const std::string& functionName, bool allowed = true);
    void allowModule(const std::string& moduleName, bool allowed = true);
    bool isFunctionAllowed(const std::string& functionName) const;
    bool isModuleAllowed(const std::string& moduleName) const;
    
    // Controlled API exposure
    void exposeEngineAPI(const std::string& apiName, AccessLevel level);
    void registerSafeFunction(const std::string& name, ScriptFunction::CppFunction func);
    void setGlobalValue(const std::string& name, const ScriptValue& value, bool readOnly = false);
    
    // Sandboxed execution
    ExecutionResult executeString(const std::string& script);
    ExecutionResult executeFile(const std::string& filename);
    ExecutionResult callFunction(const std::string& name, const std::vector<ScriptValue>& args = {});
    
    // Security checks (called by engine hooks)
    bool checkMemoryAllocation(size_t bytes);
    bool checkFunctionCall(const std::string& functionName);
    bool checkModuleLoad(const std::string& moduleName);
    void onInstructionExecuted();
    
    // Preset configurations
    static SandboxPermissions getPresetPermissions(const std::string& presetName);
    static std::vector<std::string> getAvailablePresets();
    
private:
    void setupSandboxEnvironment();
    void setupLuaSandbox();
    void installExecutionHooks();
    void checkExecutionLimits();
    void resetExecutionCounters();
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

// Coroutine macros
#define START_COROUTINE(name, func) \
    CoroutineScheduler::getInstance()->startCoroutine(name, func)

#define STOP_COROUTINE(id) \
    CoroutineScheduler::getInstance()->stopCoroutine(id)

#define YIELD_FRAME() \
    return YieldInstruction::WaitForNextFrame()

#define YIELD_SECONDS(seconds) \
    return YieldInstruction::WaitForSeconds(seconds)

#define YIELD_FRAMES(frames) \
    return YieldInstruction::WaitForFrames(frames)

#define YIELD_UNTIL(condition) \
    return YieldInstruction::WaitUntil(condition)

#define YIELD_WHILE(condition) \
    return YieldInstruction::WaitWhile(condition)

} // namespace Scripting
} // namespace JJM

#endif // SCRIPT_ENGINE_H