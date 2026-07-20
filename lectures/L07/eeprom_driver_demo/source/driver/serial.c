/**
 * @file Serial driver implementation details.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <avr/io.h>

#include "driver/serial.h"

#define BAUD_RATE_VAL 103U   // Baud rate value, corresponds to 9600 bps.
#define NEW_LINE '\n'        // New line character.
#define CARRIAGE_RETURN '\r' // Carriage return.

// -----------------------------------------------------------------------------
static void serial_write_byte(const uint8_t byte) {
  // Wait for the previous byte to be transmitted.
  while (0U == (UCSR0A & (1U << UDRE0))) {
  }

  // Place the new byte in the post box.
  UDR0 = byte;
}

// -----------------------------------------------------------------------------
void serial_init(void) {
  // Enable transmission of UART.
  UCSR0B = (1U << TXEN0);

  // Set character size to eight bits.
  UCSR0C = (1U << UCSZ00) | (1U << UCSZ01);

  // Set baud rate to 9600 bps.
  UBRR0 = BAUD_RATE_VAL;
}

// -----------------------------------------------------------------------------
int16_t serial_write(const uint8_t *buf, const uint16_t buflen) {
  // Check the buffer, return -1 if NULL.
  if (NULL == buf) {
    return -1;
  }
  uint16_t i = 0U;

  // Transmit each byte one by one.
  for (; i < buflen; ++i) {
    serial_write_byte(buf[i]);
  }
  // Return the number of transmitted bytes.
  return i;
}

// -----------------------------------------------------------------------------
int16_t serial_print(const char *text) {
  // Check the text, return -1 if NULL.
  if (NULL == text) {
    return -1;
  }
  uint16_t i = 0U;

  // Transmit each character one by one.
  for (; text[i]; ++i) {
    const char byte = text[i];

    // Ensure CRLF.
    if (NEW_LINE == byte) {
      serial_write_byte(CARRIAGE_RETURN);
    }
    serial_write_byte((uint8_t)(byte));
  }
  // Return the number of transmitted bytes.
  return i;
}
