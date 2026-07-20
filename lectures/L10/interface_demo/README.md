# Interfaces and vtables in C
A hand-written interface (`gpio_interface_t`) backed by two independent concrete GPIO
implementations—real ATmega328P hardware and an in-memory stub—that satisfy the same contract and
can be used interchangeably.

This example demonstrates, in simplified form, how C++ commonly implements `virtual` functions
and single inheritance under the hood. The interface object contains only a pointer to a table of
function pointers. Each concrete type embeds that interface object as its first field, allowing a
pointer to the concrete object to be used as a pointer to the interface without changing its
address.

Calling code holds only a `gpio_interface_t*` and dispatches operations through
`self->vptr->...`. It does not know whether the object controls a physical ATmega328P pin or stores
its state entirely in memory.

The implementation is written for compilation on an ATmega328P in Microchip Studio.

The example demonstrates:
* A hand-written GPIO interface named `gpio_interface_t`.
* An interface object containing only a pointer to a vtable.
* A vtable type, `gpio_vtable_t`, containing the operations that every implementation must
  provide:
  * `del()`
  * `read()`
  * `write()`
  * `toggle()`
* Dynamic dispatch implemented explicitly through function pointers.
* Calls written in the form:
  * `self->vptr->read(self)`
  * `self->vptr->write(self, value)`
  * `self->vptr->toggle(self)`
  * `self->vptr->del(&self)`
* A calling style similar to virtual member-function calls in C++.
* An explicit `self` parameter serving the same purpose as C++'s implicit `this` pointer.
* A `const` vtable pointer that prevents an instance from modifying the shared table through its
  interface.
* One vtable created per concrete implementation rather than one vtable per object.
* A vtable typically stored as a `static const` object inside the concrete implementation file.
* Every object of the same concrete type sharing the same vtable.
* Per-instance state stored separately from per-type behavior.
* Two independent implementations satisfying the same interface:
  * A real ATmega328P GPIO implementation.
  * A hardware-independent in-memory stub.
* Substitutability: either implementation can be stored in a `gpio_interface_t*`.
* Calling code that depends on the interface rather than on a specific implementation.
* Manual polymorphism: the same interface call reaches different concrete code depending on which
  constructor created the object.
* Runtime selection of behavior through the object's `vptr`.
* No `switch` statement or implementation-type flag required in the calling code.
* No direct dependency from `main.c` on the private fields of either concrete implementation.
* The common-first-member layout pattern:
  * `gpio_impl_t` stores `gpio_interface_t` as its first field.
  * `gpio_stub_t` also stores `gpio_interface_t` as its first field.
  * The address of either concrete object is therefore also the address of its embedded
    interface object.
* Upcasting from a concrete implementation pointer to `gpio_interface_t*`.
* Downcasting inside a concrete operation from `gpio_interface_t*` back to the known concrete
  type.
* The requirement that a downcast is only valid when the interface pointer actually refers to an
  object of that concrete type.
* A simple representation of single inheritance:
  * The interface forms the common base portion.
  * The concrete struct extends it with implementation-specific state.
* Separate constructors for separate concrete types:
  * `gpio_atmega328p_new()`
  * `gpio_stub_new()`
* Constructors returning the same public pointer type despite allocating different concrete
  structs.
* Heap-based object creation with `malloc()`.
* One independent object allocated for every successful constructor call.
* Constructor failure reported through `NULL`.
* Validation of constructor arguments before returning a usable object.
* Pairing every successful constructor call with the interface's `del()` operation.
* A destructor-like function reached through the vtable.
* Passing `gpio_interface_t**` to `del()` so it can:
  * Destroy the concrete object.
  * Release implementation-specific resources.
  * Free the allocated memory.
  * Set the caller's pointer to `NULL`.
* Reducing the risk of accidentally retaining a dangling pointer after deletion.
* Implementation-specific cleanup hidden behind the common interface.
* A real GPIO implementation that owns a physical ATmega328P pin.
* Mapping Arduino pin numbers to the appropriate:
  * `DDRB`, `DDRC`, or `DDRD` register.
  * `PORTB`, `PORTC`, or `PORTD` register.
  * `PINB`, `PINC`, or `PIND` register.
  * Bit position within the selected port.
