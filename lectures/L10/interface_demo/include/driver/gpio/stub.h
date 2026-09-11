/**
 * @brief GPIO driver stub.
 */
#ifndef DRIVER_GPIO_STUB_H_
#define DRIVER_GPIO_STUB_H_

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio/interface.h"

/**
 * @brief Create new GPIO stub.
 *
 * @return The new GPIO instance, or NULL on failure.
 */
gpio_interface_t* gpio_stub_new(void);

#endif /* DRIVER_GPIO_STUB_H_ */
