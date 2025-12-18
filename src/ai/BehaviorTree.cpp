#include "ai/BehaviorTree.h"

namespace JJM {
namespace AI {

BehaviorNode::BehaviorNode(const std::string& name)
    : name(name), status(NodeStatus::Success) {}

ActionNode::ActionNode(const std::string& name, std::function<NodeStatus(float)> action)
    : BehaviorNode(name), action(action) {}

NodeStatus ActionNode::tick(float deltaTime) {
    if (action) {
        status = action(deltaTime);
    } else {
        status = NodeStatus::Failure;
    }
    return status;
}

ConditionNode::ConditionNode(const std::string& name, std::function<bool()> condition)
    : BehaviorNode(name), condition(condition) {}

NodeStatus ConditionNode::tick(float deltaTime) {
    (void)deltaTime;
    status = (condition && condition()) ? NodeStatus::Success : NodeStatus::Failure;
    return status;
}

CompositeNode::CompositeNode(const std::string& name)
    : BehaviorNode(name), currentChild(0) {}

void CompositeNode::addChild(std::unique_ptr<BehaviorNode> child) {
    children.push_back(std::move(child));
}

void CompositeNode::reset() {
    currentChild = 0;
    for (auto& child : children) {
        child->reset();
    }
}

SequenceNode::SequenceNode(const std::string& name)
    : CompositeNode(name) {}

NodeStatus SequenceNode::tick(float deltaTime) {
    while (currentChild < children.size()) {
        NodeStatus childStatus = children[currentChild]->tick(deltaTime);
        
        if (childStatus == NodeStatus::Running) {
            status = NodeStatus::Running;
            return status;
        }
        
        if (childStatus == NodeStatus::Failure) {
            reset();
            status = NodeStatus::Failure;
            return status;
        }
        
        ++currentChild;
    }
    
    reset();
    status = NodeStatus::Success;
    return status;
}

void SequenceNode::reset() {
    CompositeNode::reset();
}

SelectorNode::SelectorNode(const std::string& name)
    : CompositeNode(name) {}

NodeStatus SelectorNode::tick(float deltaTime) {
    while (currentChild < children.size()) {
        NodeStatus childStatus = children[currentChild]->tick(deltaTime);
        
        if (childStatus == NodeStatus::Running) {
            status = NodeStatus::Running;
            return status;
        }
        
        if (childStatus == NodeStatus::Success) {
            reset();
            status = NodeStatus::Success;
            return status;
        }
        
        ++currentChild;
    }
    
    reset();
    status = NodeStatus::Failure;
    return status;
}

void SelectorNode::reset() {
    CompositeNode::reset();
}

ParallelNode::ParallelNode(const std::string& name, Policy successPolicy)
    : CompositeNode(name), successPolicy(successPolicy) {}

NodeStatus ParallelNode::tick(float deltaTime) {
    int successCount = 0;
    int failureCount = 0;
    int runningCount = 0;
    
    for (auto& child : children) {
        NodeStatus childStatus = child->tick(deltaTime);
        
        switch (childStatus) {
            case NodeStatus::Success:
                ++successCount;
                break;
            case NodeStatus::Failure:
                ++failureCount;
                break;
            case NodeStatus::Running:
                ++runningCount;
                break;
        }
    }
    
    if (successPolicy == Policy::RequireOne && successCount > 0) {
        status = NodeStatus::Success;
    } else if (successPolicy == Policy::RequireAll && successCount == static_cast<int>(children.size())) {
        status = NodeStatus::Success;
    } else if (failureCount > 0 && successPolicy == Policy::RequireAll) {
        status = NodeStatus::Failure;
    } else {
        status = NodeStatus::Running;
    }
    
    return status;
}

void ParallelNode::reset() {
    CompositeNode::reset();
}

DecoratorNode::DecoratorNode(const std::string& name)
    : BehaviorNode(name) {}

void DecoratorNode::setChild(std::unique_ptr<BehaviorNode> child) {
    this->child = std::move(child);
}

void DecoratorNode::reset() {
    if (child) {
        child->reset();
    }
}

InverterNode::InverterNode(const std::string& name)
    : DecoratorNode(name) {}

NodeStatus InverterNode::tick(float deltaTime) {
    if (!child) {
        status = NodeStatus::Failure;
        return status;
    }
    
    NodeStatus childStatus = child->tick(deltaTime);
    
    if (childStatus == NodeStatus::Success) {
        status = NodeStatus::Failure;
    } else if (childStatus == NodeStatus::Failure) {
        status = NodeStatus::Success;
    } else {
        status = NodeStatus::Running;
    }
    
    return status;
}

RepeaterNode::RepeaterNode(const std::string& name, int maxRepeats)
    : DecoratorNode(name), maxRepeats(maxRepeats), currentRepeats(0) {}

NodeStatus RepeaterNode::tick(float deltaTime) {
    if (!child) {
        status = NodeStatus::Failure;
        return status;
    }
    
    NodeStatus childStatus = child->tick(deltaTime);
    
    if (childStatus == NodeStatus::Running) {
        status = NodeStatus::Running;
        return status;
    }
    
    ++currentRepeats;
    
    if (maxRepeats > 0 && currentRepeats >= maxRepeats) {
        currentRepeats = 0;
        status = NodeStatus::Success;
    } else {
        child->reset();
        status = NodeStatus::Running;
    }
    
    return status;
}

void RepeaterNode::reset() {
    currentRepeats = 0;
    DecoratorNode::reset();
}

SucceederNode::SucceederNode(const std::string& name)
    : DecoratorNode(name) {}

NodeStatus SucceederNode::tick(float deltaTime) {
    if (child) {
        child->tick(deltaTime);
    }
    
    status = NodeStatus::Success;
    return status;
}

BehaviorTree::BehaviorTree() {}

BehaviorTree::~BehaviorTree() {}

void BehaviorTree::setRoot(std::unique_ptr<BehaviorNode> root) {
    this->root = std::move(root);
}

NodeStatus BehaviorTree::tick(float deltaTime) {
    if (root) {
        return root->tick(deltaTime);
    }
    return NodeStatus::Failure;
}

void BehaviorTree::reset() {
    if (root) {
        root->reset();
    }
}

} // namespace AI
} // namespace JJM
