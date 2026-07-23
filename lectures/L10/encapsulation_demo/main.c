/**
 * @brief Demonstrate GPIO usage with interrupt-driven callback handling.
 */
#include <stddef.h>

#include "driver/gpio.h"

/** Use GPIO pin 9 for the LED. */
#define LED_PIN 9U

/** Use GPIO pin 13 for the button. */
#define BUTTON_PIN 13U

/** GPIO devices (file-global so they can be accessed from the interrupt callback). */
static gpio_t *led1, *btn1;

/**
 * @brief Handle button event.
 *
 *        Toggle the LED when the button is pressed.
 */
static void button_event(void)
{
    if (gpio_read(btn1)) { gpio_toggle(led1); }
}

/**
 * @brief Application entry point.
 *
 * @return 0 on termination of the program (should never occur), or -1 on
 * failure.
 */
int main(void)
{
    // Initialize LED GPIO (output, no callback), return -1 on failure.
    led1 = gpio_new(LED_PIN, GPIO_MODE_OUTPUT, NULL);
    if (NULL == led1) { return -1; }

    // Initialize button GPIO (input with pull-up, use callback), return -1 on failure.
    btn1 = gpio_new(BUTTON_PIN, GPIO_MODE_INPUT_PULLUP, button_event);
    if (NULL == btn1) { return -1; }

    // Enable pin change interrupt for the button.
    gpio_enable_pci(btn1);

    // Keep program running.
    while (1)
    {
        // Handle event if occurred.
        gpio_handle_event(btn1);
    }

    // Unreachable in this example (infinite loop). Shown for completeness.
    gpio_del(&led1);
    gpio_del(&btn1);
    return 0;
}
