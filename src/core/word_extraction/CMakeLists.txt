# Word extraction component source files
set(WORD_EXTRACTION_SOURCES
  trie.cpp
  generator.cpp
  verifier.cpp
  filter.cpp
  ranker.cpp
)

# Add sources to parent target
target_sources(suzume_core_lib PRIVATE ${WORD_EXTRACTION_SOURCES})

# Include directories
target_include_directories(suzume_core_lib PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)
