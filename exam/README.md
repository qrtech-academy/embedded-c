# Written Examinations

Two complete three-hour papers for **Embedded C**, with worked solutions.

```text
paper_a.md              Paper A, questions only. Hand this out.
paper_a_solutions.md    Paper A, model answers with marks.
paper_b.md              Paper B, questions only.
paper_b_solutions.md    Paper B, model answers with marks.
```

Each paper is self-contained. It states its own rubric, and it closes with a register reference
covering every ATmega328P register and bit it refers to, so nobody sitting one needs a datasheet, an
appendix from the lectures, or this file.

---

## What these are for

The course as written has **no exam and nothing is marked**. What you get instead is the exercise
set after every lecture, and the Evaluation questions that close every lecture README.

These papers do not replace that. **They are here for you to test your own skills and knowledge,
nothing more.** They gate nothing, they are not a qualification, and no part of the course requires
them. Nothing in the repository depends on them, and neither `make build` nor `make format-check`
knows they exist.

What they check is something the exercises cannot. Every exercise is done at a keyboard, with the
appendix open in the next tab, the datasheet in the one after that, and a board on the desk to tell
you whether it worked. These papers give you a page of register names and take the rest away, then
ask what you can reconstruct — which of the rules you have absorbed and which you have merely been
looking up.

**Take one after the course is over.** Both papers draw on all ten lectures, so sitting one partway
through examines material nobody has taught you yet, and the result says more about how far you have
read than about what you have understood.

---

## The two papers

Both cover the whole course, L01 to L10, at the same weighting. They share no question. Either can
be used alone; use both as a main sitting and a resit, or in alternate years.

| Question | Topic                                             | Lectures | Marks   |
| -------- | ------------------------------------------------- | -------- | ------- |
| 1        | The C language underneath: pointers, macros, types | L01      | 12      |
| 2        | Registers and bitwise I/O                          | L02      | 12      |
| 3        | Interrupts, edges and `volatile`                   | L03      | 12      |
| 4        | The ADC, and timers that count                     | L04, L05 | 13      |
| 5        | Serial communication                               | L06      | 13      |
| 6        | EEPROM and the watchdog                            | L07, L08 | 13      |
| 7        | Structs as drivers                                 | L09      | 12      |
| 8        | Encapsulation and polymorphism in C                | L10      | 13      |
|          |                                                    |          | **100** |

**Both papers mix theory with code**, because the subject does. Roughly half the marks are for prose
— what `volatile` guarantees, why an ISR must not transmit, what an opaque struct protects — and
roughly half are for reading or writing C on paper.

The two papers differ in which half they lean on:

* **Paper A leans towards code that is put in front of you.** All eight of its questions hand you a
  snippet that compiles without a warning and is wrong: a macro whose missing parentheses silently
  turn 50 into 32, a `free()` that leaves the caller holding the freed address, a `setup()` that
  writes board pin numbers where port bit positions belong and therefore writes zero to every
  register, an interrupt armed at the peripheral but never enabled globally, an ADC read with no
  reference and no wait, a UART driver with the baud *rate* written into `UBRR0`, an EEPROM write
  with `EEPE` set before `EEMPE`, a watchdog "pet" that sets a bit instead of executing an
  instruction, a `gpio_toggle()` that uses `^=` on `PINx`, and a vtable whose interface field is not
  the struct's first member. Naming the defect is half the marks; saying what it does to the running
  program is the other half.
* **Paper B leans towards code you write and design decisions you justify.** It asks for a complete
  three-LED chase program written from nothing, an enumeration and its validity check, a
  `realloc()`-based resize that survives a failed allocation, an INT1 handler with an atomic toggle,
  `adc_read()` in full, `driver/serial.h` in full with its Doxygen, the EEPROM read/write pair, the
  watchdog timeout enumeration and its two odd values, a GPIO driver that maps an Arduino pin number
  to a port, a pin reservation table, and a `gpio_stub_t` that satisfies the vtable interface with no
  hardware behind it. Several parts are pure judgement: which of four values belongs in EEPROM and
  why, where a report belongs relative to the interrupt that triggered it, and what opacity takes
  away from a caller.

Neither paper asks anything of a compiler that a careful reader could not answer from the appendices.
Where a question turns on a rule that is easy to state and easy to get backwards — which register
reports a pin's actual level, which of `EEMPE` and `EEPE` comes first, why `PINx = (1U << n)` is
atomic and `PORTx ^= (1U << n)` is not — the solution says so at length, because that is where the
marks and the understanding both are.

---

## Conventions the papers assume

Both papers state these in their own rubric, so a candidate never has to have read this file.

* **C11 on an ATmega328P at 16 MHz**, built with `avr-gcc`. Standard and AVR headers may be assumed
  included unless a question asks for them.
* **Register and bit names are not memory-tested.** Each paper closes with a register reference of
  its own, so nobody sitting one needs a datasheet in the room. Misspelling a bit costs nothing;
  reaching for the wrong *register*, or the right register at the wrong *time*, costs everything.
  What the reference deliberately withholds is the ordering rules, the timing rules and the
  formulas — those are the parts being examined.
* **Code is marked on semantics, not syntax.** A missing semicolon, a forgotten `#include` or a brace
  in the wrong column costs nothing. A missing `volatile` costs everything.
* **The course's house style is expected** where a question asks for it: `snake_case` with
  `UPPER_CASE` macros, `static` on anything private, header guards named after the file, Doxygen in
  the imperative mood, `self` as a pointer (and `const` when the function only reads), Yoda
  comparisons, braces on every `if`, fixed-width types with `U` suffixes, `const` on by-value
  parameters in the definition but not the prototype, and `volatile` on anything shared with an ISR.
* **"State the consequence" means the consequence.** Naming a defect earns half the marks; saying
  what it does to the running program earns the other half. "This is wrong" scores nothing.
* **Where a question asks for a stated number of defects**, listing more is not penalised, but only
  that many are marked, so the strongest answers go first.

---

## Marking

Every solution is written to be marked by somebody who has read the appendices and does not
otherwise write embedded C for a living, so each carries the reasoning rather than the answer alone.
Marks are shown per part, and the arithmetic in the timer, ADC and baud-rate questions is worked
through step by step rather than asserted.

Three conventions worth agreeing before a paper is marked:

* **Method carries the marks.** A correct diagnosis with a clumsy fix is worth more than a correct
  fix with no diagnosis. Several questions feed the next part, so an error should be followed through
  rather than penalised twice.
* **The named traps are worth full marks on their own.** A number of parts exist entirely to see
  whether a candidate walks into a specific mistake: reading a button from `PORTx`, a `1U << 9U`
  written to an 8-bit register, a plain `=` where `|=` was needed, an ISR-shared flag without
  `volatile`, `EEPE` before `EEMPE`, a watchdog pet written as a register access, `^=` on `PINx`, a
  `gpio_t*` passed by value, an interface field that is not first in its struct. Where a solution
  flags one of these, a candidate who walks into it loses those marks and no others.
* **The discussion parts are not decoration.** "State why" is where the course's actual content is,
  and a paper marked only on whether the code would compile would pass a candidate who has understood
  none of it.

---
