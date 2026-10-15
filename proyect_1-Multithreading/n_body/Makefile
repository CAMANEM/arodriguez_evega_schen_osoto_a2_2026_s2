# ===== CONFIGURATION ==========================================================================

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall

# Adjust RAYLIB_PATH to your Raylib installation directory
RAYLIB_PATH = C:/raylib/w64devkit

INCLUDE_DIRS = -I./include -I../shared/include -I$(RAYLIB_PATH)/include
LIB_DIRS     = -L$(RAYLIB_PATH)/lib
LIBS         = -lraylib -lopengl32 -lgdi32 -lwinmm

# ===== FILE STRUCTURE =========================================================================

SRC_DIR = src
OBJ_DIR = obj
BIN     = n_body

SRCS = $(SRC_DIR)/main.cpp $(SRC_DIR)/body.cpp
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# ===== TARGETS ================================================================================

.PHONY: all clean

all: $(OBJ_DIR) $(BIN)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN): $(OBJS)
	$(CXX) $(OBJS) $(LIB_DIRS) $(LIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(BIN).exe
