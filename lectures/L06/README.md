# L06 - Serial Communication (UART)

## Agenda
* Why serial: a real communication channel to a PC instead of blink-and-infer debugging.
* Configuring UART: `UCSR0B`, `UCSR0C`, and the `UBRR0` baud-rate formula.
* Sending data: `UDR0`, the `UDRE0` flag, and building a reusable text-transmit helper.
* Mixing text and numbers with `sprintf()`.
* Receiving data: `RXEN0`, the `RXC0` flag, and reading `UDR0` instead of writing it.
* Worked example: button toggles an LED and reports its new state over serial, using a driver
  split across its own header and source file.

---

## Goals of the lecture
* Explain what baud rate is and compute `UBRR0` for a given clock and baud rate.
* Be able to write a byte, and then a whole string, over UART.
* Be able to combine `sprintf()` with UART transmission to report a mix of text and numeric data.
* Understand why the code waits on `UDRE0` before writing a new byte.
* Be able to receive a byte over UART and explain how `RXC0` mirrors `UDRE0`.

---

## Instructions

### Before the lecture
* Read [Appendix A](./appendix/a_uart.md) for the lecture's core material.

### During the lecture
* Participate actively in the walkthrough.

### After the lecture
* Work through the exercises in [Appendix B](./appendix/b_exercises.md).

---

## Evaluation
* What does the `UDRE0` flag represent, and what would happen if `serial_write_byte()` didn't wait
  for it?
* Compute `UBRR0` for 19200 baud at a 16 MHz clock.
* Why is `sprintf()` used to build a message before transmitting it, rather than trying to transmit
  an `int` directly?
* What does `RXC0` represent, and what would `serial_read_byte()` do if it didn't wait for it?

---

## Next lecture
This closes the introductory arc:
* C Fundamentals (L01).
* Digital I/O and bit manipulation (L02).
* Reacting to events instead of polling for them (L03).
* Reading the analog world (L04).
* Timing without blocking (L05).
* Reporting back to a PC (L06).

L07 rounds out the peripheral survey with EEPROM, and L08 covers the watchdog timer. A follow-on
arc on driver design starts after that: bundling a peripheral's state into a struct instead of
one-off macros and globals, building directly on the LEDs, buttons, and timers from these six
lectures.

---
