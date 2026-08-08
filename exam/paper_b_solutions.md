# Paper B - Solutions

Marks are shown per part. Method carries them: a correct diagnosis with a clumsy fix is worth more
than a correct fix with no diagnosis, and several later parts consume earlier answers, so an error
should be followed through rather than penalised twice.

Code is marked on semantics. Missing semicolons, missing includes and misplaced braces cost nothing.

**The candidate had the register reference at the back of the paper.** Every register and bit name,
every bit *position* where one matters, and the `ISCn1:ISCn0`, `ADPS2:ADPS0` and `CSn2:CSn0`
encodings were in front of them. What was deliberately withheld is the part being examined: the
ordering rules, the timing rules and the formulas. So a misspelled bit costs nothing, but a candidate
who cannot say which of `EEMPE` and `EEPE` comes first, or derive an encoding from bit positions the
reference gave them, has not answered the question.

Where a question asks for code "in the course's style", the conventions in the rubric are marked:
`snake_case`, `static` on anything private, `self` as a pointer, Yoda comparisons, braces on every
`if`, fixed-width types, and `volatile` on anything shared with an ISR. Doxygen comments are marked
only where the question says so.

---

## Question 1 - Saying what you mean (12 marks)

### (a) 4 marks

```c
/**
 * @brief Enumeration of LED blink patterns.
 */
typedef enum
{
    LED_PATTERN_OFF,       ///< LED is off.
    LED_PATTERN_SLOW,      ///< Slow blink.
    LED_PATTERN_FAST,      ///< Fast blink.
    LED_PATTERN_HEARTBEAT, ///< Two short pulses, then a pause.
} led_pattern_t;

/**
 * @brief Check whether the given pattern is a named member of led_pattern_t.
 *
 * @param[in] pattern Pattern to check.
 *
 * @return True if the pattern is valid, false otherwise.
 */
bool led_pattern_valid(const led_pattern_t pattern)
{
    switch (pattern)
    {
        case LED_PATTERN_OFF:
        case LED_PATTERN_SLOW:
        case LED_PATTERN_FAST:
        case LED_PATTERN_HEARTBEAT:
            return true;
        default:
            return false;
    }
}
```

*(2 marks. The prototype in a header drops the top-level `const`:
`bool led_pattern_valid(led_pattern_t pattern);` — credit a candidate who says so. A range check
`(LED_PATTERN_HEARTBEAT >= pattern)` also earns the marks, and a candidate who notes that it only
works because the members happen to be contiguous, so it silently stops being correct the day one
takes an explicit value, has answered better.)*

**Three advantages over a block of `#define`s**, any three of:

* **The type survives to runtime.** A debugger shows `LED_PATTERN_FAST` by name; a macro was erased
  by the preprocessor before there was a program to debug.
* **The compiler can help with `switch`.** Some compilers warn when a `switch` on an enumeration
  does not handle every named case. Nothing comparable exists for a set of macros, which are just
  integers by the time the compiler sees them.
* **Scope and namespacing.** An enumerator is an ordinary identifier obeying the usual scoping
  rules. A macro is live text substitution from its `#define` to the end of the translation unit and
  every file that includes it, colliding with anything of the same name anywhere.
* **The values maintain themselves.** Members count up from 0 in declaration order, so adding or
  reordering one does not mean renumbering a block of literals by hand.

*(1 mark)*

**The property `#define` has that `const uint8_t` does not: it is a compile-time constant
expression.** A `const` variable in C cannot change at runtime, but the language still refuses it
anywhere a *constant expression* is required — an array bound, a `case` label, a `#if`. A macro
works in all of them, and folds into the generated code, so `PORTB |= (1U << LED1)` becomes a single
bit-set instruction rather than a runtime shift. (An `enum` constant has this property too, which is
why the course reaches for an enum wherever a named set fits and for a macro where a bare number is
needed.)
*(1 mark)*

### (b) 3 marks

```c
#define TICKS_NEEDED(timeout_ms, tick_ms) ((timeout_ms) / (tick_ms))
```

*(1 mark)*

**The two rules.** *(1 mark)*

1. **Parenthesise the whole body**, so the macro's meaning does not depend on what surrounds it at
   the call site. Substitution happens before precedence gets a chance to group anything.
2. **Parenthesise every use of every parameter**, so an argument that is itself an expression is
   grouped as the caller wrote it.

**A call that goes wrong** when rule 2 is broken:

```c
#define TICKS_NEEDED(timeout_ms, tick_ms) (timeout_ms / tick_ms) // Parameters unparenthesised.

TICKS_NEEDED(TIMEOUT_MS + MARGIN_MS, 4U)
// Expands to: (TIMEOUT_MS + MARGIN_MS / 4U)
// Only the margin is divided. The timeout is added to it whole. No warning.
```

*(Credit a rule-1 example instead, e.g. a body ending in `+ 2U` used as `2U * TICKS_NEEDED(...)`.)*

**The third hazard: multiple evaluation.** A parameter's text is substituted at *every* point its
name appears in the body, so an argument with a side effect happens once per appearance, in an
unspecified order. A real function evaluates each argument exactly once.

**This macro is not exposed to it**, because each parameter appears exactly once in the body. That
is a property of this particular body, not a guarantee the macro mechanism offers: the moment
somebody adds a second use of `timeout_ms` — a clamp, a rounding term — `TICKS_NEEDED(read_ms(), 4U)`
starts calling `read_ms()` twice, and no amount of parenthesising prevents it.
*(1 mark)*

### (c) 3 marks

```c
/**
 * @brief Grow or shrink a buffer's backing storage.
 *
 * @param[in,out] self Pointer to the buffer to resize.
 * @param[in] size New size in bytes.
 *
 * @return 0 on success, 1 on failure (self is left untouched).
 */
static int buffer_resize(buffer_t* self, const size_t size)
{
    if (NULL == self) { return 1; }

    // A size of zero means "release everything and go back to empty".
    if (0U == size)
    {
        free(self->data);
        self->data = NULL;
        self->size = 0U;
        return 0;
    }

    // Capture the result in a temporary: a failed realloc() must not overwrite self->data.
    uint8_t* copy = (uint8_t*)(realloc(self->data, size));
    if (NULL == copy) { return 1; }

    self->data = copy;
    self->size = size;
    return 0;
}
```

*(2 marks: 0.5 for the `NULL` check, 0.5 for the zero-size branch, 1 for the temporary and the
failure path. `realloc(NULL, size)` behaves as `malloc(size)`, so the first call needs no special
case — credit a candidate who says so.)*

**The mistake in `self->data = realloc(self->data, size);`.** On failure `realloc()` returns `NULL`
and **leaves the original block allocated and entirely valid** — that is the contract. The
assignment has just overwritten the only pointer to it. The old block is therefore leaked with no
way to reach it again, the buffer's contents are gone, and `self->size` now describes a null
pointer, so the next read through it is a null dereference rather than a clean failure. Capturing
into a temporary and only committing on success costs one local variable and loses nothing.
*(1 mark)*

### (d) 2 marks

```c
#ifndef DRIVER_SERIAL_H_
#define DRIVER_SERIAL_H_

/* ... declarations ... */

#endif /* DRIVER_SERIAL_H_ */
```

