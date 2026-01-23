#include "ecs/ComponentDependency.h"
#include <algorithm>
#include <sstream>

namespace Engine {

// ComponentDependencyRegistry Implementation
ComponentDependencyRegistry& ComponentDependencyRegistry::getInstance() {
    static ComponentDependencyRegistry instance;
    return instance;
}

std::vector<ComponentDependency> ComponentDependencyRegistry::getDependencies(std::type_index componentType) const {
    auto it = m_dependencies.find(componentType);
    if (it != m_dependencies.end()) {
        return it->second;
    }
    return {};
}

bool ComponentDependencyRegistry::validateDependencies(
    std::type_index componentType,
    const std::vector<std::type_index>& presentComponents,
    std::vector<std::type_index>& outMissing,
    std::vector<std::type_index>& outIncompatible) const {
    
    outMissing.clear();
    outIncompatible.clear();
    
    auto deps = getDependencies(componentType);
    
    for (const auto& dep : deps) {
        bool isPresent = std::find(presentComponents.begin(), presentComponents.end(), 
                                   dep.componentType) != presentComponents.end();
        
        if (dep.type == DependencyType::Required && !isPresent) {
            outMissing.push_back(dep.componentType);
        } else if (dep.type == DependencyType::Incompatible && isPresent) {
            outIncompatible.push_back(dep.componentType);
        }
    }
    
    return outMissing.empty() && outIncompatible.empty();
}

bool ComponentDependencyRegistry::getAddOrder(
    const std::vector<std::type_index>& components,
    std::vector<std::type_index>& outOrder) const {
    
    DependencyGraph graph;
    graph.build(components);
    
    std::vector<std::type_index> cycle;
    if (graph.hasCircularDependency(cycle)) {
        return false;
    }
    
    return graph.topologicalSort(outOrder);
}

void ComponentDependencyRegistry::clear() {
    m_dependencies.clear();
    m_componentNames.clear();
}

std::string ComponentDependencyRegistry::getComponentName(std::type_index type) const {
    auto it = m_componentNames.find(type);
    if (it != m_componentNames.end()) {
        return it->second;
    }
    return type.name();
}

void ComponentDependencyRegistry::registerComponentName(std::type_index type, const std::string& name) {
    m_componentNames[type] = name;
}

void ComponentDependencyRegistry::registerDependency(
    std::type_index component,
    std::type_index dependency,
    DependencyType type,
    const std::string& description) {
    
    ComponentDependency dep(dependency, type, description);
    m_dependencies[component].push_back(dep);
}

// ComponentValidator Implementation
ComponentValidator::ValidationResult ComponentValidator::validate(
    const std::vector<std::type_index>& componentTypes) {
    
    ValidationResult result;
    auto& registry = ComponentDependencyRegistry::getInstance();
    
    for (const auto& compType : componentTypes) {
        std::vector<std::type_index> missing, incompatible;
        
        if (!registry.validateDependencies(compType, componentTypes, missing, incompatible)) {
            result.valid = false;
            
            for (const auto& missingType : missing) {
                std::stringstream ss;
                ss << "Component '" << registry.getComponentName(compType)
                   << "' requires '" << registry.getComponentName(missingType)
                   << "' but it is not present";
                result.errors.push_back(ss.str());
            }
            
            for (const auto& incompType : incompatible) {
                std::stringstream ss;
                ss << "Component '" << registry.getComponentName(compType)
                   << "' is incompatible with '" << registry.getComponentName(incompType) << "'";
                result.errors.push_back(ss.str());
            }
        }
    }
    
    // Check for circular dependencies
    DependencyGraph graph;
    graph.build(componentTypes);
    std::vector<std::type_index> cycle;
    if (graph.hasCircularDependency(cycle)) {
        result.valid = false;
        std::stringstream ss;
        ss << "Circular dependency detected: ";
        for (size_t i = 0; i < cycle.size(); ++i) {
            if (i > 0) ss << " -> ";
            ss << registry.getComponentName(cycle[i]);
        }
        result.errors.push_back(ss.str());
    }
    
    return result;
}

ComponentValidator::ValidationResult ComponentValidator::validateAdd(
    std::type_index newComponent,
    const std::vector<std::type_index>& existingComponents) {
    
    auto& registry = ComponentDependencyRegistry::getInstance();
    ValidationResult result;
    
    // Check if new component's dependencies are satisfied
    std::vector<std::type_index> missing, incompatible;
    if (!registry.validateDependencies(newComponent, existingComponents, missing, incompatible)) {
        result.valid = false;
        
        for (const auto& missingType : missing) {
            std::stringstream ss;
            ss << "Cannot add '" << registry.getComponentName(newComponent)
               << "' because required component '" << registry.getComponentName(missingType)
               << "' is not present";
            result.errors.push_back(ss.str());
        }
        
        for (const auto& incompType : incompatible) {
            std::stringstream ss;
            ss << "Cannot add '" << registry.getComponentName(newComponent)
               << "' because it is incompatible with existing component '"
               << registry.getComponentName(incompType) << "'";
            result.errors.push_back(ss.str());
        }
    }
    
    // Check if adding this component would break existing components
    for (const auto& existingComp : existingComponents) {
        std::vector<std::type_index> testComponents = existingComponents;
        testComponents.push_back(newComponent);
        
        std::vector<std::type_index> existingMissing, existingIncomp;
        if (!registry.validateDependencies(existingComp, testComponents, existingMissing, existingIncomp)) {
            if (!existingIncomp.empty()) {
                result.valid = false;
                std::stringstream ss;
                ss << "Adding '" << registry.getComponentName(newComponent)
                   << "' would conflict with existing component '"
                   << registry.getComponentName(existingComp) << "'";
                result.errors.push_back(ss.str());
            }
        }
    }
    
    return result;
}

ComponentValidator::ValidationResult ComponentValidator::validateRemove(
    std::type_index componentToRemove,
    const std::vector<std::type_index>& remainingComponents) {
    
    auto& registry = ComponentDependencyRegistry::getInstance();
    ValidationResult result;
    
    // Check if any remaining components depend on the one being removed
    for (const auto& remaining : remainingComponents) {
        std::vector<std::type_index> missing, incompatible;
        
        std::vector<std::type_index> testComponents = remainingComponents;
        // Don't include the component being removed
        
        if (!registry.validateDependencies(remaining, testComponents, missing, incompatible)) {
            for (const auto& missingType : missing) {
                if (missingType == componentToRemove) {
                    result.valid = false;
                    std::stringstream ss;
                    ss << "Cannot remove '" << registry.getComponentName(componentToRemove)
                       << "' because '" << registry.getComponentName(remaining)
                       << "' depends on it";
                    result.errors.push_back(ss.str());
                }
            }
        }
    }
    
    return result;
}

// DependencyGraph Implementation
void DependencyGraph::build(const std::vector<std::type_index>& components) {
    m_nodes.clear();
    auto& registry = ComponentDependencyRegistry::getInstance();
    
    // Create nodes
    for (const auto& compType : components) {
        Node node;
        node.type = compType;
        
        auto deps = registry.getDependencies(compType);
        for (const auto& dep : deps) {
            if (dep.type == DependencyType::Required) {
                node.dependencies.push_back(dep.componentType);
            }
        }
        
        m_nodes[compType] = node;
    }
    
    // Build reverse edges (dependents)
    for (auto& pair : m_nodes) {
        for (const auto& dep : pair.second.dependencies) {
            auto it = m_nodes.find(dep);
            if (it != m_nodes.end()) {
                it->second.dependents.push_back(pair.first);
            }
        }
    }
}

bool DependencyGraph::hasCircularDependency(std::vector<std::type_index>& outCycle) const {
    std::unordered_set<std::type_index> visited;
    std::unordered_set<std::type_index> recursionStack;
    outCycle.clear();
    
    for (const auto& pair : m_nodes) {
        if (visited.find(pair.first) == visited.end()) {
            if (dfsHasCycle(pair.first, visited, recursionStack, outCycle)) {
                return true;
            }
        }
    }
    
    return false;
}

bool DependencyGraph::topologicalSort(std::vector<std::type_index>& outOrder) const {
    std::unordered_set<std::type_index> visited;
    std::vector<std::type_index> stack;
    
    for (const auto& pair : m_nodes) {
        if (visited.find(pair.first) == visited.end()) {
            dfsTopological(pair.first, visited, stack);
        }
    }
    
    outOrder = stack;
    std::reverse(outOrder.begin(), outOrder.end());
    return true;
}

void DependencyGraph::getAllDependencies(
    std::type_index component,
    std::unordered_set<std::type_index>& outDeps) const {
    
    auto it = m_nodes.find(component);
    if (it == m_nodes.end()) return;
    
    for (const auto& dep : it->second.dependencies) {
        if (outDeps.insert(dep).second) {
            getAllDependencies(dep, outDeps);
        }
    }
}

std::vector<std::type_index> DependencyGraph::getImmediateDependencies(std::type_index component) const {
    auto it = m_nodes.find(component);
    if (it != m_nodes.end()) {
        return it->second.dependencies;
    }
    return {};
}

void DependencyGraph::clear() {
    m_nodes.clear();
}

bool DependencyGraph::dfsHasCycle(
    std::type_index current,
    std::unordered_set<std::type_index>& visited,
    std::unordered_set<std::type_index>& recursionStack,
    std::vector<std::type_index>& cycle) const {
    
    visited.insert(current);
    recursionStack.insert(current);
    cycle.push_back(current);
    
    auto it = m_nodes.find(current);
    if (it != m_nodes.end()) {
        for (const auto& dep : it->second.dependencies) {
            if (recursionStack.find(dep) != recursionStack.end()) {
                // Found cycle
                auto cycleStart = std::find(cycle.begin(), cycle.end(), dep);
                if (cycleStart != cycle.end()) {
                    cycle.erase(cycle.begin(), cycleStart);
                }
                cycle.push_back(dep);
                return true;
            }
            
            if (visited.find(dep) == visited.end()) {
                if (dfsHasCycle(dep, visited, recursionStack, cycle)) {
                    return true;
                }
            }
        }
    }
    
    recursionStack.erase(current);
    cycle.pop_back();
    return false;
}

void DependencyGraph::dfsTopological(
    std::type_index current,
    std::unordered_set<std::type_index>& visited,
    std::vector<std::type_index>& stack) const {
    
    visited.insert(current);
    
    auto it = m_nodes.find(current);
    if (it != m_nodes.end()) {
        for (const auto& dep : it->second.dependencies) {
            if (visited.find(dep) == visited.end()) {
                dfsTopological(dep, visited, stack);
            }
        }
    }
    
    stack.push_back(current);
}

} // namespace Engine
