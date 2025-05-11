/**
 * @file file_io.h
 * @brief Simple file I/O utilities
 */

#ifndef SUZUME_IO_FILE_IO_H_
#define SUZUME_IO_FILE_IO_H_

#include <string>
#include <vector>
#include <functional>
#include <iostream>

namespace suzume {
namespace io {

/**
 * @brief Simple file reader class
 */
class TextFileReader {
public:
    /**
     * @brief Check if path is stdin indicator
     *
     * @param path File path
     * @return true If path is "-" (stdin)
     * @return false Otherwise
     */
    static bool isStdin(const std::string& path);

    /**
     * @brief Read all lines from a file or stdin
     *
     * @param path File path or "-" for stdin
     * @param progressCallback Progress callback function
     * @return std::vector<std::string> Lines read from the file or stdin
     */
    static std::vector<std::string> readAllLines(
        const std::string& path,
        const std::function<void(double)>& progressCallback = nullptr
    );

    /**
     * @brief Process a file or stdin line by line
     *
     * @param path File path or "-" for stdin
     * @param lineProcessor Function to process each line
     * @param progressCallback Progress callback function
     */
    static void processLineByLine(
        const std::string& path,
        const std::function<void(const std::string&)>& lineProcessor,
        const std::function<void(double)>& progressCallback = nullptr
    );

    /**
     * @brief Read entire file or stdin content as a string
     *
     * @param path File path or "-" for stdin
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
     * @brief Check if path is stdout indicator
     *
     * @param path File path
     * @return true If path is "-" (stdout)
     * @return false Otherwise
     */
    static bool isStdout(const std::string& path);

    /**
     * @brief Write lines to a file or stdout
     *
     * @param path File path or "-" for stdout
     * @param lines Lines to write
     * @param progressCallback Progress callback function
     */
    static void writeLines(
        const std::string& path,
        const std::vector<std::string>& lines,
        const std::function<void(double)>& progressCallback = nullptr
    );

    /**
     * @brief Write content to a file or stdout
     *
     * @param path File path or "-" for stdout
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
