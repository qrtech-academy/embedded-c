# Embedded C
Repository for the course **Embedded C**.

This course consists of ten lectures and is intended for junior embedded developers, as well as
regular developers with some programming experience who want to learn embedded systems
programming, in C, from language fundamentals through driver design, on real ATmega328P hardware
(Arduino Uno/Nano).

---

## About the Course
The course covers embedded development in C targeting the ATmega328P, building from C
fundamentals up through interrupt-driven peripheral drivers to hand-written encapsulation and
polymorphism.

Topics include:
* A C fundamentals refresher: structs, pointers, function pointers, dynamic memory, macros.
* Register-level I/O: bitwise operations, `DDRx`/`PORTx`/`PINx`.
* Interrupts: external interrupts and pin change interrupts.
* Analog-to-digital conversion (ADC).
* Timers: prescalers, Normal and CTC modes, non-blocking timing.
* Serial communication (UART).
* EEPROM: persisting state across resets.
* The watchdog timer.
* Structs as drivers: bundling state for peripherals with multiple instances.
* Encapsulation and polymorphism in C: opaque structs, function pointers in structs, and
  hand-written vtables.

Unlike a general C course, every lecture here builds toward one real target, the ATmega328P, and
every worked example is a real program, compiled and flashed to actual hardware rather than
simulated. The course ends where [Modern Embedded C++](https://github.com/qrtech-academy/modern-embedded-cpp), a separate follow-on course, begins.

---

## During the Course
During the lectures participants will:
* Wire and program real circuits (LEDs, buttons, potentiometers) on an Arduino Uno/Nano.
* Write drivers directly against the ATmega328P's registers and datasheet.
* Move from polling to interrupt-driven, non-blocking designs.
* Bundle a peripheral's state into a struct to support multiple instances.
* Refactor a driver from a plain struct into an opaque type, then into a hand-written interface
  with multiple implementations.

Examples and exercises focus on typical embedded problems, such as:
* Digital I/O and debounced button input.
* Timer-driven, non-blocking LED patterns.
* Serial reporting of sensor readings.
* Persisting state to EEPROM across resets.
* A GPIO driver evolving from a public struct, to an opaque type, to a vtable-based interface.

---

## Learning Outcomes
After completing the course, participants should be able to:
* Write and debug C code directly against a microcontroller's registers and datasheet.
* Move from blocking, polling-based designs to interrupt-driven, non-blocking ones.
* Keep interrupt handlers safe by flagging events in the ISR and handling them in `main()`.
* Bundle a peripheral's state into a struct to support several instances of the same peripheral.
* Split a driver across a header and source file, and use opacity to protect its invariants.
* Store and call through function pointers, including as struct members.
* Understand, and hand-implement, the vtable mechanism C++ uses for virtual dispatch.
* Be prepared to move directly into the Modern Embedded C++ follow-on course.

---

## Structure

```text
ci/          CI scripts (build and format checks).
info/        Course info: instructor, course plan, and per-lecture topic breakdown.
lectures/    Lecture READMEs, appendices, exercises, and worked hardware demos, L01-L10.
```

---

## No Hardware Yet?
Every worked demo in this repo can also be run on your host machine under the
[simavr](https://github.com/buserror/simavr) simulator, including UART output, button presses,
EEPROM persistence and `avr-gdb` debugging. See
[info/simulating_with_simavr.md](info/simulating_with_simavr.md). It is a way to keep moving until
you have a board, not a replacement for one.

---
