#include "ai/CrowdSimulationSystem.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace AI {

// Helper functions
static float vectorLength(const float v[3]) {
    return std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

static void normalize(float v[3]) {
    float len = vectorLength(v);
    if (len > 0.0001f) {
        v[0] /= len; v[1] /= len; v[2] /= len;
    }
}

static float distance(const float a[3], const float b[3]) {
    float dx = b[0] - a[0];
    float dy = b[1] - a[1];
    float dz = b[2] - a[2];
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

// CrowdAgent implementation
CrowdAgent::CrowdAgent(Entity* entity)
    : m_entity(entity), m_state(CrowdAgentState::IDLE),
      m_hasTarget(false), m_currentWaypoint(0), m_enabled(true) {
    m_position[0] = m_position[1] = m_position[2] = 0;
    m_velocity[0] = m_velocity[1] = m_velocity[2] = 0;
    m_target[0] = m_target[1] = m_target[2] = 0;
}

CrowdAgent::~CrowdAgent() {
}

void CrowdAgent::setPosition(const float pos[3]) {
    m_position[0] = pos[0];
    m_position[1] = pos[1];
    m_position[2] = pos[2];
}

void CrowdAgent::getPosition(float outPos[3]) const {
    outPos[0] = m_position[0];
    outPos[1] = m_position[1];
    outPos[2] = m_position[2];
}

void CrowdAgent::setVelocity(const float vel[3]) {
    m_velocity[0] = vel[0];
    m_velocity[1] = vel[1];
    m_velocity[2] = vel[2];
}

void CrowdAgent::getVelocity(float outVel[3]) const {
    outVel[0] = m_velocity[0];
    outVel[1] = m_velocity[1];
    outVel[2] = m_velocity[2];
}

void CrowdAgent::setTarget(const float target[3]) {
    m_target[0] = target[0];
    m_target[1] = target[1];
    m_target[2] = target[2];
    m_hasTarget = true;
}

void CrowdAgent::clearTarget() {
    m_hasTarget = false;
}

void CrowdAgent::setPath(const std::vector<float>& waypoints) {
    m_path = waypoints;
    m_currentWaypoint = 0;
}

void CrowdAgent::addNeighbor(CrowdAgent* agent) {
    if (m_neighbors.size() < static_cast<size_t>(m_properties.maxNeighbors)) {
        m_neighbors.push_back(agent);
    }
}

void CrowdAgent::clearNeighbors() {
    m_neighbors.clear();
}

void CrowdAgent::calculateSteering(float outSteering[3]) {
    outSteering[0] = outSteering[1] = outSteering[2] = 0;
    
    float separation[3] = {0, 0, 0};
    float alignment[3] = {0, 0, 0};
    float cohesion[3] = {0, 0, 0};
    float avoidance[3] = {0, 0, 0};
    float pathFollowing[3] = {0, 0, 0};
    
    calculateSeparation(separation);
    calculateAlignment(alignment);
    calculateCohesion(cohesion);
    calculateAvoidance(avoidance);
    
    if (m_properties.usePathFollowing && !m_path.empty()) {
        calculatePathFollowing(pathFollowing);
    }
    
    // Combine steering forces
    outSteering[0] += separation[0] * m_properties.separationWeight;
    outSteering[1] += separation[1] * m_properties.separationWeight;
    outSteering[2] += separation[2] * m_properties.separationWeight;
    
    outSteering[0] += alignment[0] * m_properties.alignmentWeight;
    outSteering[1] += alignment[1] * m_properties.alignmentWeight;
    outSteering[2] += alignment[2] * m_properties.alignmentWeight;
    
    outSteering[0] += cohesion[0] * m_properties.cohesionWeight;
    outSteering[1] += cohesion[1] * m_properties.cohesionWeight;
    outSteering[2] += cohesion[2] * m_properties.cohesionWeight;
    
    outSteering[0] += avoidance[0];
    outSteering[1] += avoidance[1];
    outSteering[2] += avoidance[2];
    
    outSteering[0] += pathFollowing[0];
    outSteering[1] += pathFollowing[1];
    outSteering[2] += pathFollowing[2];
}

void CrowdAgent::calculateSeparation(float outForce[3]) {
    outForce[0] = outForce[1] = outForce[2] = 0;
    
    for (CrowdAgent* neighbor : m_neighbors) {
        float neighborPos[3];
        neighbor->getPosition(neighborPos);
        
        float diff[3] = {
            m_position[0] - neighborPos[0],
            m_position[1] - neighborPos[1],
            m_position[2] - neighborPos[2]
        };
        
        float dist = vectorLength(diff);
        if (dist > 0 && dist < m_properties.avoidanceRadius) {
            normalize(diff);
            float weight = 1.0f - (dist / m_properties.avoidanceRadius);
            outForce[0] += diff[0] * weight;
            outForce[1] += diff[1] * weight;
            outForce[2] += diff[2] * weight;
        }
    }
}

void CrowdAgent::calculateAlignment(float outForce[3]) {
    outForce[0] = outForce[1] = outForce[2] = 0;
    
    if (m_neighbors.empty()) return;
    
    for (CrowdAgent* neighbor : m_neighbors) {
        float neighborVel[3];
        neighbor->getVelocity(neighborVel);
        outForce[0] += neighborVel[0];
        outForce[1] += neighborVel[1];
        outForce[2] += neighborVel[2];
    }
    
    outForce[0] /= m_neighbors.size();
    outForce[1] /= m_neighbors.size();
    outForce[2] /= m_neighbors.size();
    normalize(outForce);
}

void CrowdAgent::calculateCohesion(float outForce[3]) {
    outForce[0] = outForce[1] = outForce[2] = 0;
    
    if (m_neighbors.empty()) return;
    
    float center[3] = {0, 0, 0};
    for (CrowdAgent* neighbor : m_neighbors) {
        float neighborPos[3];
        neighbor->getPosition(neighborPos);
        center[0] += neighborPos[0];
        center[1] += neighborPos[1];
        center[2] += neighborPos[2];
    }
    
    center[0] /= m_neighbors.size();
    center[1] /= m_neighbors.size();
    center[2] /= m_neighbors.size();
    
    outForce[0] = center[0] - m_position[0];
    outForce[1] = center[1] - m_position[1];
    outForce[2] = center[2] - m_position[2];
    normalize(outForce);
}

void CrowdAgent::calculateAvoidance(float outForce[3]) {
    outForce[0] = outForce[1] = outForce[2] = 0;
    // TODO: Implement RVO (Reciprocal Velocity Obstacles)
}

void CrowdAgent::calculatePathFollowing(float outForce[3]) {
    outForce[0] = outForce[1] = outForce[2] = 0;
    
    if (m_path.empty() || m_currentWaypoint * 3 >= static_cast<int>(m_path.size())) {
        return;
    }
    
    float waypoint[3] = {
        m_path[m_currentWaypoint * 3],
        m_path[m_currentWaypoint * 3 + 1],
        m_path[m_currentWaypoint * 3 + 2]
    };
    
    float toWaypoint[3] = {
        waypoint[0] - m_position[0],
        waypoint[1] - m_position[1],
        waypoint[2] - m_position[2]
    };
    
    float dist = vectorLength(toWaypoint);
    if (dist < m_properties.pathOptimizationRange) {
        m_currentWaypoint++;
        return;
    }
    
    normalize(toWaypoint);
    outForce[0] = toWaypoint[0];
    outForce[1] = toWaypoint[1];
    outForce[2] = toWaypoint[2];
}

void CrowdAgent::update(float deltaTime) {
    if (!m_enabled) return;
    
    float steering[3];
    calculateSteering(steering);
    
    // Apply acceleration
    m_velocity[0] += steering[0] * m_properties.maxAcceleration * deltaTime;
    m_velocity[1] += steering[1] * m_properties.maxAcceleration * deltaTime;
    m_velocity[2] += steering[2] * m_properties.maxAcceleration * deltaTime;
    
    // Limit speed
    float speed = vectorLength(m_velocity);
    if (speed > m_properties.maxSpeed) {
        float scale = m_properties.maxSpeed / speed;
        m_velocity[0] *= scale;
        m_velocity[1] *= scale;
        m_velocity[2] *= scale;
    }
    
    // Update position
    m_position[0] += m_velocity[0] * deltaTime;
    m_position[1] += m_velocity[1] * deltaTime;
    m_position[2] += m_velocity[2] * deltaTime;
    
    // Update state based on speed
    if (speed < 0.1f) {
        m_state = CrowdAgentState::IDLE;
    } else if (speed < m_properties.maxSpeed * 0.5f) {
        m_state = CrowdAgentState::WALKING;
    } else {
        m_state = CrowdAgentState::RUNNING;
    }
}

// CrowdGrid implementation
CrowdGrid::CrowdGrid(float cellSize) : m_cellSize(cellSize) {
}

CrowdGrid::~CrowdGrid() {
}

void CrowdGrid::clear() {
    m_cells.clear();
}

void CrowdGrid::insert(CrowdAgent* agent) {
    float pos[3];
    agent->getPosition(pos);
    uint64_t key = getCellKey(pos[0], pos[2]);
    m_cells[key].agents.push_back(agent);
}

std::vector<CrowdAgent*> CrowdGrid::queryRadius(const float position[3], float radius) {
    std::vector<CrowdAgent*> result;
    int cellRadius = static_cast<int>(std::ceil(radius / m_cellSize));
    
    for (int dx = -cellRadius; dx <= cellRadius; ++dx) {
        for (int dz = -cellRadius; dz <= cellRadius; ++dz) {
            float cellX = position[0] + dx * m_cellSize;
            float cellZ = position[2] + dz * m_cellSize;
            uint64_t key = getCellKey(cellX, cellZ);
            
            auto it = m_cells.find(key);
            if (it != m_cells.end()) {
                for (CrowdAgent* agent : it->second.agents) {
                    float agentPos[3];
                    agent->getPosition(agentPos);
                    if (distance(position, agentPos) <= radius) {
                        result.push_back(agent);
                    }
                }
            }
        }
    }
    
    return result;
}

uint64_t CrowdGrid::getCellKey(float x, float z) const {
    int32_t cellX = static_cast<int32_t>(std::floor(x / m_cellSize));
    int32_t cellZ = static_cast<int32_t>(std::floor(z / m_cellSize));
    return (static_cast<uint64_t>(cellX) << 32) | static_cast<uint64_t>(cellZ);
}

// CrowdSimulationSystem implementation
CrowdSimulationSystem::CrowdSimulationSystem()
    : m_grid(nullptr), m_navmesh(nullptr), m_maxAgents(1000),
      m_updateFrequency(30.0f), m_timeSinceLastUpdate(0.0f),
      m_useMultithreading(false), m_maxNeighborChecks(100),
      m_neighborCheckRadius(10.0f), m_debugVisualization(false) {
    m_grid = new CrowdGrid(5.0f);
    m_stats = Stats();
}

CrowdSimulationSystem::~CrowdSimulationSystem() {
    shutdown();
}

void CrowdSimulationSystem::initialize(NavigationMesh* navmesh) {
    m_navmesh = navmesh;
}

void CrowdSimulationSystem::shutdown() {
    for (auto* agent : m_agents) {
        delete agent;
    }
    m_agents.clear();
    
    for (auto* formation : m_formations) {
        delete formation;
    }
    m_formations.clear();
    
    if (m_grid) {
        delete m_grid;
        m_grid = nullptr;
    }
}

CrowdAgent* CrowdSimulationSystem::addAgent(Entity* entity, const CrowdAgentProperties& props) {
    if (m_agents.size() >= static_cast<size_t>(m_maxAgents)) {
        return nullptr;
    }
    
    CrowdAgent* agent = new CrowdAgent(entity);
    agent->setProperties(props);
    m_agents.push_back(agent);
    return agent;
}

void CrowdSimulationSystem::removeAgent(CrowdAgent* agent) {
    m_agents.erase(
        std::remove(m_agents.begin(), m_agents.end(), agent),
        m_agents.end()
    );
    delete agent;
}

void CrowdSimulationSystem::removeAllAgents() {
    for (auto* agent : m_agents) {
        delete agent;
    }
    m_agents.clear();
}

CrowdAgent* CrowdSimulationSystem::getAgent(Entity* entity) {
    for (auto* agent : m_agents) {
        if (agent->getEntity() == entity) {
            return agent;
        }
    }
    return nullptr;
}

void CrowdSimulationSystem::update(float deltaTime) {
    m_timeSinceLastUpdate += deltaTime;
    
    float updateInterval = 1.0f / m_updateFrequency;
    if (m_timeSinceLastUpdate < updateInterval) {
        return;
    }
    
    deltaTime = m_timeSinceLastUpdate;
    m_timeSinceLastUpdate = 0.0f;
    
    // Update spatial grid
    m_grid->clear();
    for (auto* agent : m_agents) {
        if (agent->isEnabled()) {
            m_grid->insert(agent);
        }
    }
    
    // Update neighbors
    updateNeighbors();
    
    // Update agent steering and positions
    for (auto* agent : m_agents) {
        if (agent->isEnabled()) {
            updateAgentSteering(agent, deltaTime);
            agent->update(deltaTime);
        }
    }
    
    // Resolve collisions
    resolveCollisions();
    
    // Update formations
    updateFormations();
    
    // Update statistics
    m_stats.totalAgents = static_cast<int>(m_agents.size());
    int active = 0;
    float totalSpeed = 0;
    for (auto* agent : m_agents) {
        if (agent->isEnabled()) {
            active++;
            float vel[3];
            agent->getVelocity(vel);
            totalSpeed += vectorLength(vel);
        }
    }
    m_stats.activeAgents = active;
    m_stats.averageSpeed = active > 0 ? totalSpeed / active : 0;
    m_stats.formationCount = static_cast<int>(m_formations.size());
}

void CrowdSimulationSystem::updateNeighbors() {
    for (auto* agent : m_agents) {
        if (!agent->isEnabled()) continue;
        
        agent->clearNeighbors();
        float pos[3];
        agent->getPosition(pos);
        
        auto nearby = m_grid->queryRadius(pos, m_neighborCheckRadius);
        for (auto* neighbor : nearby) {
            if (neighbor != agent && neighbor->isEnabled()) {
                agent->addNeighbor(neighbor);
            }
        }
    }
}

void CrowdSimulationSystem::updateAgentSteering(CrowdAgent* agent, float deltaTime) {
    // Steering is calculated in agent->update()
}

void CrowdSimulationSystem::resolveCollisions() {
    for (size_t i = 0; i < m_agents.size(); ++i) {
        if (!m_agents[i]->isEnabled()) continue;
        
        float pos1[3];
        m_agents[i]->getPosition(pos1);
        float r1 = m_agents[i]->getProperties().radius;
        
        for (size_t j = i + 1; j < m_agents.size(); ++j) {
            if (!m_agents[j]->isEnabled()) continue;
            
            float pos2[3];
            m_agents[j]->getPosition(pos2);
            float r2 = m_agents[j]->getProperties().radius;
            
            float dist = distance(pos1, pos2);
            float minDist = r1 + r2;
            
            if (dist < minDist && dist > 0.001f) {
                // Push apart
                float overlap = minDist - dist;
                float dir[3] = {
                    pos1[0] - pos2[0],
                    pos1[1] - pos2[1],
                    pos1[2] - pos2[2]
                };
                normalize(dir);
                
                float push = overlap * 0.5f;
                pos1[0] += dir[0] * push;
                pos1[1] += dir[1] * push;
                pos1[2] += dir[2] * push;
                
                pos2[0] -= dir[0] * push;
                pos2[1] -= dir[1] * push;
                pos2[2] -= dir[2] * push;
                
                m_agents[i]->setPosition(pos1);
                m_agents[j]->setPosition(pos2);
                
                if (m_onAgentCollision) {
                    m_onAgentCollision(m_agents[i], m_agents[j]);
                }
            }
        }
    }
}

void CrowdSimulationSystem::updateFormations() {
    for (auto* formation : m_formations) {
        updateFormation(formation);
    }
}

Formation* CrowdSimulationSystem::createFormation(FormationType type, const float position[3]) {
    Formation* formation = new Formation();
    formation->type = type;
    formation->position[0] = position[0];
    formation->position[1] = position[1];
    formation->position[2] = position[2];
    m_formations.push_back(formation);
    return formation;
}

void CrowdSimulationSystem::updateFormation(Formation* formation) {
    if (formation->agents.empty()) return;
    
    std::vector<float> positions;
    calculateFormationPositions(formation, positions);
    
    for (size_t i = 0; i < formation->agents.size() && i * 3 < positions.size(); ++i) {
        float target[3] = {
            positions[i * 3],
            positions[i * 3 + 1],
            positions[i * 3 + 2]
        };
        formation->agents[i]->setTarget(target);
    }
}

void CrowdSimulationSystem::calculateFormationPositions(Formation* formation,
                                                       std::vector<float>& positions) {
    positions.clear();
    int count = static_cast<int>(formation->agents.size());
    
    switch (formation->type) {
        case FormationType::LINE:
            for (int i = 0; i < count; ++i) {
                float offset = (i - count / 2.0f) * formation->spacing;
                positions.push_back(formation->position[0] + offset);
                positions.push_back(formation->position[1]);
                positions.push_back(formation->position[2]);
            }
            break;
        case FormationType::GRID:
            int cols = static_cast<int>(std::ceil(std::sqrt(count)));
            for (int i = 0; i < count; ++i) {
                int row = i / cols;
                int col = i % cols;
                positions.push_back(formation->position[0] + col * formation->spacing);
                positions.push_back(formation->position[1]);
                positions.push_back(formation->position[2] + row * formation->spacing);
            }
            break;
        default:
            break;
    }
}

void CrowdSimulationSystem::dissolveFormation(Formation* formation) {
    m_formations.erase(
        std::remove(m_formations.begin(), m_formations.end(), formation),
        m_formations.end()
    );
    delete formation;
}

void CrowdSimulationSystem::setSpatialGridSize(float size) {
    if (m_grid) {
        delete m_grid;
    }
    m_grid = new CrowdGrid(size);
}

CrowdSimulationSystem::Stats CrowdSimulationSystem::getStatistics() const {
    return m_stats;
}

void CrowdSimulationSystem::renderDebug() {
    // TODO: Render debug visualization
}

// Crowd presets
namespace CrowdPresets {
    CrowdAgentProperties getNormalCitizen() {
        CrowdAgentProperties props;
        props.maxSpeed = 2.5f;
        props.separationWeight = 1.0f;
        props.alignmentWeight = 0.5f;
        props.cohesionWeight = 0.3f;
        return props;
    }
    
    CrowdAgentProperties getPanickedCitizen() {
        CrowdAgentProperties props;
        props.maxSpeed = 5.0f;
        props.separationWeight = 2.0f;
        props.alignmentWeight = 0.2f;
        props.cohesionWeight = 0.1f;
        return props;
    }
    
    CrowdAgentProperties getSoldier() {
        CrowdAgentProperties props;
        props.maxSpeed = 3.5f;
        props.separationWeight = 0.8f;
        props.alignmentWeight = 1.0f;
        props.cohesionWeight = 0.8f;
        return props;
    }
    
    CrowdAgentProperties getZombie() {
        CrowdAgentProperties props;
        props.maxSpeed = 1.5f;
        props.separationWeight = 0.3f;
        props.alignmentWeight = 0.8f;
        props.cohesionWeight = 1.0f;
        return props;
    }
}

} // namespace AI
} // namespace JJM
