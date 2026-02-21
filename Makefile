# ─── TOOLCHAIN ────────────────────────────────────────────────────────────────
# These are the cross-compiler tools we installed via Homebrew.
# We prefix every tool with arm-none-eabi- so Make uses the ARM versions,
# not your Mac's native tools which produce code for your Mac's CPU.
CC      = arm-none-eabi-gcc       # C compiler (also drives the linker)
OBJCOPY = arm-none-eabi-objcopy   # converts between binary formats
SIZE    = arm-none-eabi-size      # prints how much Flash and RAM you use

# ─── TARGET ───────────────────────────────────────────────────────────────────
# The name of the output files. All generated files share this prefix.
TARGET = pico-baremetal

# ─── CPU FLAGS ────────────────────────────────────────────────────────────────
# These flags describe the exact hardware we are targeting.
# Without them, GCC would generate code the Pico cannot run.
#
# -mcpu=cortex-m0plus  tells GCC exactly which CPU core we have.
#                      GCC uses this to select the right instructions
#                      and optimisations for that specific core.
#
# -mthumb              Cortex-M only supports the Thumb instruction set.
#                      ARM has two instruction sets: ARM (32-bit instructions)
#                      and Thumb (16/32-bit mixed). Cortex-M0+ is Thumb-only.
#
# -mfloat-abi=soft     Cortex-M0+ has no hardware floating point unit (FPU).
#                      This tells GCC to emulate float operations in software.
#                      Using hard float on M0+ would generate FPU instructions
#                      the CPU cannot execute — instant HardFault.
CPU_FLAGS = -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft

# ─── COMPILER FLAGS ───────────────────────────────────────────────────────────
CFLAGS  = $(CPU_FLAGS)

# -ffreestanding  tells GCC this program runs without an OS.
#                 Disables assumptions about the standard library.
#                 Without this, GCC might insert calls to memset/memcpy
#                 from libc that don't exist in our bare metal environment.
CFLAGS += -ffreestanding

# -nostdlib       do not link the standard C library (libc) or startup files.
#                 We wrote our own startup. We don't want GCC adding its own.
CFLAGS += -nostdlib

# -Wall -Wextra   enable all warnings. In embedded, warnings are often real bugs.
#                 A missed volatile, a wrong pointer cast — these kill you silently.
CFLAGS += -Wall -Wextra

# -std=c11        use the C11 standard. Gives you stdint.h types (uint32_t etc)
#                 and modern C features while remaining close to the metal.
CFLAGS += -std=c11

# -g              embed debug symbols in the ELF file.
#                 Needed if you ever attach a debugger via SWD.
CFLAGS += -g

# -O2             optimisation level 2. GCC will produce tighter, faster code.
#                 In bare metal, unoptimised code can miss timing requirements.
#                 Use -O0 if you are debugging and instructions feel out of order.
CFLAGS += -O2

# ─── LINKER FLAGS ─────────────────────────────────────────────────────────────
LDFLAGS  = $(CPU_FLAGS)

# -T linker.ld    use our linker script to define the memory layout.
#                 Without this the linker has no idea where to put anything.
LDFLAGS += -T linker.ld

# -nostdlib       same as above — no standard startup files or libc.
LDFLAGS += -nostdlib

# --gc-sections   garbage collect unused sections.
#                 The linker removes any code or data that nothing references.
#                 Critical in embedded — every unused byte of Flash costs you.
LDFLAGS += -Wl,--gc-sections

# -Map            generate a map file showing exactly where every function
#                 and variable was placed in memory. Invaluable for debugging
#                 linker issues and verifying your memory layout is correct.
LDFLAGS += -Wl,-Map=$(TARGET).map

# ─── SOURCES ──────────────────────────────────────────────────────────────────
C_SOURCES   = src/startup.c \
              src/vectors.c \
              src/main.c

ASM_SOURCES = boot2/boot2.S

# ─── OBJECTS ──────────────────────────────────────────────────────────────────
# Transform source file paths into object file paths.
# src/main.c   → src/main.o
# boot2/boot2.S → boot2/boot2.o
# Make does this substitution automatically with these pattern rules.
C_OBJECTS   = $(C_SOURCES:.c=.o)
ASM_OBJECTS = $(ASM_SOURCES:.S=.o)
ALL_OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

# ─── BUILD RULES ──────────────────────────────────────────────────────────────
# Make reads these top to bottom. The first target is the default.
# $@  means "the target of this rule"  (left side of the colon)
# $<  means "the first dependency"     (first item on the right side)
# These are automatic variables — they save you from repeating names.

# Default target — building everything ends with a .uf2 file ready to flash
all: $(TARGET).uf2

# ── Compile C source files into object files
# Each .c file becomes a .o file independently.
# This is the separate compilation model — changes to one file
# only require recompiling that file, not the entire project.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ── Assemble .S files (boot2 is written in assembly)
%.o: %.S
	$(CC) $(CPU_FLAGS) -I boot2/include -c $< -o $@

# ── Link all object files into one ELF binary
# ELF (Executable and Linkable Format) is the standard binary format.
# It contains your machine code plus debug symbols and section metadata.
# The linker uses linker.ld to decide where each section goes in Flash/RAM.
$(TARGET).elf: $(ALL_OBJECTS) linker.ld
	$(CC) $(LDFLAGS) $(ALL_OBJECTS) -o $@
	$(SIZE) $@

# ── Convert ELF to raw binary
# The Pico does not understand ELF — it just wants raw bytes at raw addresses.
# objcopy strips all the ELF metadata and outputs just the machine code bytes.
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	@# Patch CRC32 into boot2 (bytes 0-251 → checksum at bytes 252-255).
	@# The ROM bootloader rejects boot2 if this doesn't match.
	python3 -c "\
	import struct, binascii; \
	f = open('$@', 'r+b'); \
	data = f.read(252); \
	crc = binascii.crc32(data) & 0xffffffff; \
	f.write(struct.pack('<I', crc)); \
	f.close(); \
	print('boot2 CRC32: 0x%08x' % crc)"

# ── Convert raw binary to UF2
# UF2 is the format the Pico's ROM USB bootloader understands.
# -b 0x10000000 tells uf2conv where in Flash to write the bytes —
#               which matches the ORIGIN of FLASH in our linker script.
# You need uf2conv.py in your project root (we will download it next).
$(TARGET).uf2: $(TARGET).bin
	python3 uf2conv.py -b 0x10000000 -f 0xe48bff56 -o $@ $<

# ── Clean all generated files
# Good practice — always have a clean target so you can do a fresh build.
clean:
	rm -f $(ALL_OBJECTS) $(TARGET).elf $(TARGET).bin $(TARGET).uf2 $(TARGET).map

.PHONY: all clean