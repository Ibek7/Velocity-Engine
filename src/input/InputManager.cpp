#include "input/InputManager.h"
#include <algorithm>

namespace JJM {
namespace Input {

InputManager::InputManager() 
    : mousePosition(0, 0), mouseMotion(0, 0), mouseWheel(0, 0), quitRequested(false) {}

InputManager::~InputManager() {}

void InputManager::update() {
    // Convert PRESSED to DOWN and RELEASED to UP
    for (auto& pair : keyStates) {
        if (pair.second == KeyState::PRESSED) {
            pair.second = KeyState::DOWN;
        } else if (pair.second == KeyState::RELEASED) {
            pair.second = KeyState::UP;
        }
    }

    for (auto& pair : mouseButtonStates) {
        if (pair.second == KeyState::PRESSED) {
            pair.second = KeyState::DOWN;
        } else if (pair.second == KeyState::RELEASED) {
            pair.second = KeyState::UP;
        }
    }

    // Reset motion and wheel
    mouseMotion = Math::Vector2D(0, 0);
    mouseWheel = Math::Vector2D(0, 0);
}

void InputManager::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_QUIT:
            quitRequested = true;
            break;

        case SDL_KEYDOWN:
            if (!event.key.repeat) {
                updateKeyState(event.key.keysym.sym, true);
            }
            break;

        case SDL_KEYUP:
            updateKeyState(event.key.keysym.sym, false);
            break;

        case SDL_MOUSEBUTTONDOWN:
            updateMouseButtonState(event.button.button, true);
            break;

        case SDL_MOUSEBUTTONUP:
            updateMouseButtonState(event.button.button, false);
            break;

        case SDL_MOUSEMOTION:
            mousePosition.x = static_cast<float>(event.motion.x);
            mousePosition.y = static_cast<float>(event.motion.y);
            mouseMotion.x = static_cast<float>(event.motion.xrel);
            mouseMotion.y = static_cast<float>(event.motion.yrel);
            break;

        case SDL_MOUSEWHEEL:
            mouseWheel.x = static_cast<float>(event.wheel.x);
            mouseWheel.y = static_cast<float>(event.wheel.y);
            break;

        default:
            break;
    }
}

bool InputManager::isKeyDown(SDL_Keycode key) const {
    KeyState state = getKeyState(key);
    return state == KeyState::DOWN || state == KeyState::PRESSED;
}

bool InputManager::isKeyUp(SDL_Keycode key) const {
    KeyState state = getKeyState(key);
    return state == KeyState::UP || state == KeyState::RELEASED;
}

bool InputManager::isKeyPressed(SDL_Keycode key) const {
    return getKeyState(key) == KeyState::PRESSED;
}

bool InputManager::isKeyReleased(SDL_Keycode key) const {
    return getKeyState(key) == KeyState::RELEASED;
}

bool InputManager::isMouseButtonDown(MouseButton button) const {
    KeyState state = getMouseButtonState(static_cast<Uint8>(button));
    return state == KeyState::DOWN || state == KeyState::PRESSED;
}

bool InputManager::isMouseButtonUp(MouseButton button) const {
    KeyState state = getMouseButtonState(static_cast<Uint8>(button));
    return state == KeyState::UP || state == KeyState::RELEASED;
}

bool InputManager::isMouseButtonPressed(MouseButton button) const {
    return getMouseButtonState(static_cast<Uint8>(button)) == KeyState::PRESSED;
}

bool InputManager::isMouseButtonReleased(MouseButton button) const {
    return getMouseButtonState(static_cast<Uint8>(button)) == KeyState::RELEASED;
}

void InputManager::reset() {
    keyStates.clear();
    mouseButtonStates.clear();
    mousePosition = Math::Vector2D(0, 0);
    mouseMotion = Math::Vector2D(0, 0);
    mouseWheel = Math::Vector2D(0, 0);
    quitRequested = false;
}

void InputManager::updateKeyState(SDL_Keycode key, bool down) {
    KeyState currentState = getKeyState(key);
    
    if (down) {
        if (currentState == KeyState::UP || currentState == KeyState::RELEASED) {
            keyStates[key] = KeyState::PRESSED;
        }
    } else {
        if (currentState == KeyState::DOWN || currentState == KeyState::PRESSED) {
            keyStates[key] = KeyState::RELEASED;
        }
    }
}

void InputManager::updateMouseButtonState(Uint8 button, bool down) {
    KeyState currentState = getMouseButtonState(button);
    
    if (down) {
        if (currentState == KeyState::UP || currentState == KeyState::RELEASED) {
            mouseButtonStates[button] = KeyState::PRESSED;
        }
    } else {
        if (currentState == KeyState::DOWN || currentState == KeyState::PRESSED) {
            mouseButtonStates[button] = KeyState::RELEASED;
        }
    }
}

KeyState InputManager::getKeyState(SDL_Keycode key) const {
    auto it = keyStates.find(key);
    return (it != keyStates.end()) ? it->second : KeyState::UP;
}

KeyState InputManager::getMouseButtonState(Uint8 button) const {
    auto it = mouseButtonStates.find(button);
    return (it != mouseButtonStates.end()) ? it->second : KeyState::UP;
}

} // namespace Input
} // namespace JJM
