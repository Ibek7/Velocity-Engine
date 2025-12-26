#include "gameplay/CraftingSystem.h"

namespace Engine {

CraftingSystem::CraftingSystem() {
}

CraftingSystem& CraftingSystem::getInstance() {
    static CraftingSystem instance;
    return instance;
}

void CraftingSystem::registerRecipe(const CraftingRecipe& recipe) {
    m_recipes[recipe.recipeId] = recipe;
}

void CraftingSystem::unregisterRecipe(const std::string& recipeId) {
    m_recipes.erase(recipeId);
}

const CraftingRecipe* CraftingSystem::getRecipe(const std::string& recipeId) const {
    auto it = m_recipes.find(recipeId);
    if (it != m_recipes.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> CraftingSystem::getAllRecipes() const {
    std::vector<std::string> recipeIds;
    for (const auto& pair : m_recipes) {
        recipeIds.push_back(pair.first);
    }
    return recipeIds;
}

bool CraftingSystem::canCraft(const std::string& recipeId, 
                              const std::unordered_map<std::string, int>& inventory,
                              int playerLevel, const std::string& currentStation) const {
    const CraftingRecipe* recipe = getRecipe(recipeId);
    if (!recipe) {
        return false;
    }
    
    // Check level requirement
    if (playerLevel < recipe->requiredLevel) {
        return false;
    }
    
    // Check station requirement
    if (!recipe->requiredStation.empty() && recipe->requiredStation != currentStation) {
        return false;
    }
    
    // Check ingredients
    return hasIngredients(*recipe, inventory);
}

bool CraftingSystem::craft(const std::string& recipeId, 
                           std::unordered_map<std::string, int>& inventory,
                           int playerLevel, const std::string& currentStation) {
    const CraftingRecipe* recipe = getRecipe(recipeId);
    if (!recipe) {
        return false;
    }
    
    if (!canCraft(recipeId, inventory, playerLevel, currentStation)) {
        return false;
    }
    
    // Consume ingredients
    consumeIngredients(*recipe, inventory);
    
    // Add result
    inventory[recipe->resultItemId] += recipe->resultQuantity;
    
    return true;
}

std::vector<std::string> CraftingSystem::getAvailableRecipes(
    const std::unordered_map<std::string, int>& inventory,
    int playerLevel, const std::string& currentStation) const {
    
    std::vector<std::string> available;
    
    for (const auto& pair : m_recipes) {
        if (canCraft(pair.first, inventory, playerLevel, currentStation)) {
            available.push_back(pair.first);
        }
    }
    
    return available;
}

void CraftingSystem::getRecipesByResult(const std::string& itemId, std::vector<std::string>& results) const {
    results.clear();
    
    for (const auto& pair : m_recipes) {
        if (pair.second.resultItemId == itemId) {
            results.push_back(pair.first);
        }
    }
}

void CraftingSystem::getRecipesByIngredient(const std::string& itemId, std::vector<std::string>& results) const {
    results.clear();
    
    for (const auto& pair : m_recipes) {
        for (const auto& ingredient : pair.second.ingredients) {
            if (ingredient.itemId == itemId) {
                results.push_back(pair.first);
                break;
            }
        }
    }
}

bool CraftingSystem::hasIngredients(const CraftingRecipe& recipe, 
                                   const std::unordered_map<std::string, int>& inventory) const {
    for (const auto& ingredient : recipe.ingredients) {
        auto it = inventory.find(ingredient.itemId);
        if (it == inventory.end() || it->second < ingredient.quantity) {
            return false;
        }
    }
    return true;
}

void CraftingSystem::consumeIngredients(const CraftingRecipe& recipe, 
                                       std::unordered_map<std::string, int>& inventory) {
    for (const auto& ingredient : recipe.ingredients) {
        inventory[ingredient.itemId] -= ingredient.quantity;
        
        // Remove item if quantity reaches 0
        if (inventory[ingredient.itemId] <= 0) {
            inventory.erase(ingredient.itemId);
        }
    }
}

} // namespace Engine
