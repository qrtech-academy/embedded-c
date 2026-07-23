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
