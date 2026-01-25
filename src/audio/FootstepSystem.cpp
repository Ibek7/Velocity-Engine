#include "audio/FootstepSystem.h"
#include "audio/AudioManager.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace JJM {
namespace Audio {

// Random number generation
static std::random_device rd;
static std::mt19937 gen(rd());

static float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(gen);
}

static int randomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

// FootstepSystem implementation
FootstepSystem::FootstepSystem()
    : m_audioManager(nullptr), m_masterVolume(1.0f), m_enabled(true),
      m_useDistanceAttenuation(true), m_maxAudibleDistance(50.0f),
      m_debugVisualization(false) {
    m_listenerPosition[0] = m_listenerPosition[1] = m_listenerPosition[2] = 0;
    initializeDefaultSurfaces();
}

FootstepSystem::~FootstepSystem() {
    shutdown();
}

void FootstepSystem::initialize(AudioManager* audioManager) {
    m_audioManager = audioManager;
}

void FootstepSystem::shutdown() {
    m_entities.clear();
    m_surfaceConfigs.clear();
}

void FootstepSystem::update(float deltaTime) {
    if (!m_enabled || !m_audioManager) return;
    
    m_stats.footstepsThisFrame = 0;
    m_stats.activeEntities = 0;
    m_stats.totalEntities = static_cast<int>(m_entities.size());
    
    for (auto& pair : m_entities) {
        Entity* entity = pair.first;
        EntityData& data = pair.second;
        
        if (data.params.velocity > data.params.speedThresholdWalk && data.params.isGrounded) {
            m_stats.activeEntities++;
        }
        
        updateEntity(entity, data, deltaTime);
    }
}

void FootstepSystem::registerEntity(Entity* entity, const FootstepParams& params) {
    EntityData data;
    data.params = params;
    data.timeSinceLastStep = 0.0f;
    data.nextStepInterval = calculateInterval(params, getSurfaceConfig(params.currentSurface));
    m_entities[entity] = data;
}

void FootstepSystem::unregisterEntity(Entity* entity) {
    m_entities.erase(entity);
}

void FootstepSystem::updateEntityParams(Entity* entity, const FootstepParams& params) {
    auto it = m_entities.find(entity);
    if (it != m_entities.end()) {
        it->second.params = params;
    }
}

const FootstepParams* FootstepSystem::getEntityParams(Entity* entity) const {
    auto it = m_entities.find(entity);
    return (it != m_entities.end()) ? &it->second.params : nullptr;
}

void FootstepSystem::configureSurface(SurfaceMaterial material, const SurfaceAudioConfig& config) {
    m_surfaceConfigs[material] = config;
}

const SurfaceAudioConfig& FootstepSystem::getSurfaceConfig(SurfaceMaterial material) const {
    auto it = m_surfaceConfigs.find(material);
    if (it != m_surfaceConfigs.end()) {
        return it->second;
    }
    
    // Return default concrete config if not found
    static SurfaceAudioConfig defaultConfig;
    return defaultConfig;
}

bool FootstepSystem::loadSurfaceConfigs(const std::string& filepath) {
    // TODO: Implement JSON/XML loading
    return false;
}

void FootstepSystem::triggerFootstep(const FootstepEvent& event) {
    if (!m_enabled || !m_audioManager) return;
    
    const SurfaceAudioConfig& config = getSurfaceConfig(event.material);
    
    std::vector<std::string> soundPool;
    switch (event.intensity) {
        case FootstepIntensity::WALK:
            soundPool = config.walkSounds;
            break;
        case FootstepIntensity::RUN:
            soundPool = config.runSounds;
            break;
        case FootstepIntensity::CROUCH:
            soundPool = config.crouchSounds;
            break;
        case FootstepIntensity::JUMP:
            soundPool = config.landSounds;
            break;
        case FootstepIntensity::SLIDE:
            soundPool = config.slideSounds;
            break;
    }
    
    if (!soundPool.empty()) {
        std::string sound = selectSound(soundPool);
        
        float volume = event.volume * m_masterVolume * config.volumeMultiplier;
        float pitch = event.pitch;
        
        // TODO: Play sound through audio manager
        // m_audioManager->playSound3D(sound, event.position, volume, pitch);
        
        if (m_footstepCallback) {
            m_footstepCallback(event);
        }
        
        m_stats.totalFootsteps++;
        m_stats.footstepsThisFrame++;
    }
}

