#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <functional>
#include <vector>

#include "graphics/Color.h"
#include "graphics/Renderer.h"
#include "math/Vector2D.h"

namespace JJM {
namespace Particles {

struct Particle {
    Math::Vector2D position;
    Math::Vector2D velocity;
    Graphics::Color color;
    float lifetime;
    float maxLifetime;
    float size;
    float rotation;
    float rotationSpeed;
    bool active;

    Particle();
    void update(float deltaTime);
    bool isAlive() const;
};

class ParticleEmitter {
   private:
    std::vector<Particle> particles;
    Math::Vector2D position;
    int maxParticles;
    float emissionRate;
    float emissionTimer;
    bool active;
    bool burst;

    // Emission properties
    float minLifetime, maxLifetime;
    float minSpeed, maxSpeed;
    float minSize, maxSize;
    float minAngle, maxAngle;
    Graphics::Color startColor, endColor;
    Math::Vector2D gravity;

    // Emission patterns
    enum class EmissionPattern {
        Point,   // Emit from a single point
        Circle,  // Emit in a circular pattern
        Ring,    // Emit from ring perimeter
        Cone,    // Emit in cone shape
        Box,     // Emit from box area
        Line,    // Emit along a line
        Spiral,  // Emit in spiral pattern
        Burst    // All at once
    };
    EmissionPattern m_emissionPattern;
    float m_patternRadius;
    float m_patternAngle;
    Math::Vector2D m_patternSize;

    // Performance optimizations
    bool m_useBatchRendering;
    bool m_useObjectPooling;
    std::vector<int> m_freeList;  // Indices of inactive particles for reuse

   public:
    ParticleEmitter(const Math::Vector2D& pos, int maxParticles = 100);
    virtual ~ParticleEmitter() = default;

    virtual void update(float deltaTime);
    virtual void render(Graphics::Renderer* renderer);

    void emit(int count = 1);
    void emitBurst(int count);
    void start();
    void stop();
    void reset();

    // Setters for emission properties
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    void setEmissionRate(float rate) { emissionRate = rate; }
    void setLifetime(float min, float max);
    void setSpeed(float min, float max);
    void setSize(float min, float max);
    void setAngleRange(float min, float max);
    void setColorRange(const Graphics::Color& start, const Graphics::Color& end);
    void setGravity(const Math::Vector2D& g) { gravity = g; }

    // Emission patterns
    void setEmissionPattern(EmissionPattern pattern) { m_emissionPattern = pattern; }
    EmissionPattern getEmissionPattern() const { return m_emissionPattern; }
    void setPatternRadius(float radius) { m_patternRadius = radius; }
    void setPatternAngle(float angle) { m_patternAngle = angle; }
    void setPatternSize(const Math::Vector2D& size) { m_patternSize = size; }

    // Preset patterns
    void usePointEmission();
    void useCircleEmission(float radius);
    void useRingEmission(float radius);
    void useConeEmission(float angle, float radius);
    void useBoxEmission(float width, float height);
    void useLineEmission(const Math::Vector2D& start, const Math::Vector2D& end);
    void useSpiralEmission(float radius, float rotationSpeed);

    // Performance optimizations
    void setBatchRendering(bool enable) { m_useBatchRendering = enable; }
    bool isBatchRenderingEnabled() const { return m_useBatchRendering; }
    void setObjectPooling(bool enable) { m_useObjectPooling = enable; }
    bool isObjectPoolingEnabled() const { return m_useObjectPooling; }
    void preallocateParticles(int count);

    // Getters
    bool isActive() const { return active; }
    int getActiveParticleCount() const;
    const Math::Vector2D& getPosition() const { return position; }

   private:
    void createParticle();
    Math::Vector2D calculateEmissionPosition();
    Math::Vector2D calculateEmissionVelocity(const Math::Vector2D& emitPos);
    float randomFloat(float min, float max);
    int allocateParticle();           // Returns index of free particle or -1
    void releaseParticle(int index);  // Mark particle as free for reuse
    void renderBatched(Graphics::Renderer* renderer);
    void renderIndividual(Graphics::Renderer* renderer);
};

class ParticleSystem {
   private:
    std::vector<ParticleEmitter*> emitters;

