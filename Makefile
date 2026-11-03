SRC_DIR=src
OBJ_DIR=build
MOLY_DIR=src
CXX=clang++

DEFAULT_NET_NAME=leecheese.nnue
DEFAULT_WDL_HEAD_NAME=leecheese.nnue

DEFAULT_EXE = $(OBJ_DIR)/Molybdenum
DATAGEN_EXE = $(OBJ_DIR)/Datagen
PREPROCESS_EXE = $(OBJ_DIR)/Preprocess
DEFAULT_NET = $(DEFAULT_NET_NAME)
DEFAULT_WDL_HEAD = $(DEFAULT_WDL_HEAD_NAME)

PREPROCESSED_NET = $(OBJ_DIR)/preprocessed.nnue

EVALFILE ?= $(DEFAULT_NET)
WDLFILE = $(EVALFILE)

all: $(DEFAULT_EXE) clean_preprocess

datagen: $(DATAGEN_EXE) clean_preprocess

# Override STATIC_LINK and ARCH_FLAGS when cross-compiling (for example, Android NDK).
STATIC_LINK ?= -static
ARCH_FLAGS ?= -march=native
COMPILE_FLAGS ?=
LINK_FLAGS ?=

# The NNUE preprocessing tool must run on the build host during cross-compiles.
PREPROCESS_CXX ?= $(CXX)
PREPROCESS_CXXFLAGS ?= $(CXXFLAGS)
PREPROCESS_LDFLAGS ?= $(LDFLAGS)

CXXFLAGS = $(STATIC_LINK) -Ofast -std=c++17
CXXFLAGS += -Wall -Wextra -pedantic $(ARCH_FLAGS) $(COMPILE_FLAGS)
CXXFLAGS += -DWDLFILE=\"$(WDLFILE)\"

LDFLAGS = $(STATIC_LINK) -Ofast $(ARCH_FLAGS) $(LINK_FLAGS)

ifeq ($(OS),Windows_NT)
	LDFLAGS += -Wl,/STACK:4294967296
endif

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
DG_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%_dg.o, $(SOURCES)) $(OBJ_DIR)/Datagen.o
PP_OBJECTS := $(OBJ_DIR)/preprocess.o

$(OBJ_DIR)/%_dg.o: $(SRC_DIR)/%.cpp $(PREPROCESSED_NET)
	$(CXX) -I$(SRC_DIR) $(CXXFLAGS) -DEVALFILE=\"$(PREPROCESSED_NET)\" -DDATAGEN -c $< -o $@

$(OBJ_DIR)/Datagen.o: $(SRC_DIR)/Datagen/Datagen.cpp $(PREPROCESSED_NET)
	$(CXX) -I$(SRC_DIR) $(CXXFLAGS) -DEVALFILE=\"$(PREPROCESSED_NET)\" -DDATAGEN -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(PREPROCESSED_NET)
	$(CXX) -I$(SRC_DIR) $(CXXFLAGS) -DEVALFILE=\"$(PREPROCESSED_NET)\" -c $< -o $@

$(OBJ_DIR)/preprocess.o: $(SRC_DIR)/util/preprocess.cpp
	$(PREPROCESS_CXX) -I$(SRC_DIR) $(PREPROCESS_CXXFLAGS) -DEVALFILE=\"$(EVALFILE)\" -c $< -o $@

$(OBJECTS): | $(OBJ_DIR)

$(DG_OBJECTS): | $(OBJ_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

EXE ?= $(DEFAULT_EXE)

$(DEFAULT_EXE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $(DEFAULT_EXE)
	@if [ $(EXE) != $(DEFAULT_EXE) ]; then cp $(DEFAULT_EXE) $(EXE); fi

$(DATAGEN_EXE): $(DG_OBJECTS)
	$(CXX) $(LDFLAGS) $(DG_OBJECTS) -o $(DATAGEN_EXE)

$(PREPROCESS_EXE): $(PP_OBJECTS) $(EVALFILE)
	$(PREPROCESS_CXX) $(PREPROCESS_LDFLAGS) $(PP_OBJECTS) -o $(PREPROCESS_EXE)

$(PREPROCESSED_NET): $(PREPROCESS_EXE)
	./$(PREPROCESS_EXE) $(PREPROCESSED_NET)

.PHONY: clean_preprocess
clean_preprocess:
	rm -f $(PREPROCESSED_NET)

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(DG_OBJECTS) $(PP_OBJECTS) $(DEFAULT_EXE) $(DATAGEN_EXE) $(PREPROCESS_EXE)
