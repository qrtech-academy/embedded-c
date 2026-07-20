# Appendix A - Serial Communication (UART)

## A.1 Why serial
* Every debugging technique so far has been "blink an LED and infer what happened." UART (Universal
  Synchronous/Asynchronous Receiver/ Transmitter) gives you a real communication channel to a PC,
  `printf()`-style debugging, logging sensor data, or eventually two-way communication with a host
  application.
* It's called *serial* because it sends one bit at a time over a single wire, framed by a start bit
  and one or more stop bits per byte, cheap on wiring, at the cost of throughput. This lecture
  builds up transmit first (A.2-A.6), then receiving (A.7), which reuses almost the same
  registers.
* Speed is measured in **baud rate**, bits per second. 9600 baud is the traditional default and
  what we'll use here.

---

## A.2 Configuring UART
Three registers:

1. **`UCSR0B`** enables transmit (and, later, receive):

   ```c
   UCSR0B = (1U << TXEN0);   // Enable transmission.
   ```

2. **`UCSR0C`** sets the frame format. 8 data bits, no parity, 1 stop bit (the near-universal
   default) needs:

   ```c
   UCSR0C = (1U << UCSZ00) | (1U << UCSZ01);
   ```

3. **`UBRR0`** holds the baud rate, computed from the CPU clock:

   ```
   UBRR0 = F_CPU / (16 * baud_rate) - 1
   ```

   At 16 MHz for 9600 baud, that's `103`:

   ```c
   UBRR0 = 103U;
   ```

```c
#define BAUD 103U // UBRR value for 9600 bps with a 16 MHz clock.

/**
 * @brief Initialize UART with a 9600 bps baud rate.
 */
static void serial_init(void)
{
    // Enable UART transmission.
    UCSR0B = (1U << TXEN0);

    // Set character size to eight bits.
    UCSR0C = (1U << UCSZ00) | (1U << UCSZ01);

    // Set baud rate to 9600 bps.
    UBRR0 = BAUD;
}
```

---

## A.3 Sending data
Transmission happens one byte at a time through the `UDR0` data register. Before writing a new
byte, wait for the previous one to finish sending; signalled by the `UDRE0` (Data Register Empty)
flag in `UCSR0A`:

```c
/**
 * @brief Transmit a single byte over UART, blocking until the data register is free.
 *
 * @param[in] data Byte to transmit.
 */
static void serial_write_byte(const char data)
{
    while (0U == (UCSR0A & (1U << UDRE0))); // Wait for the register to free up.
    UDR0 = data;
}
```

Sending a whole string is just calling that in a loop, plus a trailing `\n\r` so each message
starts on a fresh line in the terminal:

```c
/**
 * @brief Transmit text over UART.
 *
 * @param[in] text Text to transmit.
 */
static void serial_print(const char* text)
{
    // Terminate if the text pointer is NULL.
    if (NULL == text) { return; }

    // Transmit each character one by one.
    for (uint16_t i = 0U; text[i]; ++i)
    {
        serial_write_byte((uint8_t)(text[i]));
    }
    serial_write_byte('\r');
    serial_write_byte('\n');
}
```