    // Performance monitoring
    struct PerformanceMetrics {
        int totalParticleCount;
        float updateTimeMs;
        float renderTimeMs;
        float averageUpdateTime;
        float averageRenderTime;
        int frameCount;

        PerformanceMetrics()
            : totalParticleCount(0),
              updateTimeMs(0.0f),
              renderTimeMs(0.0f),
              averageUpdateTime(0.0f),
              averageRenderTime(0.0f),
              frameCount(0) {}

        void reset() {
            updateTimeMs = 0.0f;
            renderTimeMs = 0.0f;
            frameCount = 0;
        }
    };

    PerformanceMetrics m_metrics;
    bool m_enableProfiling;

   public:
    ParticleSystem();
    ~ParticleSystem();

    ParticleEmitter* createEmitter(const Math::Vector2D& position, int maxParticles = 100);
    void removeEmitter(ParticleEmitter* emitter);
    void removeAllEmitters();

    void update(float deltaTime);
    void render(Graphics::Renderer* renderer);

    int getEmitterCount() const { return static_cast<int>(emitters.size()); }

    // Performance monitoring
    void enableProfiling(bool enable) { m_enableProfiling = enable; }
    bool isProfilingEnabled() const { return m_enableProfiling; }
    int getTotalParticleCount() const;
    float getUpdateTimeMs() const { return m_metrics.updateTimeMs; }
    float getRenderTimeMs() const { return m_metrics.renderTimeMs; }
    float getAverageUpdateTime() const { return m_metrics.averageUpdateTime; }
    float getAverageRenderTime() const { return m_metrics.averageRenderTime; }
    void resetMetrics() { m_metrics.reset(); }
};

/**
 * @brief Sub-emitter trigger conditions
 */
enum class SubEmitterTrigger {
    Birth,      // When parent particle is born
    Death,      // When parent particle dies
    Collision,  // When parent particle collides
    Manual,     // Manually triggered
    Lifetime    // At specific lifetime percentage
};

/**
 * @brief Sub-emitter configuration
 */
struct SubEmitterConfig {
    SubEmitterTrigger trigger;
    float triggerProbability;  // 0-1 chance to trigger
    int particlesPerTrigger;
    float inheritVelocity;  // 0-1 velocity inheritance
    float inheritScale;     // 0-1 scale inheritance
    float inheritRotation;  // 0-1 rotation inheritance
    bool inheritColor;
    float lifetimeThreshold;  // For Lifetime trigger (0-1)

    SubEmitterConfig()
        : trigger(SubEmitterTrigger::Death),
          triggerProbability(1.0f),
          particlesPerTrigger(5),
          inheritVelocity(0.5f),
          inheritScale(0.5f),
          inheritRotation(0.0f),
          inheritColor(true),
          lifetimeThreshold(0.5f) {}
};

/**
 * @brief Sub-emitter for spawning particles from other particles
 */
class SubEmitter {
   public:
    SubEmitter(ParticleEmitter* emitterTemplate, const SubEmitterConfig& config);
    ~SubEmitter();

    void trigger(const Particle& parentParticle);
    void update(float deltaTime);
    void render(Graphics::Renderer* renderer);

    void setConfig(const SubEmitterConfig& config) { this->config = config; }
    const SubEmitterConfig& getConfig() const { return config; }

    ParticleEmitter* getEmitter() { return emitter; }
    bool isActive() const { return active; }

   private:
    ParticleEmitter* emitter;
    SubEmitterConfig config;
    bool active;

    void applyInheritance(Particle& childParticle, const Particle& parentParticle);
};

/**
 * @brief Particle affector types
 */
enum class AffectorType {
    Force,       // Constant force
    Attractor,   // Point attraction
    Repeller,    // Point repulsion
    Vortex,      // Spiral motion
    Turbulence,  // Random turbulence
    Drag,        // Air resistance
    Color,       // Color over lifetime
    Scale,       // Scale over lifetime
    Rotation     // Rotation over lifetime
};

/**
 * @brief Base particle affector
 */
class ParticleAffector {
   public:
    ParticleAffector(AffectorType type);
    virtual ~ParticleAffector() = default;

