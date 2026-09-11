# Paper A - Solutions

Marks are shown per part. Method carries them: a correct diagnosis with a clumsy fix is worth more
than a correct fix with no diagnosis, and several later parts consume earlier answers, so an error
should be followed through rather than penalised twice.

Code is marked on semantics. Missing semicolons, missing includes and misplaced braces cost nothing.

**The candidate had the register reference at the back of the paper.** Every register and bit name,
every bit *position* where one matters, and the `ISCn1:ISCn0`, `ADPS2:ADPS0` and `CSn2:CSn0`
encodings were in front of them. What was deliberately withheld is the part being examined: the
ordering rules, the timing rules and the formulas. So a misspelled bit costs nothing, but a candidate
who cannot say which of `EEMPE` and `EEPE` comes first, or derive an encoding from bit positions the
reference gave them, has not answered the question.

---

## Question 1 - The language underneath (12 marks)

### (a) 3 marks

**The substituted text.** The preprocessor replaces the parameter with the argument's text,
verbatim, before the compiler ever runs:

```c
const uint16_t duty = (2U + 3U * 100U / 10U);
```

*(1 mark)*

**The value.** `*` and `/` bind tighter than `+`, so this is `2U + ((3U * 100U) / 10U)` = `2 + 30` =
**32**. The author intended `((2U + 3U) * 100U) / 10U` = **50**.
*(1 mark)*

**The corrected definition.** Every use of a parameter gets its own parentheses, and so does the
whole body:

```c
#define DUTY_PERCENT(on_ms) (((on_ms) * 100U) / PWM_PERIOD_MS)
```

*(0.5 marks)*

**The hazard parentheses cannot remove: multiple evaluation.** A macro parameter is substituted at
every point its name appears in the body, so an argument with a side effect happens once per
appearance. `SQUARE(x) ((x) * (x))` is fully parenthesised and `SQUARE(i++)` still modifies `i`
twice, with nothing to order the two, which is undefined behaviour. A real function evaluates its
argument exactly once. `DUTY_PERCENT` names `on_ms` only once, so it escapes this particular time —
but that is a property of this body, not a guarantee, and it changes the day somebody adds a second
use.
*(0.5 marks)*

*Credit as an alternative third answer: a macro has no type, so nothing checks what is passed in,
and the error message names the expansion rather than the macro.*

### (b) 3 marks

**What `data` holds: the same address it held before.** Everything in C is passed by value,
including pointers. `release()` received a *copy* of `data`; `free(buf)` correctly released the
block, and `buf = NULL;` then nulled the copy, which ceased to exist at the closing brace. The
caller's `data` was never reachable from inside the function.
*(1 mark)*

**What it has become: a dangling pointer** — a pointer still holding the address of a block that has
been returned to the heap.
*(0.5 marks)*

**What the write is.** `NULL != data` is true, so the guard passes and `data[0U] = 1U;` writes
through a freed pointer. That is a use-after-free, which is **undefined behaviour**, not a crash you
can rely on: the block may have been handed to a later `malloc()`, so the write silently corrupts
somebody else's data instead of failing anywhere near the mistake.
*(0.5 marks)*

**The correction** is the pointer-to-pointer idiom: to change the caller's pointer, the function
needs that pointer's address.

```c
/**
 * @brief Free a dynamically allocated buffer and null out the caller's pointer.
 *
 * @param[in,out] buf Address of the pointer to the buffer to free.
 */
static void release(uint8_t** buf)
{
    if ((NULL == buf) || (NULL == *buf)) { return; }
    free(*buf);
    *buf = NULL;
}
```

```c
release(&data);
```

*(1 mark)*

### (c) 3 marks

**`static` at file scope: internal linkage.** The name is not visible outside its own `.c` file and
is not emitted for the linker to resolve. It is the closest thing C has to "private", and because
the compiler can see every call site it can inline and optimise more aggressively. This course marks
every helper in a `main.c` `static` for exactly that reason.
*(1 mark)*

**`static` on a local variable: static storage duration.** The variable's *scope* is unchanged — it
is still only nameable inside its block — but its *lifetime* becomes the whole program, so it keeps
its value between calls and is initialised once. `static uint8_t ovf_count = 0U;` inside
`ISR(TIMER0_OVF_vect)` is the course's example.
*(1 mark)*

**Why `#define` for pin numbers**, any two of:

* **A `const` variable in C is not a constant expression.** Its value cannot change at runtime, but
  the language still refuses it anywhere a *constant expression* is required: the size of a
  file-scope array, a `case` label, a `#if`. A macro works in all of them; an `enum` constant works
  in the first two but not in `#if`, because the preprocessor runs before the compiler and knows
  nothing about enums. This is the real reason embedded C keeps reaching for `#define` and not
  merely habit.
* **Code generation.** With `LED1` a literal, `PORTB |= (1U << LED1)` folds at compile time into a
  single bit-set instruction. With `LED1` a `const uint8_t` variable, the shift amount is, as far as
  the language is concerned, only known at runtime, so nothing stops the compiler emitting an actual
  shift.
* The macro is also usable from a `#if` in the preprocessor, before there is any such thing as a
  variable.

*(1 mark for any two)*

*A candidate who also notes the cost — a macro has no type, no scope, and is invisible to a debugger
by name — has understood the trade-off rather than just the rule. Credit it, but not in place of one
of the two above.*

### (d) 3 marks

**The value is 0.** Both operands of `/` are unsigned integers, so the division is integer division
and truncates: `512U / 1023U` is `0`, not `0.5005`. `0 * 10U` is `0`, and the cast to `uint8_t`
changes nothing. The LED is never lit.
*(1 mark)*

**The only reading that behaves differently is 1023**, the top of the ADC's range: `1023U / 1023U`
is `1`, so `on_ms` becomes `10` — full brightness. The pot therefore does nothing at all across its
entire travel and then snaps to fully lit at the very end of it.
*(0.5 marks)*

**Correction 1 — multiply before dividing:**

```c
const uint8_t on_ms = (uint8_t)((adc_val * PWM_PERIOD_MS) / ADC_MAX);
```

`512 * 10 = 5120`, `5120 / 1023 = 5`. *(0.5 marks)*

