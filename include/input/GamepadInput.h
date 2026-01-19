/**
 * @file GamepadInput.h
 * @brief Gamepad and controller input management system
 * @version 1.0.0
 * @date 2026-01-16
 */

#ifndef GAMEPAD_INPUT_H
#define GAMEPAD_INPUT_H

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <cstdint>

namespace JJM {
namespace Input {

/**
 * @brief Gamepad button enumeration
 */
enum class GamepadButton {
    A,              // Bottom face button (Xbox A, PS Cross)
    B,              // Right face button (Xbox B, PS Circle)
    X,              // Left face button (Xbox X, PS Square)
    Y,              // Top face button (Xbox Y, PS Triangle)
    
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
    
    LeftShoulder,   // L1
    RightShoulder,  // R1
    LeftTrigger,    // L2 (digital)
    RightTrigger,   // R2 (digital)
    
    LeftStick,      // L3 (stick press)
    RightStick,     // R3 (stick press)
    
    Start,
    Back,           // Select
    Guide,          // Home/PS/Xbox button
    
    Count
};

/**
 * @brief Gamepad analog axis enumeration
 */
enum class GamepadAxis {
    LeftX,
    LeftY,
    RightX,
    RightY,
    LeftTrigger,    // L2 (analog)
    RightTrigger,   // R2 (analog)
    
    Count
};

/**
 * @brief Gamepad connection state
 */
struct GamepadState {
    bool connected;
    std::string name;
    int id;
    
    // Button states
    bool buttons[static_cast<int>(GamepadButton::Count)];
    bool prevButtons[static_cast<int>(GamepadButton::Count)];
    
    // Axis values (-1.0 to 1.0)
    float axes[static_cast<int>(GamepadAxis::Count)];
    float prevAxes[static_cast<int>(GamepadAxis::Count)];
    
    // Rumble state
    float leftRumble;
    float rightRumble;
    
    GamepadState() : connected(false), id(-1), leftRumble(0), rightRumble(0) {
        for (int i = 0; i < static_cast<int>(GamepadButton::Count); i++) {
            buttons[i] = prevButtons[i] = false;
        }
        for (int i = 0; i < static_cast<int>(GamepadAxis::Count); i++) {
            axes[i] = prevAxes[i] = 0.0f;
        }
    }
};

/**
 * @brief Action mapping system for input abstraction
 */
class InputAction {
public:
    enum class Type {
        Button,
        Axis,
        Axis2D
    };
    
    InputAction(const std::string& name, Type type)
        : m_name(name), m_type(type), m_value(0.0f), m_prevValue(0.0f) {}
    
    const std::string& getName() const { return m_name; }
    Type getType() const { return m_type; }
    
    // Button-like interface
    bool isPressed() const { return m_value > 0.5f && m_prevValue <= 0.5f; }
    bool isReleased() const { return m_value <= 0.5f && m_prevValue > 0.5f; }
    bool isDown() const { return m_value > 0.5f; }
    
    // Analog interface
    float getValue() const { return m_value; }
    float getDelta() const { return m_value - m_prevValue; }
    
    void setValue(float value) {
        m_prevValue = m_value;
        m_value = value;
    }
    
private:
    std::string m_name;
    Type m_type;
    float m_value;
    float m_prevValue;
};

/**
 * @brief Input binding configuration
 */
struct InputBinding {
    // Gamepad binding
    int gamepadId;              // -1 for any gamepad
    GamepadButton button;
    GamepadAxis axis;
    bool useAxis;
    
    // Modifiers
    float scale;                // Multiplier for analog values
    float deadzone;            // Deadzone for analog inputs
    bool invert;               // Invert axis direction
    
    InputBinding()
        : gamepadId(-1)
        , button(GamepadButton::A)
        , axis(GamepadAxis::LeftX)
        , useAxis(false)
        , scale(1.0f)
        , deadzone(0.15f)
        , invert(false)
    {}
};

/**
 * @brief Deadzone profile presets for different input scenarios
 */
struct DeadzoneProfile {
    float inner;       // Inner deadzone (ignore small movements)
    float outer;       // Outer deadzone (1.0 maps to this value)
    float curvature;   // Response curve exponent (1.0 = linear)
    std::string name;
    
