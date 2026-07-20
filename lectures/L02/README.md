# L02 - Hardware-Near Programming in C

## Agenda
* Introduction: what's different about programming a microcontroller versus a normal computer.
* The board: Arduino Uno/Nano / ATmega328P, I/O ports and PIN↔PORT mapping.
* Bitwise operations: OR, AND, NOT, shifting, how register-level I/O actually works.
* Driving an output pin and reading an input pin (`DDRx`, `PORTx`, `PINx`).
* Two worked examples: LED follows a button; two LEDs blink while a button is held.

---

## Goals of the lecture
* Understand why embedded systems are programmed close to the hardware, in C, without an OS
  underneath.
* Be able to read and write bitwise operations (`|`, `&`, `~`, `<<`, `>>`) to set, clear, and
  read individual bits in a register.
* Be able to configure a pin as input or output and drive/read it correctly.

---

## Instructions

### Before the lecture
* Read [Appendix A](./appendix/a_registers_and_io.md) for the lecture's core material.

### During the lecture
* Participate actively in the walkthrough.

### After the lecture
* Work through the exercises in [Appendix B](./appendix/b_exercises.md).

---

## Evaluation
* What's the difference between `DDRx`, `PORTx`, and `PINx`, and when do you write to one versus
  read from another?
* How do you set bit 3 of a register without disturbing the other bits? How do you clear it?
* Why does an input pin need its pull-up resistor enabled, and what would you expect to happen
  if you left it disabled with nothing else connected to the pin?

---

## Reference
* [Appendix C](./appendix/c_arduino_via_microchip_studio.md) walks through flashing a Microchip
  Studio build onto the board via the Arduino bootloader and avrdude, useful if you don't have a
  dedicated ISP programmer.
* [Appendix D](./appendix/d_building_and_flashing_on_linux.md) is the same idea from native
  Linux: installing `avr-gcc`/`avrdude`, compiling from the command line, and flashing over
  serial, including WSL2's USB-passthrough step.

---

## Next lecture
* Interrupts: reacting to events instead of polling for them.
* External interrupts (INT0/INT1) and Pin Change Interrupts.

---
