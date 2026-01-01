#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "math/Vector2D.h"
#include <SDL.h>
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>

namespace JJM {
namespace Input {

enum class KeyState {
    UP,
    DOWN,
    PRESSED,
    RELEASED
};

enum class MouseButton {
    LEFT = SDL_BUTTON_LEFT,
    MIDDLE = SDL_BUTTON_MIDDLE,
    RIGHT = SDL_BUTTON_RIGHT
};

// Gamepad button mapping
enum class GamepadButton {
    A,
    B,
    X,
    Y,
    LeftBumper,
    RightBumper,
    Back,
    Start,
    Guide,
    LeftStick,
    RightStick,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
    Count
};

// Gamepad axis mapping  
enum class GamepadAxis {
    LeftX,
    LeftY,
    RightX,
    RightY,
    LeftTrigger,
    RightTrigger,
    Count
};

// Gamepad state
struct GamepadState {
    bool connected;
    std::string name;
    SDL_GameController* controller;
    std::unordered_map<GamepadButton, KeyState> buttons;
    std::unordered_map<GamepadAxis, float> axes;
    float deadzone;
    int playerIndex;
    
    GamepadState()
        : connected(false)
        , controller(nullptr)
        , deadzone(0.15f)
        , playerIndex(-1)
    {}
};

// Input action binding
struct InputAction {
    std::string name;
    std::vector<SDL_Keycode> keyBindings;
    std::vector<MouseButton> mouseBindings;
    std::vector<std::pair<int, GamepadButton>> gamepadBindings;
};

// Callbacks
using InputCallback = std::function<void()>;
using AxisCallback = std::function<void(float)>;

class InputManager {
private:
    std::unordered_map<SDL_Keycode, KeyState> keyStates;
    std::unordered_map<Uint8, KeyState> mouseButtonStates;
    Math::Vector2D mousePosition;
    Math::Vector2D mouseMotion;
    Math::Vector2D mouseWheel;
    bool quitRequested;
    
    // Gamepad support
    std::vector<GamepadState> gamepads;
    static constexpr int MAX_GAMEPADS = 4;
    
    // Input action system
    std::unordered_map<std::string, InputAction> actions;
    std::unordered_map<std::string, std::vector<InputCallback>> actionPressedCallbacks;
    std::unordered_map<std::string, std::vector<InputCallback>> actionReleasedCallbacks;
    
    // Text input
    bool textInputActive;
    std::string textInputBuffer;
    std::function<void(const std::string&)> onTextInput;

public:
    InputManager();
    ~InputManager();

    // Update input state
    void update();
    void handleEvent(const SDL_Event& event);

    // Keyboard input
    bool isKeyDown(SDL_Keycode key) const;
    bool isKeyUp(SDL_Keycode key) const;
    bool isKeyPressed(SDL_Keycode key) const;
    bool isKeyReleased(SDL_Keycode key) const;

    // Mouse input
    bool isMouseButtonDown(MouseButton button) const;
    bool isMouseButtonUp(MouseButton button) const;
    bool isMouseButtonPressed(MouseButton button) const;
    bool isMouseButtonReleased(MouseButton button) const;

    // Mouse position and motion
    Math::Vector2D getMousePosition() const { return mousePosition; }
    Math::Vector2D getMouseMotion() const { return mouseMotion; }
    Math::Vector2D getMouseWheel() const { return mouseWheel; }
    void setMousePosition(int x, int y);
    void setMouseVisible(bool visible);
    void setMouseRelativeMode(bool enabled);
    
    // Gamepad support
    void initializeGamepads();
    void shutdownGamepads();
    bool isGamepadConnected(int playerIndex) const;
    int getConnectedGamepadCount() const;
    std::string getGamepadName(int playerIndex) const;
    
    // Gamepad buttons
    bool isGamepadButtonDown(int playerIndex, GamepadButton button) const;
    bool isGamepadButtonPressed(int playerIndex, GamepadButton button) const;
    bool isGamepadButtonReleased(int playerIndex, GamepadButton button) const;
    
    // Gamepad axes
    float getGamepadAxis(int playerIndex, GamepadAxis axis) const;
    Math::Vector2D getGamepadLeftStick(int playerIndex) const;
    Math::Vector2D getGamepadRightStick(int playerIndex) const;
    float getGamepadLeftTrigger(int playerIndex) const;
    float getGamepadRightTrigger(int playerIndex) const;
    void setGamepadDeadzone(int playerIndex, float deadzone);
    
    // Gamepad rumble/haptics
    void setGamepadRumble(int playerIndex, float lowFreq, float highFreq, float duration);
    void stopGamepadRumble(int playerIndex);
    
    // Input actions (binding system)
    void registerAction(const std::string& name);
    void bindKeyToAction(const std::string& action, SDL_Keycode key);
    void bindMouseToAction(const std::string& action, MouseButton button);
    void bindGamepadToAction(const std::string& action, int playerIndex, GamepadButton button);
    void unbindAction(const std::string& action);
    
    bool isActionPressed(const std::string& action) const;
    bool isActionDown(const std::string& action) const;
    bool isActionReleased(const std::string& action) const;
    
    void onActionPressed(const std::string& action, InputCallback callback);
    void onActionReleased(const std::string& action, InputCallback callback);
    
    // Text input
    void startTextInput();
    void stopTextInput();
    bool isTextInputActive() const { return textInputActive; }
    const std::string& getTextInputBuffer() const { return textInputBuffer; }
    void clearTextInputBuffer() { textInputBuffer.clear(); }
    void setTextInputCallback(std::function<void(const std::string&)> callback);

    // Utility
    bool shouldQuit() const { return quitRequested; }
    void reset();
    bool anyKeyPressed() const;
    bool anyGamepadButtonPressed(int playerIndex) const;

private:
    void updateKeyState(SDL_Keycode key, bool down);
    void updateMouseButtonState(Uint8 button, bool down);
    void updateGamepadButtonState(int index, GamepadButton button, bool down);
    KeyState getKeyState(SDL_Keycode key) const;
    KeyState getMouseButtonState(Uint8 button) const;
    void handleGamepadEvent(const SDL_Event& event);
    float applyDeadzone(float value, float deadzone) const;
    void triggerActionCallbacks(const std::string& action, bool pressed);
};

} // namespace Input
} // namespace JJM

#endif // INPUT_MANAGER_H
