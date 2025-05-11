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

bool TextFileReader::isStdin(const std::string& path) {
    return path == "-";
}

bool TextFileWriter::isStdout(const std::string& path) {
    return path == "-";
}

std::vector<std::string> TextFileReader::readAllLines(
    const std::string& path,
    const std::function<void(double)>& progressCallback
) {
    // Check if reading from stdin
    if (isStdin(path)) {
        // Read from stdin
        std::vector<std::string> lines;
        std::string line;

        while (std::getline(std::cin, line)) {
            lines.push_back(line);

            // No progress reporting for stdin (unknown size)
        }

        // Final progress update
        if (progressCallback) {
            progressCallback(1.0);
        }

        return lines;
    } else {
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
            // Remove carriage return if present (for Windows CRLF handling)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
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
}

void TextFileReader::processLineByLine(
    const std::string& path,
    const std::function<void(const std::string&)>& lineProcessor,
    const std::function<void(double)>& progressCallback
) {
    // Check if reading from stdin
    if (isStdin(path)) {
        // Process lines from stdin
        std::string line;

        while (std::getline(std::cin, line)) {
            lineProcessor(line);

            // No progress reporting for stdin (unknown size)
        }

        // Final progress update
        if (progressCallback) {
            progressCallback(1.0);
        }
    } else {
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
            // Remove carriage return if present (for Windows CRLF handling)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
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
}

std::string TextFileReader::readFileContent(
    const std::string& path,
    const std::function<void(double)>& progressCallback
) {
    // Check if reading from stdin
    if (isStdin(path)) {
        // Read from stdin
        std::string content;
        std::string line;

        while (std::getline(std::cin, line)) {
            content += line + '\n';

            // No progress reporting for stdin (unknown size)
        }

        // Final progress update
        if (progressCallback) {
            progressCallback(1.0);
        }

        return content;
    } else {
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

        // Normalize line endings to \n
        std::string normalizedContent;
        normalizedContent.reserve(content.size());

        for (size_t i = 0; i < content.size(); ++i) {
            if (content[i] == '\r' && i + 1 < content.size() && content[i + 1] == '\n') {
                // Skip the \r in \r\n sequence
                continue;
            }
            normalizedContent.push_back(content[i]);
        }

        content = std::move(normalizedContent);

        // Final progress update
        if (progressCallback && fileSize > 0) {
            progressCallback(1.0);
        }

        return content;
    }
}

void TextFileWriter::writeLines(
    const std::string& path,
    const std::vector<std::string>& lines,
    const std::function<void(double)>& progressCallback
) {
    // Check if writing to stdout
    if (isStdout(path)) {
        // Write to stdout
        size_t totalLines = lines.size();
        for (size_t i = 0; i < totalLines; ++i) {
            std::cout << lines[i] << '\n';

            // Report progress
            if (progressCallback && totalLines > 0) {
                double progress = static_cast<double>(i + 1) / totalLines;
                progressCallback(progress);
            }
        }

        // Flush stdout
        std::cout.flush();

        // Final progress update
        if (progressCallback) {
            progressCallback(1.0);
        }
    } else {
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
}

void TextFileWriter::writeContent(
    const std::string& path,
    const std::string& content
) {
    // Check if writing to stdout
    if (isStdout(path)) {
        // Write to stdout
        std::cout << content;
        std::cout.flush();
    } else {
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
}

} // namespace io
} // namespace suzume
