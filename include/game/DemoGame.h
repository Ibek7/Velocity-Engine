#ifndef DEMO_GAME_H
#define DEMO_GAME_H

#include <memory>

#include "audio/AudioManager.h"
#include "camera/Camera.h"
#include "core/Config.h"
#include "events/EventSystem.h"
#include "graphics/Renderer.h"
#include "input/InputManager.h"
#include "math/Vector2D.h"
#include "particles/ParticleSystem.h"
#include "utils/DebugUtils.h"

class DemoGame {
   private:
    JJM::Graphics::Renderer renderer;
    JJM::Input::InputManager inputManager;
    std::unique_ptr<JJM::Graphics::Camera> camera;
    std::unique_ptr<JJM::Particles::ParticleSystem> particleSystem;
    JJM::Audio::AudioManager* audioManager;
    JJM::Events::EventDispatcher* eventDispatcher;
    JJM::Core::Config* config;
    JJM::Debug::FPSCounter fpsCounter;

    bool running;
    float deltaTime;
    Uint32 lastTime;

    // Demo objects
    JJM::Math::Vector2D playerPos;
    float playerSpeed;

    void processInput(const float dt);
    void updateGameObjects(const float dt);

   public:
    DemoGame();
    ~DemoGame();

    bool initialize();
    void run();

   private:
    void setupEvents();
    void handleEvents();
    void update(float dt);
    void render();
    void calculateDeltaTime();
};

#endif  // DEMO_GAME_H
