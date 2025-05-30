# WebAssembly build configuration

# Create a WebAssembly target
add_library(suzume_wasm_lib STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/wasm_main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wasm_exports.cpp
)

# Link with core libraries
target_link_libraries(suzume_wasm_lib PRIVATE
  suzume_core_lib
  suzume_io
  suzume_parallel
)

# Include directories
target_include_directories(suzume_wasm_lib PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/third_party
)

# Set C++ standard
target_compile_features(suzume_wasm_lib PRIVATE cxx_std_17)

# Emscripten specific settings
if(EMSCRIPTEN)
  # Set compile options directly on the target
  target_compile_options(suzume_wasm_lib PRIVATE
    -fvisibility=default
  )
endif()

# Note: The main executable is now created in the root CMakeLists.txt
# This library provides the necessary components for the WebAssembly build