**What it is for.** A header can be reached more than once in a single translation unit — included
directly and pulled in again through another header — and the compiler would then see the same
declarations twice, which is an error for anything that cannot be redeclared. The guard makes the
second and later inclusions expand to nothing.

**The convention** is the file name in `UPPER_CASE` with `.` replaced by `_` and a trailing `_`,
qualified by its directory where it has one (`driver/serial.h` → `DRIVER_SERIAL_H_`), chosen so the
macro is unlikely to collide with any other in the program. `#pragma once` does the same job in one
line and is widely supported, but is not part of the C standard, so the traditional guard is the
portable choice.
*(1 mark)*

**The ordering convention inside a `.c` file:** the `#include`s first, then forward declarations of
the file's own `static` helpers, then the definitions of the public functions in the order the
header declares them, and the `static` helpers' own definitions at the bottom.

**The reason:** a reader opening the file meets the module's public API first and only descends into
private implementation detail if they keep scrolling. The forward declarations at the top are what
allow that ordering without the compiler complaining about a call to a function it has not seen yet.
*(1 mark)*

---

## Question 2 - Straight to the registers (12 marks)

### (a) 5 marks

```c
#define F_CPU 16000000UL // CPU frequency in Hz.

#include <stdint.h>

#include <avr/io.h>
#include <util/delay.h>

/** GPIO pins. */
#define LED1 0U // D8  -> PORTB0.
#define LED2 1U // D9  -> PORTB1.
#define LED3 2U // D10 -> PORTB2.
#define BTN1 4U // D12 -> PORTB4.

/** Bit mask covering every LED. */
#define LED_MASK ((1U << LED1) | (1U << LED2) | (1U << LED3))

/** GPIO operations. */
#define LED_ON(led) PORTB |= (1U << (led))  // Enable the given LED.
#define LEDS_OFF PORTB &= ~LED_MASK         // Disable every LED.
#define BTN1_PRESSED (PINB & (1U << BTN1))  // High if BTN1 is pressed, low otherwise.

/** Time parameters. */
#define TICK_1MS 1U       // Generate a 1 ms tick in delay_ms().
#define STEP_TIME_MS 150U // Time each LED stays lit, in ms.

/** Number of LEDs taking part in the chase. */
#define LED_COUNT 3U

/** Bit positions of the LEDs, in the order they are lit. */
static const uint8_t leds[LED_COUNT] = {LED1, LED2, LED3};

/**
 * @brief Set up system.
 */
static void setup(void)
{
    // Configure LED1 - LED3 as outputs.
    DDRB = LED_MASK;

    // Configure BTN1 as input with its internal pull-up enabled.
    PORTB = (1U << BTN1);
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
 * @brief Light a single LED for one step of the chase.
 *
 * @param[in] index Index of the LED to light.
 */
static void run_chase_step(const uint8_t index)
{
    LEDS_OFF;
    LED_ON(leds[index]);
    delay_ms(STEP_TIME_MS);
}

/**
 * @brief Application entry point.
 *
 * @return 0 on termination of the program (should never occur).
 */
int main(void)
{
    uint8_t index = 0U;
    setup();

    while (1)
    {
        // Advance the chase while BTN1 is held, disable every LED otherwise.
        if (BTN1_PRESSED)
        {
            run_chase_step(index);
            if (LED_COUNT <= ++index) { index = 0U; }
        }
        else
        {
            LEDS_OFF;
            index = 0U;
        }
    }
    return 0;
}
```

*(5 marks, distributed: 1 for the correct port bit positions — D8/D9/D10 are `PORTB0`/`PORTB1`/
`PORTB2` and D12 is `PORTB4`, not 8/9/10/12; 1 for `F_CPU` defined before `<util/delay.h>` and the
`delay_ms()` loop; 1 for reading the button from `PINB`; 1 for `LEDS_OFF` clearing with `~` and
leaving the pull-up bit alone; 1 for the chase advancing and resetting correctly.)*

**Worth noting in a strong answer:** `run_chase_step()` blocks for the full 150 ms, so a release is
only noticed at the top of the next iteration — up to 150 ms late. That is the same blocking
limitation `_delay_ms()` carries throughout L02-L04, and exactly what L05's timers exist to remove.
A candidate who flags it has understood the program rather than only written it.

*A per-LED macro family (`LED1_ON`, `LED2_ON`, ...) and a hand-written three-way `switch` is also a
correct answer and loses nothing. It is, however, precisely the pattern L09 is about replacing.*

### (b) 4 marks

| Operation | Expression |
| --------- | ---------- |
| Set bit `n` | `reg \|= (1U << n);` |
| Clear bit `n` | `reg &= ~(1U << n);` |
| Test bit `n` | `(reg & (1U << n))` |
| Toggle bit `n` via `PORTx` | `PORTB ^= (1U << n);` |
| Toggle bit `n` atomically | `PINB = (1U << n);` |

*(2.5 marks; 0.5 each.)*

**`DDRB = (1U << 3U);` replaces the whole register.** Bit 3 becomes an output and **every other pin
on port B is forced back to input**, whatever it had been configured as. `DDRB |= (1U << 3U);` sets
bit 3 and leaves the other seven exactly as they were.
*(0.5 marks)*

**When each is right.** *(1 mark)*

* **`=`** when the line is establishing the whole port from a known state in one place — the single
  configuration line in `setup()` that declares every direction at once. It is also a plain store
  rather than a read-modify-write, so it is one instruction and cannot be torn by an interrupt.
* **`|=`** whenever some other part of the program may already have configured a bit that must not
  be disturbed: anything running after `setup()`, and anything inside a driver that owns one pin and
  knows nothing about the rest of the port. A driver that used `=` would silently deconfigure every
  other peripheral sharing its port.

### (c) 3 marks

**`PORTx` on an output pin** sets the level driven onto the pin: `1` drives it high, `0` drives it
low.

**`PORTx` on an input pin** does something else entirely: it enables (`1`) or disables (`0`) the
pin's **internal pull-up resistor**. It does not read anything and it does not drive the pin.

**The pin's actual level is read from `PINx`**, never from `PORTx`. Reading `PINB` returns the real
logic level present on each of port B's pins, whatever their direction.
*(1.5 marks)*

**The appendix's circuit.** One leg of the button goes to 5 V, a 10 kΩ resistor runs from that same
leg to ground, and D13 is tapped between the two — an **external pull-down**. So the pin reads `0`
when idle and `1` while the button is held, the opposite way round from the internal-pull-up
convention.

`PORTB = (1U << BTN1);` enables the AVR's internal pull-up on that pin anyway. It is **harmless
rather than contradictory**: the internal pull-up is weak, tens of kΩ, and the external 10 kΩ
resistor dominates the divider, so the idle level stays at ground and the reading is unchanged. It
is there so that `PORTx`'s input-mode behaviour is exercised somewhere visible, not because this
particular circuit needs it — and a candidate who says the circuit would work identically without
that line is right.
*(1.5 marks)*

---

## Question 3 - Reacting instead of polling (12 marks)

### (a) 5 marks

D3 is `PORTD3`, which is **INT1**. D10 is `PORTB2`.

