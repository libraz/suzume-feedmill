/**
 * @file executor.h
 * @brief Parallel execution utilities
 */

#ifndef SUZUME_PARALLEL_EXECUTOR_H_
#define SUZUME_PARALLEL_EXECUTOR_H_

#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

namespace suzume {
namespace parallel {

/**
 * @brief Parallel executor for batch processing
 */
class ParallelExecutor {
public:
    /**
     * @brief Initialize the executor (dummy function to ensure library has a global symbol)
     */
    static void initializeExecutor();

    /**
     * @brief Parallel map operation
     *
     * @tparam T Input type
     * @tparam R Result type
     * @param input Input vector
     * @param mapper Mapping function
     * @param threadCount Number of threads (0 = auto)
     * @return std::vector<R> Result vector
     */
    template<typename T, typename R>
    static std::vector<R> parallelMap(
        const std::vector<T>& input,
        std::function<R(const T&)> mapper,
        unsigned int threadCount = 0
    ) {
        // Determine thread count
        if (threadCount == 0) {
            threadCount = std::thread::hardware_concurrency();
        }

        // For small inputs or single thread, use sequential processing
        if (input.size() <= 100 || threadCount <= 1) {
            std::vector<R> result;
            result.reserve(input.size());

            for (const auto& item : input) {
                result.push_back(mapper(item));
            }

            return result;
        }

        // Calculate chunk size
        size_t chunkSize = input.size() / threadCount;
        if (chunkSize == 0) chunkSize = 1;

        // Prepare result vector
        std::vector<R> result(input.size());

        // Create threads
        std::vector<std::thread> threads;

        // Launch threads
        for (unsigned int i = 0; i < threadCount; ++i) {
            size_t start = i * chunkSize;
            size_t end = (i == threadCount - 1) ? input.size() : (i + 1) * chunkSize;

            threads.emplace_back([&, start, end]() {
                for (size_t j = start; j < end; ++j) {
                    result[j] = mapper(input[j]);
                }
            });
        }

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        return result;
    }

    /**
     * @brief Parallel for-each operation
     *
     * @tparam T Input type
     * @param input Input vector
     * @param processor Processing function
     * @param threadCount Number of threads (0 = auto)
     * @param progressCallback Progress callback function
     */
    template<typename T>
    static void parallelForEach(
        const std::vector<T>& input,
        std::function<void(const T&)> processor,
        unsigned int threadCount = 0,
        std::function<void(double)> progressCallback = nullptr
    ) {
        // Determine thread count
        if (threadCount == 0) {
            threadCount = std::thread::hardware_concurrency();
        }

        // For small inputs or single thread, use sequential processing
        if (input.size() <= 100 || threadCount <= 1) {
            for (size_t i = 0; i < input.size(); ++i) {
                processor(input[i]);

                // Report progress
                if (progressCallback) {
                    double progress = static_cast<double>(i + 1) / input.size();
                    progressCallback(progress);
                }
            }

            return;
        }

        // Calculate chunk size
        size_t chunkSize = input.size() / threadCount;
        if (chunkSize == 0) chunkSize = 1;

        // Create threads
        std::vector<std::thread> threads;
        std::mutex progressMutex;
        std::atomic<size_t> processedItems(0);

        // Launch threads
        for (unsigned int i = 0; i < threadCount; ++i) {
            size_t start = i * chunkSize;
            size_t end = (i == threadCount - 1) ? input.size() : (i + 1) * chunkSize;

            threads.emplace_back([&, start, end]() {
                for (size_t j = start; j < end; ++j) {
                    processor(input[j]);

                    // Update processed items count
                    size_t processed = processedItems.fetch_add(1, std::memory_order_relaxed) + 1;

                    // Report progress
                    if (progressCallback && (processed % 100 == 0 || processed == input.size())) {
                        std::lock_guard<std::mutex> lock(progressMutex);
                        double progress = static_cast<double>(processed) / input.size();
                        progressCallback(progress);
                    }
                }
            });
        }

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        // Final progress update
        if (progressCallback) {
            progressCallback(1.0);
        }
    }

    /**
     * @brief Parallel reduce operation
     *
     * @tparam T Input type
     * @tparam R Result type
     * @param input Input vector
     * @param reducer Reduction function
     * @param initialValue Initial value
     * @param threadCount Number of threads (0 = auto)
     * @return R Reduced result
     */
    template<typename T, typename R>
    static R parallelReduce(
        const std::vector<T>& input,
        std::function<R(R, const T&)> reducer,
        R initialValue,
        unsigned int threadCount = 0
    ) {
        // Determine thread count
        if (threadCount == 0) {
            threadCount = std::thread::hardware_concurrency();
        }

        // For small inputs or single thread, use sequential processing
        if (input.size() <= 100 || threadCount <= 1) {
            R result = initialValue;

            for (const auto& item : input) {
                result = reducer(result, item);
            }

            return result;
        }

        // Calculate chunk size
        size_t chunkSize = input.size() / threadCount;
        if (chunkSize == 0) chunkSize = 1;

        // Create threads
        std::vector<std::thread> threads;
        std::vector<R> partialResults(threadCount, initialValue);

        // Launch threads
        for (unsigned int i = 0; i < threadCount; ++i) {
            size_t start = i * chunkSize;
            size_t end = (i == threadCount - 1) ? input.size() : (i + 1) * chunkSize;

            threads.emplace_back([&, i, start, end]() {
                R localResult = initialValue;

                for (size_t j = start; j < end; ++j) {
                    localResult = reducer(localResult, input[j]);
                }

                partialResults[i] = localResult;
            });
        }

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        // Combine partial results
        R finalResult = initialValue;
        for (const auto& partialResult : partialResults) {
            finalResult = reducer(finalResult, partialResult);
        }

        return finalResult;
    }
};

} // namespace parallel
} // namespace suzume

#endif // SUZUME_PARALLEL_EXECUTOR_H_
