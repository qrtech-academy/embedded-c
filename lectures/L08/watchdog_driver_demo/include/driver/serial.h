/**
 * @file Serial driver.
 */
#ifndef DRIVER_SERIAL_H_
#define DRIVER_SERIAL_H_

#include <stdint.h>

/**
 * @brief Initialize the serial driver.
 */
void serial_init(void);

/**
 * @brief Transmit data over UART.
 *
 * @param[in] buf Buffer holding serial data to write.
 * @param[in] buflen Size of buf in bytes.
 *
 * @return Number of bytes transmitted, or -1 if buf is NULL.
 */
int16_t serial_write(const uint8_t* buf, uint16_t buflen);

/**
 * @brief Transmit text over UART.
 *
 * @param[in] text Text to transmit.
 *
 * @return Number of bytes consumed from text, or -1 if text  is NULL.
 *         Injected \r bytes are not counted.
 */
int16_t serial_print(const char* text);

#endif /** DRIVER_SERIAL_H_ */
