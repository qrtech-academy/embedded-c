# Embedded C - Written Examination, Paper B

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

**Where a question asks you to write code**, Doxygen comments are marked only where the question
says so. Everywhere else, write the code.

**Where a question says "state the consequence"**, naming the defect earns half the marks and
saying what it does to the running program earns the other half. "This is wrong" scores nothing.

### Marks

| Question | 1   | 2   | 3   | 4   | 5   | 6   | 7   | 8   |
| -------- | --- | --- | --- | --- | --- | --- | --- | --- |
| Marks    | 12  | 12  | 12  | 13  | 13  | 13  | 12  | 13  |

---

## Question 1 - Saying what you mean (12 marks)

**(a)** A blink module supports four patterns: off, slow, fast, and a two-pulse heartbeat.

Write, in the course's style, an enumeration `led_pattern_t` naming all four, and a function

```c
bool led_pattern_valid(led_pattern_t pattern);
```

that returns `true` only for a named member. State three concrete advantages an `enum` has over a
block of `#define`s for this, and then state the one property a `#define` has that a `const uint8_t`
does not, which is why this course still writes its pin numbers as macros. (4 marks)

**(b)** Write a function-like macro `TICKS_NEEDED(timeout_ms, tick_ms)` that yields the number of
ticks in a timeout. State the two parenthesisation rules it must obey, showing for one of them a
call that goes wrong if the rule is broken. Then name the third macro hazard, which parentheses
cannot fix, and state whether your macro is exposed to it, with the reason. (3 marks)

**(c)** A host-side test harness grows a buffer as data arrives.

```c
typedef struct
{
    uint8_t* data;
    size_t size;
} buffer_t;
```

Write `buffer_resize(buffer_t* self, size_t size)` returning `0` on success and `1` on failure. It
must leave `self` completely untouched if the allocation fails, and must handle a requested size of
zero. State the specific mistake that a naive one-line `self->data = realloc(self->data, size);`
makes, and what it costs. (3 marks)

**(d)** A driver is split into `driver/serial.h` and `driver/serial.c`. Give the header guard for
that header, in the course's convention, and say what it is for. Then state the ordering convention
this course uses inside a `.c` file for its public functions and its `static` helpers, and the
reason. (2 marks)

---

## Question 2 - Straight to the registers (12 marks)

**(a)** Three LEDs are wired to D8, D9 and D10, and a button to D12 through an external pull-down,
so the pin reads high while the button is held.

Write a complete single-file program, in the course's style, that lights exactly one LED at a time
in the order D8 → D9 → D10, advancing every 150 ms, for as long as the button is held, and turns all
three off the moment it is released. Use macros for the pins and for the GPIO operations, and a
`delay_ms()` helper. (5 marks)

**(b)** Give the single C expression for each of the following on a register `reg` and a bit
position `n`: set the bit, clear the bit, test the bit, toggle it via `PORTx`, and toggle it
atomically. Then state the difference between `DDRB = (1U << 3U);` and `DDRB |= (1U << 3U);`, and
give one situation where each is the right choice. (4 marks)

**(c)** `PORTx` means one thing on an output pin and something else entirely on an input pin. State
both, and state which register must be read to get a pin's actual level.

The button circuit in the appendix uses an external 10 kΩ pull-down, yet the code still writes
`PORTB = (1U << BTN1);`. State what that write does, what the pin reads when idle, and why the write
is harmless rather than contradictory. (3 marks)

---

## Question 3 - Reacting instead of polling (12 marks)

**(a)** A button is wired to D3 through an external pull-down, so a press drives the pin high. An
LED is on D10. Every press-down must toggle the LED, and the toggle must be immune to another
interrupt changing a different bit of the same port at the same moment.

Write `setup()` and the interrupt handler, in the course's style, giving every register write and
the exact vector name. State which edge you selected and why. (5 marks)

**(b)** The button is moved from D3 to A2. State everything that changes: the enable bit, the mask
register, the bit written into it, and the vector. Then state **two** things the handler must now do
that the D3 version did not, and why each is forced on you rather than optional. (4 marks)

**(c)** State the rule this course applies to decide what belongs inside an ISR and what belongs in
`main()`'s loop. Name two specific calls from the course's own drivers that must never appear in an
ISR, and give the concrete cost of each. Then write the three-line pattern an ISR and `main()` use
to communicate instead, and say what `volatile` is doing in it. (3 marks)

---

## Question 4 - Reading the world, and keeping time (13 marks)

**(a)** Write `adc_read(uint8_t channel)` in full, in the course's style, returning a 10-bit result.
For each register you write, state in one sentence what that write is for. State what the ÷128
prescaler is chosen to achieve and what goes wrong without it. (4 marks)

**(b)** A 16 MHz ATmega328P is configured with a ÷1024 prescaler, and a 250 ms periodic event is
required.

1. Give the tick period and the Timer 0 overflow period.
2. Give the `OCR1A` value for a 250 ms interrupt on Timer 1 in CTC mode, showing the arithmetic,
   and write the three register assignments plus the vector name.
3. Give the number of Timer 0 overflows the same 250 ms needs in Normal Mode, the actual period that
   produces, and the error as a percentage.
4. State why Timer 0 cannot reach 250 ms without counting overflows in software at any prescaler
   available to it.

(5 marks)

**(c)** A potentiometer on A0 sets the brightness of an LED on D9 by software PWM over a 10 ms
period.

