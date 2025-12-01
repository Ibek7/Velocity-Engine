#include "../../include/ai/AISystem.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <random>

namespace AI {

// =============================================================================
// Behavior Tree Implementation
// =============================================================================

// CompositeNode
void CompositeNode::addChild(std::shared_ptr<BehaviorNode> child) {
    children.push_back(child);
}

void CompositeNode::reset() {
    currentChild = 0;
    for (auto& child : children) {
        child->reset();
    }
}

// SequenceNode
NodeStatus SequenceNode::tick(float deltaTime) {
    while (currentChild < children.size()) {
        NodeStatus status = children[currentChild]->tick(deltaTime);
        
        if (status == NodeStatus::RUNNING) {
            return NodeStatus::RUNNING;
        } else if (status == NodeStatus::FAILURE) {
            currentChild = 0;
            return NodeStatus::FAILURE;
        }
        
        currentChild++;
    }
    
    currentChild = 0;
    return NodeStatus::SUCCESS;
}

// SelectorNode
NodeStatus SelectorNode::tick(float deltaTime) {
    while (currentChild < children.size()) {
        NodeStatus status = children[currentChild]->tick(deltaTime);
        
        if (status == NodeStatus::RUNNING) {
            return NodeStatus::RUNNING;
        } else if (status == NodeStatus::SUCCESS) {
            currentChild = 0;
            return NodeStatus::SUCCESS;
        }
        
        currentChild++;
    }
    
    currentChild = 0;
    return NodeStatus::FAILURE;
}

// ParallelNode
ParallelNode::ParallelNode(int successThresh, int failureThresh)
    : successThreshold(successThresh), failureThreshold(failureThresh) {}

NodeStatus ParallelNode::tick(float deltaTime) {
    int successCount = 0;
    int failureCount = 0;
    int runningCount = 0;
    
    for (auto& child : children) {
        NodeStatus status = child->tick(deltaTime);
        
        if (status == NodeStatus::SUCCESS) {
            successCount++;
        } else if (status == NodeStatus::FAILURE) {
            failureCount++;
        } else {
            runningCount++;
        }
    }
    
    if (successCount >= successThreshold) {
        return NodeStatus::SUCCESS;
    }
    if (failureCount >= failureThreshold) {
        return NodeStatus::FAILURE;
    }
    
    return NodeStatus::RUNNING;
}

void ParallelNode::reset() {
    CompositeNode::reset();
}

// DecoratorNode
void DecoratorNode::reset() {
    if (child) {
        child->reset();
    }
}

// InverterNode
NodeStatus InverterNode::tick(float deltaTime) {
    if (!child) return NodeStatus::FAILURE;
    
    NodeStatus status = child->tick(deltaTime);
    
    if (status == NodeStatus::SUCCESS) {
        return NodeStatus::FAILURE;
    } else if (status == NodeStatus::FAILURE) {
        return NodeStatus::SUCCESS;
    }
    
    return NodeStatus::RUNNING;
}

// RepeaterNode
RepeaterNode::RepeaterNode(int count) : repeatCount(count) {}

NodeStatus RepeaterNode::tick(float deltaTime) {
    if (!child) return NodeStatus::FAILURE;
    
    while (repeatCount < 0 || currentCount < repeatCount) {
        NodeStatus status = child->tick(deltaTime);
        
        if (status == NodeStatus::RUNNING) {
            return NodeStatus::RUNNING;
        } else if (status == NodeStatus::FAILURE) {
            currentCount = 0;
            return NodeStatus::FAILURE;
        }
        
        currentCount++;
        child->reset();
        
        if (repeatCount > 0 && currentCount >= repeatCount) {
            currentCount = 0;
            return NodeStatus::SUCCESS;
        }
    }
    
    return NodeStatus::SUCCESS;
}

void RepeaterNode::reset() {
    DecoratorNode::reset();
    currentCount = 0;
}

// SucceederNode
NodeStatus SucceederNode::tick(float deltaTime) {
    if (child) {
        child->tick(deltaTime);
    }
    return NodeStatus::SUCCESS;
}

// ActionNode
ActionNode::ActionNode(const std::string& name, TickFunction func)
    : name(name), action(func) {}

NodeStatus ActionNode::tick(float deltaTime) {
    if (action) {
        return action(deltaTime);
    }
    return NodeStatus::FAILURE;
}

// ConditionNode
ConditionNode::ConditionNode(const std::string& name, std::function<bool()> cond)
    : name(name), condition(cond) {}

NodeStatus ConditionNode::tick(float deltaTime) {
    if (condition && condition()) {
        return NodeStatus::SUCCESS;
    }
    return NodeStatus::FAILURE;
}

// Blackboard
bool Blackboard::has(const std::string& key) const {
    return data.find(key) != data.end();
}

void Blackboard::remove(const std::string& key) {
    data.erase(key);
}

void Blackboard::clear() {
    data.clear();
}

// BehaviorTree
BehaviorTree::BehaviorTree(const std::string& treeName) : name(treeName) {}

NodeStatus BehaviorTree::tick(float deltaTime) {
    if (root) {
        return root->tick(deltaTime);
    }
    return NodeStatus::FAILURE;
}

void BehaviorTree::reset() {
    if (root) {
        root->reset();
    }
}

// =============================================================================
// State Machine Implementation
// =============================================================================

// State
State::State(const std::string& stateName) : name(stateName) {}

void State::enter() {
    if (onEnter) {
        onEnter();
    }
}

void State::update(float deltaTime) {
    if (onUpdate) {
        onUpdate(deltaTime);
    }
}

void State::exit() {
    if (onExit) {
        onExit();
    }
}

// Transition
Transition::Transition(const std::string& from, const std::string& to,
                      std::function<bool()> cond, int prio)
    : fromState(from), toState(to), condition(cond), priority(prio) {}

// StateMachine
StateMachine::StateMachine(const std::string& machineName) : name(machineName) {}

void StateMachine::addState(const std::string& stateName) {
    states[stateName] = std::make_shared<State>(stateName);
}

void StateMachine::addTransition(const std::string& from, const std::string& to,
                                std::function<bool()> condition, int priority) {
    transitions.push_back(Transition(from, to, condition, priority));
    
    // Sort transitions by priority (higher first)
    std::sort(transitions.begin(), transitions.end(),
             [](const Transition& a, const Transition& b) {
                 return a.priority > b.priority;
             });
}

void StateMachine::setState(const std::string& stateName) {
    auto it = states.find(stateName);
    if (it == states.end()) return;
    
    if (currentState) {
        currentState->exit();
        previousState = currentState;
    }
    
    currentState = it->second;
    currentState->enter();
    
    // Update history
    stateHistory.push_back(stateName);
    if (stateHistory.size() > maxHistorySize) {
        stateHistory.erase(stateHistory.begin());
    }
}

void StateMachine::update(float deltaTime) {
    if (!currentState) return;
    
    // Check transitions
    for (const auto& transition : transitions) {
        if (transition.fromState == currentState->getName() &&
            transition.condition && transition.condition()) {
            setState(transition.toState);
            return;
        }
    }
    
    // Update current state
    currentState->update(deltaTime);
}

std::shared_ptr<State> StateMachine::getState(const std::string& stateName) {
    auto it = states.find(stateName);
    return (it != states.end()) ? it->second : nullptr;
}

std::string StateMachine::getCurrentStateName() const {
    return currentState ? currentState->getName() : "";
}

bool StateMachine::isInState(const std::string& stateName) const {
    return currentState && currentState->getName() == stateName;
}

// =============================================================================
// Pathfinding Implementation
// =============================================================================

// PathNode
PathNode::PathNode(const JJM::Math::Vector2D& pos, int x, int y)
    : position(pos), gridX(x), gridY(y) {}

// PathGrid
PathGrid::PathGrid(int w, int h, float size, const JJM::Math::Vector2D& offset)
    : width(w), height(h), cellSize(size), worldOffset(offset) {
    
    grid.resize(height);
    for (int y = 0; y < height; y++) {
        grid[y].resize(width);
        for (int x = 0; x < width; x++) {
            JJM::Math::Vector2D worldPos = gridToWorld(x, y);
            grid[y][x] = PathNode(worldPos, x, y);
        }
    }
}

void PathGrid::setWalkable(int x, int y, bool walkable) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        grid[y][x].walkable = walkable;
    }
}

