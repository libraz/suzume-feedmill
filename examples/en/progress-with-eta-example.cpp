#include <suzume_feedmill.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <thread>
#include <vector>
#include <random>

/**
 * Progress with ETA example
 *
 * This example demonstrates:
 * 1. Using the ETA-enabled progress bar
 * 2. Customizing progress display
 * 3. Comparing different progress formats
 */

// Helper function to generate a large test file
void generateTestFile(const std::string& filename, size_t lineCount) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }

    std::cout << "Generating test file with " << lineCount << " lines..." << std::endl;

    // Random generator for text variation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> wordCount(5, 20);
    std::uniform_int_distribution<> wordLength(3, 12);
    std::uniform_int_distribution<> charDist(97, 122); // a-z

    for (size_t i = 0; i < lineCount; i++) {
        // Generate a random line
        int words = wordCount(gen);
        for (int w = 0; w < words; w++) {
            int len = wordLength(gen);
            for (int c = 0; c < len; c++) {
                file << static_cast<char>(charDist(gen));
            }
            file << " ";
        }
        file << "\n";

        // Show progress
        if (i % 1000 == 0) {
            std::cout << "\rGenerated " << i << " lines..." << std::flush;
        }
    }

    file.close();
    std::cout << "\rGenerated " << lineCount << " lines.                " << std::endl;
}

// Custom progress callback with manual ETA calculation
void customProgressCallback(double ratio) {
    static auto startTime = std::chrono::steady_clock::now();
    static int lastPercent = -1;

    int percent = static_cast<int>(ratio * 100);

    if (percent != lastPercent) {
        auto now = std::chrono::steady_clock::now();
        double elapsedSeconds = std::chrono::duration<double>(now - startTime).count();

        // Calculate ETA
        double eta = 0.0;
        if (ratio > 0.0 && ratio < 1.0) {
            double totalEstimatedTime = elapsedSeconds / ratio;
            eta = totalEstimatedTime - elapsedSeconds;
        }

        // Format ETA string
        std::string etaStr;
        if (ratio <= 0.0 || ratio >= 1.0) {
            etaStr = "";
        } else {
            int etaMinutes = static_cast<int>(eta) / 60;
            int etaSeconds = static_cast<int>(eta) % 60;

            std::stringstream ss;
            ss << " ETA: ";
            if (etaMinutes > 0) {
                ss << etaMinutes << "m ";
            }
            ss << etaSeconds << "s";
            etaStr = ss.str();
        }

        // Create a progress bar
        const int barWidth = 40;
        int pos = barWidth * ratio;

        std::cout << "\r[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }

        std::cout << "] " << percent << "%" << etaStr << std::flush;
        lastPercent = percent;

        if (percent >= 100) {
            std::cout << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    // Define file paths for the example
    std::string tempDir = std::filesystem::temp_directory_path().string();
    std::string testFile = tempDir + "/eta_test_data.txt";
    std::string outputDir = tempDir + "/eta_test_output";

    // Create output directory
    std::filesystem::create_directories(outputDir);

    try {
        // Generate a test file
        generateTestFile(testFile, 50000);

        // Example 1: Using the built-in ETA progress display (TTY format)
        std::cout << "\nExample 1: Built-in ETA progress display (TTY format)" << std::endl;

        suzume::NormalizeOptions ttyOpt;
        ttyOpt.form = suzume::NormalizationForm::NFKC;
        ttyOpt.threads = 4;
        ttyOpt.progressFormat = suzume::ProgressFormat::TTY;
        // The progress callback with ETA is automatically used

        std::string ttyOutput = outputDir + "/tty_output.tsv";
        auto ttyResult = suzume::normalize(testFile, ttyOutput, ttyOpt);

        std::cout << "TTY format processing complete!" << std::endl;
        std::cout << "Processed " << ttyResult.rows << " rows in "
                  << ttyResult.elapsedMs << " ms" << std::endl;

        // Example 2: Using the JSON progress format with ETA
        std::cout << "\nExample 2: JSON progress format with ETA" << std::endl;

        suzume::NormalizeOptions jsonOpt;
        jsonOpt.form = suzume::NormalizationForm::NFKC;
        jsonOpt.threads = 4;
        jsonOpt.progressFormat = suzume::ProgressFormat::JSON;

        std::string jsonOutput = outputDir + "/json_output.tsv";

        std::cout << "JSON progress output (first few lines):" << std::endl;
        // Capture a few progress updates
        int captureCount = 0;
        jsonOpt.progressCallback = [&captureCount](double ratio) {
            if (captureCount < 5) {
                // In a real scenario, this would be sent to stderr
                std::cout << "{\"progress\":" << static_cast<int>(ratio * 100)
                          << ", \"eta\":" << (1.0 - ratio) * 30 << "}" << std::endl;
                captureCount++;
            }
        };

        auto jsonResult = suzume::normalize(testFile, jsonOutput, jsonOpt);

        std::cout << "JSON format processing complete!" << std::endl;
        std::cout << "Processed " << jsonResult.rows << " rows in "
                  << jsonResult.elapsedMs << " ms" << std::endl;

        // Example 3: Custom progress display with ETA
        std::cout << "\nExample 3: Custom progress display with ETA" << std::endl;

        suzume::NormalizeOptions customOpt;
        customOpt.form = suzume::NormalizationForm::NFKC;
        customOpt.threads = 4;
        customOpt.progressCallback = customProgressCallback;

        std::string customOutput = outputDir + "/custom_output.tsv";
        auto customResult = suzume::normalize(testFile, customOutput, customOpt);

        std::cout << "Custom format processing complete!" << std::endl;
        std::cout << "Processed " << customResult.rows << " rows in "
                  << customResult.elapsedMs << " ms" << std::endl;

        // Clean up
        std::cout << "\nCleaning up temporary files..." << std::endl;
        std::filesystem::remove(testFile);
        std::filesystem::remove_all(outputDir);

        std::cout << "Cleanup complete!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
