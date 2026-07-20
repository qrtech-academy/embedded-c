# Appendix A - C Fundamentals for Experienced Programmers
A fast-moving refresher on C syntax for programmers who already know how to program in another
language; not a full C course, but enough that later lectures can assume you're not fighting the
language while learning hardware concepts. If a section looks obvious, skip ahead; the [pointer
section](#a7-pointers-the-one-thing-thats-actually-different) is the one worth actually reading.

---

## A.1 Compiling and program shape
* C is compiled, not interpreted: source code → assembly → machine code. No runtime interpretation
  overhead, which is most of why it's the default choice for embedded systems.
* C is procedural: everything lives in functions; `main()` is the entry point.
* No safety net: no bounds checking, no garbage collection. The language trusts you.

A `Hello, world!` program is shown below:

```c
#include <stdio.h>

int main(void)
{
    printf("Hello, world!\n");
    return 0;
}
```

* `#include <stdio.h>` pulls in declarations (a header), not an implementation; that's resolved at
  link time.
* `return 0` from `main()` signals success to the OS. C99+ makes it implicit if omitted, but write
  it anyway.
* A Doxygen comment above a function, written in the imperative mood and documenting its contract,
  is conventional throughout this codebase. For example, a function that computes voltage by Ohms
  law can be documented as shown below:

```c
/**
 * @brief Compute voltage from resistance and current.
 *
 * @param[in] resistance_kohm Resistance in kOhm.
 * @param[in] current_ma Current in mA.
 *
 * @return Voltage in V.
 */
double compute_voltage(double resistance_kohm, double current_ma);
```

---

## A.2 Data types & naming
| Type | Meaning | Typical size |
|---|---|---|
| `void` | nothing (no return value) | – |
| `int` | signed integer | 2–4 bytes |
| `float` | single-precision float | 4 bytes |
| `double` | double-precision float (default for float literals) | 8 bytes |
| `char` | one character | 1 byte |
| `char[]` / `char*` | string (array of `char`, NUL-terminated) | – |
| `bool` (`<stdbool.h>`) | `true` / `false` | 1 byte |

* Prefer `const` for anything that shouldn't change after initialization, following the same intent
  as `final`/`readonly` in other languages. It also lets the compiler catch mistakes and optimize
  more aggressively.

An exception is function declarations: do not qualify arguments passed by value with const, as it
has no effect on the function interface and only reduces readability. In the corresponding function
definition, however, pass-by-value arguments should be declared const for the reasons above.
* Naming convention: `snake_case` for variables/functions/constants, `UPPER_CASE` for macros and
  enum constants. Not camelCase/PascalCase; that's a Java/C#/Pascal/C++ convention, not a C one.
* **Integer division truncates**: `5 / 2 == 2`, not `2.5`. Force float division by making at least
  one operand a `double`/`float`, or cast: `(double)(a) / b`.
* `%` is modulus, integers only; using it on a float is a compiler error.

---

## A.3 Structs, enums, and typedefs

### Structs
* A `struct` groups related fields under one name, each accessed with `.` (or `->` if you're going
  through a pointer). No methods, no inheritance, no access control; just a memory layout, laid
  out in declaration order (the compiler may insert padding between fields for alignment).
* Compare to a plain data class/record in other languages, minus the behavior.
* The idiom used throughout this codebase for functions that operate on a struct: take a pointer to
  it as the first parameter, named `self`. It's the closest C gets to a method's implicit `this`,
  without the language actually having objects:

```c
/**
 * @brief Electrical measurement structure.
 */
struct measurement
{
    /** Voltage in V. */
    double voltage_v;

    /** Current in mA. */
    double current_ma;

    /** Resistance in kOhm. */
    double resistance_kohm;
};

/**
 * @brief Store a voltage/current reading and derive its resistance (Ohm's law).
 *
 * @param[out] self Pointer to the measurement to populate.
 * @param[in] voltage_v Voltage in V.
 * @param[in] current_ma Current in mA.
 */
static void measurement_init(struct measurement* self, const double voltage_v,
                              const double current_ma)
{
    self->voltage_v       = voltage_v;
    self->current_ma      = current_ma;
    self->resistance_kohm = voltage_v / current_ma;
}

struct measurement m;
measurement_init(&m, 5.0, 0.5);
printf("%g V\n", m.voltage_v);
```

* Passing a `struct` by value copies every field, so `self` is (almost) always a pointer:
  * A `const struct measurement*` when the function only reads it.
  * A plain `struct measurement*` when it writes through it, same as any other
    [out-parameter](#a7-pointers-the-one-thing-thats-actually-different).

### `typedef`
* `typedef` gives an existing type a new name, most often to avoid typing `struct measurement`
  everywhere. The convention in this codebase is a `_t` suffix on an anonymous struct:

```c
/**
 * @brief Electrical measurement structure.
 */
typedef struct
{
    /** Voltage in V. */
    double voltage_v;

    /** Current in mA. */
    double current_ma;

    /** Resistance in kOhm. */
    double resistance_kohm;
} measurement_t;

// Create measurement instance.
measurement_t m = { .voltage_v = 5.0, .current_ma = 0.5, .resistance_kohm = 10.0 };
```

* `measurement_t` behaves exactly like `struct measurement` above; the `typedef` is purely a naming
  convenience, it doesn't change layout or behavior.

### `enum`
* An `enum` names a set of related integer constants, so a `switch` reads as intent rather than
  magic numbers, and some compilers can warn you when a `switch` doesn't handle every named case:

```c
/**
 * @brief Enumeration of sensor states.
 */
typedef enum
{
    SENSOR_STATE_IDLE,     ///< Sensor is idle.
    SENSOR_STATE_SAMPLING, ///< Sensor is sampling.
    SENSOR_STATE_ERROR     ///< Sensor is in an error state.
} sensor_state_t;

// Create variable holding the sensor state.
sensor_state_t state = SENSOR_STATE_IDLE;
```

* Unless you assign values explicitly, members count up from 0 in declaration order
  (`SENSOR_STATE_IDLE == 0`, `SENSOR_STATE_SAMPLING == 1`, ...):
* Assign explicit values when the numbers themselves matter, e.g. they're written to a hardware
  register.
* Prefer this over a block of `#define SENSOR_STATE_IDLE 0` macros: same cost at runtime, but the
  type shows up in a debugger and the compiler can help catch an unhandled `switch` case.

---

## A.4 Scope, lifetime, and storage

| Storage class | Scope | Lifetime |
| :------------ | :---- | :------- |
| **Local** | Inside a function or block. | Stack-allocated; destroyed when the block exits. |
| **`static` local** | Inside a function or block. | Persists for the program's life. |
| **Global** | Outside any function. | Exists for the program's life; stored in RAM. |

**Rule of thumb**:
* Minimize scope and lifetime.
* Mark file-scope functions and globals `static` when they shouldn't leak outside the `.c` file;
  it's the closest thing C has to "private," and it gives the compiler more room to inline and
  optimize.

---

## A.5 Printing (`printf()`)

| Specifier | Type |
|---|---|
| `%d` / `%i` | `int` |
| `%u` | unsigned int |
| `%f` / `%lf` | `float` / `double`, fixed decimals |
| `%g` | float/double, shortest representation |
| `%c` | single `char` |
| `%s` | string (`char*`) |
| `%p` | pointer address |

```c
printf("%s is %d years old!\n", name, age);
printf("Voltage: %.2f V\n", voltage);   // 2 decimal places
```

---

## A.6 Functions
```c
return_type function_name(param_type param_name, ...)
{
    ...
    return value;   // omit for void
}
```

* `static` before the return type = internal linkage (only visible in this `.c` file). Default to
  `static` for anything that isn't part of a module's public API.
* `static inline` on small, frequently-called functions hints the compiler to paste the body in at
  the call site instead of paying for a call/return. Modern compilers usually do this anyway, but
  it's idiomatic for small helpers.
* Declare functions (a prototype) before use, typically in a header, so callers don't need the full
  definition.

```c
/**
 * @brief Compute resistance from voltage and current (Ohm's law).
 *
 * @param[in] voltage Voltage in V.
 * @param[in] current Current in mA. Must not be 0.
 *
 * @return Resistance in kOhm, or 0 if the current is 0.
 */
static inline double get_resistance(const double voltage, const double current)
{
    return 0.0 != current ? voltage / current : 0.0;
}
```

* Convention for ordering a `.c` file: forward-declare its `static` helpers once, right after the
  `#include`s, then define the public functions (in the same order the header declares them), and
  put the `static` helpers' own definitions at the bottom of the file. A reader hits the public API
  first and only descends into private implementation detail if they keep scrolling:

```c
#include "sensor.h"

/* Static functions: */
static double filter_reading(double raw_v);

double sensor_read(void)
{
    const double raw_v = adc_read();
    return filter_reading(raw_v);
}

/* raw_v filtering is an implementation detail, not part of sensor.h. */
static double filter_reading(const double raw_v)
{
    ...
}
```

* When a header can be included from more than one place (or pulled in indirectly through another
  header), wrap its contents in a header guard so the compiler never sees the same declarations
  twice:

```c
#ifndef SENSOR_H_
#define SENSOR_H_

double sensor_read(void);

#endif /* SENSOR_H_ */
```

* Convention: the guard macro is the file name in `UPPER_CASE`, with `.` replaced by `_` and a
  trailing `_` (`sensor.h` -> `SENSOR_H_`), chosen so it's unlikely to collide with any other macro
  in the program. `#pragma once` does the same job in a single line and most modern compilers
  support it, but it isn't part of the C standard; traditional guards are the portable choice and
  what you'll see throughout this codebase when using C. In C++, `#pragma once` is also widely
  used.

---

## A.7 Pointers (the one thing that's actually different)
> **Everything is pass-by-value in C.** Every function argument is copied. Passing a pointer still
> passes a copy of the pointer; it just happens to point to the same object as the caller's
> pointer. This lets a function modify the caller's object through the pointer, but assigning a
> new value to the pointer itself does not affect the caller.

If you're coming from a garbage-collected language, this is the section to actually read:
* A pointer holds an *address*, e.g. `int* ptr = &x;` makes `ptr` point at `x`:
  * `&x` means "address of x".
  * `*ptr` means "the value that `ptr` points to" (dereference).
* Pointers let a function modify the caller's object because, although the pointer itself is passed
  by value, it still points to that object.

```c
/**
 * @brief Assign 3 to the variable pointed to by x.
 *
 * @param[out] x Pointer to the variable to assign.
 */
static void assign(int* x)
{
    *x = 3; // Writes through the pointer into the caller's variable.
}

int main(void)
{
    int a = 0;
    assign(&a);
    printf("a = %d\n", a); // a = 3.
}
```

This is the standard C idiom for "multiple out-parameters" or "modify in place," using no
references, no boxing, just an address. You'll see this pattern constantly once we start writing to
hardware registers:
* A pointer to a pointer (`int**`) lets a function replace *what the caller's pointer points to*,
  not just the value at that address; the same out-parameter idiom, one level deeper. You'll see
  this whenever a function needs to null out or reassign a pointer it was only given a copy of:

```c
#include <stddef.h>

static void set_to_null(int** ptr)
{
    *ptr = NULL; // Changes the caller's pointer itself, not just *ptr's target.
}

int main(void)
{
    int x = 5;
    int* p = &x;
    set_to_null(&p);
    // p is now NULL; x is untouched.
}
```

* Arrays and strings decay to a pointer to their first element automatically when passed to a
  function, no `&` needed:

```c
static void print_text(const char* s) { printf("%s", s); }
const char text[] = "no & needed here";
print_text(text);
```

* `const char*` means "pointer to data I won't modify through this pointer"; use it on any
  string/array parameter you're only reading, same spirit as marking a parameter `readonly`.
* A string is a `char` array ending in a `'\0'` (NUL) byte; there's no length field. `sizeof` gives
  you the buffer's declared capacity, not the content length, and only works on the actual array,
  not a pointer to it.

---

## A.8 Function pointers
A function pointer holds the *address of a function* instead of a variable, callable through the
pointer exactly like calling the function by name:

```c
#include <stdio.h>

static void greet1(void) { printf("Hello!\n"); }
static void greet2(void) { printf("Worls!\n"); }

int main(void)
{
    void (*fn)(void) = greet1; // fn now holds greet1's address.
    fn();                      // Calls greet1(), exactly like greet1() would.

    fn = greet2;               // fn now holds greet2's address.
    fn();                      // Calls greet2(), exactly like greet2() would.
}
```

* The declaration reads inside-out: `void (*fn)(void)` is "`fn` is a pointer to a function taking
  no arguments and returning `void`." The parentheses around `*fn` are required, `void *fn(void)`
  would instead declare a function returning `void*`.
* A function's name, used without calling it (no `()`), decays to its address the same way an
  array name decays to a pointer to its first element (A.7): `fn = greet1;` needs no `&`.
* `typedef` cleans up the syntax the same way it does for structs (A.3), and gives the pointer
  type a name that documents what it's for:

```c
typedef void (*callback_t)(void);

static void on_timeout(void) { printf("Timed out!\n"); }

static void run(callback_t cb) // cb holds a function's address, not a value.
{
    callback(); // Calls whichever function was passed in.
}

int main(void)
{
    run(on_timeout); // Prints "Timed out!".
}
```

* `run()` never mentions `on_timeout` by name, it just calls whatever `callback_t` it was given.
  That's the whole idea: the caller decides which function runs, the callee just knows the
  function's signature.
* This comes back twice, later in the course: L09's timer driver exercise stores one of these in
  a struct so a timer can run arbitrary code when it expires, and L10 builds an entire
  polymorphism system, structs whose behavior is decided at runtime rather than compile time, out
  of nothing but function pointers.

---

## A.9 Reading input
You won't have a keyboard on a microcontroller, but the buffer-safety idiom here reappears when
reading serial/UART data later, so it's worth knowing.
* `scanf("%s", buf)` is easy but unsafe: no bounds checking, and it stops at the first whitespace.
* `fgets(buf, sizeof(buf), stdin)` is the safe alternative; it takes an explicit buffer size and
  won't overflow, but it keeps the trailing `'\n'`, which callers usually strip:

```c
/**
 * @brief Read a line of input into a buffer, stripping the trailing newline.
 *
 * @param[out] buf Buffer to store the line in.
 * @param[in] buflen Capacity of the buffer, in bytes.
 */
static void readline(char* buf, const size_t buflen)
{
    // Read data from the terminal, store in the buffer.
    fgets(buf, (int)(buflen), stdin);

    // Replace '\n' characters with '\0'.
    for (size_t i = 0U; '\0' != buf[i]; i++)
    {
        if ('\n' == buf[i]) { buf[i] = '\0'; }
    }
}
```

* To get numbers out of text: read a line as a string, then convert with `atoi()` for `int` or
  `atof()` for `double` (both in `<stdlib.h>`). "Read raw bytes, then parse" is the same pattern
  you'll use for serial input later.

---

## A.10 Control flow
C's control-flow syntax is close to every C-family language you already know, with a few local
quirks:
* Conditions must be parenthesized: `if (x < 10)`.
* Braces are technically optional for a single statement, but this codebase always uses them, even
  for one-liners: it avoids the classic bug where a second statement is added later without adding
  braces to match, silently falling outside the `if`:

```c
if (10 > x) { x = 25; }
else { printf("Out of range!\n"); }
```

* `while`, `for`, and `switch` behave as expected. `for (;;)` or `while (true)` is the idiomatic
  infinite loop.
* `switch` **falls through** unless you `break`, a common bug source if you're coming from a
  language that doesn't do this. Give each `case` its own `return`/`break`.
* `&&`, `||`, `!` for logic; `==` to compare, `=` to assign; mixing them up compiles and is a
  classic bug (most compilers will warn on it).
* **Yoda notation**: writing the constant on the left of a comparison, `if (0 == x)` rather than
  `if (x == 0)`, is a defensive habit against exactly that `==`/`=` typo. Drop the second `=` with
  the constant on the left and `if (0 = x)` doesn't compile at all, you can't assign to a literal,
  so the compiler catches the mistake immediately instead of silently accepting an assignment that
  evaluates to `x`'s new value and is truthy for anything but `0`. This codebase writes comparisons
  this way throughout; `if (10 > x)` a few lines up is the same habit applied to `>`, even though
  inequalities don't carry the same typo risk, there's no way to accidentally assign with `>`.

---

## A.11 Dynamic memory allocation (`malloc()`/`realloc()`/`free()`)
You almost certainly won't reach for this on a microcontroller: no OS to reclaim a leak, and a
fragmented heap is a liability you can't always afford. It's here because
`malloc()`/`realloc()`/`free()` are core C knowledge, and the double-pointer idiom below reappears
in any hosted C code (test harnesses, host-side tooling, anything running under Linux rather than
bare-metal).

* `malloc(size)` (from `<stdlib.h>`) asks the heap for `size` bytes and returns a `void*` (a
  generic pointer) to it, or a null pointer if the request can't be satisfied. Always check the
  return value, and cast it to the pointer type you actually want; not required by C, but required
  by C++, and it makes the intent explicit:

```c
#include <stddef.h>
#include <stdlib.h>

int* data = (int*)(malloc(sizeof(int) * count));
if (NULL == data) { return 1; }
```

* `realloc(ptr, new_size)` resizes an existing allocation, possibly moving it to a new address. On
  success it frees the old block for you and returns the new address; on failure it returns null
  and leaves the original block untouched. Capture the result in a temporary first, so a failed
  `realloc()` doesn't overwrite (and thereby leak) your only pointer to the still-valid original
  block:

```c
int* copy = (int*)(realloc(data, sizeof(int) * new_count));
if (NULL == copy) { return 1; } // Data is untouched and still valid on failure.
data = copy;
```

* `free(ptr)` releases the block. Always set the pointer to `0`/`NULL` immediately after; a pointer
  that still holds a freed address is a *dangling pointer*; using it again is undefined behavior
  instead of a clean, obvious crash:

```c
free(data);
data = NULL;
```

* For a function to null out the *caller's* pointer, not just its own local copy of it, it needs
  the address of that pointer: the pointer-to-pointer idiom from
  [A.7](#a7-pointers-the-one-thing-thats-actually-different):

```c
/**
 * @brief Free a dynamically allocated array and null out the caller's pointer.
 *
 * @param[in,out] data Address of the pointer to the array to free.
 */
static void array_delete(int** data)
{
   if ((NULL == data) || (NULL == *data)) { return; }
    free(*data);
    *data = NULL;
}
```

* Putting it together: `struct`, `self`, and dynamic allocation combine into the standard C shape
  for a growable array:

```c
#include <stddef.h>
#include <stdlib.h>

/**
 * @brief Vector structure holding floating-point numbers.
 */
typedef struct
{
    /* Data field of floating-point numbers. */
    double* data;

    /** Number of elements in the datafield. */
    size_t size;
} vector_t;

/**
 * @brief Free a vector's backing storage and reset it to empty.
 *
 * @param[in,out] self Pointer to the vector to clear.
 */
static void vector_clear(vector_t* self)
{
    free(self->data);
    self->data = NULL;
    self->size = 0U;
}

/**
 * @brief Grow or shrink a vector's backing storage.
 *
 * @param[in,out] self Pointer to the vector to resize.
 * @param[in] size New number of elements.
 *
 * @return 0 on success, 1 if the allocation failed (self is left untouched).
 */
static int vector_resize(vector_t* self, const size_t size)
{
    if (NULL == self) { return 1; }
  
    if (0U == size)
    {
        vector_clear(self);
        return 0;
    }

    double* copy = (double*)(realloc(self->data, sizeof(double) * size));
    if (NULL == copy) { return 1; }
    self->data = copy;
    self->size = size;
    return 0;
}
```

`vector_resize()`'s `NULL == self` check is worth keeping, unlike a similar check you'll see argued
away later in this course (L03's `delay_ms()`, where the pointer is only ever passed one hardcoded
address from within the same file). The difference is what kind of function this is: `delay_ms()`
is a private, `static` helper with exactly one call site the compiler can see in full;
`vector_resize()` is written as the kind of function a reusable module would export, called from
code you don't control and can't audit by just reading the same file. Once "well-behaved caller"
stops being something you can verify by inspection, checking a pointer parameter before
dereferencing it stops being decorative and starts being the thing standing between a bad call and
a crash.

---

## A.12 Macros (`#define`)
You've already seen `#define` throughout this refresher: header guards (A.6),
`SENSOR_STATE_IDLE`-style constants (A.3), and every `LED1`/`F_CPU`-style pin and clock constant
this course leans on starting next lecture. It's worth understanding what it actually does before
writing your own.

* `#define` runs in the **preprocessor**, a separate text-substitution pass that happens before the
  compiler ever sees your code. `#define TIMEOUT_MS 100` doesn't create a typed variable; it tells
  the preprocessor to replace every later occurrence of `TIMEOUT_MS` with the literal text `100`,
  verbatim, before compilation starts. The compiler never sees the name `TIMEOUT_MS` at all.
* Because it's pure text substitution, a macro has no type, no scope, and isn't visible to a
  debugger by name; it's gone by the time there's a program to debug. It also isn't namespaced to a
  file, a struct, or a function the way a variable is: once defined, `TIMEOUT_MS` means the same
  thing everywhere until a matching `#undef`, even in files that `#include` this one.

### Object-like vs. function-like macros
* An **object-like macro** stands in for a value, `#define F_CPU 16000000UL` or `#define LED1 1U`,
  the pattern this course uses constantly for pin numbers and clock constants.
* A **function-like macro** takes parameters, syntactically like a function call, but is still pure
  text substitution: no type checking on the arguments, no single-evaluation guarantee:
  ```c
  #define SQUARE(x) (x * x)
  ```

### Always parenthesize: the body, and every parameter
This is the single most important rule for writing a macro, and the reason a definition like
`#define TICKS_NEEDED (TIMEOUT_MS / CLOCK_TIME_MS)` is written the way it is: the whole replacement
is wrapped in parentheses. Skip that, and the macro's meaning changes depending on what surrounds
it at the call site, because substitution happens *before* the usual rules of precedence get a
chance to group anything:

```c
#define TICKS_NEEDED TIMEOUT_MS / CLOCK_TIME_MS // No outer parentheses: fragile.

const uint16_t double_ticks = 2U * TICKS_NEEDED; // Expands to: 2U * TIMEOUT_MS / CLOCK_TIME_MS.
// * and / share precedence and evaluate left-to-right, so this one happens to still be correct.
```

```c
#define TICKS_NEEDED (TIMEOUT_MS / CLOCK_TIME_MS) // Parenthesized: safe regardless of context.

const uint16_t double_ticks = 2 * TICKS_NEEDED;
// Expands to: 2 * (TIMEOUT_MS / CLOCK_TIME_MS), unambiguous no matter what precedence surrounds
// the macro at its call site.
```

The `/`-only example above happens to survive without the parentheses because `*` and `/` share
precedence, but that's luck, not safety; swap in a macro built from `+` or `-` and it breaks
immediately:

```c
#define TIMEOUT_PLUS_MARGIN TIMEOUT_MS + 10U   // No parentheses: broken.

const uint16_t scaled = 2U * TIMEOUT_PLUS_MARGIN;
// Expands to: 2U * TIMEOUT_MS + 10U, not 2U * (TIMEOUT_MS + 10U). Silently wrong, no compiler
// warning.
```

The same rule applies to every parameter of a function-like macro, not just the overall body; each
individual use of a parameter needs its own parentheses too:

```c
#define SQUARE(x) (x * x)     // Unsafe: parameter isn't parenthesized.
#define SQUARE(x) ((x) * (x)) // Safe: parameter is parenthesized everywhere it's used.

SQUARE(a + b)
// Unsafe version expands to: (a + b * a + b), wrong.
// Safe version expands to:   ((a + b) * (a + b)), correct.
```

### Macro parameters can be evaluated more than once
A function-like macro's parameter isn't evaluated once and reused the way a real function
parameter is; the text you pass in gets substituted at every point the parameter name appears in
the macro's body. Anything with a side effect duplicates that side effect:

```c
#define SQUARE(x) ((x) * (x))

uint8_t i = 5U;
const uint8_t result = SQUARE(i++);   // Expands to ((i++) * (i++)): i is incremented twice, and
                                       // which multiplication reads which value is unspecified.
                                       // A real function would increment i exactly once.
```

Avoid passing an expression with side effects (`++`, `--`, assignment, a function call with side
effects) into a function-like macro.

### Multi-statement macros: wrap in `do { ... } while (0)`
A macro that expands to more than one statement breaks if it's ever used as the body of an `if`
without braces, because only the first statement ends up inside the `if`:

```c
#define LED_RESET LED_ON; LED_OFF;   // Two statements, no wrapper.

if (error) LED_RESET;   // Expands to: if (error) LED_ON; LED_OFF;
                        // LED_OFF now runs unconditionally, every time, regardless of `error`.
```

Wrapping the body in `do { ... } while (0)` turns it back into a single statement syntactically, so
it behaves correctly wherever a single statement is expected, including inside an `if` with no
braces:

```c
#define LED_RESET do { LED_ON; LED_OFF; } while (0)

if (error) LED_RESET; // Expands to a single do/while statement; behaves as one block.
```

This codebase always braces its `if`s (A.10), which sidesteps the specific failure shown above, but
the `do { ... } while (0)` idiom is still worth knowing: you'll see it in other people's code, and
it's the standard defense once you can't guarantee every call site remembers to brace its `if`.

### Prefer `const`/`enum` over `#define` where you can
A macro has no type and isn't scoped, so where a real alternative exists, it's usually the better
choice:
* A `const` variable is typed, scoped like any other variable, and visible to a debugger by name.
* An `enum` (A.3) groups related named integer constants the same way, with the same debugger
  visibility, and lets some compilers warn on an unhandled `switch` case.

Embedded C keeps reaching for `#define` anyway, for one specific reason: a `const` variable in C is
not a *compile-time constant expression* the way it is in C++. Even though its value can't change
at runtime, the compiler doesn't treat it as a literal for contexts that require one, such as an
array size or a `#if` preprocessor check. A `#define` (or an `enum` constant) is a true
compile-time constant and works in both. It also pays off in generated code for something like
`PORTB |= (1U << LED1)`: with `LED1` a compile-time constant, the compiler can usually fold the
whole expression into a single bit-set instruction; with `LED1` a `const uint8_t` variable, nothing
stops it from compiling, but the shift amount is only known at runtime, so the compiler may emit an
actual runtime shift instead of folding it away. That's the real reason this course's pin numbers
and bit positions are macros instead of `const uint8_t`s, not just convention.

---

## A.13 Bringing it together
The pattern you'll see throughout this course, and one worth internalizing before we start writing
to hardware registers next lecture, is small `static` functions, `const` everywhere possible,
explicit types, no hidden allocations, and a Doxygen comment above every public function that isn't
trivially self-explanatory:

```c
#include <stdio.h>


/**
 * @brief Compute resistance from voltage and current (Ohm's law).
 *
 * @param[in] voltage Voltage in V.
 * @param[in] current Current in mA.
 *
 * @return Resistance in kOhm.
 */
static inline double get_resistance(const double voltage, const double current)
{
    return voltage / current;
}

/**
 * @brief Print voltage, current, and resistance to the terminal.
 *
 * @param[in] voltage Voltage in V.
 * @param[in] current Current in mA.
 * @param[in] resistance Resistance in kOhm.
 */
static inline void print_quantities(const double voltage, const double current,
                                    const double resistance)
{
    printf("Voltage: %g V\n", voltage);
    printf("Current: %g mA\n", current);
    printf("Resistance: %g kOhm\n", resistance);
}

/**
 * @brief Application entry point.
 *
 * @return 0 on successful termination of the program.
 */
int main(void)
{
    const double voltage = 5.0;
    const double current = 0.5;
    const double resistance = get_resistance(voltage, current);
    print_quantities(voltage, current, resistance);
    return 0;
}
```

That style carries directly into embedded C, where you don't have a heap to hide behind anyway.

Practice exercises for this appendix are in [Appendix B](./b_exercises.md).

---
