# Appendix A - EEPROM

## A.1 EEPROM: what it is and why
* RAM forgets everything the instant power is removed. EEPROM (Electrically Erasable
  Programmable ROM) doesn't: it's a separate, much smaller memory that keeps its contents
  across a reset or power loss, useful for settings, calibration values, or a counter that
  should survive being unplugged.
* The ATmega328P has 1024 bytes of EEPROM, addressed `0`-`1023`, entirely separate from flash
  (where your program lives) and RAM (where your variables live).
* Two costs come with that durability: a single byte write takes a few milliseconds (eternity,
  compared to a RAM write), and each EEPROM cell only tolerates roughly 100,000 write cycles
  before it wears out. Don't write to EEPROM in a tight loop or once per iteration of `main()`;
  write only when a value actually needs to persist.

---

## A.2 EEPROM registers
Three registers, all specific to the EEPROM peripheral:

1. **`EEAR`** (EEPROM Address Register) holds the address, `0`-`1023`, to read from or write to.
2. **`EEDR`** (EEPROM Data Register) holds the byte being written, or receives the byte just
   read.
3. **`EECR`** (EEPROM Control Register) controls and reports on the read/write process:
   * `EERE` (EEPROM Read Enable) starts a read.
   * `EEMPE` (EEPROM Master Write Enable) must be set immediately before `EEPE`, it's a safety
     interlock, not a data bit.
   * `EEPE` (EEPROM Write Enable) starts a write, and stays set until the write completes; a new
     write must wait for it to clear first.

The datasheet requires `EEPE` to be set within four CPU cycles of `EEMPE` being set, or the write
silently doesn't happen at all. An interrupt firing in that window is a real risk (a few cycles
is nothing), so a write disables interrupts for the handful of instructions between the two:

```c
while (EECR & (1U << EEPE)) {} // Wait for any earlier write to finish.

EEAR = addr; // Set destination address (0-1023).
EEDR = byte; // Set data to write (one byte).

cli();                 // Disable interrupts during the write sequence.              
EECR |= (1U << EEMPE); // EEMPE must be followed by EEPE within 4 cycles.
EECR |= (1U << EEPE);
sei();                 // Re-enable interrupts once the write sequence is complete.
```

Reading doesn't have this timing constraint, no write is happening, so there's no equivalent
interlock, just the same "wait for the peripheral to be free" pattern already familiar from
`ADIF` (L04) and `UDRE0` (L06):

```c
while (EECR & (1U << EEPE)) {} // Wait for any earlier write to finish.
EEAR = addr;                   // Set the address to read from.
EECR |= (1U << EERE);          // Start read operation.
return EEDR;                   // Return the byte at the specified address.
```

---

## A.3 `eeprom_write_byte()` / `eeprom_read_byte()`

```c
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

/**
 * @brief Write a byte to EEPROM at the given address.
 *
 * @param[in] addr EEPROM address to write to (0-1023).
 * @param[in] byte Byte to write.
 */
static void eeprom_write_byte(const uint16_t addr, const uint8_t byte)
{
    // Wait for any earlier write to finish.
    while (EECR & (1U << EEPE)) {}

    // Set destination address and data.
    EEAR = addr;
    EEDR = byte;

    // Perform write operation.
    cli();
    EECR |= (1U << EEMPE);
    EECR |= (1U << EEPE);
    sei();
}

/**
 * @brief Read a byte from EEPROM at the given address.
 *
 * @param[in] addr EEPROM address to read from (0-1023).
 *
 * @return Byte stored at that address.
 */
static uint8_t eeprom_read_byte(const uint16_t addr)
{
    // Wait for any earlier write to finish.
    while (EECR & (1U << EEPE)) ;

    // Set address to read from.
    EEAR = addr;

    // Start read operation.
    EECR |= (1U << EERE);

    // Return the byte stored at the given address.
    return EEDR;
}
```

---

