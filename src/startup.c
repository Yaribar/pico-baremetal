#include <stdint.h> // Standard integer types

/* Symbols from the linker script ---------------------------------------------
    These are NOT variables. They are addresses the linker calculated.
    We declared them as extern so the C compiler knows they exist somewhere.
    The & operator is how you get their actual address value in C.
    
    Notice we declare them as uint32_t (32-bit unsigned).
    ARM is a 32-bit architecture - we copy memory word by word (bytes at a time) 
    which is the natural and most efficient unit on this CPU.
    ---------------------------------------------------------------------------*/

extern uint32_t _data_start;    /* where .data begins in RAM */
extern uint32_t _data_end;      /* where .data ends in RAM.  */
extern uint32_t _data_flash;    /* where .data initial values sit in Flash */
extern uint32_t _bss_start;     /* where .bss begins in RAM*/
extern uint32_t _bss_end;       /* where .bss ends in RAM */

/* --- Forward declaration ----------------------------------------------------*/
extern int main(void);

/* Reset_Handler ---------------------------------------------------------------
    This is the entry point of your application.
    boot2 reads its address from your vector table and jumps here.
    By the time this runs, SP is already se to _stack_top
    (boot2 loaded that from your vector table too).

    This function NEVER returns. It calls main() which loops forever.
    If main() somehow returns, we trap in an infinite loop - because there
    is nothing to return to. No OS, no runtime, nothing.
    ----------------------------------------------------------------------------*/
void Reset_Handler(void) {
    /* Problem: global variables with initial values ( int x = 42) have their 
    initial values stored in Flash (non-volatile, survives power off).
    But they need to live in RAM so your code can modify them at runtime.
    
    Solution: copy them from Flash to RAM right now, before main() runs.
    
    sr  starts at where the values sit in Flash.
    dst starts at where they should live in RAM.
    We copy word by word until dst reaches the end of the .data region.
    ----------------------------------------------------------------------------*/

uint32_t *src = &_data_flash;
uint32_t *dst = &_data_start;

while (dst < &_data_end){
    *dst++ = *src++;
}

/* ----- Step2: Zero out .bss ---------------------------------------------------
Problem: global variables with no initial value (int counter) must be zero at
program start - the C standard guarantees this. But RAM powers on with random
noise ( whatever charge was left in the capacitors ).

Solution: walk through the entire .bss region and write zero to every word.
After this loop, all your uninitialized globals are guaranteed zero.
----------------------------------------------------------------------------------*/

dst = &_bss_start;

while (dst < &_bss_end){
    *dst++ = 0;
}

/* ---- Step 3: Call main --------------------------------------------------------
    RAM is now in a valid state. .data has correct initial values.
    .bss is zeroed. Stack is ready (SP was set by boot2).
    The C environment is fully initialized. We can safely run C code.
    ------------------------------------------------------------------------------*/

main();

/* ---- Trap ----------------------------------------------------------------------
    main() should never return in bare metal - it contains an infinite loop.
    But if it somehow does, we catch it here rather than letting the CPU execute
    whatever random bytes come after this function in Flash.
    -------------------------------------------------------------------------------*/
while(1);

}