void FootstepSystem::triggerLanding(Entity* entity, float impactVelocity) {
    auto it = m_entities.find(entity);
    if (it == m_entities.end()) return;
    
    const FootstepParams& params = it->second.params;
    
    FootstepEvent event;
    event.entity = entity;
    event.material = params.currentSurface;
    event.intensity = FootstepIntensity::JUMP;
    event.velocity = impactVelocity;
    event.volume = std::min(1.0f, impactVelocity / 10.0f); // Scale with impact
    event.pitch = 1.0f;
    
    // TODO: Get entity position
    event.position[0] = event.position[1] = event.position[2] = 0;
    
    triggerFootstep(event);
}

void FootstepSystem::triggerSlide(Entity* entity, float slideVelocity) {
    auto it = m_entities.find(entity);
    if (it == m_entities.end()) return;
    
    const FootstepParams& params = it->second.params;
    
    FootstepEvent event;
    event.entity = entity;
    event.material = params.currentSurface;
    event.intensity = FootstepIntensity::SLIDE;
    event.velocity = slideVelocity;
    event.volume = 0.8f;
    event.pitch = 1.0f;
    
    // TODO: Get entity position
    event.position[0] = event.position[1] = event.position[2] = 0;
    
    triggerFootstep(event);
}

void FootstepSystem::setFootstepCallback(std::function<void(const FootstepEvent&)> callback) {
    m_footstepCallback = callback;
}

void FootstepSystem::setMasterVolume(float volume) {
    m_masterVolume = std::max(0.0f, std::min(1.0f, volume));
}

void FootstepSystem::setListenerPosition(const float position[3]) {
    m_listenerPosition[0] = position[0];
    m_listenerPosition[1] = position[1];
    m_listenerPosition[2] = position[2];
}

void FootstepSystem::renderDebug() {
    // TODO: Render debug visualization
}

FootstepSystem::Stats FootstepSystem::getStatistics() const {
    return m_stats;
}

void FootstepSystem::updateEntity(Entity* entity, EntityData& data, float deltaTime) {
    const FootstepParams& params = data.params;
    
    // Only generate footsteps if moving on ground
    if (!params.isGrounded || params.velocity < params.speedThresholdWalk) {
        data.timeSinceLastStep = 0.0f;
        return;
    }
    
    data.timeSinceLastStep += deltaTime;
    
    if (data.timeSinceLastStep >= data.nextStepInterval) {
        playFootstepSound(entity, data);
        data.timeSinceLastStep = 0.0f;
        data.nextStepInterval = calculateInterval(params, getSurfaceConfig(params.currentSurface));
        data.leftFoot = !data.leftFoot;
        data.footstepCount++;
    }
}

void FootstepSystem::playFootstepSound(Entity* entity, const EntityData& data) {
    const FootstepParams& params = data.params;
    const SurfaceAudioConfig& config = getSurfaceConfig(params.currentSurface);
    
    FootstepEvent event;
    event.entity = entity;
    event.material = params.currentSurface;
    event.intensity = determineIntensity(params);
    event.velocity = params.velocity;
    
    // TODO: Get actual entity position
    event.position[0] = event.position[1] = event.position[2] = 0;
    
    event.volume = calculateVolume(params, event.position);
    event.pitch = calculatePitch(config);
    
    triggerFootstep(event);
}

FootstepIntensity FootstepSystem::determineIntensity(const FootstepParams& params) const {
    if (params.isCrouching) {
        return FootstepIntensity::CROUCH;
    } else if (params.velocity >= params.speedThresholdRun) {
        return FootstepIntensity::RUN;
    } else {
        return FootstepIntensity::WALK;
    }
}