**The trap it introduces: silent overflow in the intermediate product.** The multiplication is
carried out in the promoted type, and on AVR `int`/`unsigned int` is **16 bits**. `1023 * 10` is
10230 and fits; change `PWM_PERIOD_MS` to `100U` and `1023 * 100` is 102300, which wraps to 36764
with no warning and no runtime symptom other than a wrong duty cycle at the top of the range.
Widening one operand — `((uint32_t)(adc_val) * PWM_PERIOD_MS) / ADC_MAX` — removes it.
*(0.5 marks)*

**Correction 2 — the appendix's, in floating point:**

```c
const float duty_cycle = (float)(adc_val) / ADC_MAX;
const uint8_t on_ms    = (uint8_t)(duty_cycle * PWM_PERIOD_MS + 0.5F);
```

The cast on the numerator forces the division to be done in `float`; `+ 0.5F` before the truncating
cast rounds to nearest rather than always down. `0.5005 * 10 + 0.5 = 5.505` → `5`.
*(0.5 marks)*

---

## Question 2 - Registers, and a program that does nothing (12 marks)

### (a) 6 marks

*(1.5 marks per defect: 0.5 for naming it, 0.5 for the consequence, 0.5 for the corrected line.)*

**1. Board pin numbers are used where port bit positions are required.**

D9 is `PORTB1`, D10 is `PORTB2` and D13 is `PORTB5` — the comments say so — but the macros hold
`9U`, `10U` and `13U`. `1U` is an `unsigned int`, 16 bits on this target, so `1U << 9U` is `512`
(`0x0200`); assigning it to the 8-bit `DDRB` keeps only the low byte, which is `0x00`. The same goes
for `1U << 10U` (1024 → `0x00`) and `1U << 13U` (8192 → `0x00`). **Every register write in the
program writes zero.** Neither LED is ever configured as an output, the pull-up is never enabled,
and `(1U << BTN1)` is a zero mask, so the button test can never be true. This alone accounts for the
program doing nothing.

The compiler does notice the three plain assignments: avr-gcc warns `large integer implicitly
truncated to unsigned type` (`-Woverflow`, on by default), which the course's `-Werror` would turn
into a failed build. The test, the `|=` and the `&=` compile silently.

```c
#define LED1 1U // D9  -> PORTB1.
#define LED2 2U // D10 -> PORTB2.
#define BTN1 5U // D13 -> PORTB5.
```

**2. `DDRB` is assigned twice, not accumulated.**

`=` replaces the whole register. The second write drops LED1's direction bit, so once defect 1 is
fixed LED1 is still an input and never lights.

```c
DDRB = (1U << LED1) | (1U << LED2);
```

*(`DDRB |= (1U << LED2);` on the second line earns the same marks.)*

**3. The button is read from `PORTB` instead of `PINB`.**

On an input pin `PORTx` is the pull-up setting, not the pin level, and `setup()` has just set that
bit to `1`. The condition is therefore permanently true regardless of the button, so LED1 would be
permanently lit rather than following anything. `PINx` is the register that reports the actual logic
level on the pins.

```c
if (PINB & (1U << BTN1)) { PORTB |= (1U << LED1); }
```

**4. The clear is missing its `~`.**

`PORTB &= (1U << LED1)` ANDs the whole port with a mask in which only LED1 is set. Every *other*
bit is cleared — LED2 goes dark and BTN1's pull-up is switched off — while LED1 itself keeps
whatever value it already had, so the branch that is supposed to turn LED1 off never turns anything
off. The clear-bit idiom is AND with the *negated* mask.

```c
else { PORTB &= ~(1U << LED1); }
```

*A candidate who also objects to enabling the internal pull-up at all, against an external
pull-down, is right: the two form a divider that holds the idle pin near 1.1 V with a typical
pull-up, and only a typical one keeps it safely below the 1.5 V a `0` needs. Credit it, but not in
place of one of the four; the course's own programs enable it too, and it is not why this one does
nothing.*

### (b) 3 marks

| Operation | Expression |
| --------- | ---------- |
| Set bit `n` | `reg \|= (1U << n);` |
| Clear bit `n` | `reg &= ~(1U << n);` |
| Test bit `n` | `(reg & (1U << n))` |
| Toggle bit `n` | `reg ^= (1U << n);` |

*(2 marks for all four; 1 mark for two or three.)*

**Why the course toggles through `PINx`.** Writing a `1` to a `PINxn` bit is a documented feature of
this chip: it toggles the corresponding `PORTxn` bit, whatever `DDRxn` says, and bits written as `0`
leave their counterpart alone. So `PINB = (1U << LED1);` is a **pure write** — no read at all.

`PORTB ^= (1U << LED1);` is a read-modify-write: read `PORTB`, flip one bit, write the byte back. If
an interrupt fires between the read and the write and changes a different bit of `PORTB`, that
change is inside the byte the interrupt wrote but not inside the byte the main program is about to
write back, so it is silently overwritten. The `PINx` write has no read and therefore no window: the
toggle is atomic no matter what else is touching the port.
*(1 mark)*

### (c) 3 marks

**`F_CPU` is the CPU clock frequency in Hz.** `<util/delay.h>` computes its busy-loop iteration
counts from it **at compile time**, so it must be defined *before* that header is included;
otherwise the header has already been expanded against its own fallback and every delay in the
program is wrong by whatever ratio.
*(1 mark)*

**The `UL` suffix makes the literal an `unsigned long`, explicitly.** `int` is only 16 bits on AVR,
far too small for 16,000,000. The value would not be truncated without the suffix: C gives an
unsuffixed decimal constant the first of `int`, `long` and `long long` that can hold it, so
`16000000` would quietly become a 32-bit signed `long`. `UL` states the type rather than leaving it
to that rule, so every calculation built on `F_CPU`, a delay's loop count or a baud-rate divisor, is
done in 32-bit unsigned arithmetic by design rather than by accident.
*(1 mark)*

**Why `_delay_ms()` cannot take a variable.** The same compile-time computation: `_delay_ms()`
turns its argument into a loop count while compiling, so the argument has to be a constant
expression. A `uint16_t` variable is not one.

