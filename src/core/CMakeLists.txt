# Core component library
add_library(suzume_core_lib
  normalize.cpp
  pmi.cpp
  text_utils.cpp
  buffer_api.cpp
  word_extraction.cpp
)

# Add word_extraction subdirectory
add_subdirectory(word_extraction)

# Include directories
target_include_directories(suzume_core_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(suzume_core_lib PUBLIC
  ${ICU_LIBRARIES}
  xxhash
)

# Set C++ standard
target_compile_features(suzume_core_lib PUBLIC cxx_std_17)
