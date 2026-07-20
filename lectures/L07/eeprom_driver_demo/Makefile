# Target MCU.
MCU := atmega328p

# CPU frequency in Hz.
F_CPU := 16000000UL

# Serial port for flashing.
PORT := /dev/ttyACM0

# Baud rate for flashing (in bps).
BAUD := 115200

# C compiler.
C_COMPILER := avr-gcc

# C compiler flags.
C_FLAGS := -Wall -Wextra -Werror -Iinclude -std=c11

# Source files.
SRC_FILES := main.c \
             source/driver/eeprom.c \
             source/driver/serial.c \

# Build the firmware image by default.
all: main.hex

# Compile and link the source files into an ELF binary.
main.elf: $(SRC_FILES)
	@$(C_COMPILER) -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -o $@ $^ $(C_FLAGS)

# Convert the ELF binary into an Intel HEX image, ready to flash.
main.hex: main.elf
	@avr-objcopy -O ihex -R .eeprom $< $@

# Flash the firmware image onto the board.
flash: main.hex
	@avrdude -c arduino -p $(MCU) -P $(PORT) -b $(BAUD) -U flash:w:$<:i

# Remove build artifacts.
clean:
	@rm -f main.elf main.hex

# Targets that don't produce a file of the same name.
.PHONY: all flash clean
