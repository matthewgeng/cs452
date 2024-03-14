XDIR:=/u/cs452/public/xdev
ARCH=cortex-a72
TRIPLE=aarch64-none-elf
XBINDIR:=$(XDIR)/bin
CC:=$(XBINDIR)/$(TRIPLE)-gcc
OBJCOPY:=$(XBINDIR)/$(TRIPLE)-objcopy
OBJDUMP:=$(XBINDIR)/$(TRIPLE)-objdump

# COMPILE OPTIONS
WARNINGS=-Wall -Wextra -Wpedantic -Wno-unused-const-variable
CFLAGS:=-g -pipe -static $(WARNINGS) -ffreestanding -nostartfiles\
	-mcpu=$(ARCH) -static-pie -mstrict-align -fno-builtin -mgeneral-regs-only -O3\
	-Iinclude/ #-Xlinker # -Map=output.map #(for memory mapping)

# Source files and include dirs
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

# LINKER:= $(call rwildcard,src/,*.ld)
LINKER:= src/kernel/boot/linker.ld

# -Wl,option tells g++ to pass 'option' to the linker with commas replaced by spaces
# doing this rather than calling the linker ourselves simplifies the compilation procedure
LDFLAGS:=-Wl,-nmagic -Wl,-T$(LINKER) -Wl,--no-warn-rwx-segments

# SOURCES := $(wildcard */*.c) $(wildcard */*.S)
# SOURCES := $(wildcard src/*.c src/server/*.c) $(wildcard src/*.S src/asm/*.S)
SOURCES := $(call rwildcard,src/,*.c) $(call rwildcard,src/,*.S) 
# Create .o and .d files for every .cc and .S (hand-written assembly) file
OBJECTS := $(patsubst %.c, %.o, $(patsubst %.S, %.o, $(SOURCES)))
DEPENDS := $(patsubst %.c, %.d, $(patsubst %.S, %.d, $(SOURCES)))

# The first rule is the default, ie. "make", "make all" and "make iotest.img" mean the same
all: iotest.img

clean:
	rm -f $(OBJECTS) $(DEPENDS) iotest.elf iotest.img

iotest.img: iotest.elf
	$(OBJCOPY) $< -O binary $@

iotest.elf: $(OBJECTS) $(LINKER)
	$(CC) $(CFLAGS) $(filter-out %.ld, $^) -o $@ $(LDFLAGS)
	@$(OBJDUMP) -d iotest.elf | grep -Fq q0 && printf "\n***** WARNING: SIMD INSTRUCTIONS DETECTED! *****\n\n" || true

%.o: %.c Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

%.o: %.S Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)