    virtual void affect(Particle& particle, float deltaTime) = 0;

    AffectorType getType() const { return type; }
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    void setStrength(float strength) { this->strength = strength; }
    float getStrength() const { return strength; }

   protected:
    AffectorType type;
    bool enabled;
    float strength;
};

/**
 * @brief Force affector - applies constant force
 */
class ForceAffector : public ParticleAffector {
   public:
    ForceAffector(const Math::Vector2D& force);

    void affect(Particle& particle, float deltaTime) override;

    void setForce(const Math::Vector2D& force) { this->force = force; }
    const Math::Vector2D& getForce() const { return force; }

   private:
    Math::Vector2D force;
};

/**
 * @brief Attractor affector - pulls particles toward a point
 */
class AttractorAffector : public ParticleAffector {
   public:
    AttractorAffector(const Math::Vector2D& position, float force, float radius);

    void affect(Particle& particle, float deltaTime) override;

    void setPosition(const Math::Vector2D& pos) { position = pos; }
    const Math::Vector2D& getPosition() const { return position; }
    void setRadius(float r) { radius = r; }
    float getRadius() const { return radius; }
    void setFalloff(float f) { falloff = f; }

   private:
    Math::Vector2D position;
    float radius;
    float falloff;
};

/**
 * @brief Vortex affector - creates spiral motion
 */
class VortexAffector : public ParticleAffector {
   public:
    VortexAffector(const Math::Vector2D& center, float rotationSpeed, float pullStrength);

    void affect(Particle& particle, float deltaTime) override;

    void setCenter(const Math::Vector2D& c) { center = c; }
    void setRotationSpeed(float speed) { rotationSpeed = speed; }
    void setPullStrength(float pull) { pullStrength = pull; }

   private:
    Math::Vector2D center;
    float rotationSpeed;
    float pullStrength;
};

/**
 * @brief Turbulence affector - adds random motion
 */
class TurbulenceAffector : public ParticleAffector {
   public:
    TurbulenceAffector(float frequency, float amplitude);

    void affect(Particle& particle, float deltaTime) override;

    void setFrequency(float f) { frequency = f; }
    void setAmplitude(float a) { amplitude = a; }

   private:
    float frequency;
    float amplitude;
    float time;
};

/**
 * @brief Color gradient for color over lifetime
 */
struct ColorGradient {
    struct ColorStop {
        float position;  // 0-1
        Graphics::Color color;
    };

    std::vector<ColorStop> stops;

    void addStop(float position, const Graphics::Color& color);
    Graphics::Color evaluate(float t) const;
};

/**
 * @brief Color over lifetime affector
 */
class ColorOverLifetimeAffector : public ParticleAffector {
   public:
    ColorOverLifetimeAffector();

    void affect(Particle& particle, float deltaTime) override;

    void setGradient(const ColorGradient& gradient) { this->gradient = gradient; }
    ColorGradient& getGradient() { return gradient; }

   private:
    ColorGradient gradient;
};

/**
 * @brief Scale over lifetime affector
 */
class ScaleOverLifetimeAffector : public ParticleAffector {
   public:
    ScaleOverLifetimeAffector(float startScale, float endScale);

    void affect(Particle& particle, float deltaTime) override;

    void setStartScale(float scale) { startScale = scale; }
    void setEndScale(float scale) { endScale = scale; }
    void setScaleCurve(std::function<float(float)> curve) { scaleCurve = curve; }

   private:
    float startScale;
    float endScale;
    std::function<float(float)> scaleCurve;
};

/**
 * @brief Advanced particle emitter with sub-emitters and affectors
 */
class AdvancedParticleEmitter : public ParticleEmitter {
   public:
    AdvancedParticleEmitter(const Math::Vector2D& pos, int maxParticles = 100);
    ~AdvancedParticleEmitter();

    void update(float deltaTime);
    void render(Graphics::Renderer* renderer) override;

    // Sub-emitter management
    SubEmitter* addSubEmitter(ParticleEmitter* emitterTemplate, const SubEmitterConfig& config);
    void removeSubEmitter(SubEmitter* subEmitter);
    void clearSubEmitters();
    size_t getSubEmitterCount() const { return subEmitters.size(); }