bool PathGrid::isWalkable(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    return grid[y][x].walkable;
}

PathNode* PathGrid::getNode(int x, int y) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        return &grid[y][x];
    }
    return nullptr;
}

JJM::Math::Vector2D PathGrid::worldToGrid(const JJM::Math::Vector2D& worldPos) const {
    float gridX = (worldPos.x - worldOffset.x) / cellSize;
    float gridY = (worldPos.y - worldOffset.y) / cellSize;
    return JJM::Math::Vector2D(gridX, gridY);
}

JJM::Math::Vector2D PathGrid::gridToWorld(int x, int y) const {
    float worldX = x * cellSize + worldOffset.x + cellSize * 0.5f;
    float worldY = y * cellSize + worldOffset.y + cellSize * 0.5f;
    return JJM::Math::Vector2D(worldX, worldY);
}

std::vector<PathNode*> PathGrid::getNeighbors(PathNode* node) {
    std::vector<PathNode*> neighbors;
    
    int directions[8][2] = {
        {0, 1}, {1, 0}, {0, -1}, {-1, 0},  // Cardinal
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}  // Diagonal
    };
    
    for (auto& dir : directions) {
        int newX = node->gridX + dir[0];
        int newY = node->gridY + dir[1];
        
        if (isWalkable(newX, newY)) {
            neighbors.push_back(&grid[newY][newX]);
        }
    }
    
    return neighbors;
}

