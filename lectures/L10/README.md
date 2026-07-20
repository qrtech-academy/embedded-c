# L10 - Encapsulation and Polymorphism in C

## Agenda
* Hiding functions with `static`, and storing function pointers directly on an otherwise still
  public `gpio_t`, before tackling the struct's fields.
* Why exposing every struct field lets callers break invariants, and how an opaque struct
  (forward-declared in a header, defined only in a `.c` file) prevents that.
* Function pointers: syntax, storing one in a struct, calling through it.
* Worked example: an interrupt-driven `gpio_t` that dispatches a stored callback from `main()`'s
  loop, an ISR only flags that an event occurred.
* Vtables: a struct of function pointers as a hand-written interface, and how it relates to what
  C++'s `virtual` does under the hood.
* Worked example: a `gpio_interface_t` interface that real ATmega328P hardware sits behind,
  alongside a swappable in-memory stub for testing without a chip attached.

---

## Goals of the lecture
* Explain how `static` restricts a function's visibility to its own file, and why that's a
  different kind of encapsulation than hiding struct fields.
* Explain what an opaque struct is and why hiding a struct's definition in a `.c` file protects
  its invariants.
* Be able to declare, assign, and call through a function pointer, including one stored in a
  struct field.
* Be able to design a small vtable-style interface and provide more than one implementation of
  it.
* Understand, at a high level, how this manual pattern relates to virtual dispatch in
  higher-level languages.

---

## Instructions

### Before the lecture
* Read [Appendix A](./appendix/a_encapsulation_and_polymorphism.md) for the lecture's core
  material.
* This lecture assumes L09's `gpio_t` driver and `timer_t` struct, plus function pointer syntax
  from [L01 Appendix A.8](../L01/appendix/a_c_fundamentals.md#a8-function-pointers).

### During the lecture
* Participate actively in the walkthrough.

### After the lecture
* Work through the exercises in [Appendix B](./appendix/b_exercises.md).

---

## Evaluation
* What stops calling code from calling the `static` `gpio_write()` in A.2's `gpio.c` directly by
  name, and what does it *not* stop it from doing to `led1.write` or `led1.pin`?
* Why can't code outside `timer.c` read or write a `timer_t`'s fields directly, once `timer_t` is
  made opaque?
* What does `void (*on_press)(void);` declare, and how would you call it?
* Why does the pin-change ISR only set a flag instead of calling the callback directly?
* What problem does a vtable-style interface solve that hardcoding a single GPIO implementation
  doesn't?

---

## Next lecture
* This closes the driver-design arc (L09-L10), built on the peripherals covered in L01-L08.
* Everything here, opaque structs, function pointers, vtables, was C standing in for object
  orientation. The next step is Modern Embedded C++, where these patterns become language features
  (classes, access specifiers, virtual functions) instead of conventions enforced by hand.
* Covered in a separate course,
  [Modern Embedded C++](https://github.com/qrtech-academy/modern-embedded-cpp).

---