    // Affector management
    template <typename T, typename... Args>
    T* addAffector(Args&&... args);
    void removeAffector(ParticleAffector* affector);
    void clearAffectors();
    size_t getAffectorCount() const { return affectors.size(); }

    // Event callbacks
    void setOnParticleBirth(std::function<void(Particle&)> callback) { onParticleBirth = callback; }
    void setOnParticleDeath(std::function<void(const Particle&)> callback) {
        onParticleDeath = callback;
    }
    void setOnParticleCollision(std::function<void(Particle&)> callback) {
        onParticleCollision = callback;
    }

   private:
    std::vector<std::unique_ptr<SubEmitter>> subEmitters;
    std::vector<std::unique_ptr<ParticleAffector>> affectors;

    std::function<void(Particle&)> onParticleBirth;
    std::function<void(const Particle&)> onParticleDeath;
    std::function<void(Particle&)> onParticleCollision;

    void triggerSubEmitters(SubEmitterTrigger trigger, const Particle& particle);
    void applyAffectors(Particle& particle, float deltaTime);
};

template <typename T, typename... Args>
T* AdvancedParticleEmitter::addAffector(Args&&... args) {
    auto affector = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = affector.get();
    affectors.push_back(std::move(affector));
    return ptr;
}

// =============================================================================
// EMITTER SHAPES
// =============================================================================

/**
 * @brief Emission shape types
 */
enum class EmitterShapeType {
    Point,
    Circle,
    CircleEdge,
    Sphere,
    SphereShell,
    Hemisphere,
    Cone,
    Box,
    BoxEdge,
    BoxShell,
    Line,
    Mesh,
    SkinnedMesh,
    Custom
};

/**
 * @brief Base emitter shape
 */
class EmitterShape {
   public:
    virtual ~EmitterShape() = default;

    virtual EmitterShapeType getType() const = 0;
    virtual void getEmissionPoint(Math::Vector2D& position, Math::Vector2D& direction) const = 0;

    // 3D version for future expansion
    virtual void getEmissionPoint3D(float& x, float& y, float& z, float& dirX, float& dirY,
                                    float& dirZ) const {
        Math::Vector2D pos, dir;
        getEmissionPoint(pos, dir);
        x = pos.x;
        y = pos.y;
        z = 0.0f;
        dirX = dir.x;
        dirY = dir.y;
        dirZ = 0.0f;
    }

    // Randomization
    void setRandomizeDirection(bool randomize) { randomizeDirection = randomize; }
    void setDirectionSpread(float spread) { directionSpread = spread; }

   protected:
    bool randomizeDirection{false};
    float directionSpread{0.0f};

    float randomFloat(float min, float max) const;
    float randomAngle() const;
};

/**
 * @brief Point emitter shape
 */
class PointShape : public EmitterShape {
   public:
    PointShape(const Math::Vector2D& point = Math::Vector2D(0, 0));

    EmitterShapeType getType() const override { return EmitterShapeType::Point; }
    void getEmissionPoint(Math::Vector2D& position, Math::Vector2D& direction) const override;

    void setPosition(const Math::Vector2D& pos) { point = pos; }

   private:
    Math::Vector2D point;
};

/**
 * @brief Circle emitter shape
 */
class CircleShape : public EmitterShape {
   public:
    CircleShape(const Math::Vector2D& center, float radius, bool edgeOnly = false);

    EmitterShapeType getType() const override {
        return edgeOnly ? EmitterShapeType::CircleEdge : EmitterShapeType::Circle;
    }
    void getEmissionPoint(Math::Vector2D& position, Math::Vector2D& direction) const override;

    void setCenter(const Math::Vector2D& c) { center = c; }
    void setRadius(float r) { radius = r; }
    void setEdgeOnly(bool edge) { edgeOnly = edge; }
    void setArc(float startAngle, float endAngle);

   private:
    Math::Vector2D center;
    float radius;
    bool edgeOnly;
    float arcStart{0.0f};
    float arcEnd{360.0f};
};

/**
 * @brief Cone emitter shape
 */
class ConeShape : public EmitterShape {
   public:
    ConeShape(const Math::Vector2D& apex, float angle, float length);