// Pathfinder
Pathfinder::Pathfinder(PathGrid* pathGrid, PathfindingAlgorithm algo)
    : grid(pathGrid), algorithm(algo) {}

float Pathfinder::heuristic(const PathNode* a, const PathNode* b) const {
    // Manhattan distance
    float dx = std::abs(a->gridX - b->gridX);
    float dy = std::abs(a->gridY - b->gridY);
    return dx + dy;
}

std::vector<JJM::Math::Vector2D> Pathfinder::reconstructPath(PathNode* endNode) {
    std::vector<JJM::Math::Vector2D> path;
    PathNode* current = endNode;
    
    while (current != nullptr) {
        path.push_back(current->position);
        current = current->parent;
    }
    
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<JJM::Math::Vector2D> Pathfinder::findPath(const JJM::Math::Vector2D& start, 
                                                  const JJM::Math::Vector2D& goal) {
    if (!grid) return {};
    
    JJM::Math::Vector2D startGrid = grid->worldToGrid(start);
    JJM::Math::Vector2D goalGrid = grid->worldToGrid(goal);
    
    PathNode* startNode = grid->getNode((int)startGrid.x, (int)startGrid.y);
    PathNode* goalNode = grid->getNode((int)goalGrid.x, (int)goalGrid.y);
    
    if (!startNode || !goalNode || !startNode->walkable || !goalNode->walkable) {
        return {};
    }
    
    // Reset grid
    for (int y = 0; y < grid->getHeight(); y++) {
        for (int x = 0; x < grid->getWidth(); x++) {
            PathNode* node = grid->getNode(x, y);
            if (node) {
                node->gCost = 0;
                node->hCost = 0;
                node->parent = nullptr;
            }
        }
    }
    
    // A* implementation
    auto compare = [](PathNode* a, PathNode* b) {
        return a->fCost() > b->fCost();
    };
    
    std::priority_queue<PathNode*, std::vector<PathNode*>, decltype(compare)> openSet(compare);
    std::set<PathNode*> closedSet;
    
    startNode->gCost = 0;
    startNode->hCost = heuristic(startNode, goalNode);
    openSet.push(startNode);
    
    while (!openSet.empty()) {
        PathNode* current = openSet.top();
        openSet.pop();
        
        if (current == goalNode) {
            return reconstructPath(goalNode);
        }
        
        closedSet.insert(current);
        
        for (PathNode* neighbor : grid->getNeighbors(current)) {
            if (closedSet.find(neighbor) != closedSet.end()) {
                continue;
            }
            
            float tentativeGCost = current->gCost + 1.0f;
            
            if (tentativeGCost < neighbor->gCost || neighbor->gCost == 0) {
                neighbor->parent = current;
                neighbor->gCost = tentativeGCost;
                
                if (algorithm == PathfindingAlgorithm::A_STAR) {
                    neighbor->hCost = heuristic(neighbor, goalNode);
                } else if (algorithm == PathfindingAlgorithm::DIJKSTRA) {
                    neighbor->hCost = 0;
                } else { // GREEDY_BEST_FIRST
                    neighbor->hCost = heuristic(neighbor, goalNode);
                    neighbor->gCost = 0;
                }
                
                openSet.push(neighbor);
            }
        }
    }
    
    return {}; // No path found
}

// =============================================================================
// Steering Behaviors Implementation
// =============================================================================

// SeekBehavior
SeekBehavior::SeekBehavior(const JJM::Math::Vector2D& t, float speed)
    : target(t), maxSpeed(speed) {}

SteeringOutput SeekBehavior::calculate(const JJM::Math::Vector2D& position,
                                       const JJM::Math::Vector2D& velocity,
                                       float orientation) {
    SteeringOutput output;
    JJM::Math::Vector2D desired = target - position;
    desired.normalize();
    desired = desired * maxSpeed;
    output.linear = desired - velocity;
    return output;
}

// FleeBehavior
FleeBehavior::FleeBehavior(const JJM::Math::Vector2D& t, float speed, float panic)
    : target(t), maxSpeed(speed), panicDistance(panic) {}

SteeringOutput FleeBehavior::calculate(const JJM::Math::Vector2D& position,
                                       const JJM::Math::Vector2D& velocity,
                                       float orientation) {
    SteeringOutput output;
    
    float distance = (target - position).magnitude();
    if (distance > panicDistance) {
        return output; // Too far to panic
    }
    
    JJM::Math::Vector2D desired = position - target;
    desired.normalize();
    desired = desired * maxSpeed;
    output.linear = desired - velocity;
    return output;
}

// ArriveBehavior
ArriveBehavior::ArriveBehavior(const JJM::Math::Vector2D& t, float speed,
                               float slowR, float targetR)
    : target(t), maxSpeed(speed), slowRadius(slowR), targetRadius(targetR) {}

SteeringOutput ArriveBehavior::calculate(const JJM::Math::Vector2D& position,
                                        const JJM::Math::Vector2D& velocity,
                                        float orientation) {
    SteeringOutput output;
    JJM::Math::Vector2D direction = target - position;
    float distance = direction.magnitude();
    
    if (distance < targetRadius) {
        output.linear = -velocity; // Stop
        return output;
    }
    
    float targetSpeed = maxSpeed;
    if (distance < slowRadius) {
        targetSpeed = maxSpeed * (distance / slowRadius);
    }
    
    JJM::Math::Vector2D desired = direction;
    desired.normalize();
    desired = desired * targetSpeed;
    output.linear = desired - velocity;
    return output;
}

// WanderBehavior
WanderBehavior::WanderBehavior(float speed, float distance, float radius, float angleChg)
    : maxSpeed(speed), circleDistance(distance), circleRadius(radius), 
      angleChange(angleChg), wanderAngle(0.0f) {}

SteeringOutput WanderBehavior::calculate(const JJM::Math::Vector2D& position,
                                        const JJM::Math::Vector2D& velocity,
                                        float orientation) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(-angleChange, angleChange);
    
    wanderAngle += dis(gen);
    
    JJM::Math::Vector2D circleCenter = velocity;
    circleCenter.normalize();
    circleCenter = circleCenter * circleDistance;
    
    JJM::Math::Vector2D displacement(
        circleRadius * std::cos(wanderAngle),
        circleRadius * std::sin(wanderAngle)
    );
    
    SteeringOutput output;
    output.linear = circleCenter + displacement;
    output.linear.normalize();
    output.linear = output.linear * maxSpeed;
    return output;
}

