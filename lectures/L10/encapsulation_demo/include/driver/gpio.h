/**
 * @brief GPIO driver for ATmega328p.
 */
#ifndef GPIO_H_
#define GPIO_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Enumeration of GPIO modes.
 */
typedef enum gpio_mode {
  GPIO_MODE_INPUT,        ///< Standard GPIO input.
  GPIO_MODE_INPUT_PULLUP, ///< GPIO input with its internal pull-up enabled.
  GPIO_MODE_OUTPUT,       ///< GPIO output.
} gpio_mode_t;

/**
 * @brief GPIO driver structure.
 */
typedef struct gpio gpio_t;

/** GPIO callback. */
typedef void (*gpio_callback_t)(void);

/**
 * @brief Create a new GPIO.
 *
 * @param[in] pin Arduino pin number.
 * @param[in] mode GPIO mode.
 * @param[in] cb Optional callback. Pass NULL if none.
 *
 * @return The initialized GPIO on success, a nullptr otherwise.
 */
gpio_t *gpio_new(uint8_t pin, gpio_mode_t mode, gpio_callback_t cb);

/**
 * @brief Delete GPIO instance.
 *
 *        Release resources allocated for the GPIO and set the associated
 * pointer to null.
 *
 * @param[in, out] self Reference to the GPIO instance.
 */
void gpio_del(gpio_t **self);

/**
 * @brief Set state of the GPIO.
 *
 * @note This operation is only supported for outputs.
 *
 * @param[in, out] self GPIO instance.
 * @param[in] state GPIO state (true = enabled, false = disabled).
 */
void gpio_write(gpio_t *self, bool state);

/**
 * @brief Read the state of the GPIO.
 *
 * @param[in] self GPIO instance.
 *
 * @return True if the GPIO is enabled, false otherwise.
 */
bool gpio_read(const gpio_t *self);

/**
 * @brief Toggle state of the GPIO.
 *
 * @note This operation is only supported for outputs.
 *
 * @param[in, out] self GPIO instance.
 */
void gpio_toggle(gpio_t *self);

/**
 * @brief Enable pin change interrupt for the given GPIO.
 *
 * @param[in, out] self GPIO instance.
 */
void gpio_enable_pci(gpio_t *self);

/**
 * @brief Disable pin change interrupts for the given GPIO.
 *
 * @param[in, out] self GPIO instance.
 * @param[in] disable_port True to disable interrupts on the entire I/O port.
 */
void gpio_disable_pci(gpio_t *self, bool disable_port);

/**
 * @brief Check if a GPIO event has occurred on the associated port.
 *
 * @return True if a GPIO event on the associated port has occurred, false
 * otherwise.
 */
bool gpio_event_occurred(const gpio_t *self);

/**
 * @brief Handle GPIO event on the associated port.
 *
 * @param[in, out] self GPIO instance.
 *
 * @return True if a GPIO event on the associated port was handled, false
 * otherwise.
 *
 * @note A callback is needed to handle occurring events.
 */
bool gpio_handle_event(gpio_t *self);

#endif /* GPIO_H_ */
