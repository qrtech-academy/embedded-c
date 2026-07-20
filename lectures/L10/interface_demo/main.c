/**
 * @brief GPIO driver example with interfaces in C.
 */
#include <stddef.h>

#include "driver/gpio/atmega328p.h"
#include "driver/gpio/stub.h"

#define LED1_PIN 8U // D8.

/** Max value of the loop counter used to simulate button events. */
#define LOOP_CNT_MAX 60000UL

/**
 * @brief Simulate button events.
 *
 * @param[out] btn Pointer to the simulated button.
 */
void simulate_btn_event(gpio_interface_t *btn) {
  static uint16_t loop_cnt = 0U;

  // Simulate a button toggle every LOOP_CNT_MAX calls.
  if (LOOP_CNT_MAX <= ++loop_cnt) {
    btn->vptr->toggle(btn);
    loop_cnt = 0U;
  }
}

/**
 * @brief Application entry point.
 *
 * @return 0 on termination of the program (should never occur), or -1 on
 * failure.
 */
int main(void) {
  // Create and initialize GPIO instances, return -1 on failure.
  gpio_interface_t *led1 = gpio_atmega328p_new(LED1_PIN, GPIO_MODE_OUTPUT);
  if (NULL == led1) {
    return -1;
  }

  gpio_interface_t *btn1 = gpio_stub_new();
  if (NULL == btn1) {
    return -1;
  }
  bool btn1_prev = false;

  while (1) {
    // Simulate button events every loop_cnt_MAX main-loop iterations.
    simulate_btn_event(btn1);

    // Detect pressdown (rising edge).
    const bool btn1_current = btn1->vptr->read(btn1);
    const bool btn1_pressed = btn1_current && !btn1_prev;

    // Toggle the LED on button pressdown (rising edge).
    if (btn1_pressed) {
      led1->vptr->toggle(led1);
    }
    btn1_prev = btn1_current;
  }
  // Unreachable in this example (infinite loop). Shown for completeness.
  led1->vptr->del(&led1);
  btn1->vptr->del(&btn1);
  return 0;
}
