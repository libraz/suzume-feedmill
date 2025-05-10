# CLI executable
add_executable(suzume_feedmill_cli
  main.cpp
  options.cpp
)

# Include directories
target_include_directories(suzume_feedmill_cli PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
  ${cli11_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(suzume_feedmill_cli PUBLIC
  suzume_core  # Use the main library instead of individual components
)

# Add CLI11 only if not building for WASM
if(NOT EMSCRIPTEN AND NOT BUILD_WASM)
  target_link_libraries(suzume_feedmill_cli PUBLIC
    CLI11::CLI11  # CLI11 library for command-line parsing
  )
endif()

# Set C++ standard
target_compile_features(suzume_feedmill_cli PUBLIC cxx_std_17)

# Set output name and directory
set_target_properties(suzume_feedmill_cli PROPERTIES
  OUTPUT_NAME "suzume-feedmill"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Add a custom command to copy the binary to a known location
add_custom_command(
  TARGET suzume_feedmill_cli
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:suzume_feedmill_cli>
    "${CMAKE_BINARY_DIR}/suzume-feedmill"
  COMMENT "Copying binary to ${CMAKE_BINARY_DIR}/suzume-feedmill"
)

# Install
install(TARGETS suzume_feedmill_cli
  RUNTIME DESTINATION bin
)
