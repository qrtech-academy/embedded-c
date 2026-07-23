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

    /**
     * @brief Set state of the given GPIO.
     *
     * @param[in] self GPIO instance.
     * @param[in] state GPIO state (true = high, false = low).
     *
     * @note This operation is only supported for GPIOs configured as outputs.
     */
    void (*write)(struct gpio* self, bool state);

    /**
     * @brief Read state of the given GPIO.
     *
     * @param[in] self GPIO instance.
     *
     * @return GPIO state (true = high, false = low).
     */
    bool (*read)(const struct gpio* self);
    /**
     * @brief Toggle state of the given GPIO.
     *
     * @param[in] self GPIO instance.
     */
    void (*toggle)(struct gpio* self);

    /**
     * @brief Enable/disable pin change interrupts for the given GPIO.
     *
     * @param[in] self GPIO instance.
     * @param[in] enable True to enable pin change interrupts, false otherwise.
     */
    void (*enable_pci)(struct gpio* self, bool enable);
} gpio_t;

/**
 * @brief Initialize GPIO.
 *
 * @param[in] self GPIO instance.
 * @param[in] pin Digital pin to use Must be in range [0, 13].
 * @param[in] mode GPIO mode.
 *
 * @return True on success, false on failure.
 */
bool gpio_init(gpio_t* self, uint8_t pin, gpio_mode_t mode);

#endif /** DRIVER_GPIO_H_ */
