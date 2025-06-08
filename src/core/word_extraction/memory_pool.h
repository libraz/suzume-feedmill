/**
 * @file memory_pool.h
 * @brief Memory pool implementation for efficient small object allocation
 */

#pragma once

#include <memory>
#include <vector>
#include <cstddef>
#include <mutex>

namespace suzume {
namespace core {

/**
 * @brief Simple memory pool for allocating objects of fixed size
 * 
 * This class provides efficient allocation for small objects of the same size,
 * reducing memory fragmentation and improving cache locality.
 */
template<typename T, size_t ChunkSize = 1024>
class MemoryPool {
public:
    MemoryPool() = default;
    
    /**
     * @brief Allocate memory for one object
     * @return Pointer to allocated memory
     */
    T* allocate() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (freeList_.empty()) {
            allocateChunk();
        }
        
        T* ptr = freeList_.back();
        freeList_.pop_back();
        return ptr;
    }
    
    /**
     * @brief Deallocate memory for one object
     * @param ptr Pointer to memory to deallocate
     */
    void deallocate(T* ptr) {
        if (!ptr) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        freeList_.push_back(ptr);
    }
    
    /**
     * @brief Get total number of allocated chunks
     * @return Number of chunks
     */
    size_t getChunkCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return chunks_.size();
    }
    
    /**
     * @brief Get total memory usage in bytes
     * @return Memory usage
     */
    size_t getMemoryUsage() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return chunks_.size() * ChunkSize * sizeof(T);
    }
    
private:
    /**
     * @brief Allocate a new chunk of memory
     */
    void allocateChunk() {
        // Allocate a new chunk
        auto chunk = std::make_unique<T[]>(ChunkSize);
        T* chunkPtr = chunk.get();
        
        // Add all objects in the chunk to the free list
        for (size_t i = 0; i < ChunkSize; ++i) {
            freeList_.push_back(chunkPtr + i);
        }
        
        // Store the chunk to keep it alive
        chunks_.push_back(std::move(chunk));
    }
    
    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<T[]>> chunks_;
    std::vector<T*> freeList_;
};

/**
 * @brief RAII wrapper for memory pool allocation
 */
template<typename T, size_t ChunkSize = 1024>
class PoolAllocated {
public:
    using PoolType = MemoryPool<T, ChunkSize>;
    
    struct PoolDeleter {
        PoolType* pool;
        
        void operator()(T* ptr) {
            if (ptr && pool) {
                ptr->~T();
                pool->deallocate(ptr);
            }
        }
    };
    
    /**
     * @brief Allocate object from pool
     * @param pool Memory pool to allocate from
     * @param args Arguments to construct the object
     * @return Unique pointer to allocated object
     */
    template<typename... Args>
    static std::unique_ptr<T, PoolDeleter> make_unique(PoolType& pool, Args&&... args) {
        T* ptr = pool.allocate();
        try {
            new(ptr) T(std::forward<Args>(args)...);
            return std::unique_ptr<T, PoolDeleter>(ptr, PoolDeleter{&pool});
        } catch (...) {
            pool.deallocate(ptr);
            throw;
        }
    }
};

} // namespace core
} // namespace suzume