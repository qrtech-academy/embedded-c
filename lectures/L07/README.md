# L07 - EEPROM

## Agenda
* EEPROM: non-volatile storage that survives a power cycle or reset, and why it's slow and
  wear-limited compared to RAM.
* EEPROM registers: `EEAR`, `EEDR`, `EECR`, and the timed write sequence (`EEMPE` then `EEPE`).
* `eeprom_write_byte()` / `eeprom_read_byte()`, and why interrupts are disabled around a write.
* Why a plain function is the right tool here: there's exactly one EEPROM on the chip.
* Worked example: persisting LED1's state across resets, and why the EEPROM write and UART
  report both happen in `main()`, never inside an ISR.

---

## Goals of the lecture
* Explain what EEPROM is for and why it's unsuitable for data that changes every loop iteration.
* Be able to read from and write to EEPROM at a given address.
* Explain why EEPROM's write sequence disables interrupts, and what could go wrong if it
  didn't.
* Explain why slow, blocking calls like `eeprom_write()` and `serial_print()` don't belong
  inside an ISR.

---

## Instructions

### Before the lecture
* Read [Appendix A](./appendix/a_eeprom.md) for the lecture's core material.

### During the lecture
* Participate actively in the walkthrough.

### After the lecture
* Work through the exercises in [Appendix B](./appendix/b_exercises.md).

---

## Evaluation
* Why does `eeprom_write_byte()` wait on the `EEPE` flag before starting a new write?
* What real AVR datasheet requirement forces `EEMPE` to be followed by `EEPE` within 4 clock
  cycles, and why does the example disable interrupts for that window?
* In A.4's worked example, why does `ISR(PCINT0_vect)` only toggle LED1 and set a flag, leaving
  the actual `eeprom_write()` and UART report to `main()`?

---

## Next lecture
* L08: the watchdog timer, another peripheral the chip has exactly one of, and a hardware
  safety net against a program that hangs.

---
