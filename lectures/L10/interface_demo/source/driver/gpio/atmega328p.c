/**
 * @brief GPIO driver implementation details for ATmega328p.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <avr/io.h>

#include "driver/gpio/atmega328p.h"

#define PIN_COUNT 20U // The number of available pins.

/** I/O port offsets (when mapping against Arduino pins). */
#define PIN_OFFSET_B 8U  // Pin offset for I/O port B (pin 9 = PORTB1).
#define PIN_OFFSET_C 14U // Pin offset for I/O port C (pin 14 = PORTC0).
#define PIN_OFFSET_D 0U  // Pin offset for I/O port D (pin 5 = PORTD5).

/**
 * @brief GPIO structure.
 */
typedef struct gpio_impl {
  /** GPIO interface. */
  gpio_interface_t itf;

  /** Pointer to the data direction register. */
  volatile uint8_t *ddrx;

  /** Pointer to the port register. */
  volatile uint8_t *portx;

  /** Pointer to the pin register. */
  volatile uint8_t *pinx;

  /** Pin on the associated I/O port. */
  uint8_t pin;

  /** Pin ID (equal to the Arduino Uno pin). */
  uint8_t id;
} gpio_impl_t;

/** Pin registry holding the state of each pin (1 = reserved, 0 = free). */
static uint32_t pin_registry = 0U;

/** Get a pointer to the GPIO virtual table. */
static const gpio_vtable_t *gpio_vptr_get_instance(void);

// -----------------------------------------------------------------------------
static inline gpio_impl_t *get_impl(gpio_interface_t *self) {
  return (gpio_impl_t *)(self);
}

// -----------------------------------------------------------------------------
static inline const gpio_impl_t *get_const_impl(const gpio_interface_t *self) {
  return (const gpio_impl_t *)(self);
}

// -----------------------------------------------------------------------------
static bool is_pin_free(const uint8_t id) {
  // Return true if the corresponding bit in the pin registry is 0, false
  // otherwise.
  return PIN_COUNT > id ? (0U == (pin_registry & (1U << id))) : false;
}

// -----------------------------------------------------------------------------
static void reserve_pin(const uint8_t id) {
  // Set the corresponding bit in the pin registry.
  pin_registry |= (1U << id);
}

// -----------------------------------------------------------------------------
static void release_pin(const uint8_t id) {
  // Clear the corresponding bit in the pin registry.
  pin_registry &= ~(1U << id);
}

// -----------------------------------------------------------------------------
static bool gpio_impl_is_output(const gpio_impl_t *impl) {
  // Retrieve implementation details, return false on failure.
  return NULL != impl ? (bool)(*(impl->ddrx) & (1U << impl->pin)) : false;
}

// -----------------------------------------------------------------------------
static void gpio_impl_init(gpio_impl_t *impl, const uint8_t pin,
                           const gpio_mode_t mode) {
  // Save the pin ID for reservation and cleanup.
  impl->id = pin;

  // Initialize the vtable pointer.
  impl->itf.vptr = gpio_vptr_get_instance();

  // If 0 <= pin <= 7 => the GPIO is connected to I/O port D.
  if (PIN_OFFSET_B > pin) {
    impl->ddrx = &DDRD;
    impl->portx = &PORTD;
    impl->pinx = &PIND;
    impl->pin = pin - PIN_OFFSET_D;
  }
  // Else if 8 <= pin <= 13 => the GPIO is connected to I/O port B.
  else if (PIN_OFFSET_C > pin) {
    impl->ddrx = &DDRB;
    impl->portx = &PORTB;
    impl->pinx = &PINB;
    impl->pin = pin - PIN_OFFSET_B;
  }
  // Else if 14 <= pin <= 19 => the GPIO is connected to I/O port C.
  else {
    impl->ddrx = &DDRC;
    impl->portx = &PORTC;
    impl->pinx = &PINC;
    impl->pin = pin - PIN_OFFSET_C;
  }

  // Set GPIO mode.
  switch (mode) {
  case GPIO_MODE_INPUT_PULLUP: {
    // Clear the associated bit in DDRx (configure as input).
    // Set the associated bit in PORTx (enable pull-up).
    *(impl->ddrx) &= ~(1U << impl->pin);
    *(impl->portx) |= (1U << impl->pin);
    break;
  }
  case GPIO_MODE_OUTPUT: {
    // Set the associated bit in DDRx (configure as output).
    // Clear the associated bit in PORTx (set the initial output value to 0).
    *(impl->ddrx) |= (1U << impl->pin);
    *(impl->portx) &= ~(1U << impl->pin);
    break;
  }
  default: {
    // Clear the associated bits in DDRx and PORTx as default.
    *(impl->ddrx) &= ~(1U << impl->pin);
    *(impl->portx) &= ~(1U << impl->pin);
    break;
  }
  }
  // Reserve the pin before terminating the function.
  reserve_pin(pin);
}