The course's helper works around it by calling `_delay_ms()` with a *constant* and looping:

```c
static void delay_ms(const uint16_t ms)
{
    for (uint16_t i = 0U; i < ms; ++i)
    {
        _delay_ms(TICK_1MS);
    }
}
```

*(1 mark)*

---

## Question 3 - An interrupt that never fires (12 marks)

### (a) 6 marks

*(1.25 marks per defect; 1 mark for the final part.)*

**1. `sei()` is never called.**

Every peripheral interrupt is gated by the global `I` bit in `SREG`, which is clear out of reset.
`EIMSK` arms INT0 at the peripheral, but with `I` clear the core never takes the vector, so
`ISR(INT0_vect)` never runs. `led1_state` stays `false` forever and the LED never lights. Add
`sei();` as the last line of `setup()`.

**2. The wrong edge is selected.**

`EICRA = (1U << ISC01);` sets `ISC01:ISC00` to `10`, which is the **falling** edge. The button is
wired through an external pull-down, so a press drives the pin *high* and a release drives it low —
the handler therefore runs on release, not on press-down. For a rising edge, both bits are set:

```c
EICRA = (1U << ISC00) | (1U << ISC01);
```

**3. `led1_state` is not `volatile`.**

It is written only inside the ISR and read only inside `main()`'s loop. Nothing in that loop, as far
as the compiler can see, ever changes it, and the compiler has no idea an interrupt exists — so it
is entitled to load the variable once before the loop and test the same register value on every
iteration thereafter. The loop then runs forever on a stale `false`. Declare it
`static volatile bool led1_state = false;`.

**4. `serial_print()` is called from inside the ISR.**

`serial_write_byte()` blocks on `UDRE0` for each byte. At 9600 baud a 15-byte message (13 characters
plus `\r\n`) is 150 bit-times, about **15.6 ms**, and for nearly all of that time every other
interrupt on the chip is held off — including the debounce timer a program like this depends on. An
ISR should flip a flag and return; the transmission belongs in `main()`'s loop:

```c
ISR(INT0_vect)
{
    led1_state = !led1_state;
    led1_event = true;   // static volatile bool, read and cleared in main().
}
```

**Which one must be fixed first: the missing `sei()`.** Until the `I` bit is set the handler never
executes at all, so the wrong edge, the stale read and the blocking transmission have no
opportunity to show themselves. Every one of them is still a defect — they are simply unobservable
while the interrupt is disabled, which is what makes fixing `sei()` alone so misleading: three
latent faults arrive at once.
*(1 mark)*

### (b) 3 marks

D7 is `PORTD7`, which is not an external-interrupt pin, so it needs a **pin change interrupt** on
port D:

```c
#define BTN1 7U // D7 -> PORTD7.

PCICR  = (1U << PCIE2);  // Enable pin change interrupts on port D.
PCMSK2 = (1U << BTN1);   // Specifically D7 / PORTD7.
sei();                   // Enable interrupts globally.

ISR(PCINT2_vect) { /* ... */ }
```

*(1.5 marks: 0.5 for `PCICR`/`PCIE2`, 0.5 for `PCMSK2` with the right bit, 0.5 for the vector.
The bit position written into `PCMSKn` is the pin's own bit position within its port; the global
`PCINT23` number is only for looking the pin up in the datasheet.)*

**Two behavioural differences**, both of them forced rather than optional:

1. **Pin change interrupts have no edge selection.** There is no `EICRA` equivalent; the interrupt
   fires on **both** edges, press and release. If only one is wanted, the handler must read the
   pin's actual level itself and return without acting otherwise — the filtering moves from hardware
   into software.
2. **The vector is shared by the whole port.** `PCINT2_vect` is the handler for every enabled PCI
   pin on port D, and nothing in the call says which pin changed. With more than one enabled, the
   handler has to work it out, typically by keeping a previous-`PIND` byte and XOR-ing it against
   the current read to see which bit flipped.

*(1.5 marks, 0.75 each)*

### (c) 3 marks

**What `volatile` says.** That the object may be read or written by something the compiler cannot
see, so every access in the source must become a real memory access in the output: no caching the
value in a register across statements, no eliding a write that appears redundant, no reordering
accesses to it relative to each other.
*(1 mark)*

**Why the bug is build-dependent.** Nothing about omitting `volatile` is wrong in a way the compiler
must diagnose; it merely *permits* an optimisation. At `-O0` the compiler reloads from memory
anyway, so the program works and the omission is invisible. Turn on `-Os` or `-O2` — which is what a
release build does — and the load is hoisted out of the loop. Same source, same board, same
hardware; the fault appears only when the build changes, which is why it is so unpleasant to track
down after the fact.
*(1 mark)*

**One that does not need it.** `static uint8_t ovf_count` declared *inside*
`ISR(TIMER0_OVF_vect)`. It is function-local: nothing outside that one handler can name it, so the
compiler can see every read and write to it and there is no invisible second party. The appendix
marks it `volatile` as a habit and says so explicitly; the moment anything outside the ISR needs to
read it, the qualifier stops being decoration.
*(1 mark)*

*Credit equally `led_state` in the same appendix's Timer 1 example, which is genuinely shared: the
button's ISR writes it and the timer's ISR reads it. On the AVR an ISR runs to completion with
interrupts disabled, so nothing can change `led_state` in the middle of either handler, and each
reads it afresh every time it runs; the appendix keeps `volatile` on it by the course's rule, and
it becomes load-bearing the moment `main()`'s loop reads it too. `led1_lit`, touched only by the
timer's ISR, is the same case as `ovf_count`.*

---

## Question 4 - Converting and counting (13 marks)

### (a) 5 marks

*(1 mark per defect, 1 mark for the replacement line.)*

**1. No voltage reference is selected.**

`ADMUX = channel;` leaves `REFS1:REFS0` at `00`, which selects the external `AREF` pin. On an
Uno/Nano nothing drives `AREF`, so the converter is measuring against a floating reference: the
results are plausible-looking numbers that drift and bear no fixed relationship to the input.
Setting `REFS0` selects `AVCC`, the chip's own supply.

```c
ADMUX = (1U << REFS0) | channel;
```

**2. No prescaler is configured.**

