# Parallel component library
add_library(suzume_parallel
  executor.cpp
)

# Include directories
target_include_directories(suzume_parallel PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(suzume_parallel PUBLIC
)

# Set C++ standard
target_compile_features(suzume_parallel PUBLIC cxx_std_17)
