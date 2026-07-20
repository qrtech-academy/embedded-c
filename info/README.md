# Course Information

## Instructor
Erik Pihl ([erik.axel.pihl@gmail.com](mailto:erik.axel.pihl@gmail.com))

---

# Course Plan – Embedded C

| Week | Lecture | Topic |
|------|---------|------|
| 1 | L01 | C fundamentals refresher |
| 3 | L02 | Registers and I/O |
| 5 | L03 | Interrupts |
| 7 | L04 | Analog-to-digital conversion (ADC) |
| 9 | L05 | Timers |
| 11 | L06 | Serial communication (UART) |
| 13 | L07 | EEPROM |
| 15 | L08 | The watchdog timer |
| 17 | L09 | Structs as drivers |
| 19 | L10 | Encapsulation and polymorphism in C |

---

## Lecture Content

### L01 – C Fundamentals Refresher
A review of the C language, before writing to hardware registers.

Topics include:
* Compiling and program shape
* Data types and naming
* Structs, enums, and typedefs
* Scope, lifetime, and storage
* Functions and `printf()`
* Pointers
* Function pointers
* Reading input and control flow
* Dynamic memory allocation (`malloc()`/`realloc()`/`free()`)
* Macros (`#define`)

---

### L02 – Registers and I/O
Introduction to embedded programming on the ATmega328P (Arduino Uno/Nano).

Topics include:
* What's different about embedded programming
* The board: Arduino Uno/Nano
* Bitwise operations
* Driving an output, reading an input
* First program: LED follows a button
* Blinking two LEDs with a hand-rolled delay

---

### L03 – Interrupts
Reacting to events instead of polling for them.

Topics include:
* Why interrupts
* External interrupts: INT0/INT1
* Pin Change Interrupts (PCI) on any port
* Choosing between INT0/INT1 and PCI

---

### L04 – Analog-to-Digital Conversion
Reading a continuous signal instead of a simple high/low.

Topics include:
* Why ADC
* Which pin, which registers
* A read function
* Example: potentiometer-controlled LED brightness (software PWM)

---

### L05 – Timers
Replacing blocking `_delay_ms()` calls with non-blocking, interrupt-driven timing.

Topics include:
* Why timers
* Prescaler → tick time → interrupt count
* Normal Mode vs. CTC Mode
* Non-blocking blink using Timer 0
* Timer 1 in CTC Mode (16-bit, arbitrary period)

---

### L06 – Serial Communication (UART)
Transmitting data to a PC instead of only driving LEDs.

Topics include:
* Why serial
* Configuring UART
* Sending and receiving data
* Mixing text and numbers
* Worked example: button toggles an LED, reports over serial

---

### L07 – EEPROM
Persisting state across resets, the chip's one non-volatile peripheral.

Topics include:
* EEPROM: what it is and why
* EEPROM registers
* `eeprom_write_byte()` / `eeprom_read_byte()`
* Worked example: persisting LED state across resets

---

### L08 – The Watchdog Timer
A hardware safety net against a program that hangs.

Topics include:
* The watchdog timer: what it is and why
* Watchdog registers
* `WDRF`: avoiding a disabling lockout
* System Reset Mode vs. Interrupt Mode
* Worked example: petting the watchdog

---

### L09 – Structs as Drivers
What changes once a program needs several instances of the same peripheral.

Topics include:
* Why bundle state
* The `self` convention: a GPIO driver
* Splitting a driver into `.h`/`.c`
* Worked example: an array of LEDs
* Worked example: a GPIO driver wired into a real multi-peripheral demo

---

### L10 – Encapsulation and Polymorphism in C
Hiding a struct's implementation details, and manual polymorphism via function pointers and
vtables.

Topics include:
* Why hide a struct's fields
* Hiding functions with `static`
* Opaque structs
* Function pointers in structs
* Worked example: an interrupt-driven driver with event flags
* Vtables: a hand-written interface

---

## Course Material

### Literature
The course material consists of:
* Lecture notes
* Code examples
* Exercises completed after the lectures

---

### Software
Recommended tools:
* **[Microchip Studio](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio)** – Primary IDE for writing, building, and flashing the ATmega328P examples
* **avr-gcc / avr-objcopy / avrdude** – The AVR toolchain, used directly from the command line on Linux
* **[Arduino IDE](https://www.arduino.cc/en/software)** – Used briefly to bootstrap the Microchip Studio/Linux workflow
* An **Arduino Uno or Nano** board – The target hardware for every worked example

Instructions for installing the development environment are available for
[Windows/Microchip Studio](../lectures/L02/appendix/c_arduino_via_microchip_studio.md) and
[Linux](../lectures/L02/appendix/d_building_and_flashing_on_linux.md).

---
