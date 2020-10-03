# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2020-21

CC   = gcc
LD   = gcc
CFLAGS =-g\
	-Wall\
	-Wextra\
	-Werror\
	-pedantic\
	-Wnull-dereference\
	-Wformat=2\
	-Wstrict-prototypes\
	-Wcast-qual\
	-Wmissing-prototypes\
	-Wformat-security\
	-Winit-self\
	-Wimplicit-fallthrough\
	-Wswitch-default\
	-Wswitch-enum\
	-Wunused-parameter\
	-Wduplicated-cond\
	-Wduplicated-branches\
	-Wlogical-op\
	-Wunsafe-loop-optimizations\
	-std=gnu99\
	-Isrc/\
	-DDEBUG
LDFLAGS=-lm

# All source files from `src/`
SRCS := $(shell find 'src/' -name '*.c')

# All object files in `obj/`
OBJS := $(patsubst src/%.c,obj/%.o,$(SRCS))

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: build/tecnicofs

# Final binary
build/tecnicofs: $(OBJS)
	@echo $@: Building binary
	@mkdir -p $(dir $@)
	@$(LD) $(CFLAGS) $(LDFLAGS) $^ -o '$@'
	@[ ! -f '$@.exe' ] || mv -f '$@.exe' '$@'

# Build all `obj/` files from `src/`
$(OBJS): obj/%.o: src/%.c
	@echo $<: Building
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -o '$@' -c '$<'

# Automatic prerequisites generation.
# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html
obj/%.d: src/%.c
	@echo $<: Generating dependencies
	@mkdir -p $(dir $@)
	@$(CC) -M $(CFLAGS) $< | sed -e 's|$(patsubst %.d,%.o,$(notdir $@))|$(patsubst %.d,%.o,$@)|' > '$@'

# Include all `.d` dependencies
include $(patsubst src/%.c,obj/%.d,$(SRCS))

# Remove build artifacts
clean:
	@echo Cleaning...
	@rm -rf obj/
	@rm -rf build/

# Run binary
run: build/tecnicofs
	@./build/tecnicofs
