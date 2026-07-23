/**
 * @brief GPIO driver stub.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio/interface.h"

/**
 * @brief Create new GPIO stub.
 *
 * @return The new GPIO instance, or a nullptr on failure.
 */
gpio_interface_t* gpio_stub_new(void);
