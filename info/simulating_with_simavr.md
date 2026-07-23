# Running the Demos Without Hardware (simavr)
This course targets real ATmega328P hardware, and the worked demos are written to be flashed onto
an Arduino Uno/Nano. If you don't have a board yet, you can still run every demo in this repo
using [simavr](https://github.com/buserror/simavr), a cycle-accurate AVR simulator.

Everything described here has been verified against the demos in `lectures/`: the LED toggles, the
button interrupt fires, the debounce timer works, UART output appears, EEPROM survives a restart,
and starving the watchdog resets the MCU exactly as it would on silicon.

> **This is a fallback, not a replacement.** Simulation gets you unblocked, but it will not teach
> you what a floating input, a missing pull-up, or a badly seated jumper wire feels like. Get a
> board when you can.

---

## What You Get, and What You Don't
There are two ways to use simavr, and it's worth being clear about the difference up front.

| | Stock `simavr` CLI | With the harness below |
|---|---|---|
| Run the firmware | Yes | Yes |
| Debug with `avr-gdb` | Yes | Yes (`simavr -g`) |
| See UART output | **No** | Yes |
| Press a button | **No** | Yes |
| See the LED change | **No** | Yes |
| Persist EEPROM across restarts | **No** | Yes |

The stock `simavr` command-line tool has no way to display UART traffic and no way to drive an input
pin. It runs your firmware in complete silence. That is fine for a GDB session, but useless for the
LED-and-button demos.

To actually interact with the demos you need a small host-side program that links against
`libsimavr` and connects a simulated UART and a simulated button to the chip. That program is
provided in full below: a couple of hundred lines, over a third of it comments, and you only build
it once.

---

## Install
On Debian/Ubuntu:

```bash
sudo apt install -y \
    gcc-avr avr-libc avrdude \
    simavr libsimavr-dev libsimavrparts1 libelf-dev \
    gdb-avr build-essential
```

* **macOS:** `brew install simavr` and `brew install osx-cross/avr/avr-gcc`. The harness builds the
  same way; adjust include/library paths to your Homebrew prefix.
* **Windows:** use WSL and follow the Debian/Ubuntu instructions. The rest of this course assumes a
  Linux-like shell anyway.

---

## Build the Firmware
Nothing changes. The existing project makefiles already produce exactly what simavr wants:

```bash
make -C lectures/L09/gpio_driver_demo
```

This gives you `main.elf` and `main.hex`. **Prefer the `.elf`**; it carries symbols, so simavr can
report meaningful addresses and GDB can show you source.

Throughout this document, shell commands run from the **repository root** unless a `cd` says
otherwise. Watch for this when you open a second terminal: a new shell starts in your home
directory, not where the previous one was.

---

## Quick Path: Just Run It

```bash
simavr -m atmega328p -f 16000000 lectures/L09/gpio_driver_demo/main.elf
```

`-m` and `-f` are required. The demo firmwares don't embed a `.mmcu` ELF section (that would mean
including simavr's `avr_mcu_section.h` in the firmware source), so simavr can't infer the target or
the clock and you must tell it.

Be aware: this prints nothing. The firmware is running, but no UART is attached and no button
can be pressed. Press `Ctrl+C` to stop. To see anything happen, use the harness.

---

## Debugging With avr-gdb
This works with the stock CLI and is worth learning; it is by far the fastest way to understand
what a driver is doing.

The project makefiles compile with `-Os` only, so first rebuild with debug symbols:

```bash
# Terminal 1, from the repository root.
cd lectures/L09/gpio_driver_demo
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -g \
    -o main_dbg.elf main.c source/driver/*.c \
    -Wall -Iinclude -std=c11
```

Start simavr as a GDB server (it waits for a connection on port 1234):

```bash
simavr -g -m atmega328p -f 16000000 main_dbg.elf
```

In another terminal. Note the `cd`: the second terminal starts in your home directory, so without
it `avr-gdb` reports `main_dbg.elf: No such file or directory`.

```bash
# Terminal 2, from the repository root.
cd lectures/L09/gpio_driver_demo
avr-gdb main_dbg.elf
```

```gdb
(gdb) target remote :1234
(gdb) break main
(gdb) continue
(gdb) next
(gdb) info locals
(gdb) print led1
(gdb) backtrace
```

You get real source-level stepping, breakpoints, locals, and struct inspection, including things
like `print led1` showing `{ddrx = ..., portx = ..., pcicrx = 0, pin = 1}`.

Two pieces of harmless noise you may see on Ubuntu, so you don't waste time chasing them:
* `Python Exception <class 'ModuleNotFoundError'>: No module named 'gdb'`: Ubuntu's `avr-gdb`
  package ships without GDB's Python support files. Purely cosmetic.
* `warning: pc 0x800024 in address map, but not in symtab` when printing a `gpio_t`. Those pointers
  point into the AVR's I/O register space, which has no symbols. Also cosmetic; the values printed
  are correct.

---

## Full Path: The Simulation Harness
Save this as `simulator.c` somewhere outside the repo (a scratch directory is fine; it is host
code, not firmware, and does not belong in `lectures/`).

```c
/**
 * @file Host-side simavr harness for the Embedded C course demos.
 *
 *       Connects a simulated UART and a simulated button to an ATmega328P firmware image, reports
 *       LED state changes, and persists EEPROM contents across runs.
 *
 * Build:
 *   gcc -O2 -o simulator simulator.c -I/usr/include/simavr \
 *       -lsimavr -lsimavrparts -lelf -lpthread -lm
 *
 * Usage:
 *   ./simulator [--led B1] [--button B5] [--eeprom eeprom.bin] <firmware.elf>
 */
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "avr_eeprom.h"
#include "avr_ioport.h"
#include "parts/button.h"
#include "parts/uart_pty.h"
#include "sim_avr.h"
#include "sim_elf.h"

/** Size of the ATmega328P EEPROM in bytes. */
#define EEPROM_SIZE 1024U

/** Button hold time in microseconds, longer than the 300 ms firmware debounce. */
#define BUTTON_HOLD_US 500000U

/** Simulated MCU instance. */
static avr_t* avr = NULL;

/** Simulated UART, bridged to a host pseudo-terminal. */
static uart_pty_t uart = {0};

/** Simulated button. */
static button_t button = {0};

/** File used to persist EEPROM contents between runs. */
static const char* eeprom_path = "eeprom.bin";

/** Port and pin the LED is connected to, defaults to PB1 (Arduino D9). */
static char led_port = 'B';
static int led_pin   = 1;

/** Port and pin the button is connected to, defaults to PB5 (Arduino D13). */
static char button_port = 'B';
static int button_pin   = 5;

/** Set from the signal handler to break out of the run loop. */
static volatile sig_atomic_t stop = 0;

/**
 * @brief Write the simulated EEPROM contents to disk.
 */
static void eeprom_save(void)
{
    uint8_t buf[EEPROM_SIZE];
    avr_eeprom_desc_t desc = {.ee = buf, .offset = 0U, .size = EEPROM_SIZE};
    avr_ioctl(avr, AVR_IOCTL_EEPROM_GET, &desc);

    FILE* file = fopen(eeprom_path, "wb");
    if (NULL == file) { return; }
    fwrite(buf, 1U, EEPROM_SIZE, file);
    fclose(file);
    printf("[sim] EEPROM saved to %s\n", eeprom_path);
}

/**
 * @brief Restore the simulated EEPROM contents from disk, if a saved image exists.
 */
static void eeprom_restore(void)
{
    uint8_t buf[EEPROM_SIZE];
    FILE* file = fopen(eeprom_path, "rb");
    if (NULL == file) { return; }

    if (EEPROM_SIZE == fread(buf, 1U, EEPROM_SIZE, file))
    {
        avr_eeprom_desc_t desc = {.ee = buf, .offset = 0U, .size = EEPROM_SIZE};
        avr_ioctl(avr, AVR_IOCTL_EEPROM_SET, &desc);
        printf("[sim] EEPROM restored from %s\n", eeprom_path);
    }
    fclose(file);
}

/**
 * @brief Request termination on Ctrl+C.
 *
 *        Only sets a flag, so that the EEPROM is saved from the main loop rather than from this
 *        handler, where file I/O would not be async-signal-safe.
 *
 * @param[in] signum The received signal, unused.
 */
static void on_terminate(int signum)
{
    (void)(signum);
    stop = 1;
}

/**
 * @brief Report a change of the LED pin.
 *
 * @param[in] irq The IRQ that fired, unused.
 * @param[in] value The new pin state.
 * @param[in] param User data, unused.
 */
static void on_led_changed(struct avr_irq_t* irq, uint32_t value, void* param)
{
    (void)(irq);
    (void)(param);
    printf("[LED] %s\n", value ? "ON" : "OFF");
    fflush(stdout);
}

/**
 * @brief Press the simulated button whenever the user hits Enter.
 *
 * @param[in] arg Thread argument, unused.
 *
 * @return NULL on termination of stdin.
 */
static void* button_thread(void* arg)
{
    (void)(arg);
    char line[64] = {0};

    while (NULL != fgets(line, sizeof(line), stdin))
    {
        printf("[BTN] press\n");
        fflush(stdout);
        button_press(&button, BUTTON_HOLD_US);
    }
    return NULL;
}

/**
 * @brief Application entry point.
 *
 * @param[in] argc Number of command line arguments.
 * @param[in] argv Command line arguments.
 *
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char** argv)
{
    const char* firmware = NULL;

    // Parse command line arguments.
    for (int i = 1; i < argc; ++i)
    {
        if ((0 == strcmp(argv[i], "--led")) && (i + 1 < argc))
        {
            led_port = argv[++i][0];
            led_pin  = atoi(argv[i] + 1);
        }
        else if ((0 == strcmp(argv[i], "--button")) && (i + 1 < argc))
        {
            button_port = argv[++i][0];
            button_pin  = atoi(argv[i] + 1);
        }
        else if ((0 == strcmp(argv[i], "--eeprom")) && (i + 1 < argc)) { eeprom_path = argv[++i]; }
        else { firmware = argv[i]; }
    }
    if (NULL == firmware)
    {
        fprintf(stderr, "usage: %s [--led B1] [--button B5] [--eeprom f] <firmware.elf>\n", argv[0]);
        return 1;
    }

    // Read the firmware image, defaulting to an ATmega328P at 16 MHz.
    elf_firmware_t f = {{0}};
    if (0 > elf_read_firmware(firmware, &f))
    {
        fprintf(stderr, "error: cannot read %s\n", firmware);
        return 1;
    }
    if ('\0' == f.mmcu[0]) { strcpy(f.mmcu, "atmega328p"); }
    if (0U == f.frequency) { f.frequency = 16000000U; }

    // Create and initialize the simulated MCU.
    avr = avr_make_mcu_by_name(f.mmcu);
    if (NULL == avr)
    {
        fprintf(stderr, "error: unknown MCU %s\n", f.mmcu);
        return 1;
    }
    avr_init(avr);
    avr_load_firmware(avr, &f);

    // Restore EEPROM contents, and save them again on exit.
    eeprom_restore();
    signal(SIGINT, on_terminate);
    signal(SIGTERM, on_terminate);

    // Bridge UART0 to a host pseudo-terminal.
    uart_pty_init(avr, &uart);
    uart_pty_connect(&uart, '0');

    // Connect the simulated button to the input pin.
    button_init(avr, &button, "button");
    avr_connect_irq(button.irq + IRQ_BUTTON_OUT,
                    avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ(button_port), button_pin));

    // Report every change of the LED pin.
    avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ(led_port), led_pin),
                            on_led_changed, NULL);

    // Press the button whenever the user hits Enter.
    pthread_t thread;
    pthread_create(&thread, NULL, button_thread, NULL);

    printf("[sim] running %s (%s @ %u Hz), press Enter to push the button\n", firmware, f.mmcu,
           f.frequency);
    fflush(stdout);

    // Run until the firmware stops or crashes, or the user interrupts us.
    while (0 == stop)
    {
        const int state = avr_run(avr);
        if ((cpu_Done == state) || (cpu_Crashed == state))
        {
            printf("[sim] CPU stopped, state %d\n", state);
            break;
        }
    }
    eeprom_save();
    return 0;
}
```

Build it once, then remember where it landed:

```bash
gcc -O2 -o simulator simulator.c -I/usr/include/simavr \
    -lsimavr -lsimavrparts -lelf -lpthread -lm
export SIM="$PWD/simulator"
```

`$SIM` is used for the rest of this document. Because it holds an absolute path, it works from any
directory, which matters once you start `cd`-ing into demo directories. Add that `export` line to
your `~/.bashrc` so new terminals inherit it.

Run a demo from its own directory, so that `main.elf` resolves and the EEPROM image is kept
alongside the firmware it belongs to. That image is written as `eeprom.bin` in the demo directory;
`.gitignore` already excludes it, so it will not show up as an untracked file:

```bash
# From the repository root.
cd lectures/L09/gpio_driver_demo
$SIM main.elf
```

You'll see something like:

```text
[sim] EEPROM restored from eeprom.bin
uart_pty_init bridge on port *** /dev/pts/14 ***
uart_pty_connect: /tmp/simavr-uart0 now points to /dev/pts/14
[sim] running .../main.elf (atmega328p @ 16000000 Hz), press Enter to push the button
[LED] OFF
```

Open a **second terminal** to read the UART:

```bash
cat /tmp/simavr-uart0
# or, for a proper terminal:
picocom /tmp/simavr-uart0
```

Now press **Enter** in the simulator terminal. The button is pushed, and you should see:

```text
[BTN] press
[LED] ON
```

and on the UART terminal:

```text
LED1 enabled!
```

Press `Ctrl+C` to stop. The EEPROM image is written to `eeprom.bin`; start the simulator again and
the demo picks up where it left off — which is exactly the point of the L07/L08/L09 EEPROM lesson.

---

## Demo Reference
Every demo uses the same wiring: **LED on `PB1` (Arduino D9)** and **button on `PB5` (Arduino
D13)**; with one exception, noted below.

`cd` into the demo directory under `lectures/`, then run the command below:

| Demo | Command | UART? | Notes |
|---|---|---|---|
| `L06/serial_driver_demo` | `$SIM main.elf` | Yes | Press Enter, watch the state report |
| `L07/eeprom_driver_demo` | `$SIM main.elf` | Yes | State survives restart |
| `L08/watchdog_driver_demo` | `$SIM main.elf` | Yes | See the watchdog experiment below |
| `L09/gpio_driver_demo` | `$SIM main.elf` | Yes | Full struct-as-driver demo |
| `L10/encapsulation_demo` | `$SIM main.elf` | No | No serial driver in this one |
| `L10/function_pointer_demo` | `$SIM main.elf` | Yes | |
| `L10/interface_demo` | `$SIM --led B0 main.elf` | No | LED is on `PB0` (D8); its button is a software stub, so it toggles on its own with no input |

### Watching the Watchdog Bite
This is the one experiment in the course that is genuinely easier to run in simulation than on the
board: there is no reflashing between the two versions and no serial monitor to wire up, so the
before-and-after is seconds apart instead of minutes. It is referenced from
[L08 Appendix A.5](../lectures/L08/appendix/a_watchdog.md#a5-worked-example-petting-the-watchdog-in-l07s-demo).

Take a scratch copy of the demo outside the repo, comment out the `watchdog_reset()` call in the
main loop, and rebuild:

```bash
cp -r lectures/L08/watchdog_driver_demo /tmp/wdt && cd /tmp/wdt
rm -f eeprom.bin   # cp brings along any saved EEPROM image; start from a blank chip
sed -i 's|watchdog_reset();|// watchdog_reset();|' main.c
make clean && make
$SIM main.elf
```

**Use `make clean && make`, not bare `make`.** `cp -r` copies the prebuilt `main.elf` along with the
sources, and the copied timestamps can leave `make` thinking the binary is current. You then run the
old firmware and see no difference, which looks exactly like "the watchdog does nothing in the
simulator". Check `md5sum main.elf` before and after if you are unsure.

Watch the **UART**. In a second terminal, `cat /tmp/simavr-uart0`:

```text
LED1 disabled!LED1 disabled!LED1 disabled!LED1 disabled!LED1 disabled!...
```

That is `setup()`'s state report running again on every boot: the starved watchdog is resetting the
MCU over and over. Restore the `watchdog_reset()` call, rebuild, and you get exactly one report and
then silence. Measured over six seconds on this demo: **11 reports starved, 1 report when pet.**

Commenting out `watchdog_init()` as well is a useful third data point, and gives 1 report again: the
reboot loop really is the watchdog firing, not the demo restarting for some other reason.

(Which message repeats depends on the stored state, so it reads `LED1 enabled!` if the EEPROM image
says the LED was on. The repetition is the signal, not the wording.)

The LED is a weaker indicator than the UART here. On each reboot the firmware restores LED1 from
EEPROM, so you only see it flicker if `eeprom.bin` holds the "on" state from a previous session.

---

## Gotchas
**A short button press does nothing.** This one is real firmware behaviour, not a simulator
artifact, and it is worth understanding. The button is wired with a pull-up, so the pin reads *high*
when released. The pin change ISR checks `gpio_read(&btn1)` and only toggles the LED when the pin is
high — that is, on **release**. But the press edge also disables pin change interrupts for 300 ms of
debounce. So if you release the button inside that 300 ms window, the release edge is never seen and
nothing happens. The harness holds the button for 500 ms for exactly this reason.

**Mashing Enter looks like a hang, but isn't.** Press quickly and you will see `[BTN] press` lines
with no `[LED]` change, as though the simulator has wedged. Two things compound. The 300 ms debounce
above hides any press inside that window. And each `button_press()` schedules its release 500 ms
later, so pressing again beforehand simply re-arms the hold: a burst of presses becomes one long
press, not several. A measured burst of 20 presses 30 ms apart produced exactly one LED toggle. Stop
pressing for a second and it responds normally again; nothing needs restarting.

**Simulated time is not wall-clock time.** simavr runs as fast as your host allows and is not
throttled to real time. A 300 ms debounce is 300 ms of *simulated* time. Delays will feel wrong.

**EEPROM starts blank unless you have an image.** Delete `eeprom.bin` (or pass a different
`--eeprom` path) to get a factory-fresh chip.

**Use the `.elf`, not the `.hex`.** The `.hex` has no symbols, so GDB is nearly useless with it and
simavr can't report symbolic addresses.

---

## Limits
The harness above covers digital I/O, interrupts, timers, UART, EEPROM and the watchdog — which is
everything the committed demos need. It does not cover:
* **ADC input** (L05's potentiometer). simavr simulates the ADC, but feeding it a value requires
  raising the ADC IRQ from the harness. Adding that is a good exercise if you need it.
* **Timing realism.** Nothing here tells you whether your code is fast enough.
* **Anything physical.** Contact bounce, brown-outs, noise, and wiring mistakes are precisely the
  problems this course exists to teach you about, and a simulator has none of them.

Use simavr to keep moving. Get a board when you can.

---
