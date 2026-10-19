# ===== CONFIGURATION ==========================================================================

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

# Adjust RAYLIB_PATH to your Raylib installation directory
RAYLIB_PATH = C:/raylib/w64devkit

INCLUDE_DIRS = -I./include -I../shared/include -I$(RAYLIB_PATH)/include
LIB_DIRS = -L$(RAYLIB_PATH)/lib
LIBS = -lraylib -lopengl32 -lgdi32 -lwinmm

# ===== FILE STRUCTURE =========================================================================

SRC_DIR = src
OBJ_DIR = obj
BIN = n_body

SRCS = $(SRC_DIR)/main.cpp $(SRC_DIR)/body.cpp $(SRC_DIR)/fine_grained.cpp $(SRC_DIR)/coarse_grained.cpp $(SRC_DIR)/smt.cpp $(SRC_DIR)/cmp.cpp $(SRC_DIR)/stall_policy.cpp
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# ===== TARGETS ================================================================================

.PHONY: all clean run_benchmark

all: $(OBJ_DIR) $(BIN)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN): $(OBJS)
	$(CXX) $(OBJS) $(LIB_DIRS) $(LIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(BIN).exe

run_benchmark: $(BIN)
	@OUTPUT="data/execution_results_all_$$(date +%Y-%m-%d_%H-%M-%S).csv"; \
	echo "Resultados en: $$OUTPUT"; \
	echo ""; \
	echo "[1/13] Sequential | n=100"; \
	./$(BIN).exe -seq -headless -n 100   -s 100 -e 200 -o "$$OUTPUT" > /dev/null; \
	echo "[2/13] Sequential | n=1000"; \
	./$(BIN).exe -seq -headless -n 1000  -s 100 -e 200 -o "$$OUTPUT" > /dev/null; \
	echo "[3/13] Sequential | n=10000"; \
	./$(BIN).exe -seq -headless -n 10000 -s 100 -e 200 -o "$$OUTPUT" > /dev/null; \
	echo "[4/13] Fine | c=4 | n=100"; \
	./$(BIN).exe -fine -headless -n 100   -s 100 -e 200 -c 4 -o "$$OUTPUT" > /dev/null; \
	echo "[4/13] Fine | c=4 | n=1000"; \
	./$(BIN).exe -fine -headless -n 1000  -s 100 -e 200 -c 4 -o "$$OUTPUT" > /dev/null; \
	echo "[4/13] Fine | c=4 | n=10000"; \
	./$(BIN).exe -fine -headless -n 10000 -s 100 -e 200 -c 4 -o "$$OUTPUT" > /dev/null; \
	echo "[5/13] Fine | c=8 | n=100"; \
	./$(BIN).exe -fine -headless -n 100   -s 100 -e 200 -c 8 -o "$$OUTPUT" > /dev/null; \
	echo "[5/13] Fine | c=8 | n=1000"; \
	./$(BIN).exe -fine -headless -n 1000  -s 100 -e 200 -c 8 -o "$$OUTPUT" > /dev/null; \
	echo "[5/13] Fine | c=8 | n=10000"; \
	./$(BIN).exe -fine -headless -n 10000 -s 100 -e 200 -c 8 -o "$$OUTPUT" > /dev/null; \
	echo "[6/13] Fine | c=12 | n=100"; \
	./$(BIN).exe -fine -headless -n 100   -s 100 -e 200 -c 12 -o "$$OUTPUT" > /dev/null; \
	echo "[6/13] Fine | c=12 | n=1000"; \
	./$(BIN).exe -fine -headless -n 1000  -s 100 -e 200 -c 12 -o "$$OUTPUT" > /dev/null; \
	echo "[6/13] Fine | c=12 | n=10000"; \
	./$(BIN).exe -fine -headless -n 10000 -s 100 -e 200 -c 12 -o "$$OUTPUT" > /dev/null; \
	echo "[7/13] Coarse | c=4 | n=100"; \
	./$(BIN).exe -coarse -headless -n 100   -s 100 -e 200 -c 4 -o "$$OUTPUT" > /dev/null; \
	echo "[7/13] Coarse | c=4 | n=1000"; \
	./$(BIN).exe -coarse -headless -n 1000  -s 100 -e 200 -c 4 -o "$$OUTPUT" > /dev/null; \
	echo "[7/13] Coarse | c=4 | n=10000"; \
	./$(BIN).exe -coarse -headless -n 10000 -s 100 -e 200 -c 4 -o "$$OUTPUT" > /dev/null; \
	echo "[8/13] Coarse | c=8 | n=100"; \
	./$(BIN).exe -coarse -headless -n 100   -s 100 -e 200 -c 8 -o "$$OUTPUT" > /dev/null; \
	echo "[8/13] Coarse | c=8 | n=1000"; \
	./$(BIN).exe -coarse -headless -n 1000  -s 100 -e 200 -c 8 -o "$$OUTPUT" > /dev/null; \
	echo "[8/13] Coarse | c=8 | n=10000"; \
	./$(BIN).exe -coarse -headless -n 10000 -s 100 -e 200 -c 8 -o "$$OUTPUT" > /dev/null; \
	echo "[9/13] Coarse | c=12 | n=100"; \
	./$(BIN).exe -coarse -headless -n 100   -s 100 -e 200 -c 12 -o "$$OUTPUT" > /dev/null; \
	echo "[9/13] Coarse | c=12 | n=1000"; \
	./$(BIN).exe -coarse -headless -n 1000  -s 100 -e 200 -c 12 -o "$$OUTPUT" > /dev/null; \
	echo "[9/13] Coarse | c=12 | n=10000"; \
	./$(BIN).exe -coarse -headless -n 10000 -s 100 -e 200 -c 12 -o "$$OUTPUT" > /dev/null; \
	echo "[10/13] SMT | c=4 (8 hilos) | n=100"; \
	./$(BIN).exe -smt -headless -n 100   -s 100 -e 200 -c 4 -o "$$OUTPUT" > /dev/null; \
	echo "[10/13] SMT | c=4 (8 hilos) | n=1000"; \
	./$(BIN).exe -smt -headless -n 1000  -s 100 -e 200 -c 4 -o "$$OUTPUT" > /dev/null; \
	echo "[10/13] SMT | c=4 (8 hilos) | n=10000"; \
	./$(BIN).exe -smt -headless -n 10000 -s 100 -e 200 -c 4 -o "$$OUTPUT" > /dev/null; \
	echo "[11/13] SMT | c=8 (16 hilos) | n=100"; \
	./$(BIN).exe -smt -headless -n 100   -s 100 -e 200 -c 8 -o "$$OUTPUT" > /dev/null; \
	echo "[11/13] SMT | c=8 (16 hilos) | n=1000"; \
	./$(BIN).exe -smt -headless -n 1000  -s 100 -e 200 -c 8 -o "$$OUTPUT" > /dev/null; \
	echo "[11/13] SMT | c=8 (16 hilos) | n=10000"; \
	./$(BIN).exe -smt -headless -n 10000 -s 100 -e 200 -c 8 -o "$$OUTPUT" > /dev/null; \
	echo "[12/13] SMT | c=12 (24 hilos) | n=100"; \
	./$(BIN).exe -smt -headless -n 100   -s 100 -e 200 -c 12 -o "$$OUTPUT" > /dev/null; \
	echo "[12/13] SMT | c=12 (24 hilos) | n=1000"; \
	./$(BIN).exe -smt -headless -n 1000  -s 100 -e 200 -c 12 -o "$$OUTPUT" > /dev/null; \
	echo "[12/13] SMT | c=12 (24 hilos) | n=10000"; \
	./$(BIN).exe -smt -headless -n 10000 -s 100 -e 200 -c 12 -o "$$OUTPUT" > /dev/null; \
	echo "[13/13] CMP | c=4 | n=100"; \
	./$(BIN).exe -cmp -headless -n 100   -s 100 -e 200 -c 4 -o "$$OUTPUT" > /dev/null; \
	echo "[13/13] CMP | c=4 | n=1000"; \
	./$(BIN).exe -cmp -headless -n 1000  -s 100 -e 200 -c 4 -o "$$OUTPUT" > /dev/null; \
	echo "[13/13] CMP | c=4 | n=10000"; \
	./$(BIN).exe -cmp -headless -n 10000 -s 100 -e 200 -c 4 -o "$$OUTPUT" > /dev/null; \
	echo "[13/13] CMP | c=8 | n=100"; \
	./$(BIN).exe -cmp -headless -n 100   -s 100 -e 200 -c 8 -o "$$OUTPUT" > /dev/null; \
	echo "[13/13] CMP | c=8 | n=1000"; \
	./$(BIN).exe -cmp -headless -n 1000  -s 100 -e 200 -c 8 -o "$$OUTPUT" > /dev/null; \
	echo "[13/13] CMP | c=8 | n=10000"; \
	./$(BIN).exe -cmp -headless -n 10000 -s 100 -e 200 -c 8 -o "$$OUTPUT" > /dev/null; \
	echo "[13/13] CMP | c=12 | n=100"; \
	./$(BIN).exe -cmp -headless -n 100   -s 100 -e 200 -c 12 -o "$$OUTPUT" > /dev/null; \
	echo "[13/13] CMP | c=12 | n=1000"; \
	./$(BIN).exe -cmp -headless -n 1000  -s 100 -e 200 -c 12 -o "$$OUTPUT" > /dev/null; \
	echo "[13/13] CMP | c=12 | n=10000"; \
	./$(BIN).exe -cmp -headless -n 10000 -s 100 -e 200 -c 12 -o "$$OUTPUT" > /dev/null; \
	echo ""; \
	echo "Benchmark completo. CSV: $$OUTPUT"
