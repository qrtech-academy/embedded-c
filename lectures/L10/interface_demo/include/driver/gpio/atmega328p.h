/**
 * @brief GPIO driver for ATmega328p.
 */
#ifndef DRIVER_GPIO_ATMEGA328P_H_
#define DRIVER_GPIO_ATMEGA328P_H_

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio/interface.h"

/**
 * @brief Create new GPIO.
 *
 * @param[in] pin Arduino pin number.
 * @param[in] mode GPIO mode.
 *
 * @return The new GPIO instance, or NULL on failure.
 */
gpio_interface_t* gpio_atmega328p_new(uint8_t pin, gpio_mode_t mode);

#endif /* DRIVER_GPIO_ATMEGA328P_H_ */
