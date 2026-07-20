# Appendix B - Exercises

## Concepts
**1.** The watchdog's `WDE`/prescaler bits need the same `WDCE`-then-write-within-four-cycles
pattern as EEPROM's `EEMPE`/`EEPE` (L07). Unlike EEPROM, this isn't just a hardware quirk, it's
a deliberate safety feature. What is it protecting against?

---

## Program
**2.** Using A.5's demo, add a `_delay_ms(1500)` (`<util/delay.h>`) inside `main()`'s loop, longer
than the watchdog's configured 1024 ms timeout. Confirm the board still resets on its own, even
though `watchdog_reset()` is still called every iteration, just not often enough. Calling it
somewhere in the code isn't enough; the loop has to actually get back around to it in time.

---

**3.** Capstone, harder. Combine EEPROM (L07) and the watchdog: reduce EEPROM wear by only
writing every 10th button press instead of every press. Keep a small `uint8_t` counter (not
itself persisted) that tracks presses since the last EEPROM write, and only call
`eeprom_write()` once it reaches `10`.

---

**4.** Capstone, harder. A.5's driver only supports System Reset Mode. Extend it yourself with
Interrupt Mode (A.4):
* Add a way to arm the watchdog with `WDIE` set instead of `WDE`, plus an `ISR(WDT_vect)`.
* Then combine both modes (`WDE` and `WDIE` together) so the first missed timeout logs a warning over UART instead of resetting, and only a second consecutive missed timeout actually resets the chip.

---
