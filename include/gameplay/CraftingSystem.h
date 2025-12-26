#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// Crafting system for item creation
namespace Engine {

struct CraftingIngredient {
    std::string itemId;
    int quantity;
};

struct CraftingRecipe {
    std::string recipeId;
    std::string name;
    std::string resultItemId;
    int resultQuantity;
    std::vector<CraftingIngredient> ingredients;
    float craftTime;
    int requiredLevel;
    std::string requiredStation; // e.g., "workbench", "forge"
};

class CraftingSystem {
public:
    static CraftingSystem& getInstance();

    // Recipe management
    void registerRecipe(const CraftingRecipe& recipe);
    void unregisterRecipe(const std::string& recipeId);
    const CraftingRecipe* getRecipe(const std::string& recipeId) const;
    std::vector<std::string> getAllRecipes() const;
    
    // Crafting
    bool canCraft(const std::string& recipeId, const std::unordered_map<std::string, int>& inventory, 
                 int playerLevel, const std::string& currentStation) const;
    bool craft(const std::string& recipeId, std::unordered_map<std::string, int>& inventory, 
              int playerLevel, const std::string& currentStation);
    
    // Query
    std::vector<std::string> getAvailableRecipes(const std::unordered_map<std::string, int>& inventory,
                                                 int playerLevel, const std::string& currentStation) const;
    void getRecipesByResult(const std::string& itemId, std::vector<std::string>& results) const;
    void getRecipesByIngredient(const std::string& itemId, std::vector<std::string>& results) const;
    
    int getRecipeCount() const { return m_recipes.size(); }

private:
    CraftingSystem();
    CraftingSystem(const CraftingSystem&) = delete;
    CraftingSystem& operator=(const CraftingSystem&) = delete;

    bool hasIngredients(const CraftingRecipe& recipe, 
                       const std::unordered_map<std::string, int>& inventory) const;
    void consumeIngredients(const CraftingRecipe& recipe, 
                           std::unordered_map<std::string, int>& inventory);

    std::unordered_map<std::string, CraftingRecipe> m_recipes;
};

} // namespace Engine
