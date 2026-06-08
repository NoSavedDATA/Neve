CXX := clang++-19 -std=c++17
MAIN_CXXFLAGS := -rdynamic -fno-exceptions -march=native -mavx -mavx2 -O3
LLVM_CONFIG := llvm-config-19 --link-static --libs core orcjit native
SYSTEM_LIBS := -latomic -ldl -lrt -pthread #-fsanitize=thread -g -O1 -fno-omit-frame-pointer
OTHER_FLAGS := -D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH -finline-functions -funroll-loops -w -flto

# Get LLVM flags (must be from static LLVM build)
LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_SYSTEM_LIBS := $(shell $(LLVM_CONFIG) --system-libs)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs core orcjit native)


# Combine all flags
CXXFLAGS := $(MAIN_CXXFLAGS) $(LLVM_CXXFLAGS) -mavx -w
LDFLAGS := $(LLVM_LDFLAGS)
LIBS := $(LLVM_LIBS) $(LLVM_SYSTEM_LIBS) $(SYSTEM_LIBS)

SYS_LIBS += -Wl,--disable-new-dtags \
           -Wl,-rpath,'$$ORIGIN/../sys_lib'
LDFLAGS += $(SYS_LIBS)


# Directories
LIB_PARSER_OBJ_DIR = lib_parser_obj
LIB_PARSER_SRC_DIR = lib_parser
RUNTIME_DIR = src/runtime
TOKENIZERS_DIR = src/compiler_frontend/tokenizers
RUNTIME_OBJ_DIR = obj/runtime
OBJ_DIR = obj
BIN_DIR = bin
STATIC_DIR = static
SRC_DIR = src
LIB_DIR := obj_static



# C++ Source and Object Files
CXX_SRC = $(shell find $(SRC_DIR) -name "*.cpp")
CXX_OBJ = $(CXX_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
CXX_DIR = $(sort $(dir $(CXX_OBJ)))

OBJ_DIRS := $(sort $(CXX_DIR))

# Runtime obj
RUNTIME_CPP_OBJ = $(shell find $(RUNTIME_OBJ_DIR) -name "*.o")

# Lib Parser Object Files
LIB_PARSER_SRC = $(shell find $(LIB_PARSER_SRC_DIR) -name "*.cpp")
TOKENIZERS_SRC = $(shell find $(TOKENIZERS_DIR) -name "*.cpp")





# Runtime
RUNTIME_OBJ := runtime.o
RUNTIME_SRC := runtime.cpp
RUNTIME_LIB := static/runtime.a
# Executable name
LIB_PARSER := bin/lib_parser.o
OBJ := bin/neve
SRC := main.cpp
COMPILER_OBJ := bin/nsc
COMPILER_SRC := main_compiler.cpp

.PHONY: prebuild

BUILD_FLAG := .build_flag



$(info var is: ${OBJ_DIRS})
$(foreach dir, $(OBJ_DIRS), \
  $(info var is: $(dir)) \
  $(shell mkdir -p $(dir)); \
)

$(shell mkdir -p $(BIN_DIR);)
$(shell mkdir -p $(LIB_DIR);)
$(shell mkdir -p $(STATIC_DIR);)


$(shell mkdir -p $(LIB_PARSER_OBJ_DIR);)




all: prebuild $(CXX_OBJ) $(OBJ) runtime check_done


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | prebuild
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(OBJ): $(SRC) $(CXX_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SRC) $(CXX_OBJ) $(LIBS) $(OTHER_FLAGS) -MMD -MP -o $(OBJ) 
	@echo "\033[1;32m\nBuild completed [✓]\n\033[0m"
	@touch $(BUILD_FLAG)


prebuild: $(LIB_PARSER)
	@echo ">>> PREBUILD STEP <<<"
	$(shell bin/lib_parser.o;)

$(LIB_PARSER_OBJ_DIR)/%.o: $(LIB_PARSER_SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(LIB_PARSER) : $(LIB_PARSER_SRC)
	$(CXX) $(CXXFLAGS) $(LIB_PARSER_SRC) $(TOKENIZERS_SRC) -o $(LIB_PARSER)
	@echo "------------PREBUILD DONE DEON DEONDEON DEON ODENDEON"



check_done:
	@if [ ! -f $(BUILD_FLAG) ]; then \
		echo "\n\n\033[1;33mNo changes found [ ]\n\033[0m"; \
	fi
	@rm -f $(BUILD_FLAG)


runtime: $(RUNTIME_LIB)
	@echo "\033[1;32mRuntime build complete [✓]\033[0m"
$(RUNTIME_LIB): $(RUNTIME_OBJ) $(RUNTIME_CPP_OBJ)
	ar rcs $(RUNTIME_LIB) $(RUNTIME_OBJ) $(RUNTIME_CPP_OBJ)
	@echo "\033[1;34mCreated libruntime.a\033[0m"


clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR) $(LIB_PARSER_OBJ_DIR) $(STATIC_DIR)

# Track dependencies
-include $(CXX_OBJ:.o=.d)
