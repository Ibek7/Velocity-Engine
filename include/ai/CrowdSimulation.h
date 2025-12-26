#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Crowd simulation system with flocking behavior
namespace Engine {

struct CrowdAgent {
    float position[3];
    float velocity[3];
    float radius;
    float maxSpeed;
    float maxForce;
    int groupId;
    bool active;
};

class CrowdSimulation {
public:
    static CrowdSimulation& getInstance();

    // Agent management
    int addAgent(float x, float y, float z, float radius = 0.5f);
    void removeAgent(int agentId);
    void clearAgents();
    
    // Agent control
    void setAgentTarget(int agentId, float x, float y, float z);
    void setAgentVelocity(int agentId, float vx, float vy, float vz);
    void setAgentGroup(int agentId, int groupId);
    void getAgentPosition(int agentId, float& x, float& y, float& z) const;
    
    // Behavior parameters
    void setSeparationWeight(float weight) { m_separationWeight = weight; }
    void setAlignmentWeight(float weight) { m_alignmentWeight = weight; }
    void setCohesionWeight(float weight) { m_cohesionWeight = weight; }
    void setNeighborRadius(float radius) { m_neighborRadius = radius; }
    
    // Update
    void update(float deltaTime);
    
    // Query
    int getAgentCount() const { return m_agents.size(); }
    void getAgentsInRadius(float x, float y, float z, float radius, std::vector<int>& results) const;
    
private:
    CrowdSimulation();
    CrowdSimulation(const CrowdSimulation&) = delete;
    CrowdSimulation& operator=(const CrowdSimulation&) = delete;

    void updateAgent(CrowdAgent& agent, float deltaTime);
    void applySeparation(CrowdAgent& agent, float force[3]);
    void applyAlignment(CrowdAgent& agent, float force[3]);
    void applyCohesion(CrowdAgent& agent, float force[3]);
    void applyAvoidance(CrowdAgent& agent, float force[3]);
    void getNeighbors(const CrowdAgent& agent, std::vector<CrowdAgent*>& neighbors);

    std::vector<CrowdAgent> m_agents;
    
    float m_separationWeight;
    float m_alignmentWeight;
    float m_cohesionWeight;
    float m_neighborRadius;
    
    int m_nextAgentId;
};

} // namespace Engine
