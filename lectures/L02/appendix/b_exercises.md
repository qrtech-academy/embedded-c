# Appendix B - Exercises

## Bit manipulation
**1.** Given a register currently holding `0b10110010`, write single-line C expressions (using
`|`, `&`, `~`, `<<`) to:
**a)** Set bit 0 without affecting any other bit.
**b)** Clear bit 5 without affecting any other bit.
**c)** Read whether bit 4 is set, as a boolean expression.

**Tip:** You don't need real hardware to test this; write a small program on your own machine and
check the results by invoking `printf()` from `<stdio.h>`:

```c
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    const uint8_t reg = (1U << 3U);
    printf("%u\n", reg);
    return 0;
}
```

---

**2.** A macro is defined as `#define LED 6U`. Write the two macros `LED_ON` and `LED_OFF`,
operating on a register called `PORTD`, following the OR/AND-NOT pattern used in the lecture.

---

## Registers
**3.** For each of the following, name the correct register (`DDRx`, `PORTx`, or `PINx`) and
explain in one sentence why:
**a)** Configuring D5 as an output.
**b)** Reading whether a button on D2 is currently pressed.
**c)** Enabling the internal pull-up resistor on D7 configured as an input.

---

## Program
**4.** Wire up LEDs and buttons as described below:
* `LED1` on D6 and `LED2` on D7.
* `BTN1` on D2 and `BTN2` on D3.

Write a complete program, from scratch, that:
* Lights `LED1` if `BTN1` is pressed and turns it off otherwise.
* Lights `LED2` if `BTN2` is released and turns it off otherwise.

Don't copy the lecture's example directly; use these different pin numbers and re-derive the
macros yourself.

---

**5.** Wire up LEDs and buttons as described below:
* `LED1` on D5, `LED2` on D6, and `LED3` on D7.
* `BTN1` on D4.

Write a complete program, from scratch, that:
* While `BTN1` is held, cycles through the LEDs one at a time, in order (`LED1` → `LED2` →
  `LED3` → `LED1` ...), lighting exactly one for 100 ms before moving to the next.
* Turns off all LEDs otherwise.

---