`ADPS2:0` is left at `000`, a division factor of 2, so the ADC clock runs at 8 MHz. The
successive-approximation hardware needs to be below roughly 200 kHz for full 10-bit accuracy; at
8 MHz the low bits are noise. At 16 MHz, ÷128 gives 125 kHz:

```c
ADCSRA = (1U << ADEN) | (1U << ADSC) | (1U << ADPS0) | (1U << ADPS1) | (1U << ADPS2);
```

**3. The conversion is never waited for.**

`ADCSRA = (1U << ADIF);` executes immediately after the start, so the result registers are read
while the conversion is still in flight; what comes back belongs to a previous conversion or to
nothing. Worse, that plain assignment also writes `0` to `ADEN`, switching the ADC off, which
*aborts* the conversion it just started. The flag must be polled first and only then cleared:

```c
while (0U == (ADCSRA & (1U << ADIF))) {}
ADCSRA = (1U << ADIF);
```

**4. `ADCH` is read before `ADCL`, and the shift precedes the widening cast.**

Reading `ADCL` first locks both result registers against update until `ADCH` has been read too, so
the pair is guaranteed to come from the same conversion. Reading `ADCH` first leaves that lock
unarmed: a conversion completing between the two reads tears the result, and the two halves end up
belonging to different samples.

In this function the wrong order does something worse, and it fits the symptom. The last read is
`ADCL`, so the function returns with the result registers still locked, and a conversion that
completes while they are locked is lost: neither register is updated. The next call then reads the
previous conversion's result again, and re-arms the lock as it goes. The numbers stay plausible and
stop tracking the potentiometer.

Separately, `high` is a `uint8_t`, so `high << 8U` promotes it to a 16-bit *signed* `int` and shifts
there; the `(uint16_t)` cast is applied to the result, too late to have protected anything. The cast
belongs on the operand:

```c
const uint8_t low  = ADCL;
const uint8_t high = ADCH;
return ((uint16_t)(high) << 8U) | (uint16_t)(low);
```

*`ADCH` never uses more than its bottom two bits, so this particular shift happens to be safe either
way — but the habit is what keeps it safe unconditionally, and a candidate who says so has answered
better than one who claims a bug that cannot fire here.*

**The one line that replaces the last three:**

```c
return ADC;
```

`ADC` is a macro that already combines `ADCL` and `ADCH` into the full 10-bit value, in the correct
order. Nothing is assembled by hand.
*(1 mark)*

### (b) 5 marks

**1.** Tick period and overflow period. *(1 mark)*

```text
tick period = 1024 / 16,000,000 s = 64 us = 0.064 ms
Timer 0 overflows every 256 ticks = 256 * 0.064 = 16.384 ms
```

**2.** Overflows for 750 ms. *(1 mark)*

```text
750 / 16.384 = 45.78  ->  46 overflows
46 * 16.384 = 753.664 ms, i.e. about +0.49 %
```

*(The course's macro rounds with `(uint8_t)(TIMEOUT_MS / OVF_PERIOD_MS + 0.5F)`, which gives the
same 46. A candidate who truncates to 45 and says so, giving −1.7 %, has done the arithmetic
correctly and should lose at most half a mark.)*

**3.** `OCR1A` for 40 ms on Timer 1 in CTC mode. *(2 marks)*

```text
ticks needed = 40 / 0.064 = 625
OCR1A counts 0 up to its value, so reaching it takes (OCR1A + 1) ticks:
OCR1A = 625 - 1 = 624
```

Vector: `ISR(TIMER1_COMPA_vect)`.

*Credit the configuration if it is given: `TCCR1B = (1U << CS10) | (1U << CS12) | (1U << WGM12);`
for the ÷1024 prescaler plus CTC mode, `OCR1A = 624U;`, `TIMSK1 = (1U << OCIE1A);`, `sei();`.*

**4.** Why Timer 0 cannot. *(1 mark)*

625 ticks does not fit an 8-bit counter, whose compare register stops at 255. Timer 0 can only reach
40 ms by counting overflow interrupts in software — which is precisely why the Normal-Mode example
in the appendix does it that way, and why Timer 1's 16-bit range is what lets CTC hit an arbitrary
period directly.

### (c) 3 marks

**What the LED does.** `ovf_count` is never cleared, so once it reaches `OVF_MAX` the condition
`OVF_MAX <= ++ovf_count` is true on **every** subsequent overflow. The LED therefore toggles once
per overflow — every 16.384 ms, about 30 Hz — which reads as a blur or a steady half-brightness
rather than a blink. It carries on until `ovf_count` wraps past 255 back to 0, at which point there
is one quiet interval of `OVF_MAX` overflows before the whole thing repeats.
*(1 mark)*

**Corrected:**

```c
ISR(TIMER0_OVF_vect)
{
    static uint8_t ovf_count = 0U;

    // Increment the overflow counter, toggle LED1 and clear the counter on timeout.
    if (OVF_MAX <= ++ovf_count)
    {
        LED1_TOGGLE;
        ovf_count = 0U;
    }
}
```

*(1 mark)*

**Timer 2's difference: its prescaler bit patterns are not the same as Timers 0 and 1.** `101`
selects ÷1024 on Timers 0 and 1, but ÷128 on Timer 2; ÷1024 on Timer 2 is `111`. Every register and
bit *name* exists on both (`CS20`/`CS22` next to `CS00`/`CS02`), so a configuration copied from one
timer to the other compiles cleanly and produces a timer running eight times too fast, with nothing
to point at.
*(1 mark)*

*The register reference gives both encodings side by side, so finding the discrepancy is a table
comparison rather than a feat of recall. The mark is for the second half — why it fails **silently**,
i.e. that `CS20`/`CS22` exist just as `CS00`/`CS02` do, so nothing in the copied line is even
suspicious. A candidate who only reports "the prescaler bits differ" earns half of it.*

---

## Question 5 - Talking to a PC (13 marks)

### (a) 4 marks

**The formula.** *(0.5 marks)*

```text
UBRR0 = F_CPU / (16 * baud_rate) - 1
```

**For 19200 baud at 16 MHz.** *(1.5 marks)*