    EmitterShapeType getType() const override { return EmitterShapeType::Cone; }
    void getEmissionPoint(Math::Vector2D& position, Math::Vector2D& direction) const override;

    void setApex(const Math::Vector2D& a) { apex = a; }
    void setAngle(float a) { angle = a; }
    void setLength(float l) { length = l; }
    void setEmitFromBase(bool fromBase) { emitFromBase = fromBase; }

   private:
    Math::Vector2D apex;
    float angle;
    float length;
    bool emitFromBase{false};
};

/**
 * @brief Box emitter shape
 */
class BoxShape : public EmitterShape {
   public:
    BoxShape(const Math::Vector2D& center, const Math::Vector2D& size, bool edgeOnly = false);

    EmitterShapeType getType() const override {
        return edgeOnly ? EmitterShapeType::BoxEdge : EmitterShapeType::Box;
    }
    void getEmissionPoint(Math::Vector2D& position, Math::Vector2D& direction) const override;

    void setCenter(const Math::Vector2D& c) { center = c; }
    void setSize(const Math::Vector2D& s) { size = s; }

   private:
    Math::Vector2D center;
    Math::Vector2D size;
    bool edgeOnly;
};

/**
 * @brief Line emitter shape
 */
class LineShape : public EmitterShape {
   public:
    LineShape(const Math::Vector2D& start, const Math::Vector2D& end);

    EmitterShapeType getType() const override { return EmitterShapeType::Line; }
    void getEmissionPoint(Math::Vector2D& position, Math::Vector2D& direction) const override;

    void setPoints(const Math::Vector2D& s, const Math::Vector2D& e) {
        start = s;
        end = e;
    }

   private:
    Math::Vector2D start;
    Math::Vector2D end;
};

// =============================================================================
// NOISE AND CURL NOISE
// =============================================================================

/**
 * @brief Noise types for particle motion
 */
enum class NoiseType {
    Perlin,
    Simplex,
    Worley,
    Curl,  // Curl noise (divergence-free)
    FBM,   // Fractal Brownian Motion
    Turbulence
};

/**
 * @brief Noise field configuration
 */
struct NoiseFieldConfig {
    NoiseType type{NoiseType::Curl};
    float frequency{1.0f};
    float amplitude{1.0f};
    int octaves{4};
    float persistence{0.5f};
    float lacunarity{2.0f};
    float scrollSpeed{0.0f};
    Math::Vector2D scrollDirection{1.0f, 0.0f};

    // Quality
    int resolution{128};
    bool useGPU{false};
};

/**
 * @brief 2D/3D noise field for particle motion
 */
class NoiseField {
   public:
    NoiseField(const NoiseFieldConfig& config = {});
    ~NoiseField();

    // Sampling
    Math::Vector2D sample(const Math::Vector2D& position) const;
    void sample3D(float x, float y, float z, float& outX, float& outY, float& outZ) const;

    // Configuration
    void setConfig(const NoiseFieldConfig& config);
    const NoiseFieldConfig& getConfig() const { return config; }

    // Animation
    void update(float deltaTime);
    void setTime(float time) { currentTime = time; }
    float getTime() const { return currentTime; }

    // GPU texture generation
    unsigned int generateNoiseTexture() const;
    unsigned int generateCurlTexture() const;

   private:
    NoiseFieldConfig config;
    float currentTime{0.0f};

    float perlinNoise(float x, float y) const;
    float simplexNoise(float x, float y) const;
    Math::Vector2D curlNoise(float x, float y) const;
};

/**
 * @brief Noise-based particle affector
 */
class NoiseAffector : public ParticleAffector {
   public:
    NoiseAffector(const NoiseFieldConfig& config = {});

    void affect(Particle& particle, float deltaTime) override;
    void update(float deltaTime);

    NoiseField& getNoiseField() { return noiseField; }
    void setPositionScale(float scale) { positionScale = scale; }

