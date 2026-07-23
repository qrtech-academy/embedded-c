/**
 * @file Serial driver implementation details.
 */
#include <stddef.h>
#include <stdint.h>

#include <avr/io.h>

#include "driver/serial.h"

#define BAUD 103U            // UBRR value for 9600 bps with a 16 MHz clock.
#define NEW_LINE '\n'        // New line character.
#define CARRIAGE_RETURN '\r' // Carriage return.

// -----------------------------------------------------------------------------
static inline void serial_write_byte(const uint8_t byte)
{
    // Wait until the transmit buffer is ready for a new byte.
    while (0U == (UCSR0A & (1U << UDRE0))) {}

    // Place the new byte in the transmit data register.
    UDR0 = byte;
}

// -----------------------------------------------------------------------------
void serial_init(void)
{
    // Enable UART transmission.
    UCSR0B = (1U << TXEN0);

    // Set character size to eight bits.
    UCSR0C = (1U << UCSZ00) | (1U << UCSZ01);

    // Set baud rate to 9600 bps.
    UBRR0 = BAUD;
}

// -----------------------------------------------------------------------------
int16_t serial_write(const uint8_t* buf, const uint16_t buflen)
{
    // Check the input parameters, return -1 if invalid.
    if ((NULL == buf) || (0U == buflen)) { return -1; }
    uint16_t i = 0U;

    // Transmit each byte one by one.
    for (; i < buflen; ++i)
    {
        serial_write_byte(buf[i]);
    }
    // Return the number of transmitted bytes.
    return (int16_t)(i);
}

// -----------------------------------------------------------------------------
int16_t serial_print(const char* text)
{
    // Check the text, return -1 if invalid.
    if (NULL == text) { return -1; }
    uint16_t i = 0U;

    // Transmit each character one by one.
    for (; text[i]; ++i)
    {
        const char byte = text[i];

        // Convert each newline character from LF to CRLF.
        if (NEW_LINE == byte) { serial_write_byte(CARRIAGE_RETURN); }
        serial_write_byte((uint8_t)(byte));
    }
    // Return the number of characters processed.
    return (int16_t)(i);
}
