/**
 * @file EEPROM driver implementation details.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#define EEPROM_ADDR_MAX 1023U // Highest address in EEPROM.

// -----------------------------------------------------------------------------
static inline bool addr_valid(const uint16_t addr) {
  return EEPROM_ADDR_MAX >= addr;
}

// -----------------------------------------------------------------------------
static bool eeprom_write_byte(const uint16_t addr, const uint8_t byte) {
  // Return false if the address is invalid.
  if (!addr_valid(addr)) {
    return false;
  }

  // Wait for any previous write to finish.
  while (EECR & (1U << EEPE)) {
  }

  // Set destination address and data.
  EEAR = addr;
  EEDR = byte;

  // Perform write operation, then return true to indicate success.
  cli();
  EECR |= (1U << EEMPE);
  EECR |= (1U << EEPE);
  sei();
  return true;
}

// -----------------------------------------------------------------------------
static bool eeprom_read_byte(const uint16_t addr, uint8_t *byte) {
  // Return false if the address is invalid or if byte is NULL.
  if (!addr_valid(addr) || (NULL == byte)) {
    return false;
  }

  // Set the address to read from.
  EEAR = addr;

  // Perform read operation.
  EECR |= (1U << EERE);

  // Store the retrieved byte, then return true to indicate success.
  *byte = EEDR;
  return true;
}

// -----------------------------------------------------------------------------
int16_t eeprom_write(const uint8_t *buf, const uint16_t buflen,
                     const uint16_t addr) {
  // Check the input arguments, return -1 if invalid.
  if ((NULL == buf) || (0U == buflen) || !addr_valid(addr)) {
    return -1;
  }
  uint16_t i = 0U;

  // Transmit each byte one by one.
  for (; i < buflen; ++i) {
    // Terminate operation on write failure (indicates out of range).
    if (!eeprom_write_byte(addr + i, buf[i])) {
      break;
    }
  }
  // Return the number of bytes written.
  return (int16_t)(i);
}

// -----------------------------------------------------------------------------
int16_t eeprom_read(uint8_t *buf, const uint16_t buflen, const uint16_t addr) {
  // Check the input arguments, return -1 if invalid.
  if ((NULL == buf) || (0U == buflen) || !addr_valid(addr)) {
    return -1;
  }
  uint16_t i = 0U;

  // Receive bytes one by one.
  for (; i < buflen; ++i) {
    // Terminate operation on read failure (indicates out of range).
    if (!eeprom_read_byte(addr + i, buf + i)) {
      break;
    }
  }
  // Return the number of bytes read.
  return (int16_t)(i);
}
