#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Combo system for fighting games
namespace Engine {

enum class InputType {
    Button,
    Direction,
    Sequence
};

struct ComboInput {
    InputType type;
    std::string value; // "A", "B", "Up", "Down+A", etc.
    float timeWindow;  // Max time between inputs
};

struct ComboAction {
    std::string name;
    std::vector<ComboInput> inputs;
    float damageMultiplier;
    int priority;
    std::function<void()> onExecute;
    std::function<bool()> condition; // Additional conditions
};

class ComboSystem {
public:
    static ComboSystem& getInstance();

    // Combo registration
    void registerCombo(const ComboAction& combo);
    void unregisterCombo(const std::string& name);
    void clearCombos();
    
    // Input processing
    void processInput(const std::string& input);
    void update(float deltaTime);
    void reset();
    
    // Configuration
    void setMaxInputBuffer(int max) { m_maxInputBuffer = max; }
    void setInputTimeout(float seconds) { m_inputTimeout = seconds; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    // Query
    bool isComboActive() const { return m_activeCombo != nullptr; }
    std::string getActiveComboName() const;
    int getComboCount() const { return m_combos.size(); }
    float getComboMultiplier() const { return m_comboMultiplier; }
    
    // Callbacks
    using ComboCallback = std::function<void(const std::string& comboName, float multiplier)>;
    void onComboExecute(ComboCallback callback);
    void onComboBreak(ComboCallback callback);

private:
    ComboSystem();
    ComboSystem(const ComboSystem&) = delete;
    ComboSystem& operator=(const ComboSystem&) = delete;

    struct InputRecord {
        std::string input;
        float timestamp;
    };

    void checkCombos();
    bool matchesCombo(const ComboAction& combo) const;
    void executeCombo(const ComboAction& combo);
    void breakCombo();
    void cleanupOldInputs();

    std::vector<ComboAction> m_combos;
    std::vector<InputRecord> m_inputBuffer;
    
    const ComboAction* m_activeCombo;
    int m_comboChain;
    float m_comboMultiplier;
    
    int m_maxInputBuffer;
    float m_inputTimeout;
    float m_totalTime;
    bool m_enabled;
    
    std::vector<ComboCallback> m_executeCallbacks;
    std::vector<ComboCallback> m_breakCallbacks;
};

} // namespace Engine
