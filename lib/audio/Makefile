# Makefile — audio lib (official Neve FFI pattern).
# Based on the official example_lib Makefile:
#   https://github.com/NoSavedDATA/example_lib
# Builds lib.so, loaded via dlopen on `import audio`.
# Does not require rebuilding the neve compiler.

CXX := clang++-21
CXXFLAGS := -g -O3 -rdynamic -fPIC -mavx -fno-math-errno -fno-trapping-math -fopenmp-simd -w -std=c++17
SYSTEM_LIBS := -ldl -lrt -pthread -lm
OTHER_FLAGS := -D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH -flto -finline-functions -funroll-loops -w

LIBS := $(SYSTEM_LIBS)

OBJ_DIR = obj
SRC_DIR = src

CXX_SRC = $(shell find $(SRC_DIR) -name "*.cpp")
CXX_OBJ = $(CXX_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
CXX_DIR = $(sort $(dir $(CXX_OBJ)))

OBJ_DIRS := $(sort $(CXX_DIR))

SO_FILE := lib.so

.PHONY: prebuild

BUILD_FLAG := .build_flag

$(info var is: ${OBJ_DIRS})
$(foreach dir, $(OBJ_DIRS), \
  $(info var is: $(dir)) \
  $(shell mkdir -p $(dir)); \
)

all: $(CXX_OBJ) $(SO_FILE) check_done


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<


$(SO_FILE): $(CXX_OBJ)
	$(CXX) -shared $(CXXFLAGS) $(CXX_OBJ) $(LIBS) $(OTHER_FLAGS) -MMD -MP -o $(SO_FILE)
	@echo "\033[1;32m\nBuild completed [✓]\n\033[0m"
	@touch $(BUILD_FLAG)


check_done:
	@if [ ! -f $(BUILD_FLAG) ]; then \
		echo "\n\n\033[1;33mNo changes found [ ]\n\033[0m"; \
	fi
	@rm -f $(BUILD_FLAG)

clean:
	rm -rf $(OBJ_DIR) $(SO_FILE)

-include $(CXX_OBJ:.o=.d)
