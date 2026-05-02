##############################################################
# Makefile — STM32F407VG-Discovery
# Toolchain : arm-none-eabi-gcc + OpenOCD + GDB
# Author    : z_benakka193
# Projet    : STM32_RFID_RC522_Reader
##############################################################

##############################################################
# TOOLCHAIN
##############################################################
CC      = arm-none-eabi-gcc
AS      = arm-none-eabi-gcc -x assembler-with-cpp
LD      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size
GDB     = gdb-multiarch
RM      = rm -rf
MKDIR   = mkdir -p

##############################################################
# PROJECT
##############################################################
TARGET = stm32-rfid-rc522
BUILD ?= release

##############################################################
# DIRECTORIES
##############################################################
SRC_DIR  = src
INC_DIR  = include
LNK_DIR  = linker
OBJ_DIR  = build/obj
BIN_DIR  = build/bin
OPENCM3_DIR = ../Drivers/libopencm3

##############################################################
# SOURCE FILES
##############################################################
C_SRCS   = $(wildcard $(SRC_DIR)/*.c)
ASM_SRCS = $(wildcard startup/*.s)

##############################################################
# OBJECT FILES
##############################################################
C_OBJS   = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(C_SRCS))
ASM_OBJS = $(addprefix $(OBJ_DIR)/,$(notdir $(ASM_SRCS:.s=.o)))
OBJS     = $(C_OBJS) $(ASM_OBJS)
DEPS     = $(OBJS:.o=.d)

##############################################################
# CPU / ARCH FLAGS
##############################################################
CPU   = -mcpu=cortex-m4
FPU   = -mfpu=fpv4-sp-d16
FLOAT = -mfloat-abi=hard
ARCH  = $(CPU) $(FPU) $(FLOAT) -mthumb

##############################################################
# COMPILER FLAGS
##############################################################
CFLAGS  = $(ARCH)
CFLAGS += -Wall -Wextra
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -MMD -MP
CFLAGS += -I$(INC_DIR)
CFLAGS += -I$(OPENCM3_DIR)/include
CFLAGS += -DSTM32F4
CFLAGS += -DSTM32F407xx

ifeq ($(BUILD), debug)
    CFLAGS += -g3 -O0 -DDEBUG
    $(info [INFO] Build mode : DEBUG)
else
    CFLAGS += -O2
    $(info [INFO] Build mode : RELEASE)
endif

##############################################################
# LINKER FLAGS
##############################################################
LDFLAGS  = $(ARCH)
LDFLAGS += -T$(LNK_DIR)/STM32F407VGTx.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(BIN_DIR)/$(TARGET).map
LDFLAGS += -L$(OPENCM3_DIR)/lib
LDFLAGS += -lopencm3_stm32f4
LDFLAGS += -nostdlib
LDFLAGS += --specs=nosys.specs
LDFLAGS += --specs=nano.specs

##############################################################
# MAIN RULES
##############################################################
.PHONY: all clean flash size disasm debug-server debug stats format format-check cppcheck help

all: $(BIN_DIR)/$(TARGET).hex
	@$(SIZE) $(BIN_DIR)/$(TARGET).elf

$(BIN_DIR)/$(TARGET).hex: $(BIN_DIR)/$(TARGET).elf
	@echo "[HEX]  $@"
	@$(OBJCOPY) -O ihex $< $@

$(BIN_DIR)/$(TARGET).elf: $(OBJS) | $(BIN_DIR)
	@echo "[LD]   $@"
	@$(LD) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "[CC]   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: startup/%.s | $(OBJ_DIR)
	@echo "[AS]   $<"
	@$(AS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	@$(MKDIR) $@

$(BIN_DIR):
	@$(MKDIR) $@

-include $(DEPS)

##############################################################
# UTILITY TARGETS
##############################################################

clean:
	@echo "[CLEAN] Removing build directory..."
	@$(RM) build/

flash: $(BIN_DIR)/$(TARGET).hex
	@echo "[FLASH] Flashing $(TARGET).hex via OpenOCD..."
	openocd -f openocd.cfg \
	        -c "program $(BIN_DIR)/$(TARGET).hex verify reset exit"

size: $(BIN_DIR)/$(TARGET).elf
	@echo "========================================"
	@echo "  Memory usage — Global"
	@echo "========================================"
	@$(SIZE) $(BIN_DIR)/$(TARGET).elf
	@echo ""
	@echo "========================================"
	@echo "  Memory usage — Par fichier objet"
	@echo "========================================"
	@$(SIZE) $(OBJ_DIR)/*.o

disasm: $(BIN_DIR)/$(TARGET).elf
	@echo "[DISASM] Generating $(BIN_DIR)/$(TARGET).disasm ..."
	@$(OBJDUMP) -d -S $(BIN_DIR)/$(TARGET).elf > $(BIN_DIR)/$(TARGET).disasm
	@echo "[DISASM] Done -> $(BIN_DIR)/$(TARGET).disasm"

debug-server:
	@echo "[DEBUG] Starting OpenOCD GDB server on port 3333..."
	openocd -f openocd.cfg

debug: $(BIN_DIR)/$(TARGET).elf
	@echo "[GDB] Connecting to OpenOCD on port 3333..."
	$(GDB) -ex "target remote :3333" \
	       -ex "monitor reset halt" \
	       -ex "load" \
	       -ex "break main" \
	       -ex "continue" \
	       $(BIN_DIR)/$(TARGET).elf

stats:
	@echo "========================================"
	@echo "  Project Statistics"
	@echo "========================================"
	@find $(SRC_DIR) -name "*.c" | wc -l | xargs echo "    .c files  :"
	@find $(INC_DIR) -name "*.h" | wc -l | xargs echo "    .h files  :"
	@find startup   -name "*.s" | wc -l | xargs echo "    .s files  :"

format:
	@echo "[FORMAT] Formatting source files..."
	@clang-format -i src/*.c include/*.h
	@echo "[FORMAT] Done"

format-check:
	@echo "[FORMAT] Checking source files..."
	@clang-format --dry-run --Werror src/*.c include/*.h && \
	    echo "[FORMAT] All files are correctly formatted" || \
	    echo "[FORMAT] Some files need formatting — run make format"

cppcheck:
	@echo "[CPPCHECK] Analysing source files..."
	@cppcheck --enable=all --inconclusive \
	          --suppress=missingIncludeSystem \
	          --suppress=unusedFunction \
	          -I $(INC_DIR) \
	          $(SRC_DIR)/ 2>&1 | tee build/cppcheck.log
	@echo "[CPPCHECK] Done -> build/cppcheck.log"

help:
	@echo "========================================"
	@echo "  STM32_RFID_RC522_Reader - Makefile help"
	@echo "========================================"
	@echo "  Build :"
	@echo "    make              -> build release"
	@echo "    make BUILD=debug  -> build debug"
	@echo "    make clean        -> clean build/"
	@echo ""
	@echo "  Flash :"
	@echo "    make flash        -> flash via OpenOCD"
	@echo ""
	@echo "  Analyse :"
	@echo "    make size         -> memory usage"
	@echo "    make disasm       -> desassembler le binaire"
	@echo "    make stats        -> statistiques du projet"
	@echo ""
	@echo "  Debug :"
	@echo "    make debug-server -> lancer OpenOCD GDB server"
	@echo "    make debug        -> lancer GDB client"
	@echo "    make cppcheck     -> analyse statique"
	@echo "    make format       -> formater le code source"