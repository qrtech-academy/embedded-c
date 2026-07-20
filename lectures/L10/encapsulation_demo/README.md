# Interrupt-driven GPIO with event flags
An opaque `gpio_t` driver extended with a stored callback and pin-change interrupt support.

This example demonstrates encapsulation (`gpio_t`'s fields are hidden behind `driver/gpio.h`)
combined with a function pointer stored in a struct, dispatched safely from `main()`'s loop. The
interrupt service routine only records that an event occurred; `gpio_handle_event()` is what
actually invokes the callback.

The implementation is written for compilation on an ATmega328P in Microchip Studio.

The example demonstrates:
* An opaque driver type whose internal state cannot be accessed directly by application code.
* Dynamic creation and destruction of GPIO driver instances through `gpio_new()` and
  `gpio_del()`.
* Configuration of GPIO pins as inputs or outputs using `gpio_mode_t`.
* Basic digital GPIO operations:
  * Writing a logical value to an output.
  * Reading the current value of a pin.
  * Toggling an output pin.
* Registration of a callback function that is associated with a GPIO instance.
* Use of the ATmega328P's pin-change interrupt system.
* Enabling and disabling pin-change interrupts for individual GPIO devices.
* Minimal interrupt service routines that avoid executing application-level callback code.
* Communication between interrupt context and the main program through shared event flags.
* Deferred callback execution through `gpio_handle_event()`.
* Separation between:
  * Detecting an interrupt.
  * Recording the event.
  * Handling the event.
  * Performing the application-specific response.
* A main-loop event-dispatch pattern suitable for small bare-metal embedded systems.
* Safe handling of relatively slow application logic outside the interrupt service routine.
* Reuse of the same GPIO abstraction for both the input button and the output LED.

---

## Files
* [driver/gpio.h](./include/driver/gpio.h):
  * Contains the GPIO driver's public interface.
  * Forward-declares `gpio_t` without exposing the definition of `struct gpio`.
  * Keeps register pointers, pin numbers, callback pointers, and other internal state private.
  * Defines the `gpio_mode_t` enum used to select the pin's operating mode.
  * Declares the lifetime-management functions:
    * `gpio_new()`
    * `gpio_del()`
  * Declares the basic GPIO operations:
    * `gpio_write()`
    * `gpio_read()`
    * `gpio_toggle()`
  * Declares the pin-change interrupt control functions:
    * `gpio_enable_pci()`
    * `gpio_disable_pci()`
  * Declares the event-processing functions:
    * `gpio_event_occurred()`
    * `gpio_handle_event()`
  * Provides the only interface that application code needs in order to use the driver.
  * Prevents application code from depending on the driver's internal representation.
* [driver/gpio.c](./source/driver/gpio.c):
  * Defines the private `struct gpio`.
  * Stores the hardware information needed to operate each GPIO pin.
  * Stores the callback function associated with an interrupt-driven GPIO instance.
  * Implements allocation and initialization performed by `gpio_new()`.
  * Implements cleanup performed by `gpio_del()`.
  * Implements GPIO register access for reading, writing, and toggling pins.
  * Maps each GPIO pin to the appropriate ATmega328P I/O registers.
  * Configures the corresponding pin-change interrupt mask bit.
  * Enables or disables the appropriate pin-change interrupt group.
  * Maintains the shared event flags used to communicate with the main loop.
  * Implements `gpio_event_occurred()` for checking whether an event is pending.
  * Implements `gpio_handle_event()` for clearing the pending event and invoking the stored
    callback.
  * Contains the pin-change interrupt handlers:
    * `ISR(PCINT0_vect)`
    * `ISR(PCINT1_vect)`
    * `ISR(PCINT2_vect)`
  * Keeps the interrupt handlers intentionally small.
  * Records events in interrupt context without directly calling application code.
  * Centralizes all ATmega328P-specific GPIO and interrupt details inside the driver
    implementation.
* [driver/utils.h](./include/driver/utils.h):
  * Contains function-like macros for common bit operations.
  * Provides macros for setting bits in a hardware register.
  * Provides macros for clearing bits in a hardware register.
  * Provides macros for toggling bits in a hardware register.
  * Provides macros for reading individual bits from a hardware register.
  * Makes low-level register operations more readable.
  * Reduces repetition in the GPIO driver implementation.
* [main.c](./main.c):
  * Demonstrates how application code uses the GPIO driver without accessing its internal
    fields.
  * Creates one GPIO instance for an LED.
  * Creates another GPIO instance for a push button.
  * Configures the LED pin as an output.
  * Configures the button pin as an input.
  * Registers a callback for the button GPIO.
  * Enables the button's pin-change interrupt.
  * Enables global interrupts after initialization is complete.
  * Polls for pending GPIO events in the main loop.
  * Calls `gpio_handle_event()` from normal program context.
  * Toggles the LED from the registered callback when the button event is handled.
  * Demonstrates deferred interrupt processing rather than executing the complete response
    inside the ISR.
  * Releases the GPIO objects through the driver's public interface when cleanup is required.

---
