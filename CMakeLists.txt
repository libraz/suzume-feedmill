cmake_minimum_required(VERSION 3.15)
# Set CMake policy to suppress deprecation warnings from dependencies
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0077 NEW)
project(suzume_feedmill VERSION 0.2.0 LANGUAGES CXX C)

# Use C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_CLI "Build CLI executable" ON)
option(BUILD_TESTING "Build tests" ON) # Enable tests
option(STATIC "Build with static libraries" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(BUILD_WASM "Build WebAssembly module" OFF) # WASM build option

# Enable testing at the top level
enable_testing()

# Sanitizer options
option(ENABLE_SANITIZER "Enable sanitizers" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_MSAN "Enable MemorySanitizer" OFF)

# Check if we're using Emscripten for WebAssembly
if(EMSCRIPTEN)
  message(STATUS "Building with Emscripten for WebAssembly")
  set(BUILD_WASM ON)
  set(BUILD_CLI OFF)
  set(BUILD_TESTING OFF)

  # Emscripten-specific compiler and linker flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 -s ALLOW_MEMORY_GROWTH=1")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s WASM=1 -s ALLOW_MEMORY_GROWTH=1")

  # Export necessary functions for JavaScript
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_FUNCTIONS='[\"_normalize\", \"_calculatePmi\", \"_extractWords\", \"_malloc\", \"_free\"]' -s EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\", \"getValue\", \"setValue\", \"UTF8ToString\", \"stringToUTF8\"]'")

  # Enable Embind for easier C++ to JavaScript bindings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME=\"SuzumeFeedmill\"")

  # Add ICU support for Emscripten
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_ICU=1")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_ICU=1")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_ICU=1")

  # Enable Embind
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --bind")
endif()

# Configure sanitizers if enabled
if(ENABLE_SANITIZER)
  message(STATUS "Sanitizers enabled")

  set(SANITIZER_FLAGS "")

  if(ENABLE_ASAN OR (NOT ENABLE_ASAN AND NOT ENABLE_UBSAN AND NOT ENABLE_TSAN AND NOT ENABLE_MSAN))
    message(STATUS "AddressSanitizer enabled")
    set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=address")
  endif()

  if(ENABLE_UBSAN OR (NOT ENABLE_ASAN AND NOT ENABLE_UBSAN AND NOT ENABLE_TSAN AND NOT ENABLE_MSAN))
    message(STATUS "UndefinedBehaviorSanitizer enabled")
    set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=undefined")
  endif()

  if(ENABLE_TSAN)
    message(STATUS "ThreadSanitizer enabled")
    set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=thread")
  endif()

  if(ENABLE_MSAN)
    message(STATUS "MemorySanitizer enabled")
    set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=memory -fsanitize-memory-track-origins=2")
  endif()

  # Add frame pointers for better stack traces
  set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fno-omit-frame-pointer")

  # Apply sanitizer flags to all build types
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

# Coverage settings
if(ENABLE_COVERAGE)
  message(STATUS "Code coverage enabled")
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # GCC specific coverage flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
    message(STATUS "Using GCC coverage flags")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang specific coverage flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    message(STATUS "Using Clang coverage flags")
  else()
    message(WARNING "Code coverage is only supported with GCC or Clang")
  endif()

  # Print compiler information for debugging
  message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
  message(STATUS "C++ compiler ID: ${CMAKE_CXX_COMPILER_ID}")
  message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
endif()

# macOS specific settings
if(APPLE)
    # Get macOS SDK path
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE MACOS_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Use SDK path if available
    if(MACOS_SDK_PATH)
        message(STATUS "Using macOS SDK path: ${MACOS_SDK_PATH}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isysroot ${MACOS_SDK_PATH}")

        # Important: Explicitly specify the path to C++ standard library headers
        include_directories(SYSTEM "${MACOS_SDK_PATH}/usr/include/c++/v1")
        include_directories(SYSTEM "${MACOS_SDK_PATH}/usr/include")
    endif()

    # Add Homebrew paths
    if(EXISTS "/usr/local/include")
        include_directories(SYSTEM "/usr/local/include")
        link_directories("/usr/local/lib")
    endif()

    if(EXISTS "/opt/homebrew/include")
        include_directories(SYSTEM "/opt/homebrew/include")
        link_directories("/opt/homebrew/lib")
    endif()

    # macOS-specific compilation options
    add_compile_options(-Wno-unused-command-line-argument -Qunused-arguments)
endif()

