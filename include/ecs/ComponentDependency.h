#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <typeindex>
#include <string>
#include <functional>

/**
 * @file ComponentDependency.h
 * @brief Component dependency tracking and validation system
 * 
 * Manages component dependencies and requirements, ensuring that
 * components are added in the correct order and required dependencies
 * are satisfied.
 */

namespace Engine {

/**
 * @enum DependencyType
 * @brief Types of component dependencies
 */
enum class DependencyType {
    Required,     ///< Component must be present
    Optional,     ///< Component is optional but will be used if present
    Incompatible  ///< Component cannot coexist with this one
};

/**
 * @struct ComponentDependency
 * @brief Describes a dependency relationship between components
 */
struct ComponentDependency {
    std::type_index componentType;
    DependencyType type;
    std::string description;
    
    ComponentDependency(std::type_index type, DependencyType depType, const std::string& desc = "")
        : componentType(type), type(depType), description(desc) {}
};

/**
 * @class ComponentDependencyRegistry
 * @brief Registry for managing component dependency relationships
 */
class ComponentDependencyRegistry {
public:
    static ComponentDependencyRegistry& getInstance();
    
    /**
     * @brief Register a required dependency
     * @param component Component type that has the dependency
     * @param required Required component type
     * @param description Optional description
     */
    template<typename ComponentType, typename RequiredType>
    void registerRequiredDependency(const std::string& description = "") {
        registerDependency(typeid(ComponentType), typeid(RequiredType), 
                         DependencyType::Required, description);
    }
    
    /**
     * @brief Register an optional dependency
     * @param component Component type that has the dependency
     * @param optional Optional component type
     * @param description Optional description
     */
    template<typename ComponentType, typename OptionalType>
    void registerOptionalDependency(const std::string& description = "") {
        registerDependency(typeid(ComponentType), typeid(OptionalType), 
                         DependencyType::Optional, description);
    }
    
    /**
     * @brief Register an incompatible component
     * @param component Component type
     * @param incompatible Incompatible component type
     * @param description Optional description
     */
    template<typename ComponentType, typename IncompatibleType>
    void registerIncompatibility(const std::string& description = "") {
        registerDependency(typeid(ComponentType), typeid(IncompatibleType), 
                         DependencyType::Incompatible, description);
    }
    
    /**
     * @brief Get all dependencies for a component type
     * @param componentType Component type
     * @return Vector of dependencies
     */
    std::vector<ComponentDependency> getDependencies(std::type_index componentType) const;
    
    /**
     * @brief Check if dependencies are satisfied
     * @param componentType Component to check
     * @param presentComponents Currently present component types
     * @param outMissing Output vector of missing required dependencies
     * @param outIncompatible Output vector of incompatible components present
     * @return True if all requirements satisfied
     */
    bool validateDependencies(std::type_index componentType,
                             const std::vector<std::type_index>& presentComponents,
                             std::vector<std::type_index>& outMissing,
                             std::vector<std::type_index>& outIncompatible) const;
    
    /**
     * @brief Get topological order for adding components
     * @param components Component types to add
     * @param outOrder Output vector with correct order
     * @return True if valid order exists (no circular dependencies)
     */
    bool getAddOrder(const std::vector<std::type_index>& components,
                    std::vector<std::type_index>& outOrder) const;
    
    /**
     * @brief Clear all registered dependencies
     */
    void clear();
    
    /**
     * @brief Get component type name (for debugging)
     * @param type Component type
     * @return Component name
     */
    std::string getComponentName(std::type_index type) const;
    
    /**
     * @brief Register a component name for better error messages
     * @param type Component type
     * @param name Friendly name
     */
    void registerComponentName(std::type_index type, const std::string& name);

private:
    ComponentDependencyRegistry() = default;
    ComponentDependencyRegistry(const ComponentDependencyRegistry&) = delete;
    ComponentDependencyRegistry& operator=(const ComponentDependencyRegistry&) = delete;
    
