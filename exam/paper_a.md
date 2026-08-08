# Embedded C - Written Examination, Paper A

**Time:** 3 hours. **Closed book.** No compiler and no datasheet. The register reference at the back
of this paper is the only material you may use. **Total: 100 marks.**

This paper exists to test your own skills and knowledge. It is not a qualification and it gates
nothing. It draws on all ten lectures, so **it is meant to be taken once the course is over**.

---

## Rubric

**C11 on an ATmega328P (Arduino Uno/Nano) at 16 MHz** is assumed throughout, built with `avr-gcc`
and `-std=c11`. Standard and AVR headers (`<stdint.h>`, `<stdbool.h>`, `<avr/io.h>`,
`<avr/interrupt.h>`, `<util/delay.h>`) may be assumed included unless a question asks you to name
them.

**Register and bit names are not memory-tested.** Every register and bit this paper refers to is
listed in the [register reference](#register-reference) at the back, with what it is; consult it
freely. What is marked is knowing which register to reach for and *when* — the ordering rules,
timing rules and formulas that govern their use are deliberately left out of it, because those are
the parts being examined. Misspelling a bit costs nothing; reaching for the wrong register, or the
right one at the wrong moment, costs everything.

**Code is marked on semantics, not on syntax.** A missing semicolon, a forgotten `#include` or a
brace in the wrong column costs nothing. A missing `volatile` costs everything.

**Where a question says "in the course's style"**, it means the conventions used in the appendices:

* `snake_case` for variables, functions and constants; `UPPER_CASE` for macros and enum constants.
* `static` on anything that is not part of a module's public API.
* A header guard named after the file in `UPPER_CASE`, `.` replaced by `_`, trailing `_`
  (`gpio.h` → `DRIVER_GPIO_H_`).
* A Doxygen comment in the imperative mood above every non-trivial function.
* `self` as the first parameter of any function operating on a driver struct, always a pointer, and
  `const` when the function only reads through it.
* Yoda notation on comparisons against a constant, and braces on every `if`, even one-liners.
* `const` on pass-by-value parameters in a function *definition*, dropped from the prototype.
* Fixed-width types from `<stdint.h>`, and `U` suffixes on unsigned literals.
* `volatile` on anything shared between an ISR and the rest of the program.

**Where a question says "state the consequence"**, naming the defect earns half the marks and
saying what it does to the running program earns the other half. "This is wrong" scores nothing.

**Where a question asks you to find defects**, the number of defects is stated. Listing more than
the stated number is not penalised, but only the stated number is marked, so put your strongest
answers first.

### Marks

| Question | 1   | 2   | 3   | 4   | 5   | 6   | 7   | 8   |
| -------- | --- | --- | --- | --- | --- | --- | --- | --- |
| Marks    | 12  | 12  | 12  | 13  | 13  | 13  | 12  | 13  |

---

## Question 1 - The language underneath (12 marks)

**(a)** A duty-cycle helper is defined as follows.

```c
#define PWM_PERIOD_MS 10U
#define DUTY_PERCENT(on_ms) (on_ms * 100U / PWM_PERIOD_MS)

const uint16_t duty = DUTY_PERCENT(2U + 3U);
```

Write the exact text the preprocessor substitutes, give the value `duty` ends up with, give the
value the author intended, and give the corrected macro definition. Then name one hazard a
function-like macro has that no correct parenthesisation can remove. (3 marks)

**(b)** The program below allocates a receive buffer and releases it again.

```c
static void release(uint8_t* buf)
{
    free(buf);
    buf = NULL;
}

int main(void)
{
    uint8_t* data = (uint8_t*)(malloc(BUF_LEN));
    if (NULL == data) { return 1; }

    release(data);

    if (NULL != data) { data[0U] = 1U; }
    return 0;
}
```

State what `data` holds after `release()` returns and why, name what the pointer has become, and
state what the write on the following line is. Then give the corrected signature, body and call
site. (3 marks)

**(c)** The keyword `static` appears in this course in two entirely different roles. State both,
naming what each one changes. Then state why this course writes pin numbers and bit positions as
`#define LED1 1U` rather than `static const uint8_t led1 = 1U;`, giving two distinct reasons.
(3 marks)

**(d)** A software-PWM routine computes its on-time from a 10-bit ADC reading.

```c
#define ADC_MAX 1023U
#define PWM_PERIOD_MS 10U

const uint16_t adc_val = adc_read(POT1);              // Returns 512 for this reading.
const uint8_t on_ms    = (uint8_t)(adc_val / ADC_MAX * PWM_PERIOD_MS);
```

Give the exact value of `on_ms` and explain how it got there. State the only reading for which the
expression produces anything else, and what it produces. Then give two different corrections, and
name the trap that the first of them introduces on this target. (3 marks)

---

## Question 2 - Registers, and a program that does nothing (12 marks)

Two LEDs sit on D9 and D10. A button sits on D13, wired through an external pull-down, so the pin
reads high while the button is held. LED1 is meant to follow the button; LED2 is meant to stay dark.

```c
#include <avr/io.h>

/** GPIO pins. */
#define LED1 9U  // D9  -> PORTB1.
#define LED2 10U // D10 -> PORTB2.
#define BTN1 13U // D13 -> PORTB5.

/**
 * @brief Set up system.
 */
static void setup(void)
{
    // Configure LED1 and LED2 as outputs.
    DDRB = (1U << LED1);
    DDRB = (1U << LED2);

    // Configure BTN1 as input with its internal pull-up enabled.
    PORTB = (1U << BTN1);
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
        // Enable LED1 while BTN1 is held, disable it otherwise.
        if (PORTB & (1U << BTN1)) { PORTB |= (1U << LED1); }
        else { PORTB &= (1U << LED1); }
    }
    return 0;
}
```

**(a)** The program compiles without a warning and does nothing at all. It contains **four**
defects. Identify each one, state the consequence, and give the corrected line. (6 marks)

**(b)** Give the single C expression for each of the following on a register `reg` and a bit
position `n`: set the bit, clear the bit, test the bit, and toggle the bit. Then state why this
course toggles an output by writing to `PINx` rather than by `PORTx ^= (1U << n)`, and what that
buys. (3 marks)

**(c)** State what `F_CPU` is for, what must be true about *where* it is defined, and what the `UL`
suffix prevents on this target. Then state why `_delay_ms(ms)` cannot be called with a variable, and
what the course's `delay_ms()` helper does about it. (3 marks)

---

## Question 3 - An interrupt that never fires (12 marks)

BTN1 is on D2 (INT0), wired through an external pull-down, so a press drives the pin high. LED1 is
on D9. Each press-down is meant to toggle LED1 and report the new state over UART.

```c
#include <stdbool.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "driver/serial.h"

/** GPIO pins. */
#define LED1 1U // D9 -> PORTB1.

/** LED1 state (true = lit, false = dark). */
static bool led1_state = false;

/**
 * @brief Set up system.
 */
static void setup(void)
{
    // Configure LED1 as output.
    DDRB = (1U << LED1);

    // Trigger an interrupt on BTN1 press-down (INT0, D2).
    EICRA = (1U << ISC01);
    EIMSK = (1U << INT0);

    // Initialize serial driver.
    serial_init();
}

/**
 * @brief Toggle the LED1 state and report it on button press.
 */
ISR(INT0_vect)
{
    led1_state = !led1_state;
    serial_print("BTN1 pressed!");
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
        if (led1_state) { PORTB |= (1U << LED1); }
        else { PORTB &= ~(1U << LED1); }
    }
    return 0;
}
```

**(a)** The program contains **four** defects. Identify each one and state the consequence. Then
state which single one of the four must be fixed before any of the other three can be observed at
all, and why. (6 marks)

**(b)** The button is moved from D2 to D7. INT0 is no longer available. Give the three register
writes and the ISR signature needed to interrupt on D7 instead, and state **two** behavioural
differences between what you have written and the INT0 version it replaces. (3 marks)

**(c)** State what `volatile` tells the compiler and why omitting it produces a bug that appears
only in some builds. Then name one variable in this course that is shared with an ISR and still
does not need `volatile`, and say why. (3 marks)

---

## Question 4 - Converting and counting (13 marks)

**(a)** A colleague's ADC routine returns plausible-looking numbers that do not track the
potentiometer.

```c
static uint16_t adc_read(const uint8_t channel)
{
    ADMUX  = channel;
    ADCSRA = (1U << ADEN) | (1U << ADSC);
    ADCSRA = (1U << ADIF);

    const uint8_t high = ADCH;
    const uint8_t low  = ADCL;
    return (uint16_t)(high << 8U) | low;
}
```

It contains **four** defects. Identify each one and state the consequence. Then give the single
line that replaces the last three lines of the function entirely. (5 marks)

**(b)** A 16 MHz ATmega328P is configured with a ÷1024 prescaler.

1. Give the tick period in milliseconds, and the time between two Timer 0 overflows.
2. Give the number of Timer 0 overflows needed for a 750 ms timeout, and the resulting error.
3. Give the value written to `OCR1A` for a 40 ms periodic interrupt on Timer 1 in CTC mode, showing
   the arithmetic, and name the vector its handler must use.
4. State why the 40 ms period cannot be produced by Timer 0 in CTC mode at this prescaler.

(5 marks)

**(c)** The overflow handler below is meant to toggle an LED once per timeout.

```c
ISR(TIMER0_OVF_vect)
{
    static uint8_t ovf_count = 0U;

    if (OVF_MAX <= ++ovf_count) { LED1_TOGGLE; }
}
```

State exactly what the LED does after the first timeout, and why. Give the corrected handler. Then
state the one thing about Timer 2 that differs from Timers 0 and 1 in a way that silently breaks a
copy-pasted configuration. (3 marks)

---

## Question 5 - Talking to a PC (13 marks)

**(a)** State the formula that produces the value written to `UBRR0`. Compute it for 19200 baud on
a 16 MHz clock, showing the arithmetic and the rounding. Give the baud rate the chip actually
produces with that value and the resulting error, and state why an error of that size is harmless.
Then state what a terminal opened at 9600 baud shows while the chip transmits at 19200. (4 marks)

**(b)** The serial code below compiles and produces nothing usable.

```c
#define BAUD 9600U // 9600 bps with a 16 MHz clock.
#define MSG_LEN 50U

static void serial_init(void)
{
    UCSR0B = (1U << TXEN0);
    UCSR0C = (1U << UCSZ00) | (1U << UCSZ01);
    UBRR0  = BAUD;
}

static void serial_write_byte(const uint8_t byte)
{
    UDR0 = byte;
    while (0U == (UCSR0A & (1U << UDRE0))) {}
}

void serial_print(const char* text)
{
    for (uint16_t i = 0U; i <= MSG_LEN; ++i)
    {
        serial_write_byte((uint8_t)(text[i]));
    }
}

ISR(PCINT0_vect)
{
    if (BTN1_PRESSED) { serial_print("BTN1 pressed!"); }
}
```

It contains **four** defects. Identify each one and state the consequence. (5 marks)

**(c)** The program is extended to receive commands from the PC as well as send to it.

Name the bit that must be added to `UCSR0B`, name the flag in `UCSR0A` that says a byte has
arrived, and state what clears that flag. Write `serial_read_byte()` in the course's style. Then
state the failure mode of that function and what the course's `read()` adds to avoid it, including
what each of its three possible return values means. (4 marks)

---

## Question 6 - Remembering, and recovering (13 marks)

**(a)** The EEPROM write below never writes anything.

```c
static void eeprom_write_byte(const uint16_t addr, const uint8_t byte)
{
    EEAR = addr;
    EEDR = byte;

    EECR |= (1U << EEPE);
    EECR |= (1U << EEMPE);
}

int main(void)
{
    setup();

    while (1)
    {
        const uint8_t led_state = LED1_ENABLED ? EEPROM_LED1_ON : EEPROM_LED1_OFF;
        eeprom_write_byte(EEPROM_LED1_ADDR, led_state);
    }
    return 0;
}
```

It contains **four** defects, one of which is in `main()` rather than in the function. Identify each
one and state the consequence, then give the corrected function. One of the four produces a fault
that appears only occasionally; say which, and why that is worse than one that never works at all.
(5 marks)

**(b)** The watchdog code below has no effect whatsoever.

```c
static void watchdog_init(void)
{
    WDTCSR = (1U << WDE) | (uint8_t)(WATCHDOG_TIMEOUT_1024MS);
}

static void watchdog_reset(void)
{
    WDTCSR |= (1U << WDE);
}
```

Identify the defect in each function and state the consequence. Give both functions corrected. For
the second, state why it cannot be written as a register access at all, and what the course writes
instead of `asm("wdr")`, with the reason. (4 marks)

**(c)** A device with a watchdog reboots once because of a genuine hang. From then on, a colleague's
attempt to disable the watchdog from `setup()` is ignored, and the board reboots roughly once a
second forever.

Name the flag responsible, name the register it lives in, and explain the mechanism. State where
the fix goes and why its position matters. Then state what `WDE` and `WDIE` each select, and what
the chip does on timeout when **both** are set. (4 marks)

---

## Question 7 - One driver, many instances (12 marks)

**(a)** A program grows from one LED to six. Contrast what has to change under the
`#define LED1_ON PORTB |= (1U << LED1)` approach with what has to change under a `gpio_t` driver.
Then state why the ADC, UART, EEPROM and watchdog drivers in this course are *correct* to have no
struct at all, and what specifically would have to be true of a peripheral before one is warranted.
(3 marks)

**(b)** The GPIO driver below compiles and is used across several files.

```c
typedef struct
{
    volatile uint8_t* ddrx;
    volatile uint8_t* portx;
    volatile uint8_t* pinx;
    const uint8_t pin;
} gpio_t;

void gpio_write(gpio_t self, const bool state)
{
    if (state) { *self.portx |= (1U << self.pin); }
    else { *self.portx &= ~(1U << self.pin); }
}

bool gpio_read(gpio_t* self)
{
    return (*self->portx & (1U << self->pin));
}

void gpio_toggle(gpio_t* self)
{
    *self->pinx ^= (1U << self->pin);
}
```

It contains **four** defects against the course's own rules. Identify each one, state the
consequence, and give the corrected signature or line. For `gpio_toggle()`, describe precisely what
happens to the whole port when it is called on a pin that currently reads high. (6 marks)

**(c)** The driver is split into `driver/gpio.h` and `driver/gpio.c`. State what goes in each.
`sensor.h` earlier in the course declared only a function; `gpio.h` must also declare `gpio_t`
itself. State why. Then state why `gpio_init()`'s `mode` parameter is written `const gpio_mode_t
mode` in the source file but plain `gpio_mode_t mode` in the header. (3 marks)

---

## Question 8 - Hiding things, and choosing at runtime (13 marks)

**(a)** A driver header contains only this, plus prototypes.

```c
typedef struct gpio gpio_t;
```

State what this declares and what it deliberately does not. State what a caller holding a `gpio_t*`
can and cannot do with it. Then state why the `gpio_init(gpio_t* self, ...)` form has to be replaced
by `gpio_create(...)` returning a pointer once the struct is opaque, and name the invariant that
opacity protects on `timer_t` but that was never really at risk on `gpio_t`. (4 marks)

**(b)** A colleague adds function pointers to a still-public `gpio_t`.

```c
typedef struct gpio
{
    volatile uint8_t* ddrx;
    volatile uint8_t* portx;
    volatile uint8_t* pinx;
    uint8_t pin;

    void (*write)(gpio_t* self, bool state);
    bool (*read)(const gpio_t* self);
} gpio_t;
```

The file does not compile. State what the compiler reports and why, and give the corrected member
declarations. Then state what marking `gpio_write()` `static` in `gpio.c` does prevent, and what it
does **not** prevent. (4 marks)

**(c)** An interface and one of its two implementations are written as follows.

```c
typedef struct gpio_interface
{
    const gpio_vtable_t* vptr;
} gpio_interface_t;

typedef struct gpio_impl
{
    volatile uint8_t* ddrx;
    volatile uint8_t* portx;
    volatile uint8_t* pinx;
    uint8_t pin;
    gpio_interface_t itf;
} gpio_impl_t;

gpio_interface_t* gpio_atmega328p_new(const uint8_t pin, const gpio_mode_t mode)
{
    gpio_impl_t* impl = (gpio_impl_t*)(malloc(sizeof(gpio_impl_t)));
    if (NULL == impl) { return NULL; }
    // ... configure impl->ddrx / portx / pinx / pin from pin and mode ...
    impl->itf.vptr = gpio_vptr_get_instance();
    return (gpio_interface_t*)(impl);
}
```

The code compiles without a warning. State exactly what happens on the first call to
`led->vptr->toggle(led)`, and why. Give the one-line change that fixes it and the rule that makes
the fix work. State why the compiler said nothing. (3 marks)

**(d)** State why every instance can share one `static const gpio_vtable_t`, why `del()` takes
`gpio_interface_t**` rather than `gpio_interface_t*`, and what a `gpio_stub_t` implementation makes
possible that `gpio_impl_t` alone does not. (2 marks)

---

## Register Reference

Every ATmega328P register and bit this paper refers to is listed here, with what it is. Nothing
below is worth marks by itself: the paper tests which register to reach for and when, not whether
you can recall its name. Where a bit's **position** matters it is given; where a timing rule, an
ordering rule or a formula matters it is not, because that is the part being examined.

The target is an Arduino Uno/Nano at 16 MHz.

### Board pins and ports

| Board pin | Port bits       |
| --------- | --------------- |
| D0 - D7   | `PORTD0`-`PORTD7` |
| D8 - D13  | `PORTB0`-`PORTB5` |
| A0 - A5   | `PORTC0`-`PORTC5` |

### Digital I/O — one set per port, `x` being `B`, `C` or `D`

| Register | What it is |
| -------- | ---------- |
| `DDRx`   | Data direction, one bit per pin: `1` = output, `0` = input. |
| `PORTx`  | On an output pin, the level driven onto it. On an input pin, the internal pull-up enable. |
| `PINx`   | Reads the actual logic level on each pin. Writing a `1` to a bit toggles the matching `PORTx` bit, whatever `DDRx` says; bits written `0` are unaffected. |

### Interrupts

| Register / name | What it is |
| --------------- | ---------- |
| `SREG`          | Status register. Bit `I` is the global interrupt enable; `sei()` sets it, `cli()` clears it. |
| `EICRA`         | External interrupt edge select: `ISC01:ISC00` for INT0, `ISC11:ISC10` for INT1. |
| `EIMSK`         | External interrupt enable: bits `INT0`, `INT1`. |
| `INT0_vect` / `INT1_vect` | External interrupt vectors, for D2 and D3 respectively. |
| `PCICR`         | Pin change interrupt enable per port: `PCIE0` (B), `PCIE1` (C), `PCIE2` (D). |
| `PCMSK0` / `PCMSK1` / `PCMSK2` | Pin change mask for ports B, C and D. The bit position matches the pin's own bit position within its port. |
| `PCINT0_vect` / `PCINT1_vect` / `PCINT2_vect` | Pin change interrupt vectors, for ports B, C and D. |

`ISCn1:ISCn0` encoding: `00` low level, `01` any logical change, `10` falling edge, `11` rising edge.

### ADC

| Register / name | What it is |
| --------------- | ---------- |
| `ADMUX`  | Bits 3:0 select the input channel. `REFS1:REFS0` (bits 7:6) select the reference: `00` = the external `AREF` pin, `01` = `AVCC`. |
| `ADCSRA` | `ADEN` enables the ADC, `ADSC` starts a conversion, `ADIF` is the conversion-complete flag (cleared by writing a `1` to it), `ADPS2:ADPS0` set the prescaler. |
| `ADCL` / `ADCH` | Low and high halves of the 10-bit result. |
| `ADC`    | Macro combining `ADCL` and `ADCH` into the full 10-bit value. |

`ADPS2:ADPS0` encoding: `000` and `001` both divide by 2, and each value above that doubles the
divisor, up to `111` = ÷128.

### Timers — `n` is `0`, `1` or `2`

| Register / name | What it is |
| --------------- | ---------- |
| `TCCRnB` | Clock select `CSn2:CSn0` (the prescaler). `WGM12`, in `TCCR1B`, selects CTC mode on Timer 1. |
| `TCNTn`  | The counter itself. |
| `OCRnA`  | Compare value. In CTC mode the timer counts from 0 up to it, then clears. |
| `TIMSKn` | `TOIEn` enables the overflow interrupt; `OCIEnA` enables the compare-match interrupt. |
| `TIMERn_OVF_vect` | Overflow vector. |
| `TIMERn_COMPA_vect` | Compare-match vector. |

Timer 0 and Timer 2 are 8-bit; Timer 1 is 16-bit.

`CSn2:CSn0` encoding differs between timers:

| Bits  | Timer 0 / Timer 1 | Timer 2 |
| ----- | ----------------- | ------- |
| `001` | ÷1                | ÷1      |
| `010` | ÷8                | ÷8      |
| `011` | ÷64               | ÷32     |
| `100` | ÷256              | ÷64     |
| `101` | ÷1024             | ÷128    |
| `110` | –                 | ÷256    |
| `111` | –                 | ÷1024   |

### UART

| Register / name | What it is |
| --------------- | ---------- |
| `UCSR0A` | `UDRE0` is set when `UDR0` is free to accept a new byte. `RXC0` is set when a fully received byte is waiting in `UDR0`. |
| `UCSR0B` | `TXEN0` enables the transmitter, `RXEN0` enables the receiver. |
| `UCSR0C` | Frame format. `UCSZ01` and `UCSZ00` set together give 8 data bits. |
| `UBRR0`  | Baud rate divisor. Only the low 12 bits are used. |
| `UDR0`   | Data register: written to transmit, read to receive. Reading it clears `RXC0`. |

### EEPROM

| Register / name | What it is |
| --------------- | ---------- |
| `EEAR` | Address to read from or write to, 0-1023. |
| `EEDR` | Data register: holds the byte to be written, or receives the byte just read. |
| `EECR` | `EERE` starts a read. `EEMPE` is the master write enable, a safety interlock rather than a data bit. `EEPE` starts a write and stays set until that write completes. |

The ATmega328P has 1024 bytes of EEPROM. A byte write takes a few milliseconds, and a cell tolerates
roughly 100,000 write cycles.

### Watchdog

| Register / name | What it is |
| --------------- | ---------- |
| `MCUSR`   | MCU status register. `WDRF` is set by a watchdog-caused reset. |
| `WDTCSR`  | Watchdog control and status. Bit 0 `WDP0`, bit 1 `WDP1`, bit 2 `WDP2`, bit 3 `WDE`, bit 4 `WDCE`, bit 5 `WDP3`, bit 6 `WDIE`, bit 7 `WDIF`. |
| `WDT_vect` | Watchdog interrupt vector. |

`WDE` is the watchdog enable, `WDIE` the watchdog interrupt enable, and `WDCE` the change enable
used when reconfiguring them. `WDP3:WDP0` select the timeout: `0000` is 16 ms, and each step doubles
it, up to `1001` = 8192 ms.

---
