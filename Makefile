# Assumes run from root of project

CXX = clang++
INCLUDE_PATHS = benchmarks
CXXFLAGS = -I$(INCLUDE_PATHS) -std=c++17 -Werror -Wall -Wextra -pedantic -O3

# Directories for object files and binaries
OBJDIR = obj
BINDIR = bin

# Source files
COMMON_SRC = benchmarks/common/utils.cc benchmarks/common/bench.cc benchmarks/common/signals.cc
COMMON_OBJ = $(COMMON_SRC:benchmarks/common/%.cc=$(OBJDIR)/common/%.o)

BENCH_SRC = benchmarks/pipe/pipe.cc
BENCH_OBJ = $(OBJDIR)/pipe/pipe.o

# Default target
all: utils benches

# Compile common utilities
utils: dirs $(COMMON_OBJ)

benches: dirs utils $(BENCH_OBJ)
	$(CXX) $(CXXFLAGS) $(BENCH_OBJ) $(COMMON_OBJ) -o $(BINDIR)/pipe

# `-c` compile object files, do not link
$(OBJDIR)/%.o: benchmarks/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

dirs:
	mkdir -p bin
	mkdir -p obj/common obj/pipe

clean:
	rm -rf $(OBJDIR) $(BINDIR)
