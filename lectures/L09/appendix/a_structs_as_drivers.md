# Appendix A - Structs as Drivers

## A.1 Why bundle state
* The drivers covered in L04 and L06–L08—the ADC, UART, EEPROM, and the watchdog—did not need any
  structs:
  * The chip has exactly one ADC (`adc_read()` already takes a channel number as a parameter, so
    it was never hardcoded to a single pin the way the examples below are).
  * It has exactly one UART.
  * It has exactly one EEPROM.
  * It has exactly one watchdog.
  * Because there is only one instance of each peripheral, plain functions operating on implicit
    global state were the simplest correct solution.

* The same single-instance pattern shows up again with peripherals the chip actually has many
  of, where it isn't the right solution:
  * `LED_ON` assumes one specific LED, even though the chip has many GPIO pins.
  * The configuration is effectively hardcoded into macros, functions, and global state.

* This pattern works well as long as the program needs only one instance of each thing.
* The pattern starts to break down when the program needs several instances:
  * L04 Appendix B contains an exercise with five LEDs.
  * Without a better abstraction, adding another LED means copying an entire family of macros or
    functions and renaming them.
  * That quickly becomes repetitive, difficult to maintain, and easy to get wrong.

* C already provides the tool needed to solve this problem: the `struct`:
  * Store one peripheral instance's configuration and state in a `struct`.
  * Create one struct value for each LED (or other multi-instance peripheral) instance.
  * Write ordinary functions that receive a pointer to the relevant struct.
  * The same functions can then operate on several different instances.

* By convention, the pointer parameter is called `self`:
  * `self` points to the particular peripheral instance that the function should operate on.
  * It plays a role similar to `this` in languages with built-in object support, such as C++, C#, and Java.
  * Unlike `this`, however, `self` is just an ordinary function parameter written explicitly in the code.

* However, C has no built-in support for object-oriented programming:
  * There is no hidden behaviour.
  * There are no constructors.
  * There is no inheritance.
  * There is no automatic method dispatch.
  * There is only a `struct` containing data and a set of ordinary functions that accept a pointer to that data.

* This lecture keeps the design deliberately simple:
  * The struct's fields remain directly accessible.
  * Functions are called normally.
  * No function pointers are required.

* The next lecture, L10, explores how much further this approach can be taken:
  * Hiding a struct's internal fields.
  * Calling operations through function pointers.
  * Creating a more object-like interface while still using C.

---

## A.2 The `self` convention: a `GPIO` driver
A minimal driver is a struct holding whatever a function needs to know to act on one specific
instance: here, pointers to the GPIO's data direction (`DDRx`), output (`PORTx`), and input
(`PINx`) registers, plus the bit position of its pin within them.