## A.4 Simple TX example
A first fully working example: A.2's `serial_init()` and A.3's `serial_write_byte()`/
`serial_print()` combined into a single flashable program. LED1 on D9 blinks every 500 ms,
and each transition is reported over serial. `delay_ms()` is the same hand-rolled helper built on
`_delay_ms()` from
[L02 Appendix A.6](../../L02/appendix/a_registers_and_io.md#a6-blinking-two-leds-with-a-hand-rolled-delay),
reused rather than redefined from scratch. Everything still lives in one file here; A.6 splits the
driver out into its own header and source file once there's enough of it to be worth the split.

```c
#define F_CPU 16000000UL // CPU frequency in Hz.

#include <stddef.h>
#include <stdint.h>

#include <avr/io.h>
#include <util/delay.h>

/** GPIO pins. */
#define LED1 1U // D9  -> PORTB1.

/** GPIO operations. */
#define LED1_TOGGLE PINB = (1U << LED1)    // Toggle LED1.
#define LED1_ENABLED (PINB & (1U << LED1)) // High if LED1 is enabled, low otherwise.

/** Time parameters. */
#define BLINK_SPEED_MS 500U // Blink speed in ms.
#define TICK_1MS 1U         // Generate a 1 ms tick in delay_ms().

/** Serial parameters. */
#define BAUD 103U            // UBRR value for 9600 bps with a 16 MHz clock.
#define NEW_LINE '\n'        // New line character.
#define CARRIAGE_RETURN '\r' // Carriage return.

/**
 * @brief Initialize UART with a 9600 bps baud rate.
 */
static void serial_init(void)
{
    // Enable UART transmission.
    UCSR0B = (1U << TXEN0);

    // Set character size to eight bits.
    UCSR0C = (1U << UCSZ00) | (1U << UCSZ01);

    // Set baud rate to 9600 bps.
    UBRR0 = BAUD;
}

/**
 * @brief Transmit a single byte over UART, blocking until the data register is free.
 *
 * @param[in] data Byte to transmit.
 */
static void serial_write_byte(const uint8_t byte)
{
    // Wait until the transmit buffer is ready for a new byte.
    while (0U == (UCSR0A & (1U << UDRE0))) {}

    // Place the new byte in the transmit data register.
    UDR0 = byte;
}

/**
 * @brief Transmit text over UART.
 *
 * @param[in] text Text to transmit.
 */
static void serial_print(const char* text)
{
    // Terminate if the text pointer is NULL.
    if (NULL == text) { return; }

    // Transmit each character one by one.
    for (uint16_t i = 0U; text[i]; ++i)
    {
        serial_write_byte((uint8_t)(text[i]));
    }
    serial_write_byte('\r');
    serial_write_byte('\n');
}

/**
 * @brief Generate delay.
 *
 * @param[in] ms Delay duration in ms.
 */
static void delay_ms(const uint16_t ms)
{
    for (uint16_t i = 0U; i < ms; ++i)
    {
        _delay_ms(TICK_1MS);
    }
}

/**
 * @brief Set up system.
 */
static void setup(void)
{
    // Configure LED1 as output.
    DDRB = (1U << LED1);

    // Initialize serial driver.
    serial_init();
}

/**
 * @brief Application entry point.
 *
 * @return 0 on termination of the program (should never occur).
 */
int main(void)
{
    setup();

    // Toggle LED1 and print its new state every 500 ms.
    while (1)
    {
        LED1_TOGGLE;
        if (LED1_ENABLED) { serial_print("LED1 enabled!"); }
        else { serial_print("LED1 disabled!"); }
        delay_ms(BLINK_SPEED_MS);
    }
    return 0;
}
```

---

## A.5 Mixing text and numbers
`sprintf()` (from `<stdio.h>`) formats into a buffer exactly like `printf()` formats to the
terminal; build the string there, then hand it to `serial_print()`. A full program, extending
A.4 with a counter reported once every 500 ms instead of a fixed message:

```c
#define F_CPU 16000000UL // CPU frequency in Hz.

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>
#include <util/delay.h>

/** Time parameters. */
#define REPORT_INTERVAL_MS 500U // Report interval in ms.
#define TICK_1MS 1U             // Generate a 1 ms tick in delay_ms().

/** Serial parameters. */
#define BAUD 103U // UBRR value for 9600 bps with a 16 MHz clock.

/** Message parameters. */
#define MSG_LEN 50U // Max length of a formatted message, including the NUL terminator.

/**
 * @brief Initialize UART with a 9600 bps baud rate.
 */
static void serial_init(void)
{
    // Enable UART transmission.
    UCSR0B = (1U << TXEN0);

    // Set character size to eight bits.
    UCSR0C = (1U << UCSZ00) | (1U << UCSZ01);

    // Set baud rate to 9600 bps.
    UBRR0 = BAUD;
}

/**
 * @brief Transmit a single byte over UART, blocking until the data register is free.
 *
 * @param[in] byte Byte to transmit.
 */
static inline void serial_write_byte(const uint8_t byte)
{
    // Wait until the transmit buffer is ready for a new byte.
    while (0U == (UCSR0A & (1U << UDRE0))) {}

    // Place the new byte in the transmit data register.
    UDR0 = byte;
}

/**
 * @brief Transmit text over UART.
 *
 * @param[in] text Text to transmit.
 */
static void serial_print(const char* text)
{
    // Terminate if the text pointer is NULL.
    if (NULL == text) { return; }

    // Transmit each character one by one.
    for (uint16_t i = 0U; text[i]; ++i)
    {
        serial_write_byte((uint8_t)(text[i]));
    }
    serial_write_byte('\r');
    serial_write_byte('\n');
}

/**
 * @brief Generate delay.
 *
 * @param[in] ms Delay duration in ms.
 */
static void delay_ms(const uint16_t ms)
{
    for (uint16_t i = 0U; i < ms; ++i)
    {
        _delay_ms(TICK_1MS);
    }
}

/**
 * @brief Set up system.
 */
static void setup(void)
{
    // Initialize serial driver.
    serial_init();
}

/**
 * @brief Application entry point.
 *
 * @return 0 on termination of the program (should never occur).
 */
int main(void)
{
    uint16_t counter = 0U;
    setup();

    // Report an incrementing counter every 500 ms.
    while (1)
    {
        // Parse the counter value into a string:
        char msg[MSG_LEN] = {'\0'};
        sprintf(msg, "Count: %u", counter++);

        // Transmit the string content over UART, then wait 500 ms.
        serial_print(msg);
        delay_ms(REPORT_INTERVAL_MS);
    }
    return 0;
}
```

`%d` for `int`, `%u` for unsigned, `%u` used here since `count` is `uint16_t`. Floating-point
formatting in AVR's default `sprintf()` is often disabled or costly to link in; prefer integers
(e.g. scale a reading to hundredths and print the two parts separately) unless you've confirmed
float formatting is available in your toolchain.

---

## A.6 Example: button toggles an LED, reports over serial
Button on D13, LED on D9. Each press flips the LED and reports its new state plus a running count
of toggles.

![](./images/circuit_serial_with_gpios.png)

This is also the course's first multi-file example: rather than inlining every function here, the
serial driver lives in its own header and source file, and `main.c` `#include`s it like any other
driver:

```
serial_driver_demo/
├── Makefile
├── main.c
├── include/
│   └── driver/
│       └── serial.h
└── source/
    └── driver/
        └── serial.c
```

The driver ([driver/serial.h](../serial_driver_demo/include/driver/serial.h),
[driver/serial.c](../serial_driver_demo/source/driver/serial.c)) adds one function beyond A.3-A.4's
`serial_write_byte()`/`serial_print()`: `serial_write()`, which takes a raw `uint8_t` buffer
and a length, for arbitrary binary data rather than NUL-terminated text. `main.c` below still only
needs `serial_print()`, the same convenience wrapper already familiar from A.3-A.4. `main.c`
itself is shown in full below:

```c
#include <stdbool.h>
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>

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
#define OVF_TIMEOUT (uint8_t)(OVF_MAX + 0.5F)        // Overflows needed for timeout, rounded.

/** Flags that a LED1 event has occurred. */
static bool led1_event = false;

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

    // Toggle LED1 if BTN1 is pressed, flag that a LED1 event has occurred.
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
        // Check if a LED1 event has occurred, report the new state over UART if true.
        if (led1_event)
        {
            if (LED1_ENABLED) { serial_print("LED1 enabled!\n"); }
            else { serial_print("LED1 disabled!\n"); }
            led1_event = false;
        }
    }
    return 0;
}
```

`ISR(PCINT0_vect)` only toggles LED1 and sets `led1_event`; the UART report happens in `main()`'s
loop instead, once per event, never inside the ISR itself. `serial_print()` blocks
byte-by-byte on `UDRE0` until each byte is sent, and an ISR that spends that time transmitting
holds up every other interrupt for the whole transmission, including the debounce timer this very
feature depends on. Keep ISRs to the minimum work needed (flip a pin, set a flag) and let
`main()`'s loop handle anything that can block; L07 Appendix A.4 reuses this exact pattern for
`eeprom_write()`, another slow, blocking call that doesn't belong in an ISR either.

On Linux, `cd ../serial_driver_demo` and run `make` to build, then `make flash` to program the board, the
same `Makefile` pattern as
[L02 Appendix D.8](../../L02/appendix/d_building_and_flashing_on_linux.md#d8-optional-a-makefile).
On Windows via Microchip Studio, there's no Makefile support: recreate this same folder structure
inside the project, then add `include` as an include directory under the project's toolchain
settings (Project Properties -> Toolchain -> AVR/GNU C Compiler -> Directories), the IDE's
equivalent of the Makefile's `-Iinclude`.

Open a serial terminal (the Arduino IDE's Serial Monitor, or `screen`/`minicom` from the command
line) at 9600 baud on the Uno/Nano's port to see the output.

---

## A.7 Receiving data
Receiving is transmit's mirror image: one more enable bit in `UCSR0B`, one more status flag in
`UCSR0A` to poll, and the same `UDR0` data register, just read instead of written.

1. **`RXEN0`** (Receiver Enable) sits right next to `TXEN0` in `UCSR0B`, the register A.2
   introduced. Set both together to enable transmit and receive at once:

   ```c
   UCSR0B = (1U << TXEN0) | (1U << RXEN0);   // Enable transmission and reception.
   ```

2. **`RXC0`** (RX Complete), in `UCSR0A`, is `UDRE0`'s counterpart: it's clear while nothing new
   has arrived, and sets itself the moment a fully-received byte is sitting in `UDR0`, waiting to
   be read. Reading `UDR0` clears it automatically, exactly as reading `UDR0` also readies it for
   the next incoming byte.

3. **`UDR0`** is the very same register `serial_write_byte()` writes to in A.3; reading it instead
   of writing it returns whatever byte just arrived.

Put together, the receive-side equivalent of `serial_write_byte()` is:

```c
/**
 * @brief Receive a single byte over UART, blocking until one arrives.
 *
 * @return Byte received.
 */
static char serial_read_byte(void)
{
    while (0U == (UCSR0A & (1U << RXC0))) {} // Wait for a byte to arrive.
    return UDR0;
}
```

This blocks indefinitely if nothing ever arrives, fine for a first pass, but worth keeping in mind
once a program also needs to do other work while waiting. Reading more than one byte into a
buffer, with an optional timeout so a silent sender can't hang the program forever, builds directly
on `serial_read_byte()` the same way A.6's driver `serial_write()` builds on `serial_write_byte()`.

`read()` below does exactly that: it fills a caller-supplied buffer up to `buflen` bytes, and takes
an optional `timeout_ms` so a sender that never shows up can't hang the program forever.

```c
/**
 * @brief Receive up to buflen bytes over UART into buf.
 *
 * @param[out] buf Buffer to receive into.
 * @param[in] buflen Size of buf in bytes.
 * @param[in] timeout_ms Milliseconds to wait for buflen bytes before giving up.
 *            0 blocks indefinitely.
 *
 * @return Number of bytes received, or -1 if buf is NULL or buflen is 0.
 */
int16_t read(uint8_t* buf, const uint16_t buflen, const uint16_t timeout_ms)
{
    // Check the input parameters, return -1 if invalid.
    if ((NULL == buf) || (buflen == 0U)) { return -1; }
    uint16_t rx_count = 0U;

    if (0U == timeout_ms)
    {
        // Read indefinitely until the buffer is full if no timeout has been specified.
        while (buflen > rx_count)
        {
            // Wait until a byte has been received.
            while (0U == (UCSR0A & (1U << RXC0))) {}
            buf[rx_count++] = UDR0;
        }
    }
    else
    {
        for (uint16_t i = 0U; i < timeout_ms; ++i)
        {
            while ((buflen > rx_count) && (UCSR0A & (1U << RXC0)))
            {
                buf[rx_count++] = UDR0;
            }
            // Stop reading if the read buffer is full.
            if (buflen <= rx_count) { break; }

            // Wait a millisecond before reading again.
            delay_ms(1U);
        }
    }
    // Return the number of bytes read.
    return (int16_t)(rx_count);
}
```

`read()` extends `serial_read_byte()` from a single byte to a whole buffer, with `timeout_ms`
choosing between two modes:
* **`timeout_ms == 0`**: block indefinitely, the same as `serial_read_byte()`, until `buflen` bytes
  have arrived.
* **`timeout_ms` nonzero**: give up after roughly that many milliseconds even if the buffer isn't
  full yet, pacing itself with the `delay_ms()` helper first defined in
  [L02 Appendix A.6](../../L02/appendix/a_registers_and_io.md#a6-blinking-two-leds-with-a-hand-rolled-delay),
  one millisecond at a time, rather than redefining it here.

`buf` is checked against `NULL` before use, the same reasoning as `vector_resize()` back in
[L01 Appendix A.11](../../L01/appendix/a_c_fundamentals.md#a11-dynamic-memory-allocation-mallocreallocfree):
this is written as a reusable, exported function with call sites outside this file, not a private
helper like L03's `delay_ms()`, so the check is doing real work rather than just demonstrating the
habit.

The return value distinguishes three outcomes: `-1` for a bad call (`NULL` buffer or
`buflen == 0`), the number of bytes actually received if the timeout elapsed first, or `buflen`
itself once the buffer filled completely.

---