```c
#include <avr/interrupt.h>
#include <avr/io.h>

/** GPIO pins. */
#define LED1 2U // D10 -> PORTB2.

/** GPIO operations. */
#define LED1_TOGGLE PINB = (1U << LED1) // Toggle LED1 atomically.

/**
 * @brief Set up system.
 */
static void setup(void)
{
    // Configure LED1 as output.
    DDRB = (1U << LED1);

    // Enable rising edge interrupt on INT1 (D3) => a button press causes an interrupt.
    EICRA = (1U << ISC10) | (1U << ISC11);
    EIMSK = (1U << INT1);

    // Enable interrupts globally.
    sei();
}

/**
 * @brief Toggle LED1 on button press.
 */
ISR(INT1_vect) { LED1_TOGGLE; }
```

*(3.5 marks: 1 for `EICRA` with both `ISC1x` bits, 0.5 for `EIMSK`, 0.5 for `sei()`, 0.5 for
`DDRB`, 1 for the vector name `INT1_vect`.)*

**Which edge, and why: the rising edge**, `ISC11:ISC10 = 11`. The external pull-down holds the pin
low when idle and the press drives it high, so the low→high transition *is* the press-down.
Selecting the falling edge (`10`) would give a handler that fires on release instead — the same
program, one press out of step.
*(1 mark)*

**Why the toggle is written to `PINx`.** Writing a `1` to a `PINxn` bit toggles the corresponding
`PORTxn` bit, and bits written as `0` are left alone. It is a **pure write**: no read is involved,
so there is no window in which another interrupt's change to a different bit of `PORTB` could be
read, ignored and then overwritten. `PORTB ^= (1U << LED1);` is a read-modify-write and has exactly
that window, which is the requirement the question asked for.
*(0.5 marks)*

*D3 itself needs no register write here: it is an input out of reset, and the external pull-down
already defines the idle level. Leaving the internal pull-up off is the right choice — enabling it
would fight the divider for no benefit. Credit a candidate who says so; do not penalise one who
writes `PORTD &= ~(1U << 3U);` for explicitness.*

### (b) 4 marks

A2 is `PORTC2`, which has no external-interrupt hardware, so it needs a **pin change interrupt** on
port C:

| | |
| --- | --- |
| Enable bit in `PCICR` | `PCIE1` → `PCICR = (1U << PCIE1);` |
| Mask register | `PCMSK1` |
| Bit written into it | bit 2 → `PCMSK1 = (1U << 2U);` |
| Vector | `ISR(PCINT1_vect)` |

The bit written into `PCMSKn` is the pin's **own bit position within its port**. The global
`PCINTx` number for this pin is 10 (`PORTCx + 8`), but that number is only for looking the pin up in
the datasheet — `PCMSK1 = (1U << PCINT10)` sets the same bit as `(1U << 2U)`.

`EICRA` and `EIMSK` disappear entirely; `sei()` is still required.
*(2 marks)*

**Two things the handler must now do**, both forced rather than optional:

1. **Filter the edge itself.** Pin change interrupts have no edge-select register — there is no
   `EICRA` equivalent — and fire on **both** edges, press and release. To act only on press-down,
   the handler must read the pin's actual level and return otherwise:
   `if (PINC & (1U << BTN1)) { ... }`. The filtering has moved out of hardware and into software
   because the hardware simply does not offer it.
2. **Work out which pin changed**, if more than one PCI pin on port C is ever enabled. The vector is
   per-**port**, not per-pin, and nothing in the call says which pin fired it. The usual technique is
   to keep a previous-`PINC` byte and XOR it against the current read to find the bit that flipped.
   Again forced: one vector genuinely serves the whole port.

*(2 marks, 1 each)*

### (c) 3 marks

**The rule.** An ISR does the minimum needed to *record* that the event happened — flip a pin, set a
flag, update one small piece of state — and returns. Everything else belongs in `main()`'s loop.
The reason is that you do not control when an ISR fires, and for as long as one is running every
other interrupt on the chip is held off; a slow handler therefore delays events it has nothing to do
with.
*(1 mark)*

**Two calls that must never appear in an ISR**, any two of:

* **`serial_print()`** — blocks on `UDRE0` once per byte. At 9600 baud a 15-byte line is about
  **15.6 ms** of held-off interrupts, including the debounce timer the button itself depends on.
* **`eeprom_write()`** — blocks on `EEPE` for roughly **3.4 ms per byte**, and carries its own
  `cli()`/`sei()` pair, which inside an ISR re-enables interrupts the handler was entered with
  disabled.
* **`delay_ms()` / `_delay_ms()`** — a busy-wait, by construction; whatever duration was asked for
  is spent with everything else blocked.

*(1 mark)*

**The pattern.** *(1 mark)*

```c
/** Flags that a BTN1 event has occurred. */
static volatile bool btn1_event = false;

ISR(PCINT0_vect)
{
    if (BTN1_PRESSED) { btn1_event = true; }
}

// In main()'s loop:
if (btn1_event)
{
    serial_print("BTN1 pressed!");
    btn1_event = false;
}
```

**What `volatile` is doing:** the flag is written only inside the ISR and read only inside `main()`'s
loop, and the compiler cannot see that an interrupt exists. Without `volatile` it is entitled to
assume nothing in the loop changes `btn1_event`, load it once into a register before the loop, and
test that stale `false` on every iteration forever. `volatile` forces every read back to memory.

---

## Question 4 - Reading the world, and keeping time (13 marks)

