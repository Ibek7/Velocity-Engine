#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

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

class BehaviorTree {
public:
    BehaviorTree();
    ~BehaviorTree();
    
    void setRoot(std::unique_ptr<BehaviorNode> root);
    
    NodeStatus tick(float deltaTime);
    void reset();
    
    BehaviorNode* getRoot() { return root.get(); }

private:
    std::unique_ptr<BehaviorNode> root;
};

} // namespace AI
} // namespace JJM
