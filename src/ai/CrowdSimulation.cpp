#include "ai/CrowdSimulation.h"
#include <cmath>
#include <algorithm>

namespace Engine {

CrowdSimulation::CrowdSimulation()
    : m_separationWeight(1.5f)
    , m_alignmentWeight(1.0f)
    , m_cohesionWeight(1.0f)
    , m_neighborRadius(5.0f)
    , m_nextAgentId(0)
{
}

CrowdSimulation& CrowdSimulation::getInstance() {
    static CrowdSimulation instance;
    return instance;
}

int CrowdSimulation::addAgent(float x, float y, float z, float radius) {
    CrowdAgent agent;
    agent.position[0] = x;
    agent.position[1] = y;
    agent.position[2] = z;
    agent.velocity[0] = 0.0f;
    agent.velocity[1] = 0.0f;
    agent.velocity[2] = 0.0f;
    agent.radius = radius;
    agent.maxSpeed = 2.0f;
    agent.maxForce = 5.0f;
    agent.groupId = 0;
    agent.active = true;
    
    m_agents.push_back(agent);
    return m_nextAgentId++;
}

void CrowdSimulation::removeAgent(int agentId) {
    if (agentId >= 0 && agentId < static_cast<int>(m_agents.size())) {
        m_agents[agentId].active = false;
    }
}

void CrowdSimulation::clearAgents() {
    m_agents.clear();
    m_nextAgentId = 0;
}

void CrowdSimulation::setAgentTarget(int agentId, float x, float y, float z) {
    if (agentId >= 0 && agentId < static_cast<int>(m_agents.size())) {
        auto& agent = m_agents[agentId];
        
        // Calculate direction to target
        float dx = x - agent.position[0];
        float dy = y - agent.position[1];
        float dz = z - agent.position[2];
        
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (dist > 0.001f) {
            agent.velocity[0] = (dx / dist) * agent.maxSpeed;
            agent.velocity[1] = (dy / dist) * agent.maxSpeed;
            agent.velocity[2] = (dz / dist) * agent.maxSpeed;
        }
    }
}

void CrowdSimulation::setAgentVelocity(int agentId, float vx, float vy, float vz) {
    if (agentId >= 0 && agentId < static_cast<int>(m_agents.size())) {
        auto& agent = m_agents[agentId];
        agent.velocity[0] = vx;
        agent.velocity[1] = vy;
        agent.velocity[2] = vz;
    }
}

void CrowdSimulation::setAgentGroup(int agentId, int groupId) {
    if (agentId >= 0 && agentId < static_cast<int>(m_agents.size())) {
        m_agents[agentId].groupId = groupId;
    }
}

void CrowdSimulation::getAgentPosition(int agentId, float& x, float& y, float& z) const {
    if (agentId >= 0 && agentId < static_cast<int>(m_agents.size())) {
        const auto& agent = m_agents[agentId];
        x = agent.position[0];
        y = agent.position[1];
        z = agent.position[2];
    }
}

void CrowdSimulation::update(float deltaTime) {
    for (auto& agent : m_agents) {
        if (agent.active) {
            updateAgent(agent, deltaTime);
        }
    }
}

void CrowdSimulation::updateAgent(CrowdAgent& agent, float deltaTime) {
    float steering[3] = {0.0f, 0.0f, 0.0f};
    
    // Apply flocking behaviors
    float separation[3] = {0.0f, 0.0f, 0.0f};
    applySeparation(agent, separation);
    
    float alignment[3] = {0.0f, 0.0f, 0.0f};
    applyAlignment(agent, alignment);
    
    float cohesion[3] = {0.0f, 0.0f, 0.0f};
    applyCohesion(agent, cohesion);
    
    float avoidance[3] = {0.0f, 0.0f, 0.0f};
    applyAvoidance(agent, avoidance);
    
    // Combine forces
    steering[0] += separation[0] * m_separationWeight;
    steering[1] += separation[1] * m_separationWeight;
    steering[2] += separation[2] * m_separationWeight;
    
    steering[0] += alignment[0] * m_alignmentWeight;
    steering[1] += alignment[1] * m_alignmentWeight;
    steering[2] += alignment[2] * m_alignmentWeight;
    
    steering[0] += cohesion[0] * m_cohesionWeight;
    steering[1] += cohesion[1] * m_cohesionWeight;
    steering[2] += cohesion[2] * m_cohesionWeight;
    
    steering[0] += avoidance[0];
    steering[1] += avoidance[1];
    steering[2] += avoidance[2];
    
    // Limit steering force
    float forceMag = std::sqrt(steering[0]*steering[0] + steering[1]*steering[1] + steering[2]*steering[2]);
    if (forceMag > agent.maxForce) {
        steering[0] = (steering[0] / forceMag) * agent.maxForce;
        steering[1] = (steering[1] / forceMag) * agent.maxForce;
        steering[2] = (steering[2] / forceMag) * agent.maxForce;
    }
    
    // Update velocity
    agent.velocity[0] += steering[0] * deltaTime;
    agent.velocity[1] += steering[1] * deltaTime;
    agent.velocity[2] += steering[2] * deltaTime;
    
    // Limit speed
    float speed = std::sqrt(agent.velocity[0]*agent.velocity[0] + 
                           agent.velocity[1]*agent.velocity[1] + 
                           agent.velocity[2]*agent.velocity[2]);
    if (speed > agent.maxSpeed) {
        agent.velocity[0] = (agent.velocity[0] / speed) * agent.maxSpeed;
        agent.velocity[1] = (agent.velocity[1] / speed) * agent.maxSpeed;
        agent.velocity[2] = (agent.velocity[2] / speed) * agent.maxSpeed;
    }
    
    // Update position
    agent.position[0] += agent.velocity[0] * deltaTime;
    agent.position[1] += agent.velocity[1] * deltaTime;
    agent.position[2] += agent.velocity[2] * deltaTime;
}

void CrowdSimulation::applySeparation(CrowdAgent& agent, float force[3]) {
    std::vector<CrowdAgent*> neighbors;
    getNeighbors(agent, neighbors);
    
    if (neighbors.empty()) return;
    
    force[0] = 0.0f;
    force[1] = 0.0f;
    force[2] = 0.0f;
    
    for (auto* neighbor : neighbors) {
        float dx = agent.position[0] - neighbor->position[0];
        float dy = agent.position[1] - neighbor->position[1];
        float dz = agent.position[2] - neighbor->position[2];
        
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (dist > 0.001f && dist < agent.radius + neighbor->radius + 1.0f) {
            // Repel stronger when closer
            float weight = 1.0f / (dist * dist);
            force[0] += (dx / dist) * weight;
            force[1] += (dy / dist) * weight;
            force[2] += (dz / dist) * weight;
        }
    }
}

void CrowdSimulation::applyAlignment(CrowdAgent& agent, float force[3]) {
    std::vector<CrowdAgent*> neighbors;
    getNeighbors(agent, neighbors);
    
    if (neighbors.empty()) return;
    
    force[0] = 0.0f;
    force[1] = 0.0f;
    force[2] = 0.0f;
    
    for (auto* neighbor : neighbors) {
        force[0] += neighbor->velocity[0];
        force[1] += neighbor->velocity[1];
        force[2] += neighbor->velocity[2];
    }
    
    force[0] /= neighbors.size();
    force[1] /= neighbors.size();
    force[2] /= neighbors.size();
    
    // Steer towards average velocity
    force[0] -= agent.velocity[0];
    force[1] -= agent.velocity[1];
    force[2] -= agent.velocity[2];
}

void CrowdSimulation::applyCohesion(CrowdAgent& agent, float force[3]) {
    std::vector<CrowdAgent*> neighbors;
    getNeighbors(agent, neighbors);
    
    if (neighbors.empty()) return;
    
    float centerX = 0.0f, centerY = 0.0f, centerZ = 0.0f;
    
    for (auto* neighbor : neighbors) {
        centerX += neighbor->position[0];
        centerY += neighbor->position[1];
        centerZ += neighbor->position[2];
    }
    
    centerX /= neighbors.size();
    centerY /= neighbors.size();
    centerZ /= neighbors.size();
    
    // Steer towards center of mass
    force[0] = centerX - agent.position[0];
    force[1] = centerY - agent.position[1];
    force[2] = centerZ - agent.position[2];
}

void CrowdSimulation::applyAvoidance(CrowdAgent& agent, float force[3]) {
    // TODO: Obstacle avoidance
    force[0] = 0.0f;
    force[1] = 0.0f;
    force[2] = 0.0f;
}

void CrowdSimulation::getNeighbors(const CrowdAgent& agent, std::vector<CrowdAgent*>& neighbors) {
    neighbors.clear();
    
    for (auto& other : m_agents) {
        if (!other.active || &other == &agent) continue;
        if (agent.groupId >= 0 && other.groupId != agent.groupId) continue;
        
        float dx = other.position[0] - agent.position[0];
        float dy = other.position[1] - agent.position[1];
        float dz = other.position[2] - agent.position[2];
        
        float distSq = dx*dx + dy*dy + dz*dz;
        if (distSq < m_neighborRadius * m_neighborRadius) {
            neighbors.push_back(const_cast<CrowdAgent*>(&other));
        }
    }
}

void CrowdSimulation::getAgentsInRadius(float x, float y, float z, float radius, std::vector<int>& results) const {
    results.clear();
    float radiusSq = radius * radius;
    
    for (size_t i = 0; i < m_agents.size(); ++i) {
        if (!m_agents[i].active) continue;
        
        float dx = m_agents[i].position[0] - x;
        float dy = m_agents[i].position[1] - y;
        float dz = m_agents[i].position[2] - z;
        
        float distSq = dx*dx + dy*dy + dz*dz;
        if (distSq <= radiusSq) {
            results.push_back(static_cast<int>(i));
        }
    }
}

} // namespace Engine
