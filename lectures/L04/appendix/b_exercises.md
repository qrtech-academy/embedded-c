# Appendix B - Exercises

## Concepts
**1.** A potentiometer reads `768` from `adc_read()`. Given a 5 V reference, what voltage does
that correspond to? Show the calculation.

---

**2.** Why does the ADC need its own clock prescaler, separate from the CPU's? What would you
expect to happen to the reading's accuracy if the ADC ran at the full 16 MHz CPU clock instead?

---

## Registers
**3.** Which single bit would you flip in `ADMUX` to read from `A1` instead of `A0`, assuming the
reference voltage stays the same?

---

**4.** Explain, in your own words, why the code polls the `ADIF` flag in a `while` loop after
starting a conversion, rather than reading `ADC` immediately after setting `ADSC`.

---

## Program
**5.** Extend the lecture's PWM-brightness example so that, instead of software PWM, it simply
lights the LED solid whenever the potentiometer reading exceeds `512`, and turns it off otherwise,
a basic threshold switch with no PWM. Rewrite `run_pwm_cycle()` (rename it too; it's no
longer PWM).

---

**6.** What would you expect to happen if the `while (0U == (ADCSRA & (1U << ADIF))) {}` wait
were removed entirely from `adc_read()`? Would the program crash outright, or misbehave more
subtly? Explain your reasoning.

---

**7.** Capstone. Extend A.4's software-PWM approach to dim five LEDs from a single potentiometer,
each individually switchable:
* LEDs on D6-D10.
* Buttons on D2, D3, D11-D13:
  * D2/D3 are `INT0`/`INT1` from Lecture 3.
  * D11-D13 share a Pin Change Interrupt, also from Lecture 3.

Each button shall toggle the PWM state of an LED:
* Button D2 toggles LED D6.
* D3 toggles D7.
* D11 toggles D8.
* D12 toggles D9.
* D13 toggles D10.

Only LEDs currently "on" should respond to the potentiometer's duty cycle; "off" LEDs stay dark no
matter where the pot is turned.

Reuse `adc_read()` from A.3 for the potentiometer reading and the duty-cycle math from A.4.
* Track which of the five LEDs are enabled using a `uint8_t` bitmask with bitwise tests.
* Drive the on/off timing for all five within one PWM period instead of just one.
* The Pin Change Interrupt runs on any change of D11-D13, so its ISR must work out which button was
  just pressed; use the `prev`/`now` technique from Lecture 3, A.8.

---
