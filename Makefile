# Assumes run from root of project

CXX = clang++
INCLUDE_PATHS = benchmarks
CXXFLAGS = -I$(INCLUDE_PATHS) -std=c++20 -Werror -Wall -Wextra -pedantic -O3

# Directories for object files and binaries
OBJDIR = obj
BINDIR = bin

# Source files
COMMON_SRC = benchmarks/common/utils.cc benchmarks/common/bench.cc benchmarks/common/signals.cc
COMMON_OBJ = $(COMMON_SRC:benchmarks/common/%.cc=$(OBJDIR)/common/%.o)
COMMON_OBJ_DIRS = $(dir $(COMMON_OBJ))

BENCH_SRCS = benchmarks/pipe/pipe.cc benchmarks/processor_effects/cache_miss.cc \
	benchmarks/processor_effects/cache_line.cc benchmarks/processor_effects/l1_l2.cc \
	benchmarks/processor_effects/op_dep.cc
# e.g. benchmarks/pipe/pipe.cc -> obj/pipe/pipe.o
BENCH_OBJS = $(BENCH_SRCS:benchmarks/%.cc=$(OBJDIR)/%.o)
# e.g. obj/pipe/pipe.cc -> obj/pipe/
BENCH_OBJ_DIRS = $(dir $(BENCH_OBJS))
BENCH_EXECS = $(BENCH_SRCS:benchmarks/%.cc=$(BINDIR)/%)
BENCH_BIN_DIRS = $(dir $(BENCH_EXECS))

# Default target
all: utils benches

# Compile common utilities
utils: dirs $(COMMON_OBJ)

# Pattern rule for linking object files to executables
$(BINDIR)/%: $(OBJDIR)/%.o dirs utils 
	@$(CXX) $(CXXFLAGS) $< $(COMMON_OBJ) -o $@

# `-c` compile object files, do not link
$(OBJDIR)/%.o: benchmarks/%.cc
	@$(CXX) $(CXXFLAGS) -c $< -o $@

benches: $(BENCH_EXECS)

dirs:
	@mkdir -p bin
	@mkdir -p ${BENCH_OBJ_DIRS} ${BENCH_BIN_DIRS} ${COMMON_OBJ_DIRS}

clean:
	@rm -rf $(OBJDIR) $(BINDIR)
