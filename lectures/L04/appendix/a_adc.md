# Appendix A - Analog-to-Digital Conversion (ADC)

## A.1 Why ADC
* Everything so far has been binary: a pin is high or low. Real-world signals, like a
  potentiometer, a temperature sensor, or a microphone, vary continuously. An ADC
  (Analog-to-Digital Converter) samples that continuous voltage and returns a discrete number
  representing it.
* The ATmega328P has a 10-bit ADC: the result is an integer between `0` and `1023` (2¹⁰ − 1),
  representing an input voltage between `0 V` and whatever reference voltage you configure
  (default: the chip's own supply, `AVCC`, 5 V on the Uno/Nano).

The mapping is linear:

```
result = 1023 * (V_in / V_ref)
```

So `V_in = 0 V` → `0`, `V_in = V_ref` → `1023`, `V_in = V_ref / 2` → `~512`. More bits would mean
finer resolution for the same voltage range; 10 bits is enough for most sensor work, which is why
the chip stops there.

There's a flip side to this same problem worth previewing: this chip's outputs are just as binary
as its inputs, a pin is either driving high or low, nothing in between. So how do you make an LED
look dimmer instead of just on or off? By toggling it faster than the eye can follow and varying
how much of each cycle it spends on versus off. That ratio, the fraction of one cycle spent "on,"
is called the **duty cycle**; the **on-time** and **off-time** are just that fraction (and its
complement) converted into actual milliseconds for a chosen cycle length. A.4 combines both ideas:
an ADC reading drives the duty cycle, so turning a potentiometer changes how bright an LED appears.

---

## A.2 Which pin, which registers
The chip exposes 8 analog input channels, `ADC0`–`ADC7` (plus a 9th, `ADC8`, wired to an internal
temperature sensor). On the Uno/Nano, `ADC0`–`ADC5` map to `A0`–`A5`; you select a channel by writing
its number into the low 4 bits of `ADMUX`.

Three registers matter:

1. **`ADMUX`** selects the input channel (bits 0–3) and the voltage reference (bits 6–7). Setting
   `REFS0` (and leaving `REFS1` clear) selects the internal voltage `AVCC` as the reference, the
   normal choice:

    ```c
    // Use internal voltage reference, select ADC channel (0 for A0, 1 for A1 etc.).
    ADMUX = (1U << REFS0) | channel;
    ```

2. **`ADCSRA`** enables the ADC, starts a conversion, and sets its clock prescaler. The ADC needs a
   clock well below the CPU's; under ~200 kHz for full accuracy. At 16 MHz, a ÷128 prescaler gets
   you to 125 kHz:

    ```c
    // Enable the ADC (ADEN), start a conversion (ADSC), set prescaler to 128 (ADPSx).
    ADCSRA = (1U << ADEN) | (1U << ADSC) | (1U << ADPS0) | (1U << ADPS1) | (1U << ADPS2);
    ```

A conversion isn't instantaneous; poll the `ADIF` flag (or use an interrupt) until it's set, then
clear it by writing a `1` back to it:

    ```c
    while (0U == (ADCSRA & (1U << ADIF))) {} // Wait for the conversion.
    ADCSRA = (1U << ADIF);                   // Clear the flag for next time.
    ```
   
3. **`ADC`** holds the result. In C this macro already combines the two underlying 8-bit registers
   (`ADCH`/`ADCL`) into the full 10-bit value; you don't assemble it by hand. Without it, you'd
   have to do it yourself, and the read order matters:

    ```c
    #define BYTE_SHIFT 8U // Parameter to shift an entire byte.

    const uint8_t low = ADCL;  // Read ADCL first: locks ADCH until it's read.
    const uint8_t high = ADCH; // Same conversion's result; safe to read now.
    const uint16_t result = ((uint16_t)(high) << BYTE_SHIFT) | (uint16_t)(low);
    ```

**Note:**
* Reading `ADCH` first risks tearing the result: if a new conversion finishes between the two
  reads, the two halves can end up belonging to different conversions.
* Reading `ADCL` first locks both registers against further updates until `ADCH` is read too, so
  the pair always comes from the same conversion; the `ADC` macro already does this for you.
* Both operands are cast to `uint16_t` before combining. The cast on `high` matters for
correctness in general, not just for this specific value: `high` is a `uint8_t`, and shifting it
left by 8 without first widening it relies on C's automatic promotion to `int`.
* On AVR, `int` is 16 bits, so shifting a *full-range* byte left by 8 (e.g. `(uint8_t)0xFF << 8U`)
  can produce a value that no longer fits in a 16-bit signed `int`, and left-shifting past what the
  type can hold is undefined behavior in C.
* `ADCH` never actually uses more than its bottom 2 bits, so this particular shift is safe either
  way, but casting to `uint16_t` first is the habit that keeps it safe unconditionally: the shift
  then happens in an unsigned 16-bit type, where the result always wraps predictably instead of
  risking undefined behavior.
* The cast on `low` isn't load-bearing the same way (`uint8_t` always fits safely in the promoted
  type), but keeps both operands of `|` explicitly `uint16_t` instead of leaning on C's
  implicit-conversion rules to sort it out.

---

## A.3 A read function

