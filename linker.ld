/* Tell the linker what CPU we are targeting */
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)

/* ----- MEMORY MAP ------------------------------------------------------------
    Here we define the physical memory regions that exist on the RP2040.
    ORIGIN = where the region starts in the address space
    LENGTH = how big it is
    rx = readable, executable (Flash: you can read and run code from it)
    rwx = readable, writable, executable (RAM: full access)
    -------------------------------------------------------------------------- */
MEMORY 
{
    FLASH (rx)  : ORIGIN = 0x10000000, LENGTH = 2048k
    RAM (rwx) :ORIGIN = 20000000, LENGTH = 264k
}

/* -------- STACK SIZE -----------------------------------------------------------
    We explicitly reserve 8KB for the stack.
    No heap - all memory is statically allocated.
    In safety critical systems you size this based on worst case
    call depth analysis. 8KB is generous for our project.
    -----------------------------------------------------------------------------*/

_stack_size = 0x2000; /* 8KB */

/* -------- SECTIONS --------------------------------------------------------------
    SECTIONS tells the linker where to place each piece of the program.
    Each .name block is a section "section" - a logical grouping of related data.
    ------------------------------------------------------------------------------*/

SECTIONS
{
    /* ---- boot2 -----------------------------------------------------------------
        Must be the very first 256 bytes of Flash.
        The ROM bootloader reads exactly these 256 bytes, checks the CRC,
        the executes it. If this is not first, the chip will not boot.
    ------------------------------------------------------------------------------*/

    .boot2 : {
        *(.boot2)

    } > FLASH

    /* ---- Vector Table ------------------------------------------------------------
        Comes immediately after boot2.
        boot2 will set VTOR to point here and jump to our Reset_Handler
        The CPU reads this table whenever an exception or interrupt occurs.
    ---------------------------------------------------------------------------------*/

    .vectors : {
        *(.vectors)
    } > FLASH

    /* ---- Code and read-only data --------------------------------------------------
        .text = your compiled instructions
        .rodata = constants, string literals (read-only, live in Flash)
        The * means "from any input object file"
        The * after .text means "any subsection of .text"
        -----------------------------------------------------------------------------*/

    .text : {
        *(.text*)
        *(.rodata*)
    } > FLASH

    /* ---- Initialized data ---------------------------------------------------------
        Global variables with initial values: int x = 42;

        These need to be in RAM at runtime ( so they can be modified)
        but their initial values must be stored in Flash (non-volatile).

        The linker places the VALUES in Flash ( AT > FLASH)
        but reserves space for them in RAM ( > RAM).

        Startup code will copy them from Flash to RAM before main() runs.
        We expose symbols (_data_start, _data_end, _data_flash)
        so startup code knows what to copy and where.
        --------------------------------------------------------------------------------*/

    .data : {
        _data_start = .;    /* . means "current address in RAM" */
        *(.data*)
        _data_end = .;
    } > RAM AT > FLASH      /* lives in RAM, stored in Flash */

    _data_flash = LOADADDR(.data);  /* where the values sit in Flash */

    /* ---- Uninitialized data ----------------------------------------------------------
        Global variables with no initial value: int counter;
        C standard guarantees these are zero at program start.
        RAM powers on with random noise so startup code must zero this region.

        NOLOAD = do not take up sapce in the Flash binary
        We only need to know the address range to zero it - no values to store.
        ---------------------------------------------------------------------------------*/

    .bss(NOLOAD) : {
        _bss_start = .;
        *(.bss*)
        *(COMMON).    /* Uninitialized globals form C */
        _bss_end = .;
    } > RAM 

    /* ----- Stack ----------------------------------------------------------------------
        We place the stack at the TOP of RAM.
        The stack grows downward so we define _stack_top as the highest
        address - this is the initial SP value in our vector table.

        No heap sections exists - this is intentional.
        All allocations in this project are static.
        ----------------------------------------------------------------------------------*/

    .stack (NOLOAD) : {
        . = ALIGN(8);
        . += _stack_size;
    } > RAM

    _stack_top = ORIGIN(RAM) + LENGTH(RAM)
}