* Support for Arduino digital pins across `PORTB`, `PORTC`, and `PORTD`.
* A pin-registry reservation scheme that models exclusive ownership of hardware resources.
* Prevention of two live GPIO objects from controlling the same physical pin.
* A constructor returning `NULL` when the requested pin is already reserved.
* Release of the pin reservation when the object is deleted.
* Resetting the physical pin to a neutral state during deletion.
* Resource acquisition during construction and release during destruction.
* An ownership pattern resembling RAII, although cleanup must still be requested explicitly in C.
* A stub implementation that requires no GPIO registers or microcontroller hardware.
* An in-memory `bool state` representing the logical level of the stub pin.
* Stub operations implementing the same observable contract as the real GPIO driver:
  * `write()` updates the stored state.
  * `read()` returns the stored state.
  * `toggle()` inverts the stored state.
  * `del()` destroys the stub object.
* A stub whose internal representation differs completely from the hardware implementation.
* Interface-based application code that remains unchanged when one implementation is substituted
  for the other.
* Dependency inversion:
  * High-level edge-detection logic depends on `gpio_interface_t`.
  * Hardware-specific register access remains in the ATmega328P implementation.
* Host-testable GPIO-dependent logic.
* The ability to exercise button and edge-detection behavior without a physical input pin.
* Simulated button events standing in for real electrical input.
* A simple rising-edge detector that compares the current and previous button states.
* Separation between:
  * Obtaining a GPIO value.
  * Detecting an application-level event.
  * Responding to the event.
* A demonstration that test doubles are easier to introduce when application code depends on an
  interface.
* A comparison point for related designs:
  * A public struct with ordinary functions.
  * Function pointers stored directly in each instance.
  * An opaque concrete driver type.
  * A separate shared interface and vtable.
  * C++ abstract base classes and virtual member functions.
* The costs associated with manual polymorphism:
  * An indirect function call for every dispatched operation.
  * One `vptr` stored in every object.
  * Additional constructor and destructor code.
  * Dynamic allocation in this implementation.
  * More complex type and lifetime rules than a simple procedural driver.
* The benefits associated with the design:
  * Interchangeable implementations.
  * Cleaner separation between hardware and application logic.
  * Easier use of stubs and test doubles.
  * Private concrete state.
  * Reduced coupling to ATmega328P-specific details.

---

## Files
* [driver/gpio/interface.h](./include/driver/gpio/interface.h):
  * Contains the implementation-independent GPIO contract.
  * Defines `gpio_mode_t`.
  * Defines `gpio_vtable_t`.
  * Declares the complete set of operations that a concrete GPIO implementation must provide:
    * `del()`
    * `read()`
    * `write()`
    * `toggle()`
  * Defines the signatures of all GPIO operation pointers.
  * Uses `gpio_interface_t*` as the common `self` parameter type.
  * Defines `gpio_interface_t`.
  * Stores only `const gpio_vtable_t* vptr` in the interface object.
  * Contains no ATmega328P register pointers.
  * Contains no pin number or hardware-specific state.
  * Contains no stub-specific state.
  * Allows calling code to dispatch operations without knowing the concrete object layout.
  * Is the only GPIO header required by code that receives an already-created interface object.
  * Serves a role similar to an abstract base class in C++.
  * Describes behavior without selecting an implementation.
  * Allows additional GPIO implementations to be added without changing the interface-using
    logic.