    void registerDependency(std::type_index component, std::type_index dependency,
                          DependencyType type, const std::string& description);
    
    std::unordered_map<std::type_index, std::vector<ComponentDependency>> m_dependencies;
    std::unordered_map<std::type_index, std::string> m_componentNames;
};

/**
 * @class ComponentValidator
 * @brief Validates component configurations on entities
 */
class ComponentValidator {
public:
    /**
     * @struct ValidationResult
     * @brief Result of component validation
     */
    struct ValidationResult {
        bool valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        
        ValidationResult() : valid(true) {}
    };
    
    /**
     * @brief Validate entity's component configuration
     * @param componentTypes Types of components on the entity
     * @return Validation result
     */
    static ValidationResult validate(const std::vector<std::type_index>& componentTypes);
    
    /**
     * @brief Check if adding a component would be valid
     * @param newComponent Component to add
     * @param existingComponents Currently present components
     * @return Validation result
     */
    static ValidationResult validateAdd(std::type_index newComponent,
                                       const std::vector<std::type_index>& existingComponents);
    
    /**
     * @brief Check if removing a component would break dependencies
     * @param componentToRemove Component to remove
     * @param remainingComponents Components that would remain
     * @return Validation result
     */
    static ValidationResult validateRemove(std::type_index componentToRemove,
                                          const std::vector<std::type_index>& remainingComponents);
};

/**
 * @class DependencyGraph
 * @brief Builds and analyzes component dependency graphs
 */
class DependencyGraph {
public:
    /**
     * @brief Build dependency graph for a set of components
     * @param components Component types
     */
    void build(const std::vector<std::type_index>& components);
    
    /**
     * @brief Check for circular dependencies
     * @param outCycle Output vector containing cycle if found
     * @return True if circular dependency exists
     */
    bool hasCircularDependency(std::vector<std::type_index>& outCycle) const;
    
    /**
     * @brief Get topological sort of components
     * @param outOrder Output vector with sorted components
     * @return True if sort successful (no cycles)
     */
    bool topologicalSort(std::vector<std::type_index>& outOrder) const;
    
    /**
     * @brief Get all dependencies of a component (recursive)
     * @param component Component type
     * @param outDeps Output set of all dependencies
     */
    void getAllDependencies(std::type_index component, 
                           std::unordered_set<std::type_index>& outDeps) const;
    
    /**
     * @brief Get immediate dependencies of a component
     * @param component Component type
     * @return Vector of immediate dependencies
     */
    std::vector<std::type_index> getImmediateDependencies(std::type_index component) const;
    
    /**
     * @brief Clear the graph
     */
    void clear();

private:
    struct Node {
        std::type_index type;
        std::vector<std::type_index> dependencies;
        std::vector<std::type_index> dependents;
    };
    
    std::unordered_map<std::type_index, Node> m_nodes;
    
    bool dfsHasCycle(std::type_index current,
                    std::unordered_set<std::type_index>& visited,
                    std::unordered_set<std::type_index>& recursionStack,
                    std::vector<std::type_index>& cycle) const;
    
    void dfsTopological(std::type_index current,
                       std::unordered_set<std::type_index>& visited,
                       std::vector<std::type_index>& stack) const;
};

/**
 * @brief Helper macro to register component dependencies
 */
#define REGISTER_COMPONENT_DEPENDENCY(ComponentType, RequiredType, description) \
    static struct ComponentType##RequiredType##Registration { \
        ComponentType##RequiredType##Registration() { \
            Engine::ComponentDependencyRegistry::getInstance().registerRequiredDependency<ComponentType, RequiredType>(description); \
            Engine::ComponentDependencyRegistry::getInstance().registerComponentName(typeid(ComponentType), #ComponentType); \
        } \
    } ComponentType##RequiredType##registration_instance;

} // namespace Engine
