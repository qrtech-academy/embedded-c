/**
 * @file Watchdog timer driver implementation details.
 */
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "driver/watchdog.h"

// -----------------------------------------------------------------------------
void watchdog_init(const watchdog_timeout_t timeout)
{
    // Clear the reset flag before anything else touches the watchdog.
    MCUSR &= ~(1U << WDRF);

    // Update the watchdog timeout.
    cli();
    WDTCSR |= (1U << WDCE) | (1U << WDE);
    WDTCSR = (1U << WDE) | (uint8_t)(timeout);
    sei();
}

// -----------------------------------------------------------------------------
void watchdog_reset(void) { __asm__("wdr"); }