```text
16,000,000 / (16 * 19200) = 16,000,000 / 307,200 = 52.083
52.083 - 1 = 51.083  ->  UBRR0 = 51
```

**The actual baud rate and the error.** *(1 mark)*

```text
16,000,000 / (16 * (51 + 1)) = 16,000,000 / 832 = 19230.8 baud
error = +0.16 %
```

**Why that is harmless.** UART is asynchronous but re-synchronises on every start bit. The receiver
only has to stay in step for the length of one frame — ten bit-times for 8-N-1 — before the next
start bit re-aligns it. A 0.16 % rate mismatch has accumulated 1.6 % of a single bit period by the
stop bit, against a budget of roughly half a bit; the usual tolerance is a couple of percent. Errors
never accumulate across bytes.
*(0.5 marks)*

**A terminal at 9600 while the chip sends 19200** does not go silent — the line is genuinely active.
It samples at half the true rate, so each of its bit slots spans two transmitted bits, start bits are
detected in the wrong places, and the stop bit frequently is not where it should be. The result is a
stream of garbage characters and framing errors.
*(0.5 marks)*

### (b) 5 marks

*(1.25 marks per defect.)*

**1. The baud *rate* is written into `UBRR0` instead of the value computed from it.**

`UBRR0` takes the divisor from part (a)'s formula, which for 9600 baud at 16 MHz is `103`, not
`9600`. Only the low 12 bits of the register pair are used, so `9600` (`0x2580`) is truncated to
`0x580` = 1408, giving `16,000,000 / (16 * 1409)` ≈ **710 baud**. Even without the truncation,
`UBRR0 = 9600` would produce about 104 baud. Either way the port is nowhere near 9600 and a terminal
opened there sees nothing but garbage.

```c
#define BAUD 103U // UBRR value for 9600 bps with a 16 MHz clock.
```

**2. `serial_write_byte()` never waits on `UDRE0`.**

`UDR0` can take a new byte only while `UDRE0` says it is free, and a byte written to it while
`UDRE0` is clear is ignored by the transmitter. The loop in `serial_print()` hands bytes over a few
CPU cycles apart, while at 9600 baud each one takes over a millisecond to send, so only the first
byte or two of each message reach the line and the rest are silently dropped. The wait belongs
before the write, so that every byte finds the register free:

```c
static void serial_write_byte(const uint8_t byte)
{
    // Wait until the transmit buffer is ready for a new byte.
    while (0U == (UCSR0A & (1U << UDRE0))) {}

    // Place the new byte in the transmit data register.
    UDR0 = byte;
}
```

**3. `serial_print()` ignores the string's terminator and reads past the end of it.**

A C string has no length field; it ends at the `'\0'`. This loop instead transmits a fixed
`MSG_LEN + 1` bytes — 51, because of the `<=` — regardless of the string it was given. For the
13-character literal at the call site it walks 38 bytes past the end and transmits whatever happens
to follow it in memory. It also has no `NULL` check, though it is an exported function with call
sites outside this file: exactly the case where the course's own rule says the check stops being
decorative.

```c
void serial_print(const char* text)
{
    // Terminate if the text pointer is NULL.
    if (NULL == text) { return; }

    // Transmit each character one by one.
    for (uint16_t i = 0U; text[i]; ++i)
    {
        serial_write_byte((uint8_t)(text[i]));
    }
    serial_write_byte('\r');
    serial_write_byte('\n');
}
```

**4. The transmission happens inside the ISR.**

Once `serial_write_byte()` waits on `UDRE0` as it must, it blocks once per byte. At 9600 baud, 15
bytes is 150 bit-times, about **15.6 ms**, and the ISR holds off every other interrupt for nearly
the whole of it — including the
debounce timer this pattern normally depends on, so presses arriving during the message are lost.
The handler should flag the event and return; `main()`'s loop does the printing.

```c
ISR(PCINT0_vect)
{
    if (BTN1_PRESSED) { btn1_event = true; }   // static volatile bool.
}
```

### (c) 4 marks

**The enable bit is `RXEN0`, in `UCSR0B`**, alongside `TXEN0`; set both to have transmit and receive
at once: `UCSR0B = (1U << TXEN0) | (1U << RXEN0);`
*(0.5 marks)*

**The flag is `RXC0`, in `UCSR0A`.** It is `UDRE0`'s counterpart: clear while nothing has arrived,
set the moment a fully received byte is sitting in `UDR0`. **Reading `UDR0` clears it**, and readies
the register for the next incoming byte — there is nothing to write back by hand.
*(1 mark)*

```c
/**
 * @brief Receive a single byte over UART, blocking until one arrives.
 *
 * @return Byte received.
 */
static char serial_read_byte(void)
{
    // Wait until a byte has been received.
    while (0U == (UCSR0A & (1U << RXC0))) {}

    // Return the received byte.
    return UDR0;
}
```

*(1 mark)*

**The failure mode: it blocks indefinitely.** If the sender never speaks, the program sits in that
loop forever and does nothing else — no button, no timer work, no main loop. With a watchdog armed,
the chip simply reboots instead, which at least makes the hang visible but does not make it correct.
*(0.5 marks)*

**What `read()` adds.** A `timeout_ms` parameter, and a buffer rather than a single byte. Non-zero,
it gives up after roughly that many milliseconds even if the buffer never filled, pacing itself one
millisecond at a time with `delay_ms(1U)`; `0` keeps the old block-until-done behaviour deliberately.
Its three return values are: **`-1`** for a bad call (`NULL` buffer or `buflen == 0`), a **count
below `buflen`** when the timeout elapsed first, and **`buflen`** itself when the buffer filled
completely.
*(1 mark)*

---

## Question 6 - Remembering, and recovering (13 marks)

### (a) 5 marks

*(1 mark per defect, 1 mark for the corrected function.)*

**1. There is no wait for a previous write to finish.**

`EEPE` stays set for the several milliseconds an EEPROM write takes. Loading a new address and data
while one is still in progress either loses the new write or disturbs the one under way. The first
line of the function must be `while (EECR & (1U << EEPE)) {}`.

**2. `EEPE` is set before `EEMPE`. This is why nothing is ever written.**

