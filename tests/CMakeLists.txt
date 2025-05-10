# Enable testing
enable_testing()

# Find GoogleTest
find_package(GTest QUIET)
if(GTEST_FOUND)
  message(STATUS "GoogleTest found, building tests")
  include_directories(${GTEST_INCLUDE_DIRS})

  # Test source files
  set(TEST_SOURCES
    # Core layer tests
    core/text_utils_test.cpp
    core/normalize_test.cpp
    core/pmi_test.cpp
    core/normalize_unicode_test.cpp
    core/normalize_error_test.cpp
    core/normalize_parallel_test.cpp
    core/normalize_detailed_error_test.cpp
    core/pmi_error_test.cpp
    core/pmi_advanced_test.cpp
    core/performance_test.cpp
    core/progress_callback_test.cpp
    core/buffer_api_test.cpp
    core/shared_memory_test.cpp
    core/shared_memory_mock.cpp
    core/word_extraction_test.cpp
    core/word_extraction_trie_test.cpp
    core/word_extraction_generator_test.cpp
    core/word_extraction_generator_advanced_test.cpp
    core/word_extraction_verifier_test.cpp
    core/word_extraction_filter_test.cpp
    core/word_extraction_ranker_test.cpp

    # IO layer tests
    io/file_io_test.cpp

    # Parallel layer tests
    parallel/executor_test.cpp

    # CLI layer tests
    cli/options_test.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/options.cpp

    # Main test file
    cpp/main_test.cpp
  )

  # Test executable
  add_executable(suzume_feedmill_test ${TEST_SOURCES})
  target_include_directories(suzume_feedmill_test PRIVATE
    ${cli11_SOURCE_DIR}/include
  )
  target_link_libraries(suzume_feedmill_test PRIVATE
    suzume_feedmill_core # Use the main library instead of individual components
    CLI11::CLI11 # CLI11 library for command-line parsing
    GTest::GTest
    GTest::Main
  )

  # Add pthread only on non-Windows platforms
  if(NOT WIN32)
    target_link_libraries(suzume_feedmill_test PRIVATE pthread)
  endif()

  # Add test to CTest
  add_test(NAME suzume_feedmill_test COMMAND suzume_feedmill_test)

  # Coverage settings
  if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      # GCC specific coverage flags
      target_compile_options(suzume_feedmill_test PRIVATE -fprofile-arcs -ftest-coverage)
      target_link_options(suzume_feedmill_test PRIVATE -fprofile-arcs -ftest-coverage)
      message(STATUS "Applied GCC coverage flags to test executable")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      # Clang specific coverage flags
      target_compile_options(suzume_feedmill_test PRIVATE -fprofile-instr-generate -fcoverage-mapping)
      target_link_options(suzume_feedmill_test PRIVATE -fprofile-instr-generate -fcoverage-mapping)
      message(STATUS "Applied Clang coverage flags to test executable")
    endif()
  endif()
else()
  message(STATUS "GoogleTest not found, using FetchContent")

  # Use FetchContent to get GoogleTest
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
  )

  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  # Make GoogleTest available
  FetchContent_MakeAvailable(googletest)

  # Test source files
  set(TEST_SOURCES
    # Core layer tests
    core/text_utils_test.cpp
    core/normalize_test.cpp
    core/pmi_test.cpp
    core/normalize_unicode_test.cpp
    core/normalize_error_test.cpp
    core/normalize_parallel_test.cpp
    core/normalize_detailed_error_test.cpp
    core/pmi_error_test.cpp
    core/pmi_advanced_test.cpp
    core/performance_test.cpp
    core/progress_callback_test.cpp
    core/buffer_api_test.cpp
    core/shared_memory_test.cpp
    core/shared_memory_mock.cpp
    core/word_extraction_test.cpp
    core/word_extraction_trie_test.cpp
    core/word_extraction_generator_test.cpp
    core/word_extraction_generator_advanced_test.cpp
    core/word_extraction_verifier_test.cpp
    core/word_extraction_filter_test.cpp
    core/word_extraction_ranker_test.cpp

    # IO layer tests
    io/file_io_test.cpp

    # Parallel layer tests
    parallel/executor_test.cpp

    # CLI layer tests
    cli/options_test.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/options.cpp

    # Main test file
    cpp/main_test.cpp
  )

  # Test executable
  add_executable(suzume_feedmill_test ${TEST_SOURCES})
  target_include_directories(suzume_feedmill_test PRIVATE
    ${cli11_SOURCE_DIR}/include
  )
  target_link_libraries(suzume_feedmill_test PRIVATE
    suzume_feedmill_core # Use the main library instead of individual components
    CLI11::CLI11 # CLI11 library for command-line parsing
    gtest
    gtest_main
  )

  # Add pthread only on non-Windows platforms
  if(NOT WIN32)
    target_link_libraries(suzume_feedmill_test PRIVATE pthread)
  endif()

  # Add test to CTest
  add_test(NAME suzume_feedmill_test COMMAND suzume_feedmill_test)

  # Coverage settings
  if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      # GCC specific coverage flags
      target_compile_options(suzume_feedmill_test PRIVATE -fprofile-arcs -ftest-coverage)
      target_link_options(suzume_feedmill_test PRIVATE -fprofile-arcs -ftest-coverage)
      message(STATUS "Applied GCC coverage flags to test executable (FetchContent)")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      # Clang specific coverage flags
      target_compile_options(suzume_feedmill_test PRIVATE -fprofile-instr-generate -fcoverage-mapping)
      target_link_options(suzume_feedmill_test PRIVATE -fprofile-instr-generate -fcoverage-mapping)
      message(STATUS "Applied Clang coverage flags to test executable (FetchContent)")
    endif()
  endif()
endif()
