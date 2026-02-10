# Changelog

All notable changes to the JJM Game Engine will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

#### January 25, 2026 - Weather, Cutscenes, and Gameplay Systems
- **Weather System**:
  - Dynamic weather conditions (clear, cloudy, fog, rain, snow, thunderstorm, sandstorm)
  - Wind simulation with gusts and directional control
  - Precipitation system with intensity, particle size, and fall speed
  - Atmospheric effects (fog density, lighting multipliers, visibility distance)
  - Smooth transitions between weather states
  - Thunder strikes with random positioning for storm weather
  - 8 built-in presets with customizable parameters
  - Random weather generation with configurable durations
  - Weather change callbacks and event system
  - Statistics tracking (time in each weather type, weather changes, thunder strikes)

- **Cutscene System**:
  - Event-based cutscene sequencing with precise timing
  - 12 event types (camera moves, animations, audio, subtitles, fades, entity control)
  - Camera movement with interpolation types (linear, smooth, bezier, spline, follow)
  - Subtitle display with speaker names and duration control
  - Playback control (play, pause, resume, stop, skip, seek)
  - Blocking events for sequential execution
  - UI control (hide UI, restore player control)
  - Fade in/out at start and end
  - Event callbacks for custom integration
  - Progress tracking and state management
  - Helper methods for building cutscene events

- **Achievement Tracking System**:
  - Multi-category achievements (story, combat, exploration, collection, skill, social, special)
  - 6 condition types (single event, cumulative, threshold, collection, combo, time-limited)
  - Progress tracking with percentage calculation
  - Achievement rewards (experience, currency, items, titles)
  - Secret and rare achievement flags
  - Player statistics (total unlocked, points earned, completion percentage)
  - Category-based filtering and progress tracking
  - Recently unlocked achievements list
  - Event-based progress system with callbacks
  - Achievement unlock notifications
  - Save/load support for persistence

- **Crowd Simulation System** (AI):
  - Large-scale NPC crowd behavior with flocking algorithms
  - Separation, alignment, and cohesion forces
  - Local collision avoidance
  - Path following for guided movement
  - 5 formation types (line, column, wedge, circle, grid)
  - Spatial partitioning with grid system for performance
  - Per-agent properties (max speed, force weights, perception radius)
  - Neighbor detection and tracking
  - Formation position assignment and maintenance

- **Footstep System** (Audio):
  - Dynamic footstep audio based on 11 surface materials
  - Surface types: concrete, wood, metal, grass, gravel, water, snow, mud, sand, carpet, tile
  - 5 intensity levels: walk, run, crouch, jump, slide
  - Material-based sound configuration with pitch and volume modulation
  - Distance attenuation for spatial audio
  - Random variation in pitch and volume per step
  - Status callbacks for footstep events
  - Entity-based tracking

- **Damage System** (Gameplay):
  - 12 damage types (physical, fire, ice, lightning, poison, etc.)
  - Armor and resistance calculations with percentages
  - Critical hit system with multipliers
  - Blocking and dodging mechanics
  - 10 status effects (burning, frozen, poisoned, stunned, etc.)
  - Damage over time (DOT) processing
  - Area damage with falloff
  - Damage statistics tracking per entity
  - Event callbacks for damage dealt/received
  - Immunity and invulnerability support

- **Tutorial System** (UI):
  - Multi-step tutorial sequences
  - 6 step types (message, highlight, wait for action, wait for input, cinematic, checkpoint)
  - Context-sensitive hints system
  - Progress tracking with skippable tutorials
  - Tutorial completion callbacks
  - Active tutorial management
  - Tutorial state persistence
  - Hint triggering based on conditions

- **Faction System** (Gameplay):
  - Faction reputation tracking (-1000 to +1000)
  - 8 reputation levels (revered, exalted, honored, friendly, neutral, unfriendly, hostile, hated)
  - 6 relationship types between factions (allied, friendly, neutral, unfriendly, hostile, enemy)
  - War and peace state management
  - Faction membership with ranks
  - Entity-faction assignment
  - Reputation change propagation to allied/enemy factions
  - Reputation gain/loss callbacks
  - Query system for faction relationships

- **Time of Day System** (Graphics):
  - 8 time periods (night, dawn, morning, noon, afternoon, dusk, evening, late night)
  - Configurable day length with time progression
  - Sun and moon positioning calculations
  - Sky color interpolation between time periods
  - Lighting configuration per time period
  - Ambient, sun, and moon intensity control
  - Smooth transitions between periods
  - Sunrise and sunset callbacks
  - Period change notifications
  - 8 built-in time presets