`EEMPE` is not a data bit; it is a safety interlock that arms the write, and `EEPE` must follow it,
not precede it. Setting `EEPE` with the interlock unarmed does nothing at all, and the later `EEMPE`
write arms an interlock that is never used. The function silently succeeds and stores nothing.

**3. Interrupts are not disabled around the sequence.**

The datasheet requires `EEPE` to be set within **four CPU cycles** of `EEMPE`. Four cycles is
nothing; any interrupt taken in that window pushes the second write outside it and the write silently
does not happen. `cli()` before the pair and `sei()` after is not optional.

**4. `main()` writes on every iteration of the loop.**

Each write blocks for roughly 3.4 ms, so the loop manages about 290 writes a second. Against the
cell's ~100,000-cycle endurance, that address is worn out in **under six minutes** — and for those
six minutes the program is doing nothing else, because the write blocks. The value should be written
only when it changes, which is what the `led1_event` flag in the course's demo is for.

**Corrected:**

```c
/**
 * @brief Write a byte to EEPROM at the given address.
 *
 * @param[in] addr EEPROM address to write to (0-1023).
 * @param[in] byte Byte to write.
 */
static void eeprom_write_byte(const uint16_t addr, const uint8_t byte)
{
    // Wait for any earlier write to finish.
    while (EECR & (1U << EEPE)) {}

    // Set destination address and data.
    EEAR = addr;
    EEDR = byte;

    // Perform write operation.
    cli();
    EECR |= (1U << EEMPE);
    EECR |= (1U << EEPE);
    sei();
}
```

**The intermittent one is defect 3**, the missing `cli()`/`sei()`.

**Why that is worse than one that never works.** A write that never works is caught by the first
test that reads the value back. A write that works except when an interrupt lands inside a four-cycle
window works on the bench, works in review, and fails in the field at a rate that depends on how
busy the interrupts happen to be — so the failure rate changes when unrelated code is added. Each
individual failure looks like "it didn't save that one time", which is indistinguishable from a
power cut at an unlucky moment, so the bug is easy to explain away and hard to reproduce.
*(Included in the marks above; a candidate who identifies defect 3 but not why it is worse loses
half a mark.)*

### (b) 4 marks

**`watchdog_init()`: the timed sequence is missing.** *(1 mark)*

Switching the watchdog *on* is always allowed: a write that sets `WDE` takes effect with no unlock.
What is protected, exactly as EEPROM's `EEPE` is, is clearing `WDE` and changing the prescaler bits,
and for a sharper reason: the interlock exists so that runaway code cannot accidentally switch off
or reconfigure the one thing meant to catch it. `WDCE` must be set together with `WDE` first, and
the real configuration written within four cycles. Without the unlock, the write enables the
watchdog but the prescaler bits keep their reset value, `0000`: the watchdog runs with a 16 ms
timeout instead of 1024 ms, and nothing reports a problem.

**`watchdog_reset()`: petting the watchdog is not a register write at all.** *(1 mark)*

`WDE` is the enable bit. Setting an already-set bit changes nothing and does not restart the
countdown. The countdown is restarted by a dedicated CPU instruction, `WDR`, which no assignment to
any register produces. As written, the watchdog is never pet. With the 16 ms watchdog that
`watchdog_init()` did switch on, the chip resets 16 ms after every boot, in an endless boot loop;
with `watchdog_init()` corrected, it resets every 1024 ms instead.

**Corrected:** *(1 mark)*

```c
/**
 * @brief Enable the watchdog timer with the given timeout.
 *
 * @param[in] timeout Timeout to arm the watchdog with.
 */
static void watchdog_init(const watchdog_timeout_t timeout)
{
    cli();
    WDTCSR |= (1U << WDCE) | (1U << WDE);      // Unlock the timed sequence.
    WDTCSR = (1U << WDE) | (uint8_t)(timeout); // Enable the watchdog with the given timeout.
    sei();
}

/**
 * @brief Reset the watchdog timer, restarting its countdown.
 */
static void watchdog_reset(void)
{
    __asm__("wdr");
}
```

**Why it cannot be a register access, and why `__asm__`.** *(1 mark)*

There is no bit anywhere whose write emits `WDR`, so no C statement produces it. The only way to
place a specific machine instruction in the compiler's output is **inline assembly**, which drops
the literal instruction straight into the generated code. The plain spelling `asm` is a GNU
extension, recognised only under a GNU dialect (`-std=gnu11`) or GCC's default mode; this course
builds with `-std=c11`, strict ISO C, where `asm` is not a keyword and the line fails to compile.
`__asm__` is recognised regardless of `-std=`, which is why the course uses that spelling.

### (c) 4 marks

**The flag is `WDRF`, the Watchdog Reset Flag, in `MCUSR`.** *(0.5 marks)*

**The mechanism.** *(1.5 marks)* A watchdog-caused reset sets `WDRF`, and while `WDRF` is set, `WDE`
**reads back as set no matter what is written to it**. Every attempt to clear `WDE` — or to disable
the watchdog by any other route — therefore does nothing at all, silently, and the watchdog stays
armed and fires again one timeout later. After a watchdog reset that timeout is the shortest, 16 ms,
because the reset cleared the prescaler bits, which is why the board resets many times a second.
That is the boot loop the colleague is seeing: the chip resets, `WDRF` is set again by that very
reset, and the disable is refused again on the next boot.

It is deliberate. It stops a chip that has just failed and reset from immediately switching off the
safety net that caught it, which is precisely the failure mode a watchdog exists for.

**The fix and why its position matters.** *(1 mark)*

```c
MCUSR &= ~(1U << WDRF); // Clear the flag; WDE cannot be cleared while it is set.
```

It must be the **first** line of `setup()`, unconditionally, before anything else touches `WDTCSR`.
Anything in between that tries to switch the watchdog off is silently ignored, so a clear placed
later does not undo those writes — they are simply lost. The watchdog must then be configured, with
the timeout actually wanted, or switched off, straight after the clear and still before anything
slow: after a watchdog reset it is running with that 16 ms timeout, and a `setup()` that first
spends 16 ms on something else (printing one line at 9600 baud takes that long) is reset before it
gets there. That is why the course's `watchdog_init()` clears `WDRF` and then configures the
watchdog, and is called first thing in `setup()`. Clearing the flag unconditionally costs nothing
and needs no knowledge of why the chip reset; a program that wants to know reads `MCUSR` into a
variable first.

