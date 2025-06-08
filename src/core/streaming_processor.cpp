/**
 * @file streaming_processor.cpp
 * @brief Implementation of streaming processor for optimized I/O
 */

#include "streaming_processor.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

#ifdef __unix__
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace suzume {
namespace core {

StreamingLineProcessor::StreamingLineProcessor(const StreamingConfig& config)
    : config_(config)
    , bytesRead_(0)
    , bytesWritten_(0)
    , processingTimeMs_(0)
{
}

size_t StreamingLineProcessor::processFile(
    const std::string& inputPath,
    const std::string& outputPath,
    const LineProcessor& processor,
    const ProgressCallback& progressCallback
) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::ifstream input(inputPath, std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open input file: " + inputPath);
    }
    
    std::ofstream output(outputPath, std::ios::binary);
    if (!output.is_open()) {
        throw std::runtime_error("Failed to open output file: " + outputPath);
    }
    
    // Set custom buffer size for better I/O performance
    std::vector<char> inputBuffer(config_.bufferSize);
    std::vector<char> outputBuffer(config_.bufferSize);
    input.rdbuf()->pubsetbuf(inputBuffer.data(), inputBuffer.size());
    output.rdbuf()->pubsetbuf(outputBuffer.data(), outputBuffer.size());
    
    size_t fileSize = getFileSize(inputPath);
    size_t totalLines = 0;
    size_t processedBytes = 0;
    
    std::string line;
    while (std::getline(input, line)) {
        processedBytes += line.size() + 1; // +1 for newline
        bytesRead_ += line.size() + 1;
        
        // Process line
        std::string processedLine = processor(line);
        
        // Write result
        if (!processedLine.empty()) {
            output << processedLine << '\n';
            bytesWritten_ += processedLine.size() + 1;
        }
        
        totalLines++;
        
        // Report progress
        if (progressCallback && fileSize > 0 && totalLines % 1000 == 0) {
            double progress = static_cast<double>(processedBytes) / fileSize;
            progressCallback(progress);
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    processingTimeMs_ = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    if (progressCallback) {
        progressCallback(1.0);
    }
    
    return totalLines;
}

size_t StreamingLineProcessor::processBatch(
    const std::string& inputPath,
    const std::string& outputPath,
    const BatchProcessor& processor,
    const ProgressCallback& progressCallback
) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::ifstream input(inputPath, std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open input file: " + inputPath);
    }
    
    std::ofstream output(outputPath, std::ios::binary);
    if (!output.is_open()) {
        throw std::runtime_error("Failed to open output file: " + outputPath);
    }
    
    // Set custom buffer size
    std::vector<char> inputBuffer(config_.bufferSize);
    std::vector<char> outputBuffer(config_.bufferSize);
    input.rdbuf()->pubsetbuf(inputBuffer.data(), inputBuffer.size());
    output.rdbuf()->pubsetbuf(outputBuffer.data(), outputBuffer.size());
    
    size_t fileSize = getFileSize(inputPath);
    size_t totalLines = 0;
    size_t processedBytes = 0;
    
    std::vector<std::string> batch;
    batch.reserve(config_.batchSize);
    
    while (!input.eof()) {
        // Read batch
        size_t linesRead = readBatch(input, batch, config_.batchSize);
        if (linesRead == 0) break;
        
        // Count bytes for progress
        for (const auto& line : batch) {
            processedBytes += line.size() + 1;
            bytesRead_ += line.size() + 1;
        }
        
        // Process batch
        auto processedBatch = processor(batch);
        
        // Write batch
        writeBatch(output, processedBatch);
        
        totalLines += linesRead;
        
        // Report progress
        if (progressCallback && fileSize > 0) {
            double progress = static_cast<double>(processedBytes) / fileSize;
            progressCallback(progress);
        }
        
        batch.clear();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    processingTimeMs_ = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    if (progressCallback) {
        progressCallback(1.0);
    }
    
    return totalLines;
}

std::tuple<size_t, size_t, size_t> StreamingLineProcessor::getStats() const {
    return std::make_tuple(bytesRead_, bytesWritten_, processingTimeMs_);
}

size_t StreamingLineProcessor::readBatch(std::istream& input, std::vector<std::string>& batch, size_t batchSize) {
    std::string line;
    size_t count = 0;
    
    while (count < batchSize && std::getline(input, line)) {
        batch.push_back(std::move(line));
        count++;
    }
    
    return count;
}

void StreamingLineProcessor::writeBatch(std::ostream& output, const std::vector<std::string>& lines) {
    for (const auto& line : lines) {
        if (!line.empty()) {
            output << line << '\n';
            bytesWritten_ += line.size() + 1;
        }
    }
}

size_t StreamingLineProcessor::getFileSize(const std::string& filePath) const {
    try {
        return std::filesystem::file_size(filePath);
    } catch (const std::filesystem::filesystem_error&) {
        return 0; // Return 0 if size cannot be determined
    }
}

// MemoryMappedProcessor implementation

#ifdef __unix__
MemoryMappedProcessor::MemoryMappedProcessor(const std::string& filePath)
    : filePath_(filePath)
    , mappedData_(nullptr)
    , fileSize_(0)
    , mapped_(false)
    , fileDescriptor_(-1)
{
    fileDescriptor_ = open(filePath.c_str(), O_RDONLY);
    if (fileDescriptor_ == -1) {
        return;
    }
    
    struct stat sb;
    if (fstat(fileDescriptor_, &sb) == -1) {
        close(fileDescriptor_);
        return;
    }
    
    fileSize_ = sb.st_size;
    
    mappedData_ = mmap(nullptr, fileSize_, PROT_READ, MAP_PRIVATE, fileDescriptor_, 0);
    if (mappedData_ == MAP_FAILED) {
        close(fileDescriptor_);
        mappedData_ = nullptr;
        return;
    }
    
    mapped_ = true;
}

MemoryMappedProcessor::~MemoryMappedProcessor() {
    unmap();
}

size_t MemoryMappedProcessor::processChunks(
    size_t chunkSize,
    const std::function<void(const char*, size_t)>& processor,
    const ProgressCallback& progressCallback
) {
    if (!mapped_) {
        throw std::runtime_error("File is not memory mapped");
    }
    
    const char* data = static_cast<const char*>(mappedData_);
    size_t chunks = 0;
    size_t processed = 0;
    
    while (processed < fileSize_) {
        size_t currentChunkSize = std::min(chunkSize, fileSize_ - processed);
        processor(data + processed, currentChunkSize);
        
        processed += currentChunkSize;
        chunks++;
        
        if (progressCallback) {
            double progress = static_cast<double>(processed) / fileSize_;
            progressCallback(progress);
        }
    }
    
    return chunks;
}

void MemoryMappedProcessor::unmap() {
    if (mapped_ && mappedData_) {
        munmap(mappedData_, fileSize_);
        mappedData_ = nullptr;
        mapped_ = false;
    }
    
    if (fileDescriptor_ != -1) {
        close(fileDescriptor_);
        fileDescriptor_ = -1;
    }
}

#else
// Windows or other platforms - simplified implementation
MemoryMappedProcessor::MemoryMappedProcessor(const std::string& filePath)
    : filePath_(filePath)
    , mappedData_(nullptr)
    , fileSize_(0)
    , mapped_(false)
    , fileDescriptor_(-1)
{
    // Memory mapping not implemented for this platform
}

MemoryMappedProcessor::~MemoryMappedProcessor() {
}

size_t MemoryMappedProcessor::processChunks(
    size_t chunkSize,
    const std::function<void(const char*, size_t)>& processor,
    const StreamingLineProcessor::ProgressCallback& progressCallback
) {
    throw std::runtime_error("Memory mapping not supported on this platform");
}

void MemoryMappedProcessor::unmap() {
}
#endif

// ParallelStreamProcessor implementation

ParallelStreamProcessor::ParallelStreamProcessor(size_t numThreads, const StreamingConfig& config)
    : config_(config)
    , totalLines_(0)
    , processingTimeMs_(0)
{
    if (numThreads == 0) {
        numThreads_ = std::thread::hardware_concurrency();
        if (numThreads_ == 0) {
            numThreads_ = 4; // Fallback
        }
    } else {
        numThreads_ = numThreads;
    }
}

size_t ParallelStreamProcessor::processFile(
    const std::string& inputPath,
    const std::string& outputPath,
    const StreamingLineProcessor::BatchProcessor& processor,
    const StreamingLineProcessor::ProgressCallback& progressCallback
) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::ifstream input(inputPath, std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open input file: " + inputPath);
    }
    
    std::ofstream output(outputPath, std::ios::binary);
    if (!output.is_open()) {
        throw std::runtime_error("Failed to open output file: " + outputPath);
    }
    
    // Producer-consumer pattern with work queue
    std::queue<std::vector<std::string>> workQueue;
    std::queue<std::vector<std::string>> resultQueue;
    std::mutex workMutex, resultMutex;
    std::condition_variable workCv, resultCv;
    std::atomic<bool> inputFinished{false};
    std::atomic<bool> processingFinished{false};
    std::atomic<size_t> totalProcessed{0};
    
    size_t fileSize = 0;
    try {
        fileSize = std::filesystem::file_size(inputPath);
    } catch (const std::filesystem::filesystem_error&) {
        // Continue without file size for progress reporting
    }
    
    // Producer thread (reader)
    std::thread producer([&]() {
        std::vector<char> buffer(config_.bufferSize);
        input.rdbuf()->pubsetbuf(buffer.data(), buffer.size());
        
        std::vector<std::string> batch;
        batch.reserve(config_.batchSize);
        
        std::string line;
        while (std::getline(input, line)) {
            batch.push_back(std::move(line));
            
            if (batch.size() >= config_.batchSize) {
                {
                    std::lock_guard<std::mutex> lock(workMutex);
                    workQueue.push(std::move(batch));
                }
                workCv.notify_one();
                batch.clear();
                batch.reserve(config_.batchSize);
            }
        }
        
        // Push remaining batch
        if (!batch.empty()) {
            {
                std::lock_guard<std::mutex> lock(workMutex);
                workQueue.push(std::move(batch));
            }
            workCv.notify_one();
        }
        
        inputFinished = true;
        workCv.notify_all();
    });
    
    // Worker threads
    std::vector<std::thread> workers;
    for (size_t i = 0; i < numThreads_; ++i) {
        workers.emplace_back([&]() {
            while (true) {
                std::vector<std::string> batch;
                
                // Get work
                {
                    std::unique_lock<std::mutex> lock(workMutex);
                    workCv.wait(lock, [&]() { return !workQueue.empty() || inputFinished; });
                    
                    if (workQueue.empty() && inputFinished) {
                        break;
                    }
                    
                    if (!workQueue.empty()) {
                        batch = std::move(workQueue.front());
                        workQueue.pop();
                    }
                }
                
                if (!batch.empty()) {
                    // Process batch
                    auto result = processor(batch);
                    totalProcessed += batch.size();
                    
                    // Put result
                    {
                        std::lock_guard<std::mutex> lock(resultMutex);
                        resultQueue.push(std::move(result));
                    }
                    resultCv.notify_one();
                }
            }
        });
    }
    
    // Consumer thread (writer)
    std::thread consumer([&]() {
        std::vector<char> buffer(config_.bufferSize);
        output.rdbuf()->pubsetbuf(buffer.data(), buffer.size());
        
        size_t writtenLines = 0;
        
        while (true) {
            std::vector<std::string> result;
            
            // Get result
            {
                std::unique_lock<std::mutex> lock(resultMutex);
                resultCv.wait(lock, [&]() { return !resultQueue.empty() || processingFinished; });
                
                if (resultQueue.empty() && processingFinished) {
                    break;
                }
                
                if (!resultQueue.empty()) {
                    result = std::move(resultQueue.front());
                    resultQueue.pop();
                }
            }
            
            // Write result
            for (const auto& line : result) {
                if (!line.empty()) {
                    output << line << '\n';
                    writtenLines++;
                }
            }
            
            // Report progress
            if (progressCallback && fileSize > 0 && writtenLines % 1000 == 0) {
                double progress = static_cast<double>(totalProcessed.load()) / (fileSize / 50); // Rough estimate
                progressCallback(std::min(progress, 0.95)); // Cap at 95% until completion
            }
        }
    });
    
    // Wait for completion
    producer.join();
    
    for (auto& worker : workers) {
        worker.join();
    }
    
    processingFinished = true;
    resultCv.notify_all();
    consumer.join();
    
    totalLines_ = totalProcessed;
    
    auto endTime = std::chrono::high_resolution_clock::now();
    processingTimeMs_ = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    if (progressCallback) {
        progressCallback(1.0);
    }
    
    return totalLines_;
}

std::tuple<size_t, size_t, size_t> ParallelStreamProcessor::getStats() const {
    return std::make_tuple(totalLines_, processingTimeMs_, numThreads_);
}

} // namespace core
} // namespace suzume