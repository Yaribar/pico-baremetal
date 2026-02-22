#include <stdint.h>

/* ── Why volatile on every hardware register ──────────────────────────────────
   The compiler optimises C code by caching values in registers and reordering
   reads and writes. That is fine for normal variables.
   
   But hardware registers are different — reading or writing them triggers
   physical side effects the compiler cannot see. A write to GPIO_OUT_XOR
   toggles a real pin. A read of RESETS_RESET_DONE checks real hardware state.
   
   Without volatile the compiler might:
   - Skip a write because "nobody reads that variable"
   - Hoist a read out of a loop because "the value never changes in C"
   - Reorder writes because "order doesn't matter for unrelated variables"
   
   volatile forces the compiler to emit every read and write exactly where
   you wrote it, in order, with no caching. Non-negotiable for MMIO.
   ────────────────────────────────────────────────────────────────────────── */
#define MMIO32(addr)   (*((volatile uint32_t*)(addr)))

/* ── RESETS ──────────────────────────────────────────────────────────────────
   On the RP2040, every peripheral starts held in reset after power-on.
   You must explicitly release a peripheral from reset before using it.
   
   The RP2040 has a clever feature: every register has three extra aliases
   at +0x1000 (XOR), +0x2000 (atomic SET), +0x3000 (atomic CLR).
   Writing to the CLR alias clears only the bits you specify — no
   read-modify-write needed, which avoids race conditions with interrupts.
   ────────────────────────────────────────────────────────────────────────── */
#define RESETS_BASE         0x4000C000u
#define RESETS_RESET        MMIO32(RESETS_BASE + 0x000)  /* reset control    */
#define RESETS_RESET_CLR    MMIO32(RESETS_BASE + 0x3000) /* atomic clear     */
#define RESETS_RESET_DONE   MMIO32(RESETS_BASE + 0x008)  /* reset status     */

#define RESET_IO_BANK0      (1u << 5)
#define RESET_PADS_BANK0    (1u << 8)

/* ── PADS_BANK0 ──────────────────────────────────────────────────────────────
   Controls the electrical properties of each GPIO pin:
   drive strength, pull-up/pull-down, slew rate, input enable.
   We need to unreset this block before configuring any GPIO.
   ────────────────────────────────────────────────────────────────────────── */
#define PADS_BANK0_BASE     0x4001C000u
#define PADS_GPIO25         MMIO32(PADS_BANK0_BASE + 0x068)

/* ── IO_BANK0 ────────────────────────────────────────────────────────────────
   Controls which function each GPIO pin serves.
   Every GPIO can be SPI, UART, I2C, PWM, or plain GPIO (SIO) among others.
   We configure GPIO 25 to function 5 = SIO (plain GPIO under software control).
   
   Each GPIO has two registers: STATUS (read-only) and CTRL (read-write).
   They are 8 bytes apart: GPIO_N_CTRL = IO_BANK0_BASE + N*8 + 4
   GPIO 25 CTRL = 0x40014000 + 25*8 + 4 = 0x400140CC
   ────────────────────────────────────────────────────────────────────────── */
#define IO_BANK0_BASE       0x40014000u
#define GPIO25_CTRL         MMIO32(IO_BANK0_BASE + 0x0CC)

#define FUNCSEL_SIO         5u    /* GPIO controlled by software via SIO     */

/* ── SIO (Single-cycle I/O) ──────────────────────────────────────────────────
   The SIO block gives you direct, single-cycle control over GPIO pins.
   Unlike most peripherals it is not in the APB peripheral region —
   it lives at 0xD0000000, directly accessible to each core with no bus
   contention. It is the fastest way to toggle a pin.
   
   OUT_XOR is particularly elegant: writing a bitmask XORs those bits with
   the current output level. Writing (1 << 25) toggles GPIO 25 every time.
   ────────────────────────────────────────────────────────────────────────── */
#define SIO_BASE            0xD0000000u
#define GPIO_OUT_XOR        MMIO32(SIO_BASE + 0x01C)   /* toggle pins        */
#define GPIO_OE_SET         MMIO32(SIO_BASE + 0x024)   /* set as output      */