**`WDE` and `WDIE`.** *(1 mark)*

* **`WDE` alone — System Reset Mode.** A timeout resets the chip.
* **`WDIE` alone — Interrupt Mode.** A timeout runs `ISR(WDT_vect)` and resets nothing.
* **Both set.** The timeout fires `ISR(WDT_vect)` *and* hardware clears `WDIE` on the way in,
  leaving `WDE` armed. That gives exactly **one warning**: the handler gets a chance to pet the
  watchdog, log the near-miss or shut something down safely, and if the *next* timeout is also
  missed the chip resets for real.

---

## Question 7 - One driver, many instances (12 marks)

### (a) 3 marks

**The macro approach.** Each LED needs its own pair of macros, `LED1_ON`/`LED1_OFF`, each with a
port and a bit position baked into its text. A sixth LED means a sixth copied-and-renamed pair, plus
another line in every function that acts on "all the LEDs". There is nothing to iterate over,
because the substitution happens before the compiler exists — a loop counter cannot select a macro.
*(1 mark)*

**The `gpio_t` approach.** The functions do not know or care which GPIO they were handed; the port
pointers and the bit live in the instance. A sixth LED is one more entry in a `gpio_t leds[]` array,
and every existing loop over that array picks it up with no new code at all.
*(1 mark)*

**Why the ADC, UART, EEPROM and watchdog drivers are right to have no struct.** The chip has exactly
one of each. There is no second instance to distinguish, so there is nothing for `self` to select
between and nothing to bundle; a struct would be ceremony around state that exists once. (`adc_read()`
already takes a channel number as an ordinary parameter, so it was never tied to a single input in
the first place.)

**What warrants one:** the moment a program can plausibly want *more than one* of the thing, each
with its own configuration and its own state. That is true of LEDs, buttons and software timers, and
false of the chip's single EEPROM.
*(1 mark)*

### (b) 6 marks

*(1.5 marks per defect: 0.5 for naming it, 0.5 for the consequence, 0.5 for the correction.)*

**1. `gpio_write()` takes `gpio_t` by value.**

Every call copies the whole struct — three pointers and a byte — onto the stack of an 8-bit MCU with
2 KB of RAM, for nothing. The deeper problem is what it means: the function is operating on a
*copy*. It happens to work only because it writes *through* the copied pointers rather than to
`self`'s own fields; the day it needs to change a field (a cached direction, a reservation flag, a
callback) the change is discarded at the closing brace and the caller never sees it. The course's
convention is a pointer, always.

```c
void gpio_write(gpio_t* self, const bool state)
```

**2. `gpio_read()` reads `portx` instead of `pinx`, and is not `const`-correct.**

On an input pin `PORTx` is the pull-up setting, not the pin level. The function therefore returns
whatever the pull-up was configured to, and never changes when the button is actually pressed —
"the button is stuck" with no bad instruction anywhere. `PINx` is the register that reports the
live logic level. And since the function only ever reads through `self`, its parameter should say so.

```c
bool gpio_read(const gpio_t* self)
{
    return (NULL != self) ? (*self->pinx & (1U << self->pin)) : false;
}
```

**3. `gpio_toggle()` uses `^=` on `PINx`.**

Writing a `1` to a `PINxn` bit toggles `PORTxn`; bits written as `0` are left alone. `^=` is a
read-modify-write, so it first *reads* `PINx` — the live level of **every** pin on the port — flips
one bit in that byte, and writes the whole thing back. Every other pin on the port that currently
reads high is written as `1`, and is therefore toggled too. Meanwhile the target bit is written as
`level XOR 1`: if the pin currently reads high it is written as `0`, so the one pin that was asked
for **does not toggle at all**. The correct form is a plain write — no read, no window, one bit:

```c
*self->pinx = (1U << self->pin);
```

**4. No `NULL` checks, and no direction check.**

These are exported functions of a reusable driver, called from files the author cannot audit: the
case where the course's own rule says a pointer check does real work rather than demonstrating a
habit. A `NULL` `self` here does not fault cleanly — address 0 on AVR is the register file, so
`*self->portx` reads a garbage pointer out of the register file and writes somewhere real.
`gpio_write()` and `gpio_toggle()` should also refuse a pin that is not configured as an output,
which is what the appendix's versions do.

```c
if (NULL == self) { return; }
const bool output = (*self->ddrx & (1U << self->pin));
if (!output) { return; }
```

### (c) 3 marks

**What goes where.** `gpio.h` is the public interface: the include guard, the `gpio_t` and
`gpio_mode_t` type definitions, and a documented prototype for every function callers are allowed to
use. `gpio.c` includes its own header, after the standard and AVR headers, then defines those same
functions in the order the header declares them; anything not in the header is `static` and lives
at the bottom of the file.
*(1 mark)*

**Why `gpio_t` has to be in the header, unlike anything in `sensor.h`.** Callers of `sensor.h` only
ever received a `double` back; they never named a type belonging to the module. Callers of `gpio.h`
must **declare their own instances** — `gpio_t led1 = {&DDRB, &PORTB, &PINB, LED1_PIN};` — and to
declare an object the compiler has to know its size and layout. A forward declaration alone would
leave `gpio_t` an incomplete type, which can be pointed at but never instantiated. That is exactly
the trade-off L10 revisits: hiding the fields costs the caller the ability to declare one, which is
why the opaque version has to replace `gpio_init()` with an allocating `gpio_create()`.
*(1 mark)*

**Why the `const` differs between the two files.** A top-level `const` on a pass-by-value parameter
constrains only what the function body may do with its own local copy. It is not part of the
function's type and cannot affect how any caller calls it, so in the prototype it says nothing and is
omitted; in the definition it says something real about the body, so it is kept.
*(1 mark)*

---

## Question 8 - Hiding things, and choosing at runtime (13 marks)

### (a) 4 marks

**What it declares.** That a type `struct gpio` exists, and that `gpio_t` is another name for it. It
deliberately does **not** declare the fields, so in every file that sees only this header `gpio_t` is
an **incomplete type**.
*(1 mark)*