* [driver/gpio/atmega328p.h](./include/driver/gpio/atmega328p.h) /
  [driver/gpio/atmega328p.c](./source/driver/gpio/atmega328p.c):
  * Declare and define `gpio_atmega328p_new(uint8_t pin, gpio_mode_t mode)`.
  * Provide the concrete implementation backed by real ATmega328P GPIO registers.
  * Return a `gpio_interface_t*` rather than exposing the private concrete type.
  * Keep `gpio_impl_t` private to `atmega328p.c`.
  * Define `gpio_impl_t` with `gpio_interface_t` as its first field.
  * Extend the embedded interface with hardware-specific state:
    * `ddrx`
    * `portx`
    * `pinx`
    * `pin`
    * `id`
  * Store register pointers for the selected hardware port.
  * Store the bit position used within those registers.
  * Store an identifier used to release the pin from the registry.
  * Define a `static const gpio_vtable_t` for the ATmega328P implementation.
  * Assign the ATmega328P vtable to the embedded interface during construction.
  * Implement private concrete operations for:
    * Reading a hardware pin.
    * Writing a hardware pin.
    * Toggling a hardware pin.
    * Deleting the hardware-backed object.
  * Convert the incoming interface pointer back to `gpio_impl_t*` inside each concrete
    operation.
  * Validate the supplied Arduino pin number.
  * Map the pin number to `PORTB`, `PORTC`, or `PORTD`.
  * Select the corresponding `DDRx` and `PINx` registers.
  * Calculate the bit position within the selected port.
  * Configure the pin according to the requested `gpio_mode_t`.
  * Allocate one private `gpio_impl_t` object with `malloc()`.
  * Return `NULL` if allocation fails.
  * Maintain a pin registry representing currently owned physical pins.
  * Check the registry before claiming a pin.
  * Return `NULL` if another live object already owns the requested pin.
  * Mark the pin as reserved after successful validation and allocation.
  * Prevent conflicting GPIO instances from silently modifying the same hardware.
  * Implement concrete deletion through the interface:
    * Validate the supplied pointer.
    * Recover the concrete `gpio_impl_t`.
    * Reset the pin's data-direction and output state.
    * Release the pin from `pin_registry`.
    * Free the concrete allocation.
    * Set the caller's interface pointer to `NULL`.
  * Keep register mapping, reservation, and cleanup details hidden from interface-using code.
* [driver/gpio/stub.h](./include/driver/gpio/stub.h) /
  [driver/gpio/stub.c](./source/driver/gpio/stub.c):
  * Declare and define `gpio_stub_new(void)`.
  * Provide a concrete implementation backed only by memory.
  * Return the same public type, `gpio_interface_t*`, as the hardware constructor.
  * Keep `gpio_stub_t` private to `stub.c`.
  * Define `gpio_stub_t` with `gpio_interface_t` as its first field.
  * Extend the embedded interface with a `bool state`.
  * Define a separate `static const gpio_vtable_t` for the stub implementation.
  * Assign the stub vtable during construction.
  * Allocate one `gpio_stub_t` object with `malloc()`.
  * Initialize its logical state.
  * Return `NULL` if allocation fails.
  * Implement `write()` by assigning the in-memory state.
  * Implement `read()` by returning the in-memory state.
  * Implement `toggle()` by inverting the state.
  * Implement `del()` by freeing the stub allocation and nulling the caller's pointer.
  * Perform no hardware register access.
  * Require no ATmega328P GPIO pin.
  * Require no external wiring.
  * Satisfy the same behavioral contract as the real GPIO implementation.
  * Allow higher-level logic to be exercised in a deterministic software-only environment.
* [main.c](./main.c):
  * Uses `gpio_interface_t*` for both GPIO objects.
  * Creates one real LED with `gpio_atmega328p_new()`.
  * Assigns the LED to Arduino digital pin D8.
  * Configures the LED pin as an output.
  * Creates one simulated button with `gpio_stub_new()`.
  * Checks both constructor results before using the returned objects.
  * Does not access the private fields of either concrete implementation.
  * Does not need to know the size of either concrete struct.
  * Calls every GPIO operation through `self->vptr`.
  * Implements `simulate_btn_event()`.
  * Toggles the stub button after approximately every 60,000 main-loop iterations.
  * Uses the simulated change in state as a substitute for a physical button press.
  * Stores the previous button state.
  * Reads the current button state through the interface.
  * Detects a rising edge when:
    * The previous state was low.
    * The current state is high.
  * Toggles the physical LED when a rising edge is detected.
  * Performs both the button read and LED toggle through the same interface contract.
  * Demonstrates one application simultaneously using two different concrete implementations.
  * Contains no special branch for distinguishing the real object from the stub object.
  * Demonstrates that dynamic dispatch is determined by each object's vtable.
  * Deletes both objects through their interface-provided `del()` operations.
  * Leaves implementation-specific cleanup to the concrete types.
  * Demonstrates hardware-independent application logic built on top of a C interface.

---