```c
/**
 * @brief Read and return a 10-bit ADC conversion result.
 *
 * @param[in] channel ADC channel to read.
 *
 * @return Conversion result (0-1023).
 */
static uint16_t adc_read(const uint8_t channel)
{
    // Use internal voltage reference and select ADC channel to read.
    ADMUX = (1U << REFS0) | channel;

    // Perform conversion, wait for conversion to complete.
    ADCSRA = (1U << ADEN) | (1U << ADSC) | (1U << ADPS0) | (1U << ADPS1) | (1U << ADPS2);
    while (0U == (ADCSRA & (1U << ADIF))) {}

    // Restore the ADC interrupt flag for next conversion, then return the result.
    ADCSRA = (1U << ADIF);
    return ADC;
}
```

---

## A.4 Example: potentiometer-controlled LED brightness (software PWM)
There's no true analog output on this chip without a dedicated PWM peripheral, but as A.1
previewed, you can fake brightness control in software instead: flick an LED on and off fast enough
that the eye averages it into perceived brightness, driven by duty cycle.

Potentiometer on A0, LED on D9, toggle button on D13 (enables/disables reading; the LED stays off
while disabled).

![](./images/circuit_pwm_with_pot.png)

```c
#define F_CPU 16000000UL // CPU frequency in Hz.

#include <stdbool.h>
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

/** GPIO pins. */
#define LED1 1U // D9  -> PORTB1.
#define BTN1 5U // D13 -> PORTB5.
#define POT1 0U // A0  -> ADC0.

/** GPIO operations. */
#define LED1_ON PORTB |= (1U << LED1)      // Enable LED1.
#define LED1_OFF PORTB &= ~(1U << LED1)    // Disable LED1.
#define BTN1_PRESSED (PINB & (1U << BTN1)) // High if BTN1 is pressed, low otherwise.

/** Time parameters. */
#define PWM_PERIOD_MS 10U // PWM period in ms.
#define TICK_1MS 1U       // Generate a 1 ms tick in delay_ms().
#define ADC_MAX 1023U     // Maximum ADC conversion value.

/** ADC state (true = enabled, false = disabled). */
static volatile bool adc_state = false;

/**
 * @brief Set up system.
 */
static void setup(void)
{
    // Configure LED1 as output.
    DDRB = (1U << LED1);

    // Configure BTN1 as input with its internal pull-up enabled.
    PORTB = (1U << BTN1);

    // Enable pin change interrupts for BTN1.
    PCICR  = (1U << PCIE0);
    PCMSK0 = (1U << BTN1);
    sei();
}

/**
 * @brief Generate delay.
 *
 * @param[in] ms Delay duration in ms.
 */
static void delay_ms(const uint16_t ms)
{
    for (uint16_t i = 0U; i < ms; ++i)
    {
        _delay_ms(TICK_1MS);
    }
}

/**
 * @brief Read and return a 10-bit ADC conversion result.
 *
 * @param[in] channel ADC channel to read.
 *
 * @return Conversion result (0-1023).
 */
static uint16_t adc_read(const uint8_t channel)
{
    // Use internal voltage reference and select ADC channel to read.
    ADMUX = (1U << REFS0) | channel;

    // Perform conversion, wait for conversion to complete.
    ADCSRA = (1U << ADEN) | (1U << ADSC) | (1U << ADPS0) | (1U << ADPS1) | (1U << ADPS2);
    while (0U == (ADCSRA & (1U << ADIF))) {}

    // Restore the ADC interrupt flag for next conversion, then return the result.
    ADCSRA = (1U << ADIF);
    return ADC;
}

/**
 * @brief Run a single PWM cycle.
 */
static void run_pwm_cycle(void)
{
    // Check if the ADC is enabled, terminate the function if not.
    if (!adc_state)
    {
        LED1_OFF;
        return;
    }

    // Read the ADC and compute the duty cycle.
    const uint16_t adc_val = adc_read(POT1);
    const float duty_cycle = (float)(adc_val) / ADC_MAX;

    // Compute the PWM durations based on the duty cycle, round to the nearest integer.
    const uint8_t on_ms  = (uint8_t)(duty_cycle * PWM_PERIOD_MS + 0.5F);
    const uint8_t off_ms = PWM_PERIOD_MS - on_ms;

    // Enable LED1 during the active duration.
    LED1_ON;
    delay_ms(on_ms);

    // Disable LED1 during the inactive duration.
    LED1_OFF;
    delay_ms(off_ms);
}

/**
 * @brief Toggle the ADC state on button press.
 */
ISR(PCINT0_vect)
{
    if (BTN1_PRESSED) { adc_state = !adc_state; }
}

/**
 * @brief Application entry point.
 *
 * @return 0 on termination of the program (should never occur).
 */
int main(void)
{
    setup();

    while (1)
    {
        run_pwm_cycle();
    }
    return 0;
}
```

Turning the potentiometer changes the ADC reading, which changes the computed duty cycle, which
changes how bright the LED appears.

Note `delay_ms()` is built on `_delay_ms()`, so it has the same blocking limitation flagged in
Lecture 2; this software-PWM approach ties up the CPU for the whole 10 ms period. The Timers
lecture (L05) replaces this with a non-blocking, interrupt-driven equivalent, using the same
overflow-counting pattern taught there instead of a dedicated hardware PWM peripheral.

---
