#include "ai/DecisionTree.h"

#include <sstream>

namespace Engine {

// DecisionTree Implementation
DecisionTree::DecisionTree() : m_debugMode(false) {}

DecisionTree::~DecisionTree() {}

void DecisionTree::setRoot(std::unique_ptr<DecisionNode> root) { m_root = std::move(root); }

DecisionResult DecisionTree::evaluate(void* context) {
    if (!m_root) {
        return DecisionResult::Failure;
    }

    return m_root->evaluate(context);
}

void DecisionTree::reset() {
    if (m_root) {
        m_root->reset();
    }
}

std::string DecisionTree::getTreeDescription() const {
    if (!m_root) {
        return "Empty tree";
    }
    return m_root->getDescription();
}

// DecisionTreeBuilder Implementation
DecisionTreeBuilder::DecisionTreeBuilder() {
    BuildState rootState;
    rootState.type = BuildState::Root;
    m_stack.push_back(std::move(rootState));
}

DecisionTreeBuilder& DecisionTreeBuilder::condition(const std::string& name,
                                                    std::function<bool(void*)> condition) {
    auto node = std::make_unique<DecisionCondition>(name, condition);
    DecisionCondition* condPtr = node.get();

    BuildState state;
    state.type = BuildState::Condition;
    state.node = std::move(node);
    state.conditionPtr = condPtr;
    state.hasSetTrueBranch = false;

    m_stack.push_back(std::move(state));
    return *this;
}

DecisionTreeBuilder& DecisionTreeBuilder::action(const std::string& name,
                                                 std::function<DecisionResult(void*)> action) {
    auto node = std::make_unique<DecisionLeaf>(name, action);

    if (m_stack.empty()) {
        m_root = std::move(node);
        return *this;
    }

    auto& current = m_stack.back();

    if (current.type == BuildState::Sequence) {
        static_cast<DecisionSequence*>(current.node.get())->addChild(std::move(node));
    } else if (current.type == BuildState::Selector) {
        static_cast<DecisionSelector*>(current.node.get())->addChild(std::move(node));
    } else if (current.type == BuildState::Condition) {
        if (!current.hasSetTrueBranch) {
            current.conditionPtr->setTrueBranch(std::move(node));
            current.hasSetTrueBranch = true;
        } else {
            current.conditionPtr->setFalseBranch(std::move(node));
        }
    }

    return *this;
}

DecisionTreeBuilder& DecisionTreeBuilder::sequence() {
    auto node = std::make_unique<DecisionSequence>();

    BuildState state;
    state.type = BuildState::Sequence;
    state.node = std::move(node);

    m_stack.push_back(std::move(state));
    return *this;
}

DecisionTreeBuilder& DecisionTreeBuilder::selector() {
    auto node = std::make_unique<DecisionSelector>();

    BuildState state;
    state.type = BuildState::Selector;
    state.node = std::move(node);

    m_stack.push_back(std::move(state));
    return *this;
}

DecisionTreeBuilder& DecisionTreeBuilder::end() {
    if (m_stack.size() <= 1) {
        return *this;
    }

    BuildState completed = std::move(m_stack.back());
    m_stack.pop_back();

    auto& parent = m_stack.back();

    if (parent.type == BuildState::Root) {
        m_root = std::move(completed.node);
    } else if (parent.type == BuildState::Sequence) {
        static_cast<DecisionSequence*>(parent.node.get())->addChild(std::move(completed.node));
    } else if (parent.type == BuildState::Selector) {
        static_cast<DecisionSelector*>(parent.node.get())->addChild(std::move(completed.node));
    } else if (parent.type == BuildState::Condition) {
        if (!parent.hasSetTrueBranch) {
            parent.conditionPtr->setTrueBranch(std::move(completed.node));
            parent.hasSetTrueBranch = true;
        } else {
            parent.conditionPtr->setFalseBranch(std::move(completed.node));
        }
    }

    return *this;
}

std::unique_ptr<DecisionTree> DecisionTreeBuilder::build() {
    auto tree = std::make_unique<DecisionTree>();

    if (m_root) {
        tree->setRoot(std::move(m_root));
    }

    return tree;
}

}  // namespace Engine