// -----------------------------------------------------------------------------
static void gpio_impl_cleanup(gpio_impl_t *impl) {
  // Clear the associated bits in the GPIO registers and the pin registry.
  *(impl->ddrx) &= ~(1U << impl->pin);
  *(impl->portx) &= ~(1U << impl->pin);
  release_pin(impl->id);
}

// -----------------------------------------------------------------------------
static void gpio_del(gpio_interface_t **self) {
  // Terminate the function if self is a nullptr.
  if (NULL == self) {
    return;
  }

  // Retrieve implementation details, terminate the function on failure.
  gpio_impl_t *impl = get_impl(*self);
  if (NULL == impl) {
    return;
  }

  // Clean up GPIO registers and the pin registry.
  gpio_impl_cleanup(impl);

  // Free allocated memory.
  free(impl);

  // Set the associated pointer to null.
  *self = NULL;
}

// -----------------------------------------------------------------------------
static bool gpio_read(const gpio_interface_t *self) {
  // Retrieve implementation details.
  const gpio_impl_t *impl = get_const_impl(self);

  // Return true if the input is high, false otherwise.
  return NULL != impl ? (bool)(*(impl->pinx) & (1U << impl->pin)) : false;
}

// -----------------------------------------------------------------------------
static void gpio_write(gpio_interface_t *self, const bool value) {
  // Retrieve implementation details.
  // Terminate the function on failure or if the GPIO is configured as input.
  gpio_impl_t *impl = get_impl(self);
  if ((NULL == impl) || !gpio_impl_is_output(impl)) {
    return;
  }

  // Set/clear the output in accordance with the given value.
  if (value) {
    *(impl->portx) |= (1U << impl->pin);
  } else {
    *(impl->portx) &= ~(1U << impl->pin);
  }
}

// -----------------------------------------------------------------------------
static void gpio_toggle(gpio_interface_t *self) {
  // Retrieve implementation details.
  // Terminate the function on failure or if the GPIO is configured as input.
  gpio_impl_t *impl = get_impl(self);
  if ((NULL == impl) || !gpio_impl_is_output(impl)) {
    return;
  }

  // Toggle the output by writing to PINx (the hardware will toggle the output
  // of PORTx if the pin is configured as output).
  *(impl->pinx) |= (1U << impl->pin);
}

// -----------------------------------------------------------------------------
static const gpio_vtable_t *gpio_vptr_get_instance(void) {
  // Create and initialize vtable holding function pointers (done once at
  // startup).
  static const gpio_vtable_t vtable = {
      .del = gpio_del,
      .read = gpio_read,
      .write = gpio_write,
      .toggle = gpio_toggle,
  };
  // Return a pointer to the vtable.
  return &vtable;
}

// -----------------------------------------------------------------------------
gpio_interface_t *gpio_atmega328p_new(const uint8_t pin,
                                      const gpio_mode_t mode) {
  // Return a nullptr if the pin is reserved.
  if (!is_pin_free(pin)) {
    return NULL;
  }

  // Create new GPIO instance, return nullptr on failure.
  gpio_impl_t *impl = (gpio_impl_t *)(malloc(sizeof(gpio_impl_t)));
  if (NULL == impl) {
    return NULL;
  }

  // Initialize the GPIO instance.
  gpio_impl_init(impl, pin, mode);
  impl->itf.vptr = gpio_vptr_get_instance();

  // Return the initialized GPIO instance, cast to the corresponding interface.
  return (gpio_interface_t *)(impl);
}
