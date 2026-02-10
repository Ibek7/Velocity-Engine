#include "game/DemoGame.h"

#include <SDL.h>

#include <iostream>

#include "graphics/Color.h"
#include "utils/DebugUtils.h"

using namespace JJM;

DemoGame::DemoGame()
    : running(false), deltaTime(0), lastTime(0), playerPos(400, 300), playerSpeed(200.0f) {
    audioManager = Audio::AudioManager::getInstance();
    eventDispatcher = Events::EventDispatcher::getInstance();
    config = Core::Config::getInstance();
}

DemoGame::~DemoGame() {
    Audio::AudioManager::destroy();
    Events::EventDispatcher::destroy();
    Core::Config::destroy();
    Debug::Profiler::destroy();
}

bool DemoGame::initialize() {
    // Load configuration
    config->loadFromFile("config.txt");

    // Initialize renderer
    if (!renderer.initialize(config->getWindowTitle(), config->getWindowWidth(),
                             config->getWindowHeight(), config->isFullscreen())) {
        return false;
    }

    // Initialize input
    inputManager.initialize();

    // Initialize audio
    audioManager->initialize();

    // Create camera
    camera = std::make_unique<Graphics::Camera>(
        Math::Vector2D(config->getWindowWidth(), config->getWindowHeight()));

    // Create particle system
    particleSystem = std::make_unique<Particles::ParticleSystem>();

    // Setup event listeners
    setupEvents();

    lastTime = SDL_GetTicks();
    running = true;

    Debug::Logger::info("Demo game initialized successfully");
    return true;
}

void DemoGame::setupEvents() {
    eventDispatcher->addEventListener(
        "player_move", [](const Events::Event& event) { Debug::Logger::debug("Player moved"); });
}

void DemoGame::run() {
    while (running) {
        calculateDeltaTime();
        handleEvents();
        update(deltaTime);
        render();

        fpsCounter.update(deltaTime);
    }
}

void DemoGame::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        }
        inputManager.processEvent(event);
    }
}

void DemoGame::update(float dt) {
    Debug::ScopedProfile profile("Update");

    inputManager.update();
    processInput(dt);
    updateGameObjects(dt);
}

void DemoGame::processInput(const float dt) {
    // Player movement
    Math::Vector2D movement(0, 0);
    if (inputManager.isKeyHeld(SDL_SCANCODE_W) || inputManager.isKeyHeld(SDL_SCANCODE_UP)) {
        movement.y -= 1;
    }
    if (inputManager.isKeyHeld(SDL_SCANCODE_S) || inputManager.isKeyHeld(SDL_SCANCODE_DOWN)) {
        movement.y += 1;
    }
    if (inputManager.isKeyHeld(SDL_SCANCODE_A) || inputManager.isKeyHeld(SDL_SCANCODE_LEFT)) {
        movement.x -= 1;
    }
    if (inputManager.isKeyHeld(SDL_SCANCODE_D) || inputManager.isKeyHeld(SDL_SCANCODE_RIGHT)) {
        movement.x += 1;
    }

    if (movement.magnitudeSquared() > 0) {
        movement.normalize();
        playerPos += movement * playerSpeed * dt;
    }

    // Spawn particles on click
    if (inputManager.isMouseButtonPressed(SDL_BUTTON_LEFT)) {
        auto* emitter = particleSystem->createEmitter(inputManager.getMousePosition(), 50);
        emitter->setLifetime(0.5f, 1.5f);
        emitter->setSpeed(50.0f, 200.0f);
        emitter->setSize(2.0f, 6.0f);
        emitter->setColorRange(Graphics::Color::Red(), Graphics::Color::Yellow());
        emitter->setGravity(Math::Vector2D(0, 100));
        emitter->emitBurst(30);
    }

    // Shake camera on space
    if (inputManager.isKeyPressed(SDL_SCANCODE_SPACE)) {
        camera->shake(10.0f, 0.3f);
    }

    // Quit on escape
    if (inputManager.isKeyPressed(SDL_SCANCODE_ESCAPE)) {
        running = false;
    }
}

void DemoGame::updateGameObjects(const float dt) {
    camera->setPosition(playerPos);
    camera->update(dt);
    particleSystem->update(dt);
}

void DemoGame::render() {
    Debug::ScopedProfile profile("Render");

    renderer.clear(Graphics::Color(20, 20, 30, 255));

    // Draw grid
    Debug::DebugDraw::drawGrid(&renderer, 50, Graphics::Color(40, 40, 50, 255));

    // Draw player
    Math::Vector2D playerSize(30, 30);
    renderer.drawRect(playerPos - playerSize * 0.5f, playerSize, Graphics::Color::Green(), true);

    // Draw particles
    particleSystem->render(&renderer);

    // Draw FPS
    char fpsText[32];
    snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", fpsCounter.getFPS());
    Debug::Logger::setLevel(Debug::Logger::Level::Info);

    // Draw instructions
    Math::Vector2D textPos(10, 10);
    renderer.drawRect(textPos, Math::Vector2D(300, 120), Graphics::Color(0, 0, 0, 180), true);

    renderer.present();
}

void DemoGame::calculateDeltaTime() {
    Uint32 currentTime = SDL_GetTicks();
    deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    // Cap delta time to prevent spiral of death
    if (deltaTime > 0.1f) {
        deltaTime = 0.1f;
    }
}
