#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "math/Vector2D.h"
#include <SDL.h>
#include <unordered_map>
#include <string>

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

class InputManager {
private:
    std::unordered_map<SDL_Keycode, KeyState> keyStates;
    std::unordered_map<Uint8, KeyState> mouseButtonStates;
    Math::Vector2D mousePosition;
    Math::Vector2D mouseMotion;
    Math::Vector2D mouseWheel;
    bool quitRequested;

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

    // Utility
    bool shouldQuit() const { return quitRequested; }
    void reset();

private:
    void updateKeyState(SDL_Keycode key, bool down);
    void updateMouseButtonState(Uint8 button, bool down);
    KeyState getKeyState(SDL_Keycode key) const;
    KeyState getMouseButtonState(Uint8 button) const;
};

} // namespace Input
} // namespace JJM

#endif // INPUT_MANAGER_H
