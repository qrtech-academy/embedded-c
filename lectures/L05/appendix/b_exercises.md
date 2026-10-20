# Appendix B - Exercises

## Concepts
**1.** At 16 MHz with a ÷256 prescaler, what's the resulting tick frequency and tick period? Show
your work.

---

**2.** Using your answer to Exercise 1, how many overflow interrupts (8-bit timer) would be needed
to time a 250 ms delay?

---

## Normal vs. CTC
**3.** In your own words, explain when you'd reach for CTC mode instead of Normal mode with
software overflow-counting. Is there a period that's awkward to hit precisely in Normal mode but
trivial in CTC?

---

## Program
**4.** Adapt the lecture's Timer 0 blink example to blink at 2 Hz (250 ms on, 250 ms off) instead
of 1 Hz. What value changes, and what does the new `OVF_MAX` work out to?

---

**5.** The lecture's Timer 0 example declares `ovf_count` as `volatile`, and A.4 says that removing
the qualifier there would change nothing. Explain why. Then suppose `ovf_count` moved to file scope
so that `main()` could wait for it, in a loop such as `while (10U > ovf_count) {}`. What specific
failure could removing `volatile` now introduce, and why might it show up in one build and not in
another?

---

**6.** Configure Timer 1 in CTC mode for a period of exactly 2 seconds at 16 MHz with a ÷1024
prescaler. What value goes in `OCR1A`? (Timer 1 is 16-bit, so check whether your answer fits.)

---

**7.** Capstone. Two LEDs on D9/D10 blink at a speed selected by whichever of three buttons
(D11-D13) was most recently pressed, each mapped to a different timer:
* D11 selects Timer 0 (1000 ms per toggle), D12 selects Timer 1 (500 ms per toggle), D13 selects
  Timer 2 (100 ms per toggle).
* Pressing a button toggles *that* timer's blinking on or off. If it was off, turn it on and turn
  the other two off (only one timer drives the LEDs at a time). If it was already the active timer,
  turn it off entirely and the LEDs go dark.
* All three buttons are on port B and share `PCINT0_vect`; detect which one was just pressed with
  the `prev`/`now` edge detection from Lecture 3, A.8.
* Configure all three timers to tick at the same rate (÷1024 prescaler, ~0.064 ms per tick) so the
  overflow-counting logic in A.4 works identically for each. Timer 1 needs CTC mode with `OCR1A`
  set so its compare-match period matches Timer 0/2's overflow period, even though its 16-bit range
  could hit the target directly, so all three interrupt handlers end up the same shape.
* Define a small struct to hold `required_interrupts` and `executed_interrupts` for one timer (see
  A.2/A.4 for the counting pattern), and use three instances, one per timer, instead of three sets
  of loose globals.
* Track the currently active timer in a single variable, using either an enum (`TIMER_NONE`,
  `TIMER_0`, `TIMER_1`, `TIMER_2`) or macros with timer indexes.

---
