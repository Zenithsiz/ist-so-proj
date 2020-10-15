# Compiler and linker
CC   = gcc
LD   = gcc

# Compiler flags
# Note: `-posix` is required to have a conforming C99 stdlib on windows MINGW (from `_mingw.h:407`)
# Note: We need `_GNU_SOURCE` for `pthread_rwlock_*`
CFLAGS =\
	-posix -pthread -std=gnu99 -g\
	-Isrc/\
	-Wall -Wextra -Werror -pedantic\
	-Wformat=2 -Winit-self -Wmissing-include-dirs -Wswitch-default\
	-Wswitch-enum -Wunused-parameter\
	-Wstrict-overflow=5 -Wfloat-equal -Wundef -Wshadow -Wunsafe-loop-optimizations\
	-Wbad-function-cast -Wcast-qual -Wcast-align=strict -Wwrite-strings -Wconversion\
	-Wsign-conversion -Wlogical-op -Wstrict-prototypes -Wold-style-definition\
	-Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn\
	-Wmissing-format-attribute -Woverride-init -Wredundant-decls\
	-Wunreachable-code -Winline -Wdouble-promotion -Wnonnull\
	-Wnonnull-compare -Wimplicit-fallthrough -Wuninitialized\
	-Wstringop-overflow=4 -Wstringop-truncation -Walloc-zero\
	-Warray-bounds=2 -Wduplicated-branches -Wduplicated-cond\
	-Wc99-c11-compat -Wformat-security -Wnull-dereference\
	-D_GNU_SOURCE

# Linker flags
LDFLAGS=-lm

# Library sources, object files and dependencies
LIB_SRCS := $(shell find 'src/tfs/' -name '*.c')
LIB_OBJS := $(patsubst src/%.c,obj/%.o,$(LIB_SRCS))
LIB_DEPS := $(patsubst src/%.c,obj/%.d,$(LIB_SRCS))

# Test sources, object files, binaries and dependencies
TEST_SRCS := $(shell find 'src/tests' -name '*.c')
TEST_OBJS := $(patsubst src/%.c,obj/%.o,$(TEST_SRCS))
TEST_BINS := $(patsubst src/%.c,build/%,$(TEST_SRCS))
TEST_DEPS := $(patsubst src/%.c,obj/%.d,$(TEST_SRCS))

# Program sources, objects, binaries and dependencies
PROG_SRCS := $(shell find 'src/bin' -name '*.c')
PROG_OBJS := $(patsubst src/%.c,obj/%.o,$(PROG_SRCS))
PROG_BINS := $(patsubst src/%.c,build/%,$(PROG_SRCS))
PROG_DEPS := $(patsubst src/%.c,obj/%.d,$(PROG_SRCS))

# Phony targets
.PHONY: all clean test

# Build all program binaries by default.
all: $(PROG_BINS)

# Binaries
$(TEST_BINS) $(PROG_BINS): build/%: obj/%.o $(LIB_OBJS)
	@echo $@: Building binary
	@mkdir -p $(dir $@)
	@$(LD) $(CFLAGS) $(LDFLAGS) -o '$@' $^

# On windows, rename the `bin.exe` to `bin`, so makefile
# doesn't rebuild it, since `bin` doesn't exist.
# Note: The double negative, `!` + `||` is so this returns
#       success even when the file doesn't exist.
	@[ ! -f '$@.exe' ] || mv -f '$@.exe' '$@'

# Object files
$(LIB_OBJS) $(PROG_OBJS) $(TEST_OBJS): obj/%.o: src/%.c
	@echo $<: Building
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -o '$@' -c '$<'

# Automatic prerequisites generation.
# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html
$(LIB_DEPS) $(PROG_DEPS) $(TEST_DEPS): obj/%.d: src/%.c
	@echo $<: Generating dependencies
	@mkdir -p $(dir $@)
# Note: The `sed` makes sure the rule that's built is relative to the build directory,
#       as otherwise we'd simply have `filename.o: ...` instead of `tfs/obj/.../filename.o: ...`
	@$(CC) -M $(CFLAGS) '$<' | sed -e 's|$(patsubst %.d,%.o,$(notdir $@))|$(patsubst %.d,%.o,$@)|' > '$@'

# Include all `.d` dependencies
include $(LIB_DEPS) $(PROG_DEPS) $(TEST_DEPS)

# Remove build artifacts
clean:
	@echo Cleaning...
	@rm -rf obj/
	@rm -rf build/

# Run all tests
test: $(TEST_BINS)
	@$(foreach test,$(TEST_BINS),./$(test) &&) true
