# L05 - Timers

## Agenda
* Why `_delay_ms()` doesn't scale, and what a hardware timer gives you instead.
* The prescaler math: from CPU clock to tick period to interrupt count.
* Normal Mode vs. CTC Mode.
* Worked example: non-blocking LED blink using Timer 0 (Normal Mode).
* Timer 1 in CTC Mode: hitting an arbitrary period directly.

---

## Goals of the lecture
* Explain why a busy-wait delay (`_delay_ms`) is unsuitable once a program needs to do more than
  one thing at a time.
* Be able to compute a timer's tick period from the CPU clock and a chosen prescaler.
* Be able to work out how many overflow interrupts are needed for a given delay, for an 8-bit
  timer.
* Understand the practical difference between Normal Mode and CTC Mode, and when you'd reach for
  each.
* Know why a variable shared between an ISR and `main()` needs to be declared `volatile`.

---

## Instructions

### Before the lecture
* Read [Appendix A](./appendix/a_timers.md) for the lecture's core material.

### During the lecture
* Participate actively in the walkthrough.

### After the lecture
* Work through the exercises in [Appendix B](./appendix/b_exercises.md).

---

## Evaluation
* At 16 MHz with a ÷256 prescaler, what's the resulting tick period?
* What's the difference between what triggers `TIMERn_OVF_vect` and what triggers
  `TIMERn_COMPA_vect`?
* Why is `overflow_count` declared `volatile` in the lecture's example, and what could go wrong
  if it weren't?

---

## Next lecture
* Serial communication (UART): transmitting data to a PC instead of only driving LEDs.
* Combining timers with UART to report readings periodically.

---
