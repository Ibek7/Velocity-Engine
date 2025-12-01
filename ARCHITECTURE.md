# Architecture Documentation

## JJM Velocity Engine Architecture

### Design Philosophy

The Velocity Engine follows several key principles:

1. **Modularity**: Systems are designed as independent modules with minimal coupling
2. **Performance**: Optimized for 2D game performance with hardware acceleration
3. **Extensibility**: Plugin system and clear interfaces for customization
4. **Developer-Friendly**: Clear APIs, comprehensive documentation, and debugging tools

### System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                         Application Layer                     │
│                      (Game-Specific Code)                     │
├─────────────────────────────────────────────────────────────┤
│                        Engine Systems                         │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐  │
│  │   ECS    │  Scene   │  State   │  Plugin  │  Script  │  │
│  │  System  │  Manager │  Machine │  System  │  Engine  │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘  │
├─────────────────────────────────────────────────────────────┤
│                      Subsystems Layer                         │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐  │
│  │  Physics │   AI     │Animation │  Audio   │ Network  │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘  │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐  │
│  │ Graphics │  Input   │ Particle │  Camera  │   GUI    │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘  │
├─────────────────────────────────────────────────────────────┤
│                       Core Services                           │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐  │
│  │ Resource │  Event   │  Memory  │ Profiler │   I/O    │  │
│  │  Manager │  System  │  Manager │          │ Serializ.│  │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘  │
├─────────────────────────────────────────────────────────────┤
│                      Foundation Layer                         │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐  │
│  │   Math   │  Utils   │  Config  │ Threading│  Logging │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘  │
├─────────────────────────────────────────────────────────────┤
│                      Platform Layer                           │
│                    SDL2 / OS Services                         │
└─────────────────────────────────────────────────────────────┘
```

### Namespace Organization

```cpp
JJM                        // Root namespace
├── Math                   // Mathematical primitives
│   ├── Vector2D
│   └── Matrix3x3
├── Graphics               // Rendering systems
│   ├── Renderer
│   ├── Texture
│   ├── SpriteBatch
│   ├── Lighting
│   └── Camera
├── Physics                // Physics simulation
│   ├── PhysicsWorld
│   ├── PhysicsBody
│   └── Collider
├── ECS                    // Entity Component System
│   ├── Entity
│   ├── EntityManager
│   ├── Component
│   └── ComponentFactory
├── AI                     // Artificial Intelligence
│   ├── BehaviorTree
│   ├── StateMachine
│   ├── Pathfinder
│   └── SteeringBehaviors
├── Animation              // Animation systems
│   ├── Animator
│   ├── AdvancedAnimator
│   ├── IKSolver
│   └── AnimationBlender
├── Audio                  // Audio management
│   ├── AudioManager
│   └── SpatialAudio
├── Serialization          // Save/load systems
│   ├── Serializer
│   └── SerializationContext
└── ...
```

### Core Systems

#### Entity Component System (ECS)

The ECS follows a data-oriented design:

```
Entity (ID) ──► Component Storage ──► System Processing
     │               │                      │
     │               ├─ Transform          ├─ PhysicsSystem
     │               ├─ Renderable         ├─ RenderSystem
     │               ├─ PhysicsBody        └─ AISystem
     │               └─ Script
     └─ Managed by EntityManager
```

**Benefits:**
- Cache-friendly iteration
- Easy parallelization
- Dynamic composition
- Hot-reloading support

#### Resource Management

```cpp
ResourceManager
├── AssetLoader (Background loading)
├── Asset Caching (LRU eviction)
├── Reference Counting (Shared ownership)
└── Streaming System (LOD management)
```

**Features:**
- Automatic lifetime management
- Weak references for non-owning access
- Priority-based loading
- Memory budget enforcement

#### Rendering Pipeline

```
Scene Graph
    │
    ▼
Culling & Sorting
    │
    ▼
Batch Generation
    │
    ▼
Shader Binding
    │
    ▼
Draw Calls
    │
    ▼
Post-Processing
    │
    ▼
Present
```

**Optimizations:**
- Sprite batching reduces draw calls
- Spatial partitioning for culling
- Material-based sorting
- Texture atlas reduces state changes

#### Physics System

Uses spatial hashing for broad-phase collision detection:

```
Spatial Hash Grid
├── Cell [0,0]: [Entity1, Entity3]
├── Cell [1,0]: [Entity2]
└── Cell [0,1]: [Entity3, Entity4]
         │
         ▼
  Narrow-phase checks only
  within same or adjacent cells
```

**Integration:**
- Fixed timestep (60 Hz)
- Velocity Verlet integration
- Impulse-based resolution
- Continuous collision detection

### Advanced Systems

#### AI Behavior Trees

Hierarchical structure for decision-making:

```
           Sequence
          /    |    \
    Condition  Action  Selector
                     /   |    \
                 Action Action Action
