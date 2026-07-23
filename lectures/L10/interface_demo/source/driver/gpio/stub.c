/**
 * @brief GPIO stub driver implementation.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "driver/gpio/stub.h"

/**
 * @brief GPIO stub structure.
 */
typedef struct gpio_stub
{
    /** GPIO interface. */
    gpio_interface_t itf;

    /** GPIO state (true = high, false = low). */
    bool state;
} gpio_stub_t;

// -----------------------------------------------------------------------------
static inline gpio_stub_t* get_impl(gpio_interface_t* self) { return (gpio_stub_t*)(self); }

// -----------------------------------------------------------------------------
static inline const gpio_stub_t* get_const_impl(const gpio_interface_t* self)
{
    return (const gpio_stub_t*)(self);
}

// -----------------------------------------------------------------------------
static void gpio_del(gpio_interface_t** self)
{
    // Terminate the function if self is a nullptr.
    if (NULL == self) { return; }

    // Retrieve implementation details, terminate the function on failure.
    gpio_stub_t* impl = get_impl(*self);
    if (NULL == impl) { return; }

    // Free allocated memory.
    free(impl);

    // Set the associated pointer to null.
    *self = NULL;
}

// -----------------------------------------------------------------------------
static bool gpio_read(const gpio_interface_t* self)
{
    // Retrieve implementation details.
    const gpio_stub_t* impl = get_const_impl(self);

    // Return true if the input is high, false otherwise.
    return NULL != impl ? impl->state : false;
}

// -----------------------------------------------------------------------------
static void gpio_write(gpio_interface_t* self, const bool value)
{
    // Retrieve implementation details, terminate the function on failure.
    gpio_stub_t* impl = get_impl(self);
    if (NULL == impl) { return; }

    // Set/clear the output in accordance with the given value.
    impl->state = value;
}

// -----------------------------------------------------------------------------
static void gpio_toggle(gpio_interface_t* self)
{
    // Retrieve implementation details, terminate the function on failure.
    gpio_stub_t* impl = get_impl(self);
    if (NULL == impl) { return; }

    // Toggle the output.
    impl->state = !impl->state;
}

// -----------------------------------------------------------------------------
static const gpio_vtable_t* gpio_vptr_get_instance(void)
{
    // Create and initialize vtable holding function pointers (done once at startup).
    static const gpio_vtable_t vtable = {
        .del    = gpio_del,
        .read   = gpio_read,
        .write  = gpio_write,
        .toggle = gpio_toggle,
    };
    // Return a pointer to the vtable.
    return &vtable;
}

// -----------------------------------------------------------------------------
gpio_interface_t* gpio_stub_new(void)
{
    // Create new GPIO instance, return nullptr on failure.
    gpio_stub_t* self = (gpio_stub_t*)(malloc(sizeof(gpio_stub_t)));
    if (NULL == self) { return NULL; }

    // Initialize the instance.
    self->itf.vptr = gpio_vptr_get_instance();
    self->state    = false;

    // Return the initialized GPIO instance, cast to the corresponding interface.
    return (gpio_interface_t*)(self);
}
