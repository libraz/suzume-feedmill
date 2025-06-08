/**
 * @file memory_pool_test.cpp
 * @brief Tests for memory pool optimization
 */

#include <gtest/gtest.h>
#include "core/word_extraction/memory_pool.h"
#include "core/word_extraction/trie.h"
#include <chrono>
#include <vector>
#include <string>
#include <thread>
#include <atomic>

namespace suzume {
namespace core {
namespace test {

// Test to verify memory pool basic functionality
TEST(MemoryPoolTest, BasicAllocationDeallocation) {
    MemoryPool<int> pool;
    
    // Test basic allocation
    int* ptr1 = pool.allocate();
    ASSERT_NE(nullptr, ptr1);
    *ptr1 = 42;
    EXPECT_EQ(42, *ptr1);
    
    // Test deallocation
    pool.deallocate(ptr1);
    
    // Test multiple allocations
    std::vector<int*> ptrs;
    for (int i = 0; i < 10; ++i) {
        int* ptr = pool.allocate();
        ASSERT_NE(nullptr, ptr);
        *ptr = i;
        ptrs.push_back(ptr);
    }
    
    // Verify values
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(i, *ptrs[i]);
        pool.deallocate(ptrs[i]);
    }
}

// Test to verify memory pool performance vs standard allocation
TEST(MemoryPoolTest, PerformanceComparison) {
    const size_t numAllocations = 10000;
    
    // Test standard allocation performance
    auto startStd = std::chrono::high_resolution_clock::now();
    {
        std::vector<int*> stdPtrs;
        for (size_t i = 0; i < numAllocations; ++i) {
            stdPtrs.push_back(new int(i));
        }
        for (auto ptr : stdPtrs) {
            delete ptr;
        }
    }
    auto endStd = std::chrono::high_resolution_clock::now();
    auto stdDuration = std::chrono::duration_cast<std::chrono::microseconds>(endStd - startStd);
    
    // Test memory pool performance
    auto startPool = std::chrono::high_resolution_clock::now();
    {
        MemoryPool<int> pool;
        std::vector<int*> poolPtrs;
        for (size_t i = 0; i < numAllocations; ++i) {
            int* ptr = pool.allocate();
            *ptr = i;
            poolPtrs.push_back(ptr);
        }
        for (auto ptr : poolPtrs) {
            pool.deallocate(ptr);
        }
    }
    auto endPool = std::chrono::high_resolution_clock::now();
    auto poolDuration = std::chrono::duration_cast<std::chrono::microseconds>(endPool - startPool);
    
    std::cout << "Standard allocation: " << stdDuration.count() << " μs" << std::endl;
    std::cout << "Memory pool allocation: " << poolDuration.count() << " μs" << std::endl;
    
    // Memory pool should generally be faster or similar
    // Note: This is not a strict requirement as performance can vary
    // but we want to ensure it's not significantly slower
    EXPECT_LT(poolDuration.count(), stdDuration.count() * 2);
}

// Test to verify memory pool handles large allocations
TEST(MemoryPoolTest, LargeAllocationTest) {
    MemoryPool<int, 64> pool; // Smaller chunk size
    
    const size_t numAllocations = 1000;
    std::vector<int*> ptrs;
    
    // Allocate many objects to force multiple chunks
    for (size_t i = 0; i < numAllocations; ++i) {
        int* ptr = pool.allocate();
        ASSERT_NE(nullptr, ptr);
        *ptr = static_cast<int>(i);
        ptrs.push_back(ptr);
    }
    
    // Verify all values are correct
    for (size_t i = 0; i < numAllocations; ++i) {
        EXPECT_EQ(static_cast<int>(i), *ptrs[i]);
    }
    
    // Check that multiple chunks were allocated
    EXPECT_GT(pool.getChunkCount(), 1);
    EXPECT_GT(pool.getMemoryUsage(), sizeof(int) * 64);
    
    // Deallocate all
    for (auto ptr : ptrs) {
        pool.deallocate(ptr);
    }
}

// Test to verify trie with memory pool optimization
TEST(MemoryPoolTest, TrieMemoryPoolIntegration) {
    NGramTrie trie;
    
    // Add many n-grams to test memory pool usage
    const size_t numNgrams = 1000;
    for (size_t i = 0; i < numNgrams; ++i) {
        std::string ngram = "word" + std::to_string(i);
        trie.add(ngram, static_cast<double>(i), static_cast<uint32_t>(i));
    }
    
    // Verify memory statistics
    EXPECT_GT(trie.getNodeCount(), numNgrams); // Should have more nodes than n-grams due to trie structure
    EXPECT_GT(trie.getMemoryUsage(), 0);
    
    std::cout << "Trie nodes: " << trie.getNodeCount() << std::endl;
    std::cout << "Memory usage: " << trie.getMemoryUsage() << " bytes" << std::endl;
    
    // Test prefix search still works
    auto results = trie.findByPrefix("word1");
    EXPECT_GT(results.size(), 0);
    
    // Verify specific results
    auto word100Results = trie.findByPrefix("word100");
    EXPECT_EQ(1, word100Results.size());
    if (!word100Results.empty()) {
        EXPECT_EQ("word100", std::get<0>(word100Results[0]));
        EXPECT_EQ(100.0, std::get<1>(word100Results[0]));
        EXPECT_EQ(100u, std::get<2>(word100Results[0]));
    }
}

// Test to verify memory pool thread safety
TEST(MemoryPoolTest, ThreadSafetyTest) {
    MemoryPool<int> pool;
    const size_t numThreads = 4;
    const size_t allocationsPerThread = 1000;
    
    std::vector<std::thread> threads;
    std::atomic<size_t> successfulAllocations{0};
    std::atomic<size_t> failedAllocations{0};
    
    // Launch threads that allocate and deallocate
    for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([&, i]() {
            std::vector<int*> localPtrs;
            
            // Allocate
            for (size_t j = 0; j < allocationsPerThread; ++j) {
                int* ptr = pool.allocate();
                if (ptr) {
                    *ptr = static_cast<int>(i * 1000 + j);
                    localPtrs.push_back(ptr);
                    successfulAllocations++;
                } else {
                    failedAllocations++;
                }
            }
            
            // Verify values
            for (size_t j = 0; j < localPtrs.size(); ++j) {
                int expected = static_cast<int>(i * 1000 + j);
                if (*localPtrs[j] != expected) {
                    failedAllocations++;
                }
            }
            
            // Deallocate
            for (auto ptr : localPtrs) {
                pool.deallocate(ptr);
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Check results
    EXPECT_EQ(successfulAllocations.load(), numThreads * allocationsPerThread);
    EXPECT_EQ(failedAllocations.load(), 0);
    
    std::cout << "Successful allocations: " << successfulAllocations.load() << std::endl;
    std::cout << "Failed allocations: " << failedAllocations.load() << std::endl;
}

// Test to verify memory pool memory usage tracking
TEST(MemoryPoolTest, MemoryUsageTracking) {
    MemoryPool<int, 10> pool; // Small chunk size for testing
    
    // Initially no memory used
    EXPECT_EQ(0, pool.getChunkCount());
    EXPECT_EQ(0, pool.getMemoryUsage());
    
    // Allocate one object - should create first chunk
    int* ptr1 = pool.allocate();
    EXPECT_EQ(1, pool.getChunkCount());
    EXPECT_EQ(10 * sizeof(int), pool.getMemoryUsage());
    
    // Allocate more objects within same chunk
    std::vector<int*> ptrs;
    for (int i = 0; i < 9; ++i) {
        ptrs.push_back(pool.allocate());
    }
    
    // Should still be one chunk
    EXPECT_EQ(1, pool.getChunkCount());
    EXPECT_EQ(10 * sizeof(int), pool.getMemoryUsage());
    
    // Allocate one more - should create second chunk
    int* ptr11 = pool.allocate();
    EXPECT_EQ(2, pool.getChunkCount());
    EXPECT_EQ(20 * sizeof(int), pool.getMemoryUsage());
    
    // Cleanup
    pool.deallocate(ptr1);
    pool.deallocate(ptr11);
    for (auto ptr : ptrs) {
        pool.deallocate(ptr);
    }
}

} // namespace test
} // namespace core
} // namespace suzume