## A.4 Worked example: persisting LED1's state across resets
Extends
[L06 Appendix A.6](../../L06/appendix/a_uart.md#a6-example-button-toggles-an-led-reports-over-serial)'s
button-toggle-report demo: LED1's state now survives a reset or power cycle, read back from
EEPROM in `setup()` and written back out every time it changes.

![](../../L06/appendix/images/circuit_serial_with_gpios.png)

```
eeprom_driver_demo/
├── Makefile
├── main.c
├── include/
│   └── driver/
│       ├── eeprom.h
│       └── serial.h
└── source/
    └── driver/
        ├── eeprom.c
        └── serial.c
```

The driver files ([driver/eeprom.h](../eeprom_driver_demo/include/driver/eeprom.h),
[driver/eeprom.c](../eeprom_driver_demo/source/driver/eeprom.c)) wrap A.3's
`eeprom_write_byte()`/`eeprom_read_byte()` behind a buffer-and-length interface,
`eeprom_write()`/`eeprom_read()`, the same shape as L06's `serial_write()`. `main.c` in full:

```c
/**
 * @file EEPROM driver demo.
 */
#include <stdbool.h>
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "driver/eeprom.h"
#include "driver/serial.h"

#define LED1 1U // D9  -> PORTB1.
#define BTN1 5U // D13 -> PORTB5.

/** GPIO operations. */
#define LED1_TOGGLE PINB = (1U << LED1)    // Toggle LED1.
#define LED1_ENABLED (PINB & (1U << LED1)) // High if LED1 is enabled, low otherwise.
#define BTN1_PRESSED (PINB & (1U << BTN1)) // High if BTN1 is pressed, low otherwise.

/** Timer operations. */
#define TIMER0_ENABLE TIMSK0 = (1U << TOIE0) // Enable timer 0 interrupt.
#define TIMER0_DISABLE TIMSK0 = 0U           // Disable timer 0 interrupt.

/** Interrupt operations. */
#define BTN1_INT_ENABLE PCMSK0 |= (1U << BTN1)   // Enable pin change interrupt for BTN1.
#define BTN1_INT_DISABLE PCMSK0 &= ~(1U << BTN1) // Disable pin change interrupt for BTN1.

/** Time parameters */
#define TICK_PERIOD_MS 0.064F    // Time between each tick, prescaler 1024.
#define TICK_MAX 256U            // Ticks per overflow for 8-bit timer.
#define DEBOUNCE_TIMEOUT_MS 300U // Debounce timeout in ms.

/** Limit parameters. */
#define OVF_TIME_MS (TICK_PERIOD_MS * TICK_MAX)     // Time between each overflow in ms.
#define OVF_MAX (DEBOUNCE_TIMEOUT_MS / OVF_TIME_MS) // Overflows needed for timeout.
#define OVF_TIMEOUT (uint8_t)(OVF_MAX + 0.5F)       // Overflows needed for timeout, rounded.

/** EEPROM parameters. */
#define EEPROM_LED1_ADDR 1000U // EEPROM address containing the LED1 state.
#define EEPROM_LED1_ON 1U      // Stored value indicating that LED1 is on.
#define EEPROM_LED1_OFF 0U     // Stored value indicating that LED1 is off.

/** True when a LED1 event occurs. */
static bool led1_event = false;

/**
 * @brief Print the LED1 state over UART.
 */
static void print_led1_state(void)
{
    if (LED1_ENABLED) { serial_print("LED1 enabled!\n"); }
    else { serial_print("LED1 disabled!\n"); }
}

/**
 * @brief Set up system.
 */
static void setup(void)
{
    // Configure LED1 as output.
    DDRB = (1U << LED1);

    // Configure BTN1 as input with its internal pull-up enabled.
    PORTB = (1U << BTN1);

    // Enable pin change interrupt for BTN1.
    PCICR = (1U << PCIE0);
    BTN1_INT_ENABLE;

    // Set up 300 ms debounce timer.
    TCCR0B = (1U << CS00) | (1U << CS02);

    // Initialize serial driver.
    serial_init();

    // Restore the previous LED state from EEPROM.
    uint8_t led_state = 0U;
    if (sizeof(led_state) == eeprom_read(&led_state, sizeof(led_state), EEPROM_LED1_ADDR))
    {
        if (EEPROM_LED1_ON == led_state) { LED1_TOGGLE; }
    }
    print_led1_state();

    // Enable interrupts globally.
    sei();
}

/**
 * @brief Toggle LED1 and transmit a message over UART on button press.
 *
 *        Disable button interrupts for 300 ms to prevent debounce.
 */
ISR(PCINT0_vect)
{
    // Disable button interrupts for 300 ms.
    BTN1_INT_DISABLE;
    TIMER0_ENABLE;

    // Toggle LED1 and save its new state if BTN1 is pressed.
    if (BTN1_PRESSED)
    {
        LED1_TOGGLE;
        led1_event = true;
    }
}

/**
 * @brief Re-enable button interrupts after 300 ms.
 */
ISR(TIMER0_OVF_vect)
{
    static volatile uint8_t ovf_counter = 0U;

    // Wait for 300 ms, then re-enable button interrupts and disable timer 0.
    if (OVF_TIMEOUT <= ++ovf_counter)
    {
        BTN1_INT_ENABLE;
        TIMER0_DISABLE;
        ovf_counter = 0U;
    }
}

/**
 * @brief Application entry point.
 *
 * @return 0 on termination of the program (should never occur).
 */
int main(void)
{
    setup();

    while (1)
    {
        // Check if a LED1 event has occurred, store the new state in EEPROM if true.
        if (led1_event)
        {
            const uint8_t led_state = LED1_ENABLED ? EEPROM_LED1_ON : EEPROM_LED1_OFF;
            eeprom_write(&led_state, sizeof(led_state), EEPROM_LED1_ADDR);
            led1_event = false;

            // Report the new LED1 state over UART.
            print_led1_state();
        }
    }
    return 0;
}
```

`ISR(PCINT0_vect)` only toggles LED1 and sets `led1_event`; the EEPROM write and the UART report
both happen in `main()`'s loop instead, once per event, never inside the ISR itself. That's
deliberate, not incidental, and it's good practice generally: both `eeprom_write()` (which can
block for several milliseconds waiting on `EECR`'s `EEPE` bit, A.2) and `serial_print()`
(which blocks byte-by-byte on `UDRE0`) are slow, blocking calls, and an ISR that spends
milliseconds inside either one holds up every other interrupt for that whole time, including the
debounce timer this very feature depends on. Keep ISRs to the minimum work needed (flip a pin,
set a flag) and let `main()`'s loop handle anything that can block, the same reasoning
[L06 Appendix A.6](../../L06/appendix/a_uart.md#a6-example-button-toggles-an-led-reports-over-serial)
gave for never calling `serial_print()` from an ISR.

Build and flash the same way as
[L06 Appendix A.6](../../L06/appendix/a_uart.md#a6-example-button-toggles-an-led-reports-over-serial):
on Linux, `cd ../eeprom_driver_demo` and run `make` (`make flash` to program the board); on Windows
via Microchip Studio, recreate this folder structure inside the project and add `include` as an
include directory.

---
