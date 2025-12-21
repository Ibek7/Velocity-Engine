#include "gameplay/InventorySystem.h"

namespace JJM {
namespace Gameplay {

// Inventory implementation
Inventory::Inventory(int cap) : capacity(cap) {
    slots.resize(capacity);
}

Inventory::~Inventory() {
}

bool Inventory::addItem(std::shared_ptr<Item> item, int quantity) {
    if (!item || quantity <= 0) return false;
    
    int remaining = quantity;
    
    for (auto& slot : slots) {
        if (!slot.isEmpty() && slot.item->id == item->id) {
            int space = item->maxStack - slot.quantity;
            if (space > 0) {
                int toAdd = std::min(space, remaining);
                slot.quantity += toAdd;
                remaining -= toAdd;
                
                if (remaining == 0) return true;
            }
        }
    }
    
    for (auto& slot : slots) {
        if (slot.isEmpty()) {
            slot.item = item;
            int toAdd = std::min(item->maxStack, remaining);
            slot.quantity = toAdd;
            remaining -= toAdd;
            
            if (remaining == 0) return true;
        }
    }
    
    return remaining == 0;
}

bool Inventory::removeItem(const std::string& itemId, int quantity) {
    if (quantity <= 0) return false;
    
    int remaining = quantity;
    
    for (auto& slot : slots) {
        if (!slot.isEmpty() && slot.item->id == itemId) {
            int toRemove = std::min(slot.quantity, remaining);
            slot.quantity -= toRemove;
            remaining -= toRemove;
            
            if (slot.quantity == 0) {
                slot.item = nullptr;
            }
            
            if (remaining == 0) return true;
        }
    }
    
    return remaining == 0;
}

bool Inventory::hasItem(const std::string& itemId, int quantity) const {
    return getItemCount(itemId) >= quantity;
}

int Inventory::getItemCount(const std::string& itemId) const {
    int count = 0;
    for (const auto& slot : slots) {
        if (!slot.isEmpty() && slot.item->id == itemId) {
            count += slot.quantity;
        }
    }
    return count;
}

InventorySlot* Inventory::getSlot(int index) {
    if (index >= 0 && index < capacity) {
        return &slots[index];
    }
    return nullptr;
}

const InventorySlot* Inventory::getSlot(int index) const {
    if (index >= 0 && index < capacity) {
        return &slots[index];
    }
    return nullptr;
}

int Inventory::getCapacity() const {
    return capacity;
}

int Inventory::getUsedSlots() const {
    int used = 0;
    for (const auto& slot : slots) {
        if (!slot.isEmpty()) used++;
    }
    return used;
}

float Inventory::getTotalWeight() const {
    float weight = 0.0f;
    for (const auto& slot : slots) {
        if (!slot.isEmpty()) {
            weight += slot.item->weight * slot.quantity;
        }
    }
    return weight;
}

void Inventory::clear() {
    for (auto& slot : slots) {
        slot.item = nullptr;
        slot.quantity = 0;
    }
}

std::vector<InventorySlot*> Inventory::getAllSlots() {
    std::vector<InventorySlot*> result;
    for (auto& slot : slots) {
        result.push_back(&slot);
    }
    return result;
}

// ItemDatabase implementation
ItemDatabase& ItemDatabase::getInstance() {
    static ItemDatabase instance;
    return instance;
}

ItemDatabase::ItemDatabase() {
}

ItemDatabase::~ItemDatabase() {
}

void ItemDatabase::registerItem(std::shared_ptr<Item> item) {
    if (item) {
        items[item->id] = item;
    }
}

std::shared_ptr<Item> ItemDatabase::getItem(const std::string& id) {
    auto it = items.find(id);
    return it != items.end() ? it->second : nullptr;
}

std::shared_ptr<Item> ItemDatabase::createItem(const std::string& id) {
    auto prototype = getItem(id);
    if (!prototype) return nullptr;
    
    auto newItem = std::make_shared<Item>(*prototype);
    return newItem;
}

} // namespace Gameplay
} // namespace JJM
