# Function pointers on a still-public GPIO driver
A `gpio_t` driver where the fields stay fully public, but `write`/`read`/`toggle`/`enable_pci`
are reached only through function pointers stored on the struct itself.

This example demonstrates encapsulation through `static` internal linkage rather than through an
opaque struct. The fields of `gpio_t`, including its function pointers, remain visible and
assignable from any source file that includes `driver/gpio.h`. Only the concrete functions behind
those pointers are hidden, because their definitions have internal linkage inside `gpio.c`.

`gpio_init()` accepts an Arduino digital pin number and derives the relevant ATmega328P registers,
bit positions, and pin-change interrupt configuration. It also configures the pin and assigns all
function pointers in a single initialization call.

The implementation is written for compilation on an ATmega328P in Microchip Studio.

The example demonstrates:
* A GPIO driver whose complete struct definition remains part of the public interface.
* Direct caller ownership of each `gpio_t` instance:
  * The application declares the objects.
  * No dynamic allocation is required.
  * `gpio_init()` initializes an existing object in place.
* A public struct whose fields can be inspected or modified by any source file that includes the
  header.
* The limitations of a public driver struct:
  * Application code can overwrite register pointers.
  * Application code can replace function pointers.
  * Application code can create an invalid or partially initialized instance.
* Encapsulation achieved through `static` internal linkage rather than through an opaque type.
* Concrete GPIO operations that cannot be called by name from outside `gpio.c`.
* A distinction between:
  * Hiding a function's symbol.
  * Hiding a struct's data.
  * Restricting how callers are expected to interact with an object.
* Storing function pointers directly as members of each `gpio_t` instance.
* An object-like calling style in C, for example:
  * `led.write(&led, true)`
  * `button.read(&button)`
  * `led.toggle(&led)`
  * `button.enable_pci(&button, true)`
* Instance-specific function dispatch without using C++ classes or virtual functions.
* Function pointers that all refer to the same internal implementations but operate on different
  `gpio_t` instances.
* Passing the instance itself to each operation so that the implementation can access its
  register pointers and pin information.
* Avoiding a separate interface or virtual-function-table struct.
* Initialization of both data fields and function pointers through a single public function.
* `gpio_init()` computing the required hardware information from one Arduino digital pin number.
* Mapping Arduino digital pins to the corresponding ATmega328P ports:
  * Pins 0-7 map to `PORTD`.
  * Pins 8-13 map to `PORTB`.
* Conversion from an Arduino pin number to the corresponding bit position within its hardware
  port.
* Selection of the correct:
  * Data-direction register (`DDRx`).
  * Output register (`PORTx`).
  * Input register (`PINx`).
  * Pin-change mask register (`PCMSKx`).
  * Pin-change interrupt group bit in `PCICR`.
* Validation of the supplied pin number before any hardware register is modified.
* `gpio_init()` returning `bool` to report whether initialization succeeded.
* Failure on an out-of-range pin number without touching the GPIO or interrupt registers.
* Wiring up all four operation pointers during successful initialization:
  * `write()`
  * `read()`
  * `toggle()`
  * `enable_pci()`
* The requirement that callers check the return value of `gpio_init()` before using the instance.
* Two fields, `pin` and `pcicrx`, that cannot be declared `const`, because their values are
  assigned during `gpio_init()` rather than when the object itself is declared.
* Runtime initialization of a struct whose hardware configuration is not known at compile time.
* Configuration of the pin's data direction and initial state according to `gpio_mode_t`.
* Reuse of the same driver type for both input and output pins.
* Use of the ATmega328P pin-change interrupt system.
* Enabling and disabling an individual pin's interrupt through the instance's own `enable_pci()` function pointer.
* Use of the pin-change interrupt mask register to arm or disarm the button pin.
* A deliberately small pin-change interrupt service routine.
* Immediate disabling of the button interrupt when an edge is detected.
* Prevention of repeated button events caused by mechanical contact bounce.
* A timer-based debounce window rather than a busy-wait delay.
* Use of Timer0's overflow interrupt to measure the debounce period.
* Separation of the two interrupt responsibilities:
  * The GPIO ISR responds to the initial button event.
  * The timer ISR re-arms the button interrupt after the debounce interval.
* Avoiding delays and polling loops inside interrupt context.
* Immediate LED toggling inside the GPIO ISR for a simple, bounded hardware response.
* Recording an event through a shared flag for later processing by the main loop.
* Deferred handling of operations that may be slow or blocking.
* Keeping EEPROM writes outside interrupt context.
* Keeping formatted UART output outside interrupt context.
* Communication between interrupt context and normal program context through an event flag.
* Persistence of the LED state across resets through the EEPROM driver.
* Restoration of the previously stored LED state during application startup.
* Reporting of the LED state over UART after each button event.
* Periodic watchdog servicing from the main loop.
* Composition of several independent drivers inside one application:
  * GPIO
  * EEPROM
  * UART
  * Watchdog
  * Hardware timer
* Reuse of drivers from earlier lectures without creating direct dependencies between them.
* A driver architecture in which the application coordinates independent modules.
* A comparison point for later designs using:
  * Ordinary public driver functions.
  * A separate interface struct.
  * An opaque driver type.
  * Dynamically allocated driver objects.
  * C++ classes and virtual member functions.