#### January 24, 2026 - Advanced Systems and Editor Tools
- **Shader Graph System**:
  - Node-based visual shader editor with material graphs
  - 50+ node types (math, texture sampling, PBR, noise, logic)
  - Visual connection system for intuitive shader creation
  - Automatic GLSL code generation from node graph
  - Material templates (PBR, unlit, terrain)
  - Hot reload and preview support
  - Undo/redo functionality for editor operations
  - Export to reusable material instances

- **Resource Prefetching System**:
  - Intelligent predictive resource loading with multiple strategies
  - Spatial prefetching based on player position and velocity
  - Sequential prefetching for level progression
  - Pattern-based learning from player behavior
  - Dependency tracking and automatic loading
  - Priority queue system with multiple load levels
  - Memory management with LRU cache eviction
  - Async loading with worker threads
  - Performance statistics and cache hit rate tracking

- **AI Vision System**:
  - Field of view (FOV) calculations with peripheral vision
  - Occlusion testing with raycast detection
  - Visual memory for last known positions
  - Multiple vision senses (sight, night vision, thermal, motion detection)
  - Distance-based visibility scoring
  - Lighting and fog effects on vision
  - Motion sensitivity and size-based detection
  - Vision cone debug visualization
  - Spatial partitioning for performance optimization
  - Observer queries for stealth gameplay

- **Reflection System**:
  - Runtime type information (RTTI) for C++ classes
  - Field metadata with serialization flags
  - Dynamic property access and modification
  - Class hierarchy with base class tracking
  - Constructor registration for factory patterns
  - Macro-based reflection registration
  - Type queries and introspection

- **Trigger Volume System**:
  - Spatial event detection with multiple shapes (box, sphere, capsule)
  - On-enter, on-exit, and on-stay callbacks
  - Configurable trigger bounds and positioning
  - Entity tracking within trigger volumes
  - Enable/disable functionality
  - Performance-optimized collision tests

- **Visual Scripting System**:
  - Blueprint-style node-based scripting
  - Execution flow with exec pins
  - Multiple data types (float, int, bool, string, object, vector)
  - Node connections with type validation
  - Script compilation to executable form
  - Runtime script execution
  - Script registration and management system

- **Morph Target System**:
  - Blend shape animation support
  - Multiple morph targets per mesh
  - Weight-based blending (0-1 range)
  - Position and normal delta storage
  - Real-time mesh deformation
  - Controller management for multiple meshes
  - Integration with animation system

- **Lightmap Baking System**:
  - Precomputed lighting with ambient occlusion
  - Global illumination with multiple bounces
  - Configurable resolution and sample counts
  - Ray tracing for accurate light calculations
  - Progress tracking for long bakes
  - Cancellation support for iterative workflows
  - Texture output for runtime use

- **Spawn System**:
  - Object pooling for efficient spawning
  - Named spawn points with tags
  - Random spawn point selection
  - Wave-based spawning with intervals
  - Prefab instantiation support
  - Pool prewarming for performance
  - Active object tracking
  - Despawn and recycling functionality

#### January 23, 2026 - Major Feature Additions Part 2
- **Particle Physics System**:
  - Comprehensive particle collision detection and response
  - World geometry collisions (planes, spheres, boxes)
  - Particle-particle collision detection with spatial partitioning
  - Distance constraints for soft-body simulation
  - Force fields (point, directional, vortex, turbulence)
  - Collision layers and group filtering for selective interactions

- **Input System**:
  - Multi-gesture recognition for touch and mouse input
  - Tap, double-tap, and multi-tap detection
  - Long press with configurable duration thresholds
  - Swipe gestures with direction and velocity tracking
  - Pinch/zoom with scale tracking
  - Rotation gesture with angle calculation
  - Pan gesture for dragging operations
  - Mouse-to-touch simulation for desktop compatibility
  - Configurable thresholds and parameters for all gestures

- **Utilities**:
  - Automatic LOD (Level of Detail) mesh generation
  - Multiple decimation algorithms (edge collapse, quadric error, vertex clustering)
  - Quadric error metrics for high-quality simplification
  - Feature preservation (boundaries, sharp edges, UV seams)
  - Vertex cache optimization
  - LODManager for runtime LOD selection and distance-based switching