# Compiler flags
if(MSVC)
  add_compile_options(/W4)
  # Disable specific warnings that are causing issues
  add_compile_options(/wd4456) # declaration of 'identifier' hides previous local declaration
  add_compile_options(/wd4267) # conversion from 'size_t' to 'type', possible loss of data
  add_compile_options(/wd4244) # conversion from 'type1' to 'type2', possible loss of data
  add_compile_options(/wd4101) # unreferenced local variable
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source directories
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# Include directories
include_directories(
  ${INCLUDE_DIR}
  ${SOURCE_DIR}
)

# Third-party libraries using FetchContent
include(FetchContent)

# ICU detection
if(NOT EMSCRIPTEN)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(ICU REQUIRED icu-uc icu-io)
  else()
    # Fallback to manual detection
    if(WIN32)
      # Windows-specific paths (including vcpkg)
      find_path(ICU_INCLUDE_DIRS unicode/uchar.h
        HINTS
          $ENV{VCPKG_INSTALLATION_ROOT}/installed/x64-windows/include
          C:/vcpkg/installed/x64-windows/include
          C:/Program Files/icu4c/include
      )

      find_library(ICU_UC_LIBRARY
        NAMES icuuc icuuc.lib
        HINTS
          $ENV{VCPKG_INSTALLATION_ROOT}/installed/x64-windows/lib
          C:/vcpkg/installed/x64-windows/lib
          C:/Program Files/icu4c/lib
      )

      find_library(ICU_IO_LIBRARY
        NAMES icuio icuio.lib
        HINTS
          $ENV{VCPKG_INSTALLATION_ROOT}/installed/x64-windows/lib
          C:/vcpkg/installed/x64-windows/lib
          C:/Program Files/icu4c/lib
      )
    elseif(APPLE)
      # macOS-specific paths
      # First try to use brew --prefix to find the exact Homebrew installation path
      execute_process(
        COMMAND brew --prefix icu4c
        OUTPUT_VARIABLE BREW_ICU_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE BREW_ICU_RESULT
      )

      if(BREW_ICU_RESULT EQUAL 0 AND BREW_ICU_PREFIX)
        message(STATUS "Found Homebrew icu4c at: ${BREW_ICU_PREFIX}")
        set(ICU_BREW_INCLUDE "${BREW_ICU_PREFIX}/include")
        set(ICU_BREW_LIB "${BREW_ICU_PREFIX}/lib")
      endif()

      # Use both Homebrew paths and standard paths
      find_path(ICU_INCLUDE_DIRS unicode/uchar.h
        HINTS
          ${ICU_BREW_INCLUDE}
          $ENV{CMAKE_PREFIX_PATH}/include
          /opt/homebrew/opt/icu4c/include
          /usr/local/opt/icu4c/include
          /opt/homebrew/include
          /usr/local/include
          /usr/include
      )

      find_library(ICU_UC_LIBRARY
        NAMES icuuc
        HINTS
          ${ICU_BREW_LIB}
          $ENV{CMAKE_PREFIX_PATH}/lib
          /opt/homebrew/opt/icu4c/lib
          /usr/local/opt/icu4c/lib
          /opt/homebrew/lib
          /usr/local/lib
          /usr/lib
      )

      find_library(ICU_IO_LIBRARY
        NAMES icuio
        HINTS
          ${ICU_BREW_LIB}
          $ENV{CMAKE_PREFIX_PATH}/lib
          /opt/homebrew/opt/icu4c/lib
          /usr/local/opt/icu4c/lib
          /opt/homebrew/lib
          /usr/local/lib
          /usr/lib
      )

      # Print debug information
      message(STATUS "ICU include dirs: ${ICU_INCLUDE_DIRS}")
      message(STATUS "ICU UC library: ${ICU_UC_LIBRARY}")
      message(STATUS "ICU IO library: ${ICU_IO_LIBRARY}")
    else()
      # Linux-specific paths
      find_path(ICU_INCLUDE_DIRS unicode/uchar.h
        HINTS
          /usr/include
          /usr/local/include
      )

      find_library(ICU_UC_LIBRARY
        NAMES icuuc
        HINTS
          /usr/lib
          /usr/lib64
          /usr/local/lib
      )

      find_library(ICU_IO_LIBRARY
        NAMES icuio
        HINTS
          /usr/lib
          /usr/lib64
          /usr/local/lib
      )
    endif()

    set(ICU_LIBRARIES ${ICU_UC_LIBRARY} ${ICU_IO_LIBRARY})
  endif()

  if(NOT ICU_INCLUDE_DIRS OR NOT ICU_LIBRARIES)
    message(FATAL_ERROR "ICU libraries not found. Please install ICU.")
  endif()

  message(STATUS "Found ICU: ${ICU_LIBRARIES}")
