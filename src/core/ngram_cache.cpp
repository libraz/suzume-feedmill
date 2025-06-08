/**
 * @file ngram_cache.cpp
 * @brief Implementation of N-gram cache for optimized processing
 */

#include "ngram_cache.h"
#include <algorithm>

namespace suzume {
namespace core {

NGramCache::NGramCache(size_t maxSize, int ttlMinutes)
    : maxSize_(maxSize)
    , ttl_(ttlMinutes)
    , hits_(0)
    , misses_(0)
{
}

std::optional<NGramCacheEntry> NGramCache::get(const std::string& ngram) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cache_.find(ngram);
    if (it != cache_.end()) {
        if (!isExpired(it->second)) {
            // Update access time
            it->second.lastAccess = std::chrono::steady_clock::now();
            hits_++;
            return it->second;
        } else {
            // Remove expired entry
            cache_.erase(it);
        }
    }
    
    misses_++;
    return std::nullopt;
}

void NGramCache::put(const std::string& ngram, double score, uint32_t frequency) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we need to evict
    if (cache_.size() >= maxSize_) {
        evictLRU();
    }
    
    // Insert or update entry
    cache_[ngram] = NGramCacheEntry(score, frequency);
}

void NGramCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    hits_ = 0;
    misses_ = 0;
}

std::tuple<size_t, size_t, size_t> NGramCache::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::make_tuple(hits_, misses_, cache_.size());
}

double NGramCache::getHitRate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = hits_ + misses_;
    return total > 0 ? static_cast<double>(hits_) / total : 0.0;
}

size_t NGramCache::cleanupExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t removedCount = 0;
    auto it = cache_.begin();
    while (it != cache_.end()) {
        if (isExpired(it->second)) {
            it = cache_.erase(it);
            removedCount++;
        } else {
            ++it;
        }
    }
    
    return removedCount;
}

void NGramCache::evictLRU() {
    if (cache_.empty()) return;
    
    // Find least recently used entry
    auto lruIt = std::min_element(cache_.begin(), cache_.end(),
        [](const auto& a, const auto& b) {
            return a.second.lastAccess < b.second.lastAccess;
        });
    
    cache_.erase(lruIt);
}

bool NGramCache::isExpired(const NGramCacheEntry& entry) const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - entry.lastAccess);
    return elapsed > ttl_;
}

// PMICache implementation

PMICache::PMICache(size_t cacheSize)
    : maxSize_(cacheSize)
    , ttl_(30) // 30 minutes default TTL
    , hits_(0)
    , misses_(0)
{
}

std::tuple<size_t, size_t, size_t> PMICache::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::make_tuple(hits_, misses_, pmiCache_.size());
}

void PMICache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    pmiCache_.clear();
    hits_ = 0;
    misses_ = 0;
}

void PMICache::evictLRU() {
    if (pmiCache_.empty()) return;
    
    // Find least recently used entry
    auto lruIt = std::min_element(pmiCache_.begin(), pmiCache_.end(),
        [](const auto& a, const auto& b) {
            return a.second.lastAccess < b.second.lastAccess;
        });
    
    pmiCache_.erase(lruIt);
}

bool PMICache::isExpired(const NGramCacheEntry& entry) const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - entry.lastAccess);
    return elapsed > ttl_;
}

} // namespace core
} // namespace suzume