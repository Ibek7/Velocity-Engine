#pragma once

#include <vector>
#include <functional>
#include <string>
#include <memory>

/**
 * @file DecisionTree.h
 * @brief Decision tree system for AI decision making
 * 
 * Provides a flexible decision tree framework for AI agents to
 * make context-aware decisions based on game state.
 */

namespace Engine {

/**
 * @enum DecisionResult
 * @brief Result of a decision node evaluation
 */
enum class DecisionResult {
    Success,
    Failure,
    Running
};

/**
 * @class DecisionNode
 * @brief Base class for decision tree nodes
 */
class DecisionNode {
public:
    virtual ~DecisionNode() = default;
    
    /**
     * @brief Evaluate this node
     * @param context Context data for evaluation
     * @return Decision result
     */
    virtual DecisionResult evaluate(void* context) = 0;
    
    /**
     * @brief Get node description
     * @return Description string
     */
    virtual std::string getDescription() const = 0;
    
    /**
     * @brief Reset node state
     */
    virtual void reset() {}
};

/**
 * @class DecisionLeaf
 * @brief Leaf node that executes an action
 */
class DecisionLeaf : public DecisionNode {
public:
    using ActionFunction = std::function<DecisionResult(void*)>;
    
    DecisionLeaf(const std::string& name, ActionFunction action)
        : m_name(name), m_action(action) {}
    
    DecisionResult evaluate(void* context) override {
        return m_action(context);
    }
    
    std::string getDescription() const override {
        return "Action: " + m_name;
    }

private:
    std::string m_name;
    ActionFunction m_action;
};

/**
 * @class DecisionCondition
 * @brief Condition node with true/false branches
 */
class DecisionCondition : public DecisionNode {
public:
    using ConditionFunction = std::function<bool(void*)>;
    
    DecisionCondition(const std::string& name, ConditionFunction condition)
        : m_name(name), m_condition(condition) {}
    
    void setTrueBranch(std::unique_ptr<DecisionNode> branch) {
        m_trueBranch = std::move(branch);
    }
    
    void setFalseBranch(std::unique_ptr<DecisionNode> branch) {
        m_falseBranch = std::move(branch);
    }
    
    DecisionResult evaluate(void* context) override {
        if (m_condition(context)) {
            return m_trueBranch ? m_trueBranch->evaluate(context) : DecisionResult::Success;
        } else {
            return m_falseBranch ? m_falseBranch->evaluate(context) : DecisionResult::Failure;
        }
    }
    
    std::string getDescription() const override {
        return "Condition: " + m_name;
    }
    
    void reset() override {
        if (m_trueBranch) m_trueBranch->reset();
        if (m_falseBranch) m_falseBranch->reset();
    }

private:
    std::string m_name;
    ConditionFunction m_condition;
    std::unique_ptr<DecisionNode> m_trueBranch;
    std::unique_ptr<DecisionNode> m_falseBranch;
};

/**
 * @class DecisionSequence
 * @brief Executes children in sequence until one fails
 */
class DecisionSequence : public DecisionNode {
public:
    void addChild(std::unique_ptr<DecisionNode> child) {
        m_children.push_back(std::move(child));
    }
    
    DecisionResult evaluate(void* context) override {
        for (auto& child : m_children) {
            DecisionResult result = child->evaluate(context);
            if (result != DecisionResult::Success) {
                return result;
            }
        }
        return DecisionResult::Success;
    }
    
    std::string getDescription() const override {
        return "Sequence (" + std::to_string(m_children.size()) + " children)";
    }
    
    void reset() override {
        for (auto& child : m_children) {
            child->reset();
        }
    }

private:
    std::vector<std::unique_ptr<DecisionNode>> m_children;
};

/**
 * @class DecisionSelector
 * @brief Tries children until one succeeds
 */
class DecisionSelector : public DecisionNode {
public:
    void addChild(std::unique_ptr<DecisionNode> child) {
        m_children.push_back(std::move(child));
    }
    
    DecisionResult evaluate(void* context) override {
        for (auto& child : m_children) {
            DecisionResult result = child->evaluate(context);
            if (result != DecisionResult::Failure) {
                return result;
            }
        }
        return DecisionResult::Failure;
    }
    
    std::string getDescription() const override {
        return "Selector (" + std::to_string(m_children.size()) + " children)";
    }
    
    void reset() override {
        for (auto& child : m_children) {
            child->reset();
        }
    }

private:
    std::vector<std::unique_ptr<DecisionNode>> m_children;
};

/**
 * @class DecisionTree
 * @brief Complete decision tree for AI
 */
class DecisionTree {
public:
    DecisionTree();
    ~DecisionTree();
    
    /**
     * @brief Set root node
     * @param root Root decision node
     */
    void setRoot(std::unique_ptr<DecisionNode> root);
    
    /**
     * @brief Evaluate tree
     * @param context Context data
     * @return Decision result
     */
    DecisionResult evaluate(void* context);
    
    /**
     * @brief Reset tree state
     */
    void reset();
    
    /**
     * @brief Get tree description (for debugging)
     * @return Tree structure as string
     */
    std::string getTreeDescription() const;
    
    /**
     * @brief Enable debug logging
     * @param enable Enable flag
     */
    void setDebugMode(bool enable) { m_debugMode = enable; }

private:
    std::unique_ptr<DecisionNode> m_root;
    bool m_debugMode;
};

/**
 * @class DecisionTreeBuilder
 * @brief Fluent API for building decision trees
 */
class DecisionTreeBuilder {
public:
    DecisionTreeBuilder();
    
    /**
     * @brief Add condition node
     * @param name Condition name
     * @param condition Condition function
     * @return Builder reference
     */
    DecisionTreeBuilder& condition(const std::string& name,
                                  std::function<bool(void*)> condition);
    
    /**
     * @brief Add action node
     * @param name Action name
     * @param action Action function
     * @return Builder reference
     */
    DecisionTreeBuilder& action(const std::string& name,
                               std::function<DecisionResult(void*)> action);
    
    /**
     * @brief Start sequence block
     * @return Builder reference
     */
    DecisionTreeBuilder& sequence();
    
    /**
     * @brief Start selector block
     * @return Builder reference
     */
    DecisionTreeBuilder& selector();
    
    /**
     * @brief End current block
     * @return Builder reference
     */
    DecisionTreeBuilder& end();
    
    /**
     * @brief Build the decision tree
     * @return Complete decision tree
     */
    std::unique_ptr<DecisionTree> build();

private:
    struct BuildState {
        enum Type { Root, Sequence, Selector, Condition };
        Type type;
        std::unique_ptr<DecisionNode> node;
        DecisionCondition* conditionPtr;
        bool hasSetTrueBranch;
    };
    
    std::vector<BuildState> m_stack;
    std::unique_ptr<DecisionNode> m_root;
};

} // namespace Engine
