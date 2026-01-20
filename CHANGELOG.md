# Changelog

All notable changes to the JJM Game Engine will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
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
