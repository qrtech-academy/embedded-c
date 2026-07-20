# L04 - Analog-to-Digital Conversion (ADC)

## Agenda
* Why ADC: reading continuous voltages instead of a simple high/low.
* The 10-bit ADC on the ATmega328P and the linear scaling formula.
* Registers: `ADMUX`, `ADCSRA`, and the combined `ADC` result register.
* A reusable `adc_read()` function.
* Worked example: potentiometer-controlled LED brightness via software PWM.

---

## Goals of the lecture
* Explain what an ADC does and why its result is a discrete number, not a continuous value.
* Be able to compute the voltage a given ADC reading corresponds to, and vice versa.
* Be able to configure and read from a chosen ADC channel.
* Understand why the ADC needs its own clock prescaler, separate from the CPU clock.

---

## Instructions

### Before the lecture
* Read [Appendix A](./appendix/a_adc.md) for the lecture's core material.

### During the lecture
* Participate actively in the walkthrough.

### After the lecture
* Work through the exercises in [Appendix B](./appendix/b_exercises.md).

---

## Evaluation
* At a 5 V reference, what analog voltage does an ADC reading of `0`, `511`, and `1023` each
  correspond to?
* Why does the code wait on the `ADIF` flag instead of reading `ADC` immediately after starting a
  conversion?
* What is "duty cycle," and how does the software-PWM example use it to control perceived LED
  brightness?

---

## Next lecture
* Timers: replacing blocking `_delay_ms()` calls with non-blocking, interrupt-driven timing.
* Normal Mode vs. CTC Mode, and the prescaler math behind both.

---
