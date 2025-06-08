/**
 * @file streaming_processor.h
 * @brief Streaming processor for large file handling with optimized I/O
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <fstream>
#include <deque>

namespace suzume {
namespace core {

/**
 * @brief Configuration for streaming processor
 */
struct StreamingConfig {
    size_t bufferSize = 64 * 1024;        // 64KB buffer
    size_t batchSize = 1000;               // Lines per batch
    size_t maxMemoryUsage = 100 * 1024 * 1024; // 100MB max memory
    bool enableCompression = false;         // Enable compression for temporary files
    std::string tempDir = "/tmp";          // Temporary directory
    
    StreamingConfig() = default;
};

/**
 * @brief High-performance streaming line processor
 * 
 * Efficiently processes large files line by line with minimal memory usage
 * and optimized I/O patterns.
 */
class StreamingLineProcessor {
public:
    using LineProcessor = std::function<std::string(const std::string&)>;
    using BatchProcessor = std::function<std::vector<std::string>(const std::vector<std::string>&)>;
    using ProgressCallback = std::function<void(double)>;
    
    /**
     * @brief Constructor
     * @param config Streaming configuration
     */
    explicit StreamingLineProcessor(const StreamingConfig& config = StreamingConfig{});
    
    /**
     * @brief Process file line by line
     * @param inputPath Input file path
     * @param outputPath Output file path
     * @param processor Line processing function
     * @param progressCallback Optional progress callback
     * @return Number of lines processed
     */
    size_t processFile(
        const std::string& inputPath,
        const std::string& outputPath,
        const LineProcessor& processor,
        const ProgressCallback& progressCallback = nullptr
    );
    
    /**
     * @brief Process file in batches for better performance
     * @param inputPath Input file path
     * @param outputPath Output file path
     * @param processor Batch processing function
     * @param progressCallback Optional progress callback
     * @return Number of lines processed
     */
    size_t processBatch(
        const std::string& inputPath,
        const std::string& outputPath,
        const BatchProcessor& processor,
        const ProgressCallback& progressCallback = nullptr
    );
    
    /**
     * @brief Get processing statistics
     * @return Tuple of (bytes_read, bytes_written, processing_time_ms)
     */
    std::tuple<size_t, size_t, size_t> getStats() const;
    
private:
    /**
     * @brief Read lines from input stream with buffering
     * @param input Input stream
     * @param batch Output batch
     * @param batchSize Maximum batch size
     * @return Number of lines read
     */
    size_t readBatch(std::istream& input, std::vector<std::string>& batch, size_t batchSize);
    
    /**
     * @brief Write lines to output stream with buffering
     * @param output Output stream
     * @param lines Lines to write
     */
    void writeBatch(std::ostream& output, const std::vector<std::string>& lines);
    
    /**
     * @brief Get file size for progress reporting
     * @param filePath File path
     * @return File size in bytes
     */
    size_t getFileSize(const std::string& filePath) const;
    
    StreamingConfig config_;
    size_t bytesRead_;
    size_t bytesWritten_;
    size_t processingTimeMs_;
};

/**
 * @brief Memory-mapped file processor for very large files
 * 
 * Uses memory mapping for efficient access to very large files
 * that might not fit in memory.
 */
class MemoryMappedProcessor {
public:
    /**
     * @brief Constructor
     * @param filePath Path to file to map
     */
    explicit MemoryMappedProcessor(const std::string& filePath);
    
    /**
     * @brief Destructor
     */
    ~MemoryMappedProcessor();
    
    /**
     * @brief Process file in chunks
     * @param chunkSize Size of each chunk to process
     * @param processor Function to process each chunk
     * @param progressCallback Optional progress callback
     * @return Number of chunks processed
     */
    size_t processChunks(
        size_t chunkSize,
        const std::function<void(const char*, size_t)>& processor,
        const StreamingLineProcessor::ProgressCallback& progressCallback = nullptr
    );
    
    /**
     * @brief Get file size
     * @return File size in bytes
     */
    size_t getFileSize() const { return fileSize_; }
    
    /**
     * @brief Check if file is successfully mapped
     * @return True if mapped
     */
    bool isMapped() const { return mapped_; }
    
private:
    void unmap();
    
    std::string filePath_;
    void* mappedData_;
    size_t fileSize_;
    bool mapped_;
    int fileDescriptor_;
};

/**
 * @brief Parallel streaming processor
 * 
 * Processes streams in parallel using multiple threads for
 * CPU-intensive operations.
 */
class ParallelStreamProcessor {
public:
    /**
     * @brief Constructor
     * @param numThreads Number of processing threads
     * @param config Streaming configuration
     */
    explicit ParallelStreamProcessor(
        size_t numThreads = 0, // 0 = auto-detect
        const StreamingConfig& config = StreamingConfig{}
    );
    
    /**
     * @brief Process file with parallel workers
     * @param inputPath Input file path
     * @param outputPath Output file path
     * @param processor Batch processing function (must be thread-safe)
     * @param progressCallback Optional progress callback
     * @return Number of lines processed
     */
    size_t processFile(
        const std::string& inputPath,
        const std::string& outputPath,
        const StreamingLineProcessor::BatchProcessor& processor,
        const StreamingLineProcessor::ProgressCallback& progressCallback = nullptr
    );
    
    /**
     * @brief Get processing statistics
     * @return Tuple of (total_lines, processing_time_ms, threads_used)
     */
    std::tuple<size_t, size_t, size_t> getStats() const;
    
private:
    size_t numThreads_;
    StreamingConfig config_;
    size_t totalLines_;
    size_t processingTimeMs_;
};

} // namespace core
} // namespace suzume