// =============================================================================
// Flocking Behaviors Implementation
// =============================================================================

void FlockingBehavior::setRadii(float separation, float neighbor) {
    separationRadius = separation;
    neighborRadius = neighbor;
}

JJM::Math::Vector2D FlockingBehavior::separation(const JJM::Math::Vector2D& position,
                                            const std::vector<JJM::Math::Vector2D>& neighbors) {
    JJM::Math::Vector2D steer(0, 0);
    
    for (const auto& neighbor : neighbors) {
        float distance = (position - neighbor).magnitude();
        if (distance > 0 && distance < separationRadius) {
            JJM::Math::Vector2D diff = position - neighbor;
            diff.normalize();
            diff = diff * (1.0f / distance); // Weight by distance
            steer = steer + diff;
        }
    }
    
    return steer;
}

JJM::Math::Vector2D FlockingBehavior::alignment(const JJM::Math::Vector2D& velocity,
                                          const std::vector<JJM::Math::Vector2D>& neighborVelocities) {
    if (neighborVelocities.empty()) {
        return JJM::Math::Vector2D(0, 0);
    }
    
    JJM::Math::Vector2D average(0, 0);
    for (const auto& vel : neighborVelocities) {
        average = average + vel;
    }
    
    average = average * (1.0f / neighborVelocities.size());
    average.normalize();
    average = average * maxSpeed;
    
    JJM::Math::Vector2D steer = average - velocity;
    return steer;
}