### (a) 4 marks

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
    // Use the internal voltage reference and select the ADC channel to read.
    ADMUX = (1U << REFS0) | channel;

    // Enable the ADC, start a conversion, use a prescaler of 128.
    ADCSRA = (1U << ADEN) | (1U << ADSC) | (1U << ADPS0) | (1U << ADPS1) | (1U << ADPS2);

    // Wait for the conversion to complete.
    while (0U == (ADCSRA & (1U << ADIF))) {}

    // Restore the ADC interrupt flag for the next conversion, then return the result.
    ADCSRA = (1U << ADIF);
    return ADC;
}
```

*(2.5 marks)*

**What each write is for.** *(1 mark)*

* **`ADMUX`** — `REFS0` (with `REFS1` clear) selects `AVCC`, the chip's own 5 V supply, as the
  voltage reference; the low four bits select which input channel is connected to the converter.
  Leaving both `REFS` bits clear would select the external `AREF` pin, which nothing drives on this
  board.
* **`ADCSRA`, first write** — `ADEN` powers the converter on, `ADSC` starts a single conversion, and
  `ADPS2:0 = 111` divides the CPU clock by 128 to produce the ADC's own clock.
* **The `ADIF` poll** — a conversion is not instantaneous; the result registers are not valid until
  the hardware sets the flag.
* **`ADCSRA = (1U << ADIF);`** — `ADIF` is cleared by writing a `1` back to it, readying it for the
  next conversion.
* **`return ADC;`** — the `ADC` macro combines `ADCL` and `ADCH` into the full 10-bit value, reading
  them in the order that guarantees both halves come from the same conversion.

**The ÷128 prescaler.** *(0.5 marks)* The successive-approximation hardware needs a clock in the
region of 50-200 kHz for full 10-bit accuracy. 16 MHz ÷ 128 = **125 kHz**, comfortably inside that
window. Without any prescaler bits, `ADPS2:0 = 000` selects a division factor of 2, i.e. 8 MHz —
about forty times too fast. The sample-and-hold and the comparator never settle, so the low bits of
every result are noise: readings that look plausible, are not repeatable, and get quietly worse as
the source impedance rises.

### (b) 5 marks

**1. Tick and overflow periods.** *(1 mark)*

```text
tick period = 1024 / 16,000,000 s = 64 us = 0.064 ms
Timer 0 overflow = 256 ticks = 16.384 ms
```

**2. Timer 1, CTC, 250 ms.** *(2 marks)*

```text
ticks needed = 250 / 0.064 = 3906.25  ->  3906
OCR1A counts 0 up to its value, so reaching it takes (OCR1A + 1) ticks:
OCR1A = 3906 - 1 = 3905
check: 3906 * 0.064 = 249.984 ms
```

```c
TCCR1B = (1U << CS10) | (1U << CS12) | (1U << WGM12); // Prescaler 1024, CTC mode.
OCR1A  = 3905U;                                       // 250 ms period.
TIMSK1 = (1U << OCIE1A);                              // Enable the compare-match interrupt.
```

Vector: `ISR(TIMER1_COMPA_vect)`. `sei()` is still needed.

**3. Timer 0, Normal Mode, 250 ms.** *(1 mark)*

```text
250 / 16.384 = 15.259  ->  15 overflows
15 * 16.384 = 245.76 ms
error = (245.76 - 250) / 250 = -1.70 %
```

*(A candidate who rounds up to 16 overflows — 262.144 ms, +4.9 % — has applied a defensible rule and
should lose at most half a mark, provided the error is computed for the number actually chosen.)*

**4. Why Timer 0 cannot reach 250 ms directly.** *(1 mark)*

Timer 0 is 8-bit: its counter and its compare register both stop at 255. At its slowest available
prescaler, ÷1024, one full count is 256 × 0.064 = **16.384 ms**, and there is no larger divisor on
that timer. 250 ms is over fifteen times that, so no combination of prescaler and compare value
reaches it in a single hardware period — the overflows have to be counted in software. Timer 1's
16-bit range is exactly what removes the need, which is why CTC on Timer 1 hits the period directly.

### (c) 4 marks

```c
/**
 * @brief Run a single PWM cycle.
 */
static void run_pwm_cycle(void)
{
    // Read the ADC and compute the duty cycle.
    const uint16_t adc_val = adc_read(POT1);
    const float duty_cycle = (float)(adc_val) / ADC_MAX;

    // Compute the PWM durations from the duty cycle, rounded to the nearest millisecond.
    const uint8_t on_ms  = (uint8_t)(duty_cycle * PWM_PERIOD_MS + 0.5F);
    const uint8_t off_ms = PWM_PERIOD_MS - on_ms;

    // Enable LED1 for the active duration, disable it for the rest of the period.
    LED1_ON;
    delay_ms(on_ms);
    LED1_OFF;
    delay_ms(off_ms);
}
```

*(2 marks. The cast on the numerator is load-bearing: without it, `adc_val / ADC_MAX` is integer
division and yields 0 for every reading below 1023. A candidate who writes
`(adc_val * PWM_PERIOD_MS) / ADC_MAX` in integer arithmetic instead is also correct, and better on
this target, provided they note that the product must not overflow 16 bits.)*

**Why `+ 0.5F`.** *(1 mark)* The cast to `uint8_t` truncates towards zero. Adding a half before it
turns truncation into round-to-nearest. Without it, a duty cycle of 0.99 gives `9.9` → `9`, so the
LED never reaches full brightness and the entire scale is biased low by up to one millisecond in
ten — a systematic error, not a rounding one.

**What it costs, and what replaces it.** *(1 mark)* `delay_ms()` is built on `_delay_ms()`, which
busy-waits. The routine therefore blocks for the whole 10 ms period, every period, so the CPU is
doing nothing else for 100 % of the time — no button polling, no other work — and only interrupts
still get through. L05's timers replace it: a hardware timer counts on its own, raises an interrupt,
and the handler counts overflows and drives the pin, leaving `main()`'s loop free. (The chip also
has dedicated PWM output hardware; the course builds the equivalent out of the timer's
overflow-counting pattern instead.)

---

## Question 5 - A channel to the host (13 marks)

### (a) 5 marks

```c
/**
 * @file UART driver.
 */
#ifndef DRIVER_SERIAL_H_
#define DRIVER_SERIAL_H_

#include <stdint.h>

/**
 * @brief Initialize UART with a 9600 bps baud rate.
 */
void serial_init(void);

/**
 * @brief Transmit text over UART.
 *
 * @param[in] text Text to transmit. Ignored if NULL.
 */
void serial_print(const char* text);

/**
 * @brief Transmit a buffer of arbitrary bytes over UART.
 *
 * @param[in] data Buffer to transmit. Ignored if NULL.
 * @param[in] size Number of bytes to transmit.
 */
void serial_write(const uint8_t* data, uint16_t size);

#endif /* DRIVER_SERIAL_H_ */
```

*(3 marks: 1 for the guard and its naming, 1 for the three prototypes with correct types, 1 for
Doxygen in the imperative mood documenting every parameter.)*

**Which functions belong where.** *(1 mark)* The three above are the module's public API and must
have external linkage: `main.c` calls them and the linker has to resolve them. `serial_write_byte()`
is not among them — it is an implementation detail that `serial_print()` and `serial_write()` are
both built on, and no caller has any business poking a single byte into `UDR0` behind the driver's
back. It should be `static` in `serial.c`, which keeps it out of the symbol table, prevents a name
collision with anything else in the program, and lets the compiler inline it into its two call sites.

**Why the `const` differs between prototype and definition.** *(1 mark)* A **top-level** `const` on a
pass-by-value parameter — `uint16_t size` — constrains only what the function body may do with its
own local copy. It is not part of the function's type and cannot affect any caller, so it carries no
information in the prototype and is omitted there; in the definition it says something real about
the body, so it is kept.

`const char* text` is **not** that case and must not be confused with it. That `const` qualifies the
*pointee*, not the parameter: it is part of the type, it is a genuine promise to the caller that the
function will not write through the pointer, and it belongs in both files. Dropping it from the
prototype would change what the function accepts.

### (b) 4 marks

```c
/**
 * @brief Transmit a single byte over UART, blocking until the data register is free.
 *
 * @param[in] byte Byte to transmit.
 */
static void serial_write_byte(const uint8_t byte)
{
    // Wait until the transmit buffer is ready for a new byte.
    while (0U == (UCSR0A & (1U << UDRE0))) {}

    // Place the new byte in the transmit data register.
    UDR0 = byte;
}

/**
 * @brief Transmit text over UART.
 *
 * @param[in] text Text to transmit.
 */
