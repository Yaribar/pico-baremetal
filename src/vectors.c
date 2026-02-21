#include <stdint.h>

/* --- Symbols we need from other files -------------------------------------
    _stack_top comes from the linker script - the address of the top of RAM.
    This is the very first entry in the vector table (initial SP value).
    Reset_Handler comes from startup.c - the first code that runs.
    -------------------------------------------------------------------------*/

extern uint32_t _stack_top;
extern void     Reset_Handler(void);

/* --- Default Handler ------------------------------------------------------
    Any interrupt or exception we have not specifically handled will land here.
    It spins forever - which is intentional. In a real system this would trigger
    a watchdog reset or log an error. For now it makes bugs visible: if your
    program freezes, an unhandled interrupt is a likely cause.
    -------------------------------------------------------------------------*/

void Default_Handler(void) {
    while(1);
}

/* --- Weak aliases ---------------------------------------------------------
    This is a GCC feature worth understanding deeply for your interview.

    __attribute__((weak)) means: use this definition UNLESS another file
    defines teh same function, in which case use that one instead.
    
    __attribute__((alias("Defualt_Handler"))) means: this function name
    is just another name for Default_Handler.
    
    Combined effect: every handler below defaults to Default_Handler,
    but if you define e.g. void SysTick_Handler(void) in main.c,
    the linker automatically uses YOUR version instead.

    This is how every professional embedded SDK works - you only write
    the handlers you actually need, everything else is handled safely.
    ------------------------------------------------------------------------*/

void NMI_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)   __attribute__((weak, alias("Default_Handler")));

/* RP2040 external interrupts (IRQ0 - IRQ25) */
void TIMER_IRQ0_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void TIMER_IRQ1_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void TIMER_IRQ2_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void TIMER_IRQ3_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void PWM_IRQ_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void USBCTRL_IRQ_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PIO0_IRQ0_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PIO0_IRQ1_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PIO1_IRQ0_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PIO1_IRQ1_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA_IRQ0_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void DMA_IRQ1_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void IO_IRQ_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void SPI0_IRQ_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQ_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void UART0_IRQ_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UART1_IRQ_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void ADC_IRQ_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void I2C0_IRQ_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void I2C1_IRQ_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void RTC_IRQ_Handler(void)     __attribute__((weak, alias("Default_Handler")));

/* --- The vector table ------------------------------------------------------
    This is plain C array of 32-bit values placed in the .vectors section.
    The linker puts .vectors immediately after boot2 in Flash at 0x10000100.
    boot2 sets VTOR to 0x10000100 and loads SP from every entry 0 and jumps to 
    entry 1.

    Every entry is a function pointer cast to uint32_t - because the CPU 
    reads raw 32-bit values from this table, not typed functions pointers.

    On cortex-M0+ the layout is fixed by the ARM architecture spec:
    entries 0-15 are system exceptions, entries 16+ are external IRQs.
    ---------------------------------------------------------------------------*/

__attribute__((section(".vectors")))
const uint32_t vector_table[] = {
    /* Entry 0 - Initial Stack pointer value
        Not a function pointer - the CPU loads this directly into SP register
        _stack_top is the top of RAM (0X20042000) defined in linker.ld.      */
    (uint32_t)&_stack_top,

    /* System exceptions — entries 1 to 15 */
    (uint32_t)&Reset_Handler,      /*  1 - Reset                            */
    (uint32_t)&NMI_Handler,        /*  2 - Non Maskable Interrupt            */
    (uint32_t)&HardFault_Handler,  /*  3 - Hard Fault                       */
    0,                             /*  4 - Reserved (no MemManage on M0+)   */
    0,                             /*  5 - Reserved (no BusFault on M0+)    */
    0,                             /*  6 - Reserved (no UsageFault on M0+)  */
    0,                             /*  7 - Reserved                         */
    0,                             /*  8 - Reserved                         */
    0,                             /*  9 - Reserved                         */
    0,                             /* 10 - Reserved                         */
    (uint32_t)&SVC_Handler,        /* 11 - Supervisor Call                  */
    0,                             /* 12 - Reserved                         */
    0,                             /* 13 - Reserved                         */
    (uint32_t)&PendSV_Handler,     /* 14 - Pendable Service Call            */
    (uint32_t)&SysTick_Handler,    /* 15 - System Tick Timer                */

    /* External interrupts — entries 16+ (IRQ0 onwards) */
    (uint32_t)&TIMER_IRQ0_Handler, /* 16 - IRQ0  Timer 0                   */
    (uint32_t)&TIMER_IRQ1_Handler, /* 17 - IRQ1  Timer 1                   */
    (uint32_t)&TIMER_IRQ2_Handler, /* 18 - IRQ2  Timer 2                   */
    (uint32_t)&TIMER_IRQ3_Handler, /* 19 - IRQ3  Timer 3                   */
    (uint32_t)&PWM_IRQ_Handler,    /* 20 - IRQ4  PWM                       */
    (uint32_t)&USBCTRL_IRQ_Handler,/* 21 - IRQ5  USB                       */
    0,                             /* 22 - IRQ6  XIP                       */
    (uint32_t)&PIO0_IRQ0_Handler,  /* 23 - IRQ7  PIO0 IRQ0                 */
    (uint32_t)&PIO0_IRQ1_Handler,  /* 24 - IRQ8  PIO0 IRQ1                 */
    (uint32_t)&PIO1_IRQ0_Handler,  /* 25 - IRQ9  PIO1 IRQ0                 */
    (uint32_t)&PIO1_IRQ1_Handler,  /* 26 - IRQ10 PIO1 IRQ1                 */
    (uint32_t)&DMA_IRQ0_Handler,   /* 27 - IRQ11 DMA 0                     */
    (uint32_t)&DMA_IRQ1_Handler,   /* 28 - IRQ12 DMA 1                     */
    (uint32_t)&IO_IRQ_Handler,     /* 29 - IRQ13 GPIO                      */
    0,                             /* 30 - IRQ14 QSPI                      */
    0,                             /* 31 - IRQ15 SIO PROC0                 */
    0,                             /* 32 - IRQ16 SIO PROC1                 */
    0,                             /* 33 - IRQ17 Clocks                    */
    (uint32_t)&SPI0_IRQ_Handler,   /* 34 - IRQ18 SPI0                      */
    (uint32_t)&SPI1_IRQ_Handler,   /* 35 - IRQ19 SPI1                      */
    (uint32_t)&UART0_IRQ_Handler,  /* 36 - IRQ20 UART0                     */
    (uint32_t)&UART1_IRQ_Handler,  /* 37 - IRQ21 UART1                     */
    (uint32_t)&ADC_IRQ_Handler,    /* 38 - IRQ22 ADC                       */
    (uint32_t)&I2C0_IRQ_Handler,   /* 39 - IRQ23 I2C0                      */
    (uint32_t)&I2C1_IRQ_Handler,   /* 40 - IRQ24 I2C1                      */
    (uint32_t)&RTC_IRQ_Handler,    /* 41 - IRQ25 RTC                       */
};
