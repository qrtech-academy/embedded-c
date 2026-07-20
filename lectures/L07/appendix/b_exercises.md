# Appendix B - Exercises

## Concepts
**1.** Why does `eeprom_write_byte()` wait on the `EEPE` flag before it starts, but
`eeprom_read_byte()` doesn't need to wait for anything after its read completes?

---

**2.** `EEMPE` must be followed by `EEPE` within four CPU cycles or the write silently fails.
Why does that specifically mean interrupts need to be disabled for that window, and what real
failure would you expect if an ISR fired in between and took longer than four cycles to return?

---

## Program
**3.** A.4's `eeprom_driver_demo` writes to EEPROM on every single button press. Given A.1's
write-cycle warning, is that actually a good idea for a device expected to run for years? Extend
the demo to reduce wear: keep a small, non-persisted counter of presses since the last EEPROM
write, and only call `eeprom_write()` once it reaches `5`, instead of on every press. `main()`'s
event handling already separates "an event happened" from "write it out", build on that rather
than restructuring it.

---
