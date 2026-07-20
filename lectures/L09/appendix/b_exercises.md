# Appendix B - Exercises

## Concepts
**1.** What problem does bundling a pin number into a `gpio_t` struct solve that a `#define`d
`LED_ON` macro can't? What has to change to support a second LED under each approach?

---

**2.** `gpio_read()` takes `const gpio_t* self` rather than `gpio_t self` (the whole struct, by
value). What would change, functionally or in cost, if it took `gpio_t self` instead? Would
`gpio_init()` still work correctly if it took `self` by value?

---

## Program
**3.** Add `gpio_free(gpio_t* self)` to A.2's driver:
* Reset the pin's `DDRx`/`PORTx` bits back to their power-on state (input, pull-up disabled).
* Decide whether `self->ddrx`, `self->portx`, and `self->pinx` should be set to `NULL`
  afterwards.
* Justify the choice:
  * What does calling `gpio_write()` or `gpio_toggle()` on a freed instance do in each case?
  * Which is preferable?

---

**4.** Rewrite `gpio_init()` so callers pass a single pin number, 0-13, instead of
`DDRx`/`PORTx`/`PINx` pointers:
* Pin mapping:
  * 0-7 map to `PORTD0`-`PORTD7`.
  * 8-13 map to `PORTB0`-`PORTB5`.
* `gpio_init()` should compute and store `ddrx`/`portx`/`pinx` (and the bit position) itself.
* Return `bool`:
  * `false` for any pin number outside 0-13, without touching any register.
  * `true` otherwise, after initialization.
* `pin` is `const` in A.2's `gpio_t` because it's only ever set at struct-literal time; now that
  `gpio_init()` derives it from the pin number instead, does it still make sense for `pin` to
  stay `const`?

---

**5.** Add a pin-reservation table:
* Declare a `static uint16_t pin_reg` in `gpio.c`.
* Use one bit for each pin number introduced in Exercise 4:
  * Bit *n* represents pin *n*.
  * `1` means that the pin is reserved.
  * `0` means that the pin is free.
* Update `gpio_init()`:
  * Check the bit corresponding to the requested pin.
  * If the bit is already set, return `false`.
  * In that case, do not modify any hardware register.
  * If the bit is clear, set it to reserve the pin.
  * Then initialize the GPIO as in Exercise 4.
  * Return `true` when the initialization succeeds.
* Extend the `gpio_free()` function from Exercise 3:
  * Clear the pin's bit in `pin_reg`.
  * This allows a later call to `gpio_init()` to reserve the same pin again.
* Answer the following questions:
  * Why must `pin_reg` be declared `static`?
  * What would break if it were not `static`?

---

**6.** Capstone — harder: Implement a timer driver named `timer_t`:
* Follow the header/source separation used in Appendix A.3:
  * `driver/timer.h`
  * `driver/timer.c`
* The timer must run a callback function whenever its timeout expires.
* `timer_t` should follow the same ownership model as `gpio_t`:
  * The caller declares and owns each `timer_t` instance.
  * The caller may declare an array of timer instances, just as Appendix A.4 declares an array of `gpio_t` instances.
  * `timer_init()` initializes an existing instance in place.
* The ATmega328P has only three hardware timer/counters:
  * At most three `timer_t` instances may therefore be active at the same time.
  * Use a reservation scheme similar to Exercise 5's `pin_reg`.
  * `timer_init()` must not create more active timer instances than there are hardware timer/counters available.
* `timer_t` is a complete timer driver instance:
  * Each instance has:
    * Its own callback.
    * Its own elapsed-time count.
    * Its own start/stop state.
    * One reserved hardware timer/counter.
  * It is driven directly by the hardware timer/counter and its compare-match interrupt.

### Recommended `driver/timer.h`
`timer_callback_t` is a function pointer, see [L01 Appendix
A.8](../../L01/appendix/a_c_fundamentals.md#a8-function-pointers) if that syntax is unfamiliar.
Document every declaration with Doxygen comments, the same way A.2/A.3's `gpio.h` does, and add
the header guard yourself (suggested guard: `DRIVER_TIMER_H_`).

```c
#include <stdbool.h>
#include <stdint.h>

/** Alias for timer callbacks. */
typedef void (*timer_callback_t)(void);

typedef struct
{
    // Left for you to design.
} timer_t;

bool timer_init(timer_t* self, uint32_t timeout_ms, timer_callback_t callback);
void timer_clear(timer_t* self);
bool timer_running(const timer_t* self);
void timer_start(timer_t* self);
void timer_stop(timer_t* self);
void timer_toggle(timer_t* self);
void timer_reset(timer_t* self);
```

### Initialization and reservation
* `timer_init()` must return `false` when:
  * `timeout_ms == 0`.
  * `callback == NULL`.
  * No hardware timer/counter is available.
* Otherwise, `timer_init()` must:
  * Reserve one free hardware timer/counter.
  * Associate it with `self`.
  * Store the timeout and callback.
  * Initialize the timer instance.
  * Return `true`.
* `timer_clear()` must:
  * Stop the timer.
  * Release its reserved hardware timer/counter.
  * Allow that hardware timer/counter to be claimed by a later `timer_init()` call.
  * Work whether the later call uses the same `timer_t` instance or another one.
* Consider the same design question raised for `gpio_free()` in Exercise 3:
  * Should `timer_clear()` also reset the fields of `self`?
  * If so, which values would leave the instance in a safe, inert state?

### Timer control
* `timer_start()` must start or enable the timer.
  * It must not change the elapsed-time count.
* `timer_stop()` must stop or disable the timer.
  * It must not change the elapsed-time count.
* `timer_toggle()` must switch between the running and stopped states.
  * It must not change the elapsed-time count.
* `timer_reset()` must:
  * Set the elapsed-time count to zero.
  * Preserve the timer's current running or stopped state.

### Driver independence
* The timer driver must not know that `gpio_t` exists.
* `timer.c` must not directly control an LED or read a button.
* Connecting a timer callback to an LED is the responsibility of `main.c`.

### Hardware demonstration
Wire the following circuit:
* Connect a button to D13.
* Connect an LED to D8.
* Configure a timer with a timeout of 100 ms.

The program must behave as follows:
* Pressing the button toggles the timer between running and stopped.
* While the timer is running:
  * Its callback runs every 100 ms.
  * The callback toggles the LED.
* While the timer is stopped:
  * The LED remains off.
  * Calling the callback manually must have no effect.
* To enforce this behavior, the callback itself must check `timer_running()` before toggling the LED.

### Testing
Confirm `timer_init()` returns `false` for each of the three invalid cases in "Initialization and
reservation": a `0` timeout, a `NULL` callback, and a fourth `timer_t` once three hardware
timer/counters are already reserved. Then confirm `timer_clear()` actually frees a hardware
timer/counter for reuse: after reserving all three, clear one and check that a new `timer_init()`
call succeeds again.

---
