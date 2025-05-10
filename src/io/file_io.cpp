/**
 * @file file_io.cpp
 * @brief Implementation of simple file I/O utilities
 */

#include "io/file_io.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <stdexcept>

namespace suzume {
namespace io {

std::vector<std::string> TextFileReader::readAllLines(
    const std::string& path,
    const std::function<void(double)>& progressCallback
) {
    // Check if file exists
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File does not exist: " + path);
    }

    // Get file size for progress reporting
    size_t fileSize = 0;
    try {
        fileSize = std::filesystem::file_size(path);
    } catch (const std::exception& e) {
        // Continue without progress reporting
    }

    // Open file
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    // Read lines
    std::vector<std::string> lines;
    std::string line;
    size_t bytesRead = 0;

    while (std::getline(file, line)) {
        lines.push_back(line);

        bytesRead += line.size() + 1; // +1 for newline

        // Report progress
        if (progressCallback && fileSize > 0) {
            double progress = static_cast<double>(bytesRead) / fileSize;
            progressCallback(progress);
        }
    }

    // Final progress update
    if (progressCallback && fileSize > 0) {
        progressCallback(1.0);
    }

    return lines;
}

void TextFileReader::processLineByLine(
    const std::string& path,
    const std::function<void(const std::string&)>& lineProcessor,
    const std::function<void(double)>& progressCallback
) {
    // Check if file exists
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File does not exist: " + path);
    }

    // Get file size for progress reporting
    size_t fileSize = 0;
    try {
        fileSize = std::filesystem::file_size(path);
    } catch (const std::exception& e) {
        // Continue without progress reporting
    }

    // Open file
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    // Process lines
    std::string line;
    size_t bytesRead = 0;

    while (std::getline(file, line)) {
        lineProcessor(line);

        bytesRead += line.size() + 1; // +1 for newline

        // Report progress
        if (progressCallback && fileSize > 0) {
            double progress = static_cast<double>(bytesRead) / fileSize;
            progressCallback(progress);
        }
    }

    // Final progress update
    if (progressCallback && fileSize > 0) {
        progressCallback(1.0);
    }
}

std::string TextFileReader::readFileContent(
    const std::string& path,
    const std::function<void(double)>& progressCallback
) {
    // Check if file exists
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File does not exist: " + path);
    }

    // Get file size for progress reporting
    size_t fileSize = 0;
    try {
        fileSize = std::filesystem::file_size(path);
    } catch (const std::exception& e) {
        // Continue without progress reporting
    }

    // Open file
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    // Read file content
    std::string content;

    if (fileSize > 0) {
        content.reserve(fileSize);
    }

    char buffer[4096];
    size_t bytesRead = 0;

    while (file.read(buffer, sizeof(buffer))) {
        content.append(buffer, file.gcount());

        bytesRead += file.gcount();

        // Report progress
        if (progressCallback && fileSize > 0) {
            double progress = static_cast<double>(bytesRead) / fileSize;
            progressCallback(progress);
        }
    }

    // Append remaining bytes
    content.append(buffer, file.gcount());

    // Final progress update
    if (progressCallback && fileSize > 0) {
        progressCallback(1.0);
    }

    return content;
}

void TextFileWriter::writeLines(
    const std::string& path,
    const std::vector<std::string>& lines,
    const std::function<void(double)>& progressCallback
) {
    // Create directory if it doesn't exist
    std::filesystem::path filePath(path);
    std::filesystem::create_directories(filePath.parent_path());

    // Open file
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file for writing: " + path);
    }

    // Write lines
    size_t totalLines = lines.size();
    for (size_t i = 0; i < totalLines; ++i) {
        file << lines[i] << '\n';

        // Report progress
        if (progressCallback && totalLines > 0) {
            double progress = static_cast<double>(i + 1) / totalLines;
            progressCallback(progress);
        }
    }

    // Final progress update
    if (progressCallback) {
        progressCallback(1.0);
    }
}

void TextFileWriter::writeContent(
    const std::string& path,
    const std::string& content
) {
    // Create directory if it doesn't exist
    std::filesystem::path filePath(path);
    std::filesystem::create_directories(filePath.parent_path());

    // Open file
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file for writing: " + path);
    }

    // Write content
    file << content;
}

} // namespace io
} // namespace suzume
