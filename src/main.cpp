/**
 * @file main.cpp
 * @brief JJM Game Engine - Demo Application
 * @copyright 2026 JJM Game Engine
 */

#include <iostream>

#include "core/Version.h"
#include "game/DemoGame.h"
#include "utils/DebugUtils.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::cout << "=== JJM Game Engine Demo v" << JJM::Core::getVersionString()
              << " ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  WASD/Arrows - Move player" << std::endl;
    std::cout << "  Left Click - Spawn particles" << std::endl;
    std::cout << "  Space - Camera shake" << std::endl;
    std::cout << "  ESC - Quit" << std::endl;
    std::cout << "===========================\n" << std::endl;

    DemoGame game;

    if (!game.initialize()) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return 1;
    }

    game.run();

    JJM::Debug::Profiler::getInstance()->printResults();

    std::cout << "\nGame exited successfully." << std::endl;
    return 0;
}