    DeadzoneProfile(float i = 0.15f, float o = 0.95f, float c = 1.0f, const std::string& n = "")
        : inner(i), outer(o), curvature(c), name(n) {}
    
    // Preset profiles
    static DeadzoneProfile Standard() { return DeadzoneProfile(0.15f, 0.95f, 1.0f, "Standard"); }
    static DeadzoneProfile Precise() { return DeadzoneProfile(0.05f, 0.98f, 1.2f, "Precise"); }
    static DeadzoneProfile Smooth() { return DeadzoneProfile(0.20f, 0.90f, 0.8f, "Smooth"); }
    static DeadzoneProfile Aggressive() { return DeadzoneProfile(0.10f, 0.95f, 1.5f, "Aggressive"); }
    static DeadzoneProfile Racing() { return DeadzoneProfile(0.05f, 0.99f, 2.0f, "Racing"); }
    static DeadzoneProfile Platformer() { return DeadzoneProfile(0.25f, 0.92f, 1.0f, "Platformer"); }
    static DeadzoneProfile Shooter() { return DeadzoneProfile(0.12f, 0.96f, 1.3f, "Shooter"); }
    static DeadzoneProfile Fighting() { return DeadzoneProfile(0.30f, 0.90f, 1.0f, "Fighting"); }
    
    // Apply deadzone profile to raw input value
    float apply(float value) const {
        float absValue = std::abs(value);
        
        // Apply inner deadzone
        if (absValue < inner) return 0.0f;
        
        // Apply outer deadzone
        if (absValue > outer) absValue = outer;
        
        // Normalize to 0-1 range
        float normalized = (absValue - inner) / (outer - inner);
        
        // Apply response curve
        float curved = std::pow(normalized, curvature);
        
        // Restore sign
        return (value < 0.0f) ? -curved : curved;
    }
};

/**
 * @brief Gamepad input manager
 */
class GamepadManager {
public:
    static GamepadManager& getInstance();
    
    GamepadManager(const GamepadManager&) = delete;
    GamepadManager& operator=(const GamepadManager&) = delete;
    
    /**
     * @brief Initialize gamepad system
     */
    bool initialize();
    
    /**
     * @brief Shutdown gamepad system
     */
    void shutdown();
    
    /**
     * @brief Update gamepad states (call once per frame)
     */
    void update();
    
    // =========================================================================
    // Gamepad State Queries
    // =========================================================================
    
    /**
     * @brief Check if gamepad is connected
     */
    bool isGamepadConnected(int gamepadId) const;
    
    /**
     * @brief Get number of connected gamepads
     */
    int getConnectedGamepadCount() const;
    
    /**
     * @brief Get gamepad state
     */
    const GamepadState* getGamepadState(int gamepadId) const;
    
    // =========================================================================
    // Button Input
    // =========================================================================
    
    /**
     * @brief Check if button is pressed this frame
     */
    bool isButtonPressed(int gamepadId, GamepadButton button) const;
    
    /**
     * @brief Check if button is released this frame
     */
    bool isButtonReleased(int gamepadId, GamepadButton button) const;
    
    /**
     * @brief Check if button is currently held down
     */
    bool isButtonDown(int gamepadId, GamepadButton button) const;
    
    // =========================================================================
    // Analog Input
    // =========================================================================
    
    /**
     * @brief Get analog axis value (-1.0 to 1.0)
     * @param applyDeadzone Apply deadzone filtering
     */
    float getAxis(int gamepadId, GamepadAxis axis, bool applyDeadzone = true) const;
    
    /**
     * @brief Get 2D stick input as vector
     */
    void getStick(int gamepadId, bool leftStick, float& x, float& y, bool applyDeadzone = true) const;
    
    /**
     * @brief Set deadzone for analog inputs (0.0 to 1.0)
     */
    void setDeadzone(float deadzone);
    
    /**
     * @brief Get current deadzone value
     */
    float getDeadzone() const { return m_deadzone; }
    
    /**
     * @brief Set deadzone profile for specific gamepad
     */
    void setDeadzoneProfile(int gamepadId, const DeadzoneProfile& profile);
    
