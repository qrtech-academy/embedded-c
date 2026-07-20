# L01 - C Fundamentals

## Agenda
* Program structure, compiling, and data types: a fast pass over what's the same as languages you
  already know.
* Structs, enums, and typedefs: grouping data, naming states, and the `self`-pointer convention
  used throughout this course.
* Functions, `static`, and header guards: how a `.c`/`.h` pair is organized in this codebase.
* Pointers: out-parameters, arrays/strings, and the pointer-to-pointer idiom for replacing a
  caller's pointer.
* Dynamic memory allocation (`malloc`/`realloc`/`free`), and why embedded C mostly avoids it.
* Worked example: pulling all of the above together in one small program.

---

## Goals of the lecture
* Be comfortable enough with C's core syntax (types, structs, functions, pointers) to not be
  fighting the language once hardware programming starts next lecture.
* Be able to define and use a `struct`, `enum`, and `typedef`, and explain why `self` is passed as
  a pointer.
* Be able to write a `.c`/`.h` pair with a correct header guard and a sensible static/public
  ordering.
* Understand the difference between a single pointer (`int*`) and a double pointer (`int**`) as a
  function parameter, and when each is needed.
* Be able to use `malloc`/`realloc`/`free` correctly, including why a pointer should be nulled
  immediately after `free`.

---

## Instructions

### Before the lecture
* Read [Appendix A](./appendix/a_c_fundamentals.md) for the lecture's core material.

### During the lecture
* Participate actively in the walkthrough.

### After the lecture
* Work through the exercises in [Appendix B](./appendix/b_exercises.md).

---

## Evaluation
* Why is `self` passed as a pointer rather than by value in this codebase's struct functions?
* What's the difference between a single pointer (`int*`) and a double pointer (`int**`) as a
  function parameter, and when do you need the second?
* Why must you capture `realloc`'s return value in a temporary before overwriting your original
  pointer?
* Why should you set a pointer to `0`/`NULL` immediately after `free`?

---

## Next lecture
* Hardware-near programming in C: registers, bitwise I/O, and driving/reading pins on the
  ATmega328P.

---
