#ifndef JJM_OBJECT_POOL_H
#define JJM_OBJECT_POOL_H

#include <vector>
#include <memory>
#include <functional>
#include <mutex>

namespace JJM {
namespace Memory {

template<typename T>
class ObjectPool {
public:
    ObjectPool(size_t initialSize = 10);
    ~ObjectPool();
    
    T* acquire();
    void release(T* obj);
    
    size_t getSize() const;
    size_t getAvailableCount() const;
    
    void clear();
    void reserve(size_t count);

private:
    struct PoolObject {
        T* object;
        bool inUse;
    };
    
    std::vector<PoolObject> pool;
    mutable std::mutex mutex;
    
    T* createObject();
    void destroyObject(T* obj);
};

template<typename T>
ObjectPool<T>::ObjectPool(size_t initialSize) {
    reserve(initialSize);
}

template<typename T>
ObjectPool<T>::~ObjectPool() {
    clear();
}

template<typename T>
T* ObjectPool<T>::acquire() {
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& entry : pool) {
        if (!entry.inUse) {
            entry.inUse = true;
            return entry.object;
        }
    }
    
    T* newObj = createObject();
    pool.push_back({newObj, true});
    return newObj;
}

template<typename T>
void ObjectPool<T>::release(T* obj) {
    if (!obj) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& entry : pool) {
        if (entry.object == obj) {
            entry.inUse = false;
            return;
        }
    }
}

template<typename T>
size_t ObjectPool<T>::getSize() const {
    std::lock_guard<std::mutex> lock(mutex);
    return pool.size();
}

template<typename T>
size_t ObjectPool<T>::getAvailableCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    size_t count = 0;
    for (const auto& entry : pool) {
        if (!entry.inUse) count++;
    }
    return count;
}

template<typename T>
void ObjectPool<T>::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& entry : pool) {
        destroyObject(entry.object);
    }
    pool.clear();
}

template<typename T>
void ObjectPool<T>::reserve(size_t count) {
    std::lock_guard<std::mutex> lock(mutex);
    
    size_t currentSize = pool.size();
    if (count <= currentSize) return;
    
    for (size_t i = currentSize; i < count; ++i) {
        T* obj = createObject();
        pool.push_back({obj, false});
    }
}

template<typename T>
T* ObjectPool<T>::createObject() {
    return new T();
}

template<typename T>
void ObjectPool<T>::destroyObject(T* obj) {
    delete obj;
}

} // namespace Memory
} // namespace JJM

#endif
