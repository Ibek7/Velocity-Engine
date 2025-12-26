#include "memory/ObjectPool.h"

namespace Engine {

PoolManager& PoolManager::getInstance() {
    static PoolManager instance;
    return instance;
}

void PoolManager::clearPool(const std::string& poolName) {
    auto it = m_pools.find(poolName);
    if (it != m_pools.end()) {
        m_pools.erase(it);
    }
}

void PoolManager::clearAllPools() {
    m_pools.clear();
}

} // namespace Engine
