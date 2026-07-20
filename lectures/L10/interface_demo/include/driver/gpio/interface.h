/**
 * @brief GPIO interface.h
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Enumeration of GPIO modes.
 */
typedef enum {
  GPIO_MODE_INPUT,        ///< Standard GPIO input.
  GPIO_MODE_INPUT_PULLUP, ///< GPIO input with its internal pull-up enabled.
  GPIO_MODE_OUTPUT,       ///< GPIO output.
} gpio_mode_t;

/** GPIO interface. */
typedef struct gpio_interface gpio_interface_t;

/**
 * @brief GPIO virtual table.
 */
typedef struct gpio_vtable {
  /**
   * @brief Delete given GPIO.
   *
   *        Relase allocated resources and set the corresponding pointer to
   * null.
   *
   * @param[in] self Double pointer to the GPIO to delete.
   */
  void (*del)(gpio_interface_t **self);

  /**
   * @brief Read input of given GPIO.
   *
   * @param[in] self The GPIO to read.
   *
   * @return True if the GPIO is enabled, false otherwise.
   */
  bool (*read)(const gpio_interface_t *self);

  /**
   * @brief Write output of given GPIO.
   *
   *        This operation is only supported for outputs.
   *
   * @param[in] self The GPIO to write.
   * @param[in] value The value to write.
   */
  void (*write)(gpio_interface_t *self, bool value);

  /**
   * @brief Toggle output of given GPIO.
   *
   *        This operation is only supported for outputs.
   *
   * @param[in] self The GPIO to toggle.
   */
  void (*toggle)(gpio_interface_t *self);
} gpio_vtable_t;

/**
 * @brief GPIO interface.
 */
typedef struct gpio_interface {
  /** Pointer to the GPIO virtual table. */
  const gpio_vtable_t *vptr;
} gpio_interface_t;
