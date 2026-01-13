# Changelog

All notable changes to the JJM Game Engine will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Project organization and documentation improvements
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