void serial_print(const char* text)
{
    // Terminate if the text pointer is NULL.
    if (NULL == text) { return; }

    // Transmit each character one by one.
    for (uint16_t i = 0U; text[i]; ++i)
    {
        serial_write_byte((uint8_t)(text[i]));
    }
    serial_write_byte('\r');
    serial_write_byte('\n');
}
```

*(2 marks: 1 each, with the loop terminating on the string's `'\0'` rather than a fixed length, and
the `NULL` check present because this is an exported function.)*

**What `UDRE0` signals.** *(0.5 marks)* USART Data Register Empty: it is set when `UDR0` is free to
accept a new byte, which happens as soon as the previous byte has been moved out into the shift
register — not when transmission has finished.

**Why the wait comes first.** *(0.5 marks)* The flag describes whether the register is ready *now*,
so it has to be consulted before the write it guards. Waiting afterwards means the very first byte
is written unconditionally, into a register that may still hold an untransmitted byte, and from then
on each wait is one step out of phase with the write it was supposed to protect. The order is not a
stylistic choice.

**`sprintf()`.** *(1 mark)* It formats into a caller-supplied buffer exactly the way `printf()`
formats to a terminal, so numbers can be mixed into text and the result handed to `serial_print()`
as an ordinary string — the whole reason the UART is useful for anything beyond fixed messages.

**The caveat:** AVR's default `sprintf()` frequently has floating-point formatting disabled, or
available only by linking a larger variant that costs flash the device may not have. `%f` then
prints nothing useful with no diagnostic. Prefer integers — scale a reading to hundredths and print
the two parts separately — unless float support has been confirmed in the toolchain.

### (c) 4 marks

**Where the calls go: both in `main()`'s loop.** *(1 mark)*

They cannot go where the event is detected — inside the pin-change ISR or inside the one-second
timer's ISR — because `serial_print()` blocks byte-by-byte on `UDRE0`. A short line at 9600 baud is
15-20 ms with every other interrupt on the chip held off, including the debounce timer the button
depends on, so presses arriving during a report would simply be lost. `sprintf()` is not much
better: it is comparatively slow and takes a fair amount of stack, neither of which belongs in a
handler that can fire at any moment.

**The pattern.** *(2 marks)*

```c
/** Flags that a report is due. */
static volatile bool report_due = false;

/** Flags that a BTN1 event has occurred. */
static volatile bool btn1_event = false;

ISR(TIMER1_COMPA_vect) { report_due = true; }                  // Fires once a second.
ISR(PCINT0_vect)       { if (BTN1_PRESSED) { btn1_event = true; } }

int main(void)
{
    setup();

    while (1)
    {
        // Report the current ADC reading once a second.
        if (report_due)
        {
            char msg[MSG_LEN] = {'\0'};
            sprintf(msg, "ADC: %u", adc_read(POT1));
            serial_print(msg);
            report_due = false;
        }

        // Handle a button press, if one occurred.
        if (btn1_event)
        {
            // ...
            btn1_event = false;
        }
    }
    return 0;
}
```

**What `volatile` is doing:** each flag is written only inside an ISR and read only inside `main()`'s
loop. The compiler cannot see that interrupts exist, so without `volatile` it is entitled to load
each flag once into a register before the loop and test that stale `false` forever. The program then
compiles cleanly, works at `-O0`, and stops responding the moment optimisation is turned up.

**`read()`'s `timeout_ms`, and its return values.** *(1 mark)*

Without a timeout, a receive blocks until the requested number of bytes has arrived. A sender that
goes quiet — unplugged, crashed, or simply not talking yet — hangs the entire program, and with a
watchdog armed the chip reboots instead of continuing. `timeout_ms` bounds the wait: non-zero gives
up after roughly that many milliseconds, pacing itself one millisecond at a time; `0` deliberately
keeps the old block-until-done behaviour for callers that want it.

The return value distinguishes three outcomes:

* **`-1`** — a bad call: a `NULL` buffer, or `buflen == 0`.
* **a count below `buflen`** — the timeout elapsed first; that many bytes did arrive.
* **`buflen`** — the buffer filled completely.

---

## Question 6 - Surviving a power cut, and a hang (13 marks)

### (a) 5 marks

```c
/**
 * @brief Write a byte to EEPROM at the given address.
 *
 * @param[in] addr EEPROM address to write to (0-1023).
 * @param[in] byte Byte to write.
 */
static void eeprom_write_byte(const uint16_t addr, const uint8_t byte)
{
    // Wait for any earlier write to finish.
    while (EECR & (1U << EEPE)) {}

    // Set destination address and data.
    EEAR = addr;
    EEDR = byte;

    // Perform write operation.
    cli();
    EECR |= (1U << EEMPE);
    EECR |= (1U << EEPE);
    sei();
}

/**
 * @brief Read a byte from EEPROM at the given address.
 *
 * @param[in] addr EEPROM address to read from (0-1023).
 *
 * @return Byte stored at that address.
 */
static uint8_t eeprom_read_byte(const uint16_t addr)
{
    // Wait for any earlier write to finish.
    while (EECR & (1U << EEPE)) {}

    // Set the address to read from.
    EEAR = addr;

    // Start the read operation.
    EECR |= (1U << EERE);

    // Return the byte stored at the given address.
    return EEDR;
}
```

*(2 marks, 1 each.)*

**The loop on `EEPE` at the top.** *(1 mark)* `EEPE` stays set for the several milliseconds an
EEPROM write takes. Loading a new address and data while a write is still in progress either loses
the new write or disturbs the one already under way, so the function's first act is to establish
that the peripheral is free. It is the same "wait for the peripheral" pattern as `ADIF` in L04 and
`UDRE0` in L06.

**The order of `EEMPE` and `EEPE`.** *(1 mark)* `EEMPE` is not a data bit; it is a **safety
interlock** that arms the write, and `EEPE` must follow it within four CPU cycles. In the other
order, `EEPE` is set with nothing armed and does nothing at all, and the subsequent `EEMPE` arms an
interlock that is never used — a function that silently writes nothing.

**The `cli()`/`sei()` pair.** *(0.5 marks)* Four CPU cycles is a handful of instructions. An
interrupt taken between the two writes pushes `EEPE` outside the window and the write silently does
not happen. Interrupts are therefore disabled for exactly those two instructions and re-enabled
immediately after — the shortest possible critical section.

**Why the read needs no equivalent.** *(0.5 marks)* There is no timed sequence on the read side.
`EERE` starts the read on its own: no interlock bit to satisfy, no four-cycle deadline, and
therefore nothing for an interrupt to break. The read still waits on `EEPE` first, but for the other
reason — the peripheral must be free before a new address is loaded.

### (b) 4 marks

```c
/**
 * @brief Enumeration of watchdog timeouts.
 */
typedef enum
{
    WATCHDOG_TIMEOUT_16MS,        ///< 16 ms timeout.
    WATCHDOG_TIMEOUT_32MS,        ///< 32 ms timeout.
    WATCHDOG_TIMEOUT_64MS,        ///< 64 ms timeout.
    WATCHDOG_TIMEOUT_128MS,       ///< 128 ms timeout.
    WATCHDOG_TIMEOUT_256MS,       ///< 256 ms timeout.
    WATCHDOG_TIMEOUT_512MS,       ///< 512 ms timeout.
    WATCHDOG_TIMEOUT_1024MS,      ///< 1024 ms timeout.
    WATCHDOG_TIMEOUT_2048MS,      ///< 2048 ms timeout.
    WATCHDOG_TIMEOUT_4096MS = 32, ///< 4096 ms timeout.
    WATCHDOG_TIMEOUT_8192MS = 33, ///< 8192 ms timeout.
} watchdog_timeout_t;