float FootstepSystem::calculateVolume(const FootstepParams& params, const float position[3]) const {
    float volume = params.baseVolume;
    
    // Reduce volume when crouching
    if (params.isCrouching) {
        volume *= 0.5f;
    }
    
    // Distance attenuation
    if (m_useDistanceAttenuation) {
        float dx = position[0] - m_listenerPosition[0];
        float dy = position[1] - m_listenerPosition[1];
        float dz = position[2] - m_listenerPosition[2];
        float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (distance > m_maxAudibleDistance) {
            return 0.0f;
        }
        
        float attenuation = 1.0f - (distance / m_maxAudibleDistance);
        volume *= attenuation;
    }
    
    return std::max(0.0f, std::min(1.0f, volume));
}

float FootstepSystem::calculatePitch(const SurfaceAudioConfig& config) const {
    float variation = randomFloat(-config.pitchVariation, config.pitchVariation);
    return 1.0f + variation;
}

float FootstepSystem::calculateInterval(const FootstepParams& params,
                                        const SurfaceAudioConfig& config) const {
    // Base interval depends on speed
    float baseInterval;
    if (params.isCrouching) {
        baseInterval = config.maxInterval * 1.5f;
    } else if (params.velocity >= params.speedThresholdRun) {
        baseInterval = config.minInterval;
    } else {
        // Interpolate between min and max based on velocity
        float t = (params.velocity - params.speedThresholdWalk) / 
                  (params.speedThresholdRun - params.speedThresholdWalk);
        t = std::max(0.0f, std::min(1.0f, t));
        baseInterval = config.maxInterval + t * (config.minInterval - config.maxInterval);
    }
    
    // Add some random variation
    float variation = randomFloat(-0.05f, 0.05f);
    return baseInterval * (1.0f + variation);
}

std::string FootstepSystem::selectSound(const std::vector<std::string>& sounds) const {
    if (sounds.empty()) return "";
    int index = randomInt(0, static_cast<int>(sounds.size()) - 1);
    return sounds[index];
}

