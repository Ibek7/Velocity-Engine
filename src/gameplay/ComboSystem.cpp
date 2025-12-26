#include "gameplay/ComboSystem.h"
#include <algorithm>

namespace Engine {

ComboSystem::ComboSystem()
    : m_activeCombo(nullptr)
    , m_comboChain(0)
    , m_comboMultiplier(1.0f)
    , m_maxInputBuffer(10)
    , m_inputTimeout(1.0f)
    , m_totalTime(0.0f)
    , m_enabled(true)
{
}

ComboSystem& ComboSystem::getInstance() {
    static ComboSystem instance;
    return instance;
}

void ComboSystem::registerCombo(const ComboAction& combo) {
    m_combos.push_back(combo);
    
    // Sort by priority (higher first)
    std::sort(m_combos.begin(), m_combos.end(),
        [](const ComboAction& a, const ComboAction& b) {
            return a.priority > b.priority;
        });
}

void ComboSystem::unregisterCombo(const std::string& name) {
    m_combos.erase(
        std::remove_if(m_combos.begin(), m_combos.end(),
            [&name](const ComboAction& combo) { return combo.name == name; }),
        m_combos.end()
    );
}

void ComboSystem::clearCombos() {
    m_combos.clear();
}

void ComboSystem::processInput(const std::string& input) {
    if (!m_enabled) return;
    
    // Add to input buffer
    InputRecord record;
    record.input = input;
    record.timestamp = m_totalTime;
    
    m_inputBuffer.push_back(record);
    
    // Limit buffer size
    if (static_cast<int>(m_inputBuffer.size()) > m_maxInputBuffer) {
        m_inputBuffer.erase(m_inputBuffer.begin());
    }
    
    // Check for combo matches
    checkCombos();
}

void ComboSystem::update(float deltaTime) {
    m_totalTime += deltaTime;
    
    // Clean up old inputs
    cleanupOldInputs();
    
    // Break combo if timeout
    if (m_activeCombo && !m_inputBuffer.empty()) {
        float lastInputTime = m_inputBuffer.back().timestamp;
        if (m_totalTime - lastInputTime > m_inputTimeout * 2.0f) {
            breakCombo();
        }
    }
}

void ComboSystem::reset() {
    m_inputBuffer.clear();
    m_activeCombo = nullptr;
    m_comboChain = 0;
    m_comboMultiplier = 1.0f;
}

void ComboSystem::checkCombos() {
    // Check combos in priority order
    for (const auto& combo : m_combos) {
        if (matchesCombo(combo)) {
            executeCombo(combo);
            return;
        }
    }
}

bool ComboSystem::matchesCombo(const ComboAction& combo) const {
    if (combo.inputs.empty()) return false;
    
    // Check if we have enough inputs
    if (m_inputBuffer.size() < combo.inputs.size()) {
        return false;
    }
    
    // Check if condition is met
    if (combo.condition && !combo.condition()) {
        return false;
    }
    
    // Match inputs from the end of the buffer
    size_t bufferIdx = m_inputBuffer.size() - 1;
    
    for (int i = combo.inputs.size() - 1; i >= 0; --i) {
        const auto& comboInput = combo.inputs[i];
        const auto& bufferedInput = m_inputBuffer[bufferIdx];
        
        // Check if input matches
        if (bufferedInput.input != comboInput.value) {
            return false;
        }
        
        // Check timing
        if (i < static_cast<int>(combo.inputs.size()) - 1) {
            float timeDiff = m_inputBuffer[bufferIdx + 1].timestamp - bufferedInput.timestamp;
            if (timeDiff > comboInput.timeWindow) {
                return false;
            }
        }
        
        if (bufferIdx == 0) break;
        --bufferIdx;
    }
    
    return true;
}

void ComboSystem::executeCombo(const ComboAction& combo) {
    m_activeCombo = &combo;
    m_comboChain++;
    
    // Increase multiplier
    m_comboMultiplier = 1.0f + (m_comboChain - 1) * 0.1f;
    m_comboMultiplier *= combo.damageMultiplier;
    
    // Execute combo action
    if (combo.onExecute) {
        combo.onExecute();
    }
    
    // Notify callbacks
    for (auto& callback : m_executeCallbacks) {
        callback(combo.name, m_comboMultiplier);
    }
    
    // Clear input buffer after successful combo
    m_inputBuffer.clear();
}

void ComboSystem::breakCombo() {
    if (m_activeCombo) {
        for (auto& callback : m_breakCallbacks) {
            callback(m_activeCombo->name, m_comboMultiplier);
        }
    }
    
    m_activeCombo = nullptr;
    m_comboChain = 0;
    m_comboMultiplier = 1.0f;
    m_inputBuffer.clear();
}

void ComboSystem::cleanupOldInputs() {
    m_inputBuffer.erase(
        std::remove_if(m_inputBuffer.begin(), m_inputBuffer.end(),
            [this](const InputRecord& record) {
                return (m_totalTime - record.timestamp) > m_inputTimeout;
            }),
        m_inputBuffer.end()
    );
}

std::string ComboSystem::getActiveComboName() const {
    if (m_activeCombo) {
        return m_activeCombo->name;
    }
    return "";
}

void ComboSystem::onComboExecute(ComboCallback callback) {
    m_executeCallbacks.push_back(callback);
}

void ComboSystem::onComboBreak(ComboCallback callback) {
    m_breakCallbacks.push_back(callback);
}

} // namespace Engine