/**
 * @brief Enable the watchdog timer with the given timeout.
 *
 * @param[in] timeout Timeout to arm the watchdog with.
 */
static void watchdog_init(const watchdog_timeout_t timeout)
{
    cli();
    WDTCSR |= (1U << WDCE) | (1U << WDE);      // Unlock the timed sequence.
    WDTCSR = (1U << WDE) | (uint8_t)(timeout); // Enable the watchdog with the given timeout.
    sei();
}
```

*(2 marks: 1 for the enum, 1 for the function with its unlock and its `cli()`/`sei()`.)*

**Why eight members can default and two cannot.** *(1 mark)* The enumerator's value is written
straight into `WDTCSR`'s prescaler bits, so it has to be the register encoding, not an index.
`WDP2`-`WDP0` occupy contiguous bits 2-0 of `WDTCSR`, so the first eight timeouts, 16 ms to 2048 ms,
encode as exactly 0-7 — which is what an `enum` counting up in declaration order already produces
for free. `WDP3`, needed for 4096 ms and 8192 ms, does **not** sit next to them: it lives at bit 5.
Those two settings therefore encode as `1 << 5` = **32** and `(1 << 5) | (1 << 0)` = **33**, values
no automatic counting would ever produce, so they must be spelled out. The payoff is that a
`watchdog_timeout_t` can be cast straight to `uint8_t` and written to the register with every bit
already in the right place.

*The register reference gives `WDTCSR`'s bit positions, so 32 and 33 are meant to be **derived**
here, not recalled. A candidate who reproduces the two numbers without being able to say where they
come from has copied the appendix; one who derives them from `WDP3` sitting at bit 5 has understood
the encoding, and should be credited even if they misremember which timeout is which.*

**What the `WDCE` write is for.** *(0.5 marks)* It is the unlock half of a timed sequence, exactly
like EEPROM's `EEMPE`, but for a sharper reason: it exists so that runaway code cannot accidentally
switch off the one mechanism meant to catch it. Setting `WDCE` and `WDE` together opens a four-cycle
window in which `WDE` and the prescaler bits may actually be changed.

**If the second write comes too late.** *(0.5 marks)* The hardware closes the window — clearing
`WDCE` after four cycles — and the write is simply ignored. There is no error and no diagnostic; the
watchdog keeps whatever configuration it had, which out of reset means disabled. A program that gets
this wrong looks exactly like a program with no watchdog in it, which is why the `cli()`/`sei()`
pair around the sequence is not optional.

### (c) 4 marks

*(0.5 marks per value, plus 0.5 for quantifying case 2; 1.5 marks for the watchdog part.)*

**1. Calibration offset written once at manufacture — yes, this is what EEPROM is for.** One write
in the life of the device, a value that must survive power loss, and no other place to keep it.
Endurance and write time are both irrelevant at a rate of one.

**2. Live ADC reading sampled at 100 Hz — no, badly wrong, for two independent reasons.**

* **Endurance.** A cell tolerates roughly 100,000 write cycles. At 100 writes a second that is
  `100,000 / 100 = 1000` seconds — **under 17 minutes** before that address stops reliably holding
  data. Permanently.
* **Time.** Each write blocks for about 3.4 ms, so 100 writes a second means 340 ms out of every
  second spent inside the write loop: a third of the CPU, doing nothing, forever.

And the value does not need to survive a reset in the first place — a live reading is by definition
current, and belongs in RAM.

**3. On/off state toggled by a user button — yes, with one condition.** It must be written *when it
changes*, not once per iteration of `main()`'s loop. A human presses a button a few times a second
at most and a few thousand times over the device's life, comfortably inside the endurance budget,
and surviving a reset is the entire point of storing it. This is exactly the course's own demo.

**4. Boot counter incremented on every reset — yes, with a caveat worth knowing.** Resets are rare,
so the write rate is fine. But the endurance ceiling now translates directly into a device lifetime
of about **100,000 reboots**, after which the counter's address is worn out. If that number is ever
likely to be approached, spread the count across several addresses rather than hammering one.

**Where the watchdog reset belongs.** *(1 mark)* As the **first** statement of `main()`'s loop, on
every iteration, unconditionally — not inside an `if`, and not inside a subroutine that a hang could
skip. Petting it only when work happened defeats the purpose: a loop spinning without making
progress would still be petting it happily.

**How to choose the timeout.** *(0.5 marks)* Comfortably longer than the slowest *normal* iteration
of the loop it is protecting, worst case included — an EEPROM write of several milliseconds, a full
serial message of tens, the longest burst of interrupts — with margin on top, so a healthy program
never trips it. But not so long that a genuine hang goes unnoticed for an unacceptable time. The
course's 1024 ms against a loop that normally completes in microseconds is generous on the first
count and still fast enough on the second.

---

## Question 7 - A driver that scales (12 marks)

### (a) 6 marks

```c
/**
 * @file GPIO driver.
 */
#ifndef DRIVER_GPIO_H_
#define DRIVER_GPIO_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief GPIO driver structure.
 */
typedef struct
{
    /** Pointer to the data direction register associated with the GPIO. */
    volatile uint8_t* ddrx;

    /** Pointer to the output register associated with the GPIO. */
    volatile uint8_t* portx;

    /** Pointer to the input register associated with the GPIO. */
    volatile uint8_t* pinx;

    /** Bit position of the GPIO pin within the associated port. */
    uint8_t pin;
} gpio_t;

/**
 * @brief Enumeration of GPIO modes.
 */
typedef enum
{
    GPIO_MODE_INPUT,        ///< Standard GPIO input.
    GPIO_MODE_INPUT_PULLUP, ///< GPIO input with its internal pull-up enabled.
    GPIO_MODE_OUTPUT,       ///< GPIO output.
} gpio_mode_t;

/**
 * @brief Initialize GPIO.
 *
 * @param[in] self GPIO instance.
 * @param[in] pin  Digital pin to use. Must be in range [0, 13].
 * @param[in] mode GPIO mode.
 *
 * @return True on success, false if self is NULL or the pin is out of range.
 */
bool gpio_init(gpio_t* self, uint8_t pin, gpio_mode_t mode);

/**
 * @brief Set state of the given GPIO.
 *
 * @param[in] self  GPIO instance.
 * @param[in] state GPIO state (true = high, false = low).
 *
 * @note This operation is only supported for GPIOs configured as outputs.
 */
void gpio_write(gpio_t* self, bool state);

/**
 * @brief Read state of the given GPIO.
 *
 * @param[in] self GPIO instance.
 *
 * @return GPIO state (true = high, false = low).
 */
bool gpio_read(const gpio_t* self);

/**
 * @brief Toggle state of the given GPIO.
 *
 * @param[in] self GPIO instance.
 */
void gpio_toggle(gpio_t* self);

#endif /* DRIVER_GPIO_H_ */
```

*(3 marks: 1 for the guard, the includes and the struct; 1 for `gpio_mode_t` and the prototypes with
`const gpio_t*` on the reader and plain `gpio_t*` elsewhere; 1 for Doxygen on every declaration,
with the top-level `const` correctly absent from the prototypes.)*

```c
#define PIN_OFFSET_B 8U  // Pin offset for I/O port B.
#define GPIO_PIN_MAX 13U // Maximum GPIO pin number.

