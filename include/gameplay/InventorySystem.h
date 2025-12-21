#ifndef JJM_INVENTORY_SYSTEM_H
#define JJM_INVENTORY_SYSTEM_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace JJM {
namespace Gameplay {

struct Item {
    std::string id;
    std::string name;
    std::string description;
    int maxStack;
    float weight;
    std::map<std::string, std::string> properties;
};

struct InventorySlot {
    std::shared_ptr<Item> item;
    int quantity;
    
    bool isEmpty() const { return item == nullptr || quantity == 0; }
};

class Inventory {
public:
    Inventory(int capacity);
    ~Inventory();
    
    bool addItem(std::shared_ptr<Item> item, int quantity = 1);
    bool removeItem(const std::string& itemId, int quantity = 1);
    bool hasItem(const std::string& itemId, int quantity = 1) const;
    
    int getItemCount(const std::string& itemId) const;
    InventorySlot* getSlot(int index);
    const InventorySlot* getSlot(int index) const;
    
    int getCapacity() const;
    int getUsedSlots() const;
    float getTotalWeight() const;
    
    void clear();
    std::vector<InventorySlot*> getAllSlots();

private:
    std::vector<InventorySlot> slots;
    int capacity;
};

class ItemDatabase {
public:
    static ItemDatabase& getInstance();
    
    void registerItem(std::shared_ptr<Item> item);
    std::shared_ptr<Item> getItem(const std::string& id);
    std::shared_ptr<Item> createItem(const std::string& id);

private:
    ItemDatabase();
    ~ItemDatabase();
    
    std::map<std::string, std::shared_ptr<Item>> items;
};

} // namespace Gameplay
} // namespace JJM

#endif