Write `run_pwm_cycle()`, in the course's style, computing the on-time and off-time from a fresh ADC
reading. State why the course adds `+ 0.5F` before the cast, what this whole approach costs the rest
of the program, and what replaces it. (4 marks)

---

## Question 5 - A channel to the host (13 marks)

**(a)** Write `driver/serial.h` in full, in the course's style: header guard, includes, and a
documented prototype for each of `serial_init()`, `serial_print()` and
`serial_write()` (the buffer-and-length form). Doxygen comments are marked here.

Then state which functions of the driver belong in the header and which must be `static` in
`serial.c`, with the reason, and state why a `const` on a by-value parameter is dropped from the
prototype but kept in the definition. (5 marks)

**(b)** Write `serial_write_byte()` and `serial_print()`. State what `UDRE0` signals, and why the
wait on it comes before the write to `UDR0` rather than after. Then state what `sprintf()` is for in
this context, and the one caveat the appendix attaches to it on AVR. (4 marks)

**(c)** A program must report an ADC reading over UART once a second while still responding to a
debounced button on a pin-change interrupt.

Describe where the `sprintf()` and the `serial_print()` calls go, and why they cannot go where the
event is detected. Write the flag pattern that connects the two, and state exactly what `volatile`
is doing. Then state why the course's `read()` takes a `timeout_ms` parameter, and what each of its
three possible return values means. (4 marks)

---

## Question 6 - Surviving a power cut, and a hang (13 marks)

**(a)** Write `eeprom_write_byte(uint16_t addr, uint8_t byte)` and `eeprom_read_byte(uint16_t addr)`
in full, in the course's style. For the write, explain each of the following in one sentence: the
loop on `EEPE` at the top, the order of `EEMPE` and `EEPE`, and the `cli()`/`sei()` pair. State why
the read needs no equivalent of the last of those. (5 marks)

**(b)** Write the enumeration `watchdog_timeout_t` covering all ten timeout settings, and
`watchdog_init(watchdog_timeout_t timeout)`.

Explain why eight of the ten members can take their default values while the last two must be given
`32` and `33` explicitly. Then explain what the `WDCE` write immediately before the real one is for,
and what happens if the second write comes too late. (4 marks)

**(c)** Four values are candidates for EEPROM storage:

1. a calibration offset written once during manufacture;
2. a live ADC reading sampled at 100 Hz;
3. an on/off state a user toggles with a button;
4. a boot counter incremented on every reset.

For each, state whether EEPROM is appropriate and why, quantifying your answer for the one that is
clearly wrong. Then state where in `main()` the watchdog reset belongs, and how the timeout should
be chosen relative to the loop it is protecting. (4 marks)

---

## Question 7 - A driver that scales (12 marks)

**(a)** Write `driver/gpio.h`, and the body of `gpio_init()`, for a GPIO driver in which the caller
passes a single Arduino pin number rather than register pointers:

```c
bool gpio_init(gpio_t* self, uint8_t pin, gpio_mode_t mode);
```

* Pins 0-7 map to `PORTD0`-`PORTD7`; pins 8-13 map to `PORTB0`-`PORTB5`.
* `gpio_init()` derives and stores the register pointers and the bit position itself.
* Three modes are supported: input, input with pull-up, and output.
* Any pin outside 0-13 returns `false` without touching a single register.

Doxygen comments are marked on the header. (6 marks)

**(b)** In the original driver, `gpio_t`'s `pin` field was declared `const uint8_t pin;`. State why
it can no longer be `const` in your version, and what specifically a `const` struct member permits
and forbids. Then state why `gpio_read()` takes `const gpio_t* self` while `gpio_write()` takes
`gpio_t* self`, and what would change if either took `gpio_t self` by value instead. (3 marks)

**(c)** Two calls to `gpio_init()` with the same pin number would silently hand out two drivers for
one piece of hardware.

Write the pin-reservation table that prevents it: the declaration in `gpio.c`, the check and the
claim inside `gpio_init()`, and the release inside `gpio_free()`. State why the table must be
`static`, and what breaks if it is not. (3 marks)

---

## Question 8 - Hiding the implementation (13 marks)

**(a)** The GPIO driver's struct currently sits, fields and all, in `gpio.h`.

Write the opaque version of that header: what replaces the full struct definition, and what has to
happen to the struct itself. Then state the three things this takes away from calling code, and for
each, what the caller must do instead. (4 marks)

**(b)** Write a `button_t` struct holding a GPIO and a press callback, together with

```c
static void button_check(const button_t* self, bool edge);
```

which invokes the callback when a press has been detected. State in words what
`void (*on_press)(void);` declares, write the line that assigns a function to it, and write the line
that calls through it. Then state why `button_check()` is better for never mentioning the LED it
ultimately toggles. (4 marks)

**(c)** Given the interface

```c
typedef struct gpio_interface gpio_interface_t;

typedef struct gpio_vtable
{
    void (*del)(gpio_interface_t** self);
    bool (*read)(const gpio_interface_t* self);
    void (*write)(gpio_interface_t* self, bool value);
    void (*toggle)(gpio_interface_t* self);
} gpio_vtable_t;

typedef struct gpio_interface
{
    const gpio_vtable_t* vptr;
} gpio_interface_t;
```

write a second implementation, `gpio_stub_t`, that touches no hardware at all: the struct, its four
vtable functions, the vtable itself, and `gpio_stub_new()`.

State the one rule your struct's field order must obey and the language guarantee that makes the
whole scheme work. State why a single shared vtable is enough for every instance ever created. Then
state what having a stub makes possible that a hardware-only driver does not. (5 marks)

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
