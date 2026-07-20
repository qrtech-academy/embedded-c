# Appendix B - Exercises

## Concepts
**1.** Calculate the value of `UBRR0` for a baud rate of `19200` at a 16 MHz clock. Round to the
nearest integer.

---

**2.** Explain what the `UDRE0` flag represents and why `serial_write_byte()` waits for it before
writing to `UDR0`.

---

## Program
**3.** Write a `serial_write_uint(const uint8_t byte)` helper that prints `byte` as decimal text,
one digit at a time, e.g. `serial_write_uint(243)` prints `243`. Extract each digit yourself via
division/modulo; don't just hand the whole number to `printf("%u", byte)`/`sprintf()` and let it do
the conversion for you, that's the same idea you'll need on any future toolchain where
`sprintf()`/float formatting isn't available.

This is a console exercise: a plain PC program is enough, no board or UART required. `printf()`
(or `putchar()`) stands in for `serial_write_byte()`, one call per digit.

Hint: dividing/modulo-ing a number by `10` repeatedly gives you its digits least-significant
first; printing them in that order comes out backwards. Instead, find the largest power of ten
that still fits inside `byte` first (`uint8_t` maxes out at `255`, so at most 3 digits), then walk
that divisor down by a factor of `10` each step, digits come out in the right order with no need
to reverse anything afterward. Watch out for `0`: make sure the search for the starting divisor
doesn't shrink it all the way down to nothing before the loop that actually emits digits ever
runs.

---

**4.** Extend `serial_driver_demo`'s driver with receive support, following A.7's 
`RXEN0`/`RXC0` pattern: enable the receiver in `serial_init()`, then add a `serial_read()` to `serial.h`/`serial.c` that blocks until a byte arrives.

Use it in `main()` so that receiving `'t'` over serial toggles LED1 the same way pressing the
button already does, reporting the new state the same way `ISR(PCINT0_vect)` already does.

---

**5.** Capstone, tying together ADC (Lecture 4), timers (Lecture 5), and UART (this lecture). Read
ambient temperature from a TMP36 sensor on A0 and report it over UART, either once a minute
(Timer-driven) or immediately on a button press on D13, debounced with a timer so contact bounce
doesn't spam the report:
* Reuse `adc_read()` from Lecture 4's A.3 to sample A0.
* TMP36 outputs `750 mV` at `25 °C` and rises `10 mV` per `°C`; converting the ADC's `0-1023`
  reading to a voltage and then to a temperature gives:
  ```
  T (°C) = 500 * adc_reading / 1023 - 50
  ```
* The reported message should look like `"Temperature: 23 C"` (build it with `sprintf()`, as in
  A.5).

For a harder version: keep the five most recent readings in a small array and report the running
average alongside the instantaneous reading; and instead of a fixed one-minute report interval,
track the time between the last five button presses and auto-adjust the report interval to match
how often the button is actually being pressed.

---
