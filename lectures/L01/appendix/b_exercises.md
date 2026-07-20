# Appendix B - Exercises

## Structs, enums, and typedefs
**1.** Define a `struct` named `temp_reading` with fields `adc_value` (`unsigned int`), `voltage_v`
(`double`), and `temperature_c` (`double`). Write a `static` function `temp_reading_init()` that
takes a pointer to a `temp_reading` (named `self`, per the convention in [Appendix A,
A.3](./a_c_fundamentals.md#a3-structs-enums-and-typedefs)) plus an ADC value, and stores it along
with the voltage and temperature derived from it (you don't need the real TMP36 formula yet; a
placeholder conversion is fine, you'll derive the real one in a later project).

**2.** Turn `struct temp_reading` into a `typedef`, `temp_reading_t`, following the `_t` convention
from Appendix A, A.3.

**3.** Define a `typedef enum` named `state_t` with five states: `STATE_OFF`, `STATE_SLOW`,
`STATE_MEDIUM`, `STATE_FAST`, `STATE_ON`. Write a `switch` over a `state_t` variable that prints a
message for each state, then remove one `case` and see whether your compiler warns about the
missing case (try building with `-Wswitch` if it doesn't by default). **Stretch:** write a function
`state_next()` that returns the next state, wrapping from `STATE_ON` back to `STATE_OFF`; the core
of the state machine you'll build in a later project.

---

## Functions and header guards
**4.** Split `temp_reading_init()` from exercise 1 into a `temp_reading.h`/`temp_reading.c` pair:
declare the struct and prototype in the header, define the function in the source file, and add a
proper header guard.

**5.** Add one `static` helper function to `temp_reading.c` that isn't declared in
`temp_reading.h`. For example, one that converts an ADC value to a voltage. Following the ordering
convention from [Appendix A, A.6](./a_c_fundamentals.md#a6-functions), where should its forward
declaration go, and where should its definition go?

---

## Pointers
**6.** Write a function `swap()` that takes two `int*` parameters and swaps the values they point
to. Call it from `main()` on two local variables and print both before and after.

**7.** Write a function `temp_reading_reset()` taking a `temp_reading_t*` that resets all fields to
zero. Call it from `main()` and verify that the caller's struct changes.

Then write a function `temp_reading_delete()` that takes a `temp_reading_t**`, a pointer to a
pointer, per Appendix A, A.7/A.11, and sets the caller's pointer to `NULL`. Why does this need a
double pointer instead of a single `temp_reading_t*`?

---

## Dynamic memory allocation
**8.** Write a function `temp_reading_array_new(size_t count)` that `malloc()`s an array of `count`
`temp_reading_t` structs and returns a pointer to it (or `NULL` on failure). Pair it with
`temp_reading_array_delete()`, taking a `temp_reading_t**`, that frees the array and nulls the
caller's pointer.

**9.** Extend the pair from exercise 8 with `temp_reading_array_resize()`, using `realloc()`,
following the pattern in [Appendix A,
A.11](./a_c_fundamentals.md#a11-dynamic-memory-allocation-mallocreallocfree). Make sure it behaves
correctly when resizing down to zero elements. This is the shape you'd reach for to keep a rolling
history of the last few readings, like the averaging window in the temperature project.

**Tip:** compile with `-fsanitize=address` (GCC/Clang) while testing exercises 8-9; it catches
use-after-free and leaks immediately, which are easy to introduce by hand here.

---