// -----------------------------------------------------------------------------
bool gpio_init(gpio_t* self, const uint8_t pin, const gpio_mode_t mode)
{
    // Check the GPIO instance, return false if invalid.
    if (NULL == self) { return false; }

    // Check the pin number, return false if out of range.
    if (GPIO_PIN_MAX < pin) { return false; }

    // Pins 0 - 7 => I/O port D, bit position = pin number.
    if (PIN_OFFSET_B > pin)
    {
        self->ddrx  = &DDRD;
        self->portx = &PORTD;
        self->pinx  = &PIND;
        self->pin   = pin;
    }
    // Pins 8 - 13 => I/O port B, bit position = pin number - 8.
    else
    {
        self->ddrx  = &DDRB;
        self->portx = &PORTB;
        self->pinx  = &PINB;
        self->pin   = pin - PIN_OFFSET_B;
    }

    // Configure GPIO mode as specified.
    switch (mode)
    {
        case GPIO_MODE_INPUT_PULLUP:
            // Configure GPIO as input with its internal pull-up enabled.
            *self->ddrx &= ~(1U << self->pin);
            *self->portx |= (1U << self->pin);
            break;
        case GPIO_MODE_OUTPUT:
            // Configure GPIO as output.
            *self->ddrx |= (1U << self->pin);
            *self->portx &= ~(1U << self->pin);
            break;
        default:
            // Configure GPIO as standard input.
            *self->ddrx &= ~(1U << self->pin);
            *self->portx &= ~(1U << self->pin);
            break;
    }
    return true;
}
```

*(3 marks: 1 for both guard clauses coming **before** any register write, so an out-of-range pin
touches nothing, as the question required; 1 for the port mapping and the `- 8` on port B; 1 for the
three modes with `|=`/`&= ~` rather than plain assignment, which would deconfigure every other pin
on the port.)*

### (b) 3 marks

**Why `pin` can no longer be `const`.** *(1 mark)* A `const` struct member can only be given a value
by an **initializer at the point the object is declared**; it can never be assigned afterwards. In
the original driver that was exactly how it arrived — `gpio_t led1 = {&DDRB, &PORTB, &PINB, 1U};` —
so `const` cost nothing. In this version `gpio_init()` derives the bit position itself and writes
`self->pin = pin - PIN_OFFSET_B;` to an object that already exists, which a `const` member forbids.
The compiler rejects it outright: assignment of read-only member.

**What a `const` member permits and forbids.** *(0.5 marks)* It permits initialisation at
declaration and reading thereafter. It forbids assignment through any route, including whole-struct
assignment `*self = other;`, which is why a struct with a `const` member is also awkward to keep in
an array that is filled in a loop.

**Why the two functions differ in their parameter.** *(1 mark)* `gpio_read()` only ever reads
through `self`, so `const gpio_t*` documents that and lets the compiler reject an accidental write.
`gpio_write()` writes to the hardware through `self`'s pointers — and in any version that caches
state would write to `self`'s own fields — so it cannot make that promise.

**If either took `gpio_t self` by value.** *(0.5 marks)* Every call would copy the whole struct —
three pointers plus a byte, on a target with 2 KB of RAM — for no benefit. `gpio_read()` would still
*work*, because it reads through the copied pointers and those point at the same registers.
`gpio_init()` would not work at all: its entire job is to fill in the caller's instance, and every
field it set would be discarded at the closing brace, leaving the caller's `gpio_t` exactly as
uninitialised as before, with no diagnostic.

### (c) 3 marks

```c
/* In gpio.c, at file scope: */

/** Pin reservation table; bit n is set while pin n is claimed. */
static uint16_t pin_reg = 0U;
```

In `gpio_init()`, after both guard clauses and before any register is touched:

```c
    // Check whether the pin is already reserved, return false if so.
    if (pin_reg & (1U << pin)) { return false; }

    // Reserve the pin.
    pin_reg |= (1U << pin);
```

Releasing needs the Arduino pin number, which the instance no longer holds — so `gpio_t` gains one
more field, set alongside the rest in `gpio_init()`:

```c
    /** Digital pin number the GPIO was initialized with, 0-13. */
    uint8_t pin_id;
```

```c
/**
 * @brief Release a GPIO and free its reserved pin.
 *
 * @param[in] self GPIO instance.
 */
void gpio_free(gpio_t* self)
{
    // Check the GPIO instance, terminate if invalid.
    if (NULL == self) { return; }

    // Return the pin to its power-on state: input, pull-up disabled.
    *self->ddrx &= ~(1U << self->pin);
    *self->portx &= ~(1U << self->pin);

    // Release the pin so a later gpio_init() call can claim it again.
    pin_reg &= ~(1U << self->pin_id);
}
```

*(2 marks. The extra field is the trap: `self->pin` is the **bit position within the port**, not the
Arduino pin number, once `gpio_init()` has subtracted the offset — clearing `pin_reg` bit
`self->pin` would free pin 1 when the caller asked to free pin 9. Either store the original number,
as above, or reconstruct it with `self->pin + PIN_OFFSET_B` when `self->portx == &PORTB`. A
candidate who spots this has read their own code; award the full 2 marks without it only if
everything else is complete.)*

**Why the table must be `static`.** *(1 mark)* At file scope `static` means **internal linkage**:
the name is not emitted for the linker and no other translation unit can reach it. That makes the
table the driver's private state, which is the whole point — it exists to enforce an invariant, and
an invariant that any file can quietly edit is not one.

Without `static`, the symbol is exported and two things break. Any other file can write
`extern uint16_t pin_reg;` and set or clear bits behind the driver's back, handing out the same pin
twice or leaking a reservation. And `pin_reg` is not an unlikely name: a second global of the same
name anywhere in the program collides at link time, giving either a multiple-definition error or —
under the older common-symbol behaviour some toolchains still allow — one silently shared variable
serving two unrelated modules.

*Note that `static` here is **not** about lifetime. A file-scope variable already lives for the whole
program either way; the qualifier is doing only the linkage job. That is the distinction between its
two roles, and a candidate who states it has understood the keyword rather than memorised the rule.*

---

## Question 8 - Hiding the implementation (13 marks)

### (a) 4 marks

```c
/**
 * @file GPIO driver.
 */
#ifndef DRIVER_GPIO_H_
#define DRIVER_GPIO_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Enumeration of GPIO modes.
 */
typedef enum
{
    GPIO_MODE_INPUT,        ///< Standard GPIO input.
    GPIO_MODE_INPUT_PULLUP, ///< GPIO input with its internal pull-up enabled.
    GPIO_MODE_OUTPUT,       ///< GPIO output.
} gpio_mode_t;

/** GPIO driver structure. Incomplete type: the fields are not visible here. */
typedef struct gpio gpio_t;

/**
 * @brief Allocate and configure a new GPIO.
 *
 * @return Pointer to the new GPIO, or NULL if allocation failed.
 */
gpio_t* gpio_create(volatile uint8_t* ddrx, volatile uint8_t* portx, volatile uint8_t* pinx,
                    uint8_t pin, gpio_mode_t mode);

