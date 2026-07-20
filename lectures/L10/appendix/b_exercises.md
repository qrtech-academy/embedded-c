# Appendix B - Exercises

## Concepts
**1.** Why does `gpio.h` only forward-declare `gpio_t` (`typedef struct gpio gpio_t;`) instead of
including the full field list? What would go wrong, from an encapsulation standpoint, if
`struct gpio { ... };` were written directly in the header instead?

---

**2.** `gpio_create()` returns `NULL` when `malloc()` fails instead of, say, overwriting the
first GPIO it ever created. Why does that matter, and what should calling code do with the
return value?

---

## Program
**3.** Add a second behavior to A.2's driver: `static` functions `gpio_write_inverted()`,
`gpio_read_inverted()`, and `gpio_toggle_inverted()` that treat the pin as active-low (a `true`
write clears the bit rather than setting it), plus a second `gpio_init_inverted(gpio_t* self,
uint8_t pin, gpio_mode_t mode)` that behaves like `gpio_init()` but wires a `gpio_t`'s pointers to
these instead of the originals. Confirm the same `led1.write(&led1, true)` call site now does the
opposite, with no change to any calling code, only which functions `gpio_init_inverted()` wired
up.

---

**4.** Make L09's `timer_t` opaque: move `struct timer { ... };` into a `timer.c` file, expose
only `typedef struct timer timer_t;` plus prototypes for `timer_create()`, `timer_clear()`,
`timer_running()`, `timer_start()`, `timer_stop()`, `timer_toggle()`, and `timer_reset()`, from
`timer.h`. `timer_create(uint32_t timeout_ms, timer_callback_t callback)` should follow
`gpio_create()`'s allocation pattern from A.3, since an opaque `timer_t` can no longer be declared
and passed by address the way L09's `timer_init(timer_t* self, uint32_t timeout_ms,
timer_callback_t callback)` expected.

---

**5.** Wire up A.4's `button_t` for real: create one `gpio_t` configured as a button input and
another as an LED output, then set `on_press` to a function that calls `gpio_toggle()` on the
LED. `button_check()`'s own code should never mention the LED's `gpio_t` by name. See A.5 for a
fully wired, interrupt-driven version of this same idea.

---

**6.** Capstone, harder. `gpio_stub_new()` (A.6) exists specifically so GPIO-dependent logic can
be exercised without a chip attached. Write a small test that creates a `gpio_interface_t*` via
`gpio_stub_new()` alone, no `gpio_atmega328p_new()`, no hardware, and confirms `write()`/
`read()`/`toggle()` behave correctly purely through the interface (write `true`, read back
`true`, `toggle()` flips it to `false`). Then write a third implementation, one that logs every
`write()`/`toggle()` call before updating its own `state`, and confirm `main.c`'s edge-detection
logic (A.6) works against it completely unmodified, just by changing which `gpio_interface_t*`
gets created.

---
