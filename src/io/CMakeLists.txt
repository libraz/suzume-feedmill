# IO component library
add_library(suzume_io
  file_io.cpp
)

# Include directories
target_include_directories(suzume_io PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(suzume_io PUBLIC
)

# Set C++ standard
target_compile_features(suzume_io PUBLIC cxx_std_17)
