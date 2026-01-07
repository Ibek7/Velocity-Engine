# JJM Game Engine

A lightweight, modular 2D game engine built with C++ and SDL2, designed for creating games and interactive applications with high performance and flexibility.

## Features

### Core Systems
- **Math Library**: Complete 2D vector and matrix operations with transformation support
- **Rendering System**: Hardware-accelerated 2D rendering with SDL2
- **Entity Component System (ECS)**: Flexible architecture for game objects
- **Physics Engine**: 2D physics with collision detection and response
- **Input Management**: Keyboard and mouse input handling with event system
- **Resource Management**: Efficient loading and management of textures and assets

### Advanced Features
- **Scene Management**: Easy scene switching and state management
- **Audio System**: Music playback and sound effects with SDL_mixer
- **Particle System**: Customizable particle emitters for visual effects
- **Animation System**: Sprite animation and tweening with 20+ easing functions
- **UI Framework**: Buttons, labels, panels, sliders, and checkboxes
- **Camera System**: 2D camera with follow, zoom, rotation, shake, and bounds
- **Tilemap Support**: Tile-based level design and rendering
- **Event System**: Global event dispatcher for decoupled communication
- **Debug Tools**: Profiler, logger, FPS counter, and debug rendering
- **State Machine**: Game state management (Menu, Game, Pause, GameOver)
- **Configuration**: File-based configuration system

## Requirements

- C++17 or later
- SDL2
- SDL2_image
- SDL2_mixer
- SDL2_ttf
- Make

## Installation

### macOS
```bash
brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf
```

### Ubuntu/Debian
```bash
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
```

## Building

```bash
# Build the project
make

# Build with debug symbols
make debug

# Clean build artifacts
make clean

# Build and run
make run
```

## Project Structure

```
JJM/
├── include/              # Header files
│   ├── animation/        # Animation and tweening
│   ├── audio/            # Audio management
│   ├── camera/           # Camera system
│   ├── core/             # Core engine components
│   ├── ecs/              # Entity Component System
│   ├── events/           # Event system
│   ├── graphics/         # Rendering and graphics
│   ├── input/            # Input handling
│   ├── math/             # Math utilities
│   ├── particles/        # Particle effects
│   ├── physics/          # Physics engine
│   ├── scene/            # Scene management
│   ├── state/            # State machine
│   ├── tilemap/          # Tilemap system
│   ├── ui/               # UI components
│   └── utils/            # Debug and utilities
├── src/                  # Source files
│   └── ...               # (mirrors include structure)
├── build/                # Build artifacts (generated)
├── bin/                  # Compiled binaries (generated)
├── Makefile              # Build configuration
└── config.txt            # Engine configuration
```

## Quick Start

### Basic Example

```cpp
#include "graphics/Renderer.h"
#include "input/InputManager.h"
#include "math/Vector2D.h"

int main() {
    JJM::Graphics::Renderer renderer;
    JJM::Input::InputManager input;
    
    renderer.initialize("My Game", 800, 600);
    input.initialize();
    
    bool running = true;
    while (running) {
        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            input.processEvent(event);
        }
        
        input.update();
        
        // Update game logic
        if (input.isKeyPressed(SDL_SCANCODE_ESCAPE)) {
            running = false;
        }
        
        // Render
        renderer.clear();
        renderer.drawRect(
            JJM::Math::Vector2D(100, 100),
            JJM::Math::Vector2D(50, 50),
            JJM::Graphics::Color::Red(),
            true
        );
        renderer.present();
    }
    
    return 0;
}
```

### Using Particles

```cpp
auto particleSystem = std::make_unique<JJM::Particles::ParticleSystem>();
auto* emitter = particleSystem->createEmitter(JJM::Math::Vector2D(400, 300), 100);

emitter->setLifetime(0.5f, 1.5f);
emitter->setSpeed(50.0f, 200.0f);
emitter->setColorRange(
    JJM::Graphics::Color::Red(),
    JJM::Graphics::Color::Yellow()
);
emitter->start();

// In game loop
particleSystem->update(deltaTime);
particleSystem->render(&renderer);
```

