/**
 * @file Watchdog timer driver.
 */
#ifndef DRIVER_WATCHDOG_H_
#define DRIVER_WATCHDOG_H_

/**
 * @brief Enumeration of watchdog timeouts.
 */
typedef enum
{
    WATCHDOG_TIMEOUT_16MS,        ///< 16 ms timeout.
    WATCHDOG_TIMEOUT_32MS,        ///< 32 ms timeout.
    WATCHDOG_TIMEOUT_64MS,        ///< 64 ms timeout.
    WATCHDOG_TIMEOUT_128MS,       ///< 128 ms timeout.
    WATCHDOG_TIMEOUT_256MS,       ///< 256 ms timeout.
    WATCHDOG_TIMEOUT_512MS,       ///< 512 ms timeout.
    WATCHDOG_TIMEOUT_1024MS,      ///< 1024 ms timeout.
    WATCHDOG_TIMEOUT_2048MS,      ///< 2048 ms timeout.
    WATCHDOG_TIMEOUT_4096MS = 32, ///< 4096 ms timeout.
    WATCHDOG_TIMEOUT_8192MS = 33, ///< 8192 ms timeout.
} watchdog_timeout_t;

/**
 * @brief Initialize the watchdog timer.
 *
 * @param[in] timeout Watchdog timeout.
 */
void watchdog_init(watchdog_timeout_t timeout);

/**
 * @brief Reset the watchdog timer.
 */
void watchdog_reset(void);

#endif /** DRIVER_WATCHDOG_H_ */
