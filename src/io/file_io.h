/**
 * @file file_io.h
 * @brief Simple file I/O utilities
 */

#ifndef SUZUME_IO_FILE_IO_H_
#define SUZUME_IO_FILE_IO_H_

#include <string>
#include <vector>
#include <functional>

namespace suzume {
namespace io {

/**
 * @brief Simple file reader class
 */
class TextFileReader {
public:
    /**
     * @brief Read all lines from a file
     *
     * @param path File path
     * @param progressCallback Progress callback function
     * @return std::vector<std::string> Lines read from the file
     */
    static std::vector<std::string> readAllLines(
        const std::string& path,
        const std::function<void(double)>& progressCallback = nullptr
    );

    /**
     * @brief Process a file line by line
     *
     * @param path File path
     * @param lineProcessor Function to process each line
     * @param progressCallback Progress callback function
     */
    static void processLineByLine(
        const std::string& path,
        const std::function<void(const std::string&)>& lineProcessor,
        const std::function<void(double)>& progressCallback = nullptr
    );

    /**
     * @brief Read entire file content as a string
     *
     * @param path File path
     * @param progressCallback Progress callback function
     * @return std::string File content
     */
    static std::string readFileContent(
        const std::string& path,
        const std::function<void(double)>& progressCallback = nullptr
    );
};

/**
 * @brief Simple file writer class
 */
class TextFileWriter {
public:
    /**
     * @brief Write lines to a file
     *
     * @param path File path
     * @param lines Lines to write
     * @param progressCallback Progress callback function
     */
    static void writeLines(
        const std::string& path,
        const std::vector<std::string>& lines,
        const std::function<void(double)>& progressCallback = nullptr
    );

    /**
     * @brief Write content to a file
     *
     * @param path File path
     * @param content Content to write
     */
    static void writeContent(
        const std::string& path,
        const std::string& content
    );
};

} // namespace io
} // namespace suzume

#endif // SUZUME_IO_FILE_IO_H_