   private:
    NoiseField noiseField;
    float positionScale{0.01f};
};

// =============================================================================
// PARTICLE TRAILS
// =============================================================================

/**
 * @brief Trail vertex data
 */
struct TrailVertex {
    float x, y, z;
    float u, v;
    float r, g, b, a;
    float width;
};

/**
 * @brief Trail point in history
 */
struct TrailPoint {
    Math::Vector2D position;
    Graphics::Color color;
    float width;
    float lifetime;
    float age;
};

/**
 * @brief Particle trail configuration
 */
struct TrailConfig {
    float lifetime{1.0f};
    float minVertexDistance{0.1f};  // Minimum distance between trail points
    int maxPoints{50};
    float widthStart{1.0f};
    float widthEnd{0.0f};
    Graphics::Color colorStart{255, 255, 255, 255};
    Graphics::Color colorEnd{255, 255, 255, 0};

    // Texture
    bool useTexture{false};
    unsigned int textureId{0};
    float textureMode{0};  // 0 = stretch, 1 = tile
    float textureScale{1.0f};

    // Rendering
    bool worldSpace{true};  // Trail in world space or local to emitter
    bool inheritParticleColor{true};
    bool dieWithParticle{true};
};

/**
 * @brief Particle trail renderer
 */
class ParticleTrail {
   public:
    ParticleTrail(const TrailConfig& config = {});
    ~ParticleTrail();

    // Update
    void addPoint(const Math::Vector2D& position, const Graphics::Color& color, float width);
    void update(float deltaTime);
    void clear();

    // Rendering
    void render(Graphics::Renderer* renderer);
    void generateMesh(std::vector<TrailVertex>& vertices, std::vector<uint32_t>& indices);

    // Configuration
    void setConfig(const TrailConfig& config) { this->config = config; }
    const TrailConfig& getConfig() const { return config; }

    // Query
    size_t getPointCount() const { return points.size(); }
    bool isEmpty() const { return points.empty(); }
    float getLength() const;

   private:
    TrailConfig config;
    std::vector<TrailPoint> points;
    float accumulatedDistance{0.0f};
    Math::Vector2D lastPosition;
    bool hasLastPosition{false};

    void removeOldPoints();
    Graphics::Color interpolateColor(float t) const;
    float interpolateWidth(float t) const;
};

/**
 * @brief Trail manager for multiple trails
 */
class ParticleTrailManager {
   public:
    ParticleTrailManager(int maxTrails = 1000);
    ~ParticleTrailManager();

    // Trail management
    int createTrail(const TrailConfig& config = {});
    void destroyTrail(int trailId);
    ParticleTrail* getTrail(int trailId);

    // Batch operations
    void updateAll(float deltaTime);
    void renderAll(Graphics::Renderer* renderer);
    void clearAll();

    // Statistics
    size_t getActiveTrailCount() const;
    size_t getTotalPointCount() const;

   private:
    std::unordered_map<int, std::unique_ptr<ParticleTrail>> trails;
    int nextTrailId{0};
    int maxTrails;
};

// =============================================================================
// PARTICLE LOD SYSTEM
// =============================================================================

/**
 * @brief LOD level configuration
 */
struct ParticleLODLevel {
    float distance;                 // Distance threshold
    float particleCountMultiplier;  // 0-1 multiplier for particle count
    float emissionRateMultiplier;   // 0-1 multiplier for emission rate
    float updateFrequency;          // Updates per second
    bool enableTrails;
    bool enableSubEmitters;
    bool enableCollision;
    bool enableSorting;
    int maxParticles;  // Override max particles (-1 = no override)

    ParticleLODLevel()
        : distance(0.0f),
          particleCountMultiplier(1.0f),
          emissionRateMultiplier(1.0f),
          updateFrequency(60.0f),
          enableTrails(true),
          enableSubEmitters(true),
          enableCollision(true),
          enableSorting(true),
          maxParticles(-1) {}
};

/**
 * @brief Particle LOD manager
 */
class ParticleLODManager {
   public:
    ParticleLODManager();

    // LOD level setup
    void addLODLevel(const ParticleLODLevel& level);
    void clearLODLevels();
    void setLODLevels(const std::vector<ParticleLODLevel>& levels);
    const std::vector<ParticleLODLevel>& getLODLevels() const { return lodLevels; }