- **Graphics Pipeline**:
  - Comprehensive post-processing effects system
  - Bloom with configurable threshold and intensity
  - Tone mapping (Reinhard, ACES, Hable operators)
  - Color grading (temperature, tint, lift-gamma-gain)
  - Vignette, chromatic aberration, depth of field
  - Motion blur with velocity buffers
  - Screen-space ambient occlusion (SSAO)
  - Anti-aliasing (FXAA, SMAA, TAA)
  - Film grain effect
  - Effect ordering and chaining
  - Preset configurations (realistic, stylized, cinematic, retro)

- **Scripting System**:
  - Multi-language support (Lua, Python, JavaScript)
  - Script context management and isolation
  - Native function binding from C++
  - Value conversion between C++ and scripts
  - Global and local variable access
  - Script file execution and hot reload
  - ScriptComponent for ECS integration
  - Script lifecycle callbacks (onInit, onUpdate, onDestroy)
  - Common engine bindings (math, input, graphics, audio, physics)
  - Template-based class/method binding
  - Module system with custom paths

- **Network System**:
  - Multiplayer state synchronization
  - Synchronized variables with dirty tracking
  - State snapshot generation and buffering
  - Client-server architecture support
  - Interpolation (linear, cubic, hermite)
  - Client-side prediction and server reconciliation
  - Remote procedure calls (RPCs)
  - Priority-based bandwidth management
  - Delta compression for efficiency
  - Interest management for relevancy filtering
  - Lag compensation with rewind/replay
  - Transform synchronization with quaternion slerp

- **Procedural Generation**:
  - Terrain generation with multiple noise algorithms
  - Perlin, Simplex, Worley, FBM, Ridged multifractal noise
  - Configurable octaves, frequency, amplitude
  - Automatic normal and UV coordinate generation
  - Biome system based on height/temperature/moisture
  - LOD support for large terrains

- **UI System**:
  - Constraint-based layout engine
  - Anchor-based positioning (top, bottom, left, right, center)
  - Constraint relationships between UI elements
  - Multiple size modes (fixed, percent, wrap_content, match_parent)
  - Min/max size constraints and aspect ratio preservation
  - Margin and padding support
  - Automatic constraint resolution

- **Asset Pipeline**:
  - Pluggable asset importer architecture
  - Support for multiple formats (mesh, texture, audio, shader, font)
  - Configurable import settings
  - Automatic type detection from file extensions
  - Mesh optimization (normal/tangent generation, vertex merging)
  - Texture processing (mipmaps, UV flipping, size limits)
  - Export capabilities for processed assets

#### January 22, 2026 - Major Feature Additions
- **Graphics System**:
  - ViewportManager for multi-view rendering with split-screen and picture-in-picture support
  - Dynamic viewport creation and management with layer-based selective rendering
  - Screen-to-viewport coordinate conversion utilities

- **Physics System**:
  - Comprehensive broad-phase collision detection with multiple spatial partitioning algorithms
  - SpatialHash implementation for uniform distribution scenarios
  - DynamicAABBTree with self-balancing capabilities for hierarchical collision detection
  - Sweep-and-prune algorithm with sorted axis lists
  - Optimized overlap queries and pair detection

- **Audio System**:
  - Streaming audio infrastructure for large audio files
  - Multi-format support (WAV, MP3, OGG, FLAC, AAC) through base streaming interface
  - Buffered playback with configurable buffer sizes and counts
  - Fade in/out effects with volume control and smooth transitions
  - Loop support and comprehensive playback state management
  - StreamingAudioManager for handling multiple concurrent streams

- **Animation System**:
  - Comprehensive curve interpolation system with 30+ easing functions
  - Customizable keyframe-based curves with Hermite spline interpolation
  - Tangent control for smooth curve transitions
  - Physics-based spring interpolation for natural motion
  - Unity-style SmoothDamp implementation for smooth value transitions
  - Support for quad, cubic, quartic, quintic, sine, expo, circular, elastic, back, and bounce easing

- **ECS System**:
  - Component dependency tracking and validation system
  - Registry for required, optional, and incompatible component relationships
  - Circular dependency detection with detailed cycle reporting
  - Topological sorting for correct component initialization order
  - Comprehensive validation with detailed error messages
  - Fluent API for registering dependencies

- **Networking System**:
  - Packet compression with multiple algorithms (RLE, Delta encoding, LZ77)
  - Bit-packing utilities for efficient data serialization
  - Float quantization with configurable precision
  - Delta encoding for incremental state updates
  - Automatic algorithm selection based on data patterns
  - BitPacker/BitUnpacker for compact binary representations

- **AI System**:
  - Decision tree framework for context-aware AI decision making
  - Condition nodes with true/false branching logic
  - Action leaf nodes for executing behaviors
  - Sequence and selector composite nodes for complex decision logic
  - Fluent builder API for intuitive tree construction
  - Reset and debug capabilities