---

## Files
* [driver/gpio.h](./include/driver/gpio.h):
  * Contains the GPIO driver's public interface.
  * Defines `struct gpio` and its `gpio_t` typedef directly in the header.
  * Exposes every field of the GPIO instance to application code.
  * Stores the GPIO hardware information required by the internal operations.
  * Stores the pin's bit number within its hardware port.
  * Stores the pin-change interrupt group bit used with `PCICR`.
  * Includes four function pointers as struct members:
    * `write()`
    * `read()`
    * `toggle()`
    * `enable_pci()`
  * Defines the function-pointer signatures used by the driver operations.
  * Defines the `gpio_mode_t` enum used to select the pin's operating mode.
  * Declares `gpio_init()`.
  * Exposes `gpio_init()` as the driver's only ordinary function with external linkage.
  * Allows callers to declare individual `gpio_t` objects or arrays of GPIO objects.
  * Requires callers to initialize each object before calling any stored function pointer.
  * Demonstrates that a public struct provides convention-based rather than enforced
    encapsulation.
* [driver/gpio.c](./source/driver/gpio.c):
  * Contains the GPIO driver's hardware-specific implementation.
  * Defines `static` implementations of:
    * `gpio_write()`
    * `gpio_read()`
    * `gpio_toggle()`
    * `gpio_enable_pci()`
  * Gives these concrete functions internal linkage.
  * Prevents other translation units from calling the implementations directly by name.
  * Allows the functions to remain reachable indirectly through the pointers stored in a
    successfully initialized `gpio_t`.
  * Implements `gpio_init()`.
  * Validates the supplied Arduino pin number.
  * Returns `false` when the pin number is outside the supported range.
  * Performs validation before changing any hardware register.
  * Maps pins 0-7 to the `PORTD` register group.
  * Maps pins 8-13 to the `PORTB` register group.
  * Calculates the pin's hardware bit position.
  * Assigns the correct `DDRx`, `PORTx`, and `PINx` register pointers.
  * Assigns the appropriate pin-change mask register.
  * Assigns the correct pin-change interrupt group bit.
  * Configures the pin according to the requested `gpio_mode_t`.
  * Establishes the pin's initial output or pull-up state where required.
  * Assigns all four internal operation functions to the instance's function-pointer fields.
  * Leaves the caller-owned `gpio_t` ready for use after successful initialization.
  * Centralizes Arduino-pin-to-ATmega-register mapping in one implementation file.
* [driver/eeprom.h](./include/driver/eeprom.h) /
  [driver/eeprom.c](./source/driver/eeprom.c):
  * Contains the EEPROM driver introduced in L07.
  * Stores the current LED state in non-volatile memory.
  * Allows the application to restore the LED state after a reset or power cycle.
  * Is called only from normal program context.
  * Remains independent of the GPIO driver.
  * Does not need access to `gpio_t` or its function pointers.
* [driver/serial.h](./include/driver/serial.h) /
  [driver/serial.c](./source/driver/serial.c):
  * Contains the UART driver introduced in L06.
  * Initializes serial communication with the configured baud rate.
  * Reports whether the LED is currently on or off.
  * Performs output from the main loop rather than from an ISR.
  * Remains independent of the GPIO implementation.
  * Demonstrates composition of separate drivers at application level.
* [driver/watchdog.h](./include/driver/watchdog.h) /
  [driver/watchdog.c](./source/driver/watchdog.c):
  * Contains the watchdog driver introduced in L08.
  * Configures the watchdog during application startup.
  * Provides the operation used to reset the watchdog counter.
  * Is serviced once during every main-loop iteration.
  * Helps recover the system if the main loop stops executing normally.
  * Operates independently of the GPIO, EEPROM, and UART drivers.
* [main.c](./main.c):
  * Declares one caller-owned `gpio_t` instance for the LED on D9.
  * Declares one caller-owned `gpio_t` instance for the button on D13.
  * Initializes the EEPROM, UART, watchdog, GPIO, and timer-related hardware.
  * Checks whether each call to `gpio_init()` succeeds.
  * Restores the previously saved LED state from EEPROM.
  * Applies the restored state to the LED.
  * Arms the button's pin-change interrupt through its `enable_pci()` function pointer.
  * Enables global interrupts only after initialization is complete.
  * Defines the shared event state used by the ISRs and main loop.
  * Implements `ISR(PCINT0_vect)`:
    * Disables the button's pin-change interrupt.
    * Starts or resets the debounce timing state.
    * Toggles the LED through the GPIO driver's function pointer.
    * Sets an event flag.
    * Performs no EEPROM write.
    * Performs no UART output.
  * Implements `ISR(TIMER0_OVF_vect)`:
    * Tracks the elapsed debounce interval.
    * Re-enables the button interrupt after approximately 300 ms.
    * Stops or resets the debounce timing state when re-arming is complete.
  * Uses the timer interrupt only for debounce timing.
  * Resets the watchdog once per main-loop iteration.
  * Checks the event flag from normal program context.
  * Clears or consumes the event flag before processing the event.
  * Reads the LED's new state.
  * Writes the new state to EEPROM.
  * Reports the new state through UART.
  * Keeps potentially slow operations outside the interrupt service routines.
  * Coordinates several independent drivers without coupling their implementations together.

---
