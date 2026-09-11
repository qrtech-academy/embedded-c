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

## Two Written Papers, and What They Are Not
Nothing in this course is marked. Assessment is the exercise set after every lecture, and the
Evaluation questions that close every lecture README.

[`exam/`](./exam/README.md) holds two three-hour papers with worked solutions, and they check
something else: **what you can reconstruct on paper, with nothing in front of you.** Eight questions
each, mixing theory with C you either read and repair or write from scratch, and no compiler,
datasheet or board in the room to tell you which it is.

**They exist purely so that participants can test their own knowledge after the course. They gate
nothing, they are not a qualification, and no part of the course requires them.** Nothing in this
repository depends on them, and neither `make build` nor `make format-check` knows they exist.

**Take one after the course is over.** Both papers draw on all ten lectures, so sitting one partway
through examines material nobody has taught you yet.

---

## The Book

The whole course is also available as a book: [Embedded C](./book/embedded-c.pdf).
It is built from the sources in [`book/`](./book/README.md), which also say how to build it yourself
(`make -C book`) and how a new edition is released. Besides the lectures, the demos and the two
papers, the book answers every exercise that has one right answer: the calculations and the
reflection questions.

---

## Structure

```text
ci/          CI scripts (build and format checks).
info/        Course info: instructor, course plan, and per-lecture topic breakdown.
lectures/    Lecture READMEs, appendices, exercises, and worked hardware demos, L01-L10.
exam/        Two written papers and their solutions. Optional, and marked by nobody here.
book/        The course typeset as a book with LuaLaTeX; `make -C book` builds the PDF.
```

---

## No Hardware Yet?
Every worked demo in this repo can also be run on your host machine under the
[simavr](https://github.com/buserror/simavr) simulator, including UART output, button presses,
EEPROM persistence and `avr-gdb` debugging. See
[info/simulating_with_simavr.md](info/simulating_with_simavr.md). It is a way to keep moving until
you have a board, not a replacement for one.

---

## License
The source code is released under the [MIT License](./LICENSE): the demo programs and their
drivers, the build and CI scripts, and the book's build files.

The course material is licensed under [CC BY-NC-SA 4.0](./LICENSE-CONTENT): the lectures,
exercises and exam papers, the other Markdown documents, the figures, and the book typeset from
them. You may share and adapt it for any non-commercial purpose, with credit, as long as what you
share carries the same license. The code examples printed in the lectures and in the book may also
be used under the MIT License. Third-party material in the figures (screenshots of other software,
and the board and component artwork in the circuit diagrams) remains subject to its owners' terms.

---
