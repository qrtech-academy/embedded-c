/**
 * @brief Implementation details for ATmega328p GPIO driver.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "driver/gpio.h"
#include "driver/utils.h"

/** Number of available pins. */
#define PIN_COUNT 20U

/** Pin offset for I/O port B. */
#define PIN_OFFSET_B 8U

/** Pin offset for I/O port C. */
#define PIN_OFFSET_C 14U

/** Pin offset for I/O port D. */
#define PIN_OFFSET_D 0U

/** Number of avilable ports. */
#define PORT_COUNT 3U

/** Port offset for I/O port B. */
#define PORT_OFFSET_B 0U

/** Port offset for I/O port C. */
#define PORT_OFFSET_C 1U

/** Port offset for I/O port D. */
#define PORT_OFFSET_D 2U

/**
 * @brief GPIO driver structure.
 */
typedef struct gpio
{
    /** Pointer to the data mode register associated with the GPIO. */
    volatile uint8_t* ddrx;

    /** Pointer to the output register associated with the GPIO. */
    volatile uint8_t* portx;

    /** Pointer to the input register associated with the GPIO. */
    volatile uint8_t* pinx;

    /** Pointer to the pin change mask register associated with the GPIO. */
    volatile uint8_t* pcmskx;

    /** Pointer to the callback associated with the GPIO. */
    gpio_callback_t cb;

    /** Pin change interrupt control register bit (PCIEx) associated with the GPIO's port. */
    uint8_t pcicrx;

    /** Bit position of the GPIO pin within the associated port. */
    uint8_t pin;

    /** Pin ID (for reserving the pin). */
    uint8_t id;
} gpio_t;

/** Pin registry (1 = pin is reserved, 0 = pin is free). */
static uint32_t pin_reg = 0U;

/** Event flags for each I/O port. */
static volatile uint8_t event_flags = 0U;

// -----------------------------------------------------------------------------
static inline bool is_pin_free(const uint8_t pin)
{
    // Return true if the pin number is valid and the pin is free.
    return PIN_COUNT > pin ? !READ(pin_reg, pin) : false;
}

// -----------------------------------------------------------------------------
static inline bool is_mode_valid(const gpio_mode_t mode)
{
    // Cast to uint8_t to avoid negative integers.
    return (uint8_t)(GPIO_MODE_OUTPUT) >= (uint8_t)(mode);
}

// -----------------------------------------------------------------------------
static inline void flag_event(const uint8_t port)
{
    // Flag event on the given port.
    if (PORT_COUNT > port) { SET(event_flags, port); }
}

// -----------------------------------------------------------------------------
static void gpio_init_attributes(gpio_t* self, const uint8_t pin)
{
    // Store pin ID (for pin registration).
    self->id = pin;

    // PIN 0 - 7 => I/O port D, pin = ID.
    if (PIN_OFFSET_B > pin)
    {
        self->ddrx   = &DDRD;
        self->portx  = &PORTD;
        self->pinx   = &PIND;
        self->pin    = pin - PIN_OFFSET_D;
        self->pcmskx = &PCMSK2;
        self->pcicrx = PCIE2;
    }
    // PIN 8 - 13 => I/O port B, pin = ID - 8.
    else if (PIN_OFFSET_C > pin)
    {
        self->ddrx   = &DDRB;
        self->portx  = &PORTB;
        self->pinx   = &PINB;
        self->pin    = pin - PIN_OFFSET_B;
        self->pcmskx = &PCMSK0;
        self->pcicrx = PCIE0;
    }
    // PIN 14 - 19 => I/O port C, pin = ID - 14.
    else if (PIN_COUNT > pin)
    {
        self->ddrx   = &DDRC;
        self->portx  = &PORTC;
        self->pinx   = &PINC;
        self->pin    = pin - PIN_OFFSET_C;
        self->pcmskx = &PCMSK1;
        self->pcicrx = PCIE1;
    }
}

// -----------------------------------------------------------------------------
static void gpio_set_mode(gpio_t* self, const gpio_mode_t mode)
{
    // Set GPIO mode.
    switch (mode)
    {
        case GPIO_MODE_INPUT_PULLUP:
        {
            // Configure as input by clearing the corresponding bit in DDRx.
            // Enable pull-up resistor by setting the corresponding bit in PORTx.
            CLEAR(*self->ddrx, self->pin);
            SET(*self->portx, self->pin);
            break;
        }
        case GPIO_MODE_OUTPUT:
        {
            // Configure as output by setting the corresponding bit in DDRx.
            SET(*self->ddrx, self->pin);
            CLEAR(*self->portx, self->pin);
            break;
        }
        default:
        {
            // Configure as input by clearing the corresponding bit in DDRx.
            CLEAR(*self->portx, self->pin);
            CLEAR(*self->ddrx, self->pin);
            break;
        }
    }
}