/**
 * @brief Release a GPIO and null out the caller's pointer.
 */
void gpio_delete(gpio_t** self);

void gpio_write(gpio_t* self, bool state);
bool gpio_read(const gpio_t* self);
void gpio_toggle(gpio_t* self);

#endif /* DRIVER_GPIO_H_ */
```

The full field list is replaced by the single line `typedef struct gpio gpio_t;`, and
`struct gpio { ... };` moves into `gpio.c` — the only file that ever sees the layout.
*(2 marks)*

**The three things it takes away, and what the caller does instead.** *(2 marks)*

1. **Declaring an instance.** `gpio_t led1 = {...};` needs the size and layout, and an incomplete
   type has neither. The caller instead calls `gpio_create(...)`, holds the returned pointer, and
   checks it against `NULL`.
2. **Reading or writing a field.** `led1.pin` and `led1.ddrx` no longer compile anywhere outside
   `gpio.c`. The caller goes through the driver's functions — which is the entire point: the code
   that could break the invariant is now the only code that can reach it.
3. **Owning the storage.** The caller no longer chooses where the object lives — stack, static
   array, member of a larger struct. The driver allocates it, and with that the caller inherits an
   obligation it did not have before: releasing it with `gpio_delete()`, and coping with
   `gpio_create()` returning `NULL` when the heap is exhausted.

### (b) 4 marks

```c
/**
 * @brief Button structure.
 */
typedef struct
{
    /** GPIO associated with the button. */
    gpio_t* pin;

    /** Handle press events. Called from main(), never from an ISR. */
    void (*on_press)(void);
} button_t;

/**
 * @brief Check for a button press event, invoke its callback if one occurred.
 *
 * @param[in] self Button instance.
 * @param[in] edge True if a press was just detected (e.g. from a flag an ISR set).
 */
static void button_check(const button_t* self, const bool edge)
{
    const bool valid = (NULL != self) && (NULL != self->on_press);
    if (valid && edge) { self->on_press(); }
}
```

*(2 marks: 1 for the struct, 1 for `button_check()` checking both `self` and the callback before
calling through it — a null function pointer called is a jump to address 0, not a diagnostic.)*

**What `void (*on_press)(void);` declares.** *(1 mark)* A member named `on_press` that is a
**pointer to a function taking no arguments and returning `void`**. The declaration reads
inside-out, and the parentheses around `*on_press` are required: `void *on_press(void);` would
instead declare a member that *is* a function returning `void*`, which is not something a struct can
contain.

```c
btn1.on_press = toggle_led;   // A function name without () decays to its address; no & needed.
```

```c
self->on_press();             // Calling through the pointer, written exactly like a normal call.
```

*(Credit a candidate who cleans the syntax up with `typedef void (*button_callback_t)(void);` and
declares the member as `button_callback_t on_press;` — provided they can still say what the
underlying declaration means.)*

**Why `button_check()` is better for never naming the LED.** *(1 mark)* It depends only on the
callback's **signature**, not on what the callback does. The same `button_check()` therefore serves
a button that toggles an LED, one that sends a UART message, one that starts a timer and one that
does nothing at all, with no edit and no recompilation of the driver. The caller decides the
behaviour; the driver stays a driver, and it can be tested without an LED existing. That is the
whole of what a callback buys, and it is the hand-written version of what a virtual function does in
a language that has them.

### (c) 5 marks

```c
/**
 * @brief In-memory GPIO stub; satisfies gpio_interface_t with no hardware at all.
 */
typedef struct gpio_stub
{
    /** Must be the first member. */
    gpio_interface_t itf;

    /** Simulated pin state (true = high, false = low). */
    bool state;
} gpio_stub_t;

// -----------------------------------------------------------------------------
static inline gpio_stub_t* get_impl(gpio_interface_t* self) { return (gpio_stub_t*)(self); }

// -----------------------------------------------------------------------------
static void gpio_del(gpio_interface_t** self)
{
    if ((NULL == self) || (NULL == *self)) { return; }
    free(*self);
    *self = NULL;
}

// -----------------------------------------------------------------------------
static bool gpio_read(const gpio_interface_t* self)
{
    const gpio_stub_t* impl = (const gpio_stub_t*)(self);
    return (NULL != impl) ? impl->state : false;
}

// -----------------------------------------------------------------------------
static void gpio_write(gpio_interface_t* self, const bool value)
{
    gpio_stub_t* impl = get_impl(self);
    if (NULL == impl) { return; }
    impl->state = value;
}

// -----------------------------------------------------------------------------
static void gpio_toggle(gpio_interface_t* self)
{
    gpio_stub_t* impl = get_impl(self);
    if (NULL == impl) { return; }
    impl->state = !impl->state;
}

// -----------------------------------------------------------------------------
static const gpio_vtable_t* gpio_vptr_get_instance(void)
{
    static const gpio_vtable_t vtable = {
        .del = gpio_del, .read = gpio_read, .write = gpio_write, .toggle = gpio_toggle,
    };
    return &vtable;
}

// -----------------------------------------------------------------------------
gpio_interface_t* gpio_stub_new(void)
{
    gpio_stub_t* impl = (gpio_stub_t*)(malloc(sizeof(gpio_stub_t)));
    if (NULL == impl) { return NULL; }

    impl->itf.vptr = gpio_vptr_get_instance();
    impl->state    = false;
    return (gpio_interface_t*)(impl);
}
```

*(3 marks: 1 for the struct with `itf` first and `state` standing in for the registers; 1 for the
four functions casting `self` back to the concrete type and being `static`; 1 for the vtable and
`gpio_stub_new()` setting `vptr` before returning, with the `malloc()` failure handled.)*

**The rule about field order, and the guarantee behind it.** *(1 mark)*
`gpio_interface_t itf;` must be the **first** member. C guarantees that a struct has no padding
before its first member, so a pointer to the struct and a pointer to that member hold the same
address. That is exactly what makes `(gpio_interface_t*)(impl)` in `gpio_stub_new()` and the reverse
cast in `get_impl()` correct with no pointer adjustment — and it is the same layout a C++ compiler
generates for single inheritance, where an object's address *is* its base class's address. Put `itf`
anywhere else and the cast silently yields a pointer to the wrong field; `self->vptr` then reads
whatever happens to be at offset 0 and the first call through it jumps to a garbage address.

**Why a single shared vtable is enough.** *(0.5 marks)* The table holds four function addresses and
nothing else — there is nothing instance-specific in it, because all per-instance state lives in
`gpio_stub_t`'s own fields, which each function reaches by casting `self` back. One
`static const gpio_vtable_t` inside the accessor therefore serves every stub ever created: each
instance carries one pointer rather than four copies of the table, and `const` lets the table live
in flash rather than RAM.

**What the stub makes possible.** *(0.5 marks)* Everything written against `gpio_interface_t*` can
now be built and run with no chip attached: unit tests on a host, logic driven by simulated inputs
that would otherwise need a soldering iron, and a third implementation — one that logs every
`write()` and `toggle()` before updating its own `state` — dropped in without touching a line of the
code under test. `main()`'s edge-detection logic cannot tell which implementation it was handed,
which is the whole point of programming against the interface rather than against the driver.

---
