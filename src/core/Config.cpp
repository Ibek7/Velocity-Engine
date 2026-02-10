#include "core/Config.h"

#include <iostream>

namespace JJM {
namespace Core {

Config* Config::instance = nullptr;

Config::Config() {
    // Set default values
    set("window_width", 800);
    set("window_height", 600);
    set("fullscreen", false);
    set("window_title", std::string("JJM Game Engine"));
    set("target_fps", 60);
    set("vsync", true);
    set("audio_enabled", true);
    set("music_volume", 100);
    set("sfx_volume", 100);
}

Config::~Config() {}

Config* Config::getInstance() {
    if (!instance) {
        instance = new Config();
    }
    return instance;
}

void Config::destroy() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        // Parse key=value
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
#include "utils/StringUtils.h"

            // ...

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Trim whitespace
            key = Utils::StringUtils::trim(key);
            value = Utils::StringUtils::trim(value);

            // Try to parse as different types
            if (value == "true" || value == "false") {
                set(key, value == "true");
            } else if (value.find('.') != std::string::npos) {
                try {
                    set(key, std::stof(value));
                } catch (...) {
                    set(key, value);
                }
            } else {
                try {
                    set(key, std::stoi(value));
                } catch (...) {
                    set(key, value);
                }
            }
        }
    }

    file.close();
    return true;
}

bool Config::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file for writing: " << filename << std::endl;
        return false;
    }

    file << "# JJM Game Engine Configuration File\n\n";

    // Write common values
    file << "window_width=" << getWindowWidth() << "\n";
    file << "window_height=" << getWindowHeight() << "\n";
    file << "fullscreen=" << (isFullscreen() ? "true" : "false") << "\n";
    file << "window_title=" << getWindowTitle() << "\n";
    file << "target_fps=" << getTargetFPS() << "\n";

    file.close();
    return true;
}

bool Config::has(const std::string& key) const { return values.find(key) != values.end(); }

void Config::remove(const std::string& key) { values.erase(key); }

void Config::clear() { values.clear(); }

// Common configuration getters
int Config::getWindowWidth() const { return get<int>("window_width", 800); }

int Config::getWindowHeight() const { return get<int>("window_height", 600); }

bool Config::isFullscreen() const { return get<bool>("fullscreen", false); }

std::string Config::getWindowTitle() const {
    return get<std::string>("window_title", "JJM Game Engine");
}

int Config::getTargetFPS() const { return get<int>("target_fps", 60); }

// Common configuration setters
void Config::setWindowWidth(int width) { set("window_width", width); }

void Config::setWindowHeight(int height) { set("window_height", height); }

void Config::setFullscreen(bool fullscreen) { set("fullscreen", fullscreen); }

void Config::setWindowTitle(const std::string& title) { set("window_title", title); }

void Config::setTargetFPS(int fps) { set("target_fps", fps); }

}  // namespace Core
}  // namespace JJM
