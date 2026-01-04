#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <any>

namespace JJM {
namespace AI {

enum class NodeStatus {
    Success,
    Failure,
    Running
};

class BehaviorNode {
public:
    BehaviorNode(const std::string& name);
    virtual ~BehaviorNode() = default;
    
    virtual NodeStatus tick(float deltaTime) = 0;
    virtual void reset() {}
    
    const std::string& getName() const { return name; }
    NodeStatus getStatus() const { return status; }

protected:
    std::string name;
    NodeStatus status;
};

class ActionNode : public BehaviorNode {
public:
    ActionNode(const std::string& name, std::function<NodeStatus(float)> action);
    
    NodeStatus tick(float deltaTime) override;

private:
    std::function<NodeStatus(float)> action;
};

class ConditionNode : public BehaviorNode {
public:
    ConditionNode(const std::string& name, std::function<bool()> condition);
    
    NodeStatus tick(float deltaTime) override;

private:
    std::function<bool()> condition;
};

class CompositeNode : public BehaviorNode {
public:
    CompositeNode(const std::string& name);
    
    void addChild(std::unique_ptr<BehaviorNode> child);
    void reset() override;

protected:
    std::vector<std::unique_ptr<BehaviorNode>> children;
    size_t currentChild;
};

class SequenceNode : public CompositeNode {
public:
    SequenceNode(const std::string& name);
    
    NodeStatus tick(float deltaTime) override;
    void reset() override;
};

class SelectorNode : public CompositeNode {
public:
    SelectorNode(const std::string& name);
    
    NodeStatus tick(float deltaTime) override;
    void reset() override;
};

class ParallelNode : public CompositeNode {
public:
    enum class Policy {
        RequireOne,
        RequireAll
    };
    
    ParallelNode(const std::string& name, Policy successPolicy = Policy::RequireAll);
    
    NodeStatus tick(float deltaTime) override;
    void reset() override;

private:
    Policy successPolicy;
};

class DecoratorNode : public BehaviorNode {
public:
    DecoratorNode(const std::string& name);
    
    void setChild(std::unique_ptr<BehaviorNode> child);
    void reset() override;

protected:
    std::unique_ptr<BehaviorNode> child;
};

class InverterNode : public DecoratorNode {
public:
    InverterNode(const std::string& name);
    
    NodeStatus tick(float deltaTime) override;
};

class RepeaterNode : public DecoratorNode {
public:
    RepeaterNode(const std::string& name, int maxRepeats = -1);
    
    NodeStatus tick(float deltaTime) override;
    void reset() override;

private:
    int maxRepeats;
    int currentRepeats;
};

class SucceederNode : public DecoratorNode {
public:
    SucceederNode(const std::string& name);
    
    NodeStatus tick(float deltaTime) override;
};

// Advanced decorator: Always returns Failure
class FailerNode : public DecoratorNode {
public:
    FailerNode(const std::string& name);
    
    NodeStatus tick(float deltaTime) override;
};

// Delay decorator: Waits for specified time before executing child
class DelayNode : public DecoratorNode {
public:
    DelayNode(const std::string& name, float delaySeconds);
    
    NodeStatus tick(float deltaTime) override;
    void reset() override;

private:
    float delayTime;
    float elapsedTime;
    bool delayComplete;
};

// Timeout decorator: Fails if child takes too long
class TimeoutNode : public DecoratorNode {
public:
    TimeoutNode(const std::string& name, float timeoutSeconds);
    
    NodeStatus tick(float deltaTime) override;
    void reset() override;

private:
    float timeout;
    float elapsedTime;
};

// Cooldown decorator: Prevents execution until cooldown expires
class CooldownNode : public DecoratorNode {
public:
    CooldownNode(const std::string& name, float cooldownSeconds);
    
    NodeStatus tick(float deltaTime) override;
    void reset() override;
    
    bool isOnCooldown() const { return onCooldown; }
    float getRemainingCooldown() const { return cooldownRemaining; }

private:
    float cooldownTime;
    float cooldownRemaining;
    bool onCooldown;
};

// Retry decorator: Retries child on failure up to N times
class RetryNode : public DecoratorNode {
public:
    RetryNode(const std::string& name, int maxRetries);
    
    NodeStatus tick(float deltaTime) override;
    void reset() override;

private:
    int maxRetries;
    int retriesRemaining;
};

// Random decorator: Executes child with a probability
class RandomNode : public DecoratorNode {
public:
    RandomNode(const std::string& name, float probability);
    
    NodeStatus tick(float deltaTime) override;
    void reset() override;
    
    void setProbability(float prob) { probability = prob; }

private:
    float probability;
    bool evaluated;
    bool shouldExecute;
};

// ForceSuccess decorator: Converts Failure to Success
class ForceSuccessNode : public DecoratorNode {
public:
    ForceSuccessNode(const std::string& name);
    
    NodeStatus tick(float deltaTime) override;
};

// ForceFailure decorator: Converts Success to Failure
class ForceFailureNode : public DecoratorNode {
public:
    ForceFailureNode(const std::string& name);
    
    NodeStatus tick(float deltaTime) override;
};

// UntilSuccess decorator: Repeats until child succeeds
class UntilSuccessNode : public DecoratorNode {
public:
    UntilSuccessNode(const std::string& name);
    
    NodeStatus tick(float deltaTime) override;
};

// UntilFailure decorator: Repeats until child fails
class UntilFailureNode : public DecoratorNode {
public:
    UntilFailureNode(const std::string& name);
    
    NodeStatus tick(float deltaTime) override;
};

// Blackboard for sharing data between nodes
class Blackboard {
public:
    template<typename T>
    void set(const std::string& key, const T& value);
    
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const;
    
    bool has(const std::string& key) const;
    void remove(const std::string& key);
    void clear();

private:
    std::unordered_map<std::string, std::any> data;
};

class BehaviorTree {
public:
    BehaviorTree();
    ~BehaviorTree();
    
    void setRoot(std::unique_ptr<BehaviorNode> root);
    
    NodeStatus tick(float deltaTime);
    void reset();
    
    BehaviorNode* getRoot() { return root.get(); }
    
    // Blackboard access
    Blackboard& getBlackboard() { return blackboard; }
    const Blackboard& getBlackboard() const { return blackboard; }

private:
    std::unique_ptr<BehaviorNode> root;
    Blackboard blackboard;
};

} // namespace AI
} // namespace JJM
