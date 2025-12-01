#ifndef AI_SYSTEM_H
#define AI_SYSTEM_H

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <queue>
#include <set>
#include <any>
#include "../math/Vector2D.h"
#include "../ecs/Entity.h"

namespace AI {

// Forward declarations
class BehaviorTree;
class StateMachine;
class Blackboard;
class AIAgent;

// =============================================================================
// Behavior Tree System
// =============================================================================

enum class NodeStatus {
    SUCCESS,
    FAILURE,
    RUNNING
};

// Base class for all behavior tree nodes
class BehaviorNode {
public:
    virtual ~BehaviorNode() = default;
    virtual NodeStatus tick(float deltaTime) = 0;
    virtual void reset() {}
    virtual std::string getName() const = 0;
};

// Composite nodes - have multiple children
class CompositeNode : public BehaviorNode {
protected:
    std::vector<std::shared_ptr<BehaviorNode>> children;
    size_t currentChild = 0;

public:
    void addChild(std::shared_ptr<BehaviorNode> child);
    const std::vector<std::shared_ptr<BehaviorNode>>& getChildren() const { return children; }
    void reset() override;
};

// Sequence node - executes children in order, fails if any child fails
class SequenceNode : public CompositeNode {
public:
    NodeStatus tick(float deltaTime) override;
    std::string getName() const override { return "Sequence"; }
};

// Selector node - executes children until one succeeds
class SelectorNode : public CompositeNode {
public:
    NodeStatus tick(float deltaTime) override;
    std::string getName() const override { return "Selector"; }
};

// Parallel node - executes all children simultaneously
class ParallelNode : public CompositeNode {
private:
    int successThreshold;
    int failureThreshold;

public:
    ParallelNode(int successThresh = 1, int failureThresh = 1);
    NodeStatus tick(float deltaTime) override;
    std::string getName() const override { return "Parallel"; }
    void reset() override;
};

// Decorator nodes - modify single child behavior
class DecoratorNode : public BehaviorNode {
protected:
    std::shared_ptr<BehaviorNode> child;

public:
    void setChild(std::shared_ptr<BehaviorNode> node) { child = node; }
    void reset() override;
};

// Inverter - inverts child result
class InverterNode : public DecoratorNode {
public:
    NodeStatus tick(float deltaTime) override;
    std::string getName() const override { return "Inverter"; }
};

// Repeater - repeats child N times or until failure
class RepeaterNode : public DecoratorNode {
private:
    int repeatCount;
    int currentCount = 0;

public:
    RepeaterNode(int count = -1); // -1 = infinite
    NodeStatus tick(float deltaTime) override;
    std::string getName() const override { return "Repeater"; }
    void reset() override;
};

// Succeeder - always returns success
class SucceederNode : public DecoratorNode {
public:
    NodeStatus tick(float deltaTime) override;
    std::string getName() const override { return "Succeeder"; }
};

// Leaf nodes - actual actions and conditions
using TickFunction = std::function<NodeStatus(float)>;

class ActionNode : public BehaviorNode {
private:
    std::string name;
    TickFunction action;

public:
    ActionNode(const std::string& name, TickFunction func);
    NodeStatus tick(float deltaTime) override;
    std::string getName() const override { return name; }
};

class ConditionNode : public BehaviorNode {
private:
    std::string name;
    std::function<bool()> condition;

public:
    ConditionNode(const std::string& name, std::function<bool()> cond);
    NodeStatus tick(float deltaTime) override;
    std::string getName() const override { return name; }
};

// Blackboard - shared memory for behavior tree
class Blackboard {
private:
    std::unordered_map<std::string, std::any> data;

public:
    template<typename T>
    void set(const std::string& key, const T& value) {
        data[key] = value;
    }

    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const {
        auto it = data.find(key);
        if (it != data.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    bool has(const std::string& key) const;
    void remove(const std::string& key);
    void clear();
};

// Behavior Tree
class BehaviorTree {
private:
    std::shared_ptr<BehaviorNode> root;
    Blackboard blackboard;
    std::string name;

public:
    BehaviorTree(const std::string& treeName = "BehaviorTree");
    
    void setRoot(std::shared_ptr<BehaviorNode> node) { root = node; }
    std::shared_ptr<BehaviorNode> getRoot() const { return root; }
    
    NodeStatus tick(float deltaTime);
    void reset();
    
    Blackboard& getBlackboard() { return blackboard; }
    const Blackboard& getBlackboard() const { return blackboard; }
    
    std::string getName() const { return name; }
};

// =============================================================================
// State Machine System
// =============================================================================

class State {
private:
    std::string name;
    std::function<void()> onEnter;
    std::function<void(float)> onUpdate;
    std::function<void()> onExit;

public:
    State(const std::string& stateName);
    
    void setOnEnter(std::function<void()> callback) { onEnter = callback; }
    void setOnUpdate(std::function<void(float)> callback) { onUpdate = callback; }
    void setOnExit(std::function<void()> callback) { onExit = callback; }
    
    void enter();
    void update(float deltaTime);
    void exit();
    
    std::string getName() const { return name; }
};

struct Transition {
    std::string fromState;
    std::string toState;
    std::function<bool()> condition;
    int priority = 0; // Higher priority transitions checked first
    
    Transition(const std::string& from, const std::string& to, 
               std::function<bool()> cond, int prio = 0);
};

class StateMachine {
private:
    std::unordered_map<std::string, std::shared_ptr<State>> states;
    std::vector<Transition> transitions;
    std::shared_ptr<State> currentState;
    std::shared_ptr<State> previousState;
    std::string name;
    
    // State history for debugging
    std::vector<std::string> stateHistory;
    size_t maxHistorySize = 10;

public:
    StateMachine(const std::string& machineName = "StateMachine");
    
    void addState(const std::string& stateName);
    void addTransition(const std::string& from, const std::string& to, 
                      std::function<bool()> condition, int priority = 0);
    
    void setState(const std::string& stateName);
    void update(float deltaTime);
    
    std::shared_ptr<State> getState(const std::string& stateName);
    std::shared_ptr<State> getCurrentState() const { return currentState; }
    std::shared_ptr<State> getPreviousState() const { return previousState; }
    
    std::string getCurrentStateName() const;
    const std::vector<std::string>& getStateHistory() const { return stateHistory; }
    
    bool isInState(const std::string& stateName) const;
};

// =============================================================================
// Pathfinding System
// =============================================================================

struct PathNode {
    JJM::Math::Vector2D position;
    float gCost = 0.0f; // Cost from start
    float hCost = 0.0f; // Heuristic cost to goal
    float fCost() const { return gCost + hCost; }
    
    PathNode* parent = nullptr;
    bool walkable = true;
    int gridX = 0;
    int gridY = 0;
    
    PathNode() = default;
    PathNode(const JJM::Math::Vector2D& pos, int x, int y);
};

class PathGrid {
private:
    std::vector<std::vector<PathNode>> grid;
    int width;
    int height;
    float cellSize;
    JJM::Math::Vector2D worldOffset;

public:
    PathGrid(int w, int h, float size, const JJM::Math::Vector2D& offset = JJM::Math::Vector2D(0, 0));
    
    void setWalkable(int x, int y, bool walkable);
    bool isWalkable(int x, int y) const;
    
    PathNode* getNode(int x, int y);
    JJM::Math::Vector2D worldToGrid(const JJM::Math::Vector2D& worldPos) const;
    JJM::Math::Vector2D gridToWorld(int x, int y) const;
    
    std::vector<PathNode*> getNeighbors(PathNode* node);
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    float getCellSize() const { return cellSize; }
};

enum class PathfindingAlgorithm {
    A_STAR,
    DIJKSTRA,
    GREEDY_BEST_FIRST
};

class Pathfinder {
private:
    PathGrid* grid;
    PathfindingAlgorithm algorithm;
    
    float heuristic(const PathNode* a, const PathNode* b) const;
    std::vector<JJM::Math::Vector2D> reconstructPath(PathNode* endNode);

public:
    Pathfinder(PathGrid* pathGrid, PathfindingAlgorithm algo = PathfindingAlgorithm::A_STAR);
    
    std::vector<JJM::Math::Vector2D> findPath(const JJM::Math::Vector2D& start, const JJM::Math::Vector2D& goal);
    void setAlgorithm(PathfindingAlgorithm algo) { algorithm = algo; }
};

// =============================================================================
// Steering Behaviors
// =============================================================================

struct SteeringOutput {
    JJM::Math::Vector2D linear;
    float angular = 0.0f;
};

class SteeringBehavior {
public:
    virtual ~SteeringBehavior() = default;
    virtual SteeringOutput calculate(const JJM::Math::Vector2D& position, 
                                     const JJM::Math::Vector2D& velocity,
                                     float orientation) = 0;
};

class SeekBehavior : public SteeringBehavior {
private:
    JJM::Math::Vector2D target;
    float maxSpeed;

public:
    SeekBehavior(const JJM::Math::Vector2D& t, float speed);
    SteeringOutput calculate(const JJM::Math::Vector2D& position, 
                            const JJM::Math::Vector2D& velocity,
                            float orientation) override;
    void setTarget(const JJM::Math::Vector2D& t) { target = t; }
};

class FleeBehavior : public SteeringBehavior {
private:
    JJM::Math::Vector2D target;
    float maxSpeed;
    float panicDistance;

public:
    FleeBehavior(const JJM::Math::Vector2D& t, float speed, float panic = 100.0f);
    SteeringOutput calculate(const JJM::Math::Vector2D& position, 
                            const JJM::Math::Vector2D& velocity,
                            float orientation) override;
};

class ArriveBehavior : public SteeringBehavior {
private:
    JJM::Math::Vector2D target;
    float maxSpeed;
    float slowRadius;
    float targetRadius;

public:
    ArriveBehavior(const JJM::Math::Vector2D& t, float speed, 
                   float slowR = 50.0f, float targetR = 5.0f);
    SteeringOutput calculate(const JJM::Math::Vector2D& position, 
                            const JJM::Math::Vector2D& velocity,
                            float orientation) override;
};

class WanderBehavior : public SteeringBehavior {
private:
    float circleDistance;
    float circleRadius;
    float wanderAngle;
    float maxSpeed;
    float angleChange;

public:
    WanderBehavior(float speed, float distance = 50.0f, 
                   float radius = 20.0f, float angleChg = 0.5f);
    SteeringOutput calculate(const JJM::Math::Vector2D& position, 
                            const JJM::Math::Vector2D& velocity,
                            float orientation) override;
};

// =============================================================================
// Flocking Behaviors
// =============================================================================

class FlockingBehavior {
private:
    float separationWeight = 1.5f;
    float alignmentWeight = 1.0f;
    float cohesionWeight = 1.0f;
    float separationRadius = 25.0f;
    float neighborRadius = 50.0f;
    float maxSpeed = 100.0f;

public:
    SteeringOutput calculate(const JJM::Math::Vector2D& position,
                            const JJM::Math::Vector2D& velocity,
                            const std::vector<JJM::Math::Vector2D>& neighborPositions,
                            const std::vector<JJM::Math::Vector2D>& neighborVelocities);
    
    void setSeparationWeight(float w) { separationWeight = w; }
    void setAlignmentWeight(float w) { alignmentWeight = w; }
    void setCohesionWeight(float w) { cohesionWeight = w; }
    void setRadii(float separation, float neighbor);
    void setMaxSpeed(float speed) { maxSpeed = speed; }

private:
    JJM::Math::Vector2D separation(const JJM::Math::Vector2D& position,
                             const std::vector<JJM::Math::Vector2D>& neighbors);
    JJM::Math::Vector2D alignment(const JJM::Math::Vector2D& velocity,
                            const std::vector<JJM::Math::Vector2D>& neighborVelocities);
    JJM::Math::Vector2D cohesion(const JJM::Math::Vector2D& position,
                           const std::vector<JJM::Math::Vector2D>& neighbors);
};

// =============================================================================
// AI Agent
// =============================================================================

struct AgentConfig {
    float maxSpeed = 100.0f;
    float maxAcceleration = 50.0f;
    float detectionRadius = 150.0f;
    bool usePathfinding = true;
    bool useFlocking = false;
};

class AIAgent {
private:
    std::string name;
    JJM::Math::Vector2D position;
    JJM::Math::Vector2D velocity;
    float orientation = 0.0f;
    
    AgentConfig config;
    
    std::unique_ptr<BehaviorTree> behaviorTree;
    std::unique_ptr<StateMachine> stateMachine;
    std::shared_ptr<SteeringBehavior> steeringBehavior;
    
    std::vector<JJM::Math::Vector2D> currentPath;
    size_t currentPathIndex = 0;
    
    // Perception
    std::vector<AIAgent*> visibleAgents;
    std::vector<JJM::Math::Vector2D> obstacles;

public:
    AIAgent(const std::string& agentName, const JJM::Math::Vector2D& startPos);
    
    void update(float deltaTime);
    
    // Behavior tree
    void setBehaviorTree(std::unique_ptr<BehaviorTree> tree);
    BehaviorTree* getBehaviorTree() const { return behaviorTree.get(); }
    
    // State machine
    void setStateMachine(std::unique_ptr<StateMachine> machine);
    StateMachine* getStateMachine() const { return stateMachine.get(); }
    
    // Steering
    void setSteeringBehavior(std::shared_ptr<SteeringBehavior> behavior);
    void applySteeringForce(const SteeringOutput& steering, float deltaTime);
    
    // Path following
    void setPath(const std::vector<JJM::Math::Vector2D>& path);
    bool isFollowingPath() const;
    void updatePathFollowing(float deltaTime);
    
    // Perception
    void updatePerception(const std::vector<AIAgent*>& allAgents);
    const std::vector<AIAgent*>& getVisibleAgents() const { return visibleAgents; }
    
    // Getters/Setters
    JJM::Math::Vector2D getPosition() const { return position; }
    void setPosition(const JJM::Math::Vector2D& pos) { position = pos; }
    
    JJM::Math::Vector2D getVelocity() const { return velocity; }
    void setVelocity(const JJM::Math::Vector2D& vel) { velocity = vel; }
    
    float getOrientation() const { return orientation; }
    void setOrientation(float ori) { orientation = ori; }
    
    AgentConfig& getConfig() { return config; }
    std::string getName() const { return name; }
};

// =============================================================================
// AI Manager
// =============================================================================

class AIManager {
private:
    static AIManager* instance;
    
    std::vector<std::unique_ptr<AIAgent>> agents;
    std::unique_ptr<PathGrid> pathGrid;
    std::unique_ptr<Pathfinder> pathfinder;
    
    bool spatialOptimizationEnabled = true;
    float updateRadius = 500.0f; // Only update agents within this radius of camera
    
    AIManager();

public:
    ~AIManager();
    
    static AIManager* getInstance();
    static void cleanup();
    
    // Agent management
    AIAgent* createAgent(const std::string& name, const JJM::Math::Vector2D& position);
    void removeAgent(const std::string& name);
    AIAgent* getAgent(const std::string& name);
    
    // Pathfinding
    void initializePathGrid(int width, int height, float cellSize, 
                           const JJM::Math::Vector2D& offset = JJM::Math::Vector2D(0, 0));
    PathGrid* getPathGrid() { return pathGrid.get(); }
    Pathfinder* getPathfinder() { return pathfinder.get(); }
    
    // Update
    void update(float deltaTime);
    void updateWithinRadius(float deltaTime, const JJM::Math::Vector2D& center);
    
    // Optimization
    void setSpatialOptimization(bool enabled) { spatialOptimizationEnabled = enabled; }
    void setUpdateRadius(float radius) { updateRadius = radius; }
    
    // Queries
    std::vector<AIAgent*> getAgentsInRadius(const JJM::Math::Vector2D& center, float radius);
    size_t getAgentCount() const { return agents.size(); }
};

} // namespace AI

#endif // AI_SYSTEM_H