void FootstepSystem::initializeDefaultSurfaces() {
    // Concrete
    SurfaceAudioConfig concrete;
    concrete.walkSounds = {"footstep_concrete_walk_01.wav", "footstep_concrete_walk_02.wav"};
    concrete.runSounds = {"footstep_concrete_run_01.wav", "footstep_concrete_run_02.wav"};
    concrete.crouchSounds = {"footstep_concrete_crouch_01.wav"};
    concrete.landSounds = {"footstep_concrete_land.wav"};
    concrete.volumeMultiplier = 1.0f;
    concrete.pitchVariation = 0.1f;
    concrete.minInterval = 0.3f;
    concrete.maxInterval = 0.6f;
    m_surfaceConfigs[SurfaceMaterial::CONCRETE] = concrete;
    
    // Wood
    SurfaceAudioConfig wood;
    wood.walkSounds = {"footstep_wood_walk_01.wav", "footstep_wood_walk_02.wav"};
    wood.runSounds = {"footstep_wood_run_01.wav", "footstep_wood_run_02.wav"};
    wood.volumeMultiplier = 0.9f;
    wood.pitchVariation = 0.15f;
    wood.minInterval = 0.3f;
    wood.maxInterval = 0.6f;
    m_surfaceConfigs[SurfaceMaterial::WOOD] = wood;
    
    // Metal
    SurfaceAudioConfig metal;
    metal.walkSounds = {"footstep_metal_walk_01.wav", "footstep_metal_walk_02.wav"};
    metal.runSounds = {"footstep_metal_run_01.wav", "footstep_metal_run_02.wav"};
    metal.volumeMultiplier = 1.1f;
    metal.pitchVariation = 0.08f;
    metal.minInterval = 0.28f;
    metal.maxInterval = 0.58f;
    m_surfaceConfigs[SurfaceMaterial::METAL] = metal;
    
    // Grass
    SurfaceAudioConfig grass;
    grass.walkSounds = {"footstep_grass_walk_01.wav", "footstep_grass_walk_02.wav"};
    grass.runSounds = {"footstep_grass_run_01.wav", "footstep_grass_run_02.wav"};
    grass.volumeMultiplier = 0.7f;
    grass.pitchVariation = 0.12f;
    grass.minInterval = 0.32f;
    grass.maxInterval = 0.65f;
    m_surfaceConfigs[SurfaceMaterial::GRASS] = grass;
    
    // Gravel
    SurfaceAudioConfig gravel;
    gravel.walkSounds = {"footstep_gravel_walk_01.wav", "footstep_gravel_walk_02.wav"};
    gravel.runSounds = {"footstep_gravel_run_01.wav", "footstep_gravel_run_02.wav"};
    gravel.volumeMultiplier = 1.2f;
    gravel.pitchVariation = 0.2f;
    gravel.minInterval = 0.25f;
    gravel.maxInterval = 0.55f;
    m_surfaceConfigs[SurfaceMaterial::GRAVEL] = gravel;
    
    // Water
    SurfaceAudioConfig water;
    water.walkSounds = {"footstep_water_walk_01.wav", "footstep_water_walk_02.wav"};
    water.runSounds = {"footstep_water_run_01.wav", "footstep_water_run_02.wav"};
    water.volumeMultiplier = 0.8f;
    water.pitchVariation = 0.15f;
    water.minInterval = 0.35f;
    water.maxInterval = 0.7f;
    m_surfaceConfigs[SurfaceMaterial::WATER] = water;
    
    // Snow
    SurfaceAudioConfig snow;
    snow.walkSounds = {"footstep_snow_walk_01.wav", "footstep_snow_walk_02.wav"};
    snow.runSounds = {"footstep_snow_run_01.wav", "footstep_snow_run_02.wav"};
    snow.volumeMultiplier = 0.6f;
    snow.pitchVariation = 0.1f;
    snow.minInterval = 0.4f;
    snow.maxInterval = 0.75f;
    m_surfaceConfigs[SurfaceMaterial::SNOW] = snow;
    
    // Mud
    SurfaceAudioConfig mud;
    mud.walkSounds = {"footstep_mud_walk_01.wav", "footstep_mud_walk_02.wav"};
    mud.runSounds = {"footstep_mud_run_01.wav", "footstep_mud_run_02.wav"};
    mud.volumeMultiplier = 0.75f;
    mud.pitchVariation = 0.12f;
    mud.minInterval = 0.38f;
    mud.maxInterval = 0.72f;
    m_surfaceConfigs[SurfaceMaterial::MUD] = mud;
    
    // Sand
    SurfaceAudioConfig sand;
    sand.walkSounds = {"footstep_sand_walk_01.wav", "footstep_sand_walk_02.wav"};
    sand.runSounds = {"footstep_sand_run_01.wav", "footstep_sand_run_02.wav"};
    sand.volumeMultiplier = 0.65f;
    sand.pitchVariation = 0.1f;
    sand.minInterval = 0.35f;
    sand.maxInterval = 0.68f;
    m_surfaceConfigs[SurfaceMaterial::SAND] = sand;
    
    // Carpet
    SurfaceAudioConfig carpet;
    carpet.walkSounds = {"footstep_carpet_walk_01.wav", "footstep_carpet_walk_02.wav"};
    carpet.runSounds = {"footstep_carpet_run_01.wav", "footstep_carpet_run_02.wav"};
    carpet.volumeMultiplier = 0.4f;
    carpet.pitchVariation = 0.08f;
    carpet.minInterval = 0.35f;
    carpet.maxInterval = 0.65f;
    m_surfaceConfigs[SurfaceMaterial::CARPET] = carpet;
    
    // Tile
    SurfaceAudioConfig tile;
    tile.walkSounds = {"footstep_tile_walk_01.wav", "footstep_tile_walk_02.wav"};
    tile.runSounds = {"footstep_tile_run_01.wav", "footstep_tile_run_02.wav"};
    tile.volumeMultiplier = 1.0f;
    tile.pitchVariation = 0.1f;
    tile.minInterval = 0.3f;
    tile.maxInterval = 0.6f;
    m_surfaceConfigs[SurfaceMaterial::TILE] = tile;
}

// SurfaceDetection implementation
namespace SurfaceDetection {
    static std::unordered_map<std::string, SurfaceMaterial> materialMappings;
    
    SurfaceMaterial detectSurfaceMaterial(const float position[3]) {
        // TODO: Implement raycast or physics query to detect surface
        return SurfaceMaterial::CONCRETE;
    }
    
    void registerMaterialMapping(const std::string& materialName, SurfaceMaterial surfaceType) {
        materialMappings[materialName] = surfaceType;
    }
}

} // namespace Audio
} // namespace JJM
