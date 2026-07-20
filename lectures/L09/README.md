# L09 - Structs as Drivers

## Agenda
* Why L04's ADC and L06-L08's UART/EEPROM/watchdog functions got away with no struct (they're
  singletons), and why per-instance macros and globals stop scaling for everything else:
  revisiting L04's multi-LED exercise.
* Bundling a peripheral's state into a struct, and writing "methods" as functions that take a
  pointer to that struct as their first argument.
* Building a GPIO (`gpio_t`) driver, split into a header and a source file.
* Worked example: rebuilding L08's watchdog demo on `gpio_t`, adding pin-change-interrupt support
  to the GPIO driver along the way.
* Exercise: a complete `Timer` driver, with its own header/source split and a callback, built the
  same way as `gpio_t`.

---

## Goals of the lecture
* Explain why hardcoded per-instance functions or macros break down once a program needs more
  than one of something.
* Be able to design a struct that bundles a peripheral's configuration and state.
* Be able to write "methods" as plain functions taking a pointer to that struct as their first
  parameter (the `self` convention).
* Refactor a program built on duplicated globals into one built on an array of struct instances.

---

## Instructions

### Before the lecture
* Read [Appendix A](./appendix/a_structs_as_drivers.md) for the lecture's core material.

### During the lecture
* Participate actively in the walkthrough.

### After the lecture
* Work through the exercises in [Appendix B](./appendix/b_exercises.md).

---

## Evaluation
* What problem does bundling a pin number into a struct solve that a `#define`d macro can't?
* Why does `gpio_read()` take a pointer to a `gpio_t` rather than a `gpio_t` by value?
* How would you rewrite five separate `LED1_ON`/`LED2_ON`/... macros as a single `gpio_write()`
  function plus an array of `gpio_t` instances?

---

## Next lecture
* L10: hiding a struct's implementation details (opaque structs) and manual polymorphism via
  function pointers and vtables.

---
