# Simple Makefile for suzume-feedmill examples
# This provides an alternative to CMake for building examples

# Build directory
BUILD_DIR = build

# Parent directories
PARENT_DIR = ..
INCLUDE_DIR = $(PARENT_DIR)/include
SRC_DIR = $(PARENT_DIR)/src

# Compiler settings
CXX = c++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
INCLUDES = -I$(INCLUDE_DIR) -I$(SRC_DIR)

# Platform-specific settings
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS
    ICU_ROOT = $(shell brew --prefix icu4c 2>/dev/null || echo "/opt/homebrew/opt/icu4c")
    INCLUDES += -I$(ICU_ROOT)/include
    LDFLAGS += -L$(ICU_ROOT)/lib
    LIBS = -licuuc -licuio
    # Add macOS SDK path
    SDK_PATH = $(shell xcrun --show-sdk-path 2>/dev/null)
    ifneq ($(SDK_PATH),)
        CXXFLAGS += -isysroot $(SDK_PATH)
        LDFLAGS += -isysroot $(SDK_PATH)
        # Add C++ standard library headers explicitly
        INCLUDES += -I$(SDK_PATH)/usr/include/c++/v1
        INCLUDES += -I$(SDK_PATH)/usr/include
    endif
else ifeq ($(UNAME_S),Linux)
    # Linux
    LIBS = -licuuc -licuio
else
    # Windows (MinGW)
    LIBS = -licuuc -licuio
endif

# Language selection - default to English
LANG ?= en

# Source files from language directory
LANG_DIR = $(LANG)
SOURCES = $(wildcard $(LANG_DIR)/*.cpp)
EXAMPLES = $(notdir $(SOURCES:.cpp=))
TARGETS = $(patsubst %,$(BUILD_DIR)/%,$(EXAMPLES))

# Library dependencies (assumes main library is built)
SUZUME_LIBS = $(PARENT_DIR)/build/src/libsuzume_core.a \
              $(PARENT_DIR)/build/src/core/libsuzume_core_lib.a \
              $(PARENT_DIR)/build/src/io/libsuzume_io.a \
              $(PARENT_DIR)/build/src/parallel/libsuzume_parallel.a \
              $(PARENT_DIR)/build/libxxhash.a

.PHONY: all clean help

all: $(BUILD_DIR) $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Check if main libraries exist
check-libs:
	@for lib in $(SUZUME_LIBS); do \
		if [ ! -f "$$lib" ]; then \
			echo "Error: suzume-feedmill library not found at $$lib"; \
			echo "Please build the main library first:"; \
			echo "  cd $(PARENT_DIR) && mkdir build && cd build && cmake .. && make"; \
			exit 1; \
		fi \
	done

# Build individual examples
$(BUILD_DIR)/%: $(LANG_DIR)/%.cpp check-libs
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(SUZUME_LIBS) $(LDFLAGS) $(LIBS)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "suzume-feedmill Examples Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make all          - Build all examples (default: English)"
	@echo "  make all LANG=ja  - Build Japanese examples"
	@echo "  make all LANG=en  - Build English examples"
	@echo "  make clean        - Clean build directory"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "Language Support:"
	@echo "  LANG=en  - English examples (default)"
	@echo "  LANG=ja  - Japanese examples"
	@echo ""
	@echo "Prerequisites:"
	@echo "  1. Build the main suzume-feedmill library first:"
	@echo "     cd .. && mkdir build && cd build && cmake .. && make"
	@echo ""
	@echo "  2. Install ICU development libraries:"
	@echo "     macOS: brew install icu4c"
	@echo "     Ubuntu/Debian: sudo apt-get install libicu-dev"
	@echo "     CentOS/RHEL: sudo yum install libicu-devel"
	@echo ""
	@echo "Examples will be built in the $(BUILD_DIR)/ directory."