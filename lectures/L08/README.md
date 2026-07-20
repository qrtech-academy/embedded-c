# L08 - The Watchdog Timer

## Agenda
* The watchdog timer: a hardware safety net that resets the chip if the program stops
  "petting" it.
* Watchdog registers: `WDTCSR`, its own timed write sequence (`WDCE` then `WDE`), and the `WDR`
  instruction.
* Why a plain function is the right tool here: like EEPROM (L07), the watchdog is a singleton,
  there's exactly one on the chip.
* `WDRF`, the flag that can lock `WDE` on after a watchdog reset, and System Reset Mode vs.
  Interrupt Mode (`WDE` vs. `WDIE`).

---

## Goals of the lecture
* Explain what the watchdog timer protects against, and be able to enable it, reset it, and
  observe what happens when it isn't reset in time.

---

## Instructions

### Before the lecture
* Read [Appendix A](./appendix/a_watchdog.md) for the lecture's core material.

### During the lecture
* Participate actively in the walkthrough.

### After the lecture
* Work through the exercises in [Appendix B](./appendix/b_exercises.md).

---

## Evaluation
* What is the watchdog timer for, and what's the practical difference between a program that
  resets it every loop iteration and one that never resets it at all?

---

## Next lecture
* L09: now that EEPROM and the watchdog have shown what plain functions look like for a
  peripheral with exactly one instance, L09 covers what changes once a program needs several
  instances of the same kind of peripheral, structs as drivers.

---
