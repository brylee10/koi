# Assumes run from root of project

CXX = clang++
INCLUDE_PATHS = benchmarks/common benchmarks
INCLUDE_FLAGS = $(addprefix -I, $(INCLUDE_PATHS))
# -Wno-c99-extensions: Allows flexible array members in structs
CXXFLAGS = $(INCLUDE_FLAGS) -std=c++20 -Wno-c99-extensions -Werror -Wall -Wextra -pedantic -O3

# Directories for object files and binaries
OBJDIR = obj
BINDIR = bin

# Source files
COMMON_FIFO_SRC = benchmarks/named_pipe/named_pipe.cc
COMMON_SHM_SRC = benchmarks/shm/shm.cc
COMMON_SRC = benchmarks/common/utils.cc benchmarks/common/bench.cc benchmarks/common/signals.cc \
			 benchmarks/common/args.cc ${COMMON_FIFO_SRC} ${COMMON_SHM_SRC}
COMMON_OBJ = $(COMMON_SRC:benchmarks/%.cc=$(OBJDIR)/%.o)
COMMON_OBJ_DIRS = $(dir $(COMMON_OBJ))

PIPE_SRCS = benchmarks/pipe/pipe.cc
PROCESSOR_EFFECT_SRCS = benchmarks/processor_effects/cache_miss.cc \
	benchmarks/processor_effects/cache_line.cc benchmarks/processor_effects/l1_l2.cc \
	benchmarks/processor_effects/op_dep.cc
MESSAGE_QUEUE_SRCS = benchmarks/message_queue/client.cc benchmarks/message_queue/server.cc \
	benchmarks/message_queue/queue_ops.cc
NAMED_PIPE_SRCS = benchmarks/named_pipe/server.cc benchmarks/named_pipe/client.cc
SHM_SRCS = benchmarks/shm/server.cc benchmarks/shm/client.cc benchmarks/shm/launcher.cc

BENCH_SRCS = $(PIPE_SRCS) $(PROCESSOR_EFFECT_SRCS) $(MESSAGE_QUEUE_SRCS) $(NAMED_PIPE_SRCS) $(SHM_SRCS)
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
	$(CXX) $(CXXFLAGS) $< $(COMMON_OBJ) -o $@

# `-c` compile object files, do not link
$(OBJDIR)/%.o: benchmarks/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

benches: $(BENCH_EXECS)

dirs:
	@mkdir -p bin
	@mkdir -p ${BENCH_OBJ_DIRS} ${BENCH_BIN_DIRS} ${COMMON_OBJ_DIRS}

clean:
	@rm -rf $(OBJDIR) $(BINDIR)