- **Profiling System**:
  - Performance markers with RAII-style scoped profiling
  - Statistical analysis including min/max/avg/total execution times
  - Frame timing and FPS tracking
  - Performance budget tracking with over-budget warnings
  - CSV export functionality for external analysis
  - Detailed report generation with formatted output
  - Convenient PROFILE_SCOPE and PROFILE_FUNCTION macros

- **Scene Management**:
  - Scene transition effects system with multiple built-in transitions
  - Support for fade, cross-fade, wipe (4 directions), and dissolve effects
  - Custom transition support through extensible base class interface
  - Transition sequencing for complex multi-stage effects
  - Scene switch callbacks and completion event handling
  - Skip/cancel functionality with progress tracking
  - State machine-based transition management

- Project organization and documentation improvements

#### January 19, 2026 - System Enhancements
- **Debug System**:
  - ANSI color support to Logger for enhanced console readability with level-specific colors
  - Detailed allocation statistics to MemoryProfiler (size distribution, age tracking, fragmentation)
  - Common debug draw helpers for shapes, paths, Bezier curves, and frustums

- **Core System**:
  - Exception handling and validation to Config system with ConfigException
  - LRU caching to ResourceManager for improved performance with cache hit rate tracking
  - Required configuration key support with `getRequired<T>()` method

- **Threading System**:
  - Comprehensive performance metrics to ThreadPool including:
    - Worker efficiency tracking (execution time vs idle time)
    - Steal attempt success rates
    - Average job execution times
    - System-wide aggregated metrics

- **Events System**:
  - Compile-time type-safe event handlers with TypedEvent<T> base class
  - Template-based event subscription and dispatch for stronger type checking
  - Static assert validation for proper event inheritance

- **Graphics System**:
  - Shader bytecode caching for 10x faster compilation on subsequent loads
  - LOD transition hysteresis to prevent flickering during distance-based LOD switching
  - Usage examples to OcclusionCulling documentation

- **Animation System**:
  - Bounds checking and validation with safe frame access methods
  - Frame validation ensuring positive dimensions and duration

- **Audio System**:
  - Multiple distance attenuation models (Linear, Inverse, InverseSquare, Exponential, Custom)
  - Custom attenuation curve support with 16-point interpolation

- **Tilemap System**:
  - Frustum culling for optimized tile rendering
  - Visible tile range calculation to reduce draw calls

- **Shader System Enhancements**:
  - ShaderCacheManager for disk caching of compiled shader binaries with compression support
  - Async compilation support integrated into hot-reload manager
  - ShaderValidator for source code validation, deprecation detection, and performance hints
  - Parameterized macro expansion in ShaderPreprocessor
  - Uniform Buffer Object (UBO) support in ShaderVariant
  - Variant compilation queue for background processing
  - Advanced shader reflection with dependency extraction and metadata generation
  - Circular dependency detection in ShaderIncludeResolver
  - ShaderDebugger utilities for introspection and runtime debugging
  - ShaderProfiler for performance tracking with CSV/JSON export
  - ShaderLibrary.glsl with common math, lighting, tone mapping, and noise functions
  - Material system with property management and serialization
  - Shared memory and advanced features for ComputeShader
  - ShaderStatistics tracker for compilation metrics and reporting

### Changed
- Enhanced README with performance benchmarks and optimization details
- Updated performance metrics showing 95%+ thread pool efficiency and 90%+ cache hit rates

## [1.0.1] - 2026-01-26

### Added
- CI/CD workflow with GitHub Actions
- `StringUtils` utility class with `trim`, `split`, `toUpper`, `toLower`
- `Timer` utility class for high-resolution timing
- `clamp` function to `MathUtils`
- `Version` header for automated versioning
- Unit tests for `Vector2D` and `StringUtils`
- `.clang-format` for Google style code formatting
- Troubleshooting section to README

### Changed
- Refactored `DemoGame` out of `main.cpp` into separate class
- Refactored `Config.cpp` to use `StringUtils`
- Updated `.gitignore` with better defaults for macOS and IDEs

## [1.0.0] - 2026-01-12

### Added
- Initial release of JJM Game Engine
- Core rendering system with SDL2
- Entity Component System (ECS)
- Physics engine with collision detection
- Input management system
- Resource management
- Scene management
- Audio system
- Particle system
- Animation system with 20+ easing functions
- UI framework
- Camera system with effects
- Tilemap support
- Event system
- Debug tools and profiler
- State machine
- Configuration system
