/**
 * @file EEPROM driver.
 */
#ifndef DRIVER_EEPROM_H_
#define DRIVER_EEPROM_H_

#include <stdint.h>

/**
 * @brief Write data to EEPROM.
 *
 * @param[in] buf Buffer containing the data to write.
 * @param[in] buflen Number of bytes to write.
 * @param[in] addr Address of the first EEPROM byte to write.
 *
 * @return Number of bytes written, or -1 on failure.
 */
int16_t eeprom_write(const uint8_t* buf, uint16_t buflen, uint16_t addr);

/**
 * @brief Read data from EEPROM.
 *
 * @param[out] buf Buffer in which to store the data read.
 * @param[in] buflen Number of bytes to read.
 * @param[in] addr Address of the first EEPROM byte to read.
 *
 * @return Number of bytes read, or -1 on failure.
 */
int16_t eeprom_read(uint8_t* buf, uint16_t buflen, uint16_t addr);

#endif /** DRIVER_EEPROM_H_ */
