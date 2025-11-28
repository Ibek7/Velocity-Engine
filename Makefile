# JJM Game Engine Makefile
# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude
RELEASEFLAGS = -O3 -march=native -DNDEBUG -flto -ffast-math
DEBUGFLAGS = -O0 -g -DDEBUG -fsanitize=address -fno-omit-frame-pointer
LDFLAGS = -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lpthread

# Build mode (default: release)
BUILD_MODE ?= release

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

# Set flags based on build mode
ifeq ($(BUILD_MODE),debug)
	CXXFLAGS += $(DEBUGFLAGS)
	LDFLAGS += -fsanitize=address
else ifeq ($(BUILD_MODE),release)
	CXXFLAGS += $(RELEASEFLAGS)
	LDFLAGS += -flto
endif

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
	@mkdir -p $(BUILD_DIR)/network
	@mkdir -p $(BUILD_DIR)/animation
	@mkdir -p $(BUILD_DIR)/camera
	@mkdir -p $(BUILD_DIR)/events
	@mkdir -p $(BUILD_DIR)/particles
	@mkdir -p $(BUILD_DIR)/state
	@mkdir -p $(BUILD_DIR)/tilemap
	@mkdir -p $(BUILD_DIR)/scripting
	@mkdir -p $(BUILD_DIR)/memory
	@mkdir -p $(BUILD_DIR)/gui
	@mkdir -p $(BUILD_DIR)/threading
	@mkdir -p $(BUILD_DIR)/editor
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

# Build modes
debug:
	@$(MAKE) BUILD_MODE=debug all

release:
	@$(MAKE) BUILD_MODE=release all

profile: CXXFLAGS += -O2 -pg
profile: LDFLAGS += -pg
profile: clean all

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

# Build information
info:
	@echo "Build Configuration:"
	@echo "  Mode: $(BUILD_MODE)"
	@echo "  Compiler: $(CXX)"
	@echo "  CXXFLAGS: $(CXXFLAGS)"
	@echo "  LDFLAGS: $(LDFLAGS)"
	@echo "  Target: $(TARGET)"

# Code statistics
stats:
	@echo "=== JJM Engine Code Statistics ==="
	@echo "Header files:"
	@find $(INCLUDE_DIR) -name "*.h" | wc -l
	@echo "Source files:"
	@find $(SRC_DIR) -name "*.cpp" | wc -l
	@echo "Total lines of code:"
	@find $(SRC_DIR) $(INCLUDE_DIR) -name "*.cpp" -o -name "*.h" | xargs wc -l | tail -1

.PHONY: all directories clean debug release profile run install-deps info stats