    // LOD calculation
    int calculateLODLevel(float distance) const;
    const ParticleLODLevel& getLODForDistance(float distance) const;

    // Global settings
    void setLODBias(float bias) { lodBias = bias; }
    float getLODBias() const { return lodBias; }
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }

    // Screen size based LOD
    void setScreenSizeLOD(bool enabled) { useScreenSize = enabled; }
    float calculateScreenSize(float distance, float boundingRadius, float fov,
                              float screenHeight) const;

    // Presets
    static ParticleLODManager createDefaultLOD();
    static ParticleLODManager createAggressiveLOD();
    static ParticleLODManager createQualityLOD();

   private:
    std::vector<ParticleLODLevel> lodLevels;
    float lodBias{1.0f};
    bool enabled{true};
    bool useScreenSize{false};

    static ParticleLODLevel defaultLevel;
};

// =============================================================================
// GPU PARTICLE SIMULATION
// =============================================================================

/**
 * @brief GPU particle data layout
 */
struct GPUParticle {
    float positionX, positionY, positionZ;
    float velocityX, velocityY, velocityZ;
    float colorR, colorG, colorB, colorA;
    float size;
    float rotation;
    float lifetime;
    float age;
    uint32_t flags;
    float userData[3];  // Custom data
};

/**
 * @brief GPU particle buffer configuration
 */
struct GPUParticleBufferConfig {
    int maxParticles{10000};
    bool doubleBuffered{true};
    bool useAtomicCounters{true};
    bool useIndirectDraw{true};
    int sortBuckets{256};  // For GPU radix sort
};

/**
 * @brief GPU particle emitter configuration
 */
struct GPUEmitterConfig {
    EmitterShapeType shape{EmitterShapeType::Point};
    float emissionRate{100.0f};
    float lifetime{2.0f};
    float lifetimeVariation{0.5f};
    float speed{5.0f};
    float speedVariation{2.0f};
    float size{1.0f};
    float sizeVariation{0.5f};
    float rotation{0.0f};
    float rotationSpeed{0.0f};
    Graphics::Color colorStart{255, 255, 255, 255};
    Graphics::Color colorEnd{255, 255, 255, 0};

    // Shape parameters
    float shapeRadius{1.0f};
    float shapeAngle{30.0f};
    float shapeLength{1.0f};
};

/**
 * @brief GPU force field types
 */
enum class GPUForceType { Directional, Point, Vortex, Turbulence, Curl };

/**
 * @brief GPU force field data
 */
struct GPUForceField {
    GPUForceType type;
    float positionX, positionY, positionZ;
    float directionX, directionY, directionZ;
    float strength;
    float radius;
    float falloff;
    bool enabled;
};

/**
 * @brief GPU-based particle system
 */
class GPUParticleSystem {
   public:
    GPUParticleSystem(const GPUParticleBufferConfig& config = {});
    ~GPUParticleSystem();

    // Initialization
    bool initialize();
    void shutdown();
    bool isInitialized() const { return initialized; }

    // Emitter management
    int addEmitter(const GPUEmitterConfig& config);
    void removeEmitter(int emitterId);
    void updateEmitter(int emitterId, const GPUEmitterConfig& config);
    GPUEmitterConfig* getEmitterConfig(int emitterId);

    // Force fields
    int addForceField(const GPUForceField& field);
    void removeForceField(int fieldId);
    void updateForceField(int fieldId, const GPUForceField& field);

    // Simulation
    void emit(int emitterId, int count);
    void update(float deltaTime);
    void render(const float* viewMatrix, const float* projectionMatrix);

    // Sorting
    void setSortingEnabled(bool enabled) { sortingEnabled = enabled; }
    bool isSortingEnabled() const { return sortingEnabled; }
    void sort(float cameraX, float cameraY, float cameraZ);

    // Buffers
    unsigned int getParticleBuffer() const { return particleBuffer; }
    unsigned int getAliveCountBuffer() const { return aliveCountBuffer; }
    unsigned int getDeadListBuffer() const { return deadListBuffer; }