JJM::Math::Vector2D FlockingBehavior::cohesion(const JJM::Math::Vector2D& position,
                                         const std::vector<JJM::Math::Vector2D>& neighbors) {
    if (neighbors.empty()) {
        return JJM::Math::Vector2D(0, 0);
    }
    
    JJM::Math::Vector2D center(0, 0);
    for (const auto& neighbor : neighbors) {
        center = center + neighbor;
    }
    
    center = center * (1.0f / neighbors.size());
    
    JJM::Math::Vector2D desired = center - position;
    desired.normalize();
    desired = desired * maxSpeed;
    
    return desired;
}

SteeringOutput FlockingBehavior::calculate(const JJM::Math::Vector2D& position,
                                          const JJM::Math::Vector2D& velocity,
                                          const std::vector<JJM::Math::Vector2D>& neighborPositions,
                                          const std::vector<JJM::Math::Vector2D>& neighborVelocities) {
    // Filter neighbors by distance
    std::vector<JJM::Math::Vector2D> closeNeighbors;
    std::vector<JJM::Math::Vector2D> closeVelocities;
    
    for (size_t i = 0; i < neighborPositions.size(); i++) {
        float distance = (position - neighborPositions[i]).magnitude();
        if (distance < neighborRadius && distance > 0) {
            closeNeighbors.push_back(neighborPositions[i]);
            if (i < neighborVelocities.size()) {
                closeVelocities.push_back(neighborVelocities[i]);
            }
        }
    }
    
    JJM::Math::Vector2D sep = separation(position, closeNeighbors) * separationWeight;
    JJM::Math::Vector2D ali = alignment(velocity, closeVelocities) * alignmentWeight;
    JJM::Math::Vector2D coh = cohesion(position, closeNeighbors) * cohesionWeight;
    
    SteeringOutput output;
    output.linear = sep + ali + coh;
    
    // Limit steering force
    if (output.linear.magnitude() > maxSpeed) {
        output.linear.normalize();
        output.linear = output.linear * maxSpeed;
    }
    
    return output;
}

// =============================================================================
// AI Agent Implementation
// =============================================================================

AIAgent::AIAgent(const std::string& agentName, const JJM::Math::Vector2D& startPos)
    : name(agentName), position(startPos) {}

void AIAgent::update(float deltaTime) {
    // Update behavior tree
    if (behaviorTree) {
        behaviorTree->tick(deltaTime);
    }
    
    // Update state machine
    if (stateMachine) {
        stateMachine->update(deltaTime);
    }
    
    // Update path following
    if (isFollowingPath()) {
        updatePathFollowing(deltaTime);
    }
    
    // Apply steering behavior
    if (steeringBehavior) {
        SteeringOutput steering = steeringBehavior->calculate(position, velocity, orientation);
        applySteeringForce(steering, deltaTime);
    }
    
    // Update position
    position = position + velocity * deltaTime;
    
    // Update orientation based on velocity
    if (velocity.magnitude() > 0.1f) {
        orientation = std::atan2(velocity.y, velocity.x);
    }
}

void AIAgent::setBehaviorTree(std::unique_ptr<BehaviorTree> tree) {
    behaviorTree = std::move(tree);
}

void AIAgent::setStateMachine(std::unique_ptr<StateMachine> machine) {
    stateMachine = std::move(machine);
}

void AIAgent::setSteeringBehavior(std::shared_ptr<SteeringBehavior> behavior) {
    steeringBehavior = behavior;
}