else()
  # For Emscripten, we use the built-in ICU support
  message(STATUS "Building with Emscripten: Using Emscripten's ICU port")
  # Define ICU include dirs and libraries for Emscripten
  set(ICU_INCLUDE_DIRS "")  # Emscripten includes ICU headers automatically
  set(ICU_LIBRARIES "")     # Emscripten links ICU libraries automatically
endif()

# xxHash options
set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "")
set(XXHASH_BUILD_SHARED_LIBS OFF CACHE BOOL "")

# xxHash
FetchContent_Declare(
  xxHash
  GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
  GIT_TAG v0.8.1
)

# robin_hood (header-only library)
FetchContent_Declare(
  robin_hood
  URL https://github.com/martinus/robin-hood-hashing/raw/master/src/include/robin_hood.h
  DOWNLOAD_NO_EXTRACT TRUE
  DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/robin_hood-src
)

# CLI11 (header-only library for command-line parsing)
if(NOT EMSCRIPTEN)
  FetchContent_Declare(
    cli11
    QUIET
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
  )
endif()

# nlohmann/json (header-only library for JSON processing)
# Use single header version instead of full repository to avoid CMake version issues
FetchContent_Declare(
  nlohmann_json
  QUIET
  URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
  DOWNLOAD_NO_EXTRACT TRUE
  DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/nlohmann_json-src/include/nlohmann
)

# Using ICU for Unicode processing
message(STATUS "Using ICU for Unicode processing")

# Make the dependencies available
if(NOT EMSCRIPTEN)
  FetchContent_MakeAvailable(xxHash robin_hood cli11)
else()
  FetchContent_MakeAvailable(xxHash robin_hood)
endif()

# Handle nlohmann_json manually since we're using the single header
FetchContent_GetProperties(nlohmann_json)
if(NOT nlohmann_json_POPULATED)
  FetchContent_Populate(nlohmann_json)
endif()

# Add include directories for third-party libraries and ICU
include_directories(
  ${ICU_INCLUDE_DIRS}
  ${xxHash_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/_deps/xxhash-src
  ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/robin_hood-src
  ${CMAKE_CURRENT_BINARY_DIR}/_deps/robin_hood-src
  ${CMAKE_BINARY_DIR}/_deps/xxhash-src
  ${CMAKE_BINARY_DIR}/_deps/robin_hood-src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party
  ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/nlohmann_json-src/include
)

# Copy xxhash.h to a location in the include path
configure_file(
  ${CMAKE_CURRENT_BINARY_DIR}/_deps/xxhash-src/xxhash.h
  ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/xxhash-src/xxhash.h
  COPYONLY
)

# Build xxHash library
add_library(xxhash STATIC ${CMAKE_CURRENT_BINARY_DIR}/_deps/xxhash-src/xxhash.c)
set_target_properties(xxhash PROPERTIES LINKER_LANGUAGE C)

# Add subdirectories for each component
add_subdirectory(src)

if(BUILD_TESTING AND NOT EMSCRIPTEN)
  add_subdirectory(tests)
endif()

# Create core library target
add_library(suzume_feedmill_core INTERFACE)
target_link_libraries(suzume_feedmill_core INTERFACE suzume_core)

# WebAssembly specific target
if(EMSCRIPTEN)
  # Create WASM module
  add_executable(suzume_wasm src/wasm/wasm_main.cpp src/wasm/wasm_exports.cpp)
  target_link_libraries(suzume_wasm PRIVATE suzume_core)

  # Set output name and properties
  set_target_properties(suzume_wasm PROPERTIES
    OUTPUT_NAME "suzume-feedmill"
    SUFFIX ".js"
  )

  # Install WASM module
  install(TARGETS suzume_wasm
    RUNTIME DESTINATION wasm
  )
endif()

# Installation
install(TARGETS suzume_feedmill_core xxhash
  EXPORT suzume_feedmill-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(DIRECTORY ${INCLUDE_DIR}/ DESTINATION include)

# pkg-config file
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/suzume_feedmill.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/suzume_feedmill.pc
  @ONLY
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/suzume_feedmill.pc
  DESTINATION lib/pkgconfig
)

# Export package
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/suzume_feedmill-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

# Create config file
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/suzume_feedmill-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/suzume_feedmill-config.cmake
  @ONLY
)

# Export targets
install(EXPORT suzume_feedmill-targets
  FILE suzume_feedmill-targets.cmake
  NAMESPACE suzume::
  DESTINATION lib/cmake/suzume_feedmill
)

# Install config files
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/suzume_feedmill-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/suzume_feedmill-config-version.cmake"
  DESTINATION
    lib/cmake/suzume_feedmill
)
