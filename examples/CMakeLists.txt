cmake_minimum_required(VERSION 3.15)
project(suzume_feedmill_examples LANGUAGES CXX)

# Use C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Try to find suzume_feedmill package first
find_package(suzume_feedmill QUIET)

if(suzume_feedmill_FOUND)
    message(STATUS "Found suzume_feedmill package")
    set(SUZUME_INCLUDE_DIRS ${suzume_feedmill_INCLUDE_DIRS})
    set(SUZUME_LIBRARIES suzume::suzume_feedmill_core)
else()
    message(STATUS "suzume_feedmill package not found, using pre-built library")
    
    # Check if pre-built libraries exist
    set(PARENT_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build")
    set(SUZUME_CORE_LIB "${PARENT_BUILD_DIR}/src/libsuzume_core.a")
    set(SUZUME_CORE_LIB_IMPL "${PARENT_BUILD_DIR}/src/core/libsuzume_core_lib.a")
    set(SUZUME_IO_LIB "${PARENT_BUILD_DIR}/src/io/libsuzume_io.a")
    set(SUZUME_PARALLEL_LIB "${PARENT_BUILD_DIR}/src/parallel/libsuzume_parallel.a")
    set(XXHASH_LIB "${PARENT_BUILD_DIR}/libxxhash.a")
    
    foreach(LIB_PATH ${SUZUME_CORE_LIB} ${SUZUME_CORE_LIB_IMPL} ${SUZUME_IO_LIB} ${SUZUME_PARALLEL_LIB} ${XXHASH_LIB})
        if(NOT EXISTS ${LIB_PATH})
            message(FATAL_ERROR "Pre-built library not found at ${LIB_PATH}. Please build the main project first:\n"
                               "  cd ${CMAKE_CURRENT_SOURCE_DIR}/.. && mkdir build && cd build && cmake .. && make")
        endif()
    endforeach()
    
    # Set include paths
    set(SUZUME_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_CURRENT_SOURCE_DIR}/../src)
    
    # Create imported library targets
    add_library(suzume_feedmill_core STATIC IMPORTED)
    set_target_properties(suzume_feedmill_core PROPERTIES IMPORTED_LOCATION ${SUZUME_CORE_LIB})
    
    add_library(suzume_core_lib STATIC IMPORTED)
    set_target_properties(suzume_core_lib PROPERTIES IMPORTED_LOCATION ${SUZUME_CORE_LIB_IMPL})
    
    add_library(suzume_io STATIC IMPORTED)
    set_target_properties(suzume_io PROPERTIES IMPORTED_LOCATION ${SUZUME_IO_LIB})
    
    add_library(suzume_parallel STATIC IMPORTED)
    set_target_properties(suzume_parallel PROPERTIES IMPORTED_LOCATION ${SUZUME_PARALLEL_LIB})
    
    add_library(xxhash STATIC IMPORTED)
    set_target_properties(xxhash PROPERTIES IMPORTED_LOCATION ${XXHASH_LIB})
    
    set(SUZUME_LIBRARIES suzume_feedmill_core suzume_core_lib suzume_io suzume_parallel xxhash)
    
    message(STATUS "Using pre-built library: ${SUZUME_LIB_PATH}")
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

    # Check for Homebrew ICU first
    if(EXISTS "/opt/homebrew/opt/icu4c@77")
        set(ICU_ROOT "/opt/homebrew/opt/icu4c@77")
        message(STATUS "Found Homebrew icu4c at: ${ICU_ROOT}")
        set(ICU_INCLUDE_DIRS "${ICU_ROOT}/include")
        set(ICU_LIBRARIES "${ICU_ROOT}/lib/libicuuc.dylib;${ICU_ROOT}/lib/libicuio.dylib")
    elseif(EXISTS "/opt/homebrew/opt/icu4c")
        set(ICU_ROOT "/opt/homebrew/opt/icu4c")
        message(STATUS "Found Homebrew icu4c at: ${ICU_ROOT}")
        set(ICU_INCLUDE_DIRS "${ICU_ROOT}/include")
        set(ICU_LIBRARIES "${ICU_ROOT}/lib/libicuuc.dylib;${ICU_ROOT}/lib/libicuio.dylib")
    endif()
else()
    # Linux/Windows ICU detection
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(ICU REQUIRED icu-uc icu-io)
        set(ICU_LIBRARIES ${ICU_LIBRARIES})
        set(ICU_INCLUDE_DIRS ${ICU_INCLUDE_DIRS})
    endif()
endif()

# Language selection - default to English
set(EXAMPLE_LANG "en" CACHE STRING "Example language (en or ja)")

# Example source files
set(EXAMPLE_SOURCES
    normalize-basic-example.cpp
    normalize-sampling-example.cpp
    progress-with-eta-example.cpp
    stats-simple-example.cpp
    streaming-io-example.cpp
)

# Set language-specific source directory
set(EXAMPLE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_LANG}")

# Verify language directory exists
if(NOT EXISTS ${EXAMPLE_SOURCE_DIR})
    message(FATAL_ERROR "Example language directory not found: ${EXAMPLE_SOURCE_DIR}")
endif()

# Create example executables
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
    set(FULL_SOURCE_PATH "${EXAMPLE_SOURCE_DIR}/${EXAMPLE_SOURCE}")
    
    # Check if source file exists
    if(NOT EXISTS ${FULL_SOURCE_PATH})
        message(WARNING "Example source not found: ${FULL_SOURCE_PATH}")
        continue()
    endif()
    
    add_executable(${EXAMPLE_NAME} ${FULL_SOURCE_PATH})
    
    # Link with suzume libraries
    target_link_libraries(${EXAMPLE_NAME} PRIVATE ${SUZUME_LIBRARIES})
    
    # Include directories
    target_include_directories(${EXAMPLE_NAME} PRIVATE
        ${SUZUME_INCLUDE_DIRS}
    )
    
    # Link with ICU libraries
    if(ICU_LIBRARIES)
        target_include_directories(${EXAMPLE_NAME} PRIVATE ${ICU_INCLUDE_DIRS})
        target_link_libraries(${EXAMPLE_NAME} PRIVATE ${ICU_LIBRARIES})
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(${EXAMPLE_NAME} PRIVATE icuuc icuio)
    elseif(WIN32)
        target_link_libraries(${EXAMPLE_NAME} PRIVATE icuuc icuio)
    endif()
    
    # macOS specific linker flags
    if(APPLE AND MACOS_SDK_PATH)
        target_link_options(${EXAMPLE_NAME} PRIVATE -isysroot ${MACOS_SDK_PATH})
    endif()
    
    # Set C++ standard
    target_compile_features(${EXAMPLE_NAME} PRIVATE cxx_std_17)
endforeach()

message(STATUS "Examples configured: ${EXAMPLE_SOURCES}")