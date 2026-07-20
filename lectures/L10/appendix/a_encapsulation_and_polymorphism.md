# Appendix A - Encapsulation and Polymorphism in C

## A.1 Why hide a struct's fields
* Every driver in L09 is fully exposed: `gpio_t` and `timer_t`'s fields are visible to any file
  that includes their definition.
  * For something as simple as `gpio_t`, that's mostly harmless.
  * For `timer_t`, it's less so: nothing stops calling code from reaching in and setting its
    elapsed-time field directly, skipping `timer_init()` and quietly breaking the invariant
    `timer_running()`/`timer_reset()` depend on.
* An **opaque struct** fixes this by splitting a driver across two files:
  * A header that only forward-declares the struct's name and lists function prototypes.
  * A `.c` file that defines the struct's actual fields.
  * Callers only ever see a pointer to an incomplete type: they can pass it around and hand it
    to the driver's functions, but they can't see or touch its fields, so they can't bypass the
    functions meant to maintain its invariants.
* Splitting a driver across a header and a `.c` file is what makes opacity possible:
  * [L09 Appendix
    A.3](../../L09/appendix/a_structs_as_drivers.md#a3-splitting-the-driver-into-gpioh-and-gpioc)
    already did this split, for a different reason.
  * It only works when the struct's real definition lives somewhere the caller doesn't (and
    can't) include.
  * The next section shows a lighter version of the same idea, hiding *functions* rather than
    fields, before A.3 hides fields too.

---

## A.2 Hiding functions with `static`
* L09's `gpio_t` (Appendix A.2) stays exactly as public here: every field still visible.
* What changes is how its behavior is reached:
  * `gpio_write()`, `gpio_read()`, `gpio_toggle()`, and a new `gpio_enable_pci()` become
    `static`.
  * At file scope, `static` means internal linkage: invisible outside `gpio.c`, a different
    meaning than the storage-duration one it has on a local variable (see [L01 Appendix
    A.4](../../L01/appendix/a_c_fundamentals.md#a4-scope-lifetime-and-storage)).
  * `gpio_t` gains four function pointers so the now-hidden functions are still reachable, just
    through the struct instead of by name.

`driver/gpio.h`, `gpio_t`'s fields all still public:

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

/**
 * @brief GPIO driver structure.
 */
typedef struct gpio
{
    /** Pointer to the data direction register associated with the GPIO. */
    volatile uint8_t* ddrx;

    /** Pointer to the output register associated with the GPIO. */
    volatile uint8_t* portx;

    /** Pointer to the input register associated with the GPIO. */
    volatile uint8_t* pinx;

    /** Pointer to the pin change mask register associated with the GPIO. */
    volatile uint8_t* pcmskx;

    /** Pin change interrupt control register bit (PCIEx) associated with the GPIO's port. */
    uint8_t pcicrx;

    /** Bit position of the GPIO pin within the associated port. */
    uint8_t pin;

    /** Set state of the given GPIO. Only supported for outputs. */
    void (*write)(struct gpio* self, bool state);

    /** Read state of the given GPIO. */
    bool (*read)(const struct gpio* self);

    /** Toggle state of the given GPIO. Only supported for outputs. */
    void (*toggle)(struct gpio* self);

    /** Enable/disable pin change interrupts for the given GPIO. */
    void (*enable_pci)(struct gpio* self, bool enable);
} gpio_t;

/**
 * @brief Initialize GPIO.
 *
 * @param[in] self GPIO instance.
 * @param[in] pin  Digital pin to use. Must be in range [0, 13].
 * @param[in] mode GPIO mode.
 *
 * @return True on success, false on failure.
 */
bool gpio_init(gpio_t* self, uint8_t pin, gpio_mode_t mode);

#endif /* DRIVER_GPIO_H_ */
```

* Each function pointer's `self` parameter is typed `struct gpio*`, not `gpio_t*`:
  * `typedef struct gpio { ... } gpio_t;` only introduces the name `gpio_t` once the whole
    declaration finishes, at the final `;` after the closing brace.
  * Inside the braces, while the struct is still being defined, `gpio_t` doesn't exist yet:
    writing `gpio_t* self` there is a compile error (`unknown type name 'gpio_t'`).
  * The tag `struct gpio`, on the other hand, is in scope from the moment `struct gpio {`
    appears, so a member can refer to a pointer to its own struct type by tag, the same trick a
    linked list's `struct node* next` relies on.
  * Everywhere *outside* the struct, `gpio_t` is the normal name to use; only these member
    declarations, written before their own type has a name, need `struct gpio` instead.

Unlike L09's `gpio_init(self, mode)`, which expected `ddrx`/`portx`/`pinx`/`pin` already set
through a struct literal, this `gpio_init(self, pin, mode)` takes an Arduino pin number and works
the registers out itself, following the pin-number rewrite from [L09 Appendix B Exercise
4](../../L09/appendix/b_exercises.md): pins 0-7 map to `PORTD`, pins 8-13 map to `PORTB`, and an
out-of-range pin returns `false` without touching any register. `driver/gpio.c`:

```c
/**
 * @file GPIO driver implementation details.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "driver/gpio.h"

#define PIN_OFFSET_B 8U  // Pin offset for I/O port B.
#define GPIO_PIN_MAX 13U // Maximum GPIO pin number.

// -----------------------------------------------------------------------------
static void gpio_write(gpio_t* self, const bool state)
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
static bool gpio_read(const gpio_t* self)
{
    // Return GPIO state if GPIO instance is valid, else false.
    return (NULL != self) ? (*self->pinx & (1U << self->pin)) : false;
}

// -----------------------------------------------------------------------------
static void gpio_toggle(gpio_t* self)
{
    // Check the GPIO instance, terminate if invalid.
    if (NULL == self) { return; }

    // Check GPIO direction, terminate if not output.
    const bool output = (*self->ddrx & (1U << self->pin));
    if (!output) { return; }

    // Toggle GPIO state.
    *self->pinx = (1U << self->pin);
}

// -----------------------------------------------------------------------------
static void gpio_enable_pci(gpio_t* self, bool enable)
{
    // Check the GPIO instance, terminate if invalid.
    if (NULL == self) { return; }

    // Enable/disable interrupts for the given pin as specified.
    if (enable)
    {
        PCICR |= (1U << self->pcicrx);
        *self->pcmskx |= (1U << self->pin);
        sei();
    }
    else { *self->pcmskx &= ~(1U << self->pin); }
}

// -----------------------------------------------------------------------------
bool gpio_init(gpio_t* self, const uint8_t pin, const gpio_mode_t mode)
{
    // Check the GPIO instance, return false if invalid.
    if (NULL == self) { return false; }

    // Check the pin number, return false if invalid.
    if (GPIO_PIN_MAX < pin) { return false; }

    // PIN 0 - 7 => I/O port D, pin = ID.
    if (PIN_OFFSET_B > pin)
    {
        self->ddrx   = &DDRD;
        self->portx  = &PORTD;
        self->pinx   = &PIND;
        self->pin    = pin;
        self->pcmskx = &PCMSK2;
        self->pcicrx = PCIE2;
    }
    // PIN 8 - 13 => I/O port B, pin = ID - 8.
    else
    {
        self->ddrx   = &DDRB;
        self->portx  = &PORTB;
        self->pinx   = &PINB;
        self->pin    = pin - PIN_OFFSET_B;
        self->pcmskx = &PCMSK0;
        self->pcicrx = PCIE0;
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

    // Set function pointers.
    self->write      = gpio_write;
    self->read       = gpio_read;
    self->toggle     = gpio_toggle;
    self->enable_pci = gpio_enable_pci;
    return true;
}
```

* Two fields that used to be `const`, `pin` and `pcicrx`, aren't anymore:
  * Both used to be known at the call site and handed in through a struct literal before
    `gpio_init()` ever ran.
  * Now `gpio_init()` derives them itself from the pin number argument and assigns them with
    `self->pin = ...`/`self->pcicrx = ...`.
  * A `const` member can only be set once, by an initializer at the point a struct is declared,
    never by assignment afterward, so once the value is only known and set inside `gpio_init()`,
    the field can no longer stay `const`. [L09 Appendix B Exercise
    4](../../L09/appendix/b_exercises.md) asks this exact question for the non-function-pointer
    version of this same rewrite.

```
function_pointer_demo/
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

* [driver/gpio.h](../function_pointer_demo/include/driver/gpio.h) and
  [driver/gpio.c](../function_pointer_demo/source/driver/gpio.c) are the full driver shown
  above.
* [driver/eeprom.h](../function_pointer_demo/include/driver/eeprom.h)/[.c](../function_pointer_demo/source/driver/eeprom.c),
  [driver/serial.h](../function_pointer_demo/include/driver/serial.h)/[.c](../function_pointer_demo/source/driver/serial.c),
  and [driver/watchdog.h](../function_pointer_demo/include/driver/watchdog.h)/[.c](../function_pointer_demo/source/driver/watchdog.c)
  are the same EEPROM, UART, and watchdog drivers from L06-L08, wired in only so `main.c` has
  something to persist state to and report over, unrelated to this lecture's material.

`main.c` toggles LED1 (`D9`) whenever BTN1 (`D13`) is pressed, debounces the button with a 300 ms
timer, persists the new LED state to EEPROM, reports it over serial, and kicks the watchdog every
loop iteration:

```c
/**
 * @file GPIO driver demo iwth function pointers.
 */
#include <stdbool.h>
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/serial.h"
#include "driver/watchdog.h"

#define LED1_PIN 9U  // D9  -> PORTB1.
#define BTN1_PIN 13U // D13 -> PORTB5.

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
static gpio_t led1, btn1;

/**
 * @brief Print the LED1 state over UART.
 */
static void print_led1_state(void)
{
    if (led1.read(&led1)) { serial_print("LED1 enabled!\n"); }
    else { serial_print("LED1 disabled!\n"); }
}

/**
 * @brief Set up system.
 */
static void setup(void)
{
    // Initialize GPIOs.
    gpio_init(&led1, LED1_PIN, GPIO_MODE_OUTPUT);
    gpio_init(&btn1, BTN1_PIN, GPIO_MODE_INPUT_PULLUP);
    btn1.enable_pci(&btn1, true);

    // Set up 300 ms debounce timer.
    TCCR0B = (1U << CS00) | (1U << CS02);

    // Initialize serial driver.
    serial_init();

    // Restore the previous LED state from EEPROM.
    uint8_t led_state = 0U;
    if (sizeof(led_state) == eeprom_read(&led_state, sizeof(led_state), EEPROM_LED1_ADDR))
    {
        if (EEPROM_LED1_ON == led_state) { led1.toggle(&led1); }
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
    btn1.enable_pci(&btn1, false);
    TIMER0_ENABLE;

    // Toggle LED1 and save its new state if BTN1 is pressed.
    if (btn1.read(&btn1))
    {
        led1.toggle(&led1);
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
        btn1.enable_pci(&btn1, true);
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
            const uint8_t led_state = led1.read(&led1) ? EEPROM_LED1_ON : EEPROM_LED1_OFF;
            eeprom_write(&led_state, sizeof(led_state), EEPROM_LED1_ADDR);
            led1_event = false;

            // Report the new LED1 state over UART.
            print_led1_state();
        }
    }
    return 0;
}
```

* `led1.toggle(&led1)` and `btn1.enable_pci(&btn1, true)` instead of `gpio_toggle(&led1)` and
  `gpio_enable_pci(&btn1, true)`: the same call shape A.6's vtables use later, just with the
  pointers sitting directly on `gpio_t` instead of behind a separate interface struct.
* `PCINT0_vect` only toggles the LED, a single register write, and sets `led1_event`; the EEPROM
  write and serial report both happen from `main()`'s loop once `led1_event` is seen, following
  the same ISR-minimality discipline as [L06 Appendix
  A.6](../../L06/appendix/a_uart.md#a6-example-button-toggles-an-led-reports-over-serial): keep
  the ISR to the minimum, let `main()`'s loop handle anything that can block.
* A second ISR, `TIMER0_OVF_vect`, re-enables the button's interrupt after the 300 ms debounce
  window, using the same `enable_pci` function pointer `main()` used to arm it.
* Nothing here hides `led1`'s or `btn1`'s *fields* yet: `led1.ddrx`, `led1.write`, all of it, are
  exactly as reachable as before. `static` only hid the *functions*; A.3 hides the fields too.

Build and flash the same way as [L09 Appendix
A.5](../../L09/appendix/a_structs_as_drivers.md#a5-worked-example-gpio_t-wired-into-l06-l08s-demo):
on Linux, `cd ../function_pointer_demo` and run `make` (`make flash` to program the board); on
Windows via Microchip Studio, recreate this folder structure inside the project and add `include`
as an include directory.

---

## A.3 Opaque structs
* Starting over from L09 Appendix A.2/A.3's `gpio_t`, not continuing from A.2's version above:
  * Hiding fields and hiding functions are independent techniques, not two steps of one
    pipeline, so `gpio_write()`/`gpio_read()`/`gpio_toggle()` go back to being ordinary global
    functions instead of struct fields.
  * Same fields, same `gpio_write()`/`gpio_read()`/`gpio_toggle()` signatures as L09; A.2's
    `enable_pci`/`pcmskx`/`pcicrx` additions don't carry over either. This section is about
    opacity, not interrupts.
* What's new relative to L09:
  * The struct itself moves out of `gpio.h` and into `gpio.c`.
  * `gpio_init()` is replaced by a `malloc()`-based `gpio_create()`, since callers can no longer
    declare their own `gpio_t` and pass its address once the struct's size and layout aren't
    visible to them.

`gpio.h`, the only file calling code includes:

```c
/**
 * @brief GPIO driver for ATmega328p.
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

/**
 * @brief GPIO driver structure.
 */
typedef struct gpio gpio_t;   // incomplete type: fields are not visible here

/**
 * @brief Allocate and configure a new GPIO.
 *
 * @param[in] ddrx  Pointer to the data direction register associated with the GPIO.
 * @param[in] portx Pointer to the output register associated with the GPIO.
 * @param[in] pinx  Pointer to the input register associated with the GPIO.
 * @param[in] pin   Bit position of the GPIO pin within the associated port.
 * @param[in] mode  GPIO mode.
 *
 * @return Pointer to the new GPIO, or NULL if allocation failed.
 */
gpio_t* gpio_create(volatile uint8_t* ddrx, volatile uint8_t* portx, volatile uint8_t* pinx,
                     uint8_t pin, gpio_mode_t mode);

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

`gpio.c`, the only file that ever sees `gpio_t`'s actual fields:

```c
#include "driver/gpio.h"
#include <stddef.h>
#include <stdlib.h>

struct gpio
{
    volatile uint8_t* ddrx;
    volatile uint8_t* portx;
    volatile uint8_t* pinx;
    uint8_t pin;
};

gpio_t* gpio_create(volatile uint8_t* ddrx, volatile uint8_t* portx, volatile uint8_t* pinx,
                     const uint8_t pin, const gpio_mode_t mode)
{
    gpio_t* self = (gpio_t*)(malloc(sizeof(gpio_t)));
    if (NULL == self) return NULL;   // allocation failed

    self->ddrx = ddrx;
    self->portx = portx;
    self->pinx = pinx;
    self->pin = pin;

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
    return self;
}

void gpio_write(gpio_t* self, const bool state)
{
    if (NULL == self) return;
    const bool output = (*self->ddrx & (1U << self->pin));
    if (!output) return;

    if (state) { *self->portx |= (1U << self->pin); }
    else { *self->portx &= ~(1U << self->pin); }
}

bool gpio_read(const gpio_t* self)
{
    return (NULL != self) ? (*self->pinx & (1U << self->pin)) : false;
}

void gpio_toggle(gpio_t* self)
{
    if (NULL == self) return;
    const bool output = (*self->ddrx & (1U << self->pin));
    if (!output) return;
    *self->pinx = (1U << self->pin);
}
```

* `gpio_create()` calls `malloc()` once per instance rather than reserving a fixed-size static
  array:
  * A pool sized for the worst case, up to 14 possible Arduino pins, reserves that much RAM on
    every board, even ones that only ever create two or three GPIOs; `malloc()` only spends what
    a program actually asks for.
  * This isn't the "avoid dynamic memory" advice AVR programs usually follow, that's really
    about avoiding *repeated* allocation and `free()` churn at runtime. `gpio_create()` is
    called once per driver instance, at startup, and the instance lives for the rest of the
    program; there's no resizing or fragmentation risk to worry about here.
  * `gpio_create()` returning `NULL` on allocation failure is the caller's only signal that the
    heap ran out, same as any other `malloc()` call.

---

## A.4 Function pointers
* [L01 Appendix A.8](../../L01/appendix/a_c_fundamentals.md#a8-function-pointers) covered the
  syntax: a variable that holds a function's address, callable through the variable exactly like
  calling the function by name.
* What's new here is storing one in a struct, which lets a driver call back into code it knows
  nothing about.

Pairing one of A.3's `gpio_t` inputs with a callback:

```c
/**
 * @brief Button structure.
 */
typedef struct
{
    /** GPIO associated with the button. */
    gpio_t* pin;

    /**
     * @brief Handle press events.
     */
    void (*on_press)(void); // Called from main(), not the ISR; see L06 Appendix A.6.
} button_t;

/**
 * @brief Check for button press event, invoke its callback if one occurred.
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

`button_t`'s code never mentions what `on_press` actually does, toggle an LED, send a UART message,
anything with a matching signature can be plugged in without touching `button_check()`.

---

## A.5 Worked example: an interrupt-driven `gpio_t` with event flags
* A.4's `button_t` took an edge flag as a parameter, wherever it came from was left unspecified.
  This demo wires the same idea into a real pin-change interrupt.
* A.3's `gpio_t` gains:
  * A `gpio_callback_t cb` field.
  * `pcmskx`/`pcicrx` (the port's pin-change mask register and its enable bit in `PCICR`).
  * Three new functions that arm, flag, and dispatch the interrupt.

`gpio_enable_pci()` arms the interrupt for one GPIO:

```c
void gpio_enable_pci(gpio_t* self)
{
    if (NULL == self) { return; }
    SET(PCICR, self->pcicrx);      // enable interrupts on the associated I/O port
    SET(*self->pcmskx, self->pin); // enable interrupts on the pin
    sei();                         // enable interrupts globally
}
```

* Following [L06 Appendix
  A.6](../../L06/appendix/a_uart.md#a6-example-button-toggles-an-led-reports-over-serial)'s
  discipline, none of the three ISRs (one per I/O port) do anything beyond flagging that
  *something* happened.
* `event_flags` is `volatile` because it's written from an ISR and read from `main()`'s loop, the
  exact situation `volatile` exists for:

```c
/** Event flags for each I/O port (set from an ISR, cleared once handled). */
static volatile uint8_t event_flags = 0U;

static inline void flag_event(const uint8_t port)
{
    if (PORT_COUNT > port) { SET(event_flags, port); }
}

ISR(PCINT0_vect) { flag_event(PORT_OFFSET_B); } // port B: pins 8-13
ISR(PCINT1_vect) { flag_event(PORT_OFFSET_C); } // port C: pins 14-19
ISR(PCINT2_vect) { flag_event(PORT_OFFSET_D); } // port D: pins 0-7
```

Deciding *what* to do about a flagged event, and actually calling the callback, happens entirely
outside the ISR, in code `main()`'s loop drives:

```c
bool gpio_event_occurred(const gpio_t* self)
{
    return NULL != self ? READ(event_flags, self->pcicrx) : false;
}

bool gpio_handle_event(gpio_t* self)
{
    bool handled = false;
    if (gpio_event_occurred(self))
    {
        if (NULL != self->cb)
        {
            self->cb();
            handled = true;
            CLEAR(event_flags, self->pcicrx);
        }
    }
    return handled;
}
```

`gpio_event_occurred()` reads `event_flags` by `pcicrx` (`PCIE0`/`PCIE1`/`PCIE2`), while the ISRs
above flag it by `PORT_OFFSET_B`/`_C`/`_D` (`0`/`1`/`2`); these only line up because the ATmega328p
happens to number `PCIE0`-`PCIE2` the same way. Worth noticing when reading unfamiliar register
code: two differently-named constants lining up by coincidence of the datasheet, not by a shared
definition.

```
encapsulation_demo/
├── Makefile
├── main.c
├── include/
│   └── driver/
│       ├── gpio.h
│       └── utils.h
└── source/
    └── driver/
        └── gpio.c
```

* [driver/gpio.h](../encapsulation_demo/include/driver/gpio.h) and
  [driver/gpio.c](../encapsulation_demo/source/driver/gpio.c) are the full driver, including
  `gpio_new()`/`gpio_del()`, extended with the callback and interrupt-handling pieces shown
  above.
* [driver/utils.h](../encapsulation_demo/include/driver/utils.h) has the `SET`/`CLEAR`/`TOGGLE`/
  `READ` register macros used throughout.

`main.c` configures pin 9 as an LED output and pin 13 as a button input with its pull-up enabled,
registers `button_event()` as the button's callback, and lets pin-change interrupts drive it:

```c
/**
 * @brief Demonstrate GPIO usage with interrupt-driven callback handling.
 */
#include <stddef.h>

#include "driver/gpio.h"

/** Use GPIO pin 9 for the LED. */
#define LED_PIN 9U

/** Use GPIO pin 13 for the button. */
#define BUTTON_PIN 13U

/** GPIO devices (file-global so they can be accessed from the interrupt callback). */
static gpio_t *led1, *btn1;

/**
 * @brief Handle button event.
 *
 *        Toggle the LED when the button is pressed.
 */
static void button_event(void)
{
    if (gpio_read(btn1)) { gpio_toggle(led1); }
}

/**
 * @brief Application entry point.
 *
 * @return 0 on termination of the program (should never occur), or -1 on failure.
 */
int main(void)
{
    // Initialize LED GPIO (output, no callback), return -1 on failure.
    led1 = gpio_new(LED_PIN, GPIO_MODE_OUTPUT, NULL);
    if (NULL == led1) { return -1; }

    // Initialize button GPIO (input with pull-up, use callback), return -1 on failure.
    btn1 = gpio_new(BUTTON_PIN, GPIO_MODE_INPUT_PULLUP, button_event);
    if (NULL == btn1) { return -1; }

    // Enable pin change interrupt for the button.
    gpio_enable_pci(btn1);

    // Keep program running.
    while (1)
    {
        // Handle event if occurred.
        gpio_handle_event(btn1);
    }

    // Unreachable in this example (infinite loop). Shown for completeness.
    gpio_del(&led1);
    gpio_del(&btn1);
    return 0;
}
```

`button_event()` itself never runs inside an ISR, it only ever runs from `gpio_handle_event()`,
called from `main()`'s loop; the ISRs above never touch `led1`, `btn1`, or the callback at all.

Build and flash the same way as [L09 Appendix
A.5](../../L09/appendix/a_structs_as_drivers.md#a5-worked-example-gpio_t-wired-into-l06-l08s-demo):
on Linux, `cd ../encapsulation_demo` and run `make` (`make flash` to program the board); on
Windows via Microchip Studio, recreate this folder structure inside the project and add `include`
as an include directory.

---

## A.6 Vtables: a `gpio_interface_t` interface
* A.3's `gpio_t` hardcodes real ATmega328P registers into every instance:
  * There's no way to run code that uses it without a chip attached.
  * There's no way to substitute a fake GPIO for host-side testing.
* The fix is a struct of function pointers, an **interface**, with each concrete implementation,
  real hardware, or an in-memory stub, providing its own vtable:

```c
/** GPIO interface. */
typedef struct gpio_interface gpio_interface_t;

/**
 * @brief GPIO virtual table.
 */
typedef struct gpio_vtable
{
    void (*del)(gpio_interface_t** self);
    bool (*read)(const gpio_interface_t* self);
    void (*write)(gpio_interface_t* self, bool value);
    void (*toggle)(gpio_interface_t* self);
} gpio_vtable_t;

/**
 * @brief GPIO interface.
 */
typedef struct gpio_interface
{
    const gpio_vtable_t* vptr;   // pointer to the concrete implementation's vtable
} gpio_interface_t;
```

Unlike A.3/A.4's `gpio_t`, `gpio_interface_t` has no register pointers, no pin, nothing
hardware-specific at all, just a pointer to a table of functions. Every operation goes through
it: `self->vptr->toggle(self)` instead of `gpio_toggle(self)`.

A concrete type implements the interface by embedding `gpio_interface_t` as its *first* field:

```c
typedef struct gpio_impl
{
    gpio_interface_t itf;   // must be first: makes gpio_impl_t* safely reinterpretable as
                             // gpio_interface_t*
    volatile uint8_t* ddrx;
    volatile uint8_t* portx;
    volatile uint8_t* pinx;
    uint8_t pin;
    uint8_t id;
} gpio_impl_t;
```

* Because `itf` is the struct's first member, C guarantees a `gpio_impl_t*` and a pointer to its
  first field share the same address, so a `gpio_impl_t*` can be cast to `gpio_interface_t*` and
  back without adjusting the pointer.
* That's the entire trick a C++ compiler automates for single inheritance: an object's address
  *is* its base class's address, and `self->vptr` is exactly the hidden vtable pointer every
  polymorphic C++ object carries.

Each vtable function casts back to the concrete type, then operates on its own fields exactly
like A.3's `gpio_write()`/`gpio_toggle()` did:

```c
static inline gpio_impl_t* get_impl(gpio_interface_t* self) { return (gpio_impl_t*)(self); }

static void gpio_toggle(gpio_interface_t* self)
{
    gpio_impl_t* impl = get_impl(self);
    if ((NULL == impl) || !gpio_impl_is_output(impl)) { return; }
    *(impl->pinx) |= (1U << impl->pin);
}

static const gpio_vtable_t* gpio_vptr_get_instance(void)
{
    static const gpio_vtable_t vtable = {
        .del = gpio_del, .read = gpio_read, .write = gpio_write, .toggle = gpio_toggle,
    };
    return &vtable;   // one shared vtable; every instance's itf.vptr points at this same address
}

gpio_interface_t* gpio_atmega328p_new(const uint8_t pin, const gpio_mode_t mode)
{
    gpio_impl_t* impl = (gpio_impl_t*)(malloc(sizeof(gpio_impl_t)));
    if (NULL == impl) { return NULL; }
    // ... configure impl->ddrx/portx/pinx/pin from pin, same mapping A.3's gpio_create() did ...
    impl->itf.vptr = gpio_vptr_get_instance();
    return (gpio_interface_t*)(impl);
}
```

* Like A.3's `gpio_create()`, `gpio_atmega328p_new()` calls `malloc()`, one instance per call,
  never resized, never repeatedly allocated:
  * What's actually new here isn't the allocation strategy, it's what can sit behind the
    pointer: `gpio_interface_t*` can point at a `gpio_impl_t` or a `gpio_stub_t`, two
    differently-sized concrete structs. A.3 only ever had one struct shape (`struct gpio`) to
    allocate.
* `del()` takes `gpio_interface_t**` rather than `gpio_interface_t*`:
  * It can `free()` the instance and null out the caller's own pointer in one call, guarding
    against a second `del()` or a use-after-free through that same variable.
  * That's a different defense than [L09 Appendix B
    Exercise 3](../../L09/appendix/b_exercises.md) considered for `gpio_free()` (whether the
    freed instance's *own* fields should be reset), but the same underlying worry: a pointer to
    a GPIO that no longer safely means what it used to.

A second implementation, `gpio_stub_t`, satisfies the exact same interface without touching any
hardware, a `bool state` field stands in for the registers:

```c
typedef struct gpio_stub
{
    gpio_interface_t itf;   // still first
    bool state;
} gpio_stub_t;

static bool gpio_read(const gpio_interface_t* self)
{
    const gpio_stub_t* impl = (const gpio_stub_t*)(self);
    return NULL != impl ? impl->state : false;
}
```

Calling code never knows the difference: anything holding a `gpio_interface_t*` can call
`self->vptr->read(self)`, whether `self` came from `gpio_atmega328p_new()` or `gpio_stub_new()`.

This is the same mechanism higher-level languages build `virtual` functions on top of: an
object with virtual methods carries a hidden pointer to a table of function addresses (its
vtable), and calling a virtual method is, under the hood, exactly the `self->vptr->toggle(...)`
pattern above, just generated by the compiler instead of written out by hand.

```
interface_demo/
├── Makefile
├── main.c
├── include/
│   └── driver/
│       └── gpio/
│           ├── atmega328p.h
│           ├── interface.h
│           └── stub.h
└── source/
    └── driver/
        └── gpio/
            ├── atmega328p.c
            └── stub.c
```

* [driver/gpio/interface.h](../interface_demo/include/driver/gpio/interface.h) is the full
  interface shown above.
* [driver/gpio/atmega328p.h](../interface_demo/include/driver/gpio/atmega328p.h)/
  [.c](../interface_demo/source/driver/gpio/atmega328p.c) and
  [driver/gpio/stub.h](../interface_demo/include/driver/gpio/stub.h)/
  [.c](../interface_demo/source/driver/gpio/stub.c) are the two implementations in full.
  * Including the pin-registry reservation `gpio_atmega328p_new()` uses: the same idea as [L09
    Appendix B Exercise 5](../../L09/appendix/b_exercises.md)'s `pin_reg`, just widened from a
    `uint16_t` covering pins 0-13 to a `uint32_t` covering pins 0-19, since this demo's pin
    mapping also reaches `PORTC`.

`main.c` toggles a real LED on D8 whenever a *simulated* button, a `gpio_stub_t` whose state
flips every 60,000 main-loop iterations instead of being wired to real hardware, reports a
rising edge:

```c
/**
 * @brief GPIO driver example with interfaces in C.
 */
#include "driver/gpio/atmega328p.h"
#include "driver/gpio/stub.h"

/** LED pin. */
#define LED_PIN 8U

/** Max value of the loop counter used to simulate button events. */
#define LOOP_COUNTER_MAX 60000UL

/**
 * @brief Simulate button events.
 *
 * @param[out] button Pointer to the simulated button.
 */
void simulate_button_event(gpio_interface_t* button)
{
    static uint16_t loop_counter = 0U;

    // Simulate a button toggle every LOOP_COUNTER_MAX calls.
    if (LOOP_COUNTER_MAX <= ++loop_counter)
    {
        button->vptr->toggle(button);
        loop_counter = 0U;
    }
}

/**
 * @brief Toggle an LED connected to ATmega328p at pressdown (rising edge) of a simulated button.
 *
 * @return Unused (this example never returns).
 */
int main(void)
{
    // Create and initialize GPIO instances.
    gpio_interface_t* led    = gpio_atmega328p_new(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_interface_t* button = gpio_stub_new();
    bool button_prev         = false;

    while (1)
    {
        // Simulate button events every LOOP_COUNTER_MAX main-loop iterations.
        simulate_button_event(button);

        // Detect pressdown (rising edge).
        const bool button_current = button->vptr->read(button);
        const bool button_pressed = button_current && !button_prev;

        // Toggle the LED on button pressdown (rising edge).
        if (button_pressed) { led->vptr->toggle(led); }
        button_prev = button_current;
    }
    // Unreachable in this example (infinite loop). Shown for completeness.
    led->vptr->del(&led);
    button->vptr->del(&button);
    return 0;
}
```

Neither `simulate_button_event()` nor `main()`'s edge-detection logic cares that `led` is real
hardware and `button` isn't: both are just `gpio_interface_t*`. Swap `gpio_stub_new()` for a
second `gpio_atmega328p_new()` call and a real button, and nothing else in this file changes.

Build and flash the same way as [L09 Appendix
A.5](../../L09/appendix/a_structs_as_drivers.md#a5-worked-example-gpio_t-wired-into-l06-l08s-demo):
on Linux, `cd ../interface_demo` and run `make` (`make flash` to program the board); on Windows
via Microchip Studio, recreate this folder structure inside the project and add `include` as an
include directory.

---
