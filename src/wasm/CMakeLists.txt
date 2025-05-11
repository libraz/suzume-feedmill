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
  # Use a completely different approach to separate compile and link flags
  # Set compile options directly on the target
  target_compile_options(suzume_wasm_lib PRIVATE
    -fvisibility=default
  )

  # Create a custom link command that will be executed after compilation
  # This ensures the link flags are only used during linking
  add_custom_command(
    TARGET suzume_wasm_lib
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Applying Emscripten link flags"
    COMMENT "Setting up Emscripten link flags"
  )

  # Store the link flags in a variable for later use in the final executable
  set(EMSCRIPTEN_LINK_FLAGS
    "-s EXPORT_NAME='SuzumeFeedmill' -s MODULARIZE=1 -s EXPORT_ES6=1 -s USE_ES6_IMPORT_META=0 -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=1 -s WASM=1 -s NO_EXIT_RUNTIME=1"
    PARENT_SCOPE
  )
endif()

# Note: The main executable is now created in the root CMakeLists.txt
# This library provides the necessary components for the WebAssembly build
