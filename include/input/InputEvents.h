#ifndef INPUT_EVENTS_H
#define INPUT_EVENTS_H

#include <SDL.h>

#include <cstdint>

namespace JJM {
namespace Input {

enum class InputEventType {
    KeyPressed,
    KeyReleased,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseWheel,
    TextInput
};

struct InputEvent {
    InputEventType type;

    // Keyboard
    SDL_Keycode key;

    // Mouse
    uint8_t mouseButton;
    float mouseX;
    float mouseY;
    float wheelX;
    float wheelY;

    // Text
    char text[32];  // Fixed size for text input

    InputEvent()
        : type(InputEventType::KeyPressed),
          key(0),
          mouseButton(0),
          mouseX(0),
          mouseY(0),
          wheelX(0),
          wheelY(0) {
        text[0] = '\0';
    }
};

}  // namespace Input
}  // namespace JJM

#endif  // INPUT_EVENTS_H
