# Appendix D - Building and Flashing on Linux

## D.1 Why this appendix
[Appendix C](./c_arduino_via_microchip_studio.md) covers the Windows/Microchip Studio path. This
is the native Linux equivalent: the same `avr-gcc`/`avrdude` toolchain Microchip Studio and the
Arduino IDE both wrap under the hood, used directly from the command line. No IDE required.

This was written and tested from WSL2 (Ubuntu on Windows); the compiler/`avrdude` steps are
identical on any Debian/Ubuntu-based Linux, but D.5's USB step is WSL2-specific; skip it on a
native Linux install, where the board just shows up as a device node once plugged in.

---

## D.2 Install the toolchain
Three packages: the AVR cross-compiler, its C standard library, and the flashing tool.

```
sudo apt -y update
sudo apt -y install gcc-avr avr-libc avrdude
```

Verify:

```
avr-gcc --version
avrdude -v
```

---

## D.3 Write the program
Same first program as
[Appendix A, A.5](./a_registers_and_io.md#a5-first-program-led-follows-a-button): LED on D9,
button on D13, LED lit while the button is held.

```c
#include <avr/io.h>

/** GPIO pins. */
#define LED1 1U // D9  -> PORTB1.
#define BTN1 5U // D13 -> PORTB5.

/** GPIO operations. */
#define LED1_ON PORTB |= (1U << LED1)         // Enable LED1.
#define LED1_OFF PORTB &= ~(1U << LED1)       // Disable LED1.
#define BTN1_IS_PRESSED (PINB & (1U << BTN1)) // High if BTN1 is pressed, low otherwise.

/**
 * @brief Set up system.
 */
static void setup(void)
{
    // Configure LED1 as output.
    DDRB = (1U << LED1);

    // Configure BTN1 as input with its pull-up enabled.
    PORTB = (1U << BTN1);
}

/**
 * @brief Application entry point.
 *
 * @return 0 on termination of the program (should never occur).
 */
int main(void)
{
    setup();

    while (1)
    {
        // Read BTN1, enable LED1 if pressed, disable if not.
        if (BTN1_IS_PRESSED) { LED1_ON; }
        else { LED1_OFF; }
    }
    return 0;
}
```

Save it as `main.c`.

---

## D.4 Compile
Two steps: compile and link to an ELF, then extract the flash-able Intel HEX from it.

```
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -std=c11 -Wall -Wextra -o main.elf main.c
avr-objcopy -O ihex -R .eeprom main.elf main.hex
```

* `-mmcu=atmega328p` targets the right instruction set and register definitions.
* `-DF_CPU=16000000UL` matters the moment any code uses `<util/delay.h>`; harmless to always
  include.
* `-Os` (optimize for size) is the AVR default of choice; flash is measured in kilobytes.
* `avr-objcopy` strips the ELF down to the raw `.hex` `avrdude` actually writes to flash.

`avr-size main.elf` shows how much flash/RAM the build uses, worth checking once code grows past
a toy example.

---

## D.5 WSL2 only: attach the board's USB device
WSL2 doesn't share Windows' USB devices by default. `usbipd-win` bridges the two; this is a
one-time install, then a per-session attach.

**On Windows** (PowerShell, once): install the bridge and confirm winusb driver support:

```
winget install usbipd
```

**On Windows** (PowerShell as Administrator, every time the board is (re)plugged in):

```
usbipd list                              # Find the board's BUSID.
usbipd bind --busid <BUSID>              # One-time per device.
usbipd attach --wsl --busid <BUSID>      # Do this each time you reconnect.
```

**Back in WSL2**, confirm it showed up:

```
lsusb
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

Arduino Uno-style boards (ATmega16U2 as USB-serial bridge) usually show up as `/dev/ttyACM0`;
boards using a CH340 or FTDI USB-serial chip (many Nano clones) show up as `/dev/ttyUSB0` instead.

---

## D.6 Permissions
Without this, `avrdude` fails with a permissions error on the serial device. Add your user to the
`dialout` group (one-time), then start a new shell session for it to take effect:

```
sudo usermod -aG dialout $USER
```

Log out and back in (or just open a new WSL terminal) for the group change to apply.

---

## D.7 Flash
Same idea as [Appendix C](./c_arduino_via_microchip_studio.md): the board runs the Arduino
bootloader, which `avrdude`'s `arduino` programmer type talks to over serial.

```
avrdude -c arduino -p atmega328p -P /dev/ttyACM0 -b 115200 -U flash:w:main.hex:i
```

* `-c arduino` selects the STK500v1-over-serial protocol the Arduino bootloader speaks (this is
  the same programmer type Appendix C's `avrdude` command uses via `-carduino`).
* `-p atmega328p` must match D.4's `-mmcu`.
* `-P` is whatever device D.5 (or, on native Linux, just plugging the board in) produced.
* `-b 115200` is the Optiboot-era default. Some older Nano boards ship a bootloader that only
  works at `-b 57600`; if 115200 fails with sync errors, that's the first thing to try instead.

A successful run ends with something like:

```
avrdude: 194 bytes of flash written
avrdude: safemode: Fuses OK
avrdude done. Thank you.
```

The LED on D9 should now light up while the button on D13 is held, exactly like Appendix C's
result, just reached without Windows or an IDE.

---

## D.8 Optional: a Makefile
Ties D.4 and D.7 together to replace the manual commands:
* Call this file `Makefile`.
* Run `make` to compile the code.
* Run `make flash` to flash the firmware.

Adjust `PORT` for your setup: 

```makefile
MCU   := atmega328p
F_CPU := 16000000UL
PORT  := /dev/ttyACM0
BAUD  := 115200

all: main.hex

main.elf: main.c
	avr-gcc -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -std=c11 -Wall -Wextra -o $@ $<

main.hex: main.elf
	avr-objcopy -O ihex -R .eeprom $< $@

flash: main.hex
	avrdude -c arduino -p $(MCU) -P $(PORT) -b $(BAUD) -U flash:w:$<:i

clean:
	rm -f main.elf main.hex

.PHONY: all flash clean
```

---
