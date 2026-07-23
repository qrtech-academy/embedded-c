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

    /** Pointer to the pin change mask register associated with the GPIO. */
    volatile uint8_t* pcmskx;

    /** Pin change interrupt control register bit (PCIEx) associated with the GPIO's port. */
    const uint8_t pcicrx;

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

/**
 * @brief Enable/disable pin change interrupts for the given GPIO.
 *
 * @param[in] self GPIO instance.
 * @param[in] enable True to enable pin change interrupts, false otherwise.
 */
void gpio_enable_pci(gpio_t* self, bool enable);

#endif /** DRIVER_GPIO_H_ */
