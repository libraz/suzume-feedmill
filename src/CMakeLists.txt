# Add subdirectories for each component
add_subdirectory(core)
add_subdirectory(io)
add_subdirectory(parallel)
if(BUILD_CLI)
  add_subdirectory(cli)
endif()
if(BUILD_WASM)
  add_subdirectory(wasm)
endif()

# Main library (STATIC library that links to other components)
add_library(suzume_core STATIC
  normalize_wrapper.cpp
)

# Link all components
target_link_libraries(suzume_core PUBLIC
  suzume_core_lib
  suzume_io
  suzume_parallel
)

# Include directories
target_include_directories(suzume_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/third_party>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/third_party/robin_hood>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/third_party/simdutf/include>
  $<INSTALL_INTERFACE:include>
)

# Set C++ standard
target_compile_features(suzume_core PUBLIC cxx_std_17)

# Export symbols
include(GenerateExportHeader)
generate_export_header(suzume_core
  EXPORT_MACRO_NAME SUZUME_EXPORT
  EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/suzume_feedmill_export.h
)

# Add a dummy source file to ensure the library has at least one compilation unit
file(WRITE ${CMAKE_BINARY_DIR}/suzume_feedmill_core_dummy.cpp
  "// Auto-generated dummy file to ensure the library has at least one compilation unit\n"
  "#include \"suzume_feedmill.h\"\n"
  "namespace suzume { void dummy_function() {} }\n"
)
target_sources(suzume_core PRIVATE ${CMAKE_BINARY_DIR}/suzume_feedmill_core_dummy.cpp)

# Install
install(TARGETS suzume_core suzume_core_lib suzume_io suzume_parallel
  EXPORT suzume_feedmill-targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(FILES
  ${CMAKE_SOURCE_DIR}/include/suzume_feedmill.h
  ${CMAKE_BINARY_DIR}/suzume_feedmill_export.h
  DESTINATION include
)