void AIAgent::applySteeringForce(const SteeringOutput& steering, float deltaTime) {
    // Apply linear acceleration
    JJM::Math::Vector2D acceleration = steering.linear;
    
    // Limit acceleration
    if (acceleration.magnitude() > config.maxAcceleration) {
        acceleration.normalize();
        acceleration = acceleration * config.maxAcceleration;
    }
    
    velocity = velocity + acceleration * deltaTime;
    
    // Limit velocity
    if (velocity.magnitude() > config.maxSpeed) {
        velocity.normalize();
        velocity = velocity * config.maxSpeed;
    }
    
    // Apply angular acceleration
    orientation += steering.angular * deltaTime;
}

void AIAgent::setPath(const std::vector<JJM::Math::Vector2D>& path) {
    currentPath = path;
    currentPathIndex = 0;
}

bool AIAgent::isFollowingPath() const {
    return currentPathIndex < currentPath.size();
}

void AIAgent::updatePathFollowing(float deltaTime) {
    if (!isFollowingPath()) return;
    
    const JJM::Math::Vector2D& target = currentPath[currentPathIndex];
    float distance = (target - position).magnitude();
    
    // Reached waypoint
    if (distance < 10.0f) {
        currentPathIndex++;
        if (currentPathIndex >= currentPath.size()) {
            velocity = JJM::Math::Vector2D(0, 0); // Stop at end
        }
    }
}

void AIAgent::updatePerception(const std::vector<AIAgent*>& allAgents) {
    visibleAgents.clear();
    
    for (AIAgent* other : allAgents) {
        if (other == this) continue;
        
        float distance = (other->position - position).magnitude();
        if (distance < config.detectionRadius) {
            visibleAgents.push_back(other);
        }
    }
}

// =============================================================================
// AI Manager Implementation
// =============================================================================

AIManager* AIManager::instance = nullptr;

AIManager::AIManager() {}

AIManager::~AIManager() {}

AIManager* AIManager::getInstance() {
    if (!instance) {
        instance = new AIManager();
    }
    return instance;
}

void AIManager::cleanup() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

AIAgent* AIManager::createAgent(const std::string& name, const JJM::Math::Vector2D& position) {
    auto agent = std::make_unique<AIAgent>(name, position);
    AIAgent* ptr = agent.get();
    agents.push_back(std::move(agent));
    return ptr;
}

void AIManager::removeAgent(const std::string& name) {
    agents.erase(
        std::remove_if(agents.begin(), agents.end(),
                      [&name](const std::unique_ptr<AIAgent>& agent) {
                          return agent->getName() == name;
                      }),
        agents.end()
    );
}

AIAgent* AIManager::getAgent(const std::string& name) {
    for (auto& agent : agents) {
        if (agent->getName() == name) {
            return agent.get();
        }
    }
    return nullptr;
}

void AIManager::initializePathGrid(int width, int height, float cellSize,
                                   const JJM::Math::Vector2D& offset) {
    pathGrid = std::make_unique<PathGrid>(width, height, cellSize, offset);
    pathfinder = std::make_unique<Pathfinder>(pathGrid.get());
}

void AIManager::update(float deltaTime) {
    // Update all agents perception
    std::vector<AIAgent*> allAgentPtrs;
    for (auto& agent : agents) {
        allAgentPtrs.push_back(agent.get());
    }
    
    for (auto& agent : agents) {
        agent->updatePerception(allAgentPtrs);
        agent->update(deltaTime);
    }
}

void AIManager::updateWithinRadius(float deltaTime, const JJM::Math::Vector2D& center) {
    if (!spatialOptimizationEnabled) {
        update(deltaTime);
        return;
    }
    
    // Only update agents within radius
    std::vector<AIAgent*> allAgentPtrs;
    for (auto& agent : agents) {
        allAgentPtrs.push_back(agent.get());
    }
    
    for (auto& agent : agents) {
        float distance = (agent->getPosition() - center).magnitude();
        if (distance < updateRadius) {
            agent->updatePerception(allAgentPtrs);
            agent->update(deltaTime);
        }
    }
}

std::vector<AIAgent*> AIManager::getAgentsInRadius(const JJM::Math::Vector2D& center, float radius) {
    std::vector<AIAgent*> result;
    
    for (auto& agent : agents) {
        float distance = (agent->getPosition() - center).magnitude();
        if (distance < radius) {
            result.push_back(agent.get());
        }
    }
    
    return result;
}

} // namespace AI