// -----------------------------------------------------------------------------
gpio_t* gpio_new(const uint8_t pin, const gpio_mode_t mode, gpio_callback_t cb)
{
    // Check the pin, return NULL if reserved or invalid.
    if (!is_pin_free(pin)) { return NULL; }

    // Check GPIO mode, return a nullptr if invalid.
    if (!is_mode_valid(mode)) { return NULL; }

    // Allocate memory for a new GPIO, return a nullptr on failure.
    gpio_t* self = (gpio_t*)(malloc(sizeof(gpio_t)));
    if (NULL == self) { return NULL; }

    // Initialize GPIO.
    gpio_init_attributes(self, pin);
    gpio_set_mode(self, mode);
    self->cb = cb;

    // Register pin, then return a pointer to the GPIO.
    SET(pin_reg, self->id);
    return self;
}

// -----------------------------------------------------------------------------
void gpio_del(gpio_t** self)
{
    // Check the GPIO instance, terminate the function if invalid.
    if (NULL == self) { return; }

    // Retrieve the GPIO implementation, terminate the function if invalid.
    gpio_t* gpio = *self;
    if (NULL == gpio) { return; }

    // Reset/clear used hardware (DDRx and PORTx).
    CLEAR(*gpio->ddrx, gpio->pin);
    CLEAR(*gpio->portx, gpio->pin);

    // Release the pin from the pin registry.
    CLEAR(pin_reg, gpio->id);

    // Free allocated memory and set the associated pointer to NULL.
    free(gpio);
    *self = NULL;
}

// -----------------------------------------------------------------------------
void gpio_write(gpio_t* self, const bool state)
{
    // Terminate the function is the GPIO is invalid.
    if (NULL == self) { return; }

    // Check data mode, terminate the function if not configured as output.
    if (!READ(*(self->ddrx), self->pin)) { return; }

    // Check the desired state, set/clear the bit in PORTx.
    if (state) { SET(*self->portx, self->pin); }
    else { CLEAR(*self->portx, self->pin); }
}

// -----------------------------------------------------------------------------
bool gpio_read(const gpio_t* self)
{
    // Read PINx if the GPIO is valid, otherwise return false.
    return NULL != self ? READ(*self->pinx, self->pin) : false;
}

// -----------------------------------------------------------------------------
void gpio_toggle(gpio_t* self)
{
    // Terminate the function is the GPIO is invalid.
    if (NULL == self) { return; }

    // Check data mode, terminate the function if not configured as output.
    if (!READ(*self->ddrx, self->pin)) { return; }

    // Toggle the output by setting the pin in PINx (the hardware will toggle the output).
    SET(*self->pinx, self->pin);
}

// -----------------------------------------------------------------------------
void gpio_enable_pci(gpio_t* self)
{
    // Terminate the function is the GPIO is invalid.
    if (NULL == self) { return; }

    // Enable interrupts on the associated I/O-port.
    SET(PCICR, self->pcicrx);

    // Enable interrupts on the pin.
    SET(*self->pcmskx, self->pin);

    // Enable interrupts globally.
    sei();
}

// -----------------------------------------------------------------------------
void gpio_disable_pci(gpio_t* self, bool disable_port)
{
    // Terminate the function is the GPIO is invalid.
    if (NULL == self) { return; }

    // Disable interrupts on the pin.
    CLEAR(*self->pcmskx, self->pin);

    // Disable interrupts on the entire I/O-port if specified.
    if (disable_port) { CLEAR(PCICR, self->pcicrx); }
}

// -----------------------------------------------------------------------------
bool gpio_event_occurred(const gpio_t* self)
{
    return NULL != self ? READ(event_flags, self->pcicrx) : false;
}

// -----------------------------------------------------------------------------
bool gpio_handle_event(gpio_t* self)
{
    bool handled = false;

    // Check event flag, invoke callback if an event has occurred.
    if (gpio_event_occurred(self))
    {
        // Invoke callback if an event has occurred, mark as handled and clear the event flag.
        if (NULL != self->cb)
        {
            self->cb();
            handled = true;
            CLEAR(event_flags, self->pcicrx);
        }
    };
    // Return true if the event was handled, false otherwise.
    return handled;
}

// -----------------------------------------------------------------------------
ISR(PCINT0_vect)
{
    // Flag event on I/O port B.
    flag_event(PORT_OFFSET_B);
}

// -----------------------------------------------------------------------------
ISR(PCINT1_vect)
{
    // Flag event on I/O port C.
    flag_event(PORT_OFFSET_C);
}

// -----------------------------------------------------------------------------
ISR(PCINT2_vect)
{
    // Flag event on I/O port D.
    flag_event(PORT_OFFSET_D);
}
