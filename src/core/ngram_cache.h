/**
 * @file ngram_cache.h
 * @brief High-performance N-gram cache for optimized processing
 */

#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <optional>

namespace suzume {
namespace core {

/**
 * @brief Cache entry for N-gram data
 */
struct NGramCacheEntry {
    double score;
    uint32_t frequency;
    std::chrono::steady_clock::time_point lastAccess;
    
    NGramCacheEntry() : score(0.0), frequency(0), lastAccess(std::chrono::steady_clock::now()) {}
    
    NGramCacheEntry(double s, uint32_t f) 
        : score(s), frequency(f), lastAccess(std::chrono::steady_clock::now()) {}
};

/**
 * @brief High-performance LRU cache for N-gram processing
 * 
 * This class provides efficient caching of N-gram computation results,
 * reducing redundant calculations and improving overall performance.
 */
class NGramCache {
public:
    /**
     * @brief Constructor with configurable cache size
     * @param maxSize Maximum number of entries to cache
     * @param ttlMinutes Time-to-live for cache entries in minutes
     */
    explicit NGramCache(size_t maxSize = 10000, int ttlMinutes = 30);
    
    /**
     * @brief Get cached entry for N-gram
     * @param ngram N-gram text
     * @return Optional cache entry if found and valid
     */
    std::optional<NGramCacheEntry> get(const std::string& ngram);
    
    /**
     * @brief Put entry into cache
     * @param ngram N-gram text
     * @param score PMI score
     * @param frequency Frequency count
     */
    void put(const std::string& ngram, double score, uint32_t frequency);
    
    /**
     * @brief Clear all cache entries
     */
    void clear();
    
    /**
     * @brief Get cache statistics
     * @return Tuple of (hits, misses, total_entries)
     */
    std::tuple<size_t, size_t, size_t> getStats() const;
    
    /**
     * @brief Get cache hit rate
     * @return Hit rate as percentage (0.0 to 1.0)
     */
    double getHitRate() const;
    
    /**
     * @brief Remove expired entries
     * @return Number of entries removed
     */
    size_t cleanupExpired();
    
private:
    /**
     * @brief Remove least recently used entry
     */
    void evictLRU();
    
    /**
     * @brief Check if entry is expired
     * @param entry Cache entry to check
     * @return True if expired
     */
    bool isExpired(const NGramCacheEntry& entry) const;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, NGramCacheEntry> cache_;
    size_t maxSize_;
    std::chrono::minutes ttl_;
    
    // Statistics
    mutable size_t hits_;
    mutable size_t misses_;
};

/**
 * @brief Thread-safe N-gram computation cache
 * 
 * Provides optimized caching specifically for PMI calculations
 * with automatic cleanup and performance monitoring.
 */
class PMICache {
public:
    /**
     * @brief Constructor
     * @param cacheSize Maximum cache size
     */
    explicit PMICache(size_t cacheSize = 50000);
    
    /**
     * @brief Get or compute PMI value
     * @param ngram N-gram text
     * @param computeFunc Function to compute PMI if not cached
     * @return PMI score
     */
    template<typename ComputeFunc>
    double getOrCompute(const std::string& ngram, ComputeFunc&& computeFunc) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = pmiCache_.find(ngram);
        if (it != pmiCache_.end() && !isExpired(it->second)) {
            // Update access time
            it->second.lastAccess = std::chrono::steady_clock::now();
            hits_++;
            return it->second.score;
        }
        
        // Compute new value
        double pmi = computeFunc();
        
        // Store in cache
        if (pmiCache_.size() >= maxSize_) {
            evictLRU();
        }
        
        pmiCache_[ngram] = NGramCacheEntry(pmi, 1); // Frequency not used for PMI cache
        misses_++;
        
        return pmi;
    }
    
    /**
     * @brief Preload cache with common N-grams
     * @param ngrams Vector of N-grams to preload
     * @param computeFunc Function to compute PMI values
     */
    template<typename ComputeFunc>
    void preload(const std::vector<std::string>& ngrams, ComputeFunc&& computeFunc) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (const auto& ngram : ngrams) {
            if (pmiCache_.find(ngram) == pmiCache_.end()) {
                double pmi = computeFunc(ngram);
                if (pmiCache_.size() >= maxSize_) {
                    evictLRU();
                }
                pmiCache_[ngram] = NGramCacheEntry(pmi, 1);
            }
        }
    }
    
    /**
     * @brief Get cache statistics
     */
    std::tuple<size_t, size_t, size_t> getStats() const;
    
    /**
     * @brief Clear cache
     */
    void clear();
    
private:
    void evictLRU();
    bool isExpired(const NGramCacheEntry& entry) const;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, NGramCacheEntry> pmiCache_;
    size_t maxSize_;
    std::chrono::minutes ttl_;
    
    mutable size_t hits_;
    mutable size_t misses_;
};

} // namespace core
} // namespace suzume