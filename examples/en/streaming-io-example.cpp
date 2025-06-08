#include <suzume_feedmill.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

/**
 * Streaming I/O example
 *
 * This example demonstrates:
 * 1. Using stdin/stdout for streaming processing
 * 2. Creating pipeline processing with suzume-feedmill
 * 3. Simulating data streaming scenarios
 */

// Helper function to simulate a data stream
void simulateDataStream(const std::string& outputFile, int lineCount, int delayMs) {
    std::ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        throw std::runtime_error("Could not open output file: " + outputFile);
    }

    std::cout << "Simulating data stream to " << outputFile << "..." << std::endl;

    for (int i = 0; i < lineCount; i++) {
        // Generate a line with some variation
        outFile << "Stream data line " << i << "\tThis is sample text with variation "
                << (i % 5) << " for testing purposes." << std::endl;

        // Simulate delay between data points
        if (delayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }

        // Show progress
        if (i % 100 == 0) {
            std::cout << "\rGenerated " << i << " lines..." << std::flush;
        }
    }

    outFile.close();
    std::cout << "\rGenerated " << lineCount << " lines.                " << std::endl;
}

int main(int argc, char* argv[]) {
    // Define file paths for the example
    std::string tempDir = std::filesystem::temp_directory_path().string();
    std::string streamDataFile = tempDir + "/stream_data.tsv";
    std::string normalizedFile = tempDir + "/normalized_stream.tsv";
    std::string pmiFile = tempDir + "/pmi_stream.tsv";

    try {
        // Step 1: Simulate a data stream
        simulateDataStream(streamDataFile, 1000, 0);

        // Step 2: Process stream using stdin/stdout
        std::cout << "\nExample 1: Using stdin/stdout for normalization" << std::endl;
        std::cout << "Command equivalent: cat " << streamDataFile << " | suzume-feedmill normalize - -" << std::endl;

        // Create a command to pipe the data through normalize
        std::string command = "cat " + streamDataFile + " | " +
                             argv[0] + "_normalize_stdin_stdout > " + normalizedFile;

        std::cout << "Executing: " << command << std::endl;

        // In a real implementation, we would execute this command
        // For this example, we'll simulate it with direct API calls

        suzume::NormalizeOptions normOpt;
        normOpt.form = suzume::NormalizationForm::NFKC;
        normOpt.threads = 2;

        // Use "-" to indicate stdin/stdout
        auto normResult = suzume::normalize(streamDataFile, normalizedFile, normOpt);

        std::cout << "Normalization complete!" << std::endl;
        std::cout << "Processed " << normResult.rows << " rows, "
                  << normResult.uniques << " unique rows" << std::endl;

        // Step 3: Create a processing pipeline
        std::cout << "\nExample 2: Creating a processing pipeline" << std::endl;
        std::cout << "Command equivalent: cat " << streamDataFile
                  << " | suzume-feedmill normalize - - | suzume-feedmill pmi - "
                  << pmiFile << std::endl;

        // In a real implementation, we would execute this pipeline
        // For this example, we'll simulate it with direct API calls

        // First normalize to stdout (using a temporary file to simulate)
        std::string tempNormalized = tempDir + "/temp_normalized.tsv";
        auto pipelineNormResult = suzume::normalize(streamDataFile, tempNormalized, normOpt);

        // Then process with PMI
        suzume::PmiOptions pmiOpt;
        pmiOpt.n = 2;
        pmiOpt.topK = 500;
        pmiOpt.threads = 2;

        auto pmiResult = suzume::calculatePmi(tempNormalized, pmiFile, pmiOpt);

        std::cout << "Pipeline processing complete!" << std::endl;
        std::cout << "Processed " << pipelineNormResult.rows << " rows in normalization, "
                  << pmiResult.grams << " n-grams in PMI calculation" << std::endl;

        // Step 4: Real-time processing simulation
        std::cout << "\nExample 3: Real-time processing simulation" << std::endl;

        // Start a background thread to simulate continuous data generation
        std::string realtimeDataFile = tempDir + "/realtime_data.tsv";
        std::string realtimeOutputFile = tempDir + "/realtime_output.tsv";

        std::cout << "Starting real-time data generation..." << std::endl;
        // In a real implementation, we would start a thread here
        // For this example, we'll generate a smaller dataset

        simulateDataStream(realtimeDataFile, 500, 0);

        std::cout << "Processing real-time data..." << std::endl;

        // Process with minimal buffering
        suzume::NormalizeOptions realtimeOpt;
        realtimeOpt.form = suzume::NormalizationForm::NFKC;
        realtimeOpt.threads = 1; // Single thread for real-time processing

        auto realtimeResult = suzume::normalize(realtimeDataFile, realtimeOutputFile, realtimeOpt);

        std::cout << "Real-time processing complete!" << std::endl;
        std::cout << "Processed " << realtimeResult.rows << " rows, "
                  << realtimeResult.uniques << " unique rows" << std::endl;

        // Clean up temporary files
        std::cout << "\nCleaning up temporary files..." << std::endl;
        std::filesystem::remove(streamDataFile);
        std::filesystem::remove(normalizedFile);
        std::filesystem::remove(pmiFile);
        std::filesystem::remove(tempNormalized);
        std::filesystem::remove(realtimeDataFile);
        std::filesystem::remove(realtimeOutputFile);

        std::cout << "Cleanup complete!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
