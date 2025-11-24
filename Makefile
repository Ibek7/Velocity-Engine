# JJM Game Engine Makefile
# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -O2
DEBUGFLAGS = -g -DDEBUG
LDFLAGS = -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lpthread

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Source files
SOURCES = $(wildcard $(SRC_DIR)/**/*.cpp $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEPS = $(OBJECTS:.o=.d)

# Target executable
TARGET = $(BIN_DIR)/game_engine

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/core
	@mkdir -p $(BUILD_DIR)/math
	@mkdir -p $(BUILD_DIR)/graphics
	@mkdir -p $(BUILD_DIR)/physics
	@mkdir -p $(BUILD_DIR)/audio
	@mkdir -p $(BUILD_DIR)/ecs
	@mkdir -p $(BUILD_DIR)/input
	@mkdir -p $(BUILD_DIR)/scene
	@mkdir -p $(BUILD_DIR)/ui
	@mkdir -p $(BUILD_DIR)/utils
	@mkdir -p $(BIN_DIR)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Include dependency files
-include $(DEPS)

# Debug build
debug: CXXFLAGS += $(DEBUGFLAGS)
debug: clean all

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Run the application
run: all
	./$(TARGET)

# Install dependencies (for documentation)
install-deps:
	@echo "Please install SDL2, SDL2_image, SDL2_mixer, SDL2_ttf"
	@echo "On macOS: brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf"
	@echo "On Ubuntu: sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev"

.PHONY: all directories clean debug run install-deps