/* ── SysTick ─────────────────────────────────────────────────────────────────
   SysTick is a 24-bit countdown timer built into every Cortex-M core.
   It is identical on every Cortex-M chip — learning it here transfers
   directly to STM32, NXP, Nordic, and every other Cortex-M product.
   
   RVR: Reload Value Register — when counter hits 0, it reloads this value
   CVR: Current Value Register — writing any value resets the counter
   CSR: Control and Status Register
        bit 0 (ENABLE)    — start/stop the timer
        bit 1 (TICKINT)   — fire SysTick_Handler when counter hits 0
        bit 2 (CLKSOURCE) — 1 = use processor clock, 0 = external ref clock
   ────────────────────────────────────────────────────────────────────────── */
#define SYST_CSR            MMIO32(0xE000E010)
#define SYST_RVR            MMIO32(0xE000E014)
#define SYST_CVR            MMIO32(0xE000E018)

/* ── LED ─────────────────────────────────────────────────────────────────── */
#define LED_PIN             25u
#define LED_MASK            (1u << LED_PIN)

/* ── Shared state between ISR and main ───────────────────────────────────────
   volatile is required here for the same reason as MMIO:
   the compiler cannot see that SysTick_Handler modifies this variable
   asynchronously. Without volatile it may cache tick_count in a register
   inside main()'s loop and never see the ISR's updates.
   ────────────────────────────────────────────────────────────────────────── */
static volatile uint32_t tick_count = 0;

/* ── SysTick_Handler ─────────────────────────────────────────────────────────
   This is a plain C function. Its name matches the weak alias in vectors.c
   so the linker automatically places its address in the vector table at
   entry 15 (VTOR + 0x3C).
   
   When SysTick counts to zero, the CPU saves 8 registers, reads VTOR+0x3C,
   and jumps here. No special syntax required — the vector table wiring we
   built earlier handles everything.
   ────────────────────────────────────────────────────────────────────────── */
void SysTick_Handler(void) {
    tick_count++;

    /* Toggle LED every 500 ticks — at 1kHz SysTick this is 500ms (0.5Hz blink)
       XOR the LED pin bit: if it was 1 it becomes 0, if 0 it becomes 1      */
    if ((tick_count % 500) == 0) {
        GPIO_OUT_XOR = LED_MASK;
    }
}

/* ── Peripheral initialisation ───────────────────────────────────────────── */
static void resets_init(void) {
    /* Release IO_BANK0 and PADS_BANK0 from reset atomically                  */
    RESETS_RESET_CLR = RESET_IO_BANK0 | RESET_PADS_BANK0;

    /* Spin until hardware confirms both are out of reset
       This is a read of a hardware status register — volatile is essential   */
    while ((RESETS_RESET_DONE & (RESET_IO_BANK0 | RESET_PADS_BANK0))
                              != (RESET_IO_BANK0 | RESET_PADS_BANK0));
}

static void gpio_init(void) {
    /* Connect GPIO 25 to the SIO block (plain software-controlled GPIO)      */
    GPIO25_CTRL = FUNCSEL_SIO;

    /* Enable GPIO 25 as an output
       The SIO OE_SET register only raises bits — never lowers others
       so we do not need a read-modify-write                                   */
    GPIO_OE_SET = LED_MASK;
    GPIO_OUT_XOR = LED_MASK;
}

static void systick_init(void) {
    /* RP2040 boots using the Ring Oscillator (~6 MHz).
       6000 ticks at 6MHz = 1ms per SysTick interrupt → 1kHz tick rate
       This gives us millisecond resolution for timing.
       
       In a production system you would configure the crystal oscillator
       and PLL first, then derive this value from a known clock frequency.
       For now ROSC is good enough to make the LED blink visibly.             */
    const uint32_t ticks_per_ms = 6000;

    SYST_RVR = ticks_per_ms - 1;   /* reload value (24-bit max: 16,777,215)  */
    SYST_CVR = 0;                   /* reset current count before starting    */
    SYST_CSR = (1u << 2)            /* CLKSOURCE: processor clock             */
             | (1u << 1)            /* TICKINT:   fire interrupt on zero       */
             | (1u << 0);           /* ENABLE:    start the timer              */
}

/* ── main ────────────────────────────────────────────────────────────────── */
int main(void) {
    resets_init();
    gpio_init();
    systick_init();

    /* main() does nothing — all work happens in SysTick_Handler.
       In a real application you would check flags set by ISRs here,
       process data, manage state machines. The ISR only sets flags
       and does minimal work — main() does the heavy lifting.
       This pattern is called "deferred processing".                          */
    while (1) {
        /* low power sleep — wake on next interrupt
           saves power and makes interrupt latency more predictable           */
        //__asm volatile ("wfi");
    }
}