    // Statistics
    struct GPUStats {
        int aliveParticles;
        int totalCapacity;
        int activeEmitters;
        int activeForceFields;
        float simulationTimeMs;
        float renderTimeMs;
    };
    GPUStats getStatistics() const;

    // Configuration
    void setGlobalGravity(float x, float y, float z);
    void setGlobalDrag(float drag) { globalDrag = drag; }
    void setTimeScale(float scale) { timeScale = scale; }

   private:
    GPUParticleBufferConfig config;
    bool initialized{false};

    // GPU buffers
    unsigned int particleBuffer{0};
    unsigned int aliveListBuffer{0};
    unsigned int deadListBuffer{0};
    unsigned int aliveCountBuffer{0};
    unsigned int indirectDrawBuffer{0};

    // Sort buffers
    unsigned int sortKeysBuffer{0};
    unsigned int sortValuesBuffer{0};
    bool sortingEnabled{true};

    // Compute shaders
    unsigned int emitShader{0};
    unsigned int updateShader{0};
    unsigned int sortShader{0};

    // Render shader
    unsigned int renderShader{0};
    unsigned int vao{0};

    // Emitters and forces
    std::unordered_map<int, GPUEmitterConfig> emitters;
    std::unordered_map<int, GPUForceField> forceFields;
    unsigned int emitterBuffer{0};
    unsigned int forceFieldBuffer{0};
    int nextEmitterId{0};
    int nextForceFieldId{0};

    // Global parameters
    float globalGravity[3]{0.0f, -9.8f, 0.0f};
    float globalDrag{0.1f};
    float timeScale{1.0f};

    mutable GPUStats stats{};

    void createBuffers();
    void deleteBuffers();
    void uploadEmitters();
    void uploadForceFields();
    void runEmitCompute(int emitterId, int count);
    void runUpdateCompute(float deltaTime);
    void runSortCompute(float cameraX, float cameraY, float cameraZ);
};

// =============================================================================
// PARTICLE SYSTEM PRESETS
// =============================================================================

/**
 * @brief Common particle effect presets
 */
class ParticlePresets {
   public:
    // Fire and smoke
    static std::unique_ptr<AdvancedParticleEmitter> createFire(const Math::Vector2D& position,
                                                               float intensity = 1.0f);
    static std::unique_ptr<AdvancedParticleEmitter> createSmoke(const Math::Vector2D& position,
                                                                float intensity = 1.0f);
    static std::unique_ptr<AdvancedParticleEmitter> createExplosion(const Math::Vector2D& position,
                                                                    float radius = 1.0f);

    // Nature
    static std::unique_ptr<AdvancedParticleEmitter> createRain(const Math::Vector2D& position,
                                                               float width = 10.0f);
    static std::unique_ptr<AdvancedParticleEmitter> createSnow(const Math::Vector2D& position,
                                                               float width = 10.0f);
    static std::unique_ptr<AdvancedParticleEmitter> createLeaves(const Math::Vector2D& position,
                                                                 float area = 5.0f);
    static std::unique_ptr<AdvancedParticleEmitter> createDust(const Math::Vector2D& position,
                                                               float intensity = 1.0f);

    // Magic/Effects
    static std::unique_ptr<AdvancedParticleEmitter> createSparkle(const Math::Vector2D& position,
                                                                  float intensity = 1.0f);
    static std::unique_ptr<AdvancedParticleEmitter> createMagicAura(const Math::Vector2D& position,
                                                                    float radius = 1.0f);
    static std::unique_ptr<AdvancedParticleEmitter> createPortal(const Math::Vector2D& position,
                                                                 float radius = 2.0f);

    // UI/Feedback
    static std::unique_ptr<AdvancedParticleEmitter> createConfetti(const Math::Vector2D& position,
                                                                   int count = 100);
    static std::unique_ptr<AdvancedParticleEmitter> createStars(const Math::Vector2D& position,
                                                                int count = 20);
    static std::unique_ptr<AdvancedParticleEmitter> createHeartBurst(const Math::Vector2D& position,
                                                                     int count = 30);
};

}  // namespace Particles
}  // namespace JJM

#endif  // PARTICLE_SYSTEM_H