    /**
     * @brief Get deadzone profile for specific gamepad
     */
    const DeadzoneProfile& getDeadzoneProfile(int gamepadId) const;
    
    /**
     * @brief Set default deadzone profile for all new gamepads
     */
    void setDefaultDeadzoneProfile(const DeadzoneProfile& profile);
    
    /**
     * @brief Apply deadzone profile to axis value
     */
    float applyDeadzoneProfile(int gamepadId, float value) const;
    
    // =========================================================================
    // Rumble/Haptics
    // =========================================================================
    
    /**
     * @brief Set rumble/vibration for gamepad
     * @param leftMotor Low frequency motor strength (0.0 to 1.0)
     * @param rightMotor High frequency motor strength (0.0 to 1.0)
     * @param durationMs Duration in milliseconds (0 for infinite)
     */
    void setRumble(int gamepadId, float leftMotor, float rightMotor, uint32_t durationMs = 0);
    
    /**
     * @brief Stop rumble for gamepad
     */
    void stopRumble(int gamepadId);
    
    // =========================================================================
    // Action Mapping System
    // =========================================================================
    
    /**
     * @brief Register an input action
     */
    void registerAction(const std::string& actionName, InputAction::Type type);
    
    /**
     * @brief Bind gamepad button to action
     */
    void bindButton(const std::string& actionName, int gamepadId, GamepadButton button);
    
    /**
     * @brief Bind gamepad axis to action
     */
    void bindAxis(const std::string& actionName, int gamepadId, GamepadAxis axis, 
                  float scale = 1.0f, bool invert = false);
    
    /**
     * @brief Remove all bindings for an action
     */
    void unbindAction(const std::string& actionName);
    
    /**
     * @brief Get action state
     */
    const InputAction* getAction(const std::string& actionName) const;
    
    /**
     * @brief Check if action is triggered (button pressed)
     */
    bool isActionPressed(const std::string& actionName) const;
    
    /**
     * @brief Check if action is active (button held)
     */
    bool isActionDown(const std::string& actionName) const;
    
    /**
     * @brief Get action analog value
     */
    float getActionValue(const std::string& actionName) const;
    
    /**
     * @brief Load action mappings from file
     */
    bool loadActionMappings(const std::string& filePath);
    
    /**
     * @brief Save action mappings to file
     */
    bool saveActionMappings(const std::string& filePath) const;
    
    // =========================================================================
    // Callbacks
    // =========================================================================
    
    using GamepadConnectedCallback = std::function<void(int gamepadId)>;
    using GamepadDisconnectedCallback = std::function<void(int gamepadId)>;
    using ButtonCallback = std::function<void(int gamepadId, GamepadButton button)>;
    
    void setGamepadConnectedCallback(GamepadConnectedCallback callback) {
        m_onGamepadConnected = callback;
    }
    
    void setGamepadDisconnectedCallback(GamepadDisconnectedCallback callback) {
        m_onGamepadDisconnected = callback;
    }
    
    void setButtonPressedCallback(ButtonCallback callback) {
        m_onButtonPressed = callback;
    }
    
private:
    GamepadManager();
    ~GamepadManager();
    
    // Gamepad states (indexed by gamepad ID)
    std::unordered_map<int, GamepadState> m_gamepads;
    
    // Deadzone profiles per gamepad
    std::unordered_map<int, DeadzoneProfile> m_deadzoneProfiles;
    DeadzoneProfile m_defaultDeadzoneProfile;
    
    // Action mapping system
    std::unordered_map<std::string, InputAction> m_actions;
    std::unordered_map<std::string, std::vector<InputBinding>> m_bindings;
    
    // Settings
    float m_deadzone;
    
    // Callbacks
    GamepadConnectedCallback m_onGamepadConnected;
    GamepadDisconnectedCallback m_onGamepadDisconnected;
    ButtonCallback m_onButtonPressed;
    
    // Helper functions
    float applyDeadzone(float value, float deadzone) const;
    void updateActions();
    void checkGamepadConnections();
};

} // namespace Input
} // namespace JJM

#endif // GAMEPAD_INPUT_H
