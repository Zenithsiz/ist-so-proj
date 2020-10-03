# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2020-21

# Compiler and linker
CC   = gcc
LD   = gcc

# Compiler flags
# Note: `-posix` is required to have conforming C99 `printf` and others on windows (reference: MinGW's `_mingw.h:407`).
CFLAGS =-g\
	-Wall -Wextra -Werror -pedantic\
	-Wnull-dereference -Wformat=2 -Wstrict-prototypes\
	-Wcast-qual -Wmissing-prototypes -Wformat-security\
	-Winit-self -Wimplicit-fallthrough -Wswitch-default\
	-Wswitch-enum -Wunused-parameter -Wduplicated-cond\
	-Wduplicated-branches -Wlogical-op -Wunsafe-loop-optimizations\
	-posix -std=gnu99 -Isrc/\
	-DDEBUG

# Linker flags
LDFLAGS=-lm

# Library sources
LIB_SRCS := $(shell find 'src/tfs/' -name '*.c')

# Library object files
LIB_OBJS := $(patsubst src/%.c,obj/%.o,$(LIB_SRCS))

# Library dependencies
LIB_DEPS := $(patsubst src/%.c,obj/%.d,$(LIB_SRCS))

# Test sources
TEST_SRCS := $(shell find 'src/tests' -name '*.c')

# Test objects
TEST_OBJS := $(patsubst src/%.c,obj/%.o,$(TEST_SRCS))

# Test dependencies
TEST_DEPS := $(patsubst src/%.c,obj/%.d,$(TEST_SRCS))

# Test binaries
TEST_BINS := $(patsubst src/%.c,build/%,$(TEST_SRCS))

# Program sources
PROG_SRCS := $(shell find 'src/bin' -name '*.c')

# Program objects
PROG_OBJS := $(patsubst src/%.c,obj/%.o,$(PROG_SRCS))

# Program dependencies
PROG_DEPS := $(patsubst src/%.c,obj/%.d,$(PROG_SRCS))

# Program binaries
PROG_BINS := $(patsubst src/%.c,build/%,$(PROG_SRCS))

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run


all: $(PROG_BINS)

# Binaries
$(TEST_BINS) $(PROG_BINS): build/%: obj/%.o $(LIB_OBJS)
	@echo $@: Building binary
	@mkdir -p $(dir $@)
	@$(LD) $(CFLAGS) $(LDFLAGS) $^ -o '$@'
	@[ ! -f '$@.exe' ] || mv -f '$@.exe' '$@'

# Build all `obj/` files from `src/`
$(LIB_OBJS) $(PROG_OBJS) $(TEST_OBJS): obj/%.o: src/%.c
	@echo $<: Building
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -o '$@' -c '$<'

# Automatic prerequisites generation.
# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html
$(LIB_DEPS) $(PROG_DEPS) $(TEST_DEPS): obj/%.d: src/%.c
	@echo $<: Generating dependencies
	@mkdir -p $(dir $@)
	@$(CC) -M $(CFLAGS) $< | sed -e 's|$(patsubst %.d,%.o,$(notdir $@))|$(patsubst %.d,%.o,$@)|' > '$@'

# Include all `.d` dependencies
include $(LIB_DEPS) $(PROG_DEPS) $(TEST_DEPS)

# Remove build artifacts
clean:
	@echo Cleaning...
	@rm -rf obj/
	@rm -rf build/

# Run all tests
test: $(TEST_BINS)
	@for test in build/tests/*; do ./$$test; done
