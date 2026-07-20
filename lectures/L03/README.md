# L03 - Interrupts

## Agenda
* Why polling doesn't scale, and what an interrupt actually is.
* External interrupts: INT0/INT1 (`SREG`, `EICRA`, `EIMSK`), edge selection, and the `ISR`/vector
  mechanism.
* Worked example: toggle an LED via INT0.
* Worked example: blink an LED via INT1.
* Pin Change Interrupts (PCI): any other pin, at the cost of per-port grouping and both-edges
  triggering.
* Worked example: toggle an LED via a PCI interrupt, and the general pattern for any port.
* When to reach for INT0/INT1 versus PCI.

---

## Goals of the lecture
* Explain, in your own words, why interrupts exist and what problem they solve compared to
  polling.
* Be able to configure an external interrupt (INT0 or INT1) for a specific edge and write its
  ISR.
* Be able to configure a Pin Change Interrupt on an arbitrary pin, including working out the
  correct `PCINTx` number.
* Understand why PCI interrupts fire on both edges, and how to filter for the one you care about.

---

## Instructions

### Before the lecture
* Read [Appendix A](./appendix/a_interrupts.md) for the lecture's core material.

### During the lecture
* Participate actively in the walkthrough.

### After the lecture
* Work through the exercises in [Appendix B](./appendix/b_exercises.md).

---

## Evaluation
* What does `EICRA` configure that `EIMSK` doesn't, and vice versa?
* Why do multiple Pin Change Interrupts on the same port share a single ISR, and what does that
  mean for how you write it?
* A PCI-based button handler toggles an LED on both press *and* release. What line is missing,
  and why?

---

## Next lecture
* Analog-to-digital conversion (ADC): reading a continuous signal instead of a simple high/low.
* A worked example combining what you already know (PCI, bitwise register I/O) with the ADC.

---