```

**Node Types:**
- Composites: Sequence, Selector, Parallel
- Decorators: Inverter, Repeater, Succeeder
- Leaves: Action, Condition

**Blackboard:**
- Shared data between nodes
- Type-erased storage (std::any)
- Read/write by name

#### Animation System

Two-tier animation architecture:

**Basic (Sprite):**
- Frame-based animation
- Sprite sheet support
- Easing functions

**Advanced (Skeletal):**
- Bone hierarchies
- Inverse kinematics
- Multi-layer blending
- Procedural animation

```
Skeleton
├── Bone: Root (Transform)
│   ├── Bone: Spine (Transform)
│   │   ├── Bone: LeftArm
│   │   └── Bone: RightArm
│   └── Bone: Pelvis
│       ├── Bone: LeftLeg (IK Target)
│       └── Bone: RightLeg (IK Target)
```

#### Serialization

Dual format support:

**Binary:**
- Compact size
- Fast read/write
- Type safety
- Version tags

**JSON:**
- Human-readable
- Easy debugging
- Cross-platform
- Schema validation

### Threading Model

```
Main Thread
├── Input Processing
├── Game Logic Update
├── Render Commands
└── Frame Synchronization

Worker Threads
├── Asset Loading
├── Physics Jobs
├── Particle Updates
└── AI Processing
```

**Synchronization:**
- Job system for parallel work
- Lock-free queues for communication
- Thread-safe resource access
- Frame-based synchronization points

### Memory Management

**Strategies:**
- RAII throughout (no manual new/delete)
- Smart pointers for ownership
- Memory pools for frequent allocations
- Stack allocators for temporary data

**Profiling:**
- Per-category tracking
- Peak usage monitoring
- Leak detection
- Fragmentation analysis

### Plugin System

Dynamic library loading:

```cpp
Plugin Interface
├── onLoad()
├── onUnload()
├── update()
├── getName()
├── getVersion()
└── getDependencies()
```

**Features:**
- Hot-reload during development
- Dependency resolution
- Version compatibility
- Isolated namespaces

### Event System

Decoupled communication:

```
Event Dispatcher (Singleton)
    │
    ├── Listeners["player_death"] = [callback1, callback2]
    ├── Listeners["score_changed"] = [callback3]
    └── Listeners["level_complete"] = [callback4]
```

**Usage:**
- Register listeners by event name
- Type-erased event data
- Priority-based dispatch
- Immediate or queued delivery

### Performance Profiling

Multi-level profiling:

```
PerformanceProfiler
├── Frame Timing
│   ├── FPS calculation
│   ├── Frame time variance
│   └── Percentiles (95th, 99th)
├── Memory Tracking
│   ├── Per-category allocation
│   ├── Peak usage
│   └── Leak detection
├── GPU Stats
│   ├── Draw calls
│   ├── Triangle count
│   └── Texture switches
└── Chrome Tracing Export
    └── Visualize in chrome://tracing
```

### Data Flow

**Update Loop:**
```
Input → Events → State → Logic → Physics → Animation → Rendering
  │                                                          │
  └──────────────── Frame Boundary ───────────────────────┘
```

**Asset Loading:**
```
Request → Priority Queue → Background Thread → Decode → Upload → Ready
                                    │              │        │
                                    └─ Progress ───┴────────┘
```

### Best Practices

1. **System Initialization Order:**
   - Config → Logging → Platform → Managers → Systems → Game

2. **Update Order:**
   - Input → AI → Physics → Animation → Rendering → Audio

3. **Resource Loading:**
   - Async for large assets
   - Sync for critical resources
   - Streaming for open worlds

4. **Error Handling:**
   - Assertions for programmer errors
   - Exceptions for runtime failures
   - Logging for diagnostics

5. **Performance:**
   - Profile before optimizing
   - Batch similar operations
   - Use object pools
   - Prefer cache-friendly access patterns

### Extension Points

**Add Custom Components:**
```cpp
class MyComponent : public JJM::ECS::Component {
    // Implementation
};
```

**Create Custom Systems:**
```cpp
class MySystem {
    void update(float dt, JJM::ECS::EntityManager& em);
};
```

**Implement Custom Serializers:**
```cpp
class MyData : public JJM::Serialization::ISerializable {
    void serialize(Serializer& s) const override;
    void deserialize(Serializer& s) override;
};
```

### Performance Targets

- **Frame Rate:** 60 FPS minimum (16.67ms budget)
- **Physics:** Fixed 60 Hz timestep
- **Draw Calls:** < 100 per frame (batching)
- **Memory:** < 500 MB typical usage
- **Load Time:** < 2s for level loading

### Future Architecture Improvements

1. **ECS v2:** Cache-friendly archetype storage
2. **Job System:** Better thread utilization
3. **Vulkan Backend:** Modern graphics API
4. **Asset Pipeline:** Build-time optimization
5. **Network Replication:** Authoritative server model