### Camera Usage

```cpp
auto camera = std::make_unique<JJM::Graphics::Camera>(
    JJM::Math::Vector2D(800, 600)
);

camera->setPosition(playerPosition);
camera->setZoom(1.5f);
camera->shake(10.0f, 0.3f);  // Intensity, duration

// In game loop
camera->update(deltaTime);
```

### Animation System

```cpp
JJM::Animation::AnimationClip walkClip("walk", true);
walkClip.addFrame(0, 0, 32, 32, 0.1f);
walkClip.addFrame(32, 0, 32, 32, 0.1f);
walkClip.addFrame(64, 0, 32, 32, 0.1f);

JJM::Animation::Animator animator;
animator.addClip(walkClip);
animator.play("walk");

// In game loop
animator.update(deltaTime);
const auto* frame = animator.getCurrentFrame();
```

### Event System

```cpp
auto* dispatcher = JJM::Events::EventDispatcher::getInstance();

// Register listener
dispatcher->addEventListener("player_death", [](const JJM::Events::Event& e) {
    std::cout << "Player died!" << std::endl;
});

// Dispatch event
JJM::Events::Event event("player_death");
event.setData("score", 1000);
dispatcher->dispatchEvent(event);
```

## Demo Game

The engine includes a demo game (`src/main.cpp`) that showcases:
- Player movement with WASD/Arrow keys
- Particle effects on mouse click
- Camera shake on spacebar
- FPS counter and debug rendering

Run the demo:
```bash
make run
```

## Configuration

Edit `config.txt` to customize engine settings:

```ini
window_width=1280
window_height=720
fullscreen=false
window_title=My Game
target_fps=60
vsync=true
audio_enabled=true
music_volume=80
sfx_volume=100
```

## Architecture

### Math System
- `Vector2D`: 2D vector operations, dot product, cross product, normalization
- `Matrix3x3`: 3x3 matrices for 2D transformations, rotation, scale, translation

### Graphics
- `Renderer`: SDL2-based rendering with primitives and texture support
- `Texture`: Texture loading and rendering
- `Color`: RGBA color representation
- `Camera`: Viewport management, follow targets, shake effects

### Physics
- `PhysicsBody`: Rigid body simulation with mass, velocity, forces
- `Collider`: AABB and circle colliders
- `PhysicsWorld`: Physics simulation and collision detection

### Audio
- `AudioManager`: Singleton for music and sound effect playback
- Supports multiple audio formats via SDL_mixer

### UI
- `Button`: Interactive buttons with callbacks
- `Label`: Text display
- `Panel`: Container with background
- `Slider`: Value selection
- `Checkbox`: Toggle controls

## Performance

- Hardware-accelerated rendering
- Efficient particle systems with object pooling
- Profiling tools for performance analysis
- Configurable frame rate targeting

## Debug Tools

```cpp
// Profiling
JJM::Debug::ScopedProfile profile("UpdatePhysics");

// Logging
JJM::Debug::Logger::info("Game started");
JJM::Debug::Logger::warning("Low health");
JJM::Debug::Logger::error("Failed to load texture");

// Debug drawing
JJM::Debug::DebugDraw::drawGrid(&renderer, 50, color);
JJM::Debug::DebugDraw::drawCircle(&renderer, position, radius, color);
```

## Contributing

This is a personal project, but suggestions and feedback are welcome!

## License

MIT License - See LICENSE file for details

## Acknowledgments

- SDL2 - Simple DirectMedia Layer
- Inspired by Unity, Godot, and other game engines
- Built as a learning project to understand game engine architecture

## Author

Built with ❤️ for game development

## Version

Current Version: 1.0.0
Lines of Code: ~5000+
Commits: 20 meaningful feature commits

---

**Happy Game Development! 🎮**