**What a caller can do:** declare, copy, store, pass and compare `gpio_t*` variables, check them
against `NULL`, and hand them to the driver's functions.

**What a caller cannot do:** declare a `gpio_t` object, take `sizeof(gpio_t)`, put one in an array,
dereference the pointer, or read or write a single field. All of those need the layout, and the
layout is only in `gpio.c`.
*(1 mark)*

**Why `gpio_init(gpio_t* self, ...)` cannot survive.** That signature puts the storage in the
caller's hands: the caller declares the object and passes its address. Declaring the object needs a
complete type, which is exactly what opacity removes. So the allocation has to move to the only file
that knows the size — `gpio.c` — and the function becomes `gpio_create(...)`, allocating with
`malloc()` and returning the pointer, or `NULL` if the allocation failed.
*(1 mark)*

**The invariant.** `timer_t` carries an elapsed-time count that `timer_running()` and `timer_reset()`
depend on, a running/stopped state, and a reserved hardware timer/counter. With public fields any
file can set the elapsed count directly, skip `timer_init()` altogether, or leave the reservation
table and the instance disagreeing about which hardware timer is claimed — all of which the driver's
own functions then reason about incorrectly. `gpio_t` had nothing comparable: its fields are a port
address and a bit position, and setting them by hand is unhelpful rather than invariant-breaking,
which is why the appendix calls the exposure "mostly harmless" there and not on `timer_t`.
*(1 mark)*

### (b) 4 marks

**What the compiler reports: `unknown type name 'gpio_t'`**, on both function-pointer members.
*(0.5 marks)*

**Why.** `typedef struct gpio { ... } gpio_t;` introduces the name `gpio_t` only when the whole
declaration finishes, at the final `;` after the closing brace. Inside the braces the struct is still
being defined and that name does not exist yet. The *tag* `struct gpio`, by contrast, is in scope
from the moment `struct gpio {` is seen, which is what lets a member refer to a pointer to its own
struct type — the same rule a linked list's `struct node* next` relies on.
*(1.5 marks)*

**Corrected:**

```c
    /** Set state of the given GPIO. Only supported for outputs. */
    void (*write)(struct gpio* self, bool state);

    /** Read state of the given GPIO. */
    bool (*read)(const struct gpio* self);
```

Everywhere *outside* the struct, `gpio_t` remains the name to use; only these member declarations,
written before their own type has a name, need the tag.
*(1 mark)*

**What `static` on `gpio_write()` prevents, and what it does not.** *(1 mark)*

It gives the function internal linkage: no other translation unit can call it **by name**, and the
symbol is not emitted for the linker, so `gpio_write(&led1, true);` in `main.c` fails to link.

It does **not** hide the data. `gpio_t`'s fields are all still public: `led1.ddrx`, `led1.pin` and
`led1.write` are readable and writable by any file that includes the header. It does not even hide
the function itself in any meaningful sense — its *address* is deliberately published in
`led1.write` by `gpio_init()`, so a caller can call through it, copy it, or overwrite it with
something else entirely. Hiding functions and hiding fields are independent techniques; `static`
does only the first, and the opaque struct is what does the second.

### (c) 3 marks

**What happens: the program jumps to a garbage address on the very first call.** *(1.5 marks)*

`itf` is not the first member of `gpio_impl_t`. The cast `(gpio_interface_t*)(impl)` yields a pointer
to the *start* of the object, which is `ddrx`, not `itf`. Reading `self->vptr` therefore reads the
first bytes of `ddrx` — the address of a hardware register, `0x0024` for `DDRB` and so on — and
treats that number as a `const gpio_vtable_t*`. `self->vptr->toggle` then loads a function pointer
from inside the I/O register area and calls whatever value happens to be sitting there. On AVR that
lands in the middle of nothing and typically ends in a reset.

The initialisation was not skipped: `impl->itf.vptr = gpio_vptr_get_instance();` did run and did
store the correct table — at an offset nobody ever reads.

**The fix and the rule.** *(1 mark)*

```c
typedef struct gpio_impl
{
    gpio_interface_t itf; // Must be first.
    volatile uint8_t* ddrx;
    volatile uint8_t* portx;
    volatile uint8_t* pinx;
    uint8_t pin;
} gpio_impl_t;
```

C guarantees that a struct has no padding before its first member, so a pointer to the struct and a
pointer to its first member hold the same address. That is what makes the cast safe in both
directions without adjusting the pointer — and it is exactly the layout a C++ compiler generates for
single inheritance, where an object's address *is* its base class's address.

**Why the compiler said nothing.** *(0.5 marks)*

The conversion was written as an explicit cast, which is the programmer asserting that it is
intended. The compiler has no way to know `gpio_impl_t` was *meant* to begin with a
`gpio_interface_t`; it takes the assertion at face value and emits no diagnostic.

### (d) 2 marks

**One shared vtable.** The table contains nothing instance-specific — four function addresses,
identical for every `gpio_impl_t` ever created. All per-instance state lives in the concrete
struct's own fields, which each function reaches by casting `self` back. So a single
`static const gpio_vtable_t` inside the accessor is enough, and every instance's `itf.vptr` points
at that same address: one pointer per object rather than four. (`const` does not put the table in
flash on the ATmega328P: avr-gcc copies read-only data into RAM at startup unless it is marked
`PROGMEM`, so the single shared table costs its 8 bytes of RAM once, not once per instance.)
*(1 mark)*

**Why `del()` takes `gpio_interface_t**`.** So that it can `free()` the instance *and* null out the
caller's own pointer in a single call — the same pointer-to-pointer idiom as Question 1(b). Given
only a `gpio_interface_t*`, it could release the memory but would have to leave the caller holding
a dangling pointer, ready for a second `del()` or a use-after-free through that variable.
*(0.5 marks)*

**What a stub makes possible.** It satisfies the same interface with a `bool state` in place of
registers, so anything written against `gpio_interface_t*` can be exercised on a host with no chip
attached: unit tests, simulated inputs, or a third logging implementation dropped in without
touching a line of the code under test. With only `gpio_impl_t`, every run needs real hardware.
*(0.5 marks)*

---