See [L01 Appendix A.3](../../L01/appendix/a_c_fundamentals.md#a3-structs-enums-and-typedefs) for
a refresher on structs and `typedef`; `gpio_t` follows the same anonymous-struct-plus-`_t`-suffix
convention introduced there.

```c
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
    const uint8_t pin;
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
 * @param[in] mode GPIO mode.
 */
void gpio_init(gpio_t* self, const gpio_mode_t mode)
{
    if (NULL == self) { return; }

    switch (mode)
    {
        case GPIO_MODE_INPUT_PULLUP:
            *self->ddrx &= ~(1U << self->pin);
            *self->portx |= (1U << self->pin);
            break;
        case GPIO_MODE_OUTPUT:
            *self->ddrx |= (1U << self->pin);
            *self->portx &= ~(1U << self->pin);
            break;
        default:
            *self->ddrx &= ~(1U << self->pin);
            *self->portx &= ~(1U << self->pin);
            break;
    }
}

/**
 * @brief Set state of the given GPIO.
 *
 * @param[in] self GPIO instance.
 * @param[in] state GPIO state (true = high, false = low).
 *
 * @note This operation is only supported for GPIOs configured as outputs.
 */
void gpio_write(gpio_t* self, const bool state)
{
    if (NULL == self) { return; }
    const bool output = (*self->ddrx & (1U << self->pin));
    if (!output) { return; }

    if (state) { *self->portx |= (1U << self->pin); }
    else { *self->portx &= ~(1U << self->pin); }
}

/**
 * @brief Read state of the given GPIO.
 *
 * @param[in] self GPIO instance.
 *
 * @return GPIO state (true = high, false = low).
 */
bool gpio_read(const gpio_t* self)
{
    return (NULL != self) ? (*self->pinx & (1U << self->pin)) : false;
}

/**
 * @brief Toggle state of the given GPIO.
 *
 * @param[in] self GPIO instance.
 */
void gpio_toggle(gpio_t* self)
{
    if (NULL == self) { return; }
    const bool output = (*self->ddrx & (1U << self->pin));
    if (!output) { return; }
    *self->pinx = (1U << self->pin);
}
```

A GPIO output instance can then be initialized as shown below:

```c
#define LED1_PIN 1U // D9 -> PORTB1.

/** Initialize LED on D9 (PORTB1). */
gpio_t led1 = {&DDRB, &PORTB, &PINB, LED1_PIN};

gpio_init(&led1, GPIO_MODE_OUTPUT);
```

Most of these functions take `gpio_t*` rather than `gpio_t`: passing a pointer avoids copying the
struct on every call, and they need to write through `self` to the hardware registers it points
at. `gpio_read()` takes `const gpio_t*` instead, since it only ever reads `self`, never writes
through it.

---

## A.3 Splitting the driver into `gpio.h` and `gpio.c`
So far A.2's `gpio_t`, `gpio_mode_t`, and every `gpio_*()` function have lived together in one
code block. In a real project a driver is split across a header and a source file, the same split
[L01 Appendix A.6](../../L01/appendix/a_c_fundamentals.md#a6-functions) introduced for
`sensor.h`/`sensor.c`, include guard and all:
* The header (`gpio.h`) is the public interface: the `gpio_t` and `gpio_mode_t` type definitions,
  plus a documented prototype for every function callers are allowed to use.
* The source file (`gpio.c`) `#include`s its own header first, then defines those same functions.
  Callers only ever see the header; the definitions in `gpio.c` are what actually gets compiled.

One thing is new compared to `sensor.h`: that header only ever declared a function, callers never
touched anything but the value it returned. `gpio.h` also declares `gpio_t` itself, because a
caller needs to create and initialize their own `gpio_t` instances directly (as in A.2's `led1`
example), not just receive values back from an opaque handle. That's also why `gpio_t`'s fields
stay public here instead of hidden behind a pointer to an incomplete type; L10 revisits that
trade-off.

Here's `driver/gpio.h`:

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
    const uint8_t pin;
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
 * @param[in] mode GPIO mode.
 */
void gpio_init(gpio_t* self, gpio_mode_t mode);

/**
 * @brief Set state of the given GPIO.
 *
 * @param[in] self GPIO instance.
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

#endif /** DRIVER_GPIO_H_ */
```

Here's `driver/gpio.c`:

```c
/**
 * @file GPIO driver implementation details.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <avr/io.h>

#include "driver/gpio.h"

// -----------------------------------------------------------------------------
void gpio_init(gpio_t* self, const gpio_mode_t mode)
{
    // Check the GPIO instance, terminate if invalid.
    if (NULL == self) { return; }

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
}

// -----------------------------------------------------------------------------
void gpio_write(gpio_t* self, const bool state)
{
    // Check the GPIO instance, terminate if invalid.
    if (NULL == self) { return; }

    // Check GPIO direction, terminate if not output.
    const bool output = (*self->ddrx & (1U << self->pin));
    if (!output) { return; }

    // Set GPIO state as specified.
    if (state) { *self->portx |= (1U << self->pin); }
    else { *self->portx &= ~(1U << self->pin); }
}

// -----------------------------------------------------------------------------
bool gpio_read(const gpio_t* self)
{
    // Return GPIO state if GPIO instance is valid, else false.
    return (NULL != self) ? (*self->pinx & (1U << self->pin)) : false;
}

// -----------------------------------------------------------------------------
void gpio_toggle(gpio_t* self)
{
    // Check the GPIO instance, terminate if invalid.
    if (NULL == self) { return; }

    // Check GPIO direction, terminate if not output.
    const bool output = (*self->ddrx & (1U << self->pin));
    if (!output) { return; }

    // Toggle GPIO state.
    *self->pinx = (1U << self->pin);
}
```

Notice `gpio_init()`'s `mode` parameter (and `gpio_write()`'s `state`) drop the `const` in the
header but keep it in the source file. A top-level `const` on a parameter only constrains what the
function body can do with its own local copy; it isn't part of the function's type and can't
affect how callers call it, so by convention it's omitted from the prototype and kept in the
definition, where it actually matters.

---

## A.4 Worked example: an array of LEDs
The whole point of a driver struct is that `gpio_write()`/`gpio_read()` don't care which GPIO they're
called on. An array of `gpio_t` instances plus a loop replaces a whole family of per-LED macros:

```c
#define LED_COUNT 5U

// Create five LED instances.
static gpio_t leds[LED_COUNT] =
{
    { &DDRD, &PORTD, &PIND, 6U}, // D6  -> PORTD6.
    { &DDRD, &PORTD, &PIND, 7U}, // D7  -> PORTD7.
    { &DDRB, &PORTB, &PINB, 0U}, // D8  -> PORTB0.
    { &DDRB, &PORTB, &PINB, 1U}, // D9  -> PORTB1.
    { &DDRB, &PORTB, &PINB, 2U}, // D10 -> PORTB2.
};

/**
 * @brief Set up system.
 */
static void setup(void)
{
    for (uint8_t i = 0U; i < LED_COUNT; i++)
    {
        gpio_init(&leds[i], GPIO_MODE_OUTPUT);
    }
}
```

Compare this to L04 Appendix B's five-LED exercise, which needed a hand-written `LED1_ON`
through `LED5_ON` (and `_OFF`), one pair of macros per pin. Here, adding a sixth LED on the same
port means adding one line to the `leds[]` array, no new functions.

---

## A.5 Worked example: `gpio_t` wired into L06-L08's demo
Combines L06's serial driver, L07's EEPROM driver, and L08's watchdog driver with this appendix's
`gpio_t`: LED1 (D9) persists its state across resets via EEPROM, BTN1 (D13) toggles LED1 through a
debounced pin-change interrupt exactly like [L08 Appendix
A.5](../../L08/appendix/a_watchdog.md#a5-worked-example-petting-the-watchdog-in-l07s-demo), and the
watchdog resets the chip if the main loop ever stalls. Every register access that went through
L08's `LED1_TOGGLE`/`LED1_ENABLED`/`BTN1_PRESSED` macros now goes through `gpio_toggle()`/
`gpio_read()` instead.

![](../../L06/appendix/images/circuit_serial_with_gpios.png)

```
gpio_driver_demo/
├── Makefile
├── main.c
├── include/
│   └── driver/
│       ├── eeprom.h
│       ├── gpio.h
│       ├── serial.h
│       └── watchdog.h
└── source/
    └── driver/
        ├── eeprom.c
        ├── gpio.c
        ├── serial.c
        └── watchdog.c
```

* The serial, EEPROM, and watchdog drivers were introduced in L06–L08.
* [driver/gpio.h](../gpio_driver_demo/include/driver/gpio.h) and [driver/gpio.c](../gpio_driver_demo/source/driver/gpio.c) extend the `gpio_t` type introduced in Appendices A.2 and A.3:
* The extended driver adds one capability not covered in those sections:
  * `gpio_enable_pci()` enables or disables the pin-change interrupt for a GPIO.
  * It does this by writing the same `PCICR` and `PCMSKx` bits that were configured manually in L08 Appendix A.5.
* Supporting `gpio_enable_pci()` requires two additional fields in `gpio_t`, beyond `ddrx`, `portx`, `pinx`, and `pin`:
  * A pointer to the port's `PCMSKx` register.
  * The bit position of the port's `PCIEx` bit in `PCICR`.
* Every `gpio_t` instance contains these two additional fields:
  * This is true even when that particular GPIO never uses `gpio_enable_pci()`.
  * For example, LED1 still carries the pin-change-interrupt configuration fields.

`main.c` in full:

```c
/**
 * @file GPIO driver demo.
 */
#include <stdbool.h>
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/serial.h"
#include "driver/watchdog.h"

#define LED1_PIN 1U // D9  -> PORTB1.
#define BTN1_PIN 5U // D13 -> PORTB5.

/** Timer operations. */
#define TIMER0_ENABLE TIMSK0 = (1U << TOIE0) // Enable timer 0 interrupt.
#define TIMER0_DISABLE TIMSK0 = 0U           // Disable timer 0 interrupt.

/** Time parameters */
#define TICK_PERIOD_MS 0.064F    // Time between each tick, prescaler 1024.
#define TICK_MAX 256U            // Ticks per overflow for 8-bit timer.
#define DEBOUNCE_TIMEOUT_MS 300U // Debounce timeout in ms.

/** Limit parameters. */
#define OVF_TIME_MS (TICK_PERIOD_MS * TICK_MAX)     // Time between each overflow in ms.
#define OVF_MAX (DEBOUNCE_TIMEOUT_MS / OVF_TIME_MS) // Overflows needed for timeout.
#define OVF_TIMEOUT (uint8_t)(OVF_MAX + 0.5F)       // Overflows needed for timeout, rounded.

/** EEPROM parameters. */
#define EEPROM_LED1_ADDR 1000U // EEPROM address containing the LED1 state.
#define EEPROM_LED1_ON 1U      // Stored value indicating that LED1 is on.
#define EEPROM_LED1_OFF 0U     // Stored value indicating that LED1 is off.

/** Flags that a LED1 event has occurred. */
static bool led1_event = false;

/** GPIO instances. */
static gpio_t led1 = {&DDRB, &PORTB, &PINB, &PCMSK0, PCIE0, LED1_PIN};
static gpio_t btn1 = {&DDRB, &PORTB, &PINB, &PCMSK0, PCIE0, BTN1_PIN};

/**
 * @brief Print the LED1 state over UART.
 */
static void print_led1_state(void)
{
    if (gpio_read(&led1)) { serial_print("LED1 enabled!\n"); }
    else { serial_print("LED1 disabled!\n"); }
}

/**
 * @brief Set up system.
 */
static void setup(void)
{
    // Initialize GPIOs.
    gpio_init(&led1, GPIO_MODE_OUTPUT);
    gpio_init(&btn1, GPIO_MODE_INPUT_PULLUP);
    gpio_enable_pci(&btn1, true);

    // Set up 300 ms debounce timer.
    TCCR0B = (1U << CS00) | (1U << CS02);

    // Initialize serial driver.
    serial_init();

    // Restore the previous LED state from EEPROM.
    uint8_t led_state = 0U;
    if (sizeof(led_state) == eeprom_read(&led_state, sizeof(led_state), EEPROM_LED1_ADDR))
    {
        if (EEPROM_LED1_ON == led_state) { gpio_toggle(&led1); }
    }
    print_led1_state();

    // Initialize watchdog with a 1024 timeout.
    watchdog_init(WATCHDOG_TIMEOUT_1024MS);

    // Enable interrupts globally.
    sei();
}

/**
 * @brief Toggle LED1 and transmit a message over UART on button press.
 *
 *        Disable button interrupts for 300 ms to prevent debounce.
 */
ISR(PCINT0_vect)
{
    // Disable button interrupts for 300 ms.
    gpio_enable_pci(&btn1, false);
    TIMER0_ENABLE;

    // Toggle LED1 and save its new state if BTN1 is pressed.
    if (gpio_read(&btn1))
    {
        gpio_toggle(&led1);
        led1_event = true;
    }
}

/**
 * @brief Re-enable button interrupts after 300 ms.
 */
ISR(TIMER0_OVF_vect)
{
    static volatile uint8_t ovf_counter = 0U;

    // Wait for 300 ms, then re-enable button interrupts and disable timer 0.
    if (OVF_TIMEOUT <= ++ovf_counter)
    {
        gpio_enable_pci(&btn1, true);
        TIMER0_DISABLE;
        ovf_counter = 0U;
    }
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
        // Reset the watchdog once every iteration of the loop.
        watchdog_reset();

        // Check if a LED1 event has occurred, store the new state in EEPROM if true.
        if (led1_event)
        {
            const uint8_t led_state = gpio_read(&led1) ? EEPROM_LED1_ON : EEPROM_LED1_OFF;
            eeprom_write(&led_state, sizeof(led_state), EEPROM_LED1_ADDR);
            led1_event = false;

            // Report the new LED1 state over UART.
            print_led1_state();
        }
    }
    return 0;
}
```

`led1_event`, the debounce timer, and the EEPROM-persistence logic are unchanged from L08 Appendix
A.5; only the GPIO handling changed, from bespoke macros tied to `PORTB` specifically to a driver
that works for however many `gpio_t` instances a program declares, on whichever pins and ports they
happen to be wired to.

Build and flash the same way as
[L08 Appendix A.5](../../L08/appendix/a_watchdog.md#a5-worked-example-petting-the-watchdog-in-l07s-demo):
on Linux, `cd ../gpio_driver_demo` and run `make` (`make flash` to program the board); on
Windows via Microchip Studio, recreate this folder structure inside the project and add `include`
as an include directory.